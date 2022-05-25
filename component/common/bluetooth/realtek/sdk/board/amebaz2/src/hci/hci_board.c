/**
 * Copyright (c) 2017, Realsil Semiconductor Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include "os_sched.h"
#include "os_pool.h"
#include "os_sync.h"
#include "os_mem.h"

#include "hci_tp.h"
#include "hci_uart.h"
#include "bt_types.h"
#include "trace_app.h"

#include "bt_board.h"
#include "hci_board.h"
#include "hci_process.h"
#include "flash_api.h"
#include "efuse_logical_api.h"
#include "hci_process.h"
#include "hal_efuse_nsc.h"
#include "bt_intf.h"
#include "wifi_conf.h" //for wifi_disable_powersave and wifi_resume_powersave
#include "device_lock.h"
#include <platform_opts_bt.h>

#define BOARD_LOGIC_EFUSE_OFFSET 0x190
#define BT_LOGIC_EFUSE_OFFSET    0x194
#define BT_EFUSE_TABLE_LEN       0x20

#define BT_EFUSE_BLOCK1_OFFSET   0x06
#define BT_EFUSE_BLOCK2_OFFSET   0x0c
#define BT_EFUSE_BLOCK3_OFFSET   0x12


static uint32_t hci_tp_baudrate;
uint8_t  hci_tp_lgc_efuse[BT_EFUSE_TABLE_LEN];
uint8_t  hci_tp_phy_efuse[18];

typedef struct {
    uint32_t IQK_xx;
    uint32_t IQK_yy;
    uint16_t IDAC;
    uint16_t QDAC;
    uint16_t IDAC2;
    uint16_t QDAC2;
}BT_Cali_TypeDef;
BT_Cali_TypeDef g_iqk_data = {0x100, 0x00, 0x20, 0x20, 0x20, 0x20};

extern uint32_t bt_adda_dck_8710c(void);
extern bool hci_read_efuse(void);
extern uint32_t bt_dck_write(uint8_t q_dck, uint8_t i_dck);
extern uint32_t bt_lok_write(uint16_t idac, uint16_t qdac, uint16_t idac2, uint16_t qdac2);
extern uint32_t bt_iqk_8710c(BT_Cali_TypeDef *cal_data, BOOLEAN store);
extern uint32_t bt_flatk_8710c(uint16_t txgain_flatk);

#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
extern hal_status_t hal_wlan_pwr_off(void);
extern void bt_lok_write_bt_only(uint16_t idac, uint16_t qdac, uint16_t idac2, uint16_t qdac2);
extern void bt_flatk_8710c_bt_only(uint16_t txgain_flatk);
#endif

static uint32_t cal_bit_shift(uint32_t Mask)
{
    uint32_t i;
    for(i=0; i<31;i++)
    {
        if(((Mask>>i) & 0x1)==1)
            break;
    }
    return (i);
}

static void set_reg_value(uint32_t reg_address, uint32_t Mask , uint32_t val)
{
    if(reg_address % 4 !=0)
    {
      hci_board_debug("\r\nthe adress must be 4byte align 0x%x\r\n", (unsigned int)reg_address);
      return;
    }
    uint32_t shift = 0;
    uint32_t data = 0;
    data = hci_board_32reg_read(reg_address);
    shift = cal_bit_shift(Mask);
    data = ((data & (~Mask)) | (val << shift));
    hci_board_32reg_set(reg_address, data);
    data = hci_board_32reg_read(reg_address);
}



const BAUDRATE_MAP baudrates[] =
{
    {0x0000701d, 115200},
    {0x0252C00A, 230400},
    {0x05F75004, 921600},
    {0x00005004, 1000000},
    {0x04928002, 1500000},
    {0x00005002, 2000000},
    {0x0000B001, 2500000},
    {0x04928001, 3000000},
    {0x052A6001, 3500000},
    {0x00005001, 4000000},
};
unsigned int baudrates_length = sizeof(baudrates) / sizeof(BAUDRATE_MAP);

int check_sw(int x)
{
	int ret=0;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	ret = (HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR) & x);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return ret;
}

bool hci_rtk_parse_config(uint8_t *config_buf, uint16_t config_len, uint8_t *efuse_buf)
{
#define BT_CONFIG_SIGNATURE             0x8723ab55
#define BT_CONFIG_HEADER_LEN            6

    uint32_t signature;
    uint16_t payload_len;
    uint16_t entry_offset;
    uint16_t entry_len;
    uint8_t *p_entry;
    uint8_t *p;
    uint8_t *p_len;
    uint8_t i;
    uint16_t tx_flatk;


    //enter the  config_len

    p = config_buf;
    p_len = config_buf + 4;

    LE_STREAM_TO_UINT32(signature, p);
    LE_STREAM_TO_UINT16(payload_len, p);

    if (signature != BT_CONFIG_SIGNATURE)
    {
        hci_board_debug("hci_rtk_parse_config: invalid signature 0x%x", (unsigned int)signature);
        return false;
    }

    if (payload_len != config_len - BT_CONFIG_HEADER_LEN)
    {
        HCI_PRINT_ERROR2("hci_rtk_parse_config: invalid len, stated %u, calculated %u",
                         payload_len, config_len - BT_CONFIG_HEADER_LEN);
        LE_UINT16_TO_STREAM(p_len, config_len - BT_CONFIG_HEADER_LEN);  //just avoid the length is not coreect
        /* FIX the len */
       // return false;
    }

    //hci_board_debug("\r\nconfig_len = %x\r\n", config_len - BT_CONFIG_HEADER_LEN);

    p_entry = config_buf + BT_CONFIG_HEADER_LEN;

    while (p_entry < config_buf + config_len)
    {
        p = p_entry;
        LE_STREAM_TO_UINT16(entry_offset, p);
        LE_STREAM_TO_UINT8(entry_len, p);
        p_entry = p + entry_len;

        switch (entry_offset)
        {
        case 0x000c:
            if ((rltk_wlan_is_mp())||(!check_sw((int)EFUSE_SW_UPPERSTACK_SWITCH))) 
            {
                //default use the 115200
                hci_board_debug("\r\ndefault use the 115200\r\n");

                memcpy(p,&(baudrates[0].bt_baudrate),4);
            }

            LE_STREAM_TO_UINT32(hci_tp_baudrate, p);
#if 0            
            if (entry_len >= 12)
            {
                p_hci_rtk->hw_flow_cntrl |= 0x80;   /* bit7 set hw flow control */
                p += 8;

                if (*p & 0x04)  /* offset 0x18, bit2 */
                {
                    p_hci_rtk->hw_flow_cntrl |= 1;  /* bit0 enable hw flow control */
                }
            }

            HCI_PRINT_TRACE2("hci_rtk_parse_config: baudrate 0x%08x, hw flow control 0x%02x",
                             p_hci_rtk->baudrate,p_hci_rtk->hw_flow_cntrl);
#endif

            if(!check_sw((int)EFUSE_SW_DRIVER_DEBUG_LOG))
            {
                  hci_board_debug("hci_rtk_parse_config: baudrate 0x%08x\n",(unsigned int)hci_tp_baudrate);
            }
            break;
        case 0x0018:
           if ((rltk_wlan_is_mp())||(!check_sw((int)EFUSE_SW_UPPERSTACK_SWITCH))) 
           {
               p[0] = p[0] & (~BIT2);
               hci_board_debug("close hci uart flow ctrl: 0x%02x\n", p[0]);
           }
           
           break;
        case 0x0030:
            if (entry_len == 6)
            {
                if ((efuse_buf[0] != 0xff) || (efuse_buf[1] != 0xff) || (efuse_buf[2] != 0xff) || \
                    (efuse_buf[3] != 0xff) || (efuse_buf[4] != 0xff) || (efuse_buf[5] != 0xff))
                {
                    for (int i = 0; i < 6; i++)
                    { 
                        p[i] = efuse_buf[5-i]; 
                    }

                    hci_board_debug("BT ADDRESS: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                        efuse_buf[0], efuse_buf[1], efuse_buf[2], efuse_buf[3], efuse_buf[4], efuse_buf[5]);
                }
                else
                {
                    hci_board_debug("hci_rtk_parse_config: BT ADDRESS %02x %02x %02x %02x %02x %02x, use the default config\r\n",
                              p[0], p[1], p[2], p[3], p[4], p[5]);
                }
            }
            break;

        case 0x194:
            if(efuse_buf[LEFUSE(0x196)]== 0xff)
            {
                if(!(hci_tp_phy_efuse[2]& BIT0))
                {
                    //0
                    tx_flatk=hci_tp_phy_efuse[0xa] | hci_tp_phy_efuse[0xb]<<8;
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
                    bt_flatk_8710c_bt_only(tx_flatk);
#else
                    bt_flatk_8710c(tx_flatk);
#endif
                    hci_board_debug("\r\n WRITE  physical FLATK=tx_flatk=%x \r\n", tx_flatk);
                }

                break;
            }
            else
            {

                p[0]= efuse_buf[LEFUSE(0x196)];
                if(efuse_buf[LEFUSE(0x196)] & BIT1)
                {
                    p[1]= efuse_buf[LEFUSE(0x197)];
                }

                if(efuse_buf[LEFUSE(0x196)] & BIT2)
                {
                    p[2]= efuse_buf[LEFUSE(0x198)];
                    p[3]= efuse_buf[LEFUSE(0x199)];
                    
                    tx_flatk=efuse_buf[LEFUSE(0x198)] | efuse_buf[LEFUSE(0x199)]<<8;
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
                    bt_flatk_8710c_bt_only(tx_flatk);
#else
                    bt_flatk_8710c(tx_flatk);
#endif
                    hci_board_debug("\r\n WRITE logic FLATK=tx_flatk=%x \r\n", tx_flatk);

                }
                else
                {
                    if(!(hci_tp_phy_efuse[2]& BIT0))
                    {
                        //0
                        tx_flatk=hci_tp_phy_efuse[0xa] | hci_tp_phy_efuse[0xb]<<8;
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
                        bt_flatk_8710c_bt_only(tx_flatk);
#else
                        bt_flatk_8710c(tx_flatk);
#endif
                        hci_board_debug("\r\n WRITE  physical FLATK=tx_flatk=%x \r\n", tx_flatk);
                    }

                }

                if(efuse_buf[LEFUSE(0x196)] & BIT5)
                {
                    p[4]= efuse_buf[LEFUSE(0x19a)];
                    p[5]= efuse_buf[LEFUSE(0x19b)];
                }
            }

            if(!check_sw((int)EFUSE_SW_DRIVER_DEBUG_LOG))
            {
                for(i = 0;i < entry_len;i ++)
                {
                    hci_board_debug("\r\n logic efuseMap[%x] = %x\r\n",0x196+i, p[i]);
                }
            }
            break;
        case 0x19f:
            for(i = 0;i <entry_len;i ++)
            {
                if(efuse_buf[LEFUSE(0x19c+i)] != 0xff)
                {
                    p[i]= efuse_buf[LEFUSE(0x19c+i)];
                    if(!check_sw((int)EFUSE_SW_DRIVER_DEBUG_LOG))
                    {
                        hci_board_debug("\r\n logic efuseMap[%x] = %x\r\n",0x19c+i, p[i]);
                    }
                }
            }
            break;
        case 0x1A4:
            for(i = 0;i < entry_len;i ++)
            {
                if(efuse_buf[LEFUSE(0x1a2+i)] != 0xff)
                {
                    p[i]= efuse_buf[LEFUSE(0x1A2+i)];
                    if(!check_sw((int)EFUSE_SW_DRIVER_DEBUG_LOG))
                    {
                        hci_board_debug("\r\n logic efuseMap[%x] = %x\r\n",0x1A2+i, p[i]);
                    }
                }
            }
            break;
        default:
            HCI_PRINT_TRACE2("hci_rtk_parse_config: entry offset 0x%04x, len 0x%02x",
                             entry_offset, entry_len);
            break;
        }
    }
    return true;
}

