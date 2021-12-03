/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     data_uart.h
  * @brief    Head file for Data uart operations.
  * @details  Data uart init and print vital data through data uart.
  * @author   bill
  * @date     2015-03-19
  * @version  v2.0
  * *************************************************************************************
  */

/* Add Includes here */
#include <string.h>
#if defined(CONFIG_PLATFORM_8721D)
#include "ameba_soc.h"
#endif
#include "mesh_data_uart.h"
#include "platform_diagnose.h"
#include "mesh_config.h"

#if MESH_DATA_UART_DEBUG

#define USE_DEDICATED_BT_MESH_DATA_UART    0

#define SPRINTF_BUF_LEN     20

static bool data_uart_flag;
static pf_send_msg_from_isr_t pf_data_uart_send_msg;
#if defined(CONFIG_PLATFORM_8721D)
extern UART_TypeDef *data_uart_def;
#elif defined(CONFIG_PLATFORM_8710C)
#include "serial_api.h"
static serial_t datauart_sobj;
#endif

/**
 * @brief  retarge print.
 *
 * @param ch    char data to sent.
 * @return char data to sent.
*/
static int32_t data_uart_send_char(int32_t ch)
{
#if defined(CONFIG_PLATFORM_8721D)
#if (USE_DEDICATED_BT_MESH_DATA_UART == 0)
    LOGUART_PutChar((uint8_t)ch);
#else
    UART_CharPut(data_uart_def, (uint8_t)ch);
    UART_WaitBusy(data_uart_def, 10);
#endif
    return (ch);
#elif defined(CONFIG_PLATFORM_8710C)
#if (USE_DEDICATED_BT_MESH_DATA_UART == 1)
    serial_putc(&datauart_sobj, (uint8_t)ch);
#else
    uart_put_char((uint8_t)ch);
#endif
    return (ch);
#endif
}

uint32_t data_uart_send_string(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
    {
        data_uart_send_char(data[i]);
    }

    return len;
}
/**
 * @brief  retarge print.
 *
 * @param buf
 * @param fmt
 * @param dp
 * @return.
*/
static int32_t data_uart_vsprintf(char *buf, const char *fmt, const int32_t *dp)
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
            char tmp[SPRINTF_BUF_LEN], *q = tmp;
            int32_t alt = 0;
            int32_t shift = 28;
            int32_t width = 0;
            bool placeholder = false;

#if 1   //wei patch for %02x
            if ((*fmt  >= '0') && (*fmt  <= '9'))
            {
                unsigned char fch = *fmt;
                if (fch == '0')
                {
                    placeholder = true;
                }
                for (; (fch >= '0') && (fch <= '9'); fch = *++fmt)
                {
                    width = width * 10 + fch - '0';
                }
                shift = (width - 1) * 4;
            }
#endif

            /*
             * Before each format q points to tmp buffer
             * After each format q points past end of item
             */

            if ((*fmt == 'x') || (*fmt == 'X') || (*fmt == 'p') || (*fmt == 'P'))
            {
                /* With x86 gcc, sizeof(long) == sizeof(int32_t) */
                const long *lp = (const long *)dp;
                long h = *lp++;
                int32_t ncase = (*fmt & 0x20);
                dp = (const int32_t *)lp;
                if ((*fmt == 'p') || (*fmt == 'P'))
                {
                    alt = 1;
                }
                if (alt)
                {
                    *q++ = '0';
                    *q++ = 'X' | ncase;
                }
                bool skip = true;
                for (; shift >= 0; shift -= 4)
                {
                    char ts = "0123456789ABCDEF"[(h >> shift) & 0xF] | ncase;
                    if (ts != '0' || shift == 0)
                    {
                        skip = false;
                    }
                    if (skip)
                    {
                        if (!width)
                        {
                            continue;
                        }
                        else if (placeholder == false)
                        {
                            ts = ' ';
                        }
                    }
                    *q++ = ts;
                    if (q - tmp >= SPRINTF_BUF_LEN)
                    {
                        break;
                    }
                }
            }
            else if (*fmt == 'd')
            {
                int32_t i = *dp++;
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
                    if (q - tmp >= SPRINTF_BUF_LEN)
                    {
                        break;
                    }
                }
                while (i);

                if (width > q - tmp)
                {
                    if (width > SPRINTF_BUF_LEN)
                    {
                        width = SPRINTF_BUF_LEN;
                    }
                    width -= (q - tmp);
                    memset(q, placeholder ? '0' : ' ', width);
                    q += width;
                }

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

