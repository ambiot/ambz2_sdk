enum { __FILE_NUM__ = 0 };

/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     at_cmd.c
* @brief
* @details
* @author   jane
* @date     2015-03-29
* @version  v0.2
*********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include "trace_app.h"
#include "bt_datatrans_uart.h"
#include "gap_conn_le.h"
#include <stdio.h>
#include <string.h>
#include "bt_datatrans_module_param_config.h"
#include "bt_datatrans_at_cmd.h"
#include "bt_datatrans_peripheral_application.h"
#include "bt_datatrans_multilink_manager.h"
#include "bt_datatrans_profile.h"
#include "bt_datatrans_at_hci_cmd_process.h"
#include "os_timer.h"
#include <os_task.h>
#include <os_msg.h>
#include <os_sync.h>
#include "bt_datatrans_client.h"
#include "gap_scan.h"
#include "gap_adv.h"
#include "os_mem.h"
#include "gap_bond_le.h"
#include "ftl_app.h"

extern T_GAP_DEV_STATE bt_datatrans_gap_dev_state;
extern T_GAP_CONN_STATE bt_datatrans_gap_conn_state;
extern T_SERVER_ID bt_datatrans_srv_id;
extern T_DATATRANS_REMOTE_BDADDR_INFO connect_dev;

const char AtCmdHeader[AT_CMD_HEADER_LENGTH] = {'A', 'T'};
const char AtCmdResponseOK[] = "OK\r\n";
const char AtCmdResponseERR[] = "ERR\r\n";

const char AtCmdResponseNAME[] = "+NAME=";
const char AtCmdResponseINQS[] = "+INQS\r\n";
const char AtCmdResponseINQE[] = "+INQE\r\n";
const char AtCmdResponseCONNS[] = "+CONNS\r\n";

const char AtCmdResponseBAUD[] = "+BAUD=";
const char AtCmdResponseROLE[] = "+ROLE=";
const char AtCmdResponsePINDIS[] =  "+PINDIS=";
const char AtCmdResponsePAIR[] =  "+PAIR=";
const char AtCmdResponseCONNTIMEOUT[] = "+CONNECTION TIME OUT\r\n";

const char AtCmdResponseADVMOD[] = "+ADVMOD=";
const char AtCmdResponseADVEN[] = "+ADVEN=";
const char AtCmdResponseScanRlt[] = "+SCANLT=";
const char AtCmdResponseVERSION[] =  "+VERSION=";
const char AtCmdResponseConnect[] = "+CONNECTED>>0x";
const char AtCmdResponseDisconnect[] = "+DISCONNECT\r\n";
const char AtCmdResponseDISC[] = "+DISCS\r\n";

extern uint8_t DT_SCAN_RSP_DATA[31];
extern uint8_t DT_ADV_DATA[31];

#define     CompareExitMagic  "AT+XXX\r\n"
#define     MAGIC_NUM                   8 // = "AT+" + "XXX" + "\r\n"

uint8_t sendbuffer[255] = {0};
void Setsendbuffer(uint16_t bufferlen)
{
    uint32_t flags;
    flags = os_lock();//enter critical section

    if (IO_Receive.ReadOffset + bufferlen <= RECEIVE_BUF_MAX_LENGTH)
    {
        memcpy(sendbuffer, IO_Receive.buf + IO_Receive.ReadOffset, bufferlen);
        IO_Receive.ReadOffset += bufferlen;
        if (IO_Receive.ReadOffset == RECEIVE_BUF_MAX_LENGTH)
        {
            IO_Receive.ReadOffset = 0;
        }
    }
    else
    {
        uint16_t len1 = RECEIVE_BUF_MAX_LENGTH - IO_Receive.ReadOffset;
        memcpy(sendbuffer, IO_Receive.buf + IO_Receive.ReadOffset, len1);
        IO_Receive.ReadOffset = 0;
        memcpy(sendbuffer + len1, IO_Receive.buf + IO_Receive.ReadOffset, bufferlen - len1);
        IO_Receive.ReadOffset += bufferlen - len1;
    }

    IO_Receive.datalen -= bufferlen;
    os_unlock(flags); //exit critical section
}

