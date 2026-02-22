param([string]$AppVer = "0.0.0", [string]$VcpkgRoot = "", [string]$RepoRoot = "")

# Ensure we run from repo root
if ($RepoRoot -and (Test-Path $RepoRoot)) {
    Set-Location -Path $RepoRoot -ErrorAction Stop
}
$buildWin = Join-Path $PWD "build-win"
if (-not (Test-Path $buildWin)) {
    throw "build-win not found at $buildWin. Run from coralapp repo root or pass -RepoRoot."
}

$exe = Get-ChildItem -Path $buildWin -Filter "coral.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) { throw "coral.exe not found" }

$ver = $AppVer.Trim()
$outDir = "coral-windows-x64-v" + $ver
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
Copy-Item $exe.FullName (Join-Path $outDir ("coral-" + $ver + ".exe")) -Force

# VC++ CRT from vswhere
$raw = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
$installDir = if ($raw) { $raw.ToString().Trim() } else { $null }
if ($installDir -and (Test-Path $installDir)) {
  $verFile = Join-Path $installDir "VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"
  if (Test-Path $verFile) {
    $vcVer = (Get-Content $verFile -Raw).Trim()
    $crtPath = Join-Path $installDir ("VC\Tools\MSVC\" + $vcVer + "\redist\x64")
    $crtDir = Get-ChildItem -Path $crtPath -Filter "Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($crtDir) {
      Copy-Item (Join-Path $crtDir.FullName "msvcp140.dll") $outDir -Force
      Copy-Item (Join-Path $crtDir.FullName "vcruntime140.dll") $outDir -Force
      Copy-Item (Join-Path $crtDir.FullName "vcruntime140_1.dll") $outDir -Force
      Write-Host "Bundled VC++ CRT"
    }
  }
}

if (-not (Test-Path (Join-Path $outDir "msvcp140.dll"))) {
  $sys32 = $env:SystemRoot + "\System32"
  if (Test-Path (Join-Path $sys32 "msvcp140.dll")) {
    Copy-Item (Join-Path $sys32 "msvcp140.dll") $outDir -Force
    Copy-Item (Join-Path $sys32 "vcruntime140.dll") $outDir -Force
    Copy-Item (Join-Path $sys32 "vcruntime140_1.dll") $outDir -Force
    Write-Host "Bundled VC++ CRT from System32"
  } else {
    Write-Host "WARNING: VC++ DLLs not found - copy msvcp140.dll, vcruntime140.dll, vcruntime140_1.dll manually"
  }
}

# vcpkg DLLs
if ($VcpkgRoot -and (Test-Path "$VcpkgRoot\installed\x64-windows\bin")) {
  Get-ChildItem "$VcpkgRoot\installed\x64-windows\bin\*.dll" | Copy-Item -Destination $outDir -Force
}

Compress-Archive -Path ($outDir + "\*") -DestinationPath ("coral-windows-x64-v" + $ver + ".zip") -Force
Write-Host ""
Write-Host "=== Build complete ==="
Write-Host "Output: $outDir"
Get-ChildItem $outDir | Format-Table Name, Length -AutoSize
