/* Linker script to configure memory regions. */
MEMORY
{
  /* !!! These memory map configuration MUST match with the SAU setting !!! */
  VECTOR_ROM (rx) 		: ORIGIN = 0x00000000, LENGTH = 0x00000100

  /* The ROM functions stubs table */
  ROM_STUB (rx) 		: ORIGIN = 0x00000100, LENGTH = 0x00000A00 - 0x00000100

  ITCM_ROM (rx) 		: ORIGIN = 0x00000A00, LENGTH = 0x00019000 - 0x00000A00

  /* The vector table, it must start with 256 bytes aligned address */
  VECTORS_RAM (rwx)     : ORIGIN = 0x10000000, LENGTH = 0x100000A0 - 0x10000000

  /* reserved for ROM code */
  ROM_USED_RAM (rwx)    : ORIGIN = 0x100000A0, LENGTH = 0x10000480 - 0x100000A0

  /* RAM functions entry table */
  RAM_FUN_TABLE (rwx)   : ORIGIN = 0x10000480, LENGTH = 0x100004F0 - 0x10000480

  /* RAM image Signature */
  RAM_IMG_SIGN (rwx)    : ORIGIN = 0x100004F0, LENGTH = 0x10000500 - 0x100004F0

  /* Internal RAM for program data & text */
  DTCM_RAM (wrx) 		: ORIGIN = 0x10000500, LENGTH = 0x10007000 - 0x10000500
  DTCM_RAM_NSC (wrx) 	: ORIGIN = 0x10007000, LENGTH = 0x10008000 - 0x10007000

  /* Internal RAM for boot temp use to load image1, will be recycle when leave ROM code */
  DTCM_TEMP_RAM (wrx) 	: ORIGIN = 0x1002A000, LENGTH = 0x1002F000 - 0x1002A000

  /* this memory space is located in NS domain, just for booting, 
     the stack pointer will be re-allocated at RAM code */
  /* Note: 0x1003FA00 ~ 0x1003FFFF is reserved for NS ROM code */
  STACK_RAM (wrx) 		: ORIGIN = 0x1003EA00, LENGTH = 0x1003FA00 - 0x1003EA00

  /* External PSRAM for text, rodata & data */
  PSRAM (rwx)           : ORIGIN = 0x60000000, LENGTH = 0x60100000 - 0x60000000

  /* XIP for NSC (Non-Secure Callable) */
  XIP_FLASH_NSC (rx)    : ORIGIN = 0x9BFF0000, LENGTH = 0x9C000000 - 0x9BFF0000
}

/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(Reset_Handler)
__eram_end__ = ORIGIN(PSRAM) + LENGTH(PSRAM);
__psram_start__ = ORIGIN(PSRAM);
__psram_end__ = ORIGIN(PSRAM) + LENGTH(PSRAM);

