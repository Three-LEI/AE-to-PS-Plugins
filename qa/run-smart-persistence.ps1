param([ValidateSet('create','reopen')][string]$Phase='create',[string]$OutputRoot=(Join-Path (Split-Path $PSScriptRoot -Parent) 'evidence\persistence-0.36\smart'),[string]$ExpectedVersion='21.2.9')
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Path $OutputRoot -Force|Out-Null
$fixtures=Get-Content -LiteralPath "$root\evidence\persistence-0.36\new-effect-fixtures.json" -Raw -Encoding utf8|ConvertFrom-Json
$entries=Get-Content -LiteralPath "$root\plugins.json" -Raw -Encoding utf8|ConvertFrom-Json
$cases=@();foreach($entry in $entries){
 $metadata=Get-ChildItem -LiteralPath "$root\$($entry.module)\src" -Filter '*PiPL*.json'|ForEach-Object{(Get-Content -LiteralPath $_.FullName -Raw -Encoding utf8|ConvertFrom-Json).PiPLs[0]}|Where-Object { $_.ZStringName.Split('=')[-1] -eq $entry.name }
 if(!$metadata){throw "No parameter metadata for $($entry.name)"}
 $event=$metadata.HasTerminology.EventID;$keys=@($metadata.Terminology.Suites[0].Events[0].Parameters|ForEach-Object{$_.ParamKeyword})
 $case=@{name=$entry.name;module=$entry.module;event=$event;keys=$keys}
 if($entry.module -eq 'surface'){$case.legacy='surface';$case.kind=if($entry.file -match 'Blobby'){0}else{1}}
 elseif($entry.module -eq 'creative'){$case.legacy='creative';$case.kind=if($entry.file -match 'Turbulent'){0}elseif($entry.file -match 'Sweep'){1}else{2}}
 elseif($entry.module -eq 'roughen'){$case.legacy='roughen'}
 else{$case.fixtures=@();foreach($variant in 1..2){$row=$fixtures|Where-Object {$_.event -eq $event -and $_.variant -eq $variant};$case.fixtures+=,@($row.fields)}}
 $cases+=$case
}
$code='var AEPS_SMART_OUT='+ (ConvertTo-Json ([IO.Path]::GetFullPath($OutputRoot).Replace('\','/')) -Compress)+";`n"+'var AEPS_SMART_PHASE="'+$Phase+'";'+"`n"+'var AEPS_SMART_CASES='+ (ConvertTo-Json $cases -Depth 20 -Compress)+";`n"
foreach($helper in @('surface/tests/smart_helpers.jsxinc','creative/tests/smart_helpers.jsxinc','roughen/tests/smart_helpers.jsxinc')){$code+=[IO.File]::ReadAllText((Join-Path $root $helper),[Text.Encoding]::UTF8)+"`n"}
$code+=[IO.File]::ReadAllText("$PSScriptRoot\smart-persistence.jsx",[Text.Encoding]::UTF8)
[IO.File]::WriteAllText((Join-Path $OutputRoot 'cases.json'),(ConvertTo-Json $cases -Depth 20),[Text.Encoding]::UTF8)
$ps=New-Object -ComObject Photoshop.Application
try{if($ps.Version -ne $ExpectedVersion -or $ps.Documents.Count -ne 0){throw 'Expected specified Photoshop version and empty session'};$ps.DoJavaScript($code)}finally{[void][Runtime.InteropServices.Marshal]::ReleaseComObject($ps)}
