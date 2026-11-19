param([string]$Output='evidence/offcanvas-v01')
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent;$paths=& "$root/scripts/resolve-build-paths.ps1"
$out=Join-Path $root $Output;New-Item -ItemType Directory -Path $out -Force|Out-Null
$entries=Get-Content -LiteralPath "$root/plugins.json" -Raw -Encoding utf8|ConvertFrom-Json
[IO.File]::WriteAllLines("$out/plugins.txt",@($entries|ForEach-Object {$_.module+'/build/'+$_.file}),[Text.UTF8Encoding]::new($false))
& "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" -std=c++20 -O2 -static -municode -DWIN32=1 -D_WINDOWS -DUNICODE -D_UNICODE -I "$root/displacement/include" -I "$root/native/ui/include" -isystem "$($paths.Sdk)/photoshopapi/photoshop" -isystem "$($paths.Sdk)/photoshopapi/pica_sp" "$PSScriptRoot/offcanvas-adapters.cpp" -o "$out/offcanvas-adapters.exe"
if($LASTEXITCODE){throw 'Offcanvas DLL harness build failed'}
& "$out/offcanvas-adapters.exe" $root "$out/plugins.txt" | Tee-Object "$out/adapter-results.txt"
if($LASTEXITCODE){throw 'Offcanvas DLL checks failed'}