unsigned char rtlbt_init_config[] =
{
    0x55, 0xab, 0x23, 0x87,
    0x10, 0x00,
    0x30, 0x00, 0x06, 0x99, 0x88, 0x77, 0x44, 0x55, 0x66, /* BT MAC address */
    //0x0c, 0x00, 0x04, 0x1d, 0x70, 0x00, 0x00, /* Baudrate 115200 */
     0x0c, 0x00, 0x04, 0x04, 0x50, 0xF7, 0x05,//*/ /* Baudrate 921600 */
    
    0x18, 0x00, 0x01,0x5c, /* flow control */
    /*efuse about*/
    0x94, 0x01, 0x06, 0x08, 0x00, 0x00, 0x00,0x2e, 0x07,
    
    0x9f, 0x01, 0x05, 0x2a, 0x2a, 0x2a, 0x2a,0x50,
    
    0xA4, 0x01, 0x04, 0xfe, 0xfe, 0xfe, 0xfe,
};


extern unsigned char rtlbt_config[];
extern unsigned int  rtlbt_config_len;

uint8_t *hci_rtk_combine_config(void)
{
#define HCI_CONFIG_HEAD 6
    uint16_t config_length = rtlbt_config_len + sizeof(rtlbt_init_config) - HCI_CONFIG_HEAD;

    uint8_t *full_config_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, config_length);
    uint8_t *p_len = full_config_buf+4;

    memcpy(full_config_buf,rtlbt_init_config, sizeof(rtlbt_init_config));
    memcpy(full_config_buf+sizeof(rtlbt_init_config),rtlbt_config+HCI_CONFIG_HEAD, rtlbt_config_len-HCI_CONFIG_HEAD);

    HCI_PRINT_WARN1("hci_rtk_combine_config: invalid len, calculated %u",
             config_length);
   // hci_board_debug("hci_rtk_combine_config: invalid len, calculated %u\r\n", config_length);

   
    LE_UINT16_TO_STREAM(p_len, config_length);  //just avoid the length is not coreect
    if(!check_sw((int)EFUSE_SW_DRIVER_DEBUG_LOG))
    {
		hci_board_debug("hci_rtk_combine_config: all config length is %u\r\n", config_length);
		for(uint8_t i=0;i< config_length;i++)
		{
			hci_board_debug(":%02x", full_config_buf[i]);
		}
	}
    return full_config_buf;
}

