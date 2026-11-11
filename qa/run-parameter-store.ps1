param([string]$OutputRoot=(Join-Path (Split-Path $PSScriptRoot -Parent) 'evidence\persistence-0.36'))
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$paths=& "$project\scripts\resolve-build-paths.ps1"
$build=Join-Path $PSScriptRoot 'build';New-Item -ItemType Directory -Path $build,$OutputRoot -Force|Out-Null
$catalog=Get-Content -LiteralPath "$project\native\common\parameter_fields.json" -Raw -Encoding utf8|ConvertFrom-Json
$includes=@();$units=@();$coreFlags=@()
foreach($entry in $catalog.modules.PSObject.Properties){
 $module=$entry.Name
 $coreFlags+=@('-I',"$project\$module\include")
 $core=Get-ChildItem -LiteralPath "$project\$module\include" -Filter '*core.hpp'|Select-Object -First 1
 $includes+='#include "'+$core.FullName.Replace('\','/')+'"'
 $includes+='#include "'+("$project\$module\build\parameter_schema.hpp").Replace('\','/')+'"'
 $units+=(Get-ChildItem -LiteralPath "$project\$module\src" -Filter '*core.cpp'|Select-Object -First 1).FullName
}
[IO.File]::WriteAllLines("$build\parameter-test-includes.inc",$includes,[Text.Encoding]::UTF8)
$cases=@()
foreach($entry in $catalog.profiles.PSObject.Properties){
 $module=$entry.Name.Split('/')[0];$profile=$entry.Value;$type=$catalog.modules.$module.type;$namespace=$type.Split(':')[0]
 $valid=if($module -eq 'native'){'tint_valid'}else{$namespace+'::valid'}
 $kind=if($entry.Name -match 'DeepGlow|CCLens'){1}else{0}
 $magic='0x'+('{0:X8}' -f [uint32]$profile.magic)+'u'
 $cases+='run<'+$type+','+$valid+','+$magic+'>("'+$profile.event+'",'+$kind+',out,comma);'
}
[IO.File]::WriteAllLines("$build\parameter-test-cases.inc",$cases,[Text.Encoding]::UTF8)
& (Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe') -std=c++20 -O2 -ffp-contract=off -static -DWIN32=1 -D_WINDOWS -DUNICODE -D_UNICODE -I $build -I "$project\native\ui\include" @coreFlags -isystem "$($paths.Sdk)\photoshopapi\photoshop" -isystem "$($paths.Sdk)\photoshopapi\pica_sp" "$PSScriptRoot\parameter-store.cpp" @units -o "$build\parameter-store.exe"
if($LASTEXITCODE -ne 0){throw 'Parameter store harness build failed'}
& "$build\parameter-store.exe" (Join-Path $OutputRoot 'new-effect-fixtures.json') | Tee-Object (Join-Path $OutputRoot 'parameter-store.log')
if($LASTEXITCODE -ne 0){throw 'Parameter store tests failed'}
