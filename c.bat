@echo off
cd /d "%~dp0"

if not exist build mkdir build
if not exist results mkdir results

gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\vector.c -o build\vector.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\matrix.c -o build\matrix.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\dataset.c -o build\dataset.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\pca.c -o build\pca.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\kmeans.c -o build\kmeans.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\point2d.c -o build\point2d.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\brute_force.c -o build\brute_force.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\kdtree.c -o build\kdtree.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\svg.c -o build\svg.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\spatial_svg.c -o build\spatial_svg.o || goto error
gcc -std=c11 -Wall -Wextra -Wpedantic -Imy_libs -c my_libs\dbscan.c -o build\dbscan.o || goto error


ar rcs build\libdimred.a build\vector.o build\matrix.o build\dataset.o build\pca.o build\kmeans.o build\point2d.o build\brute_force.o build\kdtree.o build\svg.o build\spatial_svg.o build\dbscan.o || goto error

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
