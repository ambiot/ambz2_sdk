enum { __FILE_NUM__ = 0 };

/**
*********************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     module_param_config.c
* @brief
* @details
* @author       jane
* @date     2015-03-19
* @version  v0.1
*********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include <trace_app.h>
#include "bt_datatrans_module_param_config.h"
#include "gap_conn_le.h"
#include <string.h>
#include <gap_adv.h>
#include <os_msg.h>
#include <os_task.h>
#include <os_sync.h>
#include "os_timer.h"
#include "bt_datatrans_uart.h"
#include "ftl_app.h"
#include <platform/platform_stdlib.h>
#include "sys_api.h"

uint16_t    MTU_SIZE = 23;
uint8_t     BT_Credits = 10;
uint8_t     CON_ID = 0;

const uint16_t defaultNameLength = 7;
const char defaultName[7] = {'R', 'e', 'a', 'l', 't', 'e', 'k'};

const uint8_t  default_service_uuid[16] = {0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFF, 0xE0, 0x00, 0x00};
const uint16_t default_service_uuid_len = 16;

const uint16_t default_connection_interval = 0x06;//6*1.25 = 7.5ms
const uint16_t default_slave_lantency = 0;
const uint16_t default_supervision_timeout = 160;//160*10 = 1.6s
const uint16_t default_adv_interval = 320;
const uint16_t default_wake_delay = 200; //default wake delay 200ms;
const uint32_t defaultPin = 123456;

const uint16_t default_write_uuid = 0xFFE1;
const uint16_t default_notify_uuid = 0xFFE2;
const uint16_t default_flow_uuid = 0xFFE3;



const uint32_t defaultbaudrate = 115200;
const uint8_t default_adv_mode = 1;
const uint8_t default_uart_flow = 0;
const uint8_t default_bt_flow = 0;
const uint8_t default_gap_role = 0xA0;
const uint8_t default_pair = 0;
const uint8_t default_tx_power = 0;
const uint8_t default_rom_version = 0;
const uint8_t default_patch_version = 0;

extern T_GAP_CONN_STATE bt_datatrans_gap_conn_state;


ReceiveBufStruct        IO_Receive;

uint32_t datatrans_efuse_failed_addr = 0;

//T_DATATRANS_KEY_INFO g_ble_key_info;

void *TimersUartConfigChange;
void *TimersReset;
void *TimersConnTimeOut;
//void *TimersEnterLowPower;
void *TimersConnParamUpdate;
//void *TimersSwitchToHCI;


PBT_UART_BUF g_AppBufCB;
TBT_UART_BUF g_TappbufCB;
QUEUE_T  txUartDataQueueFree;
void *TxMessageQueueHandle;
void *TxTaskHandle;
void *DataTrans_Semaphore;


#define DT_BEACON_EFUSE_SIZE           44

#define DEFAULT_BAUD      BAUD_115200

#define TX_TASK_PRIORITY          1   /* Task priorities. */
#define TX_TASK_STACK_SIZE        256 * 4


#define  ADV_LENGTH_INDEX  21
#define  ADV_REMOTE_NAME_INDEX 23
#define  ADV_DATA_FIX_DATA_LENGTH  23
#define SCAN_RSP_LENGTH_START_INDEX 0
#define SCAN_RSP_LOCAL_NAME_START_INDEX 2
#define SCAN_DATA_FIX_DATA_LENGTH 2


// GAP - SCAN RSP data (max size = 31 bytes)
#define DT_SCAN_RSP_LENGTH 9
uint8_t DT_SCAN_RSP_DATA[31] =
{
    0x08,
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'R', 'e', 'a', 'l', 't', 'e', 'k' /* Default local name */

};

// GAP - Advertisement data (max size = 31 bytes, though this is
#define DT_ADV_LENGTH 21
uint8_t DT_ADV_DATA[31] =
{
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    0x11,
    GAP_ADTYPE_128BIT_COMPLETE,
    0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0xFF, 0xE0, 0x00, 0x00,      /* Service UUID */
};


