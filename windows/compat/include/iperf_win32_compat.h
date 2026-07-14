#ifndef IPERF_WIN32_COMPAT_H
#define IPERF_WIN32_COMPAT_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __attribute__
#define __attribute__(x)
#endif

#ifndef SIGHUP
#define SIGHUP 1
#endif

#ifndef SIGPIPE
#define SIGPIPE 13
#endif

#ifndef SIG_BLOCK
#define SIG_BLOCK 0
#endif

typedef int sigset_t;

static inline int sigemptyset(sigset_t *set)
{
    if (set)
        *set = 0;
    return 0;
}

static inline int sigaddset(sigset_t *set, int sig)
{
    if (set)
        *set |= sig;
    return 0;
}

#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

#ifndef ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif

#ifndef EMSGSIZE
#define EMSGSIZE WSAEMSGSIZE
#endif

#ifndef ssize_t
typedef intptr_t ssize_t;
#endif

typedef int socklen_t;
typedef unsigned long nfds_t;
typedef unsigned int uint;

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
typedef int clockid_t;

struct iovec {
    void *iov_base;
    size_t iov_len;
};

struct utsname {
    char sysname[64];
    char nodename[64];
    char release[64];
    char version[64];
    char machine[64];
};

struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;
};

#ifndef RUSAGE_SELF
#define RUSAGE_SELF 0
#endif

#ifndef POLLIN
#define POLLIN 0x0001
#endif
#ifndef POLLOUT
#define POLLOUT 0x0004
#endif
#ifndef POLLERR
#define POLLERR 0x0008
#endif
#ifndef POLLHUP
#define POLLHUP 0x0010
#endif
#ifndef POLLNVAL
#define POLLNVAL 0x0020
#endif

#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0x00000400
#endif

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

#ifndef TCP_MAXSEG
#define TCP_MAXSEG 4
#endif

#ifndef TCP_INFO
#define TCP_INFO 0x22
#endif

struct tcp_info {
    uint32_t tcpi_snd_cwnd;
    uint32_t tcpi_snd_mss;
    uint32_t tcpi_rtt;
    uint32_t tcpi_rttvar;
    uint32_t tcpi_pmtu;
    uint32_t tcpi_snd_wnd;
    uint32_t tcpi_total_retrans;
};

#define close(fd) iperf_win32_close(fd)
#define read(fd, buf, count) iperf_win32_read((fd), (buf), (count))
#define write(fd, buf, count) iperf_win32_write((fd), (buf), (count))
#define open _open
#define fstat _fstat64
#define stat _stat64
#define mkdir(path, mode) _mkdir(path)
#define unlink _unlink
#define access _access
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf _snprintf
#define fileno _fileno
#define isatty _isatty
#define lseek _lseeki64

#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif

#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef MAP_SHARED
#define MAP_SHARED 0x01
#endif
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

typedef int pid_t;

int iperf_win32_close(int fd);
int iperf_win32_read(int fd, void *buf, size_t count);
int iperf_win32_write(int fd, const void *buf, size_t count);
int iperf_win32_startup(void);
int iperf_win32_socket_errno(void);

int gettimeofday(struct timeval *tv, void *tz);
int clock_gettime(int clk_id, struct timespec *ts);
int nanosleep(const struct timespec *req, struct timespec *rem);
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
int fcntl(int fd, int cmd, ...);
int getrusage(int who, struct rusage *usage);
int uname(struct utsname *name);
char *index(const char *s, int c);
char *rindex(const char *s, int c);
int daemon(int nochdir, int noclose);
char *getpass(const char *prompt);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
int mkstemp(char *template_name);
int ftruncate(int fd, int64_t length);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset);
int munmap(void *addr, size_t length);
int getpid(void);
int kill(pid_t pid, int sig);
char *strsignal(int sig);

#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif

#ifdef __cplusplus
}
#endif

#endif
