#ifndef IPERF_WIN32_TERMIOS_H
#define IPERF_WIN32_TERMIOS_H

#include "iperf_win32_compat.h"

#define ECHO 0x0008
#define TCSAFLUSH 2

struct termios {
    int c_lflag;
};

static inline int tcgetattr(int fd, struct termios *termios_p)
{
    (void)fd;
    if (termios_p)
        termios_p->c_lflag = ECHO;
    return 0;
}

static inline int tcsetattr(int fd, int optional_actions, const struct termios *termios_p)
{
    (void)fd;
    (void)optional_actions;
    (void)termios_p;
    return 0;
}

#endif
