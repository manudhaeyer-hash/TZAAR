# ============================================================================
#  Equivalent PowerShell de tools/bundle.py, pour ne pas dependre de Python.
#    .\tools\bundle.ps1                 -> submit\tzaar_bot.cpp
#    .\tools\bundle.ps1 -Out autre.cpp
# ============================================================================
param([string]$Out = "submit\tzaar_bot.cpp")

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

$sources = @(
  "src\core\geometry.cpp", "src\core\zobrist.cpp", "src\core\position.cpp",
  "src\engine\eval.cpp", "src\engine\tt.cpp", "src\engine\search.cpp",
  "src\io\protocol.cpp", "src\debug\trace.cpp", "src\main_cg.cpp"
)

$seen = New-Object System.Collections.Generic.HashSet[string]
$sys  = New-Object System.Collections.Generic.HashSet[string]
$body = New-Object System.Collections.Generic.List[string]

function Inline-File([string]$path) {
  $full = [System.IO.Path]::GetFullPath($path)
  if (-not $seen.Add($full)) { return }
  foreach ($line in [System.IO.File]::ReadAllLines($full)) {
    if ($line -match '^\s*#\s*include\s*"([^"]+)"\s*$') {
      Inline-File (Join-Path (Split-Path -Parent $full) $Matches[1]); continue
    }
    if ($line -match '^\s*#\s*include\s*<([^>]+)>\s*$') { [void]$sys.Add($Matches[1]); continue }
    $body.Add($line.TrimEnd())
  }
  $body.Add("")
}

foreach ($s in $sources) { Inline-File (Join-Path $root $s) }

$header = @(
  "// ==========================================================================",
  "// TZAAR - solver mono-fichier genere par tools\bundle.ps1",
  "// NE PAS EDITER : modifier les sources dans src\ puis relancer le bundler.",
  "// ==========================================================================",
  '#pragma GCC optimize("O3,unroll-loops")',
  '#pragma GCC target("popcnt,bmi,bmi2")',
  ""
)
$header += ($sys | Sort-Object | ForEach-Object { "#include <$_>" })
$header += ""

$outPath = if ([System.IO.Path]::IsPathRooted($Out)) { $Out } else { Join-Path $root $Out }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outPath) | Out-Null
[System.IO.File]::WriteAllLines($outPath, ($header + $body))
Write-Host "$outPath : $($header.Count + $body.Count) lignes"
