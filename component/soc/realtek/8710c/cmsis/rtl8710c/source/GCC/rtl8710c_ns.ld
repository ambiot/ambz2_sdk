/* Linker script to configure memory regions. */
MEMORY
{
  ITCM_ROM (rx) 		: ORIGIN = 0x00019000, LENGTH = 0x0005EC00 - 0x00019000

  /* The ROM functions stubs table */
  ROM_STUB (rx) 		: ORIGIN = 0x0005EC00, LENGTH = 0x0005FF80 - 0x0005EC00
  ROM_NS4S_STUB (rx) 	: ORIGIN = 0x0005FF80, LENGTH = 0x00060000 - 0x0005FF80

  /* Internal RAM for program data & text */
  DTCM_RAM (wrx) 		: ORIGIN = 0x10008000, LENGTH = 0x10028000 - 0x10008000

  DTCM_TEMP_RAM (wrx) 	: ORIGIN = 0x1002F000, LENGTH = 0x10030000 - 0x1002F000

  /* RAM image Signature */
  RAM_IMG_SIGN (rwx)    : ORIGIN = 0x1003FC70, LENGTH = 0x1003FC80 - 0x1003FC70

  /* RAM functions entry table */
  RAM_FUN_TABLE (rwx)   : ORIGIN = 0x1003FC80, LENGTH = 0x1003FD00 - 0x1003FC80

  /* reserved for ROM code */
  ROM_USED_RAM (rwx)    : ORIGIN = 0x1003FD00, LENGTH = 0x1003FF00 - 0x1003FD00

  /* The vector table, it must start with 256 bytes aligned address */
  /* The Reset_Handler also is the NS region code entry function */
  VECTORS_RAM (rwx)     : ORIGIN = 0x1003FF00, LENGTH = 0x10040000 - 0x1003FF00
    
  /* External PSRAM for text, rodata & data */
  PSRAM (rwx)           : ORIGIN = 0x60000000, LENGTH = 0x60400000 - 0x60000000

  /* Flash memory for XIP */
  /* XIP image must start with 64K(0x10000) aligned address */
  XIP_FLASH (rx) 		: ORIGIN = 0x9B000000, LENGTH = 0x9BFF0000 - 0x9B000000
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
_start_addr = 0x00000000;
ENTRY(_start_addr)
__eram_end__ = ORIGIN(PSRAM) + LENGTH(PSRAM);
__psram_start__ = ORIGIN(PSRAM);
__psram_end__ = ORIGIN(PSRAM) + LENGTH(PSRAM);

SECTIONS
{
	.rom.ns_vector_tbl :
	{
		. = ALIGN(0x100);
		KEEP(*(.rom_ns.vectors))
		__Vectors_End = .;
		__Vectors_Size = __Vectors_End - __Vectors_NS;
		__end__ = .;

	} > ITCM_ROM

	.rom.text :
	{
		KEEP(*startup_rtl8710c.o(.text*))
		KEEP(*system_rtl8710c.o(.text*))
		KEEP(*(.startup.text))
		KEEP(*(.vector.text))
		KEEP(*(.sysstart.text))
		KEEP(*(.sys_misc.text))
		KEEP(*(.rom.irq.rodata*))

		/**** HAL ROM Code ****/
		KEEP(*(.rom.hal_syson.text))
		KEEP(*(.rom.hal_syson.rodata))
		KEEP(*(.rom.hal_uart.text))
		KEEP(*(.rom.hal_uart.rodata))
		KEEP(*(.rom.hal_timer.text))
		KEEP(*(.rom.hal_timer.rodata))
		KEEP(*(.rom.hal_timer_ba.rodata))
		KEEP(*(.rom.hal_pwm.text))
		KEEP(*(.rom.hal_pwm.rodata))
		KEEP(*(.rom.hal_gpio.text))
		KEEP(*(.rom.hal_gpio.rodata))
		KEEP(*(.rom.hal_gdma.text*))
		KEEP(*(.rom.hal_gdma.rodata*))
		KEEP(*(.rom.hal_spic.text*))
		KEEP(*(.rom.hal_spic.rodata*))
		KEEP(*(.rom.hal_flash.text*))
		KEEP(*(.rom.hal_flash.rodata*))
		KEEP(*(.rom.hal_ssi.text*))
		KEEP(*(.rom.hal_ssi.rodata*))
		KEEP(*(.rom.hal_cache.text*))
		KEEP(*(.rom.hal_cache.rodata*))
		KEEP(*(.rom.hal_i2c.text*))
		KEEP(*(.rom.hal_i2c.rodata*))
		KEEP(*(.rom.hal_lpi.text*))
		KEEP(*(.rom.hal_lpi.rodata*))
		KEEP(*(.rom.hal_sdiod.text*))
		KEEP(*(.rom.hal_sdiod.rodata*))
		
		KEEP(*rtl8710c_syson.o(.rodata*))
		KEEP(*rtl8710c_uart.o(.rodata*))
		KEEP(*rtl8710c_gdma.o(.text*))
		KEEP(*rtl8710c_gdma.o(.rodata*))
		KEEP(*rtl8710c_i2c.o(.text*))
		KEEP(*rtl8710c_i2c.o(.rodata*))
		KEEP(*rtl8710c_crypto.o(.text*))
		KEEP(*rtl8710c_crypto.o(.rodata*))
		KEEP(*rtl8710c_lpi.o(.rodata*))
		KEEP(*rtl8710c_sdio_dev.o(.rodata*))

		/**** Utility ROM code ****/
		KEEP(*(.infra.rom.text))
		KEEP(*(.stdioport.text))
		KEEP(*(.stdioport.rodata))
		KEEP(*(.xprintf.text))
		KEEP(*(.xprintf.rodata))
		KEEP(*(.rtlprintf.text))
		KEEP(*(.rtlprintf.rodata))
		KEEP(*(.rtlprintf.text))
		KEEP(*(.logbuf.text))
		KEEP(*(.shell.text))
		KEEP(*(.shell.rodata))
		KEEP(*(.shellcmd.text))
		KEEP(*(.shellcmd.rodata))
		KEEP(*(.ccmd.text))
		KEEP(*(.ccmd.rodata))
		KEEP(*(.rom.utility.text*))

		KEEP(*xprintf.o(.rodata*))
		KEEP(*rtl_printf.o(.rodata*))
		KEEP(*strproc.o(.text*))
		KEEP(*strproc.o(.rodata*))
		KEEP(*atoi.o(.text*))
		KEEP(*atoi.o(.rodata*))
		KEEP(*scanf.o(.text*))
		KEEP(*scanf.o(.rodata*))

		/**** WLAN ROM code ****/
		KEEP(*(.rom.wlan.text))
		KEEP(*(.rom.wlan.rodata))

		/**** Apple crypto ROM code ****/
		KEEP(*(.rom.apple.text))
		KEEP(*(.rom.apple.rodata))


		/**** SSL ROM code ****/
		KEEP(*(.rom.ssl.text))
		KEEP(*(.rom.ssl.rodata))

		KEEP(*(.rom.ssl.itcm))
		KEEP(*(.rom.apple.itcm))
		KEEP(*(.rom.ssl.dtcm))
		KEEP(*(.rom.apple.dtcm))

		/**** FAAC ROM code ****/
		KEEP(*(.rom.faac.text))
		KEEP(*(.rom.faac.rodata))

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

		KEEP(*startup_rtl8710c.o(.rodata.str1*))
		KEEP(*startup.o(.rodata.str1*))
		KEEP(*consol_cmds.o(.rodata.str1*))
		KEEP(*shell.o(.rodata.str1*))
		KEEP(*xprintf.o(.rodata.str1*))
		KEEP(*rtl_printf.o(.rodata.str1*))
		KEEP(*utility.o(.rodata.str1*))

		KEEP(*rtl8710c_syson.o(.rodata.str1*))
		KEEP(*rtl8710c_irq.o(.rodata.str1*))
		KEEP(*rtl8710c_timer.o(.rodata.str1*))
		KEEP(*rtl8710c_pwm.o(.rodata.str1*))
		KEEP(*rtl8710c_uart.o(.rodata.str1*))
		KEEP(*rtl8710c_gdma.o(.rodata.str1*))
		KEEP(*rtl8710c_spic.o(.rodata.str1*))
		KEEP(*rtl8710c_flash.o(.rodata.str1*))
		KEEP(*rtl8710c_gpio.o(.rodata.str1*))
		KEEP(*rtl8710c_i2c.o(.rodata.str1*))
		KEEP(*rtl8710c_lpi.o(.rodata.str1*))
		KEEP(*rtl8710c_misc.o(.rodata.str1*))
		KEEP(*rtl8710c_sdio_dev.o(.rodata.str1*))
		KEEP(*rtl8710c_uart.o(.rodata*))		
		KEEP(*rtl8710c_gdma.o(.rodata*))		
		KEEP(*rtl8710c_ssi.o(.rodata*))		
		KEEP(*rtl8710c_misc.o(.rodata*))		

		KEEP(*rom_rtw_message_e.o(.rodata.str1*))
		KEEP(*rom_rtw_message_f.o(.rodata.str1*))
		KEEP(*rom_mbedtls_bignum.o(.rodata.str1*))
		KEEP(*rom_mbedtls_ecp.o(.rodata.str1*))
		KEEP(*rom_mbedtls_ecp_curves.o(.rodata.str1*))
		KEEP(*rom_mbedtls_oid.o(.rodata*))
		KEEP(*rom_mbedtls_md.o(.rodata*))
		KEEP(*rom_mbedtls_md_wrap.o(.rodata.str1*))
		KEEP(*rom_mbedtls_pem.o(.rodata.str1*))
		KEEP(*rom_mbedtls_dhm.o(.rodata.str1*))
		KEEP(*rom_mbedtls_ecjpake.o(.rodata.str1*))
		KEEP(*rom_mbedtls_pk.o(.rodata*))
		KEEP(*rom_mbedtls_pk_wrap.o(.rodata.str1*))
		KEEP(*rom_mbedtls_pkwrite.o(.rodata.str1*))

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

	.note.gnu.build-id :
	{
		*(.note.gnu.build-id)
	} > ITCM_ROM

	.rom.stubs :
	{
		. = ALIGN(0x10);
		__rom_stubs_hal_syson = .;
		KEEP(*(.rom.hal_syson.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_timer = .;
		KEEP(*(.rom.hal_timer.stubs))

		. = ALIGN(0x10);
		__rom_stubs_int_vector = .;
		KEEP(*(.rom.hal_vector.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_crypto = .;
		__rom_stubs_hal_crypto_ns = .;
		KEEP(*(.rom.hal_crypto_ns.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_pwm = .;
		KEEP(*(.rom.hal_pwm.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_uart = .;
		KEEP(*(.rom.hal_uart.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_gpio = .;
		KEEP(*(.rom.hal_gpio.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_gdma = .;
		__rom_stubs_hal_gdma_ns = .;
		KEEP(*(.rom.hal_gdma.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_spic = .;
		KEEP(*(.rom.hal_spic.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_flash = .;
		KEEP(*(.rom.hal_flash.stubs))

        . = ALIGN(0x10);
		__rom_stubs_hal_ssi = .;
		KEEP(*(.rom.hal_ssi.stubs))
		
		. = ALIGN(0x10);
		__rom_stubs_hal_cache = .;
		KEEP(*(.rom.hal_cache.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_i2c = .;
		KEEP(*(.rom.hal_i2c.stubs))
		
		. = ALIGN(0x10);
		__rom_stubs_hal_misc = .;
		KEEP(*(.rom.hal_misc.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_lpi = .;
		KEEP(*(.rom.hal_lpi.stubs))

		. = ALIGN(0x10);
		__rom_stubs_hal_sdiod = .;
		KEEP(*(.rom.hal_sdiod.stubs))
				
		. = ALIGN(0x10);  
		__rom_stubs_shell = .;
		KEEP(*(.rom.shellcmd.stubs))

		. = ALIGN(0x10);
		__rom_stubs_stdprintf_ns = .;
		KEEP(*(.rom.printf.stubs))		

		. = ALIGN(0x10);
		__rom_stubs_strproc = .;
		KEEP(*(.rom.strproc.stubs))

		. = ALIGN(0x10);
		__rom_stubs_utility = .;
		__rom_stubs_utility_ns = .;
		KEEP(*(.rom.utility.stubs))

		. = ALIGN(0x10);
		__rom_stubs_wlan = .;
		KEEP(*(.rom.wlan.stubs))

		. = ALIGN(0x10);
		__rom_stubs_apple = .;
		KEEP(*(.rom.apple.stubs))

		. = ALIGN(0x10);
		__rom_stubs_ssl = .;
		KEEP(*(.rom.ssl.stubs))

		. = ALIGN(0x10);
		__rom_stubs_faac = .;
		KEEP(*(.rom.faac.stubs))

	} > ROM_STUB

	.rom.ns4s_stubs :
	{
	/* __rom_stubs_symb_ns4s always must be the 1st symbol of ROM stubs */
		__rom_stubs_symb_ns4s = .;
		KEEP(*(.rom.ns4s.stubs))
	} > ROM_NS4S_STUB

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
/*	.data_of_rom :						/* for RAM image building */
	{
		. = ALIGN(4);
		__rom_data_start__ = .;

		/**** ROM data ****/
		KEEP(*system_rtl8710c.o(.data*))
		KEEP(*rtl8710c_handler_ns.o(.data*))
		KEEP(*rtl8710c_irq.o(.data*))
		KEEP(*rtl8710c_symbns4s.o(.data*))
		KEEP(*startup_ns.o(.data*))
		KEEP(*(.rom.hal_uart.data))
		KEEP(*(.rom.hal_timer.data))
		KEEP(*(.rom.hal_pwm.data))
		KEEP(*(.rom.hal_gpio.data))
		KEEP(*(.rom.hal_cache.data))
		KEEP(*(.rom.hal_gdma.data))
		KEEP(*(.rom.hal_crypto_ns.data))
		KEEP(*(.rom.hal_ssi.data))
		KEEP(*(.rom.hal_i2c.data))
		KEEP(*(.xprintf.data))
		KEEP(*rtl8710c_i2c.o(.data*))
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
		KEEP(*(.rom.hal_pwm.bss))
		KEEP(*(.rom.hal_gpio.bss))
		KEEP(*(.rom.hal_i2c.bss))
        KEEP(*(.rom.hal_gdma.bss))
        KEEP(*(.rom.hal_spic.bss))
        KEEP(*(.rom.hal_flash.bss))
        KEEP(*(.rom.hal_ssi.bss))
		KEEP(*(.rom.hal_cache.bss))
		KEEP(*(.rom.hal_crypto_ns.bss))
		KEEP(*(.rom.hal_lpi.bss))
		KEEP(*(.rom.hal_sdiod.bss))
		KEEP(*(.xprintf.bss))
		KEEP(*(.logbuf.bss))
		KEEP(*(.stdioport.bss))
		KEEP(*strproc.o(.bss*))
		KEEP(*rtl8710c_i2c.o(.bss*))
		KEEP(*rtl8710c_flash.o(.bss*))

		KEEP(*(.rom.wlan.bss))
		KEEP(*(.rom.ssl.bss))
		KEEP(*(.rom.hal_crypto_ns.bss))
		__rom_bss_end__ = .;

	} > ROM_USED_RAM

	.ram.func.table :
	{
		__fw_img_start__ = .;
		__ram_start_table_start__ = .;
		KEEP(*(SORT(.start.ram.data*)))
		__ram_start_table_end__ = .;
		
	} > RAM_FUN_TABLE

	.xip_reserved :
	{
		/* XIP image section must start with 64K aligned address, reserve 0x140 for image header */
		. += 0x140;
	} > XIP_FLASH

	.xip.code_text :
	{
		. = ALIGN(16);
		__xip_code_text_start__ = .;

		*(.xip.text*)
		
		__xip_code_text_end__ = .;
	} > XIP_FLASH

	.xip.code_rodata :
	{
		. = ALIGN(4);
		__xip_code_rodata_start__ = .;

		*(.xip.rodata*)
		
		. = ALIGN(4);
		__xip_code_rodata_end__ = .;
	} > XIP_FLASH

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

	.data : AT (__etext)
	{
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
	} > DTCM_RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(DTCM_RAM) + LENGTH(DTCM_RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);
	PROVIDE(__sram_end__ = __StackLimit);

	/* Check if data + heap + stack exceeds RAM limit */
/* TODO:	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack") */
}
