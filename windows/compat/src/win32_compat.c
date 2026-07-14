#include "iperf_win32_compat.h"
#include "pthread.h"

#include <errno.h>
#include <stdarg.h>

static int
win32_wsa_to_errno(int wsa_error)
{
    switch (wsa_error) {
    case 0:
        return 0;
    case WSAEWOULDBLOCK:
        return EWOULDBLOCK;
    case WSAEINPROGRESS:
        return EINPROGRESS;
    case WSAECONNRESET:
        return ECONNRESET;
    case WSAETIMEDOUT:
        return ETIMEDOUT;
    case WSAEMSGSIZE:
        return EMSGSIZE;
    case WSAEINTR:
        return EINTR;
    case WSAEINVAL:
        return EINVAL;
    case WSAENOTSOCK:
        return EBADF;
    default:
        return EIO;
    }
}

static int
set_wsa_errno(void)
{
    errno = win32_wsa_to_errno(WSAGetLastError());
    return errno;
}

int
iperf_win32_startup(void)
{
    static volatile LONG initialized;
    if (InterlockedCompareExchange(&initialized, 1, 0) == 0) {
        WSADATA wsa;
        int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
        if (rc != 0) {
            errno = win32_wsa_to_errno(rc);
            InterlockedExchange(&initialized, 0);
            return -1;
        }
    }
    return 0;
}

int
iperf_win32_socket_errno(void)
{
    return set_wsa_errno();
}

int
iperf_win32_close(int fd)
{
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    if (closesocket((SOCKET)(uintptr_t)fd) == 0)
        return 0;

    if (WSAGetLastError() != WSAENOTSOCK) {
        set_wsa_errno();
        return -1;
    }

    if (fd >= 0 && fd < 64)
        return _close(fd);

    errno = EBADF;
    return -1;
}

int
iperf_win32_read(int fd, void *buf, size_t count)
{
    int rc = recv((SOCKET)(uintptr_t)fd, (char *)buf, (int)count, 0);
    if (rc >= 0)
        return rc;

    if (WSAGetLastError() != WSAENOTSOCK) {
        set_wsa_errno();
        return -1;
    }

    return _read(fd, buf, (unsigned int)count);
}

int
iperf_win32_write(int fd, const void *buf, size_t count)
{
    int rc = send((SOCKET)(uintptr_t)fd, (const char *)buf, (int)count, 0);
    if (rc >= 0)
        return rc;

    if (WSAGetLastError() != WSAENOTSOCK) {
        set_wsa_errno();
        return -1;
    }

    return _write(fd, buf, (unsigned int)count);
}

int
gettimeofday(struct timeval *tv, void *tz)
{
    FILETIME ft;
    ULARGE_INTEGER uli;
    (void)tz;

    if (!tv) {
        errno = EINVAL;
        return -1;
    }

    GetSystemTimePreciseAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    uli.QuadPart -= 116444736000000000ULL;
    tv->tv_sec = (long)(uli.QuadPart / 10000000ULL);
    tv->tv_usec = (long)((uli.QuadPart % 10000000ULL) / 10ULL);
    return 0;
}

int
clock_gettime(int clk_id, struct timespec *ts)
{
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    struct timeval tv;

    if (!ts) {
        errno = EINVAL;
        return -1;
    }

    if (clk_id == CLOCK_MONOTONIC) {
        if (frequency.QuadPart == 0)
            QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        ts->tv_sec = (time_t)(counter.QuadPart / frequency.QuadPart);
        ts->tv_nsec = (long)(((counter.QuadPart % frequency.QuadPart) * 1000000000LL) / frequency.QuadPart);
        return 0;
    }

    if (gettimeofday(&tv, NULL) == 0) {
        ts->tv_sec = tv.tv_sec;
        ts->tv_nsec = tv.tv_usec * 1000L;
        return 0;
    }

    return -1;
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    DWORD millis;
    (void)rem;

    if (!req || req->tv_sec < 0 || req->tv_nsec < 0) {
        errno = EINVAL;
        return -1;
    }

    millis = (DWORD)(req->tv_sec * 1000 + (req->tv_nsec + 999999) / 1000000);
    Sleep(millis);
    return 0;
}