void datatrans_setdefault(void)
{
    dataTransInfo.device_mode.advtype = GAP_ADTYPE_ADV_IND;
    dataTransInfo.device_mode.adv_mode = AUTO_ADV;
    dataTransInfo.device_mode.baudrateidx = DEFAULT_BAUD;
    dataTransInfo.device_mode.bt_flowctrl = false;
    dataTransInfo.device_mode.role = ROLE_PERIPHERAL;
    dataTransInfo.device_mode.sleep_mode = true;
    dataTransInfo.device_mode.uart_flowctrl = false;
    dataTransInfo.device_mode.tx_power = TXP_0DBM;
    dataTransInfo.device_mode.code_mode = 0;
    ftl_save(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4);

    dataTransInfo.devicename_info.length = DEFAULT_DEVICE_NAME_LEN;//false
    memcpy(dataTransInfo.devicename_info.device_name, DEFAULT_DEVICE_NAME, DEFAULT_DEVICE_NAME_LEN);
    ftl_save(&dataTransInfo.devicename_info, INFO_DEVICE_NAME_OFFSET, 16);

    dataTransInfo.uuid_info.length = default_service_uuid_len;//false
    memcpy(dataTransInfo.uuid_info.uuid, default_service_uuid, dataTransInfo.uuid_info.length);
    ftl_save(&dataTransInfo.uuid_info, INFO_UUID_OFFSET, 20);

    //local bda
//    if(ftl_load(&dataTransInfo.local_bda, INFO_ADV_DATA_OFFSET, 8))
//    {
//        dataTransInfo.local_bda.length = default_service_uuid_len;//false
//        memcpy(dataTransInfo.local_bda, default_service_uuid,dataTransInfo.uuid_info.length);
//    }
    //DBG_DIRECT("local_bda%x:%x:%x:%x:%x:%x",dataTransInfo.local_bda.bda[0],dataTransInfo.local_bda.bda[1],dataTransInfo.local_bda.bda[2],
    //                                dataTransInfo.local_bda.bda[3],dataTransInfo.local_bda.bda[4],dataTransInfo.local_bda.bda[5]);


    dataTransInfo.advdata.length = DT_ADV_LENGTH;
    memcpy(dataTransInfo.advdata.adv, DT_ADV_DATA, DT_ADV_LENGTH);
    ftl_save(&dataTransInfo.advdata, INFO_ADV_DATA_OFFSET, 32);

    dataTransInfo.scanrspdata.length = DT_SCAN_RSP_LENGTH;
    memcpy(dataTransInfo.scanrspdata.adv, DT_SCAN_RSP_DATA, DT_SCAN_RSP_LENGTH);
    ftl_save(&dataTransInfo.scanrspdata, INFO_RSP_DATA_OFFSET, 32);

    dataTransInfo.char_uuid.write_uuid = default_write_uuid;//false
    dataTransInfo.char_uuid.notify_uuid = default_notify_uuid;
    dataTransInfo.char_uuid.flow_uuid = default_flow_uuid;
    ftl_save(&dataTransInfo.char_uuid, INFO_CHAR_UUID_OFFSET, 8);

    dataTransInfo.connection_para.interval_min = default_connection_interval;
    dataTransInfo.connection_para.interval_max = default_connection_interval;
    dataTransInfo.connection_para.slave_lantency = default_slave_lantency;
    dataTransInfo.connection_para.supervision_timeout = default_supervision_timeout;
    //load connection parameter
    ftl_save(&dataTransInfo.connection_para, INFO_CON_PARA_OFFSET, 8);

    dataTransInfo.adv_interval = default_adv_interval;
    dataTransInfo.wake_delay   = default_wake_delay;
    ftl_save(&dataTransInfo.adv_interval, INFO_ADV_INTERVAL_OFFSET, 4);

    dataTransInfo.pincode = defaultPin;
    ftl_save(&dataTransInfo.pincode, INFO_PINCODE_OFFSET, 4);

    dataTransInfo.pair_info.authenflag.value = GAP_AUTHEN_BIT_NONE;
    dataTransInfo.pair_info.authen_iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    dataTransInfo.pair_info.auto_security_req = false;
    dataTransInfo.pair_info.paired_flag = false;
    dataTransInfo.pair_info.pair_mode = NO_PASS_WORD;
    ftl_save(&dataTransInfo.pair_info, INFO_PAIR_INFO_OFFSET, 4);

    transferConfigInfo.baudrate = 115200;
    //read version
//    transferConfigInfo.app_version = 0x1111;
//    transferConfigInfo.patch_version = 0x2222222;
    transferConfigInfo.adv_param_update = false;
    transferConfigInfo.stop_scan_then_adv = false;
    //transferConfigInfo.tx_power_upd = false;
}
void readconfig(void)
{
    //load mode
    if (ftl_load(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4))
    {
        dataTransInfo.device_mode.advtype = GAP_ADTYPE_ADV_IND;
        dataTransInfo.device_mode.adv_mode = AUTO_ADV;
        dataTransInfo.device_mode.baudrateidx = DEFAULT_BAUD;
        dataTransInfo.device_mode.bt_flowctrl = false;
        dataTransInfo.device_mode.role = ROLE_PERIPHERAL;
        dataTransInfo.device_mode.sleep_mode = true;
        dataTransInfo.device_mode.uart_flowctrl = false;
        dataTransInfo.device_mode.tx_power = TXP_0DBM;
        dataTransInfo.device_mode.code_mode = 0;
    }

    switch (dataTransInfo.device_mode.baudrateidx)
    {
    case BAUD_2400:
        transferConfigInfo.baudrate = 2400;
        break;
    case BAUD_4800:
        transferConfigInfo.baudrate = 4800;
        break;
    case BAUD_9600:
        transferConfigInfo.baudrate = 9600;
        break;
    case BAUD_19200:
        transferConfigInfo.baudrate = 19200;
        break;
    case BAUD_38400:
        transferConfigInfo.baudrate = 38400;
        break;
    case BAUD_57600:
        transferConfigInfo.baudrate = 57600;
        break;
    case BAUD_115200:
        transferConfigInfo.baudrate = 115200;
        break;
    case BAUD_921600:
        transferConfigInfo.baudrate = 921600;
        break;
    case BAUD_2000000:
        transferConfigInfo.baudrate = 2000000;
        break;
    default:
        transferConfigInfo.baudrate = 115200;
        break;
    }
	
    //load device name
    if (ftl_load(&dataTransInfo.devicename_info, INFO_DEVICE_NAME_OFFSET, 16))
    {
        dataTransInfo.devicename_info.length = DEFAULT_DEVICE_NAME_LEN;
        memcpy(dataTransInfo.devicename_info.device_name, DEFAULT_DEVICE_NAME, DEFAULT_DEVICE_NAME_LEN);
        ftl_save(&dataTransInfo.devicename_info, INFO_DEVICE_NAME_OFFSET, 16);
    }

    //load uuid
    if (ftl_load(&dataTransInfo.uuid_info, INFO_UUID_OFFSET, 20))
    {
        dataTransInfo.uuid_info.length = default_service_uuid_len;//false
        memcpy(dataTransInfo.uuid_info.uuid, default_service_uuid, dataTransInfo.uuid_info.length);
    }
    
    //load bda
    if (ftl_load(&dataTransInfo.local_bda, INFO_ADV_DATA_OFFSET, 8))
    {
//        dataTransInfo.uuid_info.length = default_service_uuid_len;//false
//        memcpy(dataTransInfo.uuid_info.uuid, default_service_uuid,dataTransInfo.uuid_info.length);
    }
    //DBG_DIRECT("local_bda%x:%x:%x:%x:%x:%x",dataTransInfo.local_bda.bda[0],dataTransInfo.local_bda.bda[1],dataTransInfo.local_bda.bda[2],
    //                                dataTransInfo.local_bda.bda[3],dataTransInfo.local_bda.bda[4],dataTransInfo.local_bda.bda[5]);
    
    //load adv data
    if (ftl_load(&dataTransInfo.advdata, INFO_ADV_DATA_OFFSET, 32))
    {
        dataTransInfo.advdata.length = DT_ADV_LENGTH;//false
        memcpy(dataTransInfo.advdata.adv, DT_ADV_DATA, DT_ADV_LENGTH);
    }
    //DBG_DIRECT("DT_ADV_LENGTH %d",dataTransInfo.advdata.length);
    
    //load scan rsp data
    if (ftl_load(&dataTransInfo.scanrspdata, INFO_RSP_DATA_OFFSET, 32))
    {
        dataTransInfo.scanrspdata.length = DT_SCAN_RSP_LENGTH;//false
        memcpy(dataTransInfo.scanrspdata.adv, DT_SCAN_RSP_DATA, DT_SCAN_RSP_LENGTH);
    }
    //DBG_DIRECT("DT_SCAN_RSP_LENGTH %d",dataTransInfo.advdata.length);

    //load char uuid
    if (ftl_load(&dataTransInfo.char_uuid, INFO_CHAR_UUID_OFFSET, 8))
    {
        dataTransInfo.char_uuid.write_uuid = default_write_uuid;//false
        dataTransInfo.char_uuid.notify_uuid = default_notify_uuid;
        dataTransInfo.char_uuid.flow_uuid = default_flow_uuid;
    }
    //DBG_DIRECT("write_uuid %x;notify_uuid %x;flow_uuid %x",dataTransInfo.char_uuid.write_uuid,dataTransInfo.char_uuid.notify_uuid,dataTransInfo.char_uuid.flow_uuid);

    //load connection parameter
    if (ftl_load(&dataTransInfo.connection_para, INFO_CON_PARA_OFFSET, 8))
    {
        dataTransInfo.connection_para.interval_min = default_connection_interval;
        dataTransInfo.connection_para.interval_max = default_connection_interval;
        dataTransInfo.connection_para.slave_lantency = default_slave_lantency;
        dataTransInfo.connection_para.supervision_timeout = default_supervision_timeout;
    }

    //load adv interval
    if (ftl_load(&dataTransInfo.adv_interval, INFO_ADV_INTERVAL_OFFSET, 4))
    {
        dataTransInfo.adv_interval = default_adv_interval;
        dataTransInfo.wake_delay   = default_wake_delay;
    }
    //DBG_DIRECT("adv_interval %d.",dataTransInfo.adv_interval);
    
    //load pincode
    if (ftl_load(&dataTransInfo.pincode, INFO_PINCODE_OFFSET, 4))
    {
        dataTransInfo.pincode = defaultPin;
    }
    //DBG_DIRECT("pincode %x.",dataTransInfo.pincode);
    
    //load pair info
    if (ftl_load(&dataTransInfo.pair_info, INFO_PAIR_INFO_OFFSET, 4))
    {
        dataTransInfo.pair_info.authenflag.value = GAP_AUTHEN_BIT_NONE;
        dataTransInfo.pair_info.authen_iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
        dataTransInfo.pair_info.auto_security_req = false;
        dataTransInfo.pair_info.paired_flag = false;
        dataTransInfo.pair_info.pair_mode = NO_PASS_WORD;
    }
    //DBG_DIRECT("pair_mode %x.",dataTransInfo.pair_info.pair_mode);

	transferConfigInfo.select_io  = UART_AT;
	transferConfigInfo.select_mode = DATATRANS_MODE;

	//read version
	transferConfigInfo.app_version = 0;  
	transferConfigInfo.patch_version = 0;  

    transferConfigInfo.adv_param_update = false;
    //transferConfigInfo.tx_power_upd = false;

	APP_PRINT_INFO1("datatrans uart baudrate is %d", transferConfigInfo.baudrate);
	APP_PRINT_INFO1("device name is %s", TRACE_STRING(dataTransInfo.devicename_info.device_name));
	
}

