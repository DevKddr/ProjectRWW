@echo off
cd /d "%~dp0"

echo ===================================
echo  Converting PlayerBaseStat.xlsx to JSON
echo ===================================
echo.

python player_base_stat_converter.py convert --input PlayerBaseStat.xlsx --outdir ./output

echo.
echo ===================================
echo  Done. Check the log above.
echo ===================================
pause
