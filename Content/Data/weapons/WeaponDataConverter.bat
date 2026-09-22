@echo off
setlocal

rem Always run from the folder this .bat file is located in,
rem regardless of where it was double-clicked from.
cd /d "%~dp0"

echo ============================================
echo Weapon Data Converter
echo ============================================
echo Working directory: %cd%
echo.

where python >nul 2>nul
if errorlevel 1 (
    echo ERROR: python was not found in PATH.
    echo Please install Python and make sure "Add Python to PATH" was checked.
    goto :end
)

if not exist "WeaponData.xlsx" (
    echo ERROR: WeaponData.xlsx not found in this folder.
    goto :end
)

if not exist "WeaponDataConverter.py" (
    echo ERROR: WeaponDataConverter.py not found in this folder.
    goto :end
)

if not exist "..\output" (
    mkdir "..\output"
)

python WeaponDataConverter.py WeaponData.xlsx ..\output

echo.
echo ============================================
echo Done. Check the "output" folder (one level up) for results.
echo ============================================

:end
echo.
pause
