#include "hrp.h"
#include "trace_app.h"
#include "os_msg.h"
#include "btltp_uart.h"
#include "ameba_soc.h"

#if F_BT_DLPS_EN
#include "hrp_dlps.h"
#endif

#define UART_IRQ    12
uint8_t RxTriggerLevel = 14;
UART_TypeDef *data_uart_def;

#define DATA_UART_TX_PIN    P2_4
#define DATA_UART_RX_PIN    P2_5
//#define LTP_TIM_PRESCALER        40      /* 40 bits timeout */

//#define LTP_BAUDRATE             115200  /* baudrate */

const unsigned char ltpEventUartRx          = LTP_EVENT_UART_RX;
const unsigned char ltpEventUartTx          = LTP_EVENT_UART_TX;
const unsigned char ltpEventUartTxCompleted = LTP_EVENT_UART_TX_COMPLETED;
/****************************************************************************/
/**
 * @brief send to ltp task to release tx buffer space.
 *
 * @param p_tx_buf, pointer to the tx bufer to be released.
 * @return .
*/
bool ltpSendTxBufReleaseMsg(T_HRP_DATA *p_tx_buf)
{
    bool ReturnValue;

    ReturnValue = os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleTxRel, p_tx_buf, 0);

    return (ReturnValue);
}

/**
 * @brief send event to ltp task.
 *
 * @param pEvent, pointer to the event to b sent.
 * @return send result.
 * @retval true--send successfully.
 *         false-- queue is full.
*/
bool ltpSendEventMsg(const unsigned char *pEvent)
{
    bool ReturnValue;

    ReturnValue = os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleEvent, (void *)pEvent, 0);
    return (ReturnValue);
}

/**
 * @brief tx task, tx bytes in polling mode.
 *
 * @param pParameters, not used.
 * @return void.
*/
void TxAssistTask(void *pParameters)
{
    uint32_t i = 0;
    uint8_t *pBuffer = NULL;
    uint8_t event = 0;
    uint16_t blk_count = 0;
    T_HRP_DATA data = {NULL, 0};

    while (true)
    {
        if (os_msg_recv(P_BtHrp->p_aci_tcb->QueueHandleTxData, &data, 0xFFFFFFFF) == true)
        {
            pBuffer = data.pBuffer;
            blk_count = data.Length / 16;

            for (i = 0; i < blk_count; i++)
            {
                //while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
                UART_SendData(data_uart_def, pBuffer, 16);
                pBuffer += 16;
            }

            //the ramain data
            blk_count = data.Length % 16;

            if (blk_count)
            {
                //while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
                UART_SendData(data_uart_def, pBuffer, blk_count);
            }

            if (false == ltpSendTxBufReleaseMsg(&data))
            {
                if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
                {
                    APP_PRINT_ERROR0("ltpSendTxBufReleaseMsg fail");
                }
            }
            event = LTP_EVENT_UART_TX_COMPLETED;
            if (false == ltpSendEventMsg(&event))
            {
                if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
                {
                    APP_PRINT_ERROR0("hrp_send_event(TX_COMPLETED) fail");
                }
            }
#if F_BT_DLPS_EN
            if (hrp_dlps_status == HRP_DLPS_STATUS_ACTIVED)
            {
                uint32_t msg_num;
                os_msg_queue_peek(P_BtHrp->p_aci_tcb->QueueHandleTxData, &msg_num);
                if (msg_num == 0)
                {
                    hrp_dlps_allow_enter(true); /* no data to be sent*/
                    //APP_PRINT_INFO0("TxAssistTask allow enter dlps");
                }
            }
#endif

#if ACI_LP_EN
            /* reset moniter timer */
            if (MoniterTimer)
            {
                xTimerReset(MoniterTimer, 5);
                MonitorTimeout = 0;
            }
#endif
        }
        else
        {
            if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
            {
                APP_PRINT_ERROR0("TxAssistTask: xQueueReceive fail");
            }
        }
    }

}

