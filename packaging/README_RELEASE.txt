iperf3 3.21 native Windows MSVC build
=====================================

Unofficial community build by Daniel Doguet / Active-Software.

This package contains a native Windows command-line iperf3.exe built with
Visual Studio 2022 / MSVC for Windows 11 and later.

No Cygwin or MSYS runtime is required.

Usage:

  iperf3.exe --version
  iperf3.exe -c <server-ip>
  iperf3.exe -s

Limitations:

  - SCTP is not enabled.
  - OpenSSL authentication support is not enabled.
  - Linux sendfile zero-copy and FQ socket pacing are not enabled.

Windows service:

  Run from an elevated terminal:

    iperf3.exe --service-install
    iperf3.exe --service-start
    iperf3.exe --service-stop
    iperf3.exe --service-remove

  The service runs iperf3 -s and logs to:

    %ProgramData%\iperf3\iperf3-service.log

  If the server process exits unexpectedly, the service keeps running and
  starts it again.

Upstream project:

  https://github.com/esnet/iperf
  https://software.es.net/iperf/

License:

  See LICENSE.
