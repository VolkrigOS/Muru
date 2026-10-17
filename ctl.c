/*
*       main muructl structure
*       see LICENSE for more licensing details.
*/

#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "muru.h"

int
send_command(const char *command)
{
    int fd;
    struct sockaddr_un addr;
    char response[256];
    ssize_t n;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return 1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, MURU_SOCKET,
        sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return 1;
    }

    if (write(fd, command, strlen(command)) < 0) {
        close(fd);
        return 1;
    }

    n = read(fd, response, sizeof(response) - 1);
    if (n > 0) {
        response[n] = '\0';
        fputs(response, stdout);
    }

    close(fd);
    return n > 0 ? 0 : 1;
}

void
usage(void)
{
    fprintf(stdout,
        "usage: muructl <command> [service]\n"
                "usage: muructl <command> [service]\n"
        "\n"
        "commands:\n"
        "  start NAME      Start service\n"
        "  stop NAME       Stop service\n"
        "  restart NAME    Restart service\n"
        "  status NAME     Show service status\n"
        "  reload NAME     Reload service\n"
        "  list       List services\n");
}

int
s_start(const char *service)
{
    char command[BUFFER_SIZE];

    if (access(MURU_SOCKET, F_OK) != 0) {
        fprintf(stderr, "muructl: %s does not exist\n", MURU_SOCKET);
        return 1;
    }

    if (snprintf(command, sizeof(command), "start %s\n",
        service) >= (int)sizeof(command)) {
        fprintf(stderr, "muructl: command too long\n");
        return 1;
    }

    return send_command(command);
}

int
main(int argc, char **argv) 
{
    if (argc < 2)
    {
        usage();
        return 1;
    }

    const char *command = argv[1];
    if (strcmp(command, "start") == 0 && argc == 3)
        return s_start(argv[2]);

    usage();
    return 1;
}
