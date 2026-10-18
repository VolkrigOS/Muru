
#ifndef MURU_H
#define MURU_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MURU_VERSION "0.0.6"

#define BUFFER_SIZE 1024

#define MURU_DIR          "/etc/muru"
#define MURU_SERVICES_DIR MURU_DIR "/services"
#define MURU_RUN_DIR      "/run/muru"
#define MURU_SOCKET       MURU_RUN_DIR "/muru.sock"
#define MURU_STATE_DIR    MURU_RUN_DIR "/services"

#define RC_PATH "/etc/rc"
#define RC_SHUT "/etc/rc.shutdown"
#define RC_US1 "/etc/rc.su1"

#endif
