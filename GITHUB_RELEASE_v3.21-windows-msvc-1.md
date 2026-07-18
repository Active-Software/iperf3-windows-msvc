# iperf 3.21 native Windows MSVC build

Unofficial community build by Daniel Doguet / Active-Software.

This release provides native Windows command-line `iperf3.exe` builds produced
with Visual Studio 2022 / MSVC for Windows 11 and later.

## Assets

- `iperf3-3.21-windows-msvc-x64.zip`
- `iperf3-3.21-windows-msvc-arm64.zip`
- `SHA256SUMS.txt`

Each zip contains:

- `iperf3.exe`
- `LICENSE`
- `README_RELEASE.txt`

## Build notes

- Upstream source: ESnet iperf 3.21
- Upstream archive: `https://downloads.es.net/pub/iperf/iperf-3.21.tar.gz`
- Upstream source SHA256: `656e4405ebd620121de7ceca3eaf43a88f79ea1b857d041a6a0b1314801acdd8`
- Compiler: Visual Studio 2022 / MSVC v143
- Platforms: `x64`, `ARM64`
- Optimization: size (`/O1`)
- Runtime: static MSVC runtime (`/MT`)
- No Cygwin/MSYS runtime dependency

## Limitations

- SCTP is not enabled.
- OpenSSL authentication support is not enabled.
- The Unix daemon option `-D` / `--daemon` is not supported on native Windows.
  Use the Windows service mode from build 2 and later for a persistent iperf3 server.

## Quick test

```powershell
.\iperf3.exe --version
.\iperf3.exe -c <server-ip>
```
