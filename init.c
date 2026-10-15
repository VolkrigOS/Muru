/*
 *	main init structure
 *	see LICENSE for more licensing details.
 */

#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "muru.h"

#define TTYS_MAX 16
#define TTYS_PATH "/etc/ttys"
#define RC_PATH "/etc/rc"
#define RC_SHUT "/etc/rc.shutdown"
#define BAUD "38400"

#define S_NONE 0
#define S_POWEROFF 1
#define S_REBOOT 2
#define S_RESCUE 3

typedef struct
{
    char tty[64];
    char cmd[256];
    char term[64];
    pid_t pid;
} Tty;

static volatile sig_atomic_t g_sig = S_NONE;
static Tty g_ttys[TTYS_MAX];
static int g_ntty = 0;

static void on_sig(int s)
{
    if (s == SIGUSR2 || s == SIGTERM)
        g_sig = S_POWEROFF;
    else if (s == SIGINT)
        g_sig = S_REBOOT;
}

static void setup_sigs(void)
{
    struct sigaction sa;
    sa.sa_handler = on_sig;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
}

static void mount_dev(void)
{
    mkdir("/dev", 0755);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
}

static void init_console(void)
{
    int fd = open("/dev/console", O_RDWR);

    if (fd < 0)
        return;

    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    if (fd > STDERR_FILENO)
        close(fd);
}

static void wait_for(pid_t pid)
{
    int st;
    while (waitpid(pid, &st, 0) < 0 && errno == EINTR)
        ;
}

static int parse_ttys(void)
{
    FILE *fp;
    char line[512];
    int n;
    n = 0;
    fp = fopen(TTYS_PATH, "r");
    if (!fp)
        return 0;
    while (fgets(line, (int)sizeof(line), fp) && n < TTYS_MAX)
    {
        char tty[64], cmd[256], term[64], onoff[8];
        char *p, *q;
        p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '#' || *p == '\n' || *p == '\0')
            continue;
        if (sscanf(p, "%63s", tty) != 1)
            continue;
        while (*p && *p != ' ' && *p != '\t')
            p++;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '"')
        {
            int i;
            p++;
            q = cmd;
            i = 0;
            while (*p && *p != '"' && i < 255)
            {
                *q++ = *p++;
                i++;
            }
            *q = '\0';
            if (*p == '"')
                p++;
        }
        else
        {
            if (sscanf(p, "%255s", cmd) != 1)
                continue;
            while (*p && *p != ' ' && *p != '\t')
                p++;
        }
        while (*p == ' ' || *p == '\t')
            p++;
        if (sscanf(p, "%63s %7s", term, onoff) != 2)
            continue;
        if (strcmp(onoff, "on") != 0)
            continue;
        strncpy(g_ttys[n].tty, tty, 63);
        g_ttys[n].tty[63] = '\0';
        strncpy(g_ttys[n].cmd, cmd, 255);
        g_ttys[n].cmd[255] = '\0';
        strncpy(g_ttys[n].term, term, 63);
        g_ttys[n].term[63] = '\0';
        g_ttys[n].pid = -1;
        n++;
    }
    fclose(fp);
    return n;
}

static pid_t start_getty(int i)
{
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        setsid();
        execl(g_ttys[i].cmd, g_ttys[i].cmd,
              BAUD, g_ttys[i].tty, g_ttys[i].term, (char *)NULL);
        _exit(1);
    }
    return pid;
}

static void spawn_gettys(void)
{
    int i;
    for (i = 0; i < g_ntty; i++)
        g_ttys[i].pid = start_getty(i);
}


static void run_rc(void)
{
    pid_t pid = fork();

    if (pid == 0) {
        if (access("/bin/sh", X_OK) != 0) {
            char buf[64];
            snprintf(buf, sizeof(buf),
                     "/bin/sh errno=%d", errno);
            xwrite(MURU_ERROR, buf);
            _exit(127);
        }

        xwrite(MURU_OK, "/bin/sh encontrado");

        if (access(RC_PATH, R_OK) != 0) {
            char buf[64];
            snprintf(buf, sizeof(buf),
                     "/etc/rc errno=%d", errno);
            xwrite(MURU_ERROR, buf);
            _exit(127);
        }

        xwrite(MURU_OK, "/etc/rc encontrado");

        execl("/bin/sh", "/bin/sh", RC_PATH, (char *)NULL);

        {
            char buf[64];
            snprintf(buf, sizeof(buf),
                     "Falha ao executar /etc/rc: errno=%d", errno);
            xwrite(MURU_ERROR, buf);
        }

        _exit(127);
    }

    if (pid > 0)
        wait_for(pid);
    else
        xwrite(MURU_ERROR, "Falha ao criar processo para /etc/rc");
}