void moduleParam_InitAdvAndScanRspData(void)
{
    if (dataTransInfo.device_mode.role != ROLE_BEACON)
    {
        /* set adv data*/
        if (dataTransInfo.uuid_info.length == 16)
        {
            DT_ADV_DATA[3] = 17;
            DT_ADV_DATA[4] = GAP_ADTYPE_128BIT_COMPLETE;
            memcpy(DT_ADV_DATA + 5, dataTransInfo.uuid_info.uuid, 16);
        }
        else if (dataTransInfo.uuid_info.length == 2)
        {
            DT_ADV_DATA[3] = 3;
            DT_ADV_DATA[4] = GAP_ADTYPE_16BIT_COMPLETE;
            memcpy(DT_ADV_DATA + 5, dataTransInfo.uuid_info.uuid, 2);
        }

        le_adv_set_param(GAP_PARAM_ADV_DATA, 5 + dataTransInfo.uuid_info.length, DT_ADV_DATA);

        /* set scan response data */
        DT_SCAN_RSP_DATA[SCAN_RSP_LENGTH_START_INDEX] = 1 + dataTransInfo.devicename_info.length;
        memcpy(DT_SCAN_RSP_DATA + SCAN_RSP_LOCAL_NAME_START_INDEX,
               dataTransInfo.devicename_info.device_name,
               dataTransInfo.devicename_info.length);
        dataTransInfo.scanrspdata.length = SCAN_DATA_FIX_DATA_LENGTH +
                                           dataTransInfo.devicename_info.length;//???? move
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA,
                         SCAN_DATA_FIX_DATA_LENGTH + dataTransInfo.devicename_info.length,
                         DT_SCAN_RSP_DATA);
    }
    else
    {
        le_adv_set_param(GAP_PARAM_ADV_DATA, dataTransInfo.advdata.length, DT_ADV_DATA);
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, dataTransInfo.scanrspdata.length, DT_SCAN_RSP_DATA);
    }

}


