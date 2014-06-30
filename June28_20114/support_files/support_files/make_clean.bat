@ECHO off

FOR /F "tokens=*" %%G IN ('DIR /B /AD /S debug*') DO RMDIR /S /Q "%%G"
FOR /F "tokens=*" %%G IN ('DIR /B /AD /S release*') DO RMDIR /S /Q "%%G"
FOR /F "tokens=*" %%G IN ('DIR /B /AD /S ipch*') DO RMDIR /S /Q "%%G"
FOR /F "tokens=*" %%G IN ('DIR /B /AD /S CMakeFiles*') DO RMDIR /S /Q "%%G"
FOR /F "tokens=*" %%G IN ('DIR /B /AD /S *.dir') DO RMDIR /S /Q "%%G"

RMDIR Win32

del /F /Q /S *.vcproj
del /F /Q /S *.vcxproj
del /F /Q /S *.vcproj.cmake
del /F /Q /S *.user
del /F /Q /S *.ncb
del /F /Q /S *.filters
del /F /Q /S *.sdf
del /F /Q /S *.sln
del /F /Q /S /A:H *.opensdf
del /F /Q /S /A:H *.suo
del /F /Q /S CMakeCache.txt
del /F /Q /S cmake_install.cmake

pause