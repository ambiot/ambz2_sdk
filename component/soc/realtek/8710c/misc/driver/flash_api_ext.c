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

	u32 data_4=0;
	u8 data_8[8];
	/* erase backup sector 4k bytes*/ 
	flash_erase_sector(&flash, FLASH_RESERVED_DATA_BASE);

	/* backup this sector */
	for (idx = 0; idx < 0x1000; idx += 4) {
		read_addr = opt_sector + idx;
		flash_read_word(&flash, read_addr,&data_4);
		memcpy(data, &data_4, 4);
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
		flash_stream_read(&flash, FLASH_RESERVED_DATA_BASE + idx, 8, data_8);
		memcpy(data, data_8, 8);
		flash_stream_write(&flash, (opt_sector + idx), 8, (u8*)data);
	}
}
/**
  * @brief  This function is used to check flash type and read flash status register and unlock it.
  * @note   
  *	    - this function only support three main types that current use.
  *	    - this function will check some flash protect related bits and set them.
  * @retval -1, check flash type or set flash status failed.
  *          1, check flash type and set flash status success.
  */
int flash_register_check_and_unlock(void){
    flash_t flash;
    int ret;
    u8 flash_id = 0;
    int len = 3;
    int SR1 = 0, SR2 = 0, SR3 = 0;

    //get flash id
    ret = flash_read_id(&flash, &flash_id, len);
    if(ret < 0){
        dbg_printf("[%s] get flash id failed\r\n", __func__);
        return -1;
    }
    //dbg_printf("[%s] flash id is 0x%02X\r\n", __func__, flash_id);
    switch(flash_id){
        case 0xEF://Winbond
            //printf("flash type is Winbond\r\n");
            //SR1
            SR1 = flash_get_status(&flash);
            if(SR1 & (BIT2 | BIT3 | BIT4 | BIT5 | BIT6)){
                dbg_printf("SR1 is %02X, reset bit 2~6 as 0 \r\n", SR1);
                flash_set_status(&flash, (SR1 & ~(BIT2 | BIT3 | BIT4 | BIT5 | BIT6)));
            }
            //read SR1 back 
            SR1 = flash_get_status(&flash);
            if(SR1 & (BIT2 | BIT3 | BIT4 | BIT5 | BIT6)){
                dbg_printf("[%s] set SR1 failed\r\n", __func__);
                return -1;
            }
            //SR2
            SR2 = flash_get_status2(&flash);
            if( SR2 & (BIT6)){
                dbg_printf("SR2 is %02X, reset bit 6 as 0 \r\n", SR2);
                flash_set_status2(&flash, (SR2 & ~(BIT6)));
            }
            //read SR2 back 
            SR2 = flash_get_status2(&flash);
            if( SR2 & (BIT6)){
                dbg_printf("[%s] set SR2 failed\r\n", __func__);
                return -1;
            }
            //SR3
            SR3 = flash_get_status3(&flash);
            if( SR3 & (BIT2)){
                dbg_printf("SR3 is %02X,  reset bit 2 as 0 \r\n", SR3);
                flash_set_status3(&flash, SR3 & ~(BIT2));
            }
            //read SR3 back 
            SR2 = flash_get_status3(&flash);
            if( SR3 & (BIT2)){
                dbg_printf("[%s] set SR3 failed\r\n", __func__);
                return -1;
            }
            break;
        case 0xC8://GD
            //printf("flash type is GD\r\n");
            //SR1
            SR1 = flash_get_status(&flash);
            if(SR1 & (BIT2 | BIT3 | BIT4 | BIT5 | BIT6)){
                dbg_printf("SR1 is %02X, reset bit 2~6 as 0 \r\n", SR1);
                flash_set_status(&flash, (SR1 & ~(BIT2 | BIT3 | BIT4 | BIT5 | BIT6)));
            }
            //read SR1 back 
            SR1 = flash_get_status(&flash);
            if(SR1 & (BIT2 | BIT3 | BIT4 | BIT5 | BIT6)){
                dbg_printf("[%s] set SR1 failed\r\n", __func__);
                return -1;
            }
            //SR2
            SR2 = flash_get_status2(&flash);
            if( SR2 & (BIT6)){
                dbg_printf("SR2 is %02X, reset bit 6 as 0 \r\n", SR2);
                flash_set_status2(&flash, (SR2 & ~(BIT6)));
            }
            //read SR2 back 
            SR2 = flash_get_status2(&flash);
            if( SR2 & (BIT6)){
                dbg_printf("[%s] set SR2 failed\r\n", __func__);
                return -1;
            }
            break;
        case 0x1C://ESMT
            //printf("flash type is ESMT\r\n");
            //SR1
            SR1 = flash_get_status(&flash);
            if(SR1 & (BIT2 | BIT3 | BIT4 | BIT5)){
                dbg_printf("SR1 is %02X, reset bit 2~5 as 0 \r\n", SR1);
                flash_set_status(&flash, SR1 & ~(BIT2 | BIT3 | BIT4 | BIT5));
            }
            //read SR1 back
            SR1 = flash_get_status(&flash);
            if(SR1 & (BIT2 | BIT3 | BIT4 | BIT5)){
                dbg_printf("[%s] set SR1 failed\r\n", __func__);
                return -1;
            }
            break;
        default:
            dbg_printf("[%s] flash type is others\r\n", __func__);
            return -1;
        }
    return 1;
}

