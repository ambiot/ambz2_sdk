cd /D %1
:: Generate build_info.h
echo off
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

