/**
 * Copyright (c) 2017, Realsil Semiconductor Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include "os_mem.h"
#include "hci_uart.h"
#include "trace_app.h"
#include "bt_board.h"
#include "os_sync.h"
#include "os_task.h"
#include "serial_api.h"
#include "serial_ex_api.h"

#define HCI_UART_TX_DMA    0

static serial_t hci_serial_obj;
extern uint8_t flag_for_hci_trx;

#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
void *uart_send_done_task = NULL;
void *uart_send_done_sem = NULL;

#define UART_SEND_DONE_TASK_PRIORITY             5
#define UART_SEND_DONE_TASK_STACK_SIZE           512

#define HCI_UART_TX_BUF_SIZE        512      /* TX buffer size 512 */
#endif

#define HCI_UART_RX_BUF_SIZE        0x2000   /* RX buffer size 8K */
#define HCI_UART_RX_ENABLE_COUNT    (HCI_UART_RX_BUF_SIZE - 2 * (1021 + 5))   /* Enable RX */
#define HCI_UART_RX_DISABLE_COUNT   (HCI_UART_RX_BUF_SIZE - 1021 - 5 - 10)   /* Disable RX */
typedef struct
{
//tx
    uint32_t            tx_len;
    P_UART_TX_CB        tx_cb;
#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
    uint8_t             tx_buffer[HCI_UART_TX_BUF_SIZE];
    void*               tx_buffer_aligned;
#endif
//rx
    bool                rx_disabled;
    uint16_t            rx_read_idx;
    uint16_t            rx_write_idx;
    uint8_t             rx_buffer[HCI_UART_RX_BUF_SIZE];
    void*               rx_timer_handle;
    P_UART_RX_CB        rx_ind;

    bool                hci_uart_bridge_flag;
} T_HCI_UART;

T_HCI_UART *hci_uart_obj;

//=========================HCI_UART_OUT=========================
void set_hci_uart_out(bool flag)
{
    T_HCI_UART *p_uart_obj = hci_uart_obj;
    if(p_uart_obj != NULL)
    {
        p_uart_obj->hci_uart_bridge_flag = flag;
    }
    else
    {
        hci_board_debug("set_hci_uart_out: hci_uart_obj is NULL\r\n");
    }
}

bool hci_uart_tx_bridge(uint8_t rc)
{
    serial_putc(&hci_serial_obj, rc);
    return true;
}

bool hci_uart_rx_bridge(uint8_t rc)
{
extern void bt_uart_tx(uint8_t rc);
    bt_uart_tx(rc);
    return true;
}

//=============================interal=========================
#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
static void uart_send_done(uint32_t id)
{
    os_sem_give(uart_send_done_sem);
}

static void uart_send_done_handler(void *p_param)
{
    (void)p_param;

    while (1) {
        if (os_sem_take(uart_send_done_sem, 0xFFFFFFFF) == false) {
            hci_board_debug("uart_send_done_handle: os_sem_take uart_send_done_sem fail\r\n");
        } else {
            T_HCI_UART *hci_rtk_obj = hci_uart_obj;
            if (hci_rtk_obj->tx_cb)
            {
                hci_rtk_obj->tx_cb();
            }
        }
    }
}
#endif

void hci_uart_rx_disable(T_HCI_UART *hci_adapter)
{
    /* We disable received data available and rx timeout interrupt, then
     * the rx data will stay in UART FIFO, and RTS will be pulled high if
     * the watermark is higher than rx trigger level. */
    hci_board_debug("hci_uart_rx_disable\r\n");
    serial_rts_control(&hci_serial_obj,0); 
    hci_adapter->rx_disabled = true;
}

void hci_uart_rx_enable(T_HCI_UART *hci_adapter)
{
    hci_board_debug("hci_uart_rx_enable\r\n");
    serial_rts_control(&hci_serial_obj,1); 
    hci_adapter->rx_disabled = false;
}

uint8_t hci_rx_empty(void)
{
    uint16_t tmpRead = hci_uart_obj->rx_read_idx;
    uint16_t tmpWrite = hci_uart_obj->rx_write_idx;
    return (tmpRead == tmpWrite);
}

uint16_t hci_rx_data_len(void)
{
    return (hci_uart_obj->rx_write_idx + HCI_UART_RX_BUF_SIZE - hci_uart_obj->rx_read_idx) % HCI_UART_RX_BUF_SIZE;
}

uint16_t hci_rx_space_len(void)
{
    return (hci_uart_obj->rx_read_idx + HCI_UART_RX_BUF_SIZE - hci_uart_obj->rx_write_idx - 1) % HCI_UART_RX_BUF_SIZE;
}