/**
 * @brief update rx data length when received data from uart or spi.
 *
 * @param none.
 * @return none.
 * @retval void.
*/
void LtpRxDataLengthUpdate(void)
{
    uint16_t RxOffset = 0;
    uint16_t Length = 0;

#ifdef PLATFORM_STO
    RxOffset = RX_BUFFER_SIZE - AciRxDMAChannel->CTL_HIGH ;
#endif

#if 1
    RxOffset = P_BtHrp->p_aci_tcb->P_RxBuffer -
               &P_BtHrp->p_aci_tcb->p_rx_buf[0]; /* tifnan: num of char received */
#endif
    /* will not occur in our uart framework!!! */
    if (P_BtHrp->p_aci_tcb->RxOffset == RxOffset)
    {
        if (P_BtHrp->p_aci_tcb->RxDataLength == RX_BUFFER_SIZE)  /* overrun */
        {
            if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
            {
                APP_PRINT_ERROR0("LtpRxDataLengthUpdate: Rx overrun");
            }

            P_BtHrp->p_aci_tcb->RxDataLength = 0;
            Length = RX_BUFFER_SIZE;
        }
        else
        {
            return;       /* no data */
        }
    }
    else
    {
        /* [p_aci_tcb->RxOffset----RxBufferSize-1] + [P_BtLtp->p_aci_tcb->p_rx_buf[0]----RxOffset] */
        if (P_BtHrp->p_aci_tcb->RxOffset > RxOffset)
        {
            Length = RX_BUFFER_SIZE - P_BtHrp->p_aci_tcb->RxOffset + RxOffset;
        }
        /* [p_aci_tcb->RxOffset ---- RxOffset] */
        else
        {
            Length = RxOffset - P_BtHrp->p_aci_tcb->RxOffset;
        }

        /* update new P_BtHrp->p_aci_tcb->RxOffset */
        P_BtHrp->p_aci_tcb->RxOffset = RxOffset;
    }

    if ((Length + P_BtHrp->p_aci_tcb->RxDataLength) > RX_BUFFER_SIZE)   /* Rx overrun */
    {
        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
        {
            APP_PRINT_ERROR1("LtpRxDataLengthUpdate: Rx overrun (%d)",
                             Length + P_BtHrp->p_aci_tcb->RxDataLength);
        }

        P_BtHrp->p_aci_tcb->RxDataLength  = RX_BUFFER_SIZE;
        P_BtHrp->p_aci_tcb->RxWriteIndex += Length;
        P_BtHrp->p_aci_tcb->RxWriteIndex &= (RX_BUFFER_SIZE - 1);
        P_BtHrp->p_aci_tcb->RxReadIndex   = P_BtHrp->p_aci_tcb->RxWriteIndex;
    }
    else
    {
        P_BtHrp->p_aci_tcb->RxDataLength += Length;         /* update length */
        P_BtHrp->p_aci_tcb->RxWriteIndex += Length;
        P_BtHrp->p_aci_tcb->RxWriteIndex &= (RX_BUFFER_SIZE - 1);
    }
}

