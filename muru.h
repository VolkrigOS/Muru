
#ifndef MURU_H
#define MURU_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MURU_VERSION "0.0.3"

#define MURU_OK      1
#define MURU_WARNING 2
#define MURU_ERROR   0

static void xwrite(int type, const char *msg)
{
    struct winsize ws;
    int fd = type == MURU_ERROR ? STDERR_FILENO : STDOUT_FILENO;
    FILE *out = type == MURU_ERROR ? stderr : stdout;
    int width = 80;

    if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        width = ws.ws_col;

    const char *color = type == MURU_OK
        ? "\033[38;2;0;255;80m"
        : type == MURU_WARNING
        ? "\033[38;2;255;200;0m"
        : "\033[38;2;255;30;30m";

    const char *status = type == MURU_OK ? "ok" : "!!";
    int prefix = 2 + (int)strlen(msg);
    int suffix = type == MURU_WARNING ? 0 : (int)strlen(status) + 5;
    int spaces = width - prefix - suffix;

    if (spaces < 1)
        spaces = 1;

    fprintf(out, "%s* \033[0m\033[97m%s\033[0m", color, msg);

    if (type != MURU_WARNING) {
        for (int i = 0; i < spaces; i++)
            fputc(' ', out);

        fprintf(out, " \033[94m[\033[0m %s%s\033[0m \033[94m]\033[0m",
                color, status);
    }

    fputc('\n', out);
    fflush(out);
}

#endif
