param([switch]$CoreOnly,[ValidateSet("Both","Starglow","Shine")][string]$Effect="Both",[string]$Evidence="")
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$paths=& "$project\scripts\resolve-build-paths.ps1" -CoreOnly:$CoreOnly
$tool=$paths.Toolchain;$cxx=Join-Path $tool 'x86_64-w64-mingw32-clang++.exe';$sdk=$paths.Sdk;$out="$PSScriptRoot\build"
New-Item -ItemType Directory -Path $out -Force|Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-Wall','-Wextra','-Wpedantic','-I',"$PSScriptRoot\include",'-I',"$out",'-I',"$project\native\ui\include",'-I',"$project\displacement\include",'-I',$paths.ImGui)+$paths.CompilerFlags
& $cxx @flags "$PSScriptRoot\src\lightfx_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" -o "$out\core_tests.exe"
if($LASTEXITCODE){throw 'LightFX core build failed'}
if(!$Evidence){$Evidence="$project\evidence\lightfx-v01"}
& "$out\core_tests.exe" "$Evidence\core"|Tee-Object "$out\core-results.txt"
if($LASTEXITCODE){throw 'LightFX core tests failed'}
if($CoreOnly){return}
& "$project\native\ui\verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
$libs=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
$names=@('Starglow','Shine')
for($kind=0;$kind -lt 2;$kind++){
 $name=$names[$kind];if($Effect -ne "Both" -and $Effect -ne $name){continue};$variant=@("-DLIGHTFX_KIND=$kind")
 & "$project\scripts\build-legacy-resources.ps1" -Metadata "$PSScriptRoot\src\PiPL-$name.json" -Output "$out\PiPL-$name-legacy.rc" -Sdk $sdk
 Push-Location "$PSScriptRoot\src";try{& "$tool\x86_64-w64-mingw32-windres.exe" --codepage=65001 -i "$name.rc" -o "$out\$name-resource.o" -O coff}finally{Pop-Location}
 if($LASTEXITCODE){throw 'LightFX resources failed'}
 & $cxx @flags @hostFlags @variant -shared "$PSScriptRoot\src\lightfx_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\$name-resource.o" @libs -o "$out\ZaYu$name.8bf"
 if($LASTEXITCODE){throw 'LightFX DLL build failed'}
 & $cxx @flags @hostFlags @variant "$PSScriptRoot\src\lightfx_core.cpp" "$PSScriptRoot\tests\adapter_tests.cpp" @libs -o "$out\$name-adapter.exe"
 if($LASTEXITCODE){throw 'LightFX adapter build failed'}
 & "$out\$name-adapter.exe"|Tee-Object "$out\$name-adapter-results.txt"
 if($LASTEXITCODE){throw 'LightFX adapter checks failed'}
 & $cxx @flags @hostFlags @variant "$PSScriptRoot\src\lightfx_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libs -o "$out\$name-preview.exe"
 if($LASTEXITCODE){throw 'LightFX panel harness build failed'}
}
Write-Output "PASS: local LightFX build ($Effect); no installation or publication performed"
