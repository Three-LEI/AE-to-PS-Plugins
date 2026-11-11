param([Parameter(Mandatory=$true)][string]$PluginDirectory,[string]$Evidence=(Join-Path (Split-Path $PSScriptRoot -Parent) 'evidence\open-source-0.34'))
$ErrorActionPreference='Stop'
if(Get-Process Photoshop -ErrorAction SilentlyContinue){throw 'Close Photoshop before installing; an existing session is never closed.'}
$root=Split-Path $PSScriptRoot -Parent
$baseline=Get-Content -LiteralPath (Join-Path $Evidence 'baseline.json') -Raw -Encoding utf8|ConvertFrom-Json
$qa=Get-Content -LiteralPath (Join-Path $Evidence 'preinstall.json') -Raw -Encoding utf8|ConvertFrom-Json
if($qa.status -ne 'PASS' -or $qa.plugins.Count -ne 19){throw 'Verified nineteen-filter preinstall report required'}
foreach($entry in $baseline.plugins){if((Get-FileHash -LiteralPath (Join-Path $PluginDirectory $entry.file)).Hash.ToLowerInvariant() -ne $entry.sha256){throw "Installed baseline differs: $($entry.file)"}}
foreach($entry in $qa.plugins){if((Get-FileHash -LiteralPath $entry.path).Hash.ToLowerInvariant() -ne $entry.sha256){throw 'Candidate binary changed after verification'}}
foreach($source in $qa.source_files.PSObject.Properties){if((Get-FileHash -LiteralPath (Join-Path $root $source.Name)).Hash.ToLowerInvariant() -ne $source.Value){throw "Source changed after verification: $($source.Name)"}}
$backup=Join-Path $root ('backups\open-source-install-'+(Get-Date -Format 'yyyyMMdd-HHmmss'));New-Item -ItemType Directory -Path $backup -Force|Out-Null
foreach($entry in $qa.plugins){Copy-Item -LiteralPath (Join-Path $PluginDirectory $entry.file) -Destination (Join-Path $backup $entry.file)}
foreach($entry in $qa.plugins){
 Copy-Item -LiteralPath $entry.path -Destination (Join-Path $PluginDirectory $entry.file) -Force
 if((Get-FileHash -LiteralPath (Join-Path $PluginDirectory $entry.file)).Hash.ToLowerInvariant() -ne $entry.sha256){throw 'Installed binary verification failed'}
}
@{status='PASS';release='0.34';plugins=19;backup=$backup;source_repository=$qa.repository}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $Evidence 'installed.json') -Encoding utf8
Write-Output 'PASS: nineteen source-link builds backed up and installed; no scripts installed'
