#include <stdio.h>
#include <stdlib.h>
#include <data_uart.h>
#include <os_msg.h>
#include <trace_app.h>
#include <app_msg.h>
#include "serial_api.h"
#include "bt_board.h"
#if defined(CONFIG_PLATFORM_8710C)
#include "platform_opts_bt.h"
#endif

#ifdef CONFIG_BT_USER_COMMAND
#define USE_DEDICATED_BT_DATA_UART    CONFIG_BT_USER_COMMAND
#else
#define USE_DEDICATED_BT_DATA_UART    0
#endif

static void *h_event_q;
static void *h_io_q;
#if (USE_DEDICATED_BT_DATA_UART == 1)
static serial_t datauart_sobj;
#else
extern void uart_put_char(uint8_t c);
#endif
int data_uart_send_char(int ch);

void data_uart_msg_init(void *event_queue_handle, void *io_queue_handle)
{
    h_event_q = event_queue_handle;
    h_io_q = io_queue_handle;
}

void data_uart_msg_send(uint8_t rx_char)
{
    T_IO_MSG io_driver_msg_send;
    uint8_t event  = EVENT_IO_TO_APP;
    io_driver_msg_send.type = IO_MSG_TYPE_UART;
    io_driver_msg_send.subtype = rx_char;

    if (os_msg_send(h_io_q, &io_driver_msg_send, 0) == false)
    {
        APP_PRINT_ERROR0("data_uart_msg_send: send data failed");
    }
    else if (os_msg_send(h_event_q, &event, 0) == false)
    {
        APP_PRINT_ERROR0("data_uart_msg_send: send event failed");
    }
}

int data_uart_vsprintf(char *buf, const char *fmt, const int *dp)
{
    char *p, *s;

    s = buf;
    for (; *fmt != '\0'; ++fmt)
    {
        if (*fmt != '%')
        {
            buf ? *s++ = *fmt : data_uart_send_char(*fmt);
            continue;
        }
        if (*++fmt == 's')
        {
            for (p = (char *)*dp++; *p != '\0'; p++)
            {
                buf ? *s++ = *p : data_uart_send_char(*p);
            }
        }
        else    /* Length of item is bounded */
        {
            char tmp[20], *q = tmp;
            int shift = 28;

            if ((*fmt  >= '0') && (*fmt  <= '9'))
            {
                int width;
                unsigned char fch = *fmt;
                for (width = 0; (fch >= '0') && (fch <= '9'); fch = *++fmt)
                {
                    width = width * 10 + fch - '0';
                }
                shift = (width - 1) * 4;
            }
            /*
             * Before each format q points to tmp buffer
             * After each format q points past end of item
             */

            if ((*fmt == 'x') || (*fmt == 'X') || (*fmt == 'p') || (*fmt == 'P'))
            {
                /* With x86 gcc, sizeof(long) == sizeof(int) */
                const long *lp = (const long *)dp;
                long h = *lp++;
                int ncase = (*fmt & 0x20);
                int alt = 0;

                dp = (const int *)lp;
                if ((*fmt == 'p') || (*fmt == 'P'))
                {
                    alt = 1;
                }
                if (alt)
                {
                    *q++ = '0';
                    *q++ = 'X' | ncase;
                }
                for (; shift >= 0; shift -= 4)
                {
                    * q++ = "0123456789ABCDEF"[(h >> shift) & 0xF] | ncase;
                }
            }
            else if (*fmt == 'd')
            {
                int i = *dp++;
                char *r;
                if (i < 0)
                {
                    *q++ = '-';
                    i = -i;
                }
                p = q;      /* save beginning of digits */
                do
                {
                    *q++ = '0' + (i % 10);
                    i /= 10;
                }
                while (i);
                /* reverse digits, stop in middle */
                r = q;      /* don't alter q */
                while (--r > p)
                {
                    i = *r;
                    *r = *p;
                    *p++ = i;
                }
            }
            else if (*fmt == 'c')
            {
                *q++ = *dp++;
            }
            else
            {
                *q++ = *fmt;
            }
            /* now output the saved string */
            for (p = tmp; p < q; ++p)
            {
                buf ? *s++ = *p : data_uart_send_char(*p);
            }
        }
    }
    if (buf)
    {
        *s = '\0';
    }
    return (s - buf);
}

void data_uart_print(char *fmt, ...)
{
    (void)data_uart_vsprintf(0, fmt, ((const int *)&fmt) + 1);
}

int data_uart_send_char(int ch)
{
#if (USE_DEDICATED_BT_DATA_UART == 1)
	serial_putc(&datauart_sobj, (uint8_t)ch);
#else
	uart_put_char((uint8_t)ch);
#endif
    return ch;
}
/****************************************************************************/
/* UART interrupt                                                           */
/****************************************************************************/
void  data_uart_irq(uint32_t id, SerialIrq event)
{
    uint8_t rx_char;
    serial_t    *sobj = (void *)id;

    if (event == RxIrq)
    {
        rx_char = serial_getc(sobj);
        data_uart_msg_send(rx_char);

    }

}


/** 
  * UART Parameter
  * BaudRate: 115200
  * Word Length: 8 bit
  * Stop Bit: 1 bit
  * Parity: None
  * RX FIFO: 1 byte
  */
void data_uart_init(void *event_queue_handle, void *io_queue_handle)
{
    data_uart_msg_init(event_queue_handle, io_queue_handle);

#if (USE_DEDICATED_BT_DATA_UART == 1)
#if 1
    hal_pinmux_unregister(DATA_TX, 0x01 << 4);
    hal_pinmux_unregister(DATA_RX, 0x01 << 4);
    hal_gpio_pull_ctrl(DATA_TX, 0);
    hal_gpio_pull_ctrl(DATA_RX, 0);
#endif
    serial_init(&datauart_sobj, DATA_TX, DATA_RX);
    serial_baud(&datauart_sobj, 115200);
    serial_format(&datauart_sobj, 8, ParityNone, 1);
    serial_irq_handler(&datauart_sobj, data_uart_irq, (uint32_t)&datauart_sobj);
    serial_irq_set(&datauart_sobj, RxIrq, 1);
    serial_irq_set(&datauart_sobj, TxIrq, 1);
#endif
}

