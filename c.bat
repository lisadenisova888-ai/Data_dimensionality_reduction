@echo off
cd /d "%~dp0"

if not exist build mkdir build

gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\vector.c -o build\vector.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\matrix.c -o build\matrix.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\dataset.c -o build\dataset.o || goto error

ar rcs build\libdimred.a build\vector.o build\matrix.o build\dataset.o || goto error

gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs main.c -Lbuild -ldimred -lm -o build\main.exe || goto error

echo Build completed: build\main.exe

build\main.exe
set "RESULT=%ERRORLEVEL%"
pause
exit /b %RESULT%

:error
echo Build failed.
pause
exit /b 1