uint16_t fix_config_len(void)
{
    return (rtlbt_config_len + sizeof(rtlbt_init_config) - HCI_CONFIG_HEAD);
}


bool hci_rtk_find_patch(uint8_t bt_hci_chip_id)
{
    extern unsigned char rtlbt_config[];
    extern unsigned int  rtlbt_config_len;

    uint8_t            *fw_buf;
    uint8_t            *config_buf;
    uint16_t            fw_len;
    uint32_t            fw_offset;
    uint16_t            config_len;
    uint32_t            lmp_subversion;;
    uint16_t            mp_num_of_patch=0;
    uint16_t            fw_chip_id = 0;
    //uint32_t val;
    uint8_t *p_merge_addr = NULL;
    flash_t flash;
    uint8_t i = 0;
    const uint8_t rtb_patch_smagic[8]= {0x52, 0x65, 0x61, 0x6C, 0x74, 0x65, 0x63, 0x68};
    uint8_t tmp_patch_head[8];
    const uint8_t single_patch_sing[4]= {0xFD, 0x63, 0x05, 0x62};

#define MERGE_PATCH_SWITCH_SINGLE 0xAAAAAAAA
#define MERGE_PATCH_SWITCH_ADDR   0x1f00
#define MERGE_PATCH_ADDRESS       0x110000
#define HCI_CHIP_VER    (((hci_board_32reg_read(0x400001F0) & (BIT4|BIT5|BIT6|BIT7)) >>4) + 1)

    //check the switch
    if (check_sw((int)EFUSE_SW_USE_FLASH_PATCH))
    {
        //use the default sdk patch
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
        p_merge_addr = (uint8_t *)rltk_bt_get_patch_code_bt_only();
#else
        p_merge_addr = (uint8_t *)rltk_bt_get_patch_code();
#endif
        //hci_board_debug("use default patch = %x\r\n", p_merge_addr);
    }
    else
    {
        //use the flash patch;
        //hci_board_debug("use flash patch\r\n");

        //check flash img
        device_mutex_lock(RT_DEV_LOCK_FLASH);
        flash_stream_read(&flash, MERGE_PATCH_ADDRESS ,8, tmp_patch_head);
        device_mutex_unlock(RT_DEV_LOCK_FLASH);
        if(!memcmp(tmp_patch_head, rtb_patch_smagic, sizeof(rtb_patch_smagic)))
        {
            hci_board_debug("=========use the changed patch===========\r\n");
            device_mutex_lock(RT_DEV_LOCK_FLASH);
            flash_stream_read(&flash, MERGE_PATCH_ADDRESS+8 ,4, (uint8_t *)&lmp_subversion);
            flash_stream_read(&flash, MERGE_PATCH_ADDRESS+12 ,2, (uint8_t *)&mp_num_of_patch);
            device_mutex_unlock(RT_DEV_LOCK_FLASH);
            hci_board_debug("patch mp_num_of_patch = %d\r\n", mp_num_of_patch);
            for(i = 0 ; i < mp_num_of_patch; i++)
            {
                device_mutex_lock(RT_DEV_LOCK_FLASH);
                flash_stream_read(&flash, MERGE_PATCH_ADDRESS+0x0e + 2*i ,2, (uint8_t *)&fw_chip_id);
                device_mutex_unlock(RT_DEV_LOCK_FLASH);
                //LE_ARRAY_TO_UINT16(fw_chip_id, p_merge_addr+0x0e + 2*i);
                hci_board_debug("fw_chip_id patch = 0x%x\r\n", fw_chip_id);
                if(fw_chip_id == bt_hci_chip_id)
                {
                    device_mutex_lock(RT_DEV_LOCK_FLASH);
                    flash_stream_read(&flash,MERGE_PATCH_ADDRESS+0x0e +2*mp_num_of_patch + 2*i ,2, (uint8_t *)&fw_len);
                    //LE_ARRAY_TO_UINT16(fw_len, p_merge_addr+0x0e +2*mp_num_of_patch + 2*i);
                    flash_stream_read(&flash,MERGE_PATCH_ADDRESS+0x0e +4*mp_num_of_patch + 4*i,4, (uint8_t *)&fw_offset);
                    device_mutex_unlock(RT_DEV_LOCK_FLASH);
                    //LE_ARRAY_TO_UINT32(fw_offset, p_merge_addr+0x0e +4*mp_num_of_patch + 4*i);
                    hci_board_debug("lmp_subversion = 0x%x , fw_len = 0x%x, fw_offset = 0x%x\r\n", (unsigned int)lmp_subversion, fw_len, (unsigned int)fw_offset);
                    break;
                }
            }

            if(i >= mp_num_of_patch)
            {
                hci_board_debug("ERROR:no match patch\r\n");
                return false;
            }
            else
            {
                fw_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, fw_len);
                if(fw_buf == NULL)
                {
                    hci_board_debug("fw_buf malloc %d byte fail\r\n", fw_len);
                    return false;
                }
                else
                {
                    device_mutex_lock(RT_DEV_LOCK_FLASH);
                    flash_stream_read(&flash, MERGE_PATCH_ADDRESS+fw_offset ,fw_len, fw_buf);
                    device_mutex_unlock(RT_DEV_LOCK_FLASH);
                    //memcpy(fw_buf,p_merge_addr+fw_offset, fw_len);
                    LE_UINT32_TO_ARRAY(fw_buf+fw_len-4,lmp_subversion);
                    goto parse_config;
                }
            }
        }
    }

    //use merged patch or single patch
    if(!memcmp(p_merge_addr, single_patch_sing, sizeof(single_patch_sing)))
    {
        hci_board_debug("single patch\r\n");

#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
        fw_len = rltk_bt_get_patch_code_len_bt_only();
#else
        fw_len = rltk_bt_get_patch_code_len();
#endif
        fw_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, fw_len);
        if(fw_buf == NULL)
        {
            hci_board_debug("fw_buf malloc %d byte fail\r\n", fw_len);
            return false;
        }
        else
        {
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
            memcpy(fw_buf,(unsigned char*)rltk_bt_get_patch_code_bt_only(), fw_len);
#else
            memcpy(fw_buf,rltk_bt_get_patch_code(), fw_len);
#endif
        }
    }
    else if(!memcmp(p_merge_addr, rtb_patch_smagic, sizeof(rtb_patch_smagic)))
    {
        LE_ARRAY_TO_UINT32(lmp_subversion, (p_merge_addr+8));
        LE_ARRAY_TO_UINT16(mp_num_of_patch, p_merge_addr+0x0c);
        //hci_board_debug("patch mp_num_of_patch = %d\r\n", mp_num_of_patch);

        for(i = 0 ; i < mp_num_of_patch; i++)
        {
            LE_ARRAY_TO_UINT16(fw_chip_id, p_merge_addr+0x0e + 2*i);
            //hci_board_debug("fw_chip_id patch = 0x%x\r\n", fw_chip_id);
            if(fw_chip_id == bt_hci_chip_id)
            {
                LE_ARRAY_TO_UINT16(fw_len, p_merge_addr+0x0e +2*mp_num_of_patch + 2*i);
                LE_ARRAY_TO_UINT32(fw_offset, p_merge_addr+0x0e +4*mp_num_of_patch + 4*i);
                //hci_board_debug("lmp_subversion = 0x%x, fw_len = 0x%x, fw_offset = 0x%x\r\n", lmp_subversion, fw_len, fw_offset);
                break;
            }
        }

        if(i >= mp_num_of_patch)
        {
            hci_board_debug("ERROR:no match patch\r\n");
            return false;
        }
        else
        {
            fw_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, fw_len);
            if(fw_buf == NULL)
            {
                hci_board_debug("fw_buf malloc %d byte fail\r\n", fw_len);
                return false;
            }
            else
            {
                memcpy(fw_buf,p_merge_addr+fw_offset, fw_len);
                LE_UINT32_TO_ARRAY(fw_buf+fw_len-4,lmp_subversion);
            }
        }
    }
    else
    {
        hci_board_debug("patch single is error\r\n");
    }