void moduleParam_SetSystemReset(void)
{
    //WDG_SystemReset(RESET_ALL, (T_SW_RESET_REASON)0xE0);
    sys_reset();
}


void TimerResetCallback(void *pxTimer)
{
    APP_PRINT_INFO0("TimerResetCallback.");
    moduleParam_SetSystemReset();
}

void TimersUartConfigChangeCallback(void *pxTimer)
{
    APP_PRINT_INFO0("TimersUartConfigChangeCallback.");
    bt_datatrans_set_baud();
}

void TimerConnTimeOutCallback(void *pxTimer)
{
	APP_PRINT_INFO0("TimerConnTimeOutCallback"); 
	printf("\n\rTimerConnTimeOutCallback\n\r"); 
    AtCmdSendResponse(AtCmdResponseCONNTIMEOUT, strlen(AtCmdResponseCONNTIMEOUT));
}

void TimerConnParamUpdateCallback(void *pxTimer)
{
	APP_PRINT_INFO0("TimerConnParamUpdateCallback"); 
	printf("\n\rTimerConnParamUpdateCallback\n\r"); 

    if (transferConfigInfo.select_io  == UART_AT &&
        bt_datatrans_gap_conn_state == GAP_CONN_STATE_CONNECTED)
    {
        le_update_conn_param(CON_ID, dataTransInfo.connection_para.interval_min, \
                             dataTransInfo.connection_para.interval_max, \
                             dataTransInfo.connection_para.slave_lantency, \
                             dataTransInfo.connection_para.supervision_timeout, \
                             dataTransInfo.connection_para.interval_min * 2 - 1, \
                             dataTransInfo.connection_para.interval_max * 2 - 1);
    }
}