bool DataTrans_GetCMDBuffer(uint16_t len)
{
    uint16_t  rxBufferReadOffset_temp;

    if ((len == 0) || (len > IO_Receive.datalen))
    {
        IO_Receive.datalen = 0;
        IO_Receive.ReadOffset = IO_Receive.WriteOffset;
        return false;
    }

    rxBufferReadOffset_temp = IO_Receive.ReadOffset;
    IO_Receive.atcmdlength = len;
    if (IO_Receive.ReadOffset + len <= RECEIVE_BUF_MAX_LENGTH)
    {
        memcpy(IO_Receive.atcmd, IO_Receive.buf + IO_Receive.ReadOffset, len);
        IO_Receive.ReadOffset += len;
        if (IO_Receive.ReadOffset == RECEIVE_BUF_MAX_LENGTH)
        {
            IO_Receive.ReadOffset = 0;
        }
    }
    else
    {
        uint16_t len1 = RECEIVE_BUF_MAX_LENGTH - IO_Receive.ReadOffset;
        memcpy(IO_Receive.atcmd, IO_Receive.buf + IO_Receive.ReadOffset, len1);
        IO_Receive.ReadOffset = 0;
        memcpy(IO_Receive.atcmd + len1, IO_Receive.buf + IO_Receive.ReadOffset, len - len1);
        IO_Receive.ReadOffset += len - len1;
    }


    if ((len % 8 == 0) && ((IO_Receive.atcmd[len - 1] != '\n') || (IO_Receive.atcmd[len - 2] != '\r')))
    {
        IO_Receive.ReadOffset = rxBufferReadOffset_temp;
        return false;
    }

    uint32_t flags;
    flags = os_lock();//enter critical section
    IO_Receive.datalen -= len;
    os_unlock(flags); //exit critical section

    return true;
}