parse_config:
    config_buf = rtlbt_init_config;
    config_len = sizeof(rtlbt_init_config);
    if (config_len != 0)
    {
        if (hci_rtk_parse_config(config_buf, config_len, hci_tp_lgc_efuse) == false)
        {
            return false;
        }
        else
        {
            config_buf = hci_rtk_combine_config();
            config_len = fix_config_len();
        }
    }

    hci_board_debug("We use fw_buf = 0x%x, fw_len = 0x%x, config_buf = 0x%x, config_len = 0x%x\r\n", (unsigned int)fw_buf, (unsigned int)fw_len, (unsigned int)config_buf, (unsigned int)config_len);
    hci_set_patch(fw_buf, fw_len, config_buf, config_len, hci_tp_baudrate);

    return true;
}

//=============================efuse about=============

void hci_uart_out(void)
{
    hci_board_debug("\r\nHCI UART OUT OK: PA15 RX, PA16 TX\r\n");
    os_delay(100);
  
    //PA15  BT_UART_IN
    set_reg_value(0x400000CC, BIT18|BIT17|BIT16, 6);
    os_delay(5);
    set_reg_value(0x400000CC, BIT24, 1);
    os_delay(5);
    
    //PA16 BT_UART_OUT
    set_reg_value(0x400000D0, BIT2|BIT1|BIT0, 6);
    os_delay(5);
    set_reg_value(0x400000D0, BIT8, 1);
    os_delay(5);
    set_reg_value(0x40000214, BIT29, 1);
    os_delay(5);
}