SECTIONS
{
	.rom.vector_tbl :
	{
		/* Vector table must start with a 256bytes aligned address */
		KEEP(*(.rom.vectors))
		__Vectors_End = .;
		__Vectors_Size = __Vectors_End - __Vectors;
	} > VECTOR_ROM

	.rom.text :
	{
		KEEP(*startup_rtl8710c.o(.text*))
		KEEP(*(.sysstart.text))
		KEEP(*startup.o(.rodata*))
		KEEP(*startup.o(.rodata.str1*))
		KEEP(*fw_flash_boot.o(.text*))
		KEEP(*fw_flash_boot.o(.rodata*))
		KEEP(*startup_rtl8710c.o(.rodata.str1*))
		KEEP(*startup.o(.rodata.str1*))
		KEEP(*fw_flash_boot.o(.rodata.str1*))

		KEEP(*system_rtl8710c.o(.text*))
		KEEP(*(.startup.text))
		KEEP(*(.vector.text))
		KEEP(*(.sys_misc.text))
		KEEP(*(.rom.irq.rodata*))

		/**** HAL ROM Code ****/
		KEEP(*(.rom.hal_syson.text))
		KEEP(*(.rom.hal_syson.rodata))
		KEEP(*(.rom.hal_pinmux.rodata*))
		KEEP(*(.rom.hal_uart.text))
		KEEP(*(.rom.hal_uart.rodata))
		KEEP(*(.rom.hal_timer.text))
		KEEP(*(.rom.hal_timer.rodata))
		KEEP(*(.rom.hal_timer_ba.rodata))
		KEEP(*(.rom.hal_gpio.text))
		KEEP(*(.rom.hal_gpio.rodata))
		KEEP(*(.rom.hal_efuse.text))
		KEEP(*(.rom.hal_efuse.rodata))
		KEEP(*(.rom.hal_gdma.text*))
		KEEP(*(.rom.hal_gdma.rodata*))
		KEEP(*(.rom.hal_spic.text*))
		KEEP(*(.rom.hal_spic.rodata*))
		KEEP(*(.rom.hal_flash.text*))
		KEEP(*(.rom.hal_flash.rodata*))
		KEEP(*(.rom.hal_cache.text*))
		KEEP(*(.rom.hal_cache.rodata*))
		KEEP(*(.rom.hal_sce.text*))
		KEEP(*(.rom.hal_sce.rodata*))
		KEEP(*(.rom.hal_vdr.text*))
		KEEP(*(.rom.hal_vdr.rodata*))

		KEEP(*rtl8710c_syson.o(.rodata*))
		KEEP(*rtl8710c_uart.o(.rodata*))
		KEEP(*rtl8710c_gdma.o(.text*))
		KEEP(*rtl8710c_gdma.o(.rodata*))
		KEEP(*rtl8710c_pinmux.o(.text*))
		KEEP(*(.rom.hal_pinmux.rodata*))
		KEEP(*rtl8710c_pinmux.o(.rodata*))
		KEEP(*rtl8710c_crypto.o(.text*))
		KEEP(*rtl8710c_crypto.o(.rodata*))
		/**** Utility ROM code ****/
		KEEP(*(.infra.rom.text))
		KEEP(*(.stdioport.text))
		KEEP(*(.stdioport.rodata))
		KEEP(*(.rtlprintf.text))
		KEEP(*(.rtlprintf.rodata))
		KEEP(*(.rtlprintf.text))
		KEEP(*(.shell.text))
		KEEP(*(.shell.rodata))
		KEEP(*(.shellcmd.text))
		KEEP(*(.shellcmd.rodata))
		KEEP(*(.ccmd.text))
		KEEP(*(.ccmd.rodata))
		KEEP(*(.rom.utility.text*))

		KEEP(*rtl_printf.o(.rodata*))
		KEEP(*strproc.o(.text*))
		KEEP(*strproc.o(.rodata*))
		KEEP(*atoi.o(.text*))
		KEEP(*atoi.o(.rodata*))

		KEEP(*xmodem.o(.text*))
		KEEP(*xmodem.o(.rodata*))
		KEEP(*xmport_uart.o(.text*))
		KEEP(*xmport_uart.o(.rodata*))
		KEEP(*fw_otu.o(.text*))
		KEEP(*fw_otu.o(.rodata*))
		KEEP(*fw_otu.o(.rodata.str1*))
		KEEP(*fw_uart_boot.o(.text*))
		KEEP(*fw_uart_boot.o(.rodata*))
		KEEP(*fw_uart_boot.o(.rodata.str1*))
		KEEP(*fw_sdio_boot.o(.text*))
		KEEP(*fw_sdio_boot.o(.rodata*))
		KEEP(*fw_sdio_boot.o(.rodata.str1*))
		
/*		KEEP(*cmse_nonsecure_call.o(.text)) */

		__lib_gcc_start = .;
		*_arm_cmpdf2.o(.text*)
		*_aeabi_uldivmod.o(.text*)
		*_aeabi_ldivmod.o(.text*)
		*_aeabi_*.o(.text*)
		*bpabi.o(.text*)
		*_muldi3.o(.text*)
		*_divdi3.o(.text*)
		*_fixunsdfsi.o(.text*)
		*_fixdfdi.o(.text*)
		*_fixunsdfdi.o(.text*)
		*_floatdidf.o(.text*)
		*_floatundidf.o(.text*)
		*_udivmoddi4.o(.text*)
		*adddf3.o(.text*)
		*divdf3.o(.text*)
		*eqdf2.o(.text*)
		*gedf2.o(.text*)
		*ledf2.o(.text*)
		*muldf3.o(.text*)
		*subdf3.o(.text*)
		*fixdfsi.o(.text*)
		*floatsidf.o(.text*)
		*floatunsidf.o(.text*)
		*truncdfsf2.o(.text*)
		*_thumb1_case_uhi.o(.text*)
		*_dvmd_tls.o(.text*)
		*_clzsi2.o(.text*)
		*divsf3.o(.text*)
		*mulsf3.o(.text*)
		*addsf3.o(.text*)
		*subsf3.o(.text*)
		*extendsfdf2.o(.text*)
		*_thumb1_case_uqi.o(.text*)
		*floatsisf.o(.text*)
		*_fixunssfsi.o(.text*)
		*fixsfsi.o(.text*)
		*floatunsisf.o(.text*)
		*_arm_cmpsf2.o(.text*)
		*eqsf2.o(.text*)
		*gesf2.o(.text*)
		*lesf2.o(.text*)		
		__lib_gcc_end = .;
		
		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
 		*crtbegin.o(.dtors)
 		*crtbegin?.o(.dtors)
 		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
 		*(SORT(.dtors.*))
 		*(.dtors)

		KEEP(*shell.o(.rodata.str1*))
		KEEP(*rtl_printf.o(.rodata.str1*))
		KEEP(*utility.o(.rodata.str1*))

		KEEP(*rtl8710c_irq.o(.rodata.str1*))
		KEEP(*rtl8710c_uart.o(.rodata.str1*))
		KEEP(*rtl8710c_timer.o(.rodata.str1*))
		KEEP(*rtl8710c_gpio.o(.rodata.str1*))		
		KEEP(*rtl8710c_efuse.o(.rodata.str1*))
		KEEP(*rtl8710c_gdma.o(.rodata.str1*))
		KEEP(*rtl8710c_spic.o(.rodata.str1*))
		KEEP(*rtl8710c_flash.o(.rodata.str1*))
		KEEP(*rtl8710c_pinmux.o(.rodata.str1*))
		KEEP(*rtl8710c_crypto.o(.rodata.str1*))
		KEEP(*rtl8710c_syson.o(.rodata.str1*))
		KEEP(*rtl8710c_misc.o(.rodata.str1*))		
		KEEP(*fw_flash_boot.o(.rodata.str1*))
		KEEP(*rtl8710c_sce.o(.rodata.str1*))

  	    KEEP(*rtl8710c_*.o(.rodata.str1*))

		KEEP(*rtl8710c_uart.o(.rodata*))		
		KEEP(*rtl8710c_icc.o(.rodata*))		
		KEEP(*rtl8710c_efuse.o(.rodata*))		
		KEEP(*rtl8710c_gdma.o(.rodata*))		
		KEEP(*rtl8710c_pinmux.o(.rodata*))		
		KEEP(*rtl8710c_misc.o(.rodata*))		
		KEEP(*rtl8710c_sce.o(.rodata*))		

		KEEP(*(.eh_frame*))
	} > ITCM_ROM

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > ITCM_ROM

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > ITCM_ROM
	__exidx_end = .;

	/* To copy multiple ROM to RAM sections,
	 * uncomment .copy.table section and,
	 * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
	/*	
	.copy.table :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;
		LONG (__etext)
		LONG (__data_start__)
		LONG (__data_end__ - __data_start__)
		LONG (__etext2)
		LONG (__ram_code_text_start__)
		LONG (__ram_code_text_end__ - __ram_code_text_start__)
		__copy_table_end__ = .;
	} > ITCM_ROM
	*/	
	. = ALIGN(4);
	__rom_etext = .;

	/* To clear multiple BSS sections,
	 * uncomment .zero.table section and,
	 * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
	/*
	.zero.table :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;
		LONG (__bss_start__)
		LONG (__bss_end__ - __bss_start__)
		LONG (__bss2_start__)
		LONG (__bss2_end__ - __bss2_start__)
		__zero_table_end__ = .;
	} > ITCM_ROM
	*/

	.rom.stubs :
	{
		. = ALIGN(0x10);
		__rom_stubs_hal_syson = .;
		KEEP(*(.rom.hal_syson.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_pinmux = .;
		KEEP(*(.rom.hal_pinmux.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_timer = .;
		__rom_stubs_hal_timer_s = .;
		KEEP(*(.rom.hal_timer.stubs))

		. = ALIGN(0x10);
		__rom_stubs_int_vector = .;
		KEEP(*(.rom.hal_vector.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_crypto = .;
		KEEP(*(.rom.hal_crypto_s.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_uart = .;
		KEEP(*(.rom.hal_uart.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_gpio_s = .;
		KEEP(*(.rom.hal_gpio.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_efuse = .;
		KEEP(*(.rom.hal_efuse.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_gdma = .;
		__rom_stubs_hal_gdma_s = .;
		KEEP(*(.rom.hal_gdma.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_spic = .;
		KEEP(*(.rom.hal_spic.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_flash = .;
		KEEP(*(.rom.hal_flash.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_cache = .;
		KEEP(*(.rom.hal_cache.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_sce = .;
		KEEP(*(.rom.hal_sce.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_vendor = .;
		KEEP(*(.rom.hal_vdr.stubs))
						
		. = ALIGN(0x10);
		__rom_stubs_hal_misc = .;
		KEEP(*(.rom.hal_misc.stubs))		

		. = ALIGN(0x10);
		__rom_stubs_flash_boot = .;
		KEEP(*(.rom.sboot.stubs))		
        
		. = ALIGN(0x10);
		__rom_stubs_stdprintf_s = .;
		KEEP(*(.rom.printf.stubs))		

		. = ALIGN(0x10);
		__rom_stubs_strproc = .;
		KEEP(*(.rom.strproc.stubs))

		. = ALIGN(0x10);
		__rom_stubs_utility = .;
		KEEP(*(.rom.utility.stubs))	
	} > ROM_STUB


/************* End of ROM **************/

/* MPU protection Area Start */
	.ram.vector :
	{
		. = ALIGN(4);
		__ram_vector_start__ = .;
		KEEP(*(.ram_vector_table))
		__ram_vector_end__ = .;
		. = ALIGN(4);
		/* thses should be protected by MPU */
		KEEP(*(.rom.irq.bss))
	} > VECTORS_RAM
/* MPU protection Area End */

	.data_of_rom : AT (__rom_etext)		/* for ROM image building */
	{
		. = ALIGN(4);
		__rom_data_start__ = .;

		/**** ROM data ****/
		KEEP(*system_rtl8710c.o(.data*))
		KEEP(*startup.o(.data*))
		KEEP(*rtl8710c_irq.o(.data*))
		KEEP(*fw_flash_boot.o(.data*))
		KEEP(*fw_otu.o(.data*))
		KEEP(*fw_uart_boot.o(.data*))
		KEEP(*(.rom.hal_cache.data))
		KEEP(*(.rom.hal_flash.data))
		KEEP(*(.rom.hal_gdma.data))
		KEEP(*(.rom.hal_uart.data))
		KEEP(*(.rom.hal_timer.data))
		KEEP(*(.rom.hal_efuse.data))
		KEEP(*(.rom.hal_spic.data))
		KEEP(*rtl8710c_crypto.o(.data*))

		/* All ROM data end */
		__rom_data_end__ = .;

	} > ROM_USED_RAM

	.bss_of_rom (NOLOAD) :
	{
		. = ALIGN(4);
		
		/**** ROM BSS ****/
		__rom_bss_start__ = .;
		KEEP(*(.sysstart.bss))
		KEEP(*(.sys_misc.bss))
		KEEP(*(.diag.bss))
		KEEP(*(.rom.hal_syson.bss))
		KEEP(*(.rom.hal_uart.bss))
		KEEP(*(.rom.hal_timer.bss))
		KEEP(*(.rom.hal_icc.bss))
		KEEP(*(.rom.hal_efuse.bss))
        KEEP(*(.rom.hal_gdma.bss))
        KEEP(*(.rom.hal_spic.bss))
        KEEP(*(.rom.hal_flash.bss))        
		KEEP(*(.rom.hal_pinmux.bss))
		KEEP(*(.rom.hal_cache.bss))
		KEEP(*(.rom.hal_sce.bss))
		KEEP(*(.infra.rom.bss))
		KEEP(*(.stdioport.bss))
		KEEP(*strproc.o(.bss*))
		KEEP(*(.rom.wlan.bss))
		KEEP(*(.rom.hal_crypto_s.bss))
		__rom_bss_end__ = .;

	} > ROM_USED_RAM

	/* Non-secure callable function */
	/* .sgstubs : for Old ASDK */
	.sgstubs :
	{
		KEEP(*(.ns_entry_func.text*))
	} > DTCM_RAM_NSC

	.gnu.sgstubs :
	{
		__xip_code_text_start__ = .;
		__nsc_start__ = .;
		KEEP(*(.ns_entry_func.text*))
		__nsc_end__ = .;
		__xip_code_text_end__ = .;
	} > DTCM_RAM_NSC

	.psram.code_text :
	{
		. = ALIGN(16);
		__psram_code_text_start__ = .;

		*(.psram.text*)
		
		__psram_code_text_end__ = .;
	} > PSRAM

	.psram.code_rodata :
	{
		. = ALIGN(16);
		__psram_code_rodata_start__ = .;

		*(.psram.rodata*)

		. = ALIGN(4);
		__psram_code_rodata_end__ = .;
	} > PSRAM

	.psram.bss :
	{
		. = ALIGN(16);
		__psram_bss_start__ = .;

		*(.psram.bss*)

		__psram_bss_end__ = .;

		. = ALIGN(32);
		__eram_bss_end__ = .;
	} > PSRAM

	.ram.img.signature :
	{
		__ram_img_signature__ = .;
		KEEP(*(.start.ram.sign*))		
	} > RAM_IMG_SIGN

	.ram.func.table :
	{
		__ram_start_table_start__ = .;
		KEEP(*(SORT(.start.ram.data*)))
		__ram_start_table_end__ = .;

	} > RAM_FUN_TABLE

	.data :
	{
		__fw_img_start__ = .;
  	    __etext = .; 
		__data_start__ = .;
		*(vtable)
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > DTCM_RAM

	.ram.code_text :
	{
		. = ALIGN(4);
		__etext2 = .;

		. = ALIGN(32);
		__ram_entry_text_start__ = .;
		*(.ram_entry_func.text*)
		__ram_entry_text_end__ = .;
		
		__ram_code_text_start__ = .;
		*(.text*)
		. = ALIGN(4);
		__ram_code_text_end__ = .;
	} > DTCM_RAM

	.ram.code_rodata :
	{
		. = ALIGN(4);
		__ram_code_rodata_start__ = .;

		*(.rodata*)
		*(.rodata.str1*)
		. = ALIGN(4);
		__ram_code_rodata_end__ = .;
	} > DTCM_RAM

	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
	} > DTCM_RAM

	.non_secure.bss :
	{
		. = ALIGN(16);
		__ns_bss_start__ = .;
		*(.nonsecure.bss*)

		. = ALIGN(4);
		__ns_bss_end__ = .;

		. = ALIGN(32);
		__bss_end__ = .;
	} > DTCM_RAM

	/* templary BSS for ROM code, it will be recycled
	   and be used by RAM code */
	.rom_temp_use.bss :
	{
		. = ALIGN(4);
		__rom_temp_bss_start__ = .;
		KEEP(*(.rom_temp.bss))		
		__rom_temp_bss_end__ = .;

		. = ALIGN(32);
		
		__rom_sboot_bss_start__ = .;
		KEEP(*(.rom.sboot.bss))		
		__rom_sboot_bss_end__ = .;

		. = ALIGN(32);

		__rom_boot_loader_tmp_buf_start__ = .;
		KEEP(*(.rom_boot_export.bss*))		
		__rom_boot_loader_tmp_buf_end__ = .;

	} > DTCM_TEMP_RAM

	.heap (COPY):
	{
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		KEEP(*(.heap*))
		__HeapLimit = .;
	} > DTCM_RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (COPY):
	{
		KEEP(*(.stack*))
	} > STACK_RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(STACK_RAM) + LENGTH(STACK_RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);
	PROVIDE(__sram_end__ = __StackLimit);

	/* Check if data + heap + stack exceeds RAM limit */
/* TODO:	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack") */
}
