@echo off
setlocal

rem 이 배치 파일이 있는 폴더에서 항상 실행되도록 설정
rem (더블클릭한 위치와 상관없이 동작)
cd /d "%~dp0"

echo ============================================
echo 창고 등급 데이터 변환기
echo ============================================
echo 작업 폴더: %cd%
echo.

where python >nul 2>nul
if errorlevel 1 (
    echo 오류: python을 찾을 수 없습니다.
    echo Python을 설치하고 "Add Python to PATH" 옵션을 체크했는지 확인하세요.
    goto :end
)

if not exist "StorageTierData.xlsx" (
    echo 오류: 이 폴더에 StorageTierData.xlsx 파일이 없습니다.
    goto :end
)

if not exist "StorageTierDataConverter.py" (
    echo 오류: 이 폴더에 StorageTierDataConverter.py 파일이 없습니다.
    goto :end
)

if not exist "..\output" (
    mkdir "..\output"
)

python StorageTierDataConverter.py StorageTierData.xlsx ..\output

echo.
echo ============================================
echo 완료. 상위 폴더의 "output" 폴더에서 결과를 확인하세요.
echo ============================================

:end
echo.
pause
