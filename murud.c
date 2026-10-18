#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "muru.h"

static int make_server(void)
{
    int fd;
    struct sockaddr_un addr;

    if (mkdir(MURU_RUN_DIR, 0755) < 0 && errno != EEXIST)
        return -1;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
    {
        close(fd);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(MURU_SOCKET) >= sizeof(addr.sun_path))
    {
        close(fd);
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(addr.sun_path, MURU_SOCKET);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(fd);
        return -1;
    }
    if (chmod(MURU_SOCKET, 0600) < 0 || listen(fd, 8) < 0)
    {
        close(fd);
        unlink(MURU_SOCKET);
        return -1;
    }
    return fd;
}

static int read_command(const char *name, char *command, size_t size)
{
    char path[256];
    char line[512];
    FILE *file;

    if (name[0] == '\0' || strchr(name, '/') != NULL || strstr(name, ".."))
        return -1;
    if (snprintf(path, sizeof(path), "%s/%s", MURU_SERVICES_DIR, name) >=
        (int)sizeof(path))
        return -1;

    file = fopen(path, "r");
    if (file == NULL)
        return -1;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strncmp(line, "command=", 8) == 0)
        {
            char *value = line + 8;
            value[strcspn(value, "\r\n")] = '\0';
            if (*value != '\0' && strlen(value) < size)
            {
                strcpy(command, value);
                fclose(file);
                return 0;
            }
            break;
        }
    }
    fclose(file);
    return -1;
}

static int start_service(const char *name, pid_t *pid)
{
    char command[512];
    pid_t child;

    if (read_command(name, command, sizeof(command)) < 0)
        return -1;
    child = fork();
    if (child < 0)
        return -1;
    if (child == 0)
    {
        execl(command, command, (char *)NULL);
        _exit(127);
    }
    *pid = child;
    return 0;
}

static void handle_client(int client)
{
    char request[BUFFER_SIZE];
    char response[256];
    char name[64];
    pid_t pid;
    ssize_t n = read(client, request, sizeof(request) - 1);

    if (n <= 0)
        return;
    request[n] = '\0';

    if (sscanf(request, "start %63s", name) == 1 &&
        strncmp(request, "start ", 6) == 0)
    {
        if (start_service(name, &pid) == 0)
            snprintf(response, sizeof(response), "started %s (PID %ld)\n",
                     name, (long)pid);
        else
            snprintf(response, sizeof(response), "failed to start %s\n", name);
    }
    else
        snprintf(response, sizeof(response), "invalid command\n");

    (void)write(client, response, strlen(response));
}

int main(void)
{
    int server = make_server();

    if (server < 0)
    {
        perror("murud: socket setup");
        return 1;
    }

    for (;;)
    {
        fd_set readfds;
        struct timeval timeout = {1, 0};
        int ready;

        FD_ZERO(&readfds);
        FD_SET(server, &readfds);
        ready = select(server + 1, &readfds, NULL, NULL, &timeout);
        if (ready > 0 && FD_ISSET(server, &readfds))
        {
            int client = accept(server, NULL, NULL);
            if (client >= 0)
            {
                (void)fcntl(client, F_SETFD, FD_CLOEXEC);
                handle_client(client);
                close(client);
            }
        }
        else if (ready < 0 && errno != EINTR)
            perror("murud: select");

        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
}
