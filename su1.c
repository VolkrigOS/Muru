/*
*       main su1 structure
*       see LICENSE for more licensing details.
*/

#include <sys/reboot.h>
#include <stdio.h>

int main(void) {
    if (reboot(RB_HALT_SYSTEM) == -1) {
        fprintf(stderr, "must be run as root");
        return 1;
    }
    return 0;
}