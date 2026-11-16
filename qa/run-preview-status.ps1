$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent;$out="$root/evidence/ui-buttons-v01/overlay"
New-Item -ItemType Directory -Path $out -Force|Out-Null
$paths=& "$root/scripts/resolve-build-paths.ps1"
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-I',"$root/native/ui/include",'-I',"$root/displacement/include",'-I',$paths.ImGui,'-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")+$paths.CompilerFlags
$libs=@("$root/native/build/library_ui.lib","$root/native/build/psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
& "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" @flags "$root/qa/preview-status.cpp" @libs -o "$out/preview-status.exe"
if($LASTEXITCODE){throw 'Overlay harness build failed'}
& "$out/preview-status.exe" $out | Tee-Object "$out/results.txt"
if($LASTEXITCODE){throw 'Overlay harness failed'}