int
poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int rc = WSAPoll((WSAPOLLFD *)fds, nfds, timeout);
    if (rc == SOCKET_ERROR) {
        set_wsa_errno();
        return -1;
    }
    return rc;
}

int
fcntl(int fd, int cmd, ...)
{
    va_list ap;
    unsigned long mode;

    if (cmd == F_GETFL)
        return 0;

    if (cmd != F_SETFL) {
        errno = EINVAL;
        return -1;
    }

    va_start(ap, cmd);
    mode = (va_arg(ap, int) & O_NONBLOCK) ? 1UL : 0UL;
    va_end(ap);

    if (ioctlsocket((SOCKET)(uintptr_t)fd, FIONBIO, &mode) == SOCKET_ERROR) {
        set_wsa_errno();
        return -1;
    }

    return 0;
}

int
getrusage(int who, struct rusage *usage)
{
    FILETIME creation;
    FILETIME exit_time;
    FILETIME kernel;
    FILETIME user;
    ULARGE_INTEGER k;
    ULARGE_INTEGER u;
    (void)who;

    if (!usage) {
        errno = EINVAL;
        return -1;
    }

    memset(usage, 0, sizeof(*usage));
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit_time, &kernel, &user))
        return -1;

    k.LowPart = kernel.dwLowDateTime;
    k.HighPart = kernel.dwHighDateTime;
    u.LowPart = user.dwLowDateTime;
    u.HighPart = user.dwHighDateTime;
    usage->ru_stime.tv_sec = (long)(k.QuadPart / 10000000ULL);
    usage->ru_stime.tv_usec = (long)((k.QuadPart % 10000000ULL) / 10ULL);
    usage->ru_utime.tv_sec = (long)(u.QuadPart / 10000000ULL);
    usage->ru_utime.tv_usec = (long)((u.QuadPart % 10000000ULL) / 10ULL);
    return 0;
}

int
uname(struct utsname *name)
{
    SYSTEM_INFO si;
    DWORD size;

    if (!name) {
        errno = EINVAL;
        return -1;
    }

    memset(name, 0, sizeof(*name));
    strcpy_s(name->sysname, sizeof(name->sysname), "Windows");
    size = sizeof(name->nodename);
    GetComputerNameA(name->nodename, &size);
    strcpy_s(name->release, sizeof(name->release), "Windows 11+");
    strcpy_s(name->version, sizeof(name->version), "MSVC");
    GetNativeSystemInfo(&si);
    strcpy_s(name->machine, sizeof(name->machine),
             si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ? "arm64" : "x86_64");
    return 0;
}

char *
index(const char *s, int c)
{
    return strchr(s, c);
}

char *
rindex(const char *s, int c)
{
    return strrchr(s, c);
}

int
daemon(int nochdir, int noclose)
{
    (void)nochdir;
    (void)noclose;
    errno = ENOSYS;
    return -1;
}

char *
getpass(const char *prompt)
{
    static char buffer[256];
    size_t len;

    if (prompt) {
        fputs(prompt, stderr);
        fflush(stderr);
    }

    if (!fgets(buffer, sizeof(buffer), stdin))
        return NULL;
    len = strlen(buffer);
    if (len && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
    return buffer;
}

struct pthread_start {
    void *(*start_routine)(void *);
    void *arg;
};

static DWORD WINAPI
pthread_entry(LPVOID arg)
{
    struct pthread_start *start = (struct pthread_start *)arg;
    void *(*start_routine)(void *) = start->start_routine;
    void *start_arg = start->arg;
    free(start);
    (void)start_routine(start_arg);
    return 0;
}

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
               void *(*start_routine)(void *), void *arg)
{
    struct pthread_start *start;
    (void)attr;

    if (!thread || !start_routine)
        return EINVAL;

    start = (struct pthread_start *)calloc(1, sizeof(*start));
    if (!start)
        return ENOMEM;
    start->start_routine = start_routine;
    start->arg = arg;

    *thread = CreateThread(NULL, 0, pthread_entry, start, 0, NULL);
    if (!*thread) {
        free(start);
        return EAGAIN;
    }
    return 0;
}

