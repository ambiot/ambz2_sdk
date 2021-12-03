#include "hrp.h"
#include "trace_app.h"
#include "os_msg.h"
#include "btltp_uart.h"
#include "serial_api.h"
#include "serial_ex_api.h"
#include "timer_api.h"
#if 0
#include "ameba_soc.h"
#endif
#include "basic_types.h"
#if F_BT_DLPS_EN
#include "hrp_dlps.h"
#endif
static uint32_t hrp_rx_idle = 1;
#define UART_IRQ    12
uint8_t RxTriggerLevel = 14;
#if 0
UART_TypeDef *data_uart_def;
#endif

static void *bb3_UarthandleEvent;
#define DATA_UART_TX_PIN    PA_16
#define DATA_UART_RX_PIN    PA_15
static serial_t ltpuart_sobj;
//#define LTP_TIM_PRESCALER        40      /* 40 bits timeout */

//#define LTP_BAUDRATE             115200  /* baudrate */
gtimer_t my_timer3;
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
void LtpRxDataLengthUpdate(void);
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

static inline void uart_hrp_insert_char(uint8_t ch)
{
    /* Should neve happen */
    P_BtHrp->p_aci_tcb->P_RxBuffer[0] = ch;
    P_BtHrp->p_aci_tcb->P_RxBuffer ++;
    if (P_BtHrp->p_aci_tcb->P_RxBuffer - &P_BtHrp->p_aci_tcb->p_rx_buf[0]  >= RX_BUFFER_SIZE)
    {
        P_BtHrp->p_aci_tcb->P_RxBuffer = &P_BtHrp->p_aci_tcb->p_rx_buf[0];
    }

    LtpRxDataLengthUpdate();
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
        //     if (os_sem_take(bb3_UarthandleEvent, 0xFFFFFFFF) == true)
        // {

        if (os_msg_recv(P_BtHrp->p_aci_tcb->QueueHandleTxData, &data, 0xFFFFFFFF) == true)
        {
            pBuffer = data.pBuffer;
            blk_count = data.Length;
            // dbg_printf("TxAssistTask: send %d\r\n", data.Length);
            serial_send_blocked(&ltpuart_sobj, pBuffer, blk_count, 0xffffffff);

#if 0
            for (i = 0; i < blk_count; i++)
            {
                //while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
                //UART_SendData(data_uart_def, pBuffer, 16);
                pBuffer += 16;
            }

            //the ramain data
            blk_count = data.Length % 16;

            if (blk_count)
            {
                //while (UART_GetFlagState(UART, UART_FLAG_THR_EMPTY) != SET);
                // UART_SendData(data_uart_def, pBuffer, blk_count);

                serial_putc();
            }
#endif

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
#if 0
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
        //}
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
void  data_uart_irq(uint32_t id, SerialIrq event)
{
    uint8_t rx_char;
    uint8_t ltp_event  = LTP_EVENT_UART_RX;
    int max_count = 16;

    serial_t    *sobj = (void *)id;
    if (event == RxIrq)
    {
        /* For reading the remaining in rx fifo if time-out */
        gtimer_reload(&my_timer3, 2000);
        do
        {
            rx_char = serial_getc(sobj);
            uart_hrp_insert_char(rx_char);
        }
        while (serial_readable(sobj) && max_count-- > 0);

        if (hrp_rx_idle)
        {
            if (false == ltpSendEventMsg(&ltp_event))
            {
                APP_PRINT_ERROR1("ltpSendEventMsg fai %d", ltp_event);
            }
        }
        hrp_rx_idle = 0;
    }
#if 0
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
        HCI_PRINT_ERROR1("data_uart_irq: Unknown interrupt type %u", int_id);
        break;
    }
#endif
    //return 0;
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

#if 0
    if (P_BtHrp->p_aci_tcb->TxData.pBuffer == (uint8_t)0 &&
        xQueueReceive(P_BtHrp->p_aci_tcb->QueueHandleTxData, &P_BtHrp->p_aci_tcb->TxData, 0) == pdPASS)
    {
        P_BtHrp->p_aci_tcb->DMA_Tx_InitStructure.DMA_MemoryBaseAddr = (uint32_t)
                                                                      P_BtHrp->p_aci_tcb->TxData.pBuffer;
        P_BtHrp->p_aci_tcb->DMA_Tx_InitStructure.DMA_BufferSize     = P_BtHrp->p_aci_tcb->TxData.Length;

#if 0
        DMA_Init(DMA1_Channel4, &P_BtHrp->p_aci_tcb->DMA_Tx_InitStructure);

        DMA_ITConfig(DMA1_Channel4, (DMA_IT_TC | DMA_IT_TE), ENABLE);
        USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
        DMA_Cmd(DMA1_Channel4, ENABLE);

        GDMA_INTConfig(AciTxDMAChannelNum, GDMA_INT_Transfer, ENABLE);
        GDMA_SetSourceAddress(AciTxDMAChannel, (uint32_t)P_BtHrp->p_aci_tcb->TxData.pBuffer);
        GDMA_SetBufferSize(AciTxDMAChannel, P_BtHrp->p_aci_tcb->TxData.Length);
        GDMA_Cmd(AciTxDMAChannelNum, ENABLE);
#endif
        int ret;
        ret = serial_send_stream(&ltpuart_sobj, P_BtHrp->p_aci_tcb->TxData.pBuffer,
                                 P_BtHrp->p_aci_tcb->TxData.Length);
        if (ret != 0)
        {
            dbg_printf("%s Error(%d)\n", __FUNCTION__, ret);
        }
    }
#endif

}
void timer3_timeout_handler(uint32_t id)
{
    serial_t *sobj = (void *)id;
    uint16_t rx_len = 0;
    uint8_t ch;
    uint32_t s;
    uint8_t ltp_event  = LTP_EVENT_UART_RX;
    //  s=os_lock();

    while (serial_readable(sobj))
    {
        ch = serial_getc(sobj);
        uart_hrp_insert_char(ch);
        rx_len++;

    }

    /* If use hci_comuart.rx_data_len to check, the rx event queue will be full
     */
    if (rx_len > 0 || hrp_rx_idle == 0)
    {
        if (false == ltpSendEventMsg(&ltp_event))
        {
            APP_PRINT_ERROR1("ltpSendEventMsg fai %d", ltp_event);
        }
    }
    hrp_rx_idle = 1;

//  os_unlock(s);
}
void ltpPeripheralInit(void)
{

    hal_pinmux_unregister(DATA_UART_TX_PIN, 0x01 << 4);
    hal_pinmux_unregister(DATA_UART_RX_PIN, 0x01 << 4);
    hal_gpio_pull_ctrl(DATA_UART_TX_PIN, 0);
    hal_gpio_pull_ctrl(DATA_UART_RX_PIN, 0);
    serial_init(&ltpuart_sobj, DATA_UART_TX_PIN, DATA_UART_RX_PIN);
    serial_baud(&ltpuart_sobj, 115200);
    serial_format(&ltpuart_sobj, 8, ParityNone, 1);
    serial_irq_handler(&ltpuart_sobj, data_uart_irq, (uint32_t)&ltpuart_sobj);
    serial_irq_set(&ltpuart_sobj, RxIrq, 0);
    serial_irq_set(&ltpuart_sobj, RxIrq, 1);
    //serial_irq_set(&ltpuart_sobj, TxIrq, 1);
    serial_rx_fifo_level(&ltpuart_sobj, FifoLvHalf);
    //serial_irq_set(&ltpuart_sobj, TxIrq, 1);
    serial_clear_rx(&ltpuart_sobj);

    gtimer_init(&my_timer3, TIMER3);
    gtimer_start_periodical(&my_timer3, 2000, (void *)timer3_timeout_handler, (uint32_t)&ltpuart_sobj);

    // os_sem_create(&bb3_UarthandleEvent, 1, 1);

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
#if 0
    IRQn_Type irqn = UART0_IRQ;
    UART_InitTypeDef    UART_InitStruct;

    data_uart_def = UART0_DEV;

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
#endif
}

