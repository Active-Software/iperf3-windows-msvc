#ifndef IPERF_WIN32_PTHREAD_H
#define IPERF_WIN32_PTHREAD_H

#include "iperf_win32_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef int pthread_attr_t;
typedef int pthread_mutexattr_t;

#define PTHREAD_CANCEL_ENABLE 0
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_DEFERRED 0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1
#define PTHREAD_MUTEX_ERRORCHECK 1

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_cancel(pthread_t thread);
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);
int pthread_setcanceltype(int type, int *oldtype);
int pthread_setcancelstate(int state, int *oldstate);

#ifdef __cplusplus
}
#endif

#endif