/**
 * @brief  Data UART interrupt handle.
 *
 * @param none
 * @return none
*/
#if defined(CONFIG_PLATFORM_8721D)
void mesh_data_uart_irq(void *data)
{
    /* avoid gcc compile warning */
    (void)data;
    uint8_t int_id;
    uint8_t rx_data;
    uint8_t reg_iir = UART_IntStatus(data_uart_def);
    if ((reg_iir & RUART_IIR_INT_PEND) != 0)
    {
        /* no pending irq */
        return ;
    }

    int_id = (reg_iir & RUART_IIR_INT_ID) >> 1;

    switch (int_id)
    {
    /* rx data valiable */
    case RUART_RECEIVER_DATA_AVAILABLE:
    case RUART_TIME_OUT_INDICATION:
        if (UART_Readable(data_uart_def))
        //while (UART_Readable(data_uart_def))
        {
            UART_CharGet(data_uart_def, &rx_data);
            pf_data_uart_send_msg(rx_data);
        }
        break;
    /* receive line status interrupt */
    case RUART_RECEIVE_LINE_STATUS:
        printe("data_uart_isr Line status error, fail!");
        break;
    default:
        break;
    }
}
#elif defined(CONFIG_PLATFORM_8710C)
void mesh_data_uart_irq(uint32_t id, SerialIrq event)
{
    uint8_t rx_char;
    serial_t *sobj = (void *)id;

    if (event == RxIrq)
    {
        rx_char = serial_getc(sobj);
        pf_data_uart_send_msg(rx_char);
    }
}
#endif

/**
 * @brief  initiate Data UART.
 *
 * @param none
 * @return none
*/
#define DATAUART_IRQ_PRIO      11
void mesh_data_uart_init(uint32_t tx_pin, uint32_t rx_pin, pf_send_msg_from_isr_t pf_send)
{
    /* avoid gcc compile warning */
    (void)tx_pin;
    (void)rx_pin;

    data_uart_flag = true;
    pf_data_uart_send_msg = pf_send;
#if defined(CONFIG_PLATFORM_8721D)
#if (USE_DEDICATED_BT_MESH_DATA_UART == 1)
    //PIN TX:A_18 RX:PA 19
    IRQn_Type irqn = UART0_IRQ;
    UART_InitTypeDef    UART_InitStruct;
	Pinmux_Config(tx_pin, PINMUX_FUNCTION_UART);
	Pinmux_Config(rx_pin, PINMUX_FUNCTION_UART);

	PAD_PullCtrl(tx_pin, GPIO_PuPd_UP);
	PAD_PullCtrl(rx_pin, GPIO_PuPd_UP);
    data_uart_def = UART0_DEV;

    //UART_PinMuxInit(0, S0);
    UART_StructInit(&UART_InitStruct);
    UART_InitStruct.WordLen = RUART_WLS_8BITS;
    UART_InitStruct.StopBit = RUART_STOP_BIT_1;
    UART_InitStruct.Parity = RUART_PARITY_DISABLE;
    UART_InitStruct.ParityType = RUART_EVEN_PARITY;
    UART_InitStruct.StickParity = RUART_STICK_PARITY_DISABLE;
    UART_InitStruct.RxFifoTrigLevel = UART_RX_FIFOTRIG_LEVEL_1BYTES;
    UART_InitStruct.FlowControl = FALSE;
    UART_Init(data_uart_def, &UART_InitStruct);
    UART_SetBaud(data_uart_def, 115200);

    InterruptDis(irqn);
    InterruptUnRegister(irqn);
    InterruptRegister((IRQ_FUN)mesh_data_uart_irq, irqn, NULL, DATAUART_IRQ_PRIO);
    InterruptEn(irqn, DATAUART_IRQ_PRIO);
    UART_INTConfig(data_uart_def, RUART_IER_ERBI | RUART_IER_ETOI | RUART_IER_ELSI, ENABLE);

    UART_RxCmd(data_uart_def, ENABLE);
#endif
#elif defined(CONFIG_PLATFORM_8710C)
#if (USE_DEDICATED_BT_MESH_DATA_UART == 1)
    hal_pinmux_unregister(tx_pin, 0x01 << 4);
    hal_pinmux_unregister(rx_pin, 0x01 << 4);
    hal_gpio_pull_ctrl(tx_pin, 0);
    hal_gpio_pull_ctrl(rx_pin, 0);

    serial_init(&datauart_sobj, tx_pin, rx_pin);
    serial_baud(&datauart_sobj, 115200);
    serial_format(&datauart_sobj, 8, ParityNone, 1);
    serial_irq_handler(&datauart_sobj, mesh_data_uart_irq, (uint32_t)&datauart_sobj);
    serial_irq_set(&datauart_sobj, RxIrq, 1);
    serial_irq_set(&datauart_sobj, TxIrq, 1);
#endif
#endif
}

void data_uart_debug(char *fmt, ...)
{
    if (data_uart_flag)
    {
        data_uart_vsprintf(0, fmt, ((const int32_t *)&fmt) + 1);
    }
}

void data_uart_dump(uint8_t *pbuffer, uint32_t len)
{
    if (data_uart_flag)
    {
        data_uart_send_char('0');
        data_uart_send_char('x');
        for (uint32_t loop = 0; loop < len; loop++)
        {
            char data = "0123456789ABCDEF"[pbuffer[loop] >> 4];
            data_uart_send_char(data);
            data = "0123456789ABCDEF"[pbuffer[loop] & 0x0f];
            data_uart_send_char(data);
        }
        data_uart_send_char('\r');
        data_uart_send_char('\n');
    }
}

#endif
