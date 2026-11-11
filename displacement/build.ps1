param([switch]$CoreOnly,[switch]$PortProbe)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$compilerRoot=$aepsPaths.Toolchain
$compiler=Join-Path $compilerRoot 'x86_64-w64-mingw32-clang++.exe'
$sdk=$aepsPaths.Sdk
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-static','-I',"$PSScriptRoot\include",'-I',"$project\native\ui\include",'-I',$aepsPaths.ImGui)
$flags+=$aepsPaths.CompilerFlags
$flags+=@('-I',"$PSScriptRoot\build")
if($PortProbe){$flags+='-DZAYU_PORT_PROBE'}
& $compiler @flags "$PSScriptRoot\src\displacement_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" -o "$out\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Displacement core build failed'}
& "$out\core_tests.exe" | Tee-Object "$out\core-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Displacement core tests failed'}
if($CoreOnly){return}
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
[IO.File]::WriteAllText("$out\PiPL-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
 & (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\build-legacy-resources.ps1") -Metadata "$PSScriptRoot\src\PiPL.json" -Output "$out\PiPL-legacy.rc" -Sdk $aepsPaths.Sdk
Push-Location "$PSScriptRoot\src"
try{& (Join-Path $compilerRoot 'x86_64-w64-mingw32-windres.exe') --codepage=65001 -i displacement.rc -o "$out\resource.o" -O coff}finally{Pop-Location}
if($LASTEXITCODE -ne 0){throw 'Displacement resource build failed'}
$libraries=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
& $compiler @flags @hostFlags -shared "$PSScriptRoot\src\displacement_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\resource.o" @libraries -o "$out\ZaYuDisplacementMap.8bf"
if($LASTEXITCODE -ne 0){throw 'Displacement plugin build failed'}
& $compiler @flags @hostFlags "$PSScriptRoot\tests\adapter_tests.cpp" @libraries -o "$out\adapter_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Displacement adapter build failed'}
& "$out\adapter_tests.exe" | Tee-Object "$out\adapter-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Displacement adapter tests failed'}
& $compiler @flags @hostFlags "$PSScriptRoot\src\displacement_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libraries -o "$out\dialog_preview.exe"
if($LASTEXITCODE -ne 0){throw 'Displacement dialog harness build failed'}
Get-FileHash -LiteralPath "$out\ZaYuDisplacementMap.8bf"
