param([switch]$CoreOnly)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$tool=$aepsPaths.Toolchain
$cxx=Join-Path $tool 'x86_64-w64-mingw32-clang++.exe'
$sdk=$aepsPaths.Sdk
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-static','-I',"$PSScriptRoot\include",'-I',"$project\native\ui\include",'-I',"$project\displacement\include",'-I',$aepsPaths.ImGui)
$flags+=$aepsPaths.CompilerFlags
& $cxx @flags "$PSScriptRoot\src\vector_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" -o "$out\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Vector Blur core build failed'}
& "$out\core_tests.exe" | Tee-Object "$out\core-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Vector Blur core tests failed'}
if($CoreOnly){return}
& "$project\native\ui\verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
[IO.File]::WriteAllText("$out\PiPL-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
Push-Location "$PSScriptRoot\src"
try{& "$tool\x86_64-w64-mingw32-windres.exe" --codepage=65001 -i vector.rc -o "$out\resource.o" -O coff}finally{Pop-Location}
if($LASTEXITCODE -ne 0){throw 'Vector Blur resources failed'}
$libs=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-ld3dcompiler','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
& $cxx @flags @hostFlags -shared "$PSScriptRoot\src\vector_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\resource.o" @libs -o "$out\ZaYuVectorBlur.8bf"
if($LASTEXITCODE -ne 0){throw 'Vector Blur plugin build failed'}
& $cxx @flags @hostFlags "$PSScriptRoot\src\vector_core.cpp" "$PSScriptRoot\tests\adapter_tests.cpp" @libs -o "$out\adapter_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Vector Blur adapter build failed'}
& "$out\adapter_tests.exe" | Tee-Object "$out\adapter-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Vector Blur adapter tests failed'}
$referenceInput=Join-Path $project 'evidence\vector-ae-reference\ae-noise-source.ppm'
if(Test-Path -LiteralPath $referenceInput){
 & $cxx @flags @hostFlags "$PSScriptRoot\tests\native_reference.cpp" -o "$out\native_reference.exe"
 if($LASTEXITCODE -ne 0){throw 'Native reference harness failed'}
 $referenceFolder=Join-Path $project 'evidence\vector-v02'
 New-Item -ItemType Directory -Path $referenceFolder -Force | Out-Null
 & "$out\native_reference.exe" $referenceInput (Join-Path $referenceFolder 'native-reference.ppm')
 if($LASTEXITCODE -ne 0){throw 'Native reference render failed'}
}
& $cxx @flags @hostFlags "$PSScriptRoot\src\vector_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libs -o "$out\dialog_preview.exe"
if($LASTEXITCODE -ne 0){throw 'Vector Blur production dialog harness failed'}
Get-FileHash -LiteralPath "$out\ZaYuVectorBlur.8bf"
