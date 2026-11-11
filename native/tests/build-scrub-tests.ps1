param([switch]$SkipLibraryBuild)
$ErrorActionPreference='Stop'
$project=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$paths=& (Join-Path $project 'scripts\resolve-build-paths.ps1')
$compiler=Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe'
if(!$SkipLibraryBuild){& "$project\native\build-library-ui.ps1"}
& $compiler '-std=c++20' '-O2' '-Wall' '-Wextra' '-Wpedantic' '-static' '-I' "$project\native\ui\include" '-I' $paths.ImGui "$PSScriptRoot\scrub_tests.cpp" "$project\native\build\library_ui.lib" '-lshell32' '-o' "$project\native\build\scrub_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Scrub test compilation failed'}
& "$project\native\build\scrub_tests.exe" | Tee-Object "$project\native\build\scrub-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Scrub regression tests failed'}
