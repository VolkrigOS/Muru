/*
 *	main init structure
 *	see LICENSE for more licensing details.
 */
#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "muru.h"

static void xwrite(int type, const char *msg)
{
    FILE *out = type == 0 ? stderr : stdout;
    const char *color = type == 1 ? "\033[38;2;0;255;80m" : type == 2 ? "\033[38;2;255;200;0m" : "\033[38;2;255;30;30m";
    fprintf(out, "%s* \033[0m\033[97m%s\033[0m", color, msg);
    fputc('\n', out);
    fflush(out);
}

static void run_rc(void)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        char buf[64];
        if (access("/bin/sh", X_OK) != 0)
        {
            snprintf(buf, sizeof(buf), "/bin/sh errno=%d", errno);
            xwrite(0, buf);
            _exit(127);
        }
        xwrite(1, "/bin/sh");
        if (access(RC_PATH, R_OK) != 0)
        {
            snprintf(buf, sizeof(buf), "%s don't exist", RC_PATH);
            xwrite(0, buf);
            _exit(127);
        }
        xwrite(1, RC_PATH);
        if (access(RC_SHUT, F_OK) != 0)
        {
            snprintf(buf, sizeof(buf), "%s don't exist", RC_SHUT);
            xwrite(0, buf);
            _exit(127);
        }
        xwrite(1, RC_SHUT);
        execl("/bin/sh", "/bin/sh", RC_PATH, (char *)NULL);
        _exit(127);
    }
    if (pid < 0) xwrite(0, "failed to exec /etc/rc");
}

static void reap_loop(void)
{
    sigset_t signals;
    int signal_number;

    sigemptyset(&signals);
    sigaddset(&signals, SIGCHLD);
    sigprocmask(SIG_BLOCK, &signals, NULL);

    for (;;)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0); 
        sigwait(&signals, &signal_number);
    }
}

int main(void)
{
    if (getpid() != 1)
    {
        write(STDERR_FILENO, "Already running\n", 16);
        return 1;
    }
    fprintf(stdout, "   \033[95mMURU \033[96m" MURU_VERSION "\033[0m is running...\n\n");
    run_rc();
    reap_loop();
    return 0;
}