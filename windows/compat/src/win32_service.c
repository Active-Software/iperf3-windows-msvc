#include "iperf_win32_service.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define IPERF_SERVICE_DEFAULT_NAME "iperf3"
#define IPERF_SERVICE_DISPLAY_NAME "iperf3 Server"

static SERVICE_STATUS_HANDLE g_status_handle;
static SERVICE_STATUS g_status;
static HANDLE g_stop_event;
static PROCESS_INFORMATION g_child;
static const char *g_service_name = IPERF_SERVICE_DEFAULT_NAME;

static int
streq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

static const char *
service_name_from_args(int argc, char **argv, int index)
{
    if (argc > index + 1 && argv[index + 1] && argv[index + 1][0] != '-') {
        return argv[index + 1];
    }
    return IPERF_SERVICE_DEFAULT_NAME;
}

static void
set_service_status(DWORD state, DWORD win32_exit_code, DWORD wait_hint)
{
    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwCurrentState = state;
    g_status.dwControlsAccepted = (state == SERVICE_RUNNING) ? SERVICE_ACCEPT_STOP : 0;
    g_status.dwWin32ExitCode = win32_exit_code;
    g_status.dwServiceSpecificExitCode = 0;
    g_status.dwCheckPoint = (state == SERVICE_START_PENDING ||
                             state == SERVICE_STOP_PENDING) ? 1 : 0;
    g_status.dwWaitHint = wait_hint;

    if (g_status_handle) {
        SetServiceStatus(g_status_handle, &g_status);
    }
}

static DWORD WINAPI
service_control_handler(DWORD control, DWORD event_type, LPVOID event_data, LPVOID context)
{
    (void)event_type;
    (void)event_data;
    (void)context;

    if (control == SERVICE_CONTROL_STOP) {
        set_service_status(SERVICE_STOP_PENDING, NO_ERROR, 5000);
        if (g_stop_event) {
            SetEvent(g_stop_event);
        }
        return NO_ERROR;
    }
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static int
get_log_path(char *buffer, size_t buffer_size)
{
    char program_data[MAX_PATH];
    DWORD len = GetEnvironmentVariableA("ProgramData", program_data, (DWORD)sizeof(program_data));

    if (len == 0 || len >= sizeof(program_data)) {
        strcpy_s(program_data, sizeof(program_data), "C:\\ProgramData");
    }

    if (snprintf(buffer, buffer_size, "%s\\iperf3", program_data) < 0) {
        return 0;
    }
    CreateDirectoryA(buffer, NULL);

    if (snprintf(buffer, buffer_size, "%s\\iperf3\\iperf3-service.log", program_data) < 0) {
        return 0;
    }
    return 1;
}

static void
append_service_log(const char *message, DWORD value)
{
    char log_path[MAX_PATH];
    FILE *log;

    if (!get_log_path(log_path, sizeof(log_path))) {
        return;
    }

    log = fopen(log_path, "a");
    if (!log) {
        return;
    }

    if (value == (DWORD)-1) {
        fprintf(log, "[service] %s\n", message);
    } else {
        fprintf(log, "[service] %s: %lu\n", message, value);
    }
    fclose(log);
}

static void
close_child_handles(void)
{
    if (g_child.hThread) {
        CloseHandle(g_child.hThread);
    }
    if (g_child.hProcess) {
        CloseHandle(g_child.hProcess);
    }
    memset(&g_child, 0, sizeof(g_child));
}

static int
start_child_server(void)
{
    char module[MAX_PATH];
    char log_path[MAX_PATH];
    char command_line[MAX_PATH * 3];
    STARTUPINFOA si;

    if (!GetModuleFileNameA(NULL, module, (DWORD)sizeof(module))) {
        return 0;
    }
    if (!get_log_path(log_path, sizeof(log_path))) {
        return 0;
    }

    if (snprintf(command_line, sizeof(command_line),
                 "\"%s\" -s --logfile \"%s\"",
                 module, log_path) < 0) {
        return 0;
    }

    memset(&si, 0, sizeof(si));
    memset(&g_child, 0, sizeof(g_child));
    si.cb = sizeof(si);

    return CreateProcessA(NULL, command_line, NULL, NULL, FALSE,
                          CREATE_NO_WINDOW, NULL, NULL, &si, &g_child) != 0;
}

static void WINAPI
service_main(DWORD argc, LPSTR *argv)
{
    (void)argc;
    (void)argv;

    g_status_handle = RegisterServiceCtrlHandlerExA(g_service_name,
                                                    service_control_handler, NULL);
    if (!g_status_handle) {
        return;
    }

    set_service_status(SERVICE_START_PENDING, NO_ERROR, 5000);
    g_stop_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!g_stop_event) {
        set_service_status(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    set_service_status(SERVICE_RUNNING, NO_ERROR, 0);

    for (;;) {
        HANDLE wait_handles[2];
        DWORD wait_rc;

        if (!start_child_server()) {
            DWORD err = GetLastError();
            append_service_log("failed to start iperf3 server process", err);
            set_service_status(SERVICE_STOPPED, err, 0);
            break;
        }

        append_service_log("started iperf3 server process", GetProcessId(g_child.hProcess));

        wait_handles[0] = g_stop_event;
        wait_handles[1] = g_child.hProcess;
        wait_rc = WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);

        if (wait_rc == WAIT_OBJECT_0) {
            append_service_log("stop requested", (DWORD)-1);
            TerminateProcess(g_child.hProcess, 0);
            WaitForSingleObject(g_child.hProcess, 5000);
            close_child_handles();
            set_service_status(SERVICE_STOPPED, NO_ERROR, 0);
            break;
        }

        if (wait_rc == WAIT_OBJECT_0 + 1) {
            DWORD child_exit_code = 0;
            GetExitCodeProcess(g_child.hProcess, &child_exit_code);
            append_service_log("iperf3 server process exited; restarting", child_exit_code);
            close_child_handles();

            if (WaitForSingleObject(g_stop_event, 1000) == WAIT_OBJECT_0) {
                set_service_status(SERVICE_STOPPED, NO_ERROR, 0);
                break;
            }
            continue;
        }

        append_service_log("unexpected service wait result", wait_rc);
        close_child_handles();
        set_service_status(SERVICE_STOPPED, GetLastError(), 0);
        break;
    }

    CloseHandle(g_stop_event);
    g_stop_event = NULL;
}

static int
run_service_dispatcher(void)
{
    SERVICE_TABLE_ENTRYA table[] = {
        { (LPSTR)g_service_name, service_main },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcherA(table)) {
        fprintf(stderr, "Unable to start Windows service dispatcher: %lu\n", GetLastError());
        return 1;
    }
    return 0;
}

static int
install_service(const char *service_name)
{
    SC_HANDLE scm;
    SC_HANDLE service;
    char module[MAX_PATH];
    char command_line[MAX_PATH * 2];

    if (!GetModuleFileNameA(NULL, module, (DWORD)sizeof(module))) {
        fprintf(stderr, "Unable to resolve executable path: %lu\n", GetLastError());
        return 1;
    }

    if (snprintf(command_line, sizeof(command_line),
                 "\"%s\" --service-run \"%s\"", module, service_name) < 0) {
        fprintf(stderr, "Unable to build service command line\n");
        return 1;
    }

    scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        fprintf(stderr, "OpenSCManager failed: %lu\n", GetLastError());
        return 1;
    }

    service = CreateServiceA(scm, service_name, IPERF_SERVICE_DISPLAY_NAME,
                             SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                             SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                             command_line, NULL, NULL, NULL, NULL, NULL);
    if (!service) {
        DWORD err = GetLastError();
        fprintf(stderr, "CreateService failed: %lu\n", err);
        CloseServiceHandle(scm);
        return 1;
    }

    printf("Installed Windows service '%s'.\n", service_name);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}

