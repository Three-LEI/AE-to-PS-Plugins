param(
    [ValidateSet('Sphere', 'Cylinder')][string[]]$Effect = @('Sphere', 'Cylinder'),
    [ValidateSet('create', 'reopen')][string]$Phase = 'create',
    [ValidateSet('2020', '2026')][string]$PhotoshopYear = '2020',
    [string]$OutputRoot,
    [switch]$CylinderSelfOverlay
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$existing = @(Get-Process -Name Photoshop -ErrorAction SilentlyContinue)
if ($existing | Where-Object { $_.MainWindowHandle -ne 0 -or $_.Threads.Count -gt 1 }) {
    throw 'A Photoshop session is already running.'
}
$catalog = Get-Content -LiteralPath "$root/native/common/parameter_fields.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$script = [IO.File]::ReadAllText("$PSScriptRoot/texturewrap-host.jsx", [Text.Encoding]::UTF8)

$progId, $expected = if ($PhotoshopYear -eq '2026') { 'Photoshop.Application.200', '27.7.0' }
                    else { 'Photoshop.Application.140', '21.2.9' }
$baselineEvidence = Join-Path $root 'evidence/texturewrap-v041'
$previousEvidence = Join-Path $root 'evidence/texturewrap-v04'
$legacyEvidence = Join-Path $root 'evidence/texturewrap-v03'
if ($PhotoshopYear -eq '2026') {
    $baselineEvidence = Join-Path $baselineEvidence 'ps2026'
    $previousEvidence = Join-Path $previousEvidence 'ps2026'
    $legacyEvidence = Join-Path $legacyEvidence 'ps2026'
}
$app = New-Object -ComObject $progId
$owned = $null
try {
    for ($attempt = 0; $attempt -lt 25; ++$attempt) {
        try { $version = $app.Version; break } catch {
            if ($attempt -eq 24) { throw }
            Start-Sleep -Seconds 1
        }
    }
    $processes = @(Get-Process -Name Photoshop | Where-Object { $_.Id -notin $existing.Id -and $_.Threads.Count -gt 1 })
    if ($processes.Count -ne 1 -or $version -ne $expected -or $app.Documents.Count -ne 0) {
        throw "Expected an owned, empty Photoshop $expected test session."
    }
    $owned = $processes[0]
    "Photoshop $version QA process $($owned.Id)"
    foreach ($name in $Effect) {
        $checkSelf = $CylinderSelfOverlay.IsPresent -and $name -eq 'Cylinder'
        $fixtureVersion = if ($checkSelf) { 'v06' } else { 'v05' }
        $evidence = if ($OutputRoot) { [IO.Path]::GetFullPath($OutputRoot) }
                    else { Join-Path $root "evidence/texturewrap-$($name.ToLowerInvariant())-$fixtureVersion" }
        if ($PhotoshopYear -eq '2026') { $evidence = Join-Path $evidence 'ps2026' }
        $out = Join-Path $evidence ($name.ToLowerInvariant())
        New-Item -ItemType Directory -Path $out -Force | Out-Null
        $profile = $catalog.profiles."texturewrap/src/PiPL-$name.json"
        $legacy = Join-Path $legacyEvidence ($name.ToLowerInvariant())
        $previous = Join-Path $previousEvidence ($name.ToLowerInvariant())
        $baseline = Join-Path $baselineEvidence ($name.ToLowerInvariant())
        if (!(Test-Path -LiteralPath $legacy -PathType Container)) { $legacy = '' }
        if (!(Test-Path -LiteralPath $previous -PathType Container)) { $previous = '' }
        if (!(Test-Path -LiteralPath $baseline -PathType Container)) { $baseline = '' }
        $priorCylinder = ''
        if ($checkSelf) {
            $priorEvidence = Join-Path $root 'evidence/texturewrap-cylinder-v05'
            if ($PhotoshopYear -eq '2026') { $priorEvidence = Join-Path $priorEvidence 'ps2026' }
            $priorCylinder = Join-Path $priorEvidence 'cylinder'
            if (!(Test-Path -LiteralPath $priorCylinder -PathType Container)) { $priorCylinder = '' }
        }
        $config = [ordered]@{ out = $out; legacy = $legacy; previous = $previous; baseline = $baseline; priorCylinder = $priorCylinder; selfOverlay = $checkSelf; phase = $Phase; effect = $name; event = $profile.event; magic = $profile.magic }
        $code = 'var TEXTURE_WRAP = ' + (ConvertTo-Json -InputObject $config -Compress) + ';' + $script
        $app.DoJavaScript($code)
    }
} finally {
    try { if ($owned -and $app.Documents.Count -eq 0) { $app.Quit() } }
    finally {
        [void][Runtime.InteropServices.Marshal]::ReleaseComObject($app)
        [GC]::Collect()
        [GC]::WaitForPendingFinalizers()
    }
}
if ($owned -and !$owned.WaitForExit(12000)) {
    $residue = Get-Process -Id $owned.Id -ErrorAction SilentlyContinue
    if ($residue -and $residue.MainWindowHandle -eq 0 -and $residue.Threads.Count -eq 1) {
        Stop-Process -Id $residue.Id
    } elseif ($residue) { throw 'The test session did not exit.' }
}
