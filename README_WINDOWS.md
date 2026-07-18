# iperf3 Windows MSVC build

Unofficial community build by Daniel Doguet / Active-Software.

This tree contains the official ESnet iperf 3.21 sources from:

https://downloads.es.net/pub/iperf/iperf-3.21.tar.gz

Verified SHA256:

```text
656e4405ebd620121de7ceca3eaf43a88f79ea1b857d041a6a0b1314801acdd8
```

The Visual Studio files are native VS2022/MSVC projects for Windows 11+ and do
not require a Cygwin or MSYS runtime.

Release builds use MSVC size optimization (`/O1`) plus linker dead-code folding
and reference optimization.

## Build

```powershell
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild .\iperf3.sln /p:Configuration=Release /p:Platform=x64
& $msbuild .\iperf3.sln /p:Configuration=Release /p:Platform=ARM64
```

Outputs:

```text
build\x64\Release\iperf3.exe
build\ARM64\Release\iperf3.exe
```

Debug configurations are also present for `x64` and `ARM64`.

## Windows port notes

The Windows-specific compatibility layer is in `windows\compat`. It provides
small MSVC shims for POSIX APIs used by iperf3, including sockets-style
`read/write/close`, pthread-like wrappers, timing, temporary buffers, and random
bytes via `BCryptGenRandom`.

SCTP, OpenSSL authentication, Linux FQ socket pacing, and sendfile zero-copy are
not enabled in this project. Native Windows builds add:

```powershell
.\iperf3.exe --service-install
.\iperf3.exe --service-start
.\iperf3.exe --service-stop
.\iperf3.exe --service-remove
```

The service commands require an elevated terminal. The service runs `iperf3 -s`
and writes its log to `%ProgramData%\iperf3\iperf3-service.log`. If the server
process exits unexpectedly, the service keeps running and starts it again. This
is the native Windows replacement for the Unix `-D` / `--daemon` mode, which is
not supported by this build.

## Release package contents

The GitHub release assets should contain:

```text
iperf3.exe
LICENSE
README_RELEASE.txt
```

Publish both `x64` and `ARM64` zip files, plus `SHA256SUMS.txt`.

## Portable packages

```powershell
.\packaging\make-release.ps1
```

The script builds `Release|x64` and `Release|ARM64`, then creates the zip files
and `SHA256SUMS.txt` under `dist\`.
