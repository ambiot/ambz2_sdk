::echo TARGET_DIR %1
::echo PROJ_DIR %2
::echo CONFIG_NAME %3
::echo IAR_TOOL_DIR %4

::pause
cd /D %2

set tooldir=%2\..\..\..\component\soc\realtek\8710c\misc\iar_utility
set libdir=%2\..\..\..\component\soc\realtek\8710c\misc\bsp
set cfgdir=%3
set iartooldir=%4
set IAR_VER=""
set ILINKER="%~4\bin\ilinkarm.exe"
for /f "tokens=4" %%a in ('%ILINKER% --version') do set IAR_VER=%%a
echo  %IAR_VER%

:: check boot.bin exist, copy default
::if not exist %cfgdir%\Exe\bootloader.bin (
::	copy %libdir%\image\bootloader.bin %cfgdir%\Exe\bootloader.bin
::)

:: check bootloader.out exist, copy default
if exist postbuild_is_error.txt (
	DEL /F postbuild_tz_error.txt
)
if exist %cfgdir%\Exe\bootloader.out (
	DEL /F %cfgdir%\Exe\bootloader.out
)
if not exist %libdir%\image\bootloader.out (
	echo bootloader.out is missing
	echo bootloader.out is missing > postbuild_tz_error.txt
	goto error_exit
)
copy %libdir%\image\bootloader.out %cfgdir%\Exe\bootloader.out

if not exist %cfgdir%\Exe\application_s.out (
	echo application_s.out is missing, please build application_s first.
	echo application_s.out is missing, please build application_s first. > postbuild_tz_error.txt
	goto error_exit
)

::update key.json
if exist key.json (
	DEL /F key.json
)
if not exist keycfg.json (
	echo keycfg.json is missing
	echo keycfg.json is missing > postbuild_tz_error.txt
	goto error_exit
)
%tooldir%\elf2bin.exe keygen keycfg.json > postbuild_tz_log.txt
if not exist key.json (
	echo key.json isn't generated, check postbuild_tz_log.txt
	echo key.json isn't generated > postbuild_tz_error.txt
	goto error_exit
)

::build new bootloader.bin from bootloader.out in case user wants secure boot
if not exist amebaz2_bootloader.json (
	echo amebaz2_bootloader.json is missing
	echo amebaz2_bootloader.json is missing > postbuild_tz_error.txt
	goto error_exit
)
if exist %cfgdir%\Exe\bootloader.bin (
	DEL /F %cfgdir%\Exe\bootloader.bin
)
%tooldir%\elf2bin.exe convert amebaz2_bootloader.json BOOTLOADER secure_bit=0 >> postbuild_tz_log.txt
if not exist Debug\Exe\bootloader.bin (
	echo bootloader.bin isn't generated, check postbuild_tz_log.txt
	echo bootloader.bin isn't generated > postbuild_tz_error.txt
	goto error_exit
)

::generate partition table
if exist %cfgdir%\Exe\partition.bin (
	DEL /F %cfgdir%\Exe\partition.bin
)
%tooldir%\elf2bin.exe convert amebaz2_bootloader.json PARTITIONTABLE secure_bit=0 >> postbuild_tz_log.txt
if not exist Debug\Exe\partition.bin (
	echo partition.bin isn't generated, check postbuild_tz_log.txt
	echo partition.bin isn't generated > postbuild_tz_error.txt
	goto error_exit
)

::generate firmware image
if not exist amebaz2_firmware_tz.json (
	echo amebaz2_firmware_tz.json is missing
	echo amebaz2_firmware_tz.json is missing > postbuild_tz_error.txt
	goto error_exit
)
if exist %cfgdir%\Exe\firmware_tz.bin (
	DEL /F %cfgdir%\Exe\firmware_tz.bin
)
%tooldir%\elf2bin.exe convert amebaz2_firmware_tz.json FIRMWARE secure_bit=0 >> postbuild_tz_log.txt
if not exist Debug\Exe\firmware_tz.bin (
	echo firmware_tz.bin isn't generated, check postbuild_tz_log.txt
	echo firmware_tz.bin isn't generated > postbuild_tz_error.txt
	goto error_exit
)

