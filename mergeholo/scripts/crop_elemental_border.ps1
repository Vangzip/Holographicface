param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,

    [Parameter(Mandatory = $true)]
    [string]$DestinationDir
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$sourcePath = (Resolve-Path -LiteralPath $SourceDir).Path
if (Test-Path -LiteralPath $DestinationDir) {
    throw "Destination directory already exists: $DestinationDir"
}

$frames = @{}
$maxRow = 0
$maxColumn = 0
foreach ($file in Get-ChildItem -LiteralPath $sourcePath -File -Filter '*.jpg') {
    if ($file.Name -notmatch '^(?<row>\d{3})(?<column>\d{3})\.jpg$') {
        continue
    }

    $row = [int]$Matches.row
    $column = [int]$Matches.column
    $key = "$row,$column"
    if ($frames.ContainsKey($key)) {
        throw "Duplicate elemental frame index: $key"
    }
    $frames[$key] = $file.FullName
    $maxRow = [Math]::Max($maxRow, $row)
    $maxColumn = [Math]::Max($maxColumn, $column)
}

if ($maxRow -lt 6 -or $maxColumn -lt 6) {
    throw 'Elemental grid must contain at least 6 rows and 6 columns.'
}

for ($row = 1; $row -le $maxRow; ++$row) {
    for ($column = 1; $column -le $maxColumn; ++$column) {
        if (-not $frames.ContainsKey("$row,$column")) {
            throw ("Elemental grid is incomplete: missing frame {0:000}{1:000}.jpg" -f $row, $column)
        }
    }
}

$verticalTrim = [Math]::Floor($maxRow / 6)
$horizontalTrim = [Math]::Floor($maxColumn / 6)
$firstRow = $verticalTrim + 1
$lastRow = $maxRow - $verticalTrim
$firstColumn = $horizontalTrim + 1
$lastColumn = $maxColumn - $horizontalTrim

New-Item -ItemType Directory -Path $DestinationDir | Out-Null
for ($row = $firstRow; $row -le $lastRow; ++$row) {
    for ($column = $firstColumn; $column -le $lastColumn; ++$column) {
        $outputRow = $row - $verticalTrim
        $outputColumn = $column - $horizontalTrim
        $outputName = '{0:000}{1:000}.jpg' -f $outputRow, $outputColumn
        Copy-Item -LiteralPath $frames["$row,$column"] -Destination (Join-Path $DestinationDir $outputName)
    }
}

Write-Output ("Cropped {0}x{1} to {2}x{3}: removed {4} rows from top/bottom and {5} columns from left/right." -f 
    $maxRow, $maxColumn, ($lastRow - $firstRow + 1), ($lastColumn - $firstColumn + 1), $verticalTrim, $horizontalTrim)
