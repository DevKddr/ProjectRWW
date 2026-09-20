@echo off
setlocal

rem Always run from the folder this .bat file is located in,
rem regardless of where it was double-clicked from.
cd /d "%~dp0"

echo ============================================
echo Item Data Converter
echo ============================================
echo Working directory: %cd%
echo.

where python >nul 2>nul
if errorlevel 1 (
    echo ERROR: python was not found in PATH.
    echo Please install Python and make sure "Add Python to PATH" was checked.
    goto :end
)

if not exist "items.xlsx" (
    echo ERROR: items.xlsx not found in this folder.
    goto :end
)

if not exist "ItemDataConverter.py" (
    echo ERROR: ItemDataConverter.py not found in this folder.
    goto :end
)

if not exist "output" (
    mkdir "output"
)

python ItemDataConverter.py items.xlsx ./output

echo.
echo ============================================
echo Done. Check the "output" folder for results.
echo ============================================

:end
echo.
pause
