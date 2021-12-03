/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    multilink_manager.c
  * @brief   Multilink manager functions.
  * @details
  * @author  jane
  * @date    2016-02-18
  * @version v0.1
  ******************************************************************************
  */
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include <bt_datatrans_app_flags.h>
#if CENTRAL_MODE
/** Add Includes here **/
#include "bt_datatrans_multilink_manager.h"
#include "gap.h"
#include "trace_app.h"
#include <string.h>
#include "bt_datatrans_module_param_config.h"
#include <stdio.h>

TAppLinkCB DataTransLinkTable[BT_DATATRANS_APP_MAX_LINKS];
TDevInfo DT_DevList[BT_DATATRANS_APP_MAX_DEVICE_INFO];
uint8_t DTDevListNum = 0;

/**
 * @brief   Split valid information from peer device's ADV data or SCAN RSP data.
 *          NOTE--this function just for demo, user can cutter the code according
 *                to requirement.
 *
 * @param   scan_info - message informed from upper stack
 */
//#ifdef _IS_ASIC_
void DataTrans_Multilink_ParseScanInfo(T_LE_SCAN_INFO *scan_info)
{
    uint8_t buffer[32];
    uint8_t pos = 0;

    while (pos < scan_info->data_len)
    {
        /* Length of the AD structure. */
        uint8_t length = scan_info->data[pos++];
        uint8_t type;

        if ((length < 1) || (length >= 31))
        {
            return;
        }

        if ((length > 0x01) && ((pos + length) <= 31))
        {
            /* Copy the AD Data to buffer. */
            memcpy(buffer, scan_info->data + pos + 1, length - 1);
            /* AD Type, one octet. */
            type = scan_info->data[pos];

            APP_PRINT_INFO2("  AD Structure Info: AD type = 0x%x, AD Data Length = %d", type, length - 1);

            switch (type)
            {
            case GAP_ADTYPE_FLAGS:
                {
                    /* (flags & 0x01) -- LE Limited Discoverable Mode */
                    /* (flags & 0x02) -- LE General Discoverable Mode */
                    /* (flags & 0x04) -- BR/EDR Not Supported */
                    /* (flags & 0x08) -- Simultaneous LE and BR/EDR to Same Device Capable (Controller) */
                    /* (flags & 0x10) -- Simultaneous LE and BR/EDR to Same Device Capable (Host) */
                    uint8_t flags = scan_info->data[pos + 1];
                    APP_PRINT_INFO1("  AD Data: Flags = 0x%x", flags);
                }
                break;

            case GAP_ADTYPE_16BIT_MORE:
            case GAP_ADTYPE_16BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_16BIT:
                {
                    uint16_t *pUUID = (uint16_t *)(buffer);
                    uint8_t i = length - 1;

                    while (i >= 2)
                    {
                        APP_PRINT_INFO2("  AD Data: UUID16 List Item %d = 0x%x", i / 2, *pUUID++);
                        i -= 2;
                    }
                }
                break;

            case GAP_ADTYPE_32BIT_MORE:
            case GAP_ADTYPE_32BIT_COMPLETE:
                {
                    uint32_t *pUUID = (uint32_t *)(buffer);
                    uint8_t    i     = length - 1;

                    while (i >= 4)
                    {
                        APP_PRINT_INFO2("  AD Data: UUID32 List Item %d = 0x%x", i / 4, *pUUID++);
                        i -= 4;
                    }
                }
                break;

            case GAP_ADTYPE_128BIT_MORE:
            case GAP_ADTYPE_128BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_128BIT:
                {
                    uint32_t *pUUID = (uint32_t *)(buffer);
                    APP_PRINT_INFO4("  AD Data: UUID128 value: 0x%8.8x%8.8x%8.8x%8.8x",
                                    pUUID[3], pUUID[2], pUUID[1], pUUID[0]);
                }
                break;

            case GAP_ADTYPE_LOCAL_NAME_SHORT:
            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                {
                    buffer[length - 1] = '\0';
                    APP_PRINT_INFO0("  AD Data: Local Name");
                }
                break;

            case GAP_ADTYPE_POWER_LEVEL:
                {
                    APP_PRINT_INFO1("  AD Data: TX power = 0x%x", scan_info->data[pos + 1]);
                }
                break;

            case GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE:
                {
                    uint16_t *pMin = (uint16_t *)(buffer);
                    uint16_t *pMax = pMin + 1;
                    APP_PRINT_INFO2("  AD Data: Slave conn interval range, 0x%x - 0x%x", *pMin, *pMax);
                }
                break;

            case GAP_ADTYPE_SERVICE_DATA:
            case GAP_ADTYPE_MANUFACTURER_SPECIFIC:
                {
                    uint16_t *pUUID = (uint16_t *)(buffer);
                    uint8_t    i;

                    APP_PRINT_INFO2("  AD Data: Service Data UUID = 0x%x, len = 0x%x", *pUUID, length - 2);

                    for (i = 2; i < length; i++)
                    {
                        APP_PRINT_INFO1("  AD Data: Service Data = 0x%x", buffer[i]);
                    }
                }
                break;

            default:
                {
                    uint8_t i = 0;

                    for (i = 0; i < (length - 1); i++)
                    {
                        APP_PRINT_INFO1("  AD Data: Unhandled Data = 0x%x", scan_info->data[pos + i]);
                    }
                }
                break;
            }
        }

        pos += length;
    }
}
//#endif

