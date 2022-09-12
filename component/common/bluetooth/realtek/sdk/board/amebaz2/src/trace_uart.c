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
    bool tx_switch;
} TRACE_UART_INFO;

TRACE_UART_INFO g_uart_obj;
serial_t trace_sobj;

bool trace_uart_init(void)
{
   if (!check_sw((int)EFUSE_SW_TRACE_SWITCH))
   {
        printf("trace_uart_init: TRACE OPEN\r\n");
        hal_pinmux_unregister(TRACE_TX, 0x01 << 4);
        hal_pinmux_unregister(TRACE_RX, 0x01 << 4);
        hal_gpio_pull_ctrl(TRACE_TX, 0);
        hal_gpio_pull_ctrl(TRACE_RX, 0);

        serial_init(&trace_sobj, TRACE_TX, TRACE_RX);

        serial_baud(&trace_sobj, TRACE_UART_BAUDRATE);

        serial_format(&trace_sobj, 8, ParityNone, 1);

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
    if (!check_sw((int)EFUSE_SW_TRACE_SWITCH))
    {
        if (g_uart_obj.tx_switch == true)
        {
            serial_free(&trace_sobj);
            g_uart_obj.tx_switch = false;
            return true;
        }
        else
        {
            printf("trace_uart_deinit: no need\r\n");
            return false;
        }
    }
    return true;
}

bool trace_uart_tx(uint8_t *pstr, uint16_t len, UART_TX_CB tx_cb)
{
    if (g_uart_obj.tx_switch == false)
    {
        if (tx_cb)
            tx_cb();
        return true;
    }

    serial_send_blocked(&trace_sobj, (char *)pstr, len, len);

    if (tx_cb)
        tx_cb();

    return true;
}

