param([Parameter(Mandatory)][string]$Output)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$folder = [IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Path $folder -Force | Out-Null

function Color([string]$Hex) { [Drawing.ColorTranslator]::FromHtml($Hex) }
function Save-Texture([string]$Slug, [scriptblock]$Paint) {
    $bitmap = [Drawing.Bitmap]::new(768, 768, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.TextRenderingHint = [Drawing.Text.TextRenderingHint]::AntiAliasGridFit
        & $Paint $graphics
        $bitmap.Save((Join-Path $folder "$Slug-before.png"), [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$paintLabel = {
    param($graphics)
    $graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::None
    $paper = [Drawing.SolidBrush]::new((Color '#103C39'))
    $cream = [Drawing.SolidBrush]::new((Color '#FFE4A0'))
    $secondary = [Drawing.SolidBrush]::new((Color '#5EB29A'))
    $name = [Drawing.Font]::new('Segoe UI', 70, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
    $caption = [Drawing.Font]::new('Segoe UI', 22, [Drawing.FontStyle]::Regular, [Drawing.GraphicsUnit]::Pixel)
    $align = [Drawing.StringFormat]::new()
    $align.Alignment = [Drawing.StringAlignment]::Center
    try {
        $graphics.FillRectangle($paper, 64, 244, 640, 280)
        foreach ($y in 265, 497) { $graphics.FillRectangle($cream, 64, $y, 640, 5) }
        for ($x = 77; $x -lt 700; $x += 20) {
            $graphics.FillRectangle($secondary, $x, 280, 3, 35)
            $graphics.FillRectangle($secondary, $x, 452, 3, 35)
        }
        $graphics.DrawString('ORBIT', $name, $cream, [Drawing.PointF]::new(384, 327), $align)
        $graphics.DrawString('BOTANICAL  /  07', $caption, $cream, [Drawing.PointF]::new(384, 413), $align)
    } finally {
        $paper.Dispose(); $cream.Dispose(); $secondary.Dispose()
        $name.Dispose(); $caption.Dispose(); $align.Dispose()
    }
}
Save-Texture 'cylinder' $paintLabel
Save-Texture 'sphere' $paintLabel