/**
 * @brief   Discovery 16-bit uuid information from ADV data or SCAN RSP data.
 *
 * @param   uuid used to filter scan_info
 * @param   scan_info message informed from upper stack
 * @retval true found the uuid from scan_info.
 * @retval false Not found.
 */
bool DataTrans_Multilink_FilterScanInfoByUuid(uint8_t *uuid, T_LE_SCAN_INFO *scan_info)
{
    uint8_t buffer[32];
    uint8_t pos = 0;

    while (pos < scan_info->data_len)
    {
        /* Length of the AD structure. */
        uint8_t length = scan_info->data[pos++];
        uint8_t type;

        if ((length < 1) || (length >= 31))
        {
            return false;
        }

        if ((length > 0x01) && ((pos + length) <= 31))
        {
            /* Copy the AD Data to buffer. */
            memcpy(buffer, scan_info->data + pos + 1, length - 1);
            /* AD Type, one octet. */
            type = scan_info->data[pos];

            switch (type)
            {
            case GAP_ADTYPE_16BIT_MORE:
            case GAP_ADTYPE_16BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_16BIT:
                {
                    if (!memcmp(buffer, uuid, 2))
                    {
                        return true;
                    }
                }
                break;

            case GAP_ADTYPE_128BIT_MORE:
            case GAP_ADTYPE_128BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_128BIT:
                {
                    if (!memcmp(buffer, uuid, 16))
                    {
                        return true;
                    }

                }
                break;

            default:
                break;
            }
        }

        pos += length;
    }
    return false;
}

#if 0
/**
 * @brief   Discovery 16-bit uuid information from ADV data or SCAN RSP data.
 *
 * @param   uuid used to filter ex_scan_info
 * @param   scan_info message informed from upper stack
 * @retval true found the uuid from ex_scan_info.
 * @retval false Not found.
 */
bool DataTrans_Multilink_FilterExScanInfoByUuid(uint8_t *uuid, T_LE_EXT_ADV_REPORT_INFO *scan_info)
{
    uint8_t buffer[32];
    uint8_t pos = 0;

    while (pos < scan_info->data_len)
    {
        /* Length of the AD structure. */
        uint8_t length = scan_info->p_data[pos++];
        uint8_t type;

        if ((length < 1) || (length >= 31))
        {
            return false;
        }

        if ((length > 0x01) && ((pos + length) <= 31))
        {
            /* Copy the AD Data to buffer. */
            memcpy(buffer, scan_info->p_data + pos + 1, length - 1);
            /* AD Type, one octet. */
            type = scan_info->p_data[pos];

            switch (type)
            {
            case GAP_ADTYPE_16BIT_MORE:
            case GAP_ADTYPE_16BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_16BIT:
                {
                    if (!memcmp(buffer, uuid, 2))
                    {
                        return true;
                    }
                }
                break;

            case GAP_ADTYPE_128BIT_MORE:
            case GAP_ADTYPE_128BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_128BIT:
                {
                    if (!memcmp(buffer, uuid, 16))
                    {
                        return true;
                    }

                }
                break;

            default:
                break;
            }
        }

        pos += length;
    }
    return false;
}
#endif

/**
 * @brief   Add device information to device list.
 *
 * @param   bd_addr Peer device address.
 * @param   bd_type Peer device address type.
 * @retval true Success.
 * @retval false Failed, device list is full.
 */
bool DataTrans_Multilink_AddDeviceInfo(uint8_t *bd_addr, uint8_t bd_type)
{
    /* If result count not at max */
    if (DTDevListNum < BT_DATATRANS_APP_MAX_DEVICE_INFO)
    {
        uint8_t i;
        /* Check if device is already in device list*/
        for (i = 0; i < DTDevListNum; i++)
        {
            if (memcmp(bd_addr, DT_DevList[i].bd_addr, GAP_BD_ADDR_LEN) == 0)
            {
                return true;
            }
        }

        /*Add addr to device list list*/
        memcpy(DT_DevList[DTDevListNum].bd_addr, bd_addr, GAP_BD_ADDR_LEN);
        DT_DevList[DTDevListNum].bd_type = bd_type;

		if (transferConfigInfo.select_io == UART_AT)
        {
            char addstr[15];
            addstr[0] = DTDevListNum + '0';
            addstr[1] = ':';

            sprintf(addstr + 2, "%02X%02X%02X%02X%02X%02X", DT_DevList[DTDevListNum].bd_addr[5],
                    DT_DevList[DTDevListNum].bd_addr[4], \
                    DT_DevList[DTDevListNum].bd_addr[3], DT_DevList[DTDevListNum].bd_addr[2],
                    DT_DevList[DTDevListNum].bd_addr[1], DT_DevList[DTDevListNum].bd_addr[0]);

            AtCmdSendResponse((const char *)addstr, 14);
            AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
        }

        /*Increment device list count*/
        DTDevListNum++;
    }
    else
    {
        return false;
    }
    return true;
}

/**
 * @brief   Clear device list.
 * @retval None.
 */
void DataTrans_Multilink_ClearDeviceList(void)
{
    DTDevListNum = 0;
}

#endif

#endif
