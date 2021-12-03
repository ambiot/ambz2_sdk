/**
*********************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      btltp_FreeRTOS.c
* @brief     aci iterface implementation.
* @details   none.
* @author    Tifnan
* @date      2014-10-17
* @version   v0.1
* *********************************************************************************************************
*/
#include <os_msg.h>
#include <os_mem.h>
#include <os_task.h>
#include <os_sync.h>
#include <os_sched.h>
#include <trace_app.h>
#include <hrp_application.h>
#include <hrp_btif_entry.h>
#include "wdt_reset.h"
#include <btif.h>
#include <hrp_gap_le.h>
#include <app_msg.h>
#include <btltp_uart.h>
#include "gap.h"



/* task  */
#define LTP_PRIORITY                    (0 + 2)   /* Task priorities.  tskIDLE_PRIORITY + 2*/
#define LTP_TASK_STACK_SIZE             0x800
#define TX_TASK_STACK_SIZE              0x200

void *hEventQueueHandle;
void *hrp_hTimerQueueHandle;
void *hIoQueueHandle;

P_BT_HRP  P_BtHrp = NULL;

TAciConfig AciConfig;
PAciConfig P_AciConfig = &AciConfig;

T_HRP_STATUS     system_status = HRP_STATUS_RESET;
HRP_MODULE_ID    active_module = HRP_MODULE_RESERVED;

extern uint8_t btif_get_up_data_len(T_BTIF_UP_MSG *p_msg);

void *LTPTaskHandle = NULL;
void *LTPTxAssistHandle = NULL;
void hrp_task(void *pParameters);

/**
 * @brief send event to ltp task.
 *
 * @param pEvent, pointer to the event to be sent.
 * @return send result.
 * @retval true--send successfully.
 *         false-- queue is full.
*/
bool hrp_send_event(const unsigned char *pEvent)
{
    bool ReturnValue;

    ReturnValue = os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleEvent, (void *)pEvent, 0);
    return (ReturnValue);
}

/**
 * @brief callback function, upper stack will call it to send message to ltp.
 *
 * @param pMsg --message pointer from upper stack.
 * @return none.
 * @retal void
*/
/*  //move to individual module
void hrp_btif_callback(T_BTIF_UP_MSG *pMsg)
{
    unsigned char Event = LTP_EVENT_BTIF_MESSAGE;

    if (os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleMessage, &pMsg, 0) == false)
    {
        btif_buffer_put(pMsg);
    }
    else
    {
        hrp_send_event(&Event);
    }
}
*/

/**
 * @brief call this fucntion will start to send data through ltp.
 * @param p_buf pointer to the buffer start address.
 * @param buf_len the length of the buffer.
 * @return none.
 * @retal   void.
*/
void hrp_write(uint8_t *p_buf, uint32_t buf_len)
{
    T_HRP_DATA tx_data;
    unsigned char ltpEvent  = LTP_EVENT_UART_TX;
    tx_data.pBuffer       = p_buf;
    tx_data.Length        = buf_len;

    //APP_PRINT_INFO2("hrp_write: value_size %d, value %b",
    //tx_data.Length, TRACE_BINARY(tx_data.Length, tx_data.pBuffer));
    if (os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleTxData, &tx_data, 0xffffffff) == false)
    {
        APP_PRINT_ERROR0("hrp_write: No queue");
    }
    else
    {
        hrp_send_event(&ltpEvent);
    }

    return;
}


/**
 * @brief release ltp buffer when ltp command has executed completely.
 *
 * @param p_buf the buffer start address to release, not used now!!
 * @return none.
 * @retal   void.
*/
void hrp_buffer_release(void *p_buf)
{
    T_HRP_DATA RxData;

    if (os_msg_recv(P_BtHrp->p_aci_tcb->QueueHandleRxData, &RxData, 0) == true)
    {
        if (RxData.pBuffer == &P_BtHrp->p_aci_tcb->p_rx_buf[P_BtHrp->p_aci_tcb->RxReadIndex])
        {
            uint32_t RxDataLength;
            uint32_t s;

            s = os_lock();
            P_BtHrp->p_aci_tcb->RxDataLength -= RxData.Length;
            RxDataLength = P_BtHrp->p_aci_tcb->RxDataLength;
            os_unlock(s);

            P_BtHrp->p_aci_tcb->RxReadIndex += RxData.Length;
            P_BtHrp->p_aci_tcb->RxReadIndex &= (RX_BUFFER_SIZE - 1);

            if (P_BtHrp->p_aci_tcb->RxDataIndication)   /* waiting for response */
            {
                P_BtHrp->p_aci_tcb->RxDataIndication -= RxData.Length;
            }

            if (P_BtHrp->p_aci_tcb->RxDataIndication == 0 &&   /* no response pending and */
                RxDataLength != 0)                         /* still data available */
            {
                uint8_t event = LTP_EVENT_UART_RX;
                if (false == hrp_send_event(&event))
                {
                    if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
                    {
                        APP_PRINT_ERROR0("hrp_send_event fail");
                    }
                }
            }

        }
        else
        {
            if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
            {
                APP_PRINT_ERROR0("hrp_buffer_release: Wrong buffer");
            }
        }
    }
    else
    {
        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_INFO)
        {
            APP_PRINT_INFO0("hrp_buffer_release: No RxData");
        }
    }

    return;
}