static void run_shutdown_rc(void)
{
    pid_t pid;
    pid = fork();
    
    if (pid == 0)
    {
        execl("/bin/sh", "/bin/sh", RC_SHUT, (char *)NULL);
        int err = errno;
        xwrite(MURU_ERROR, strerror(err));
        _exit(1);
    }
    if (pid > 0)
        wait_for(pid);
    else
    {
        int err = errno;
        xwrite(MURU_ERROR, strerror(err));
    }
}

static void kill_all(void)
{
    kill(-1, SIGTERM);
    sleep(3);
    kill(-1, SIGKILL);
    sleep(1);
}

static void umount_all(void)
{
    FILE *fp;
    char line[512];
    char mpts[64][256];
    int n, i;
    n = 0;
    fp = fopen("/proc/mounts", "r");
    if (!fp)
        return;
    while (fgets(line, (int)sizeof(line), fp) && n < 64)
    {
        char dev[256], mnt[256], rest[128];
        if (sscanf(line, "%255s %255s %127s", dev, mnt, rest) < 2)
            continue;
        if (strcmp(mnt, "/") == 0)
            continue;
        strncpy(mpts[n], mnt, 255);
        mpts[n][255] = '\0';
        n++;
    }
    fclose(fp);
    for (i = n - 1; i >= 0; i--)
        umount(mpts[i]);
}

static void do_poweroff(void)
{
    run_shutdown_rc();
    kill_all();
    sync();
    umount_all();
    mount(NULL, "/", NULL, MS_REMOUNT | MS_RDONLY, NULL);
    reboot(RB_POWER_OFF);
}

static void do_reboot(void)
{
    run_shutdown_rc();
    kill_all();
    sync();
    umount_all();
    mount(NULL, "/", NULL, MS_REMOUNT | MS_RDONLY, NULL);
    reboot(RB_AUTOBOOT);
}

static void do_rescue(void)
{
    pid_t pid;
    g_ntty = 0;
    run_shutdown_rc();
    kill_all();
    sync();
    mount(NULL, "/", NULL, MS_REMOUNT | MS_RDONLY, NULL);
    pid = fork();
    if (pid == 0)
    {
        execl("/bin/sh", "/bin/sh", (char *)NULL);
        _exit(1);
    }
    if (pid > 0)
        wait_for(pid);
}

static void handle_sig(void)
{
    int s;
    s = g_sig;
    g_sig = S_NONE;
    if (s == S_POWEROFF)
        do_poweroff();
    else if (s == S_REBOOT)
        do_reboot();
    else if (s == S_RESCUE)
        do_rescue();
}

static void mainloop(void)
{
    pid_t p;
    int st, i;
    while (1)
    {
        p = waitpid(-1, &st, 0);
        if (p < 0)
        {
            if (errno == EINTR && g_sig)
                handle_sig();
            continue;
        }
        if (g_sig)
            handle_sig();
        for (i = 0; i < g_ntty; i++)
        {
            if (g_ttys[i].pid == p)
            {
                g_ttys[i].pid = start_getty(i);
                break;
            }
        }
    }
}

static void normal_boot(void)
{
    run_rc();
    g_ntty = parse_ttys();
    spawn_gettys();
    mainloop();
}

static void single_boot(void)
{
    pid_t pid = fork();

    if (pid == 0) {
        execl("/bin/sh", "/bin/sh", (char *)NULL);
        xwrite(MURU_ERROR, "Failed in start shell");
        _exit(127);
    }

    if (pid > 0)
        wait_for(pid);
    else
        xwrite(MURU_ERROR, "Error on creating shell process");

    normal_boot();
}

int main(int argc, char *argv[])
{
    int single;
    single = 0;
    if (getpid() != 1)
    {
        write(STDERR_FILENO, "Already running\n", 16);
        return 1;
    }
    mount_dev();
    init_console();
    setup_sigs();
    if (argc > 1)
    {
        if (strcmp(argv[1], "single") == 0 || strcmp(argv[1], "1") == 0)
            single = 1;
    }

    xwrite(MURU_WARNING, "MURU " MURU_VERSION " is loading...");
    if (single)
        single_boot();
    else
    {
        normal_boot();
    }
    return 0;
}
