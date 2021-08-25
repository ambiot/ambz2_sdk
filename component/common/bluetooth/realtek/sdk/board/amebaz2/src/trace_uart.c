/**
 * Copyright (c) 2015, Realsil Semiconductor Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "trace_uart.h"

#include "bt_board.h"
#include "serial_api.h"
#include "serial_ex_api.h"

typedef struct _TraceUartInfo
{
    uint8_t  *tx_buffer;
    uint16_t tx_len;
    uint8_t  tx_busy;
    bool     tx_switch;
    UART_TX_CB  tx_cb;
} TRACE_UART_INFO;

static TRACE_UART_INFO   g_uart_obj;

serial_t    trace_sobj;

#ifndef TRACE_SWITCH_CLOSE
static void uart_recv_done(uint32_t id)
{
    //do nothing
}

static uint32_t traceuart_dma_tx_complete(void *data)
{
    TRACE_UART_INFO *trace_pointer = (TRACE_UART_INFO *) data;
    if (trace_pointer->tx_cb)
        trace_pointer->tx_cb();
    return 0;
}
#endif

bool trace_uart_init(void)
{
   if (!CHECK_SW(EFUSE_SW_TRACE_SWITCH))
   {
        printf("trace_uart_init: TRACE OPEN\r\n");
        hal_pinmux_unregister(TRACE_TX, 0x01 << 4);
        hal_pinmux_unregister(TRACE_RX, 0x01 << 4);
        hal_gpio_pull_ctrl(TRACE_TX, 0);
        hal_gpio_pull_ctrl(TRACE_RX, 0);

        serial_init(&trace_sobj, TRACE_TX, TRACE_RX);

        serial_baud(&trace_sobj, TRACE_UART_BAUDRATE);

        serial_format(&trace_sobj, 8, ParityNone, 1);

        serial_send_comp_handler(&trace_sobj, (void *)traceuart_dma_tx_complete,
                                (uint32_t)&g_uart_obj);


        serial_recv_comp_handler(&trace_sobj, (void *)uart_recv_done,
                                (uint32_t)&g_uart_obj);
        g_uart_obj.tx_switch = true;
    }
    else
    {
        g_uart_obj.tx_switch = false;
    }
    return true;
}

bool trace_uart_deinit(void)
{
    if (!CHECK_SW(EFUSE_SW_TRACE_SWITCH))
    {
        if (g_uart_obj.tx_switch == true) {
            serial_free(&trace_sobj);
            g_uart_obj.tx_switch = false;
            return true;
        }
        else {
            printf("\r\n: trace_uart_deinit: no need\r\n");
            return false;
        }
    }
    return true;
}

bool trace_uart_tx(uint8_t *pstr, uint16_t len, UART_TX_CB tx_cb)
{
    g_uart_obj.tx_cb = tx_cb;
    if (g_uart_obj.tx_switch == false)
    {
        if(g_uart_obj.tx_cb)
            g_uart_obj.tx_cb();
        return true;
    }
    g_uart_obj.tx_cb = tx_cb;
    serial_send_blocked(&trace_sobj, (char *)pstr, len, len);
    
    if (g_uart_obj.tx_cb)
    {
        g_uart_obj.tx_cb();
    }
#if 0
    ret = serial_send_stream_dma(&trace_sobj, (char *)pstr,len);
    if (ret != 0)
    {
        printf("%s Error(%d)\n", __FUNCTION__, ret);
    }
#endif
    return true;
}

void bt_trace_set_switch(bool flag)
{
    g_uart_obj.tx_switch = flag;
}
