$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$scriptPath = Join-Path $repoRoot 'scripts\crop_elemental_border.ps1'
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("crop-elemental-test-" + [guid]::NewGuid())
$sourceDir = Join-Path $tempRoot 'source'
$destinationDir = Join-Path $tempRoot 'cropped'

try {
    New-Item -ItemType Directory -Path $sourceDir -Force | Out-Null
    for ($row = 1; $row -le 6; ++$row) {
        for ($column = 1; $column -le 6; ++$column) {
            $name = '{0:D3}{1:D3}.jpg' -f $row, $column
            Set-Content -LiteralPath (Join-Path $sourceDir $name) -Value "$row,$column" -NoNewline
        }
    }

    & $scriptPath -SourceDir $sourceDir -DestinationDir $destinationDir

    $output = @(Get-ChildItem -LiteralPath $destinationDir -File -Filter '*.jpg')
    if ($output.Count -ne 16) {
        throw "Expected 16 cropped files, got $($output.Count)."
    }
    if ((Get-Content -LiteralPath (Join-Path $destinationDir '001001.jpg') -Raw) -ne '2,2') {
        throw 'The cropped grid was not reindexed from source row 2, column 2.'
    }
    if ((Get-Content -LiteralPath (Join-Path $destinationDir '004004.jpg') -Raw) -ne '5,5') {
        throw 'The cropped grid did not retain the expected final source frame.'
    }
    if (@(Get-ChildItem -LiteralPath $sourceDir -File -Filter '*.jpg').Count -ne 36) {
        throw 'The source grid must remain unchanged.'
    }
    Write-Output 'PASS: crop_elemental_border'
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}