void bt_dump_iqk(BT_Cali_TypeDef *iqk_data)
{
	//hci_board_debug
      printf("bt_dump_iqk:    DUMP,\r\n");
      printf("the IQK_xx  data is 0x%x,\r\n", (unsigned int)iqk_data->IQK_xx);
      printf("the IQK_yy  data is 0x%x,\r\n", (unsigned int)iqk_data->IQK_yy);
      printf("the QDAC   data is 0x%x,\r\n", iqk_data->QDAC );
      printf("the IDAC   data is 0x%x,\r\n", iqk_data->IDAC );
      printf("the QDAC2  data is 0x%x,\r\n", iqk_data->QDAC2);
      printf("the IDAC2  data is 0x%x,\r\n", iqk_data->IDAC2);
}
bool hci_read_efuse(void)
{
     //phy_efuse
#define BT_PHY_EFUSE_BASE 0x100
    // hci_board_debug("\n phy_efuse data end  is =============\n");
     device_mutex_lock(RT_DEV_LOCK_EFUSE);
     for(int i=0;i <16;i++)
     {
         hal_efuse_read(BT_PHY_EFUSE_BASE+i, hci_tp_phy_efuse+i, LDO_OUT_DEFAULT_VOLT);
     }
     
     hal_efuse_read(0xF8, hci_tp_phy_efuse+16, LDO_OUT_DEFAULT_VOLT);
     hal_efuse_read(0xF9, hci_tp_phy_efuse+17, LDO_OUT_DEFAULT_VOLT);

     //logic_efuse  
     efuse_logical_read(BOARD_LOGIC_EFUSE_OFFSET, 0x20, hci_tp_lgc_efuse);
     device_mutex_unlock(RT_DEV_LOCK_EFUSE);

    //int8_t bd_ddr[]={0x01, 0x02, 0x03,0x04, 0x05,0x06};
    // efuse_logical_write(0x197,3,bd_ddr);
    if(!check_sw((int)EFUSE_SW_DRIVER_DEBUG_LOG))
    {
        //0
        hci_board_debug("\r\n==bt phy_efuse 0x120~0x130:==\r\n ");
        for (int i = 0; i < 18; i++)
        {
            hci_board_debug("%x:", hci_tp_phy_efuse[i]);
        }
           hci_board_debug("\n efuse data is =============\n");
           for(int i =0; i<0x20;i++)
           {
             hci_board_debug("%x:",hci_tp_lgc_efuse[i]);
           }
           hci_board_debug("\n efuse data end  is =============\n");
    }
     
     return true;
}
extern void wlan_init_btonly(void);
bool hci_board_init(void)
{
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
  rltk_wlan_init_btonly();
#else
  if(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
        hci_board_debug("\nWIFI is off !Please restart BT after WIFI on!\n");
        return false;
   }
#endif
  HCI_PRINT_INFO1("hci_tp_open, this cut is AmebaZ2 %X CUT",HCI_CHIP_VER);

  if (rltk_wlan_is_mp()) 
  {
      hci_board_debug("\r\n==========this is BT MP DRIVER===========,\r\n this cut is AMEBAZII %x CUT\r\n", HCI_CHIP_VER);
  }
  hci_board_debug("BT BUILD Date: %s, %s \r\n",__DATE__, __TIME__ );

  hci_read_efuse();
  //uint8_t debug_bit = hci_tp_lgc_efuse[0x11] &(~(BIT1|BIT5|BIT6));
  //uint8_t debug_bit = hci_tp_lgc_efuse[0x11] |(BIT1|BIT5|BIT6);
  //uint8_t debug_bit = hci_tp_lgc_efuse[0x11] &(~(BIT1|BIT5|BIT6 |BIT3));
  //efuse_logical_write(0x1A1,1,&debug_bit);
  
  extern void bt_trace_set_switch(bool flag);
  if(!check_sw((int)EFUSE_SW_DRIVER_DEBUG_LOG))
  {
      hci_board_debug("\r\n We use Debug Val: 0x%x\r\n", HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR));
      //bt_trace_set_switch(true);
  }

  return true;
}

void bt_power_on(void)
{
    set_reg_value(0x40000214, BIT24 |BIT25, 3);
}

void bt_power_off(void)
{
	set_reg_value(0x40000214, BIT24 |BIT25, 0);
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
	hal_wlan_pwr_off();
#else
	rltk_coex_bt_enable(0);
	if (!rltk_wlan_is_mp() ) {
		wifi_resume_powersave();
	}
#endif
}

