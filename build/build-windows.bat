@echo off
setlocal EnableDelayedExpansion
REM Build coral for Windows - steps from windows-build.yaml
REM Run from coralapp directory: build\build-windows.bat
REM Or from build: .\build-windows.bat

cd /d "%~dp0\.."
set REPO_ROOT=%CD%

REM Paths: default to sibling of coralapp (override with env: VCPKG_ROOT, WHISPER_DIR)
if "%VCPKG_ROOT%"=="" set VCPKG_ROOT=%REPO_ROOT%\..\vcpkg
if "%WHISPER_DIR%"=="" set WHISPER_DIR=%REPO_ROOT%\..\whispercpp
if not exist "%REPO_ROOT%\coral-electron\package.json" (
    echo Error: Run from coralapp directory. coral-electron\package.json not found.
    exit /b 1
)
echo === Coral Windows Build ===
echo Repo root: %REPO_ROOT%
echo vcpkg:     %VCPKG_ROOT%
echo whisper:   %WHISPER_DIR%
echo.

REM ---- Set up vcpkg ----
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Setting up vcpkg...
    if not exist "%VCPKG_ROOT%" git clone https://github.com/microsoft/vcpkg "%VCPKG_ROOT%"
    call "%VCPKG_ROOT%\bootstrap-vcpkg.bat"
)

echo Installing portaudio and libsndfile...
"%VCPKG_ROOT%\vcpkg.exe" install portaudio:x64-windows libsndfile:x64-windows

REM ---- Compute app version ----
for /f "delims=" %%a in ('powershell -NoProfile -Command "((Get-Content coral-electron\package.json | ConvertFrom-Json).version).ToString().Trim()"') do set APPVER=%%a
if "%APPVER%"=="" set APPVER=0.0.0
echo APPVER=%APPVER%

REM ---- Set up whisper.cpp ----
if not exist "%WHISPER_DIR%" (
    echo Cloning whisper.cpp...
    git clone --depth 1 https://github.com/ggerganov/whisper.cpp "%WHISPER_DIR%"
)

REM ---- Build whisper.cpp ----
echo Building whisper.cpp...
cmake -S "%WHISPER_DIR%" -B "%WHISPER_DIR%\build" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DGGML_OPENMP=OFF
cmake --build "%WHISPER_DIR%\build" --config Release --target ggml whisper

REM ---- Detect whisper and ggml paths ----
echo Listing .lib files...
dir /s /b "%WHISPER_DIR%\build\*.lib" 2>nul

set WHISPERLIB=
for /f "delims=" %%f in ('dir /s /b "%WHISPER_DIR%\build\whisper*.lib" 2^>nul') do (
    if "!WHISPERLIB!"=="" set WHISPERLIB=%%f
)
if "%WHISPERLIB%"=="" (
    for /f "delims=" %%f in ('dir /s /b "%WHISPER_DIR%\build\whispercpp.lib" 2^>nul') do set WHISPERLIB=%%f
)
if "%WHISPERLIB%"=="" (
    echo ERROR: whisper.lib not found under %WHISPER_DIR%\build
    exit /b 1
)
echo WHISPERLIB=%WHISPERLIB%

set GGMLLIBS=
for /f "delims=" %%f in ('dir /s /b "%WHISPER_DIR%\build\ggml*.lib" 2^>nul') do (
    if "!GGMLLIBS!"=="" (set GGMLLIBS=%%f) else (set GGMLLIBS=!GGMLLIBS!;%%f)
)
if "%GGMLLIBS%"=="" (
    echo ERROR: ggml*.lib not found
    exit /b 1
)
echo GGMLLIBS=%GGMLLIBS%

REM ---- Build coral ----
if exist build-win rmdir /s /q build-win
echo Building coral...
cmake -S coral -B build-win -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release -DWHISPER_LIBRARY="%WHISPERLIB%" -DGGML_LIBRARIES="%GGMLLIBS%" -DAPP_VERSION="%APPVER%" -DBUILD_DATE="%DATE%" -DGIT_COMMIT="local"
cmake --build build-win --config Release

REM ---- Bundle (use PowerShell for vswhere and file ops) ----
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
"$exe = Get-ChildItem -Path 'build-win' -Filter 'coral.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1; ^
if (-not $exe) { throw 'coral.exe not found' }; ^
$ver = '%APPVER%'.Trim(); ^
$outDir = 'coral-windows-x64-v' + $ver; ^
New-Item -ItemType Directory -Path $outDir -Force | Out-Null; ^
Copy-Item $exe.FullName (Join-Path $outDir ('coral-' + $ver + '.exe')) -Force; ^
$raw = & \"${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe\" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null; ^
$installDir = if ($raw) { $raw.ToString().Trim() } else { $null }; ^
if ($installDir -and (Test-Path $installDir)) { ^
  $verFile = Join-Path $installDir 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'; ^
  if (Test-Path $verFile) { ^
    $vcVer = (Get-Content $verFile -Raw).Trim(); ^
    $crtPath = Join-Path $installDir ('VC\Tools\MSVC\' + $vcVer + '\redist\x64'); ^
    $crtDir = Get-ChildItem -Path $crtPath -Filter 'Microsoft.VC*.CRT' -Directory -ErrorAction SilentlyContinue | Select-Object -First 1; ^
    if ($crtDir) { ^
      Copy-Item (Join-Path $crtDir.FullName 'msvcp140.dll') $outDir -Force; ^
      Copy-Item (Join-Path $crtDir.FullName 'vcruntime140.dll') $outDir -Force; ^
      Copy-Item (Join-Path $crtDir.FullName 'vcruntime140_1.dll') $outDir -Force; ^
      Write-Host 'Bundled VC++ CRT' ^
    } ^
  } ^
}; ^
if (-not (Test-Path (Join-Path $outDir 'msvcp140.dll'))) { ^
  $sys32 = $env:SystemRoot + '\System32'; ^
  if (Test-Path (Join-Path $sys32 'msvcp140.dll')) { ^
    Copy-Item (Join-Path $sys32 'msvcp140.dll') $outDir -Force; ^
    Copy-Item (Join-Path $sys32 'vcruntime140.dll') $outDir -Force; ^
    Copy-Item (Join-Path $sys32 'vcruntime140_1.dll') $outDir -Force; ^
    Write-Host 'Bundled VC++ CRT from System32' ^
  } else { ^
    Write-Host 'WARNING: VC++ DLLs not found - copy msvcp140.dll, vcruntime140.dll, vcruntime140_1.dll manually' ^
  } ^
}; ^
$vcpkgBin = '%VCPKG_ROOT%\installed\x64-windows\bin'; ^
if (Test-Path $vcpkgBin) { Get-ChildItem $vcpkgBin\*.dll | Copy-Item -Destination $outDir -Force }; ^
Compress-Archive -Path ($outDir + '\*') -DestinationPath ('coral-windows-x64-v' + $ver + '.zip') -Force; ^
Write-Host ''; ^
Write-Host '=== Build complete ==='; ^
Write-Host \"Output: $outDir\"; ^
Get-ChildItem $outDir | Format-Table Name, Length -AutoSize"

echo.
echo Done. Run: coral-windows-x64-v%APPVER%\coral-%APPVER%.exe