void DataTrans_HandleATCMD(void)
{
    if (IO_Receive.datalen == 0) //Data length invalid
    {
        return;
    }
	
    if ((bt_datatrans_gap_conn_state == GAP_CONN_STATE_CONNECTED) && (transferConfigInfo.select_mode != CMD_MODE))
    {
        if ((memcmp(IO_Receive.buf + IO_Receive.ReadOffset, CompareExitMagic, MAGIC_NUM) == 0))
        {
            APP_PRINT_ERROR0("Switch to AT mode.");
            transferConfigInfo.select_mode = CMD_MODE;
            uint32_t flags;
            flags = os_lock();//enter critical section
            IO_Receive.datalen = 0;
            IO_Receive.ReadOffset = IO_Receive.WriteOffset;
            os_unlock(flags); //exit critical section
            return;
        }

        uint16_t len = 0;
        le_get_gap_param(GAP_PARAM_LE_REMAIN_CREDITS, &BT_Credits);

		for(; IO_Receive.datalen != 0; )
		{

            if (BT_Credits)
            {
                if (IO_Receive.datalen >= MTU_SIZE - 3) //buffer length > MTU size - 3, send immediately
                {
                    len = MTU_SIZE - 3;
                    Setsendbuffer(len);
                    if (dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
                    {
                        server_send_data(CON_ID, bt_datatrans_srv_id, GATT_UUID_CHAR_DATA_NOTIFY_INDEX, sendbuffer, len,
                                         GATT_PDU_TYPE_ANY);
                    }
#if CENTRAL_MODE
                    else if (dataTransInfo.device_mode.role == ROLE_CENTRAL)
                    {
                        kns_client_write_data_char(CON_ID, len, sendbuffer, GATT_WRITE_TYPE_CMD);
                    }
#endif
                }
                else if (transferConfigInfo.uart_idle)    //buffer length < MTU size - 3
                {
                    len = IO_Receive.datalen;
                    Setsendbuffer(len);
                    if (dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
                    {
                        server_send_data(CON_ID, bt_datatrans_srv_id, GATT_UUID_CHAR_DATA_NOTIFY_INDEX, sendbuffer, len,
                                         GATT_PDU_TYPE_ANY);
                    }
#if CENTRAL_MODE
                    else if (dataTransInfo.device_mode.role == ROLE_CENTRAL)
                    {
                        kns_client_write_data_char(CON_ID, len, sendbuffer, GATT_WRITE_TYPE_CMD);
                    }
#endif
                    transferConfigInfo.uart_idle = 0;
					
                }
                BT_Credits--;

            }
            else
            {
                break;
            }
        }
    }
    else
    {
        uint16_t T_index = IO_Receive.ReadOffset + 1;

        if (T_index >= RECEIVE_BUF_MAX_LENGTH)
        {
            T_index = 0;
        }
        if ((IO_Receive.buf[IO_Receive.ReadOffset] == 'A' ||
             IO_Receive.buf[IO_Receive.ReadOffset] == 'a') && \
            (IO_Receive.buf[T_index] == 'T' ||
             IO_Receive.buf[T_index] == 't'))
        {
            uint16_t len = IO_Receive.datalen;
            APP_PRINT_INFO1("received len is %d", len);
            if (len > AT_CMD_MAX_LENGTH || len < AT_CMD_MIN_LENGTH)
            {
                uint32_t flags;
                flags = os_lock();//enter critical section
                IO_Receive.datalen = 0;
                IO_Receive.ReadOffset = IO_Receive.WriteOffset;
                os_unlock(flags); //exit critical section
            }
            else
            {
                if (DataTrans_GetCMDBuffer(len))
                {
                    AtCmdParse();
                }
            }

        }
        else
        {
            uint32_t flags;
            flags = os_lock();//enter critical section
            IO_Receive.datalen = 0;
            IO_Receive.ReadOffset = IO_Receive.WriteOffset;
            os_unlock(flags); //exit critical section
        }

    }
	
}

/**
* @brief   Send the reponse data of AT cmd to the TX handle task.
* @param   p_resp, the pointer of the response data.
* @param   len, the length of the response data.
* @return  void
*/
void AtCmdSendResponse(const char *p_resp, uint16_t len)
{
    PTxData pTxData = NULL;

    uint32_t flags;
    flags = os_lock();
    pTxData = datatrans_app_queue_out(&txUartDataQueueFree);
    os_unlock(flags);
    if (pTxData != NULL)
    {
        uint16_t mem_length = len + 4 - len % 4;
        pTxData->tx_buffer = os_mem_alloc(RAM_TYPE_DATA_ON, mem_length);
        if (pTxData->tx_buffer != NULL)
        {
            memcpy(pTxData->tx_buffer, p_resp, len);
            pTxData->length = len;
            pTxData->is_stack_buf = false;
            pTxData->stack_buf_offset = 0;

            if (os_msg_send(TxMessageQueueHandle, &pTxData, 0) == false)
            {
                APP_PRINT_INFO0("DataTrans_SendResp:send data failed\n");
                os_mem_free(pTxData->tx_buffer);   //send message fail, free memory
                pTxData->tx_buffer = NULL;
                uint32_t flags;
                flags = os_lock();//enter critical section
                datatrans_app_queue_in(&txUartDataQueueFree, pTxData);
                os_unlock(flags); //exit critical section
            }
        }
        else
        {
            APP_PRINT_INFO0("AtCmdSendResponse: mem malloc fail\n");
            uint32_t flags;
            flags = os_lock();//enter critical section
            datatrans_app_queue_in(&txUartDataQueueFree, pTxData);
            os_unlock(flags); //exit critical section
        }
    }
    else
    {
        APP_PRINT_INFO0("AtCmdSendResponse: queue is full\n");
    }

}

/**
* @brief   convert num to string.
* @param   num, the number need to convert
* @param   str, the result
* @return  void
*/
void DataTrans_Itoa(uint32_t num, char *str)
{
    uint32_t i = 0, j, k;
    do
    {
        str[i++] = (num % 10) + '0';
        num = num / 10;
    }
    while (num);

    str[i] = '\0';
    k = 0;
    char temp;
    for (j = k; j <= (i - 1) / 2; j++)
    {
        temp = str[j];
        str[j] = str[i - 1 + k - j];
        str[i - 1 + k - j] = temp;
    }
}

/**
* @brief   convert string to uint32_t.
* @param   str
* @return  uint32_t
*/
uint32_t DataTrans_Atoi(const char *str)
{
    uint32_t res = 0, begin = 0;

    while (*str != '\0')
    {
        if (begin == 0 && (('0' <= *str && *str <= '9') || *str == '-'))
        {
            begin = 1;
        }
        else if (begin == 1 && (*str < '0' || *str > '9'))
        {
            break;
        }
        if (begin == 1)
        {
            res = res * 10 + (*str - '0');
        }
        str++;
    }
    return res;
}

void DataTrans_Hex2String(uint8_t *Src, uint8_t *Dest, uint8_t len)
{
    uint8_t i = 0;
    uint8_t byte_low = 0;
    uint8_t byte_high = 0;
    for (i = 0; i < len; i++)
    {
        byte_low = Src[i] % 16;
        byte_high = Src[i] / 16;
        if (byte_low <= 9)
        {
            Dest[i * 2 + 1] = byte_low + '0';
        }
        else
        {
            Dest[i * 2 + 1] = byte_low - 10 + 'A';
        }
        if (byte_high <= 9)
        {
            Dest[i * 2] = byte_high + '0';
        }
        else
        {
            Dest[i * 2] = byte_high - 10 + 'A';
        }
    }
}

bool DataTrans_String2Hex(uint8_t *Src, uint8_t *Dest, uint8_t len)
{
    uint8_t i = 0;
    uint8_t byte_low = 0;
    uint8_t byte_high = 0;
    for (i = 0; i < len; i = i + 2)
    {
        if (Src[i] >= '0' && Src[i] <= '9')
        {
            byte_high = Src[i] - '0';
        }
        else if (Src[i] >= 'A' && Src[i] <= 'F')
        {
            byte_high = Src[i] - 'A' + 10;
        }
        else if (Src[i] >= 'a' && Src[i] <= 'f')
        {
            byte_high = Src[i] - 'a' + 10;
        }
        else
        {
            return false;
        }

        if (Src[i + 1] >= '0' && Src[i + 1] <= '9')
        {
            byte_low = Src[i + 1] - '0';
        }
        else if (Src[i + 1] >= 'A' && Src[i + 1] <= 'F')
        {
            byte_low = Src[i + 1] - 'A' + 10;
        }
        else if (Src[i + 1] >= 'a' && Src[i + 1] <= 'f')
        {
            byte_low = Src[i + 1] - 'a' + 10;
        }
        else
        {
            return false;
        }
        Dest[i / 2] = (byte_high << 4) | byte_low;
    }

    return true;
}


/* ****************************************** AtCmdSendResponse  ******************************************* */
/**
* @brief   Send the reponse data of AT\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseOK()
{
    AtCmdSendResponse(AtCmdResponseOK, strlen(AtCmdResponseOK));
}

void AtCmdSendResponseERR()
{
    AtCmdSendResponse(AtCmdResponseERR, strlen(AtCmdResponseERR));
}

void AtCmdSendResponseConnect(uint8_t conn_id)//,uint8_t role)
{
    char bda[12];
    AtCmdSendResponse(AtCmdResponseConnect, strlen(AtCmdResponseConnect));

    le_get_conn_param(GAP_PARAM_CONN_BD_ADDR, connect_dev.bdaddr, conn_id);
    DataTrans_Hex2String(connect_dev.bdaddr, (uint8_t *)bda, 6);

    AtCmdSendResponse((const char *)bda, sizeof(bda));
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
}

void AtCmdSendResponseDisconnect()
{
    AtCmdSendResponse(AtCmdResponseDisconnect, strlen(AtCmdResponseDisconnect));
}

void AtCmdSendResponsePinDis()
{
    char pin_buffer[10] = {0};

    DataTrans_Itoa(dataTransInfo.pincode,  pin_buffer);
    AtCmdSendResponse(AtCmdResponsePINDIS, strlen(AtCmdResponsePINDIS));
    AtCmdSendResponse((const char *)(pin_buffer), strlen(pin_buffer));
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
}


/**
* @brief   Send the reponse data of AT+NAME\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseName()
{
    AtCmdSendResponse(AtCmdResponseNAME, strlen(AtCmdResponseNAME));
    AtCmdSendResponse((const char *)(dataTransInfo.devicename_info.device_name),
                      dataTransInfo.devicename_info.length);
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
}


/**
* @brief   Send the reponse data of AT+BAUD<Param>\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseBaudConfig()
{
    AtCmdSendResponse(AtCmdResponseBAUD, strlen(AtCmdResponseBAUD));
    AtCmdSendResponse((const char *)(IO_Receive.atcmd + 8),
                      IO_Receive.atcmdlength - AT_CMD_BAUD_LENGTH - 1);
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
    AtCmdSendResponseOK();
}

/**
* @brief   Send the reponse data of AT+BAUD\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseBaud()
{
    char baud[10] = {0};
    DataTrans_Itoa(transferConfigInfo.baudrate, baud);
    AtCmdSendResponse(AtCmdResponseBAUD, strlen(AtCmdResponseBAUD));
    AtCmdSendResponse((const char *)baud, strlen(baud));
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
}

void AtCmdSendResponseDISC()
{
	AtCmdSendResponseOK();
	AtCmdSendResponse(AtCmdResponseDISC, strlen(AtCmdResponseDISC));
}

/**
* @brief   Send the reponse data of AT+ROLE<Param>\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseRoleConfig()
{
    AtCmdSendResponse(AtCmdResponseROLE, strlen(AtCmdResponseROLE));
    AtCmdSendResponse((const char *)(IO_Receive.atcmd + 8), 1);
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
    AtCmdSendResponseOK();
}

/**
* @brief   Send the reponse data of AT+ROLE\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseRole()
{

    char index = (char)(dataTransInfo.device_mode.role  + '0');
    const char *gaproleindex = (const char *)(&index);
    AtCmdSendResponse(AtCmdResponseROLE, strlen(AtCmdResponseROLE));
    AtCmdSendResponse(gaproleindex, 1);
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
}


void AtCmdSendResponsePair()//pair flg
{
    char index = (char)(dataTransInfo.pair_info.pair_mode + '0');
    const char *pairindex = (const char *)(&index);
    AtCmdSendResponse(AtCmdResponsePAIR, strlen(AtCmdResponsePAIR));
    AtCmdSendResponse(pairindex, 1);
    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
}

#if CENTRAL_MODE
/**
* @brief   Send the reponse data of AT+INQ\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseInqStart()
{
    AtCmdSendResponseOK();
    AtCmdSendResponse(AtCmdResponseINQS, strlen(AtCmdResponseINQS));
}

/**
* @brief   Send the reponse data of AT+CONN<Param>\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseConnStart()
{
    AtCmdSendResponseOK();
    AtCmdSendResponse(AtCmdResponseCONNS, strlen(AtCmdResponseCONNS));
}

/**
* @brief   Send the reponse data of AT+SINQ\r\n.
* @param   void
* @return  void
*/
void AtCmdSendResponseInqEnd()
{
    AtCmdSendResponse(AtCmdResponseINQE, strlen(AtCmdResponseINQE));
}
#endif
/* ******************************************************************************************** */

/* **************************************** AtCmdHandle *************************************** */
/**
* @brief   Handle the Reset cmd.
* @param   void
* @return  void
*/
void AtCmdHandleReset(void)
{
    if (IO_Receive.atcmdlength == AT_CMD_RESET_LENGTH)
    {
        os_timer_start(&TimersReset);
        AtCmdSendResponseOK();
    }
}

/**
* @brief   Handle AT+BAUD cmd.
* @param   void
* @return  void
*/
void AtCmdHandleBaud(void)
{
    if (IO_Receive.atcmdlength == AT_CMD_BAUD_LENGTH)                 /*  AT CMD: AT+BAUD */
    {
        AtCmdSendResponseBaud();
    }
    else      /*  AT CMD: AT+BAUD<Param>  */
    {
        IO_Receive.atcmd[IO_Receive.atcmdlength] = '\0';
        if (IO_Receive.atcmd[7] == '=')
        {
            uint32_t newbaud = DataTrans_Atoi((const char *)(IO_Receive.atcmd + 8));
            APP_PRINT_INFO1("newbaud is %d", newbaud);
			printf("ATCMD set baud, newbaud is %d\n", newbaud); 
            if (/*(newbaud == 2000000) ||*/(newbaud == 921600) || (newbaud == 2400) || (newbaud == 4800) ||
                                           (newbaud == 9600) ||
                                           (newbaud == 19200) || (newbaud == 38400) || (newbaud == 57600) || (newbaud == 115200))
            {
                if (transferConfigInfo.baudrate != newbaud)
                {
                    transferConfigInfo.baudrate = newbaud;
                    os_timer_start(&TimersUartConfigChange);
                }
                AtCmdSendResponseBaudConfig();
            }
        }
    }
}

/**
* @brief   Handle AT+DEFAULT cmd.
* @param   void
* @return  void
*/
void AtCmdHandleDefault(void)
{
    datatrans_setdefault();
    os_timer_start(&TimersReset);
    AtCmdSendResponseOK();
}

/**
* @brief   Handle AT+NAME cmd.
* @param   void
* @return  void
*/
void AtCmdHandleName(void)
{

    if (IO_Receive.atcmdlength == AT_CMD_NAME_LENGTH)     /*  AT CMD: AT+NAME */
    {
        AtCmdSendResponseName();
    }
    else                                                  /*  AT CMD: AT+NAME<Param>  */
    {
        uint16_t len;
        if (IO_Receive.atcmd[7] == '=')
        {
            len =  IO_Receive.atcmdlength - (AT_CMD_NAME_LENGTH + 1);
            if (len <= DEVICE_NAME_MAX_LENGTH)
            {
                Device_Name_Write(IO_Receive.atcmd + 8, len);
                //const char *rename = (const char *)(name);
                AtCmdSendResponse(AtCmdResponseNAME, strlen(AtCmdResponseNAME));
                AtCmdSendResponse((const char *)dataTransInfo.devicename_info.device_name,
                                  dataTransInfo.devicename_info.length);
                AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
                AtCmdSendResponseOK();
            }
        }
    }
}

/**
* @brief   Handle AT+ROLE cmd.
* @param   void
* @return  void
*/
void AtCmdHandleRole(void)
{
    if (IO_Receive.atcmdlength == AT_CMD_ROLE_LENGTH)                 /*  AT CMD: AT+ROLE */
    {
        AtCmdSendResponseRole();
    }
    else if (IO_Receive.atcmdlength == AT_CMD_ROLE_LENGTH + 2)        /*  AT CMD: AT+ROLE<Param>  */
    {
        if (IO_Receive.atcmd[7] == '=')
        {
            uint16_t gaproleindex = IO_Receive.atcmd[8] - '0';

            if (gaproleindex == 0 || gaproleindex == 0x1 || gaproleindex == 0x2)
            {
                Role_Set(gaproleindex);
                AtCmdSendResponseRoleConfig();
            }
        }
    }
}

/**
* @brief   Handle AT+PAIR cmd.
* @param   void
* @return  void
*/
void AtCmdHandlePair(void)
{
    if (IO_Receive.atcmdlength == AT_CMD_PAIR_LENGTH)   /*  AT CMD: AT+PAIR  */
    {
        AtCmdSendResponsePair();
    }
    else if (IO_Receive.atcmdlength == AT_CMD_PAIR_LENGTH + 2)
    {
        if (IO_Receive.atcmd[7] == '=')
        {
            uint8_t newpair = IO_Receive.atcmd[8] - '0';
            if (newpair <= 3)
            {
                if (newpair != dataTransInfo.pair_info.pair_mode)
                {
                    dataTransInfo.pair_info.pair_mode = newpair;
                    Pair_Mode_Set(newpair);
                    AtCmdSendResponse(AtCmdResponsePAIR, strlen(AtCmdResponsePAIR));
                    AtCmdSendResponse((const char *)(IO_Receive.atcmd + 8),
                                      IO_Receive.atcmdlength - AT_CMD_PAIR_LENGTH - 1);
                    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
                    AtCmdSendResponseOK();
                }
            }
        }
    }
}

/**
* @brief   Handle AT+ADVMODE cmd.
* @param   void
* @return  void
*/
void AtCmdHandleAdvMode(void)
{
    if (IO_Receive.atcmdlength == AT_CMD_ADVMOD_LENGTH)
    {
        char index = (char)(dataTransInfo.device_mode.adv_mode + '0');
        const char *advmodeindex = (const char *)(&index);
        AtCmdSendResponse(AtCmdResponseADVMOD, strlen(AtCmdResponseADVMOD));
        AtCmdSendResponse(advmodeindex, 1);
        AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
    }
    else if (IO_Receive.atcmdlength == AT_CMD_ADVMOD_LENGTH + 2)
    {
        if (IO_Receive.atcmd[9] == '=')
        {
            uint8_t advmode = IO_Receive.atcmd[10] - '0';
            if (advmode <= 1)
            {
                if (dataTransInfo.device_mode.adv_mode != advmode)
                {
                    dataTransInfo.device_mode.adv_mode = advmode;
                    if ((dataTransInfo.device_mode.adv_mode == 1) &&
                        (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_IDLE) &&
                        (bt_datatrans_gap_conn_state != GAP_CONN_STATE_CONNECTED) &&
                        (dataTransInfo.device_mode.role == ROLE_PERIPHERAL))
                    {
                        le_adv_start();//adv mode =1, auto mode, adv mode =0 handle mode
                    }
                    else if ((dataTransInfo.device_mode.adv_mode == 0) &&
                             (bt_datatrans_gap_dev_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING) &&
                             (bt_datatrans_gap_conn_state != GAP_CONN_STATE_CONNECTED) &&
                             (dataTransInfo.device_mode.role == ROLE_PERIPHERAL))
                    {
                        le_adv_stop();
                    }
                    char index = (char)(dataTransInfo.device_mode.adv_mode + '0');
                    const char *advmodeindex = (const char *)(&index);
                    AtCmdSendResponse(AtCmdResponseADVMOD, strlen(AtCmdResponseADVMOD));
                    AtCmdSendResponse(advmodeindex, 1);
                    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
                    ftl_save(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4);
                }
                AtCmdSendResponseOK();
            }
        }
    }
}

/**
* @brief   Handle AT+ADVEN cmd.
* @param   void
* @return  void
*/
void AtCmdHandleAdvEN(void)
{
    if ((dataTransInfo.device_mode.adv_mode == 0) &&
        (dataTransInfo.device_mode.role == ROLE_PERIPHERAL))
    {
        if (IO_Receive.atcmdlength == AT_CMD_ADVEN_LENGTH + 2)
        {
            if (IO_Receive.atcmd[8] == '=')
            {
                uint8_t adven = IO_Receive.atcmd[9] - '0';
                if (adven <= 1)
                {
                    if (ADV_Enable_Disable(adven))
                    {
                        AtCmdSendResponse(AtCmdResponseADVEN, strlen(AtCmdResponseADVEN));
                        const char *adven = (const char *)(IO_Receive.atcmd + 9);
                        AtCmdSendResponse(adven, 1);
                        AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
                        AtCmdSendResponseOK();
                    }
                }
            }
        }
    }
}

/**
* @brief   Handle AT+VERSION cmd.
* @param   void
* @return  void
*/
void AtCmdHandleVersion(void)
{
    uint8_t ver[8];
    if (IO_Receive.atcmdlength == AT_CMD_VERSION_LENGTH)
    {
        uint8_t verstr[16];
        ver[3] = transferConfigInfo.app_version;
        ver[2] = transferConfigInfo.app_version >> 8;
        ver[1] = transferConfigInfo.app_version >> 16;
        ver[0] = transferConfigInfo.app_version >> 24;
        ver[7] = transferConfigInfo.patch_version;
        ver[6] = transferConfigInfo.patch_version >> 8;
        ver[5] = transferConfigInfo.patch_version >> 16;
        ver[4] = transferConfigInfo.patch_version >> 24;
        DataTrans_Hex2String(ver, verstr, 8);

        AtCmdSendResponse(AtCmdResponseVERSION, strlen(AtCmdResponseVERSION));
        AtCmdSendResponse((const char *)verstr, 8);
        AtCmdSendResponse(",", 1);
        AtCmdSendResponse((const char *)verstr + 8, 8);
        AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
    }
}

/**
* @brief   Handle AT+DISC cmd.
* @param   void
* @return  void
*/
void AtCmdHandleDISC(void)
{
	if (bt_datatrans_gap_conn_state == GAP_CONN_STATE_CONNECTED)
	{
		if (IO_Receive.atcmdlength == AT_CMD_DISC_LENGTH + 2)
		{
			uint8_t conn_id;
			if (IO_Receive.atcmd[7] == '=')
			{
				conn_id = IO_Receive.atcmd[8] - '0';
				if(le_disconnect(conn_id) == GAP_CAUSE_SUCCESS)
				{
					transferConfigInfo.select_mode = DATATRANS_MODE;
					AtCmdSendResponseDISC();
				}
				
			}
		}	
	}
}

#if CENTRAL_MODE
/**
* @brief   Handle AT+INQ cmd.
* @param   void
* @return  void
*/
void AtCmdHandleInq(void)
{
    if (IO_Receive.atcmdlength == AT_CMD_INQ_LENGTH)  /*  AT CMD: AT+INQ*/
    {
        if (dataTransInfo.device_mode.role == ROLE_CENTRAL &&
            bt_datatrans_gap_dev_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            DataTrans_Multilink_ClearDeviceList();
            le_scan_start();
            AtCmdSendResponseInqStart();
        }

    }
}

/**
* @brief   Handle AT+SINQ cmd.
* @param   void
* @return  void
*/
void AtCmdHandleSInq(void)
{
    if (IO_Receive.atcmdlength == AT_CMD_SINQ_LENGTH)  /*  AT CMD: AT+SINQ */
    {
        if (dataTransInfo.device_mode.role == ROLE_CENTRAL &&
            bt_datatrans_gap_dev_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            le_scan_stop();
            AtCmdSendResponseInqEnd();
        }
    }
}

/**
* @brief   Handle AT+SCANRLT cmd.
* @param   void
* @return  void
*/
void AtCmdHandleScanResult(void)
{
    if (dataTransInfo.device_mode.role == ROLE_CENTRAL)
    {
        if (IO_Receive.atcmdlength == AT_CMD_SCANRLT_LENGTH + 2)
        {
            if (IO_Receive.atcmd[10] == '=')
            {
                uint8_t devicenum = IO_Receive.atcmd[11] - '0';
                if (devicenum < BT_DATATRANS_APP_MAX_DEVICE_INFO)
                {
                    char addstr[15];
                    addstr[0] = devicenum + '0';
                    addstr[1] = ':';

                    sprintf(addstr + 2, "%02X%02X%02X%02X%02X%02X", DT_DevList[devicenum].bd_addr[5],
                            DT_DevList[devicenum].bd_addr[4], \
                            DT_DevList[devicenum].bd_addr[3], DT_DevList[devicenum].bd_addr[2],
                            DT_DevList[devicenum].bd_addr[1], DT_DevList[devicenum].bd_addr[0]);

                    AtCmdSendResponse(AtCmdResponseScanRlt, strlen(AtCmdResponseScanRlt));
                    AtCmdSendResponse((const char *)addstr, 14);
                    AtCmdSendResponse((const char *)("\r\n"), strlen("\r\n"));
                }
            }
        }
    }
}

/**
* @brief   Handle AT+CONN cmd.
* @param   void
* @return  void
*/
void AtCmdHandleConn(void)
{
    if (dataTransInfo.device_mode.role != ROLE_CENTRAL)
    {
        return;
    }
    if (IO_Receive.atcmdlength == AT_CMD_CONN_LENGTH + 1)
    {
        uint8_t  connindex;
        if (IO_Receive.atcmd[7] == '=')
        {
            connindex = IO_Receive.atcmd[8] - '0';
            APP_PRINT_INFO2("connindex is %x,DTDevListNum is %x", connindex, DTDevListNum);
            if (connindex < DTDevListNum)
            {
                AtCmdSendResponseConnStart();
                Connect_Device_Num(connindex);
            }
        }
    }
    else
    {
        AtCmdSendResponseERR();
    }
}
#endif

/* ************************************************************************************ */

/**
* @brief   Check if the string is at cmd.
* @param   at_cmd, the at cmd
* @param   pos, index of the cmd buffer received from UART.
* @return  true, is at cmd. false, isn't at cmd.
*/
bool IsAtCmd(char *at_cmd, uint16_t pos)
{
    bool res = true;

    while (*at_cmd != '\0')
    {
        if ((IO_Receive.atcmd[pos] != *at_cmd) && (IO_Receive.atcmd[pos] != (*at_cmd) + 32))
        {
            res = false;
            break;
        }
        pos++;
        at_cmd++;
    }

    return res;
}

/**
* @brief   AT cmd parse.
* @param   void
* @return  void
*/
void AtCmdParse(void)
{
    uint16_t pos = 0;
    APP_PRINT_INFO0("ENTER cmd parse 000");
    if (IO_Receive.atcmdlength < AT_CMD_HEADER_LENGTH)
    {
        return;
    }

    if (((IO_Receive.atcmd[pos] != 'A') && (IO_Receive.atcmd[pos] != 'a')) ||
        ((IO_Receive.atcmd[pos + 1] != 'T') && (IO_Receive.atcmd[pos + 1] != 't')))
    {
        return;
    }

    if ((IO_Receive.atcmd[IO_Receive.atcmdlength - 2] != '\r') ||
        (IO_Receive.atcmd[IO_Receive.atcmdlength - 1] != '\n'))
    {
        return;
    }

    pos += AT_CMD_HEADER_LENGTH;

    if ((IO_Receive.atcmd[pos] == '\r') && (IO_Receive.atcmd[pos + 1] == '\n') &&
        (IO_Receive.atcmdlength == AT_CMD_AT_LENGTH))
    {
        AtCmdSendResponseOK();  /*  AT cmd: AT  */
    }
    else if (IO_Receive.atcmd[pos] == '+')
    {
        APP_PRINT_INFO0("ENTER cmd parse 111");
        pos++;

        if (IsAtCmd("RESET", pos) && (IO_Receive.atcmdlength == AT_CMD_RESET_LENGTH))//
        {
            AtCmdHandleReset();
        }
        else if (IsAtCmd("VERSION", pos))//1
        {
            AtCmdHandleVersion();
        }
        else if (IsAtCmd("NAME", pos))
        {
            AtCmdHandleName();
        }
        else if (IsAtCmd("BAUD", pos))
        {
            AtCmdHandleBaud();
        }
        else if (IsAtCmd("ADVMOD", pos))
        {
            AtCmdHandleAdvMode();
        }
        else if (IsAtCmd("ADVEN", pos))
        {
            AtCmdHandleAdvEN();
        }
        else if (IsAtCmd("ROLE", pos))
        {
            AtCmdHandleRole();
        }
        else if (IsAtCmd("PAIR", pos))//1
        {
            AtCmdHandlePair();
        }
        else if (IsAtCmd("DEFAULT", pos)) //1
        {
            AtCmdHandleDefault();
        }
#if CENTRAL_MODE
        else if (IsAtCmd("INQ", pos))//start inq device
        {
            AtCmdHandleInq();
        }
        else if (IsAtCmd("SINQ", pos))//stop inq
        {
            AtCmdHandleSInq();
        }
        else if (IsAtCmd("CONN", pos))
        {
            AtCmdHandleConn();
        }
        else if (IsAtCmd("SCANRLT", pos))//1
        {
            AtCmdHandleScanResult();
        }
#endif
		else if(IsAtCmd("DISC", pos))
		{
			AtCmdHandleDISC();
		}
    }
}

#endif