void hci_normal_start(void)
{
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
	rltk_coex_bt_enable_bt_only();
#else
	if (rltk_wlan_is_mp() ) {
		rtlk_bt_set_gnt_bt(PTA_BT);
	}
	else {
		rltk_coex_bt_enable(1);
	}
#endif
}

void bt_reset(void)
{
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
#else
	if (!rltk_wlan_is_mp() ) {
		wifi_disable_powersave();
	}
#endif
	hci_board_debug("BT RESET LOG...\n");
	set_reg_value(0x40000244, BIT9|BIT8, 3);
	os_delay(5);

	if(!check_sw((int)EFUSE_SW_BT_FW_LOG))
	{
		hci_board_debug("BT FW LOG OPEN\n");
		set_reg_value(0x400000cc, BIT2|BIT1|BIT0, 6);
		os_delay(5);
		set_reg_value(0x400000cc, BIT8, 1);
		os_delay(5);
	}
extern void bt_only_enable_func(void);
	//bt_only_enable_func();
#if 1
	//HCI
	//Enale BT block
	set_reg_value(0x40000214, BIT24 |BIT25, 0);
	os_delay(50);
	set_reg_value(0x40000214, BIT24 |BIT25, 3);
	dbg_printf("BT Reset ok\n");
	os_delay(50);
#endif
}

void bt_only_enable_func(void)
{
    hci_board_debug("BT HCI UART OUT ONLY ...\n");
    set_reg_value(0x40000244, BIT9|BIT8, 3);
    os_delay(5);
    
    //BT LOG ENABLE PA14
    set_reg_value(0x400000cc, BIT2|BIT1|BIT0, 6);
    os_delay(5);
    set_reg_value(0x400000cc, BIT8, 1);
    os_delay(5);
    //PA15  BT_UART_IN
    set_reg_value(0x400000CC, BIT18|BIT17|BIT16, 6);
    os_delay(5);
    set_reg_value(0x400000CC, BIT24, 1);
    os_delay(5);
    
    //PA16 BT_UART_OUT
    set_reg_value(0x400000D0, BIT2|BIT1|BIT0, 6);
    os_delay(5);
    set_reg_value(0x400000D0, BIT8, 1);
    os_delay(5);
    set_reg_value(0x40000214, BIT29, 1);
    os_delay(5);
    //HCI
    //Enale BT block
    set_reg_value(0x40000214, BIT24 |BIT25, 0);
    os_delay(200);
    set_reg_value(0x40000214, BIT24 |BIT25, 3);
    dbg_printf("BT Reset ok\n");
    os_delay(200);
}
void bt_enable_func_uart_only(void)
{
    hci_board_debug("BT HCI UART OUT ONLY ...\n");
    
    //PA15  BT_UART_IN
    set_reg_value(0x400000CC, BIT18|BIT17|BIT16, 6);
    os_delay(5);
    set_reg_value(0x400000CC, BIT24, 1);
    os_delay(5);
    
    //PA16 BT_UART_OUT
    set_reg_value(0x400000D0, BIT2|BIT1|BIT0, 6);
    os_delay(5);
    set_reg_value(0x400000D0, BIT8, 1);
    os_delay(5);
    set_reg_value(0x40000214, BIT29, 1);
    os_delay(5);
    //HCI

}

/////////IQK 
bool bt_iqk_efuse_valid(BT_Cali_TypeDef  *bt_iqk_data)
{
    if((hci_tp_phy_efuse[3] == 0xff) &&
            (hci_tp_phy_efuse[4] == 0xff)&&
            (hci_tp_phy_efuse[5] == 0xff)&&
            (hci_tp_phy_efuse[6] == 0xff))
    {
        hci_board_debug("bt_iqk_efuse_valid: no data\r\n");
        return false;
    }
    else
    {
        bt_iqk_data->IQK_xx = hci_tp_phy_efuse[3] | hci_tp_phy_efuse[4] <<8;
        bt_iqk_data->IQK_yy = hci_tp_phy_efuse[5] | hci_tp_phy_efuse[6] <<8;
        bt_iqk_data->QDAC = hci_tp_phy_efuse[0x0c];
        bt_iqk_data->IDAC = hci_tp_phy_efuse[0x0d];
        bt_iqk_data->QDAC2 = hci_tp_phy_efuse[0x0e];
        bt_iqk_data->IDAC2 = hci_tp_phy_efuse[0x0f];
        hci_board_debug("bt_iqk_efuse_valid: has data\r\n");
        return true;
    }
}

/*
bool bt_iqk_flash_valid(BT_Cali_TypeDef  *bt_iqk_data)
{
#define FLASH_BT_PARA_ADDR				(SYS_DATA_FLASH_BASE + 0xFF0)
  
   if((HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR) == 0xFFFFFFFF) && 
	(HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 4) == 0xFFFFFFFF) &&
	(HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 8) == 0xFFFFFFFF) && 
	(HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 12) == 0xFFFFFFFF))
	{
		 hci_board_debug("bt_iqk_flash_valid: no data\r\n");
		 return false;
	}
        else
	{
          bt_iqk_data->IQK_xx = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR);
          bt_iqk_data->IQK_yy = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+4);
          bt_iqk_data->QDAC = HAL_READ16(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+6);
          bt_iqk_data->IDAC = HAL_READ16(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+8);
          bt_iqk_data->QDAC2 = HAL_READ16(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+10);
          bt_iqk_data->IDAC2 = HAL_READ16(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+12);
          hci_board_debug("bt_iqk_flash_valid: has data\r\n");
          return true;
	}
}
*/

