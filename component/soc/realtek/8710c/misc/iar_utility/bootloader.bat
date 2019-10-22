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

%tooldir%\elf2bin.exe convert amebaz2_bootloader.json BOOTLOADER > postbuild_is_log.txt

%iartooldir%\bin\ielfdumparm.exe --code Debug/Exe/bootloader.out Debug/Exe/bootloader.asm

:: update boot.bin, bootloader.out for application
:: copy Debug\Exe\bootloader.bin %libdir%\image\bootloader.bin
if exist %libdir%\image\bootloader.out (
	DEL /F %libdir%\image\bootloader.out
)
copy Debug\Exe\bootloader.out %libdir%\image\bootloader.out
:: force application rebuilt and generate flash image with new bootloader
if exist Debug\Exe\application_is.out (
	DEL /F Debug\Exe\application_is.out
)
if exist Debug\Exe\application_ns.out (
	DEL /F Debug\Exe\application_ns.out
)
::pause
