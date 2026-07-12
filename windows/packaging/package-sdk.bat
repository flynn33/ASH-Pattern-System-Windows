@echo off
REM ASH Pattern System - Windows SDK Packaging Script (Batch Wrapper)
REM Usage: package-sdk.bat x64|ARM64 [OutputPath]

setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: package-sdk.bat x64|ARM64 [OutputPath]
    exit /b 1
)

set Platform=%~1
if "%Platform%"=="x64" (
    set OutputPath=D:\GitHub\APS\packages\ash-windows-sdk-x64
) else if "%Platform%"=="ARM64" (
    set OutputPath=D:\GitHub\APS\packages\ash-windows-sdk-arm64
) else (
    echo Invalid platform. Use x64 or ARM64.
    exit /b 1
)

set ScriptDir=%~dp0
set BinDir=%ScriptDir%\..\build\bin\%Platform%-Release

echo ========================================
echo ASH Pattern System - SDK Packaging
echo ========================================
echo Platform:      %Platform%
echo Output Path:   %OutputPath%
echo Library:       %BinDir%\ash_core.lib
echo ========================================
echo.

REM Verify build outputs exist
if not exist "%BinDir%" (
    echo ERROR: Build output not found: %BinDir%
    exit /b 1
)

set LibFile=%BinDir%\ash-core.lib
if not exist "%LibFile%" (
    echo ERROR: Library not found: %LibFile%
    exit /b 1
)

echo Staging SDK layout...

REM Create output directory
if exist "%OutputPath%" (
    echo Removing existing package: %OutputPath%
    rmdir /s /q "%OutputPath%"
)
mkdir "%OutputPath%"

REM Stage headers
xcopy /E /I /Y "%ScriptDir%\..\src\include\ash" "%OutputPath%" >nul
echo   [OK] Headers staged

REM Stage library
copy /Y "%LibFile%" "%OutputPath%\ash-core.lib" >nul
echo   [OK] Library staged

REM Create README
echo "# ASH Pattern System Windows SDK" > "%OutputPath%\README.md"
echo "" >> "%OutputPath%\README.md"
echo "## Platform: %Platform%" >> "%OutputPath%\README.md"
echo "" >> "%OutputPath%\README.md"
echo "### Contents" >> "%OutputPath%\README.md"
echo "- Headers: windows/src/include/ash/" >> "%OutputPath%\README.md"
echo "- Library: ash_core.lib" >> "%OutputPath%\README.md"
echo "- PDBs: Debug symbols (if available)" >> "%OutputPath%\README.md"
echo "" >> "%OutputPath%\README.md"
echo "### Installation" >> "%OutputPath%\README.md"
echo "1. Extract this ZIP to your target directory" >> "%OutputPath%\README.md"
echo "2. Copy ash_core.lib to your project's lib folder" >> "%OutputPath%\README.md"
echo "3. Include headers in your project references" >> "%OutputPath%\README.md"
echo "4. Link against ash_core.lib" >> "%OutputPath%\README.md"
echo "" >> "%OutputPath%\README.md"
echo "### Build Requirements" >> "%OutputPath%\README.md"
echo "- MSVC 18.x (Visual Studio 2022/2026)" >> "%OutputPath%\README.md"
echo "- Windows SDK 10.0.x" >> "%OutputPath%\README.md"
echo "" >> "%OutputPath%\README.md"
echo "### License" >> "%OutputPath%\README.md"
echo "See LICENSE file in repository root." >> "%OutputPath%\README.md"

echo.
echo Creating archive...

REM Create ZIP archive using 7z if available, otherwise create tar.gz
if exist "C:\Program Files\7-Zip\7z.exe" (
    echo Using 7-Zip for compression...
    "C:\Program Files\7-Zip\7z.exe" a "%OutputPath%.zip" "%OutputPath%\*" >nul
) else if exist "C:\Program Files\GnuWin32\bin\tar.exe" (
    echo Using tar for compression...
    "C:\Program Files\GnuWin32\bin\tar.exe" -czf "%OutputPath%.tar.gz" -C "%OutputPath%" . >nul
) else (
    echo WARNING: No archiver found. Manual extraction required.
    echo Contents staged in: %OutputPath%
)

echo   [OK] Archive created

echo.
echo ========================================
echo Packaging Complete!
echo ========================================
echo.
if exist "%OutputPath%.zip" (
    echo Package location: %OutputPath%.zip
) else if exist "%OutputPath%.tar.gz" (
    echo Package location: %OutputPath%.tar.gz
) else (
    echo Contents staged in: %OutputPath% (manual extraction required)
)
echo.
echo To install: Extract archive to target directory
echo.
