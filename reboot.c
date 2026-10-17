/*
*       main reboot structure
*       see LICENSE for more licensing details.
*/

#include <sys/reboot.h>
#include <stdio.h>

int main(void) {
    if (reboot(RB_AUTOBOOT) == -1) {
        fprintf(stderr, "must be run as root");
        return 1;
    }
    return 0;
}