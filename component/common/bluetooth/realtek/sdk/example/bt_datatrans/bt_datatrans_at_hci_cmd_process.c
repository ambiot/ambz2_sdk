enum { __FILE_NUM__ = 0 };

/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     hci_cmd_event.c
* @brief
* @details
* @author   parker_xue
* @date     2017_4_20
* @version  v0.1
*********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include "trace_app.h"
#include "bt_datatrans_at_hci_cmd_process.h"
#include "gap.h"
#include "gap_conn_le.h"
#include "gap_bond_le.h"
#include "gap_adv.h"
#include "gap_scan.h"
#include "bt_datatrans_multilink_manager.h"
#include <string.h>
#include "bt_datatrans_profile.h"
#include "bt_datatrans_module_param_config.h"
#include "os_timer.h"
#include "ftl_app.h"
#include <os_sync.h>
#include <os_msg.h>
#include "os_mem.h"
#include <platform/platform_stdlib.h>
#include "bt_datatrans_app_flags.h"
#include "bt_datatrans_uart.h"

#define DEVICE_NAME_MAX_LENGTH      15

extern T_GAP_DEV_STATE bt_datatrans_gap_dev_state;
extern T_GAP_CONN_STATE bt_datatrans_gap_conn_state;
extern T_SERVER_ID bt_datatrans_srv_id;

extern uint8_t DT_SCAN_RSP_DATA[31];
extern uint8_t DT_ADV_DATA[31];

void Device_Name_Write(uint8_t *payload, uint16_t payloadlen)
{
    memset(dataTransInfo.devicename_info.device_name, 0, DEVICE_NAME_MAX_LENGTH);
    memcpy(dataTransInfo.devicename_info.device_name, payload, payloadlen);
    dataTransInfo.devicename_info.length = payloadlen;

    if (dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
    {
        if (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            le_adv_stop();
            transferConfigInfo.adv_param_update = true;
        }
        else
        {
            moduleParam_InitAdvAndScanRspData();
            uint8_t DeviceName[GAP_DEVICE_NAME_LEN] = {0};
            memcpy(DeviceName, dataTransInfo.devicename_info.device_name, dataTransInfo.devicename_info.length);
            le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, DeviceName);
        }
    }
    else
    {
        moduleParam_InitAdvAndScanRspData();
        uint8_t DeviceName[GAP_DEVICE_NAME_LEN] = {0};
        memcpy(DeviceName, dataTransInfo.devicename_info.device_name, dataTransInfo.devicename_info.length);
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, DeviceName);
    }
    //save device name
    ftl_save(&dataTransInfo.devicename_info, INFO_DEVICE_NAME_OFFSET, 16);
	
}


void Pair_Mode_Set(uint8_t pairmode)
{
    uint16_t auth_flags = GAP_AUTHEN_BIT_NONE;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_sec_req_enalbe = true;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_NONE;

    switch (dataTransInfo.pair_info.pair_mode)
    {
    case NO_PASS_WORD:
        {
            auth_sec_req_enalbe = false;
        }
        break;
    case JUST_WORK:
        break;
    case PASS_WORD:
        {
            auth_flags = GAP_AUTHEN_BIT_MITM_FLAG;
            auth_sec_req_flags = GAP_AUTHEN_BIT_MITM_FLAG;
            if (dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
            {
                auth_io_cap = GAP_IO_CAP_KEYBOARD_ONLY;
            }
#if CENTRAL_MODE
            else if (dataTransInfo.device_mode.role == ROLE_CENTRAL)
            {
                auth_io_cap = GAP_IO_CAP_DISPLAY_ONLY;
            }
#endif
        }
        break;
    case PASS_WORD_BOND:
        {
            auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_MITM_FLAG;
            auth_sec_req_flags = GAP_AUTHEN_BIT_MITM_FLAG;
            if (dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
            {
                auth_io_cap = GAP_IO_CAP_KEYBOARD_ONLY;
            }
#if CENTRAL_MODE
            else if (dataTransInfo.device_mode.role == ROLE_CENTRAL)
            {
                auth_io_cap = GAP_IO_CAP_DISPLAY_ONLY;
            }
#endif
        }
        break;
    default:
        break;
    }
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enalbe), &auth_sec_req_enalbe);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);
    gap_set_pairable_mode();
    dataTransInfo.pair_info.authenflag.value = auth_flags;
    APP_PRINT_INFO2("sc_flag is %x,mitm_flag is %x", \
                    dataTransInfo.pair_info.authenflag.flag.sc_flag, dataTransInfo.pair_info.authenflag.flag.mitm_flag);
    ftl_save(&dataTransInfo.pair_info, INFO_PAIR_INFO_OFFSET, 4);
}


