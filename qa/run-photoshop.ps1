param([ValidateSet('baseline','updated')][string]$Phase='updated',[string]$OutputRoot=(Join-Path (Split-Path $PSScriptRoot -Parent) 'evidence\open-source-0.34'),[string]$ExpectedVersion,[switch]$UseEmptySession)
$ErrorActionPreference='Stop'
if(!$UseEmptySession -and (Get-Process Photoshop -ErrorAction SilentlyContinue)){throw 'Close Photoshop before running this disposable-document check; an existing session is never closed.'}
$folder=Join-Path $OutputRoot ('host-'+$Phase)
New-Item -ItemType Directory -Path $folder -Force|Out-Null
$outputLiteral=ConvertTo-Json ([IO.Path]::GetFullPath($folder).Replace('\','/')) -Compress
$code='var AEPS_QA_OUTPUT = '+$outputLiteral+";`n"+[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'host-regression.jsx'),[Text.Encoding]::UTF8)
$ps=New-Object -ComObject Photoshop.Application
try{if($ExpectedVersion -and $ps.Version -ne $ExpectedVersion){throw "COM connected to Photoshop $($ps.Version); expected $ExpectedVersion"};if($ps.Documents.Count -ne 0){throw 'Disposable regression requires an empty Photoshop session'};$ps.DisplayDialogs=3;$ps.DoJavaScript($code)}finally{
 if(!$UseEmptySession -and $ps.Documents.Count -eq 0){$owned=Get-Process Photoshop -ErrorAction SilentlyContinue;$ps.Quit();foreach($process in $owned){[void]$process.WaitForExit(10000)}}
 [Runtime.InteropServices.Marshal]::ReleaseComObject($ps)|Out-Null
}
