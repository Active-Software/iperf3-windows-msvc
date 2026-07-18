#ifndef IPERF_WIN32_SERVICE_H
#define IPERF_WIN32_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

int iperf_win32_service_maybe_handle(int argc, char **argv, int *exit_code);

#ifdef __cplusplus
}
#endif

#endif
