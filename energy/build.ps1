param([switch]$CoreOnly,[string]$EvidenceRoot='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$paths=& "$root/scripts/resolve-build-paths.ps1" -CoreOnly:$CoreOnly
$tool=$paths.Toolchain;$cxx=Join-Path $tool 'x86_64-w64-mingw32-clang++.exe';$sdk=$paths.Sdk;$out="$PSScriptRoot/build"
New-Item -ItemType Directory -Path $out -Force|Out-Null
if(!$EvidenceRoot){$EvidenceRoot="$root/evidence/energy-v07"}
New-Item -ItemType Directory -Path $EvidenceRoot -Force|Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-Wall','-Wextra','-Wpedantic','-I',"$PSScriptRoot/include",'-I',$out,'-I',"$root/native/ui/include",'-I',"$root/displacement/include",'-I',$paths.ImGui)+$paths.CompilerFlags
& $cxx @flags "$PSScriptRoot/src/energy_core.cpp" "$PSScriptRoot/tests/core_tests.cpp" -o "$out/core_tests.exe"
if($LASTEXITCODE){throw 'Energy core build failed'}
& "$out/core_tests.exe" "$EvidenceRoot/legacy-core"
if($LASTEXITCODE){throw 'Energy core tests failed'}
& $cxx @flags "$PSScriptRoot/src/energy_core.cpp" "$PSScriptRoot/tests/material_tests.cpp" -o "$out/material_tests.exe"
if($LASTEXITCODE){throw 'Energy material build failed'}
& "$out/material_tests.exe" "$EvidenceRoot/core"
if($LASTEXITCODE){throw 'Energy material tests failed'}
& $cxx @flags "$PSScriptRoot/src/energy_core.cpp" "$PSScriptRoot/tests/glow_tests.cpp" -o "$out/glow_tests.exe"
if($LASTEXITCODE){throw 'Energy layered-light build failed'}
& "$out/glow_tests.exe" | Tee-Object "$EvidenceRoot/glow-core.txt"
if($LASTEXITCODE){throw 'Energy layered-light tests failed'}
& $cxx @flags "$PSScriptRoot/src/energy_core.cpp" "$PSScriptRoot/tests/electric_tests.cpp" -o "$out/electric_tests.exe"
if($LASTEXITCODE){throw 'Energy electric build failed'}
& "$out/electric_tests.exe" | Tee-Object "$EvidenceRoot/electric-core.txt"
if($LASTEXITCODE){throw 'Energy electric tests failed'}
& $cxx @flags "$PSScriptRoot/src/energy_core.cpp" "$PSScriptRoot/tests/burning_tests.cpp" -o "$out/burning_tests.exe"
if($LASTEXITCODE){throw 'Energy burning build failed'}
& "$out/burning_tests.exe" | Tee-Object "$EvidenceRoot/burning-core.txt"
if($LASTEXITCODE){throw 'Energy burning tests failed'}
& $cxx @flags "$PSScriptRoot/src/energy_core.cpp" "$PSScriptRoot/tests/unified_tests.cpp" -o "$out/unified_tests.exe"
if($LASTEXITCODE){throw 'Energy common pipeline build failed'}
& "$out/unified_tests.exe" | Tee-Object "$EvidenceRoot/unified-core.txt"
if($LASTEXITCODE){throw 'Energy common pipeline tests failed'}
if($CoreOnly){return}
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk/photoshopapi/photoshop",'-isystem',"$sdk/photoshopapi/pica_sp")
$libs=@("$root/native/build/library_ui.lib","$root/native/build/psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
& "$root/scripts/build-legacy-resources.ps1" -Metadata "$PSScriptRoot/src/PiPL.json" -Output "$out/PiPL-legacy.rc" -Sdk $sdk
Push-Location "$PSScriptRoot/src";try{& "$tool/x86_64-w64-mingw32-windres.exe" --codepage=65001 -i EnergyBeam.rc -o "$out/resource.o" -O coff}finally{Pop-Location}
if($LASTEXITCODE){throw 'Energy resource build failed'}
$sources=@("$PSScriptRoot/src/energy_core.cpp","$PSScriptRoot/src/energy_host.cpp","$PSScriptRoot/src/library_dialog.cpp")
& $cxx @flags @hostFlags -shared @sources "$PSScriptRoot/src/filter.cpp" "$out/resource.o" @libs -o "$out/ZaYuSaber.8bf"
if($LASTEXITCODE){throw 'Energy DLL build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/panel_tests.cpp" @libs -o "$out/panel_tests.exe"
if($LASTEXITCODE){throw 'Energy UI harness build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/path_panel_tests.cpp" @libs -o "$out/path_panel_tests.exe"
if($LASTEXITCODE){throw 'Energy path UI harness build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/material_panel_tests.cpp" @libs -o "$out/material_panel_tests.exe"
if($LASTEXITCODE){throw 'Energy material UI harness build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/glow_panel_tests.cpp" @libs -o "$out/glow_panel_tests.exe"
if($LASTEXITCODE){throw 'Energy layered-light UI harness build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/electric_panel_tests.cpp" @libs -o "$out/electric_panel_tests.exe"
if($LASTEXITCODE){throw 'Energy electric UI harness build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/burning_panel_tests.cpp" @libs -o "$out/burning_panel_tests.exe"
if($LASTEXITCODE){throw 'Energy burning UI harness build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/unified_panel_tests.cpp" @libs -o "$out/unified_panel_tests.exe"
if($LASTEXITCODE){throw 'Energy common UI harness build failed'}
& $cxx @flags @hostFlags @sources "$PSScriptRoot/tests/unified_conversion_tests.cpp" @libs -o "$out/unified_conversion_tests.exe"
if($LASTEXITCODE){throw 'Energy conversion UI harness build failed'}
& $cxx @flags @hostFlags "$PSScriptRoot/src/energy_core.cpp" "$PSScriptRoot/src/energy_host.cpp" "$PSScriptRoot/tests/host_tests.cpp" @libs -o "$out/host_tests.exe"
if($LASTEXITCODE){throw 'Energy host harness build failed'}
& "$out/host_tests.exe"|Tee-Object "$EvidenceRoot/host-adapter.txt"
if($LASTEXITCODE){throw 'Energy host checks failed'}
Write-Output 'PASS: Saber 0.7 built locally; other filters were not rebuilt'
