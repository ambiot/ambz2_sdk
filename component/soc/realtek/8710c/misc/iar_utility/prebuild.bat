cd /D %1

echo off

::; Need to restore the .out file as our postbuild action will rename .out to .dbg.out then generate a different .out for flashing.
if exist %1\Debug\Exe\%2.out (
del %1\Debug\Exe\%2.out
if exist %1\Debug\Exe\%2.dbg.out (
rename %1\Debug\Exe\%2.dbg.out %2.out
)
)

::;*****************************************************************************#
::;                     Generate Git revision tracking                          #
::;*****************************************************************************#
if exist %1\..\..\..\component\soc\realtek\8710c\misc\iar_utility\prebuild_version.bat (
call %1\..\..\..\component\soc\realtek\8710c\misc\iar_utility\prebuild_version.bat %2
)

:: Generate build_info.h
::echo %date:~0,10%-%time:~0,8%
::echo %USERNAME%
for /f "usebackq" %%i in (`hostname`) do set hostname=%%i
::echo %hostname%

echo #define RTL_FW_COMPILE_TIME RTL8710CFW_COMPILE_TIME> ..\inc\build_info.h
echo #define RTL_FW_COMPILE_DATE RTL8710CFW_COMPILE_DATE>> ..\inc\build_info.h
echo #define UTS_VERSION "%date:~0,10%-%time:~0,8%" >> ..\inc\build_info.h
echo #define RTL8710CFW_COMPILE_TIME "%date:~0,10%-%time:~0,8%" >> ..\inc\build_info.h
echo #define RTL8710CFW_COMPILE_DATE "%date:~0,10%" >> ..\inc\build_info.h
echo #define RTL8710CFW_COMPILE_BY "%USERNAME%" >> ..\inc\build_info.h
echo #define RTL8710CFW_COMPILE_HOST "%hostname%" >> ..\inc\build_info.h
echo #define RTL8710CFW_COMPILE_DOMAIN >> ..\inc\build_info.h
echo #define RTL8710CFW_COMPILER "IAR compiler" >> ..\inc\build_info.h


exit

