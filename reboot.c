/*
*       main reboot structure
*       see LICENSE for more licensing details.
*/

#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "muru.h"

static int run_shutrc(void)
{
    pid_t pid = fork();

    if (pid == 0) {
        execl("/bin/sh", "sh", RC_SHUT, (char *)NULL);
        _exit(127);
    }

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    while (waitpid(pid, NULL, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return -1;
        }
    }

    return 0;
}

int main(void) {
    if (run_shutrc() < 0) return 1;

    if (reboot(RB_AUTOBOOT) == -1) {
        fprintf(stderr, "must be run as root");
        return 1;
    }
    return 0;
}