:: generate firmware ota image
%tooldir%\checksum.exe Debug\Exe\firmware_tz.bin

::generate flash image, including partition + bootloader + firmware
if exist %cfgdir%\Exe\flash_tz.bin (
	DEL /F %cfgdir%\Exe\flash_tz.bin
)
%tooldir%\elf2bin.exe combine Debug/Exe/flash_tz.bin PTAB=Debug/Exe/partition.bin,BOOT=Debug/Exe/bootloader.bin,FW1=Debug/Exe/firmware_tz.bin >> postbuild_tz_log.txt
if not exist Debug\Exe\flash_tz.bin (
	echo flash_tz.bin isn't generated, check postbuild_tz_log.txt
	echo flash_tz.bin isn't generated > postbuild_tz_error.txt
	goto error_exit
)

%iartooldir%\bin\ielfdumparm.exe --code Debug/Exe/application_s.out Debug/Exe/application_s.asm
%iartooldir%\bin\ielfdumparm.exe --code Debug/Exe/application_ns.out Debug/Exe/application_ns.asm
::pause

:: For flash download and debug
:: find main symbol
for /f "tokens=2" %%i in ('cmd /c findstr "^main" Debug\List\application_ns.map') do set main_addr=%%i
::echo main=%main_addr%

set /a tmp = %main_addr:'=%-1 
call :toHex %tmp% main_addr
::echo main=%main_addr%

:: rename
if exist Debug\Exe\application_ns.dbg.out (
	del Debug\Exe\application_ns.dbg.out
)
rename Debug\Exe\application_ns.out application_ns.dbg.out
if not exist Debug\Exe\application_ns.dbg.out (
	echo application_ns.dbg.out isn't generated, check postbuild_tz_log.txt
	echo application_ns.dbg.out isn't generated > postbuild_tz_log.txt
	goto error_exit
)
%iartooldir%\bin\ilinkarm.exe %tooldir%\ilinkhappy.o --image_input Debug\Exe\flash_tz.bin,flash,firmware,32 --cpu Cortex-M33 --no_entry --keep flash --keep __vector_table --define_symbol main=0x%main_addr% -o Debug\Exe\application_ns.out

:: disassembly, very long time, turn on if needed
:: %iartooldir%\bin\ielfdumparm.exe --code Debug\Exe\application_ns.dbg.out Debug\Exe\application_ns.asm

set IAR_VER_83x=V8.3
call set REPLACED_IAR_VER=%%IAR_VER:%IAR_VER_83x%=%%
if not "%IAR_VER%"=="%REPLACED_IAR_VER%" (
	%tooldir%\objcopy.exe -I elf32-little -j "BTTRACE rw" -Obinary Debug\Exe\application_ns.dbg.out Debug\Exe\APP.trace
	echo %IAR_VER_83x%
) else (
	%tooldir%\objcopy.exe -I elf32-little -j "BTTRACE" -Obinary Debug\Exe\application_ns.dbg.out Debug\Exe\APP.trace
	echo %IAR_VER%
)
::pause

exit 0 /b

:error_exit
if exist Debug\Exe\application_is.out (
	DEL /F Debug\Exe\application_is.out
)
pause
exit 2 /b

:toHex dec hex -- convert a decimal number to hexadecimal, i.e. -20 to FFFFFFEC or 26 to 0000001A
@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set /a dec=%~1
set "hex="
set "map=0123456789ABCDEF"
for /L %%N in (1,1,8) do (
    set /a "d=dec&15,dec>>=4"
    for %%D in (!d!) do set "hex=!map:~%%D,1!!hex!"
)

( ENDLOCAL & REM RETURN VALUES
    IF "%~2" NEQ "" (SET %~2=%hex%) ELSE ECHO.%hex%
)
EXIT /b