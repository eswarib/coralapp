@echo off
setlocal EnableDelayedExpansion
REM Build coral for Windows and produce MSI installer via electron-builder.
REM Run from coralapp directory: build\build-win-msi.bat
REM Prerequisites: build-windows.bat deps + Node.js (npm)

cd /d "%~dp0\.."
set REPO_ROOT=%CD%

if not exist "%REPO_ROOT%\coral-electron\package.json" (
    echo Error: Run from coralapp directory. coral-electron\package.json not found.
    exit /b 1
)

REM ---- Step 1: Build native bundle (zip) ----
echo === Step 1: Building native bundle ===
call "%~dp0build-windows.bat"
if errorlevel 1 exit /b 1

REM ---- Get version ----
for /f "delims=" %%a in ('powershell -NoProfile -Command "((Get-Content coral-electron\package.json | ConvertFrom-Json).version).ToString().Trim()"') do set APPVER=%%a
if "%APPVER%"=="" set APPVER=0.0.0

set BUNDLE_DIR=coral-windows-x64-v%APPVER%
if not exist "%BUNDLE_DIR%" (
    echo Error: Bundle directory %BUNDLE_DIR% not found after build.
    exit /b 1
)

REM ---- Step 2: Stage dist/win-resources for electron-builder ----
echo.
echo === Step 2: Staging dist/win-resources ===
set STAGE_DIR=%REPO_ROOT%\dist\win-resources
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%STAGE_DIR%"
mkdir "%STAGE_DIR%\models"

REM coral.exe (electron-builder expects coral.exe, not coral-X.X.X.exe)
copy /Y "%BUNDLE_DIR%\coral-%APPVER%.exe" "%STAGE_DIR%\coral.exe"

REM DLLs
copy /Y "%BUNDLE_DIR%\*.dll" "%STAGE_DIR%\" 2>nul

REM config.json (Windows defaults)
copy /Y "%REPO_ROOT%\coral\conf\config-windows.json" "%STAGE_DIR%\config.json"

REM Model: ggml-small.en.bin
set MODEL_URL=https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin
set MODEL_PATH=%STAGE_DIR%\models\ggml-small.en.bin

if exist "%REPO_ROOT%\models\ggml-small.en.bin" (
    copy /Y "%REPO_ROOT%\models\ggml-small.en.bin" "%MODEL_PATH%"
    echo Using local model: models\ggml-small.en.bin
) else (
    echo Downloading ggml-small.en.bin...
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "if (-not (Test-Path '%MODEL_PATH%')) { ^
      Invoke-WebRequest -Uri '%MODEL_URL%' -OutFile '%MODEL_PATH%' -UseBasicParsing; ^
      Write-Host 'Downloaded ggml-small.en.bin' ^
    } else { Write-Host 'Model already present' }"
)

if not exist "%MODEL_PATH%" (
    echo Error: Model not found at %MODEL_PATH%. Download failed or path incorrect.
    exit /b 1
)

echo Staged contents:
dir "%STAGE_DIR%" /b
dir "%STAGE_DIR%\models" /b

REM ---- Step 3: Build MSI with electron-builder ----
echo.
echo === Step 3: Building MSI ===
cd coral-electron
call npm run build:win:msi
if errorlevel 1 (
    cd "%REPO_ROOT%"
    exit /b 1
)
cd "%REPO_ROOT%"

echo.
echo === MSI build complete ===
echo Output: build\Release\*.msi