void ADV_Interval_Set(uint16_t newadvi)
{
    if (dataTransInfo.adv_interval != newadvi)
    {
        dataTransInfo.adv_interval = newadvi;
        if (dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
        {
            if (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
            {
                APP_PRINT_INFO0("GAP adv stoped: because adv interval update");
                le_adv_stop();
                transferConfigInfo.adv_param_update = true;
            }
            else
            {
                uint16_t adv_int_min = newadvi;
                uint16_t adv_int_max = newadvi;
                le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
                le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
            }
        }
        dataTransInfo.adv_interval = newadvi;
        ftl_save(&dataTransInfo.adv_interval, INFO_ADV_INTERVAL_OFFSET, 4);
    }
}


bool ADV_Enable_Disable(uint8_t adven)
{
    bool ret = true;
    if ((adven == 1) && (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_IDLE))
    {
        le_adv_start();
    }
    else if ((adven == 0) && (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING))
    {
        le_adv_stop();
    }
    else
    {
        ret = false;
    }
    return ret;
}


void Role_Set(uint8_t newrole)
{
    uint8_t oldrole = dataTransInfo.device_mode.role;
    if (oldrole != newrole)
    {
        dataTransInfo.device_mode.role = newrole;
        if (oldrole == ROLE_BEACON)
        {
            os_timer_start(&TimersReset);
            //ftl_save(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4);
        }
        else
        {
            if (newrole == ROLE_BEACON)
            {
                os_timer_start(&TimersReset);
                //ftl_save(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4);
            }
            else
            {
                if (dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
                {
                    if (bt_datatrans_gap_dev_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
                    {
                        if (dataTransInfo.device_mode.adv_mode)
                        {
                            le_adv_start();
                        }
                    }
#if CENTRAL_MODE
                    else if (bt_datatrans_gap_dev_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
                    {
                        if (dataTransInfo.device_mode.adv_mode)
                        {
                            transferConfigInfo.stop_scan_then_adv = true;
                        }

                        le_scan_stop();
                    }
#endif
                }
#if CENTRAL_MODE
                else if (dataTransInfo.device_mode.role == ROLE_CENTRAL &&
                         bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
                {
                    le_adv_stop();
                }
#endif
            }
        }

        // write config
        ftl_save(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4);
        if (ftl_load(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4))
        {
            APP_PRINT_ERROR0("ROLE Write err.");
        }
        else
        {
            APP_PRINT_INFO1("ROLE xx is %d", dataTransInfo.device_mode.role);
			printf("ROLE xx is %d\n\r", dataTransInfo.device_mode.role);
        }
    }
}

#if CENTRAL_MODE
void Connect_Device_Num(uint8_t connindex)
{
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6; //6*1.25=7.5ms
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 1000; 
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (bt_datatrans_gap_dev_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
    {
        le_scan_stop();
        transferConfigInfo.create_connection = true;
        transferConfigInfo.connect_by_add = false;
        transferConfigInfo.connect_dev_num = connindex;
    }
    else if (bt_datatrans_gap_dev_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
    {
        APP_PRINT_ERROR0("le_connect");
		printf("le_connect\n\r");
        le_connect(0, DT_DevList[connindex].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)DT_DevList[connindex].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC,
                   1000);

        os_timer_start(&TimersConnTimeOut);
    }
}
#endif

void Enable_New_ADV_SCANRSP_Data(void)
{
    if (dataTransInfo.device_mode.role == ROLE_BEACON)
    {
        if (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            le_adv_stop();
            transferConfigInfo.adv_param_update = true;
        }
        else if (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            moduleParam_InitAdvAndScanRspData();
        }
    }
}


void DataTrans_SendBTFlowResponse(void)
{
    if (transferConfigInfo.bt_buf_free)
    {
        uint8_t data = 0x01;
        server_send_data(CON_ID, bt_datatrans_srv_id, GATT_UUID_CHAR_FLOW_NOTIFY_INDEX,
                         &data, sizeof(uint8_t), GATT_PDU_TYPE_ANY);
        transferConfigInfo.bt_buf_free--;
    }
}

T_APP_RESULT HandleBTReceiveData(uint8_t conn_id, uint16_t wLength, uint8_t *pValue)
{
    T_APP_RESULT  wCause = APP_RESULT_SUCCESS;
    PTxData pTxData = NULL;

    uint32_t flags;
    flags = os_lock();
    pTxData = datatrans_app_queue_out(&txUartDataQueueFree);
    os_unlock(flags);
    if (pTxData != NULL)
    {   	
		// tx_buffer allocate memmory
        uint16_t mem_length = wLength + 4 - wLength % 4;
        pTxData->tx_buffer = os_mem_alloc(RAM_TYPE_DATA_ON, mem_length);
        if (pTxData->tx_buffer != NULL)
        {
            memcpy(pTxData->tx_buffer, pValue, wLength);
            pTxData->length = wLength;
            pTxData->is_stack_buf = false;
            pTxData->stack_buf_offset = 0;
        }
		
		if (pTxData->tx_buffer != NULL && (pTxData->stack_buf_offset == 0))
        {
            APP_PRINT_INFO2("DTS_DATA_NOTIFY: value_size %d, value %b",
                            pTxData->length, TRACE_BINARY(pTxData->length, pTxData->tx_buffer + pTxData->stack_buf_offset));
            if (os_msg_send(TxMessageQueueHandle, &pTxData, 0) == false)
            {
                APP_PRINT_INFO0("HandleBTReceiveData:send queue failed\n");
                //gap_buffer_free(pTxData->tx_buffer);
                os_mem_free(pTxData->tx_buffer); 
                pTxData->tx_buffer = NULL;

                uint32_t flags;
                flags = os_lock();//enter critical section
                datatrans_app_queue_in(&txUartDataQueueFree, pTxData);
                os_unlock(flags); //exit critical section
            }
            
            wCause = APP_RESULT_SUCCESS; 
			
        }
        else
        {
            APP_PRINT_INFO0("HandleBTReceiveData: get stack buffer fail\n");
            uint32_t flags;
            flags = os_lock();//enter critical section
            datatrans_app_queue_in(&txUartDataQueueFree, pTxData);
            os_unlock(flags); //exit critical section
        }
    }
    else
    {
        APP_PRINT_INFO0("HandleBTReceiveData: queue is full\n");
    }

    return wCause;
}


void TxHandleTask(void *pParameters)
{
    uint32_t i = 0;
    uint8_t *pBuffer = NULL;
    PTxData pData = NULL;

    while (true)
    {
        if (os_msg_recv(TxMessageQueueHandle, &pData, 0xFFFFFFFF) == true)
        {
            APP_PRINT_INFO1("TxHandleTask: %d", pData->length);

			pBuffer = pData->tx_buffer + pData->stack_buf_offset;
			*(pBuffer + pData->length)= '\0';
			//APP_PRINT_INFO2("TxHandle received BT data: value_size %d, value %s",pData->length, TRACE_STRING(pBuffer)); //debug
			
			//send bt data to uart tx fifo
			for (i = 0; i < pData->length; i++)
			{
				bt_datatrans_uart_tx_char(pBuffer[i]);
			}

			
            os_mem_free(pData->tx_buffer);
            pData->tx_buffer = NULL;

            uint32_t flags;
            flags = os_lock();//enter critical section
            datatrans_app_queue_in(&txUartDataQueueFree, pData);
            os_unlock(flags); //exit critical section

            DataTrans_SendBTFlowResponse();

        }
        else
        {
            APP_PRINT_INFO0("TxHandleTask: xQueueReceive fail");
        }
    }
}

#endif