static inline void uart_insert_char(T_HCI_UART *hci_adapter, uint8_t ch)
{
    /* Should neve happen */
    if (hci_rx_space_len() == 0)
    {
        hci_board_debug("uart_insert_char: rx buffer full\r\n");
        return;
    }

    if (rltk_wlan_is_mp()) { //#if HCI_MP_BRIDGE
        if(hci_adapter->hci_uart_bridge_flag == true)
        {
          hci_uart_rx_bridge(ch);
          return;
        }
    }

    hci_adapter->rx_buffer[hci_adapter->rx_write_idx++] = ch;
    hci_adapter->rx_write_idx %= HCI_UART_RX_BUF_SIZE;

    if (hci_rx_data_len() >= HCI_UART_RX_DISABLE_COUNT && hci_adapter->rx_disabled == false)
    {
        hci_board_debug("uart_insert_char: rx disable, data len %d\r\n", hci_rx_data_len());
        hci_uart_rx_disable(hci_adapter);
    }
}

static void hciuart_irq(uint32_t id, SerialIrq event)
{
    serial_t    *sobj = (void *)id;
    int max_count = 16;
    uint8_t ch;

    if (event == RxIrq)
    {
        do
        {
            ch = serial_getc(sobj);
            uart_insert_char(hci_uart_obj, ch);
        }
        while (serial_readable(sobj) && max_count-- > 0);

        if (flag_for_hci_trx == 0) {
            if (hci_uart_obj->rx_ind)
            {
                hci_uart_obj->rx_ind();
            }
        }
    }
}
static bool hci_uart_malloc(void)
{
    if(hci_uart_obj == NULL)
    {
        hci_uart_obj = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_HCI_UART)); //reopen not need init uart

        if(!hci_uart_obj)
        {
            hci_board_debug("hci_uart_malloc: need %d, left %d\r\n", sizeof(T_HCI_UART), os_mem_peek(RAM_TYPE_DATA_ON));
            return false;
        }
        else
        {
          hci_uart_obj->rx_read_idx = 0;
          hci_uart_obj->rx_write_idx = 0;
#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
          if ((uint32_t)hci_uart_obj->tx_buffer % 32 != 0)
              hci_uart_obj->tx_buffer_aligned = hci_uart_obj->tx_buffer + (32 - (uint32_t)hci_uart_obj->tx_buffer % 32);
          else
              hci_uart_obj->tx_buffer_aligned = hci_uart_obj->tx_buffer;
          //hci_board_debug("hci_uart_obj->tx_buffer = 0x%x\r\n", hci_uart_obj->tx_buffer);
          //hci_board_debug("hci_uart_obj->tx_buffer_aligned = 0x%x\r\n", hci_uart_obj->tx_buffer_aligned);
#endif
        }
    }
    else
    {
        hci_board_debug("hci_uart_malloc: rx_buffer not free\r\n");
        return false;
    }
    return true;
}

static bool hci_uart_free(void)
{
    if(hci_uart_obj == NULL)
    {
        hci_board_debug("hci_uart_malloc: hci_uart_obj = NULL, no need free\r\n");
        return false;
    }
    os_mem_free(hci_uart_obj);
    hci_uart_obj = NULL;
    //hci_board_debug("%s: hci_uart_obj  free\r\n",__FUNCTION__);
    return true;
}

//==============================================================
void hci_uart_set_baudrate(uint32_t baudrate)
{
      hci_board_debug("Set baudrate to %d\r\n", (int)baudrate);
      serial_baud(&hci_serial_obj, baudrate);
}

bool hci_uart_tx(uint8_t *p_buf, uint16_t len, P_UART_TX_CB tx_cb)
{
    T_HCI_UART *uart_obj = hci_uart_obj;

    uart_obj->tx_len = len;
    uart_obj->tx_cb = tx_cb;

#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
    int ret;

    if (((uint32_t)p_buf >> 28 == 0x6) && ((uint32_t)p_buf % 32 != 0)) { //if p_buf is in psram && address is not 32B aligned
        if ((uint32_t)uart_obj->tx_buffer_aligned - (uint32_t)uart_obj->tx_buffer + len > HCI_UART_TX_BUF_SIZE) {
            hci_board_debug("hci_uart_tx: len %d is too long\r\n", len);
            return false;
        }
        memset(uart_obj->tx_buffer, 0, HCI_UART_TX_BUF_SIZE);
        memcpy(uart_obj->tx_buffer_aligned, p_buf, len);
        ret = serial_send_stream_dma(&hci_serial_obj, (char *)uart_obj->tx_buffer_aligned, len);
    } else
        ret = serial_send_stream_dma(&hci_serial_obj, (char *)p_buf, len);

    if (ret != 0)
    {
        hci_board_debug("hci_uart_tx: serial_send_stream_dma fail %d\r\n", ret);
        return false;
    }
#else
    serial_send_blocked(&hci_serial_obj, (char *)p_buf, len, UART_WAIT_FOREVER);
    if (uart_obj->tx_cb)
    {
        uart_obj->tx_cb();
    }
#endif

    return true;
}

