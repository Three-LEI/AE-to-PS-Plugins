param([Parameter(Mandatory=$true)][string]$Report)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$paths=& "$project\scripts\resolve-build-paths.ps1"
$build=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $build -Force|Out-Null
$metadata=Get-Content -LiteralPath "$project\native\src\TintPiPL.json" -Raw -Encoding utf8|ConvertFrom-Json
$item=$metadata.PiPLs[0];$item.Name='AEPS Locale Probe';$item.Category='AEPS Development';$item.ComponentName='AEPSLocaleProbeDevelopment'
$item.ZStringName='$$$/AEPS/QA/LocaleProbe=AEPS Locale Probe';$item.ZStringCategory='$$$/AEPS/QA/Development=AEPS Development'
$metadata|ConvertTo-Json -Depth 15|Set-Content -LiteralPath "$build\locale-probe.json" -Encoding utf8
& "$project\scripts\build-legacy-resources.ps1" -Metadata "$build\locale-probe.json" -Output "$build\locale-probe.rc" -Sdk $paths.Sdk
& (Join-Path $paths.Toolchain 'x86_64-w64-mingw32-windres.exe') '--codepage=65001' '-i' "$build\locale-probe.rc" '-o' "$build\locale-probe-resource.o" '-O' coff
if($LASTEXITCODE -ne 0){throw 'Probe resource build failed'}
$define='-DAEPS_LOCALE_QA_OUTPUT=L"'+[IO.Path]::GetFullPath($Report).Replace('\','/')+'"'
& (Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe') -std=c++20 -O2 -static -shared -DWIN32=1 -D_WINDOWS -DUNICODE -D_UNICODE $define -I "$project\native\ui\include" -isystem "$($paths.Sdk)\photoshopapi\photoshop" -isystem "$($paths.Sdk)\photoshopapi\pica_sp" "$PSScriptRoot\locale-probe.cpp" "$project\native\ui\src\photoshop_host.cpp" "$build\locale-probe-resource.o" -luser32 -lgdi32 -o "$build\AEPSLocaleProbe.8bf"
if($LASTEXITCODE -ne 0){throw 'Probe build failed'}
Write-Output 'PASS: development-only native locale probe built; not installed'