int
pthread_join(pthread_t thread, void **retval)
{
    (void)retval;
    if (!thread)
        return EINVAL;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return 0;
}

int
pthread_cancel(pthread_t thread)
{
    if (!thread)
        return EINVAL;
    return 0;
}

int pthread_attr_init(pthread_attr_t *attr) { if (attr) *attr = 0; return 0; }
int pthread_attr_destroy(pthread_attr_t *attr) { (void)attr; return 0; }

int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    (void)attr;
    InitializeCriticalSection(mutex);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) { DeleteCriticalSection(mutex); return 0; }
int pthread_mutex_lock(pthread_mutex_t *mutex) { EnterCriticalSection(mutex); return 0; }
int pthread_mutex_unlock(pthread_mutex_t *mutex) { LeaveCriticalSection(mutex); return 0; }
int pthread_mutexattr_init(pthread_mutexattr_t *attr) { if (attr) *attr = 0; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) { (void)attr; return 0; }
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) { (void)attr; (void)type; return 0; }
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset) { (void)how; (void)set; (void)oldset; return 0; }
int pthread_setcanceltype(int type, int *oldtype) { if (oldtype) *oldtype = type; return 0; }
int pthread_setcancelstate(int state, int *oldstate) { if (oldstate) *oldstate = state; return 0; }

ssize_t
getline(char **lineptr, size_t *n, FILE *stream)
{
    int ch;
    size_t pos = 0;
    char *newptr;

    if (!lineptr || !n || !stream) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = (char *)malloc(*n);
        if (!*lineptr)
            return -1;
    }

    while ((ch = fgetc(stream)) != EOF) {
        if (pos + 1 >= *n) {
            size_t new_size = *n * 2;
            newptr = (char *)realloc(*lineptr, new_size);
            if (!newptr)
                return -1;
            *lineptr = newptr;
            *n = new_size;
        }
        (*lineptr)[pos++] = (char)ch;
        if (ch == '\n')
            break;
    }

    if (pos == 0 && ch == EOF)
        return -1;

    (*lineptr)[pos] = '\0';
    return (ssize_t)pos;
}

int
mkstemp(char *template_name)
{
    char *x;
    unsigned int value;

    if (!template_name) {
        errno = EINVAL;
        return -1;
    }

    x = strstr(template_name, "XXXXXX");
    if (!x) {
        errno = EINVAL;
        return -1;
    }

    value = (unsigned int)GetCurrentProcessId() ^ (unsigned int)GetTickCount();
    for (int i = 0; i < 100; ++i) {
        sprintf_s(x, 7, "%06X", (value + i) & 0xFFFFFF);
        int fd = _open(template_name, _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY | _O_TEMPORARY, _S_IREAD | _S_IWRITE);
        if (fd >= 0)
            return fd;
    }

    errno = EEXIST;
    return -1;
}

int
ftruncate(int fd, int64_t length)
{
    intptr_t handle = _get_osfhandle(fd);
    LARGE_INTEGER li;

    if (handle == -1) {
        errno = EBADF;
        return -1;
    }

    li.QuadPart = length;
    if (!SetFilePointerEx((HANDLE)handle, li, NULL, FILE_BEGIN) ||
        !SetEndOfFile((HANDLE)handle)) {
        errno = EIO;
        return -1;
    }

    return 0;
}

void *
mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset)
{
    void *mem;
    (void)addr;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;

    mem = calloc(1, length);
    return mem ? mem : MAP_FAILED;
}

int
munmap(void *addr, size_t length)
{
    (void)length;
    if (addr && addr != MAP_FAILED)
        free(addr);
    return 0;
}

int
getpid(void)
{
    return (int)GetCurrentProcessId();
}

int
kill(pid_t pid, int sig)
{
    HANDLE process;
    (void)sig;

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
    if (!process) {
        errno = ESRCH;
        return -1;
    }
    CloseHandle(process);
    return 0;
}

char *
strsignal(int sig)
{
    static char buffer[32];
    sprintf_s(buffer, sizeof(buffer), "signal %d", sig);
    return buffer;
}
