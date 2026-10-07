@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%

cl /nologo /EHsc /std:c++20 /W4 /WX /utf-8 ^
  /I. ^
  /I..\External\KamataEngine\include ^
  /I..\External\DirectXTex\include ^
  /I..\External\imgui ^
  Tests\TiledRoomLoaderTests.cpp TiledRoom.cpp ^
  /Fe:..\Generated\Outputs\Debug\TiledRoomLoaderTests.exe
if errorlevel 1 exit /b %errorlevel%

..\Generated\Outputs\Debug\TiledRoomLoaderTests.exe
exit /b %errorlevel%
