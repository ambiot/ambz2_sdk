/* Linker script to configure memory regions. */

ENTRY(ram_start)
/* !! the include symbole may failed if the symbol file name is too long!! */
INCLUDE "romsym_ns.so"

MEMORY
{
  /* RAM functions entry table */
  RAM_FUN_TABLE (rwx)   : ORIGIN = 0x10008000, LENGTH = 0x10008080 - 0x10008000

  /* Internal RAM for program data & text */
  DTCM_RAM (wrx) 		: ORIGIN = 0x10008080, LENGTH = 0x1003FA00 - 0x10008080

  /* 0x1003FA00 ~ 0x1003FE70 is reserved for ROM code */

  /* RAM image Signature */
/*  RAM_IMG_SIGN (rwx)    : ORIGIN = 0x1003FE70, LENGTH = 0x1003FE80 - 0x1003FE70 */

  /* The vector table, it must start with 256 bytes aligned address */
  /* The Reset_Handler also is the NS region code entry function */
  VECTORS_RAM (rwx)     : ORIGIN = 0x1003FF00, LENGTH = 0x10040000 - 0x1003FF00

  /* External PSRAM for text, rodata & data */
  PSRAM (rwx)           : ORIGIN = 0x60100000, LENGTH = 0x60400000 - 0x60100000

  /* Flash memory for XIP */
  /* XIP image must start with 64K(0x10000) aligned address */
  /* XIP Chiper section: TEXT/RODATA in this section can be encrypted (decrypt by SCE) */
  XIP_FLASH_C (rx) 		: ORIGIN = 0x9B000000, LENGTH = 0x9B800000 - 0x9B000000
  /* XIP Plantext section: RODATA in this section will not be encrypted */
  XIP_FLASH_P (rx) 		: ORIGIN = 0x9B800000, LENGTH = 0x9BA80000 - 0x9B800000
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
_start_addr = 0x1000;
ENTRY(_start_addr)
__eram_end__ = ORIGIN(PSRAM) + LENGTH(PSRAM);
__psram_start__ = ORIGIN(PSRAM);
__psram_end__ = ORIGIN(PSRAM) + LENGTH(PSRAM);

SECTIONS
{
	.ram.vector :
	{
		. = ALIGN(256);
		__ram_vector_start__ = .;
		KEEP(*(.ram_vector_table))
		__ram_vector_end__ = .;
		. = ALIGN(4);
	} > VECTORS_RAM

	.ram.func.table :
	{
		. = ALIGN(16);
		__fw_img_start__ = .;
		__ram_start_table_start__ = .;
		KEEP(*(SORT(.start.ram.data*)))
		__ram_start_table_end__ = .;

	} > RAM_FUN_TABLE

	.psram.data : AT (__psram_etext)
	{
		. = ALIGN(16);
	    __psram_etext = .; 
		__psram_data_start__ = .;

		*(.psram.data*)

		__psram_data_end__ = .;

	} > PSRAM

	.data :
	{
		. = ALIGN(16);
		__data_start__ = .;
		*(vtable)
		*(.sram.data*)		
		*(.data*)

		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(16);
		/* All data end */
		__data_end__ = .;

	} > DTCM_RAM

	/* Set the XIP memory range for the SAU configuration */
	__XIP_Start = ORIGIN(XIP_FLASH_C);
	__XIP_Limit = __XIP_Start + LENGTH(XIP_FLASH_P);

	.xip_c_reserved :
	{
		/* XIP image section must start with 64K aligned address, reserve 0x140 for image header */
		. += 0x140;
		/* If the XIP is the 1st sub-image, then we should reserve 0xE0 bytes meore for OTA Sign. and 6 Public Key */
		/*. += 0xE0; */
	} > XIP_FLASH_C

	.xip_p_reserved :
	{
		/* XIP image section must start with 64K aligned address, reserve 0x140 for image header */
		. += 0x140;
		/* If the XIP is the 1st sub-image, then we should reserve 0xE0 bytes meore for OTA Sign. and 6 Public Key */
		/*. += 0xE0; */
	} > XIP_FLASH_P

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

	.ram.code_text :
	{
		. = ALIGN(16);
		__etext2 = .;
		__ram_code_text_start__ = .;

		/* SPI flash controller related code should be located in SRAM, never locates them in XIP */
		*hal_spic*.o(.text*)
		*hal_flash*.o(.text*)

		/* LPDDR controller related code should be located in SRAM, never locates them in LPDDR */
		*hal_lpcram*.o(.text*)

		*ram_start*.o(.text*)
		*hal_power_mode*.o(.text*)
		*hal_sys_ctrl*.o(.text*)
		*rtl8710c_pinmux_patch*.o(.text*)
		*hal_syson*.o(.text*)
		*hal_pinmux*.o(.text*)
		*sys_irq*.o(.text*)

		*(.lpddr.text*)
		*(.sram.text*)
		. = ALIGN(4);
		__ram_code_text_end__ = .;
	} > DTCM_RAM

	.ram.code_rodata :
	{
		. = ALIGN(16);
		__ram_code_rodata_start__ = .;

		/* SPI flash controller related code should be located in SRAM, never locates them in XIP */
		*hal_spic*.o(.rodata*)
		*hal_flash*.o(.rodata*)

		/* LPDDR controller related code should be located in SRAM, never locates them in LPDDR */
		*hal_lpcram*.o(.rodata*)

		*ram_start*.o(.rodata*)
		*hal_power_mode*.o(.rodata*)
		*hal_sys_ctrl*.o(.rodata*)
		*rtl8710c_pinmux_patch*.o(.rodata*)
		*hal_syson*.o(.rodata*)
		*hal_pinmux*.o(.rodata*)
		*sys_irq*.o(.rodata*)

		. = ALIGN(4);
		__ram_code_rodata_end__ = .;
	} > DTCM_RAM
			
	.xip.code_c :
	{
		/* code and RO data in this section will be encrypted */
		. = ALIGN(16);
		__xip_code_text_start__ = .;

		*(.xip.text*)

		*(.text*)

		/* put RO data sections need to be encrypted here */
		*(.xip.sec_rodata*)
		
		__xip_code_text_end__ = .;
	} > XIP_FLASH_C

	.xip.code_p :
	{
		. = ALIGN(4);
		/* code and RO data in this section will NOT be encrypted */
		/* put DMA RO data here */
		__xip_code_rodata_start__ = .;

		*(.xip.rodata*)

		*(.rodata*)
		*(.rodata.str1*)
		
		. = ALIGN(4);
		__xip_code_rodata_end__ = .;
	} > XIP_FLASH_P

	.system_restore_data :
	{
		/* data in this section will not be initialed by ram_start() */
		/* some of them will be initialed by boot loader */
		. = ALIGN(32);
		*(.sys_restore.bss*)
		*(.ram.bss.noinit*)
	} > DTCM_RAM
	
	.bss :
	{
		. = ALIGN(16);
		__bss_start__ = .;
		*(.sram.bss*)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
	} > DTCM_RAM

	/* Non-Cacheable memory, the memory range should be configured by a MPU region */
	.non_cacheable :
	{
		. = ALIGN(32);
		__nc_data_start__ = .;
		*(.non_cache.data*)
		__nc_bss_start__ = .;
		*(.non_cache.bss*)
		__nc_bss_end__ = .;

		. = ALIGN(32);
		__bss_end__ = .;
	} > DTCM_RAM

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