void Datatrans_TimerInit(void)
{
    os_timer_create(&TimersUartConfigChange, "UartConfigChange", 1, 500, false,
                    TimersUartConfigChangeCallback);

    if (TimersUartConfigChange == NULL)
    {
        APP_PRINT_INFO0("TimerInit UartConfigChange init failed");
    }

    os_timer_create(&TimersReset, "TimersReset", 1, 500, false, TimerResetCallback);

    if (TimersReset == NULL)
    {
        APP_PRINT_INFO0("TimerInit TimersReset init failed");
    }

    os_timer_create(&TimersConnTimeOut, "TimersConnTimeOut", 1, 11000, false, TimerConnTimeOutCallback);

    if (TimersConnTimeOut == NULL)
    {
        APP_PRINT_INFO0("TimerInit TimersConnTimeOut init failed");
    }

    os_timer_create(&TimersConnParamUpdate, "TimersConnParamUpdate", 1, 1000, false,
                    TimerConnParamUpdateCallback);

    if (TimersConnParamUpdate == NULL)
    {
        APP_PRINT_INFO0("TimerInit TimersConnParamUpdate init failed");
    }

}

#if 0
void DataTransSemaphoreInit(void)
{
    os_sem_create(&DataTrans_Semaphore, 0, 1);
    if (DataTrans_Semaphore == NULL)
    {
        APP_PRINT_INFO0("DataTrans_Semaphore init failed");
    }
}
#endif
void TxUartQueueInit(void)
{
    uint8_t i = 0;
    uint8_t tx_queue_size = TX_PACKET_COUNT;
    PTxData pTxData = g_AppBufCB->Bt2UART;
    txUartDataQueueFree.ElementCount = 0;
    txUartDataQueueFree.First = NULL;
    txUartDataQueueFree.Last = NULL;

    for (i = 0; i < tx_queue_size; i++)
    {
        pTxData->tx_buffer = NULL;
        datatrans_app_queue_in(&txUartDataQueueFree, pTxData);
        pTxData++;
    }
}