void aci_config_default(void)
{
    P_AciConfig->ltp_trace_level = LTP_TRACE_ALL;
}

/**
 * @brief init ltp module, call this function before calling other ltp functions .
 *
 * @param none.
 * @return the init result.
 * @retal  0 -- init ltp failed.
            1 -- init ltp successfully.
*/

// 1. init uart
// 2. init trace
// 3. init hrp related  struction
// all stack related init please move to individual modules ,
// for example when  the state is idle and received the first command

uint8_t hrp_init(void)
{
#if LTP_TEST_LOG_CLOSE
    for (uint8_t i = 0; i < MODULE_NUM; i++)
    {
        log_module_trace_set((T_MODULE_ID)i, LEVEL_TRACE, false);
        log_module_trace_set((T_MODULE_ID)i, LEVEL_INFO, false);
    }
    log_module_trace_set(MODULE_SNOOP, LEVEL_INFO, false);
    log_module_trace_set(MODULE_SNOOP, LEVEL_TRACE, false);
    log_module_trace_set(MODULE_SNOOP, LEVEL_ERROR, false);
    log_module_trace_set(MODULE_SNOOP, LEVEL_WARN, false);
#endif

    aci_config_default();
//    APP_PRINT_TRACE2("memory before alloc DATA ON:%d  DATA OFF:%d",
//                     os_mem_peek(RAM_TYPE_DATA_ON), os_mem_peek(RAM_TYPE_DATA_ON));
    APP_PRINT_TRACE2("memory before alloc DATA ON:%d  DATA OFF:%d",
                     os_mem_peek(RAM_TYPE_DATA_ON), os_mem_peek(RAM_TYPE_DATA_ON));

    P_BtHrp = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_BT_HRP));//&BtLtp;
    P_BtHrp->p_aci_tcb = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(ACI_TCB));// & AciTcb;

    P_BtHrp->p_aci_tcb->p_rx_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, RX_BUFFER_SIZE);
    P_BtHrp->p_aci_tcb->p_tx_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, TX_BUFFER_SIZE1);
    P_BtHrp->p_aci_tcb->p_rx_handle_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, RX_HANDLE_BUFFER_SIZE);

    APP_PRINT_TRACE2("memory after alloc DATA ON:%d  DATA OFF:%d",
                     os_mem_peek(RAM_TYPE_DATA_ON), os_mem_peek(RAM_TYPE_DATA_ON));

    /* allocate failed */
    if (NULL == P_BtHrp
        || NULL == P_BtHrp->p_aci_tcb
        || NULL == P_BtHrp->p_aci_tcb->p_rx_buf
        || NULL == P_BtHrp->p_aci_tcb->p_tx_buf
        || NULL == P_BtHrp->p_aci_tcb->p_rx_handle_buf)
    {
        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
        {
            APP_PRINT_ERROR0("Ltp ram allocated failed!");
        }

        return 0;
    }

    if (P_AciConfig->ltp_trace_level >= LTP_TRACE_INFO)
    {
        APP_PRINT_INFO0("Ltp ram allocated successfully!");
    }

    memset(&(P_BtHrp->p_aci_tcb->tx_mem_tcb), 0, sizeof(TxMemTCB));
    P_BtHrp->p_aci_tcb->tx_mem_tcb.free_size = TX_BUFFER_SIZE1;

    /* tasks and queues */
    P_BtHrp->p_aci_tcb->Handle = LTPTaskHandle;
    P_BtHrp->p_aci_tcb->P_RxBuffer = &P_BtHrp->p_aci_tcb->p_rx_buf[0];
    os_msg_queue_create(&(P_BtHrp->p_aci_tcb->QueueHandleEvent), MAX_NUMBER_OF_RX_EVENT,
                        sizeof(unsigned char));
    os_msg_queue_create(&(P_BtHrp->p_aci_tcb->QueueHandleTxData), MAX_NUMBER_OF_TX_DATA,
                        sizeof(T_HRP_DATA));
    os_msg_queue_create(&(P_BtHrp->p_aci_tcb->QueueHandleRxData), MAX_NUMBER_OF_RX_DATA,
                        sizeof(T_HRP_DATA));
    os_msg_queue_create(&(P_BtHrp->p_aci_tcb->QueueHandleTxRel), MAX_NUMBER_OF_TX_REL,
                        sizeof(T_HRP_DATA)); /* tx release */
    os_msg_queue_create(&(P_BtHrp->p_aci_tcb->QueueHandleMessage), MAX_NUMBER_OF_MESSAGE,
                        sizeof(T_BTIF_UP_MSG *));

    os_msg_queue_create(&hIoQueueHandle, BLT_MAX_NUMBER_OF_IO_EVENT, sizeof(T_IO_MSG));

    os_msg_queue_create(&(P_BtHrp->QueueHandleProfileMessage), MAX_NUMBER_OF_PROFILE_MSG,
                        sizeof(TProfileEvent));

    os_sem_create(&(P_BtHrp->p_aci_tcb->UarthandleEvent), 1, 1);

