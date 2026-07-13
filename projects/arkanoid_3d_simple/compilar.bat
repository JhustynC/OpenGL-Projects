@echo off
setlocal

set "DEPS=D:\BACKUP Archivos\Materias\Ciclo 9\Graficos\OpenGL Dependencies"
set "GPP=C:\msys64\ucrt64\bin\g++.exe"

"%GPP%" -std=c++17 -Wall -Wextra -g "%~dp0main.cpp" "%DEPS%\src\glad.c" ^
  -I "%DEPS%\include" -I "%DEPS%\include\glm" -L "%DEPS%\lib" ^
  -lglfw3dll -lopengl32 -lshell32 -o "%~dp0main.exe"

if errorlevel 1 (
  echo.
  echo La compilacion fallo.
  exit /b 1
)

copy /Y "%DEPS%\lib\glfw3.dll" "%~dp0glfw3.dll" >nul
echo Compilacion completada: main.exe