static int
start_service_cmd(const char *service_name)
{
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE service;

    if (!scm) {
        fprintf(stderr, "OpenSCManager failed: %lu\n", GetLastError());
        return 1;
    }
    service = OpenServiceA(scm, service_name, SERVICE_START);
    if (!service) {
        fprintf(stderr, "OpenService failed: %lu\n", GetLastError());
        CloseServiceHandle(scm);
        return 1;
    }
    if (!StartServiceA(service, 0, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_ALREADY_RUNNING) {
            fprintf(stderr, "StartService failed: %lu\n", err);
            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            return 1;
        }
    }
    printf("Started Windows service '%s'.\n", service_name);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}

static int
stop_service_cmd(const char *service_name)
{
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE service;
    SERVICE_STATUS status;

    if (!scm) {
        fprintf(stderr, "OpenSCManager failed: %lu\n", GetLastError());
        return 1;
    }
    service = OpenServiceA(scm, service_name, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        fprintf(stderr, "OpenService failed: %lu\n", GetLastError());
        CloseServiceHandle(scm);
        return 1;
    }
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_NOT_ACTIVE) {
            fprintf(stderr, "ControlService failed: %lu\n", err);
            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            return 1;
        }
    }
    printf("Stopped Windows service '%s'.\n", service_name);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}

static int
remove_service(const char *service_name)
{
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SC_HANDLE service;

    if (!scm) {
        fprintf(stderr, "OpenSCManager failed: %lu\n", GetLastError());
        return 1;
    }
    service = OpenServiceA(scm, service_name, DELETE);
    if (!service) {
        fprintf(stderr, "OpenService failed: %lu\n", GetLastError());
        CloseServiceHandle(scm);
        return 1;
    }
    if (!DeleteService(service)) {
        fprintf(stderr, "DeleteService failed: %lu\n", GetLastError());
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return 1;
    }
    printf("Removed Windows service '%s'.\n", service_name);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}

int
iperf_win32_service_maybe_handle(int argc, char **argv, int *exit_code)
{
    int rc = 0;

    if (argc < 2 || !argv || !argv[1]) {
        return 0;
    }

    if (streq(argv[1], "--service-run")) {
        g_service_name = service_name_from_args(argc, argv, 1);
        rc = run_service_dispatcher();
    } else if (streq(argv[1], "--service-install")) {
        rc = install_service(service_name_from_args(argc, argv, 1));
    } else if (streq(argv[1], "--service-start")) {
        rc = start_service_cmd(service_name_from_args(argc, argv, 1));
    } else if (streq(argv[1], "--service-stop")) {
        rc = stop_service_cmd(service_name_from_args(argc, argv, 1));
    } else if (streq(argv[1], "--service-remove")) {
        rc = remove_service(service_name_from_args(argc, argv, 1));
    } else {
        return 0;
    }

    if (exit_code) {
        *exit_code = rc;
    }
    return 1;
}