//move to individual   modle when received reset command
    /*   le_key_init();

       legacy_key_init();
    */

    return 1;
}


/**
  * @brief  Callback function should be register to upper stack to send message to application.
  * @param[in]  pMsg message sent from upper stack.
  * @retval None
  */
/*
void aci_btif_callback(T_BTIF_UP_MSG *pMsg)
{
  unsigned char event = LTP_EVENT_BTIF_MESSAGE;

  if (os_msg_send(hMessageQueueHandle, &pMsg, 0) == false)
  {
      btif_buffer_put(pMsg);
  }
  else if (os_msg_send(hEventQueueHandle, &event, 0) == false)
  {

  }
}
*/
uint8_t hrp_task_init(void)
{
    uint8_t ret = false;
    ret = os_task_create(&LTPTaskHandle, "BTLTP", hrp_task, NULL, LTP_TASK_STACK_SIZE, LTP_PRIORITY);

    if (ret == false)
    {
        APP_PRINT_ERROR0("BTlTP task create fail: ");
    }
#ifndef PLATFORM_STO
    ret = os_task_create(&LTPTxAssistHandle, "TxAssist", TxAssistTask, NULL, TX_TASK_STACK_SIZE,
                         LTP_PRIORITY - 1);

    if (ret == false)
    {
        APP_PRINT_ERROR0("BTlTP task create fail: ");
    }
#endif

    return 1;
}

