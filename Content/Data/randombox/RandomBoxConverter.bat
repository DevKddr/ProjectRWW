@echo off
setlocal

rem Always run from the folder this .bat file is located in,
rem regardless of where it was double-clicked from.
cd /d "%~dp0"

echo ============================================
echo Random Box Converter
echo ============================================
echo Working directory: %cd%
echo.

where python >nul 2>nul
if errorlevel 1 (
    echo ERROR: python was not found in PATH.
    echo Please install Python and make sure "Add Python to PATH" was checked.
    goto :end
)

if not exist "RandomBox.xlsx" (
    echo ERROR: RandomBox.xlsx not found in this folder.
    goto :end
)

if not exist "RandomBoxConverter.py" (
    echo ERROR: RandomBoxConverter.py not found in this folder.
    goto :end
)

if not exist "..\output\items.json" (
    echo ERROR: ..\output\items.json not found. Run ItemDataConverter first.
    goto :end
)

if not exist "..\output" (
    mkdir "..\output"
)

python RandomBoxConverter.py RandomBox.xlsx ..\output

echo.
echo ============================================
echo Done. Check the "output" folder (one level up) for results.
echo ============================================

:end
echo.
pause
