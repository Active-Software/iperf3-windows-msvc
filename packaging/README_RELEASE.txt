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
  - The Unix daemon option -D is not supported on native Windows.

Upstream project:

  https://github.com/esnet/iperf
  https://software.es.net/iperf/

License:

  See LICENSE.
