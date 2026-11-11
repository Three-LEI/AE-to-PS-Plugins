param([Parameter(Mandatory=$true)][string]$Metadata,[Parameter(Mandatory=$true)][string]$Output,[Parameter(Mandatory=$true)][string]$Sdk)
$ErrorActionPreference='Stop'
$python=if($env:AEPS_PYTHON){$env:AEPS_PYTHON}else{(Get-Command python.exe -ErrorAction Stop).Source}
& $python (Join-Path $PSScriptRoot 'generate-parameter-schema.py') --metadata $Metadata --output (Join-Path (Split-Path $Output -Parent) 'parameter_schema.hpp')
if($LASTEXITCODE -ne 0){throw 'Parameter schema generation failed'}
& $python (Join-Path $PSScriptRoot 'generate-legacy-pipl.py') --metadata $Metadata --output $Output --sdk $Sdk
if($LASTEXITCODE -ne 0){throw 'Legacy PiPL/aete conversion failed'}