/**
 * @brief uart interrupt handle ISR.
 *
 * @param none.
 * @return none.
 * @retval void.
*/
u32 data_uart_irq(void *data)
{
    volatile u8 reg_iir;
    u8 int_id;
    u32 reg_val;
    uint8_t event  = LTP_EVENT_UART_RX;
    reg_iir = UART_IntStatus(data_uart_def);
    if ((reg_iir & RUART_IIR_INT_PEND) != 0)
    {
        /* No pending IRQ */
        return 0;
    }

    int_id = (reg_iir & RUART_IIR_INT_ID) >> 1;

    switch (int_id)
    {
    case RUART_RECEIVER_DATA_AVAILABLE:
        if ((P_BtHrp->p_aci_tcb->P_RxBuffer - &P_BtHrp->p_aci_tcb->p_rx_buf[0] + RxTriggerLevel)\
            <= RX_BUFFER_SIZE)
        {
            UART_ReceiveData(data_uart_def, P_BtHrp->p_aci_tcb->P_RxBuffer, RxTriggerLevel);
            P_BtHrp->p_aci_tcb->P_RxBuffer += RxTriggerLevel;
        }
        else
        {
            uint16_t len = 0;
            len = RX_BUFFER_SIZE - (P_BtHrp->p_aci_tcb->P_RxBuffer - &P_BtHrp->p_aci_tcb->p_rx_buf[0]);
            UART_ReceiveData(data_uart_def, P_BtHrp->p_aci_tcb->P_RxBuffer, len);
            P_BtHrp->p_aci_tcb->P_RxBuffer = &P_BtHrp->p_aci_tcb->p_rx_buf[0];
            UART_ReceiveData(data_uart_def, P_BtHrp->p_aci_tcb->P_RxBuffer, RxTriggerLevel - len);
            P_BtHrp->p_aci_tcb->P_RxBuffer += (RxTriggerLevel - len);
        }

        /* update rx data length */
        LtpRxDataLengthUpdate();
        /* notify ltp task */
        if (false == ltpSendEventMsg(&event))
        {
            APP_PRINT_ERROR0("ltpSendEventMsg fail");
        }
        break;
    case RUART_TIME_OUT_INDICATION:
        /* read out all bytes in fifo */
        while (UART_Readable(data_uart_def))
        {
            if (P_BtHrp->p_aci_tcb->P_RxBuffer - &P_BtHrp->p_aci_tcb->p_rx_buf[0] == RX_BUFFER_SIZE)
            {
                P_BtHrp->p_aci_tcb->P_RxBuffer = &P_BtHrp->p_aci_tcb->p_rx_buf[0];
            }
            UART_CharGet(data_uart_def, P_BtHrp->p_aci_tcb->P_RxBuffer);
            P_BtHrp->p_aci_tcb->P_RxBuffer++;
        }

        /* update rx data length */
        LtpRxDataLengthUpdate();
        /* notify ltp task */
        if (false == ltpSendEventMsg(&event))
        {
            APP_PRINT_ERROR0("ltpSendEventMsg fai");
        }
        break;

    case RUART_RECEIVE_LINE_STATUS:
        reg_val = (UART_LineStatusGet(data_uart_def));
        APP_PRINT_INFO1("data_uart_irq: LSR %08x interrupt", reg_val);

        if (reg_val & RUART_LINE_STATUS_ERR_OVERRUN)
        {
            APP_PRINT_ERROR0("data_uart_irq: LSR over run interrupt");
        }

        if (reg_val & RUART_LINE_STATUS_ERR_PARITY)
        {
            APP_PRINT_ERROR0("data_uart_irq: LSR parity error interrupt");
        }

        if (reg_val & RUART_LINE_STATUS_ERR_FRAMING)
        {
            APP_PRINT_ERROR0("data_uart_irq: LSR frame error(stop bit error) interrupt");
        }

        if (reg_val & RUART_LINE_STATUS_ERR_BREAK)
        {
            APP_PRINT_ERROR0("data_uart_irq: LSR break error interrupt");
        }

        /* if (reg_val & RUART_LINE_STATUS_REG_THRE)
         *     transmit_chars(hci_adapter);
         */

        break;

    default:
        APP_PRINT_ERROR1("data_uart_irq: Unknown interrupt type %u", int_id);
        break;
    }

    return 0;
}
#if 0
void UART0_Handler(void)
{
    /* read interrupt status */
    uint32_t int_status;
    uint8_t event = LTP_EVENT_UART_RX;
    /* read interrupt id */
    int_status = UART_GetIID(UART);
    /* disable interrupt */
    UART_INTConfig(UART, UART_INT_RD_AVA | UART_INT_LINE_STS, DISABLE);

    switch (int_status)
    {
    /* tx fifo empty */
    case UART_INT_ID_TX_EMPTY:
        break;

    /* rx data valiable */
    case UART_INT_ID_RX_LEVEL_REACH:
        if ((P_BtHrp->p_aci_tcb->P_RxBuffer - &P_BtHrp->p_aci_tcb->p_rx_buf[0] + RxTriggerLevel)\
            <= RX_BUFFER_SIZE)
        {
            UART_ReceiveData(UART, P_BtHrp->p_aci_tcb->P_RxBuffer, RxTriggerLevel);
            P_BtHrp->p_aci_tcb->P_RxBuffer += RxTriggerLevel;
        }
        else
        {
            uint16_t len = 0;
            len = RX_BUFFER_SIZE - (P_BtHrp->p_aci_tcb->P_RxBuffer - &P_BtHrp->p_aci_tcb->p_rx_buf[0]);
            UART_ReceiveData(UART, P_BtHrp->p_aci_tcb->P_RxBuffer, len);
            P_BtHrp->p_aci_tcb->P_RxBuffer = &P_BtHrp->p_aci_tcb->p_rx_buf[0];
            UART_ReceiveData(UART, P_BtHrp->p_aci_tcb->P_RxBuffer, RxTriggerLevel - len);
            P_BtHrp->p_aci_tcb->P_RxBuffer += (RxTriggerLevel - len);
        }

        /* update rx data length */
        LtpRxDataLengthUpdate();
        /* notify ltp task */
        if (false == ltpSendEventMsg(&event))
        {
            APP_PRINT_ERROR0("ltpSendEventMsg fail");
        }
        break;

    /* rx time out */
    case UART_INT_ID_RX_TMEOUT:
        /* read out all bytes in fifo */
        while (UART_GetFlagState(UART, UART_FLAG_RX_DATA_RDY) == SET)
        {
            if (P_BtHrp->p_aci_tcb->P_RxBuffer - &P_BtHrp->p_aci_tcb->p_rx_buf[0] == RX_BUFFER_SIZE)
            {
                P_BtHrp->p_aci_tcb->P_RxBuffer = &P_BtHrp->p_aci_tcb->p_rx_buf[0];
            }
            UART_ReceiveData(UART, P_BtHrp->p_aci_tcb->P_RxBuffer, 1);
            P_BtHrp->p_aci_tcb->P_RxBuffer++;
        }

        /* update rx data length */
        LtpRxDataLengthUpdate();
        /* notify ltp task */
        if (false == ltpSendEventMsg(&event))
        {
            APP_PRINT_ERROR0("ltpSendEventMsg fai");
        }
        break;

    /* receive line status interrupt */
    case UART_INT_ID_LINE_STATUS:
        APP_PRINT_ERROR0("Line status error!");
        break;

    default:
        break;
    }

    /* enable interrupt again */
    //UART_INTConfig(UART, UART_INT_RD_AVA | UART_INT_LINE_STS, ENABLE);
    UART_INTConfig(UART, UART_INT_RD_AVA, ENABLE);
}
#endif