bool bt_iqk_logic_efuse_valid(BT_Cali_TypeDef  *bt_iqk_data)
{
    if((hci_tp_lgc_efuse[0x16] == 0xff) &&
            (hci_tp_lgc_efuse[0x17] == 0xff)&&
            (hci_tp_lgc_efuse[0x18] == 0xff)&&
            (hci_tp_lgc_efuse[0x19] == 0xff))
    {
        hci_board_debug("bt_iqk_efuse_valid: no data\r\n");
        return false;
    }
    else
    {
        bt_iqk_data->IQK_xx = (uint32_t)(((uint32_t)hci_tp_lgc_efuse[0x17]) <<8) | hci_tp_lgc_efuse[0x16];
        bt_iqk_data->IQK_yy = (uint32_t) (((uint32_t)hci_tp_lgc_efuse[0x19]) <<8) | hci_tp_lgc_efuse[0x18]  ;
        bt_iqk_data->QDAC = hci_tp_lgc_efuse[0x1a];
        bt_iqk_data->IDAC = hci_tp_lgc_efuse[0x1b];
        bt_iqk_data->QDAC2 = hci_tp_lgc_efuse[0x1c];
        bt_iqk_data->IDAC2 = hci_tp_lgc_efuse[0x1d];
        hci_board_debug("bt_iqk_logic_efuse_valid: has data\r\n");
        return true;
    }
}

bool bt_check_iqk(void)
{
	BT_Cali_TypeDef     bt_iqk_data;
        
	if(!hci_tp_lgc_efuse[LEFUSE(0x1a1)] & BIT0)
	{
		hci_board_debug("\r\n%s: USE FIX LOGIC EFUSE\r\n",__FUNCTION__);
		if (bt_iqk_logic_efuse_valid(&bt_iqk_data))
		{
			bt_dump_iqk(&bt_iqk_data);
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
            bt_lok_write_bt_only(bt_iqk_data.IDAC , bt_iqk_data.QDAC, bt_iqk_data.IDAC2  , bt_iqk_data.QDAC2);
#else
            bt_lok_write(bt_iqk_data.IDAC , bt_iqk_data.QDAC, bt_iqk_data.IDAC2  , bt_iqk_data.QDAC2);
#endif
            hci_tp_phy_efuse[0] = 0;
            hci_tp_phy_efuse[1] =hci_tp_phy_efuse[1] & (~BIT0);
            //hci_tp_phy_efuse[1] = 0xfe;
            //hci_tp_phy_efuse[2] = 0xff;
			hci_tp_phy_efuse[3] = bt_iqk_data.IQK_xx & 0xff;
			hci_tp_phy_efuse[4] = (bt_iqk_data.IQK_xx >> 8) & 0xff;
			hci_tp_phy_efuse[5] = bt_iqk_data.IQK_yy & 0xff;
			hci_tp_phy_efuse[6] = (bt_iqk_data.IQK_yy >> 8) & 0xff;
			return true;
		}
		hci_board_debug("\r\n%s: LOGIC EFUSE HAS NO DATA\r\n",__FUNCTION__);
		return false;
	}
	
	if (bt_iqk_efuse_valid(&bt_iqk_data))
    {
        if(hci_tp_phy_efuse[0]!=0)
        {
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
            bt_dck_write_btonly(hci_tp_phy_efuse[0x0e], hci_tp_phy_efuse[0x0f]);
#else
            bt_dck_write(hci_tp_phy_efuse[0x0e], hci_tp_phy_efuse[0x0f]);
#endif
        }
        else
        {
            hci_board_debug("\r\nhci_tp_phy_efuse[0]=0,\r\n");
        }
        bt_dump_iqk(&bt_iqk_data);
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
	    bt_lok_write_bt_only(bt_iqk_data.IDAC , bt_iqk_data.QDAC, bt_iqk_data.IDAC2  , bt_iqk_data.QDAC2);
#else
            bt_lok_write(bt_iqk_data.IDAC , bt_iqk_data.QDAC, bt_iqk_data.IDAC2  , bt_iqk_data.QDAC2);
#endif
        return true;
    }
	else if (bt_iqk_logic_efuse_valid(&bt_iqk_data))
	{
		bt_dump_iqk(&bt_iqk_data);
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
	    bt_lok_write_bt_only(bt_iqk_data.IDAC , bt_iqk_data.QDAC, bt_iqk_data.IDAC2  , bt_iqk_data.QDAC2);
#else
            bt_lok_write(bt_iqk_data.IDAC , bt_iqk_data.QDAC, bt_iqk_data.IDAC2  , bt_iqk_data.QDAC2);
#endif
		hci_tp_phy_efuse[0] = 0;
		hci_tp_phy_efuse[1] =hci_tp_phy_efuse[1] & (~BIT0);
		//hci_tp_phy_efuse[1] = 0xfe;
		//hci_tp_phy_efuse[2] = 0xff;
		hci_tp_phy_efuse[3] = bt_iqk_data.IQK_xx & 0xff;
		hci_tp_phy_efuse[4] = (bt_iqk_data.IQK_xx >> 8) & 0xff;
		hci_tp_phy_efuse[5] = bt_iqk_data.IQK_yy & 0xff;
		hci_tp_phy_efuse[6] = (bt_iqk_data.IQK_yy >> 8) & 0xff;
		hci_tp_phy_efuse[0x0e] = hci_tp_lgc_efuse[0x1E];
		hci_tp_phy_efuse[0x0f] = hci_tp_lgc_efuse[0x1f];
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
            bt_dck_write_btonly(hci_tp_phy_efuse[0x0e], hci_tp_phy_efuse[0x0f]);
#else
            bt_dck_write(hci_tp_phy_efuse[0x0e], hci_tp_phy_efuse[0x0f]);
#endif
		return true;
	}
	else
	{
		hci_board_debug("\r\n%s: NO IQK LOK DATA need start LOK,\r\n", __FUNCTION__);
		return false;
	}
}

