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
REM if not exist %cfgdir%\Exe\bootloader.out (
	REM if not exist %libdir%\image\bootloader.out (
		REM echo bootloader.out is missing > postbuild_error.txt
		REM exit /b
	REM )
	REM copy %libdir%\image\bootloader.out %cfgdir%\Exe\bootloader.out
REM )

::update key.json
REM %tooldir%\elf2bin.exe keygen keycfg.json > postbuild_tz_log.txt
REM %tooldir%\elf2bin.exe convert amebaz2_bootloader.json PARTITIONTABLE >> postbuild_tz_log.txt
REM %tooldir%\elf2bin.exe convert amebaz2_firmware_tz.json FIRMWARE >> postbuild_tz_log.txt
REM %tooldir%\elf2bin.exe combine Debug/Exe/flash_tz.bin PTAB=Debug/Exe/partition.bin,BOOT=Debug/Exe/bootloader.bin,FW1=Debug/Exe/firmware_tz.bin >> postbuild_tz_log.txt

%iartooldir%\bin\ielfdumparm.exe --code Debug/Exe/application_s.out Debug/Exe/application_s.asm
copy Debug\Exe\application_s_import_lib.o %libdir%\lib\common\IAR\application_s_import_lib.o
::pause
