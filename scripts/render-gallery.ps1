param([Parameter(Mandatory)][string]$Source,[Parameter(Mandatory)][string]$Parameters,[Parameter(Mandatory)][string]$Output,[string[]]$Only=@(),[string]$PhotoshopProgId='Photoshop.Application.140',[string]$ExpectedVersion='21.2.9')
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Path $Output -Force|Out-Null
$code='var GALLERY_SOURCE='+([IO.Path]::GetFullPath($Source).Replace('\','/')|ConvertTo-Json -Compress)+';var GALLERY_OUT='+([IO.Path]::GetFullPath($Output).Replace('\','/')|ConvertTo-Json -Compress)+';var GALLERY_ROWS='+[IO.File]::ReadAllText([IO.Path]::GetFullPath($Parameters),[Text.Encoding]::UTF8)+';var GALLERY_ONLY='+(ConvertTo-Json -InputObject @($Only) -Compress)+';'
foreach($helper in @('surface/tests/smart_helpers.jsxinc','creative/tests/smart_helpers.jsxinc','roughen/tests/smart_helpers.jsxinc','scripts/render-gallery.jsx')){$code+=[IO.File]::ReadAllText("$root/$helper",[Text.Encoding]::UTF8)+"`n"}
$app=New-Object -ComObject $PhotoshopProgId
try{if($app.Version -ne $ExpectedVersion){throw 'Unexpected Photoshop version; no document was changed'};$app.DoJavaScript($code)}finally{[void][Runtime.InteropServices.Marshal]::ReleaseComObject($app)}