bool hci_start_iqk(void)
{
	uint32_t ret = 0;
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
	bt_iqk_8710c_btonly(&g_iqk_data);
#else
	ret = bt_iqk_8710c(&g_iqk_data, 0);
#endif
	if(_FAIL == ret)
	{
		hci_board_debug("\r\n%s:  Warning: IQK Fail, please connect driver !!!!!!!!!\r\n", __FUNCTION__);
		return false;
	}
	
	bt_dump_iqk(&g_iqk_data);
#if defined(CONFIG_BT_ONLY_WITHOUT_WLAN) && CONFIG_BT_ONLY_WITHOUT_WLAN
            bt_lok_write_bt_only(g_iqk_data.IDAC, g_iqk_data.QDAC, g_iqk_data.IDAC2, g_iqk_data.QDAC2);
#else
            bt_lok_write(g_iqk_data.IDAC, g_iqk_data.QDAC, g_iqk_data.IDAC2, g_iqk_data.QDAC2);
#endif
	
	hci_tp_phy_efuse[0] = 0;
    hci_tp_phy_efuse[1] =hci_tp_phy_efuse[1] & (~BIT0);
    //hci_tp_phy_efuse[1] = 0xfe;
    //hci_tp_phy_efuse[2] = 0xff;
	hci_tp_phy_efuse[3] = g_iqk_data.IQK_xx & 0xff;
	hci_tp_phy_efuse[4] = (g_iqk_data.IQK_xx >> 8) & 0xff;
	hci_tp_phy_efuse[5] = g_iqk_data.IQK_yy & 0xff;
	hci_tp_phy_efuse[6] = (g_iqk_data.IQK_yy >> 8) & 0xff;
	
	reset_iqk_type();

	return true;
}

static bool mp_driver_init_done = 0;

bool bt_mp_driver_init_done(void)
{
	return mp_driver_init_done;
}

bool hci_board_complete(void)
{
	if (rltk_wlan_is_mp() ) {
        hci_board_debug("EFUSE_SW_MP_MODE: UPPERSTACK NOT UP \r\nGNT_BT %x...\n", (unsigned int)HAL_READ32(0x40080000, 0x0764));
		mp_driver_init_done = 1;
		return false;
	}

       hci_board_debug("Start upperStack\n");

	return true;
}

void bt_write_lgc_efuse_value(void)
{
    hci_tp_lgc_efuse[0x16] = g_iqk_data.IQK_xx & 0xff;
    hci_tp_lgc_efuse[0x17] = (g_iqk_data.IQK_xx >> 8) & 0xff;
    hci_tp_lgc_efuse[0x18] = g_iqk_data.IQK_yy & 0xff;
    hci_tp_lgc_efuse[0x19] = (g_iqk_data.IQK_yy >> 8) & 0xff;
    hci_tp_lgc_efuse[0x1a] = g_iqk_data.QDAC;
    hci_tp_lgc_efuse[0x1b] = g_iqk_data.IDAC;
    hci_tp_lgc_efuse[0x1c] = g_iqk_data.QDAC2;
    hci_tp_lgc_efuse[0x1d] = g_iqk_data.IDAC2;

    hci_board_debug("\r\n write logic efuse 0x1A6 =0x%02x", hci_tp_lgc_efuse[0x16]);
    hci_board_debug("\r\n write logic efuse 0x1A7 =0x%02x", hci_tp_lgc_efuse[0x17]);
    hci_board_debug("\r\n write logic efuse 0x1A8 =0x%02x", hci_tp_lgc_efuse[0x18]);
    hci_board_debug("\r\n write logic efuse 0x1A9 =0x%02x", hci_tp_lgc_efuse[0x19]);
    hci_board_debug("\r\n write logic efuse 0x1Aa =0x%02x", hci_tp_lgc_efuse[0x1a]);
    hci_board_debug("\r\n write logic efuse 0x1Ab =0x%02x", hci_tp_lgc_efuse[0x1b]);
    hci_board_debug("\r\n write logic efuse 0x1Ac =0x%02x", hci_tp_lgc_efuse[0x1c]);
    hci_board_debug("\r\n write logic efuse 0x1Ad =0x%02x", hci_tp_lgc_efuse[0x1d]);
    //EFUSE_LMAP_WRITE(0x1A4,8,(uint8_t *)&hci_tp_lgc_efuse[0x14]);
}

int bt_get_mac_address(uint8_t *mac)
{
    uint8_t addr_size = 6;
    uint8_t read_ddr[6];

    //Check BT address
    device_mutex_lock(RT_DEV_LOCK_EFUSE);
    efuse_logical_read(BOARD_LOGIC_EFUSE_OFFSET, addr_size, read_ddr);
    device_mutex_unlock(RT_DEV_LOCK_EFUSE);

    memcpy(mac, read_ddr, addr_size);
    
    return 0;
}

int bt_set_mac_address(uint8_t *mac)
{
    uint8_t addr_size = 6;
    int ret = 0;
    uint8_t read_ddr[6];

    device_mutex_lock(RT_DEV_LOCK_EFUSE);
    efuse_logical_write(BOARD_LOGIC_EFUSE_OFFSET, addr_size, mac);

    //Check BT address
    efuse_logical_read(BOARD_LOGIC_EFUSE_OFFSET, addr_size, read_ddr);
    device_mutex_unlock(RT_DEV_LOCK_EFUSE);

    if(memcmp(read_ddr, mac, addr_size))
    {
        ret = -1;
    }

    return ret;
}