bool hci_uart_init(P_UART_RX_CB rx_ind)
{
    if(hci_uart_malloc() != true)
    {
        return false;
    }
    hci_uart_obj->rx_ind = rx_ind;

#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
    if (os_sem_create(&uart_send_done_sem, 0, 1) == false) {
        hci_board_debug("hci_uart_init: os_sem_create uart_send_done_sem fail\r\n");
        return false;
    }
    if (os_task_create(&uart_send_done_task, "uart_send_done_handler", uart_send_done_handler, 0, UART_SEND_DONE_TASK_STACK_SIZE, UART_SEND_DONE_TASK_PRIORITY) == false){
        hci_board_debug("hci_uart_init: os_task_create uart_send_done_task fail\r\n");
        return false;
    }
#endif

    serial_init(&hci_serial_obj,(PinName) PIN_UART3_TX, (PinName)PIN_UART3_RX);
    serial_baud(&hci_serial_obj, 115200);
    serial_format(&hci_serial_obj, 8, ParityNone, 1);
    hci_serial_obj.uart_adp.base_addr->fcr_b.rxfifo_trigger_level = FifoLvHalf;
    //serial_rx_fifo_level(&comuart_sobj, FifoLvHalf);
    serial_set_flow_control(&hci_serial_obj, FlowControlRTSCTS, NC, NC);
#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
    serial_send_comp_handler(&hci_serial_obj, (void *)uart_send_done, (uint32_t)hci_uart_obj);
#endif
    serial_clear_rx(&hci_serial_obj);
    serial_irq_handler(&hci_serial_obj, hciuart_irq, (uint32_t)&hci_serial_obj);
    serial_irq_set(&hci_serial_obj, RxIrq, 0);
    serial_irq_set(&hci_serial_obj, RxIrq, 1);

    return true;
}

bool hci_uart_deinit(void)
{
    if(hci_uart_obj != NULL)
    {
        serial_free(&hci_serial_obj);
        memset(&hci_serial_obj,0,sizeof(serial_t));

#if defined(HCI_UART_TX_DMA) && HCI_UART_TX_DMA
        if (uart_send_done_task != NULL) {
            os_task_delete(uart_send_done_task);
            uart_send_done_task = NULL;
        }
        if (uart_send_done_sem != NULL) {
            os_sem_delete(uart_send_done_sem);
            uart_send_done_sem = NULL;
        }
#endif

        hci_uart_free();
    }
    else
    {
        hci_board_debug("hci_uart_deinit: deinit call twice\r\n");
        
    }
    return true;
}

uint16_t hci_uart_recv(uint8_t *p_buf, uint16_t size)
{
    uint16_t rx_len;

    T_HCI_UART *p_uart_obj = hci_uart_obj;
    //hci_board_debug("hci_uart_recv: write:%d, read:%d, rx_len:%d, need:%d, space_len:%d\r\n", p_uart_obj->rx_write_idx, p_uart_obj->rx_read_idx, hci_rx_data_len(), size, hci_rx_space_len());

    if(p_uart_obj == NULL)
    {
        hci_board_debug("hci_uart_recv: p_uart_obj is NULL\r\n");
        return 0;
    }
    if(hci_rx_empty())
    {
         //rx empty
         return 0;
    }
    rx_len = hci_rx_data_len();

    if (rx_len > size)
    {
        rx_len = size;
    }

    if (rx_len > HCI_UART_RX_BUF_SIZE - p_uart_obj->rx_read_idx)    /* index overflow */
    {
        rx_len = HCI_UART_RX_BUF_SIZE - p_uart_obj->rx_read_idx;
    }

    if (rx_len)
    {
        memcpy(p_buf, &(p_uart_obj->rx_buffer[p_uart_obj->rx_read_idx]), rx_len);

        p_uart_obj->rx_read_idx += rx_len;
        p_uart_obj->rx_read_idx %= HCI_UART_RX_BUF_SIZE;

        if (p_uart_obj->rx_disabled == true)    /* flow control */
        {
            if (hci_rx_data_len() < HCI_UART_RX_ENABLE_COUNT)
            {
                hci_board_debug("hci_uart_recv: rx enable, data len %d\r\n", hci_rx_data_len());
                hci_uart_rx_enable(p_uart_obj);
            }
        }
    }

    return rx_len;
}
