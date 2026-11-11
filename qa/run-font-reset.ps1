param([string]$OutputRoot=(Join-Path (Split-Path $PSScriptRoot -Parent) 'evidence\persistence-0.36\font-reset'))
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent;$paths=& "$root\scripts\resolve-build-paths.ps1"
New-Item -ItemType Directory -Path "$PSScriptRoot\build",$OutputRoot -Force|Out-Null
& (Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe') -std=c++20 -O2 -static -DWIN32=1 -D_WINDOWS -DUNICODE -D_UNICODE -I "$root\native\ui\include" -I "$root\displacement\include" -I $paths.ImGui -isystem "$($paths.Sdk)\photoshopapi\photoshop" -isystem "$($paths.Sdk)\photoshopapi\pica_sp" "$PSScriptRoot\font-reset.cpp" "$root\native\build\library_ui.lib" "$root\native\build\psui.lib" -ld3d11 -ldxgi -limm32 -luser32 -lgdi32 -ldwmapi -lshell32 -o "$PSScriptRoot\build\font-reset.exe"
if($LASTEXITCODE -ne 0){throw 'Font/reset harness build failed'}
foreach($scale in @(1,2)){& "$PSScriptRoot\build\font-reset.exe" $scale $OutputRoot|Tee-Object (Join-Path $OutputRoot "$scale.txt");if($LASTEXITCODE -ne 0){throw 'Font/reset checks failed'}}
