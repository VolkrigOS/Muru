/*
*       main reboot structure
*       see LICENSE for more licensing details.
*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int
main(void) {
    if (kill(1, SIGINT) != 0) {
        fprintf(stderr, "Failed to send SIGINT to PID 1. Are you root?\n");
        return 1;
    }
    return 0;
}
