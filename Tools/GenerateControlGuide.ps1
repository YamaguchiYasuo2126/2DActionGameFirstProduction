# 操作ガイドのSprite画像を再生成する。Windows PowerShell / PowerShellで実行する。
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$outputPath = Join-Path (Split-Path $PSScriptRoot -Parent) 'Resources/controlGuide.png'
$bitmap = [System.Drawing.Bitmap]::new(390, 145, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$background = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(220, 19, 28, 39))
$labelBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 114, 222, 232))
$valueBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 255, 244, 220))
$borderPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(230, 128, 215, 225), 2)
$separatorPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(80, 128, 215, 225), 1)
$labelFont = [System.Drawing.Font]::new('Segoe UI', 14, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
$valueFont = [System.Drawing.Font]::new('Segoe UI', 14, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)

try {
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.FillRectangle($background, 1, 1, 388, 143)
    $graphics.DrawRectangle($borderPen, 1, 1, 387, 142)

    $rows = @(
        @('MOVE', 'A / D   or   LEFT / RIGHT'),
        @('JUMP', 'W   or   UP'),
        @('RUSH', 'HOLD SPACE + WASD'),
        @('PAD', 'HOLD A + LEFT STICK'),
        @('GRAB', 'E / PAD B   (TRACE: DROP)')
    )
    for ($i = 0; $i -lt $rows.Count; ++$i) {
        $y = 8 + $i * 27
        if ($i -gt 0) {
            $lineY = $y - 4
            $graphics.DrawLine($separatorPen, 11, $lineY, 378, $lineY)
        }
        $graphics.DrawString($rows[$i][0], $labelFont, $labelBrush, 13, $y)
        $graphics.DrawString($rows[$i][1], $valueFont, $valueBrush, 108, $y)
    }

    $bitmap.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    $valueFont.Dispose()
    $labelFont.Dispose()
    $separatorPen.Dispose()
    $borderPen.Dispose()
    $valueBrush.Dispose()
    $labelBrush.Dispose()
    $background.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
}
