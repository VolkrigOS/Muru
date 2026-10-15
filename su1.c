/*
*       main su1 structure
*       see LICENSE for more licensing details.
*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int
main(void) {
    if (kill(1, SIGTERM) != 0) {
        fprintf(stderr, "Failed to send SIGTERM to PID 1. Are you root?\n");
        return 1;
    }
    fprintf(stdout, "ok\n");
    return 0;
}