void ltpStartTransmit(void)
{
#ifdef PLATFORM_STO
    if (P_BtHrp->p_aci_tcb->TxData.pBuffer == (uint8_t)0 &&
        xQueueReceive(P_BtHrp->p_aci_tcb->QueueHandleTxData, &P_BtHrp->p_aci_tcb->TxData, 0) == pdPASS)
    {
        P_BtHrp->p_aci_tcb->DMA_Tx_InitStructure.DMA_MemoryBaseAddr = (uint32_t)
                                                                      P_BtHrp->p_aci_tcb->TxData.pBuffer;
        P_BtHrp->p_aci_tcb->DMA_Tx_InitStructure.DMA_BufferSize     = P_BtHrp->p_aci_tcb->TxData.Length;
        DMA_Init(DMA1_Channel4, &P_BtHrp->p_aci_tcb->DMA_Tx_InitStructure);

        DMA_ITConfig(DMA1_Channel4, (DMA_IT_TC | DMA_IT_TE), ENABLE);
        USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
        DMA_Cmd(DMA1_Channel4, ENABLE);

        GDMA_INTConfig(AciTxDMAChannelNum, GDMA_INT_Transfer, ENABLE);
        GDMA_SetSourceAddress(AciTxDMAChannel, (uint32_t)P_BtHrp->p_aci_tcb->TxData.pBuffer);
        GDMA_SetBufferSize(AciTxDMAChannel, P_BtHrp->p_aci_tcb->TxData.Length);
        GDMA_Cmd(AciTxDMAChannelNum, ENABLE);
    }
#endif
}
#define DATAUART_IRQ_PRIO      11
void ltpPeripheralInit(void)
{
#if 0
    //Config DATA UART0 pinmux
    RCC_PeriphClockCmd(APBPeriph_UART0, APBPeriph_UART0_CLOCK, ENABLE);
    Pinmux_Config(DATA_UART_TX_PIN, UART0_TX);
    Pinmux_Config(DATA_UART_RX_PIN, UART0_RX);
    Pad_Config(DATA_UART_TX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(DATA_UART_RX_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    /* uart init */
    UART_InitTypeDef uartInitStruct;
    UART_StructInit(&uartInitStruct);
    uartInitStruct.rxTriggerLevel = UART_RX_FIFO_TRIGGER_LEVEL_14BYTE;
    UART_Init(UART, &uartInitStruct);
    UART_INTConfig(UART, UART_INT_RD_AVA, ENABLE);

    /*  Enable UART IRQ  */
    NVIC_InitTypeDef nvic_init_struct;
    nvic_init_struct.NVIC_IRQChannel         = UART0_IRQn;
    nvic_init_struct.NVIC_IRQChannelCmd      = ENABLE;
    nvic_init_struct.NVIC_IRQChannelPriority = 5;
    NVIC_Init(&nvic_init_struct);
#endif
    IRQn_Type irqn = UART0_IRQ;
    UART_InitTypeDef    UART_InitStruct;

    data_uart_def = UART0_DEV;
//PIN TX:A_18 RX:PA 19
    Pinmux_Config(_PA_18, PINMUX_FUNCTION_UART);
    Pinmux_Config(_PA_19, PINMUX_FUNCTION_UART);

    PAD_PullCtrl(_PA_18, GPIO_PuPd_UP);
    PAD_PullCtrl(_PA_19, GPIO_PuPd_UP);
    //UART_PinMuxInit(0, S0);
    UART_StructInit(&UART_InitStruct);
    UART_InitStruct.WordLen = RUART_WLS_8BITS;
    UART_InitStruct.StopBit = RUART_STOP_BIT_1;
    UART_InitStruct.Parity = RUART_PARITY_DISABLE;
    UART_InitStruct.ParityType = RUART_EVEN_PARITY;
    UART_InitStruct.StickParity = RUART_STICK_PARITY_DISABLE;
    UART_InitStruct.RxFifoTrigLevel = UART_RX_FIFOTRIG_LEVEL_14BYTES;
    UART_InitStruct.FlowControl = FALSE;
    UART_Init(data_uart_def, &UART_InitStruct);
    UART_SetBaud(data_uart_def, 115200);

    InterruptDis(irqn);
    InterruptUnRegister(irqn);
    InterruptRegister((IRQ_FUN)data_uart_irq, irqn, NULL, DATAUART_IRQ_PRIO);
    InterruptEn(irqn, DATAUART_IRQ_PRIO);
    UART_INTConfig(data_uart_def, RUART_IER_ERBI | RUART_IER_ETOI | RUART_IER_ELSI, ENABLE);

    UART_RxCmd(data_uart_def, ENABLE);
}

