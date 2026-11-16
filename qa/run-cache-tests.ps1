$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent;$paths=& "$root/scripts/resolve-build-paths.ps1" -CoreOnly
$out="$root/qa/build";New-Item -ItemType Directory -Path $out -Force|Out-Null
& "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" -std=c++20 -O3 -ffp-contract=off -static -I "$root/glow/include" "$root/glow/src/glow_core.cpp" "$root/qa/performance-cache-tests.cpp" -o "$out/cache-tests.exe"
if($LASTEXITCODE){throw 'Cache tests failed to compile'}
& "$out/cache-tests.exe"
if($LASTEXITCODE){throw 'Cache tests failed'}
