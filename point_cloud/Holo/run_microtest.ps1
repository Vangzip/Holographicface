param(
    [ValidateSet("Release")]
    [string]$Configuration = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$holoDir = Split-Path -Parent $PSCommandPath
$repoRoot = Resolve-Path (Join-Path $holoDir "..\..")
$configPath = Join-Path $holoDir "holo_config.microtest.ini"
$slnPath = Join-Path $holoDir "Holo.sln"
$exePath = Join-Path $holoDir "target\Holo.exe"
$inputDir = Join-Path $holoDir "face_roate"
$outputRoot = Join-Path $holoDir "output\micro"
$multiviewDir = Join-Path $outputRoot "multiview"
$elementalDir = Join-Path $outputRoot "elemental"

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) {
        throw $Message
    }
}

function Remove-TreeSafe([string]$Path, [string]$AllowedRoot) {
    $resolvedAllowed = [System.IO.Path]::GetFullPath($AllowedRoot)
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    Assert-True ($fullPath.StartsWith($resolvedAllowed, [System.StringComparison]::OrdinalIgnoreCase)) "Refusing to remove outside $resolvedAllowed`: $fullPath"
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

function Remove-GeneratedForStem([string]$Stem) {
    $patterns = @(
        "$Stem`_rgb.ply",
        "$Stem`_mesh.ply",
        "$Stem`_mesh_model.obj",
        "$Stem`_mesh_model.mtl",
        "$Stem`_mesh_model.jpg"
    )
    foreach ($pattern in $patterns) {
        $path = Join-Path $inputDir $pattern
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Force
        }
    }
}

function Get-JpegSize([string]$Path) {
    Add-Type -AssemblyName System.Drawing
    $image = [System.Drawing.Image]::FromFile($Path)
    try {
        return @{ Width = $image.Width; Height = $image.Height }
    }
    finally {
        $image.Dispose()
    }
}

Assert-True (Test-Path -LiteralPath $configPath) "Missing config: $configPath"
Assert-True (Test-Path -LiteralPath (Join-Path $inputDir "0.tiff")) "Missing input depth: face_roate\0.tiff"
Assert-True (Test-Path -LiteralPath (Join-Path $inputDir "0.jpg")) "Missing input color: face_roate\0.jpg"

$extraInputs = @(Get-ChildItem -LiteralPath $inputDir -Filter "*.tiff" -File | Where-Object { $_.BaseName -ne "0" })
Assert-True ($extraInputs.Count -eq 0) "Microtest expects only 0.tiff in face_roate; remove: $($extraInputs.Name -join ', ')"

if (-not $SkipBuild) {
    Push-Location $repoRoot
    try {
        & msbuild $slnPath /t:Rebuild /p:Configuration=$Configuration /p:Platform=x64 /m /v:minimal
        if ($LASTEXITCODE -ne 0) {
            throw "MSBuild exited with code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

Assert-True (Test-Path -LiteralPath $exePath) "Missing Holo executable: $exePath"

Remove-TreeSafe $outputRoot (Join-Path $holoDir "output")
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

Remove-GeneratedForStem "0"
Remove-GeneratedForStem "3"
Remove-GeneratedForStem "25"

Push-Location $repoRoot
try {
    & $exePath --config $configPath --stage all
    if ($LASTEXITCODE -ne 0) {
        throw "Holo exited with code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

$multiviewFiles = @(Get-ChildItem -LiteralPath $multiviewDir -Filter "*.jpg" -File)
Assert-True ($multiviewFiles.Count -eq 9) "Expected 9 multiview images, got $($multiviewFiles.Count)"
Assert-True (Test-Path -LiteralPath (Join-Path $multiviewDir "001001.jpg")) "Missing first multiview image 001001.jpg"
Assert-True (Test-Path -LiteralPath (Join-Path $multiviewDir "003003.jpg")) "Missing last multiview image 003003.jpg"
$firstViewSize = Get-JpegSize (Join-Path $multiviewDir "001001.jpg")
Assert-True ($firstViewSize.Width -eq 3 -and $firstViewSize.Height -eq 3) "Expected multiview image size 3x3, got $($firstViewSize.Width)x$($firstViewSize.Height)"

$elementalFiles = @(Get-ChildItem -LiteralPath $elementalDir -Filter "*.jpg" -File)
Assert-True ($elementalFiles.Count -eq 9) "Expected 9 elemental images, got $($elementalFiles.Count)"
$firstElementalSize = Get-JpegSize (Join-Path $elementalDir "11.jpg")
Assert-True ($firstElementalSize.Width -eq 3 -and $firstElementalSize.Height -eq 3) "Expected elemental image size 3x3, got $($firstElementalSize.Width)x$($firstElementalSize.Height)"

Write-Host "[microtest] PASS: 0 input -> 9 multiview images and 9 elemental images"