extern void TxHandleTask(void *pParameters);
void DataTransApplicationInit(void)
{
    /* Initialize the Memory of the Queue */
    g_AppBufCB = &g_TappbufCB;
    memset(g_AppBufCB, 0, sizeof(TBT_UART_BUF));

    os_msg_queue_create(&TxMessageQueueHandle, MAX_NUMBER_OF_TX_MESSAGE, sizeof(PTxData));

    TxUartQueueInit();
    Datatrans_TimerInit();
    //DataTransSemaphoreInit();
    os_task_create(&TxTaskHandle, "TxHandle", TxHandleTask, 0, TX_TASK_STACK_SIZE, TX_TASK_PRIORITY);
}

void DataTransApplicationDeinit(void)
{
	if (TxTaskHandle){
		os_task_delete(TxTaskHandle);
	}

	if (TxMessageQueueHandle){
		os_msg_queue_delete(TxMessageQueueHandle);
	}

	if (TimersUartConfigChange){
		os_timer_delete(&TimersUartConfigChange);
	}
	if (TimersReset){
		os_timer_delete(&TimersReset);
	}
	if (TimersConnTimeOut){
		os_timer_delete(&TimersConnTimeOut);
	}
	if (TimersConnParamUpdate){
		os_timer_delete(&TimersConnParamUpdate);
	}

	TxMessageQueueHandle = NULL;
	TimersUartConfigChange = NULL;
	TimersReset = NULL;
	TimersConnTimeOut = NULL;
	TimersConnParamUpdate = NULL;
	TxTaskHandle = NULL;
	
}

#endif