/**
 * @brief ltp task implementation .
 *
 * @param pParameters --task parameters, no used in ltp task.
 * @return none.
 * @retal void
*/
void hrp_task(void *p_parameters)
{
    int  loop;
    char event;

    /*step1: init uart or spi */
    ltpPeripheralInit();
    P_BtHrp->State = btltpStateInit;

    for (loop = 0; loop < BTHRP_QUEUE_ELEMENT_COUNT; loop++)
    {
        hrp_queue_in(&P_BtHrp->FreeElementQueue, &P_BtHrp->ElementPool[loop]);
    }

    for (loop = 0; loop < BTHRP_ACTION_POOL_SIZE; loop++)
    {
        P_BtHrp->ActionPool[loop].Action = btltpActionNotUsed;
    }
    P_BtHrp->pBufferAction = NULL; /* no action pending */
    hrp_lib_initialize(&P_BtHrp->HRPLib,
                       (HRP_TGT_APPHANDLE)P_BtHrp,
                       0,
                       BTLTP_MAX_MSG_SIZE,
                       0
                      );

    /*step2:  regist callback to stack*/
    /*  //move to individual modules
    #ifdef TEST_PROFILE
    gap_start_bt_stack(aci_btif_callback);
    #else
    btif_register_req(aci_gap_btif_callback);
    #endif
    */
    P_BtHrp->State = btltpStateIdle; /* not use !!*/

    APP_PRINT_TRACE0("ltp_task start ");

    /*step3:  process events*/
    while (true)
    {
        if (os_msg_recv(P_BtHrp->p_aci_tcb->QueueHandleEvent, &event, 0xFFFFFFFF) == true)
        {
            switch (event)
            {
            case LTP_EVENT_UART_RX:          /* RxData available */
                {
                    uint32_t RxDataLength;
                    uint16_t RxReadIndex;
                    uint32_t s;

                    s = os_lock();
                    /* skip data filed in handling */
                    RxDataLength = P_BtHrp->p_aci_tcb->RxDataLength - P_BtHrp->p_aci_tcb->RxDataIndication;
                    os_unlock(s);
                    RxReadIndex  = P_BtHrp->p_aci_tcb->RxReadIndex + P_BtHrp->p_aci_tcb->RxDataIndication;
                    RxReadIndex &= (RX_BUFFER_SIZE - 1);

                    while (RxDataLength)
                    {
                        T_HRP_DATA RxData;

                        /* exceed rx buffer tail */
                        if ((RxReadIndex + RxDataLength) > RX_BUFFER_SIZE)
                        {
                            RxData.Length = RX_BUFFER_SIZE - RxReadIndex;
                        }
                        else
                        {
                            RxData.Length = RxDataLength;
                        }
                        RxData.pBuffer = &P_BtHrp->p_aci_tcb->p_rx_buf[RxReadIndex];

                        if (os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleRxData, &RxData, 0) == true)
                        {
                            P_BtHrp->p_aci_tcb->RxDataIndication += RxData.Length;
                            /* hrp_lib_handle_receive_data return when all data in RxData has been copied */
                            if (!hrp_lib_handle_receive_data(&P_BtHrp->HRPLib, RxData.pBuffer, RxData.Length, 0))
                            {
                                hrp_buffer_release(RxData.pBuffer);
                            }

                            RxDataLength -= RxData.Length;
                            RxReadIndex  += RxData.Length;
                            RxReadIndex  &= (RX_BUFFER_SIZE - 1);
                        }
                        else
                        {
                            break;
                        }
                    }
                    break;
                }

            case LTP_EVENT_UART_TX:                    /* transmit data */
                ltpStartTransmit();
                break;

            case LTP_EVENT_UART_TX_COMPLETED:          /* transmit completed */
                {
                    T_HRP_DATA data;

                    if (os_msg_recv(P_BtHrp->p_aci_tcb->QueueHandleTxRel, &data, 0) == true)
                    {
                        P_BtHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx += data.Length;
                        P_BtHrp->p_aci_tcb->tx_mem_tcb.free_size += data.Length;

                        if (P_BtHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx > TX_BUFFER_SIZE1)
                        {
                            P_BtHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx = data.Length;
                            P_BtHrp->p_aci_tcb->tx_mem_tcb.tx_un_used_size = 0;
                        }
                        else if (P_BtHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx == TX_BUFFER_SIZE1)
                        {
                            P_BtHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx = 0;
                            P_BtHrp->p_aci_tcb->tx_mem_tcb.tx_un_used_size = 0;
                        }
                    }
                    else
                    {
                        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_INFO)
                        {
                            APP_PRINT_ERROR0("QueueHandleTxRel recieve fail: ");
                        }
                    }

                    if (P_BtHrp->pBufferAction)
                    {
                        if (P_BtHrp->pBufferAction->Action >= btltpActionReset &&
                            P_BtHrp->pBufferAction->Action <  btltpActionReleaseBuffer)
                        {
                            void *Handle = (void *)P_BtHrp->pBufferAction;
                            bt_hrp_buffer_callback(Handle);
                            P_BtHrp->pBufferAction->Action = btltpActionNotUsed;
                            P_BtHrp->pBufferAction = NULL;
                        }
                    }

                    ltpStartTransmit();
                    break;
                }

            case LTP_EVENT_BTIF_MESSAGE:            /* BTIF */
                {
                    T_BTIF_UP_MSG *pMsg;

                    while (os_msg_recv(P_BtHrp->p_aci_tcb->QueueHandleMessage, &pMsg, 0) == true)
                    {
#if F_BT_LE_BTIF_SUPPORT
                        hrp_btif_handle_msg(pMsg);
#endif
                    }
                    break;
                }
#ifdef TEST_PROFILE
            case LTP_EVENT_PROFILE_MESSAGE:     /* spp tx innner data event */
                {
                    TProfileEvent profile_event;
                    while (os_msg_recv(P_BtHrp->QueueHandleProfileMessage, &profile_event, 0) == true)
                    {
                        profiles_handle_profile_message(profile_event);
                    }
                    break;
                }
#endif
            case EVENT_GAP_MSG:
            case EVENT_GAP_TIMER:
                gap_handle_msg(event);
                break;
            case EVENT_IO_TO_APP:
                {
                    T_IO_MSG io_msg;
                    if (os_msg_recv(hIoQueueHandle, &io_msg, 0) == true)
                    {
                        hrp_gap_le_handle_io_msg(io_msg);
                    }
                }
                break;

            default:
                if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
                {
                    APP_PRINT_ERROR1("hrp_task: Unknown event (%d)", event);
                }
                break;
            }
        }
        else
        {
            if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
            {
                APP_PRINT_ERROR0("hrp_task: os_msg_recv fail");
            }
        }
    }
}



void hrp_system_reset()
{
    APP_PRINT_INFO1("system_status= %d", system_status);
    if (system_status == HRP_STATUS_RESET)
    {
        return;
    }
    else
    {
        APP_PRINT_INFO0("HRP: reset (wait for WD to kick in)");

        /* wait for last char of ResetRsp (buffercallback is executed on txempty, NOT on txcomplete) */
        os_delay(20);   /* 20 ms delay */
        wdt_reset();
    }
}
