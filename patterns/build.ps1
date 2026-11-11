param([switch]$CoreOnly,[ValidateSet('Both','Grid','Checkerboard')][string]$BuildTarget='Both')
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$tool=$aepsPaths.Toolchain
$compiler=Join-Path $tool 'x86_64-w64-mingw32-clang++.exe'
$sdk=$aepsPaths.Sdk
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-static','-I',"$PSScriptRoot\include",'-I',"$project\native\ui\include",'-I',"$project\displacement\include",'-I',$aepsPaths.ImGui)
$flags+=$aepsPaths.CompilerFlags
& $compiler @flags "$PSScriptRoot\src\pattern_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" -o "$out\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Pattern core build failed'}
& "$out\core_tests.exe" | Tee-Object "$out\core-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Pattern core tests failed'}
if($CoreOnly){return}
& "$project\native\ui\verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
$libraries=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-ld3dcompiler','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
foreach($effect in @('Grid','Checkerboard')){
 if($BuildTarget -ne 'Both' -and $BuildTarget -ne $effect){continue}
 [string[]]$define=@();if($effect -eq 'Checkerboard'){$define=@('-DZAYU_CHECKERBOARD=1')}
 [IO.File]::WriteAllText("$out\PiPL-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL-$effect.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
 Push-Location "$PSScriptRoot\src"
 try {& "$tool\x86_64-w64-mingw32-windres.exe" --codepage=65001 -i pattern.rc -o "$out\$effect-resource.o" -O coff} finally {Pop-Location}
 if($LASTEXITCODE -ne 0){throw "$effect resources failed"}
 & $compiler @flags @hostFlags @define -shared "$PSScriptRoot\src\pattern_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\$effect-resource.o" @libraries -o "$out\ZaYu$effect.8bf"
 if($LASTEXITCODE -ne 0){throw "$effect plugin build failed"}
 & $compiler @flags @hostFlags @define "$PSScriptRoot\src\pattern_core.cpp" "$PSScriptRoot\tests\adapter_tests.cpp" @libraries -o "$out\adapter_$effect.exe"
 if($LASTEXITCODE -ne 0){throw "$effect adapter build failed"}
 & "$out\adapter_$effect.exe" | Tee-Object "$out\adapter-$effect-results.txt"
 if($LASTEXITCODE -ne 0){throw "$effect adapter tests failed"}
 & $compiler @flags @hostFlags @define "$PSScriptRoot\src\pattern_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libraries -o "$out\dialog_$effect.exe"
 if($LASTEXITCODE -ne 0){throw "$effect production UI harness failed"}
 Get-FileHash -LiteralPath "$out\ZaYu$effect.8bf"
}
