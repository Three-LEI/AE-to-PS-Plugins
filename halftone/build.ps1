param([switch]$CoreOnly)
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent
$paths=& "$root/scripts/resolve-build-paths.ps1" -CoreOnly:$CoreOnly
$cxx=Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe';$out="$PSScriptRoot/build";$evidence="$root/evidence/halftone-v01"
New-Item -ItemType Directory -Path $out,$evidence -Force|Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-Wall','-Wextra','-Wpedantic','-I',"$PSScriptRoot/include",'-I',$out,'-I',"$root/native/ui/include",'-I',"$root/displacement/include",'-I',$paths.ImGui)+$paths.CompilerFlags
& $cxx @flags "$PSScriptRoot/src/halftone_core.cpp" "$PSScriptRoot/tests/core_tests.cpp" -o "$out/core_tests.exe"
if($LASTEXITCODE){throw 'Halftone core build failed'}
& "$out/core_tests.exe" "evidence/halftone-v01/deterministic.f32" | Tee-Object "$evidence/core-results.txt"
if($LASTEXITCODE){throw 'Halftone core checks failed'}
& $cxx @flags -DAEPS_FORCE_SERIAL "$PSScriptRoot/src/halftone_core.cpp" "$PSScriptRoot/tests/core_tests.cpp" -o "$out/core_serial.exe"
if($LASTEXITCODE){throw 'Halftone serial core build failed'}
& "$out/core_serial.exe" 'evidence/halftone-v01/deterministic-serial.f32' | Tee-Object "$evidence/core-serial-results.txt"
if($LASTEXITCODE){throw 'Halftone serial core checks failed'}
if((Get-FileHash "$evidence/deterministic.f32").Hash -ne (Get-FileHash "$evidence/deterministic-serial.f32").Hash){throw 'Halftone serial/parallel pixels differ'}
'PASS: six serial/parallel float output fixtures are byte-identical'
if($CoreOnly){return}
& "$root/native/ui/verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")
$libs=@("$root/native/build/library_ui.lib","$root/native/build/psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
& "$root/scripts/build-legacy-resources.ps1" -Metadata "$PSScriptRoot/src/PiPL.json" -Output "$out/PiPL-legacy.rc" -Sdk $paths.Sdk
Push-Location "$PSScriptRoot/src";try{& "$($paths.Toolchain)/x86_64-w64-mingw32-windres.exe" --codepage=65001 -i Halftone.rc -o "$out/resource.o" -O coff}finally{Pop-Location}
if($LASTEXITCODE){throw 'Halftone resources failed'}
& $cxx @flags @hostFlags -shared "$PSScriptRoot/src/halftone_core.cpp" "$PSScriptRoot/src/filter.cpp" "$PSScriptRoot/src/library_dialog.cpp" "$out/resource.o" @libs -o "$out/ZaYuHalftone.8bf"
if($LASTEXITCODE){throw 'Halftone DLL build failed'}
& $cxx @flags @hostFlags "$PSScriptRoot/src/halftone_core.cpp" "$PSScriptRoot/tests/adapter_tests.cpp" @libs -o "$out/adapter_tests.exe"
if($LASTEXITCODE){throw 'Halftone adapter build failed'}
& "$out/adapter_tests.exe" | Tee-Object "$evidence/adapter-results.txt"
if($LASTEXITCODE){throw 'Halftone adapter checks failed'}
& $cxx @flags @hostFlags "$PSScriptRoot/src/halftone_core.cpp" "$PSScriptRoot/src/library_dialog.cpp" "$PSScriptRoot/tests/panel_tests.cpp" @libs -o "$out/panel_tests.exe"
if($LASTEXITCODE){throw 'Halftone UI harness build failed'}
'PASS local Halftone build; existing filters were not rebuilt'
