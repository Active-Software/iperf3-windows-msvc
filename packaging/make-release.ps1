param(
    [string]$Configuration = "Release",
    [string]$Version = "3.21",
    [string]$MsBuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$dist = Join-Path $root "dist"
$platforms = @("x64", "ARM64")

if (-not (Test-Path $MsBuild)) {
    throw "MSBuild not found: $MsBuild"
}

New-Item -ItemType Directory -Force -Path $dist | Out-Null
Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $dist "iperf3-$Version-windows-msvc-*.zip")
Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $dist "SHA256SUMS.txt")

foreach ($platform in $platforms) {
    & $MsBuild (Join-Path $root "iperf3.sln") /m /p:Configuration=$Configuration /p:Platform=$platform
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $platform"
    }

    $packageRoot = Join-Path $dist "iperf3-$Version-windows-msvc-$($platform.ToLowerInvariant())"
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $packageRoot
    New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null

    Copy-Item -Force (Join-Path $root "build\$platform\$Configuration\iperf3.exe") $packageRoot
    Copy-Item -Force (Join-Path $root "LICENSE") $packageRoot
    Copy-Item -Force (Join-Path $root "packaging\README_RELEASE.txt") $packageRoot

    $zipPath = "$packageRoot.zip"
    Remove-Item -Force -ErrorAction SilentlyContinue $zipPath
    Compress-Archive -Path (Join-Path $packageRoot "*") -DestinationPath $zipPath -Force
    Remove-Item -Recurse -Force $packageRoot
}

$hashLines = foreach ($zip in Get-ChildItem -Path $dist -Filter "iperf3-$Version-windows-msvc-*.zip" | Sort-Object Name) {
    $hash = Get-FileHash -Algorithm SHA256 $zip.FullName
    "$($hash.Hash.ToLowerInvariant())  $($zip.Name)"
}

$hashLines | Set-Content -Encoding ascii (Join-Path $dist "SHA256SUMS.txt")
Write-Host "Release packages written to $dist"
