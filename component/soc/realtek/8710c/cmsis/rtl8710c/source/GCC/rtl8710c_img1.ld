/* Linker script to configure memory regions. */

/* !! the include symbole may failed if the symbol file name is too long!! */
/* this symbol file is from S & NS symbol merged */
INCLUDE "romsym_is.so"

MEMORY
{
  /* Internal RAM for program data & text */
  RAM (rwx)             : ORIGIN = 0x10030000, LENGTH = 0x10038000 - 0x10030000

  /* The vector table, it must start with 256 bytes aligned address */
  /* The Reset_Handler also is the NS region code entry function */
  VECTORS_RAM (rwx)     : ORIGIN = 0x10038000, LENGTH = 0x10038100 - 0x10038000

  /* we collect all RO data and put them at the buttom of the image to shorten the image loading time */
  /* RAM functions entry table */
  RAM_FUN_TABLE (rwx)   : ORIGIN = 0x10038100, LENGTH = 0x10038180 - 0x10038100  
  RAM_RO (rwx)          : ORIGIN = 0x10038180, LENGTH = 0x1003EA00 - 0x10038180

  /* Note: 0x1003FA00 ~ 0x1003FF00 is reserved for ROM code and 0x1003EA00 ~ 0x1003FA00 is reserved as S ROM's stack */

  /* Note: 0x10040000 - 0x1003FF00 currently is used by NS RAM code, so Boot loader shouldn't touch it */

  /* External PSRAM for text, rodata & data */
  PSRAM (rwx)           : ORIGIN = 0x60000000, LENGTH = 0x60400000 - 0x60000000

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
__psram_start__ = ORIGIN(PSRAM);
__psram_end__ = ORIGIN(PSRAM) + LENGTH(PSRAM);

SECTIONS
{
	/* templary BSS for boot loader, it will be recycled
	   and be used by FW code */
	.boot_temp_use.bss :
	{
		. = ALIGN(4);
		__boot_temp_bss_start__ = .;
		KEEP(*(.rom_temp.bss))		
		__boot_temp_bss_end__ = .;
		__boot_sboot_bss_start__ = .;
		KEEP(*(.sboot.bss))		
		__boot_sboot_bss_end__ = .;
		
	} > RAM

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
		/* Boot image start here. for image decryption, 
		  the image start addr. should be aligned to 16 */
		. = ALIGN(32);
		__ram_start_table_start__ = .;
		KEEP(*(SORT(.start.ram.data*)))
		__ram_start_table_end__ = .;

	} > RAM_FUN_TABLE

	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.data : AT (__data_start__)
	{
		. = ALIGN(16);
		__fw_img_start__ = .;
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

	} > RAM_RO

	.ram.code_text : AT (__ram_code_text_start__)
	{
		. = ALIGN(32);
		__ram_code_text_start__ = .;

		__ram_img_signature__ = .;
		KEEP(*(.start.ram.sign*))		

		*(.text*)
		
		__ram_code_text_end__ = .;
	} > RAM_RO

	.ram.code_rodata :
	{
		__ram_code_rodata_start__ = .;

		*(.rodata*)
		*(.rodata.str1*)
		__ram_code_rodata_end__ = .;
	} > RAM_RO

	.system_restore_data :
	{
		/* data in this section will not be initialed by ram_start() */
		/* some of them will be initialed by boot loader */
		. = ALIGN(32);		
		*(.sys_restore.bss*)
		*(.ram.bss.noinit*)
	} > RAM_RO

	.heap (COPY):
	{
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		KEEP(*(.heap*))
		__HeapLimit = .;
	} > RAM

	/* Continue to use the stack memory space initialed by ROM code */
	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (COPY):
	{
		KEEP(*(.stack*))
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
/* TODO:	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack") */
}
