@echo off
:: Qt 6.8.3 설치 경로에 맞게 수정하세요
set QT_PATH=C:\Qt\6.8.2\msvc2022_64

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="%QT_PATH%"

echo.
echo Visual Studio 솔루션이 build\ 폴더에 생성되었습니다.
echo build\JuDoMarble.sln 을 열어서 빌드하세요.
pause
