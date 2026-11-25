param([Parameter(Mandatory)][string]$Source,[Parameter(Mandatory)][string]$Parameters,[Parameter(Mandatory)][string]$Output,[string[]]$Only=@(),[string]$PhotoshopProgId='Photoshop.Application.140',[string]$ExpectedVersion='21.2.9')
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent
New-Item -ItemType Directory -Path $Output -Force|Out-Null
$rows=[IO.File]::ReadAllText([IO.Path]::GetFullPath($Parameters),[Text.Encoding]::UTF8)
$textureFolder=Join-Path ([IO.Path]::GetFullPath($Output)) 'texturewrap-inputs'
if (($rows|ConvertFrom-Json) | Where-Object { (!$Only.Count -or $_.slug -in $Only) -and $_.input -in @('globe','label') }) {
 & "$PSScriptRoot/new-texturewrap-gallery-inputs.ps1" -Output $textureFolder
}
$code='var GALLERY_SOURCE='+([IO.Path]::GetFullPath($Source).Replace('\','/')|ConvertTo-Json -Compress)+';var GALLERY_OUT='+([IO.Path]::GetFullPath($Output).Replace('\','/')|ConvertTo-Json -Compress)+';var GALLERY_ROWS='+$rows+';var GALLERY_ONLY='+(ConvertTo-Json -InputObject @($Only) -Compress)+';var GALLERY_TEXTURE_SOURCE='+($textureFolder.Replace('\','/')|ConvertTo-Json -Compress)+';'
foreach($helper in @('surface/tests/smart_helpers.jsxinc','creative/tests/smart_helpers.jsxinc','roughen/tests/smart_helpers.jsxinc','scripts/render-gallery.jsx')){$code+=[IO.File]::ReadAllText("$root/$helper",[Text.Encoding]::UTF8)+"`n"}
$app=New-Object -ComObject $PhotoshopProgId
try{if($app.Version -ne $ExpectedVersion){throw 'Unexpected Photoshop version; no document was changed'};$app.DoJavaScript($code)}finally{[void][Runtime.InteropServices.Marshal]::ReleaseComObject($app)}
