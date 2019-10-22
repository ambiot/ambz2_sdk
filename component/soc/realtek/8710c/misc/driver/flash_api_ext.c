#include "osdep_service.h"
#include "flash_api.h"

/**
  * @brief  Lock resource before flash processing
  * @param  none
  * @retval  none
  */
void flash_resource_lock( void )
{
	rtw_cpu_lock();
}

/**
  * @brief  Unlock resource after flash processing
  * @param  none
  * @retval  none
  */
void flash_resource_unlock( void )
{
	rtw_cpu_unlock();
}

/**
  * @brief  This function is used to erase some dwords, and keep other dowrds unchanged in one sector.
  * @param  address: Start address in flash to be erased.
  * @param  dword_num: the number of dwords to be erased. 
  * @note   
  *		- this function is just used for change some dwords in one sector.
  *		- this function will erase whole sector and then write back other dwords.
  *		- please dont use this function if not needed !!!!!!!!!!!!!!
  * @retval none
  */

#define FLASH_RESERVED_DATA_BASE 0x00003000

void flash_erase_dword(u32 address, u32 dword_num)
{
	u32 data[2];
	u32 idx = 0;
	u32 opt_sector = address & ~(0xFFF);
	u32 erase_addr = address;
	u32 erase_num = dword_num;
	u32 read_addr;
	flash_t flash;

	/* erase backup sector 4k bytes*/ 
	flash_erase_sector(&flash, FLASH_RESERVED_DATA_BASE);

	/* backup this sector */
	for (idx = 0; idx < 0x1000; idx += 4) {
		read_addr = opt_sector + idx;
		memcpy(data, (const void *)(SPI_FLASH_BASE + read_addr), 4);
		if (erase_num > 0) {
			if (erase_addr == read_addr) {
				data[0] = 0xFFFFFFFF;
				erase_addr += 4;
				erase_num--;
			}
		}
		flash_stream_write(&flash, (FLASH_RESERVED_DATA_BASE + idx), 4, (u8*)data);
	}
	/* erase this sector */
	flash_erase_sector(&flash, opt_sector);

	/* write this sector with target data erased */
	for (idx = 0; idx < 0x1000; idx += 8) {
		memcpy(data, (const void *)(SPI_FLASH_BASE + FLASH_RESERVED_DATA_BASE + idx), 8);
		flash_stream_write(&flash, (opt_sector + idx), 8, (u8*)data);
	}
}

