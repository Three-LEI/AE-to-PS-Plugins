param([switch]$CoreOnly)
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent
$paths=& "$root/scripts/resolve-build-paths.ps1" -CoreOnly:$CoreOnly
$cxx=Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe';$out="$PSScriptRoot/build";$evidence="$root/evidence/flare-v03"
New-Item -ItemType Directory -Path $out,$evidence -Force|Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-Wall','-Wextra','-Wpedantic','-I',"$PSScriptRoot/include",'-I',$out,'-I',"$root/native/ui/include",'-I',"$root/displacement/include",'-I',$paths.ImGui)+$paths.CompilerFlags
& $cxx @flags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/core_tests.cpp" -o "$out/core_tests.exe"
if($LASTEXITCODE){throw 'Flare core build failed'}
& "$out/core_tests.exe" "evidence/flare-v03/deterministic.f32" "evidence/flare-v03" | Tee-Object "$evidence/core-results.txt"
if($LASTEXITCODE){throw 'Flare core checks failed'}
& $cxx @flags -DAEPS_FORCE_SERIAL "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/core_tests.cpp" -o "$out/core_serial.exe"
if($LASTEXITCODE){throw 'Flare serial core build failed'}
& "$out/core_serial.exe" 'evidence/flare-v03/deterministic-serial.f32' | Tee-Object "$evidence/core-serial-results.txt"
if($LASTEXITCODE){throw 'Flare serial core checks failed'}
if((Get-FileHash "$evidence/deterministic.f32").Hash -ne (Get-FileHash "$evidence/deterministic-serial.f32").Hash){throw 'Flare serial/parallel pixels differ'}
'PASS: eight serial/parallel float output fixtures are byte-identical'
& $cxx @flags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/design_tests.cpp" -o "$out/design_tests.exe"
if($LASTEXITCODE){throw 'Designer core build failed'}
& "$out/design_tests.exe" "$evidence/designer.f32" | Tee-Object "$evidence/design-results.txt"
if($LASTEXITCODE){throw 'Designer core checks failed'}
& $cxx @flags -DAEPS_FORCE_SERIAL "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/design_tests.cpp" -o "$out/design_serial.exe"
if($LASTEXITCODE){throw 'Designer serial build failed'}
& "$out/design_serial.exe" "$evidence/designer-serial.f32" | Tee-Object "$evidence/design-serial-results.txt"
if($LASTEXITCODE){throw 'Designer serial checks failed'}
if((Get-FileHash "$evidence/designer.f32").Hash -ne (Get-FileHash "$evidence/designer-serial.f32").Hash){throw 'Designer serial/parallel pixels differ'}
'PASS: twelve designer serial/parallel float fixtures are byte-identical'
& $cxx @flags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/thumbnail_tests.cpp" -o "$out/thumbnail_tests.exe"
if($LASTEXITCODE){throw 'Thumbnail harness build failed'}
& "$out/thumbnail_tests.exe" | Tee-Object "$evidence/thumbnail-results.txt"
if($LASTEXITCODE){throw 'Thumbnail cache checks failed'}
if($CoreOnly){return}
& "$root/native/ui/verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")
$libs=@("$root/native/build/library_ui.lib","$root/native/build/psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32','-lcomdlg32')
& "$root/scripts/build-legacy-resources.ps1" -Metadata "$PSScriptRoot/src/PiPL.json" -Output "$out/PiPL-legacy.rc" -Sdk $paths.Sdk
Push-Location "$PSScriptRoot/src";try{& "$($paths.Toolchain)/x86_64-w64-mingw32-windres.exe" --codepage=65001 -i Flare.rc -o "$out/resource.o" -O coff}finally{Pop-Location}
if($LASTEXITCODE){throw 'Flare resources failed'}
& $cxx @flags @hostFlags -shared "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/src/filter.cpp" "$PSScriptRoot/src/library_dialog.cpp" "$out/resource.o" @libs -o "$out/ZaYuLensFlare.8bf"
if($LASTEXITCODE){throw 'Flare DLL build failed'}
& $cxx @flags @hostFlags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/adapter_tests.cpp" @libs -o "$out/adapter_tests.exe"
if($LASTEXITCODE){throw 'Flare adapter build failed'}
& "$out/adapter_tests.exe" | Tee-Object "$evidence/adapter-results.txt"
if($LASTEXITCODE){throw 'Flare adapter checks failed'}
& $cxx @flags @hostFlags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/settings_tests.cpp" @libs -o "$out/settings_tests.exe"
if($LASTEXITCODE){throw 'Designer settings build failed'}
& "$out/settings_tests.exe" "$evidence/preset-library" | Tee-Object "$evidence/settings-results.txt"
if($LASTEXITCODE){throw 'Designer settings checks failed'}
& $cxx @flags @hostFlags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/tests/fixture_writer.cpp" -o "$out/fixture_writer.exe"
if($LASTEXITCODE){throw 'Flare fixture writer build failed'}
& "$out/fixture_writer.exe" "$evidence/host-fixtures.json"
if($LASTEXITCODE){throw 'Flare host fixtures failed'}
& $cxx @flags @hostFlags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/src/library_dialog.cpp" "$PSScriptRoot/tests/panel_tests.cpp" @libs -o "$out/panel_tests.exe"
if($LASTEXITCODE){throw 'Flare UI harness build failed'}
'PASS local Flare build; existing filters were not rebuilt'

& $cxx @flags @hostFlags "$PSScriptRoot/src/flare_core.cpp" "$PSScriptRoot/src/library_dialog.cpp" "$PSScriptRoot/tests/designer_panel_tests.cpp" @libs -o "$out/designer_panel_tests.exe"
if($LASTEXITCODE){throw 'Designer UI harness build failed'}
