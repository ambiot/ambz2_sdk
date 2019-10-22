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

:: check boot.bin exist, copy default
::if not exist %cfgdir%\Exe\bootloader.bin (
::	copy %libdir%\image\bootloader.bin %cfgdir%\Exe\bootloader.bin
::)

:: check bootloader.out exist, copy default
if exist postbuild_is_error.txt (
	DEL /F postbuild_is_error.txt
)
if exist %cfgdir%\Exe\bootloader.out (
	DEL /F %cfgdir%\Exe\bootloader.out
)
if not exist %libdir%\image\bootloader.out (
	echo bootloader.out is missing
	echo bootloader.out is missing > postbuild_is_error.txt
	goto error_exit
)
copy %libdir%\image\bootloader.out %cfgdir%\Exe\bootloader.out

::update key.json
if exist key.json (
	DEL /F key.json
)
if not exist keycfg.json (
	echo keycfg.json is missing
	echo keycfg.json is missing > postbuild_is_error.txt
	goto error_exit
)
%tooldir%\elf2bin.exe keygen keycfg.json > postbuild_is_log.txt
if not exist key.json (
	echo key.json isn't generated, check postbuild_is_log.txt
	echo key.json isn't generated > postbuild_is_error.txt
	goto error_exit
)

::build new bootloader.bin from bootloader.out in case user wants secure boot
if not exist amebaz2_bootloader.json (
	echo amebaz2_bootloader.json is missing
	echo amebaz2_bootloader.json is missing > postbuild_is_error.txt
	goto error_exit
)
%tooldir%\elf2bin.exe convert amebaz2_bootloader.json BOOTLOADER secure_bit=0 >> postbuild_is_log.txt
if not exist Debug\Exe\bootloader.bin (
	echo bootloader.bin isn't generated, check postbuild_is_log.txt
	echo bootloader.bin isn't generated > postbuild_is_error.txt
	goto error_exit
)

::generate partition table
%tooldir%\elf2bin.exe convert amebaz2_bootloader.json PARTITIONTABLE secure_bit=0 >> postbuild_is_log.txt
if not exist Debug\Exe\partition.bin (
	echo partition.bin isn't generated, check postbuild_is_log.txt
	echo partition.bin isn't generated > postbuild_is_error.txt
	goto error_exit
)

::generate firmware image
if not exist amebaz2_firmware_is.json (
	echo amebaz2_firmware_is.json is missing
	echo amebaz2_firmware_is.json is missing > postbuild_is_error.txt
	goto error_exit
)
%tooldir%\elf2bin.exe convert amebaz2_firmware_is.json FIRMWARE secure_bit=0 >> postbuild_is_log.txt
if not exist Debug\Exe\firmware_is.bin (
	echo firmware_is.bin isn't generated, check postbuild_is_log.txt
	echo firmware_is.bin isn't generated > postbuild_is_error.txt
	goto error_exit
)

::generate flash image, including partition + bootloader + firmware
%tooldir%\elf2bin.exe combine Debug/Exe/flash_is.bin PTAB=Debug\Exe\partition.bin,BOOT=Debug\Exe\bootloader.bin,FW1=Debug\Exe\firmware_is.bin >> postbuild_is_log.txt
if not exist Debug\Exe\flash_is.bin (
	echo flash_is.bin isn't generated, check postbuild_is_log.txt
	echo flash_is.bin isn't generated > postbuild_is_error.txt
	goto error_exit
)

:: For flash download and debug
:: find main symbol
for /f "tokens=2" %%i in ('cmd /c findstr "^main" Debug\List\application_is.map') do set main_addr=%%i
::echo main=%main_addr%

set /a tmp = %main_addr:'=%-1 
call :toHex %tmp% main_addr
::echo main=%main_addr%

:: rename
if exist Debug\Exe\application_is.dbg.out (
	del Debug\Exe\application_is.dbg.out
)
rename Debug\Exe\application_is.out application_is.dbg.out
if not exist Debug\Exe\application_is.dbg.out (
	echo application_is.dbg.out isn't generated, check postbuild_is_log.txt
	echo application_is.dbg.out isn't generated > postbuild_is_error.txt
	goto error_exit
)
%iartooldir%\bin\ilinkarm.exe %tooldir%\ilinkhappy.o --image_input Debug\Exe\flash_is.bin,flash,firmware,32 --cpu Cortex-M33 --no_entry --keep flash --keep __vector_table --define_symbol main=0x%main_addr% -o Debug\Exe\application_is.out

:: disassembly, very long time, turn on if needed
%iartooldir%\bin\ielfdumparm.exe --code Debug\Exe\application_is.dbg.out Debug\Exe\application_is.asm
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