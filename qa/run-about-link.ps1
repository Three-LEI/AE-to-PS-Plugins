param([string]$OutputRoot=(Join-Path (Split-Path $PSScriptRoot -Parent) 'evidence\open-source-0.34\about'),[switch]$LegacyDpi)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$paths=& "$project\scripts\resolve-build-paths.ps1"
$compiler=Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe'
$build=Join-Path $PSScriptRoot 'build';New-Item -ItemType Directory -Path $build,$OutputRoot -Force|Out-Null
& $compiler -std=c++20 -O2 -static -DWIN32=1 -D_WINDOWS -DUNICODE -D_UNICODE -I "$project\native\ui\include" -I $paths.ImGui -isystem "$($paths.Sdk)\photoshopapi\photoshop" -isystem "$($paths.Sdk)\photoshopapi\pica_sp" "$PSScriptRoot\about-link.cpp" "$project\native\build\library_ui.lib" "$project\native\build\psui.lib" -ld3d11 -ldxgi -limm32 -luser32 -lgdi32 -ldwmapi -lshell32 -o "$build\about-link.exe"
if($LASTEXITCODE -ne 0){throw 'About link harness compilation failed'}
foreach($language in @('zh_CN','en_US')){foreach($theme in 0..3){foreach($scale in @(1,2)){
 $log=Join-Path $OutputRoot "$language-$theme-$scale.txt"
 [string[]]$dpiArgs=@();if($LegacyDpi){$dpiArgs=@('legacy-dpi')}
 & "$build\about-link.exe" $language $theme $scale $OutputRoot @dpiArgs|Tee-Object $log
 if($LASTEXITCODE -ne 0){throw "About link failed: $language theme=$theme scale=$scale"}
}}}
