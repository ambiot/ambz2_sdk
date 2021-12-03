/**
*********************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     dataTrans_uart.c
* @brief    Data uart operations for testing profiles.
* @details  Data uart init and print data through data uart.
* @author
* @date     2015-03-19
* @version  v0.1
*********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include "bt_datatrans_uart.h"
#include "bt_datatrans_module_param_config.h"
#include "trace_app.h"
#include "app_msg.h"
#include "os_msg.h"
#include "bt_datatrans_multilink_manager.h"
#include "ftl_app.h"
#include "os_timer.h"
#include "gap_conn_le.h"
#include "os_sched.h"
#include "bt_board.h"
#include "serial_api.h"
#include "serial_ex_api.h"
#include <platform/platform_stdlib.h>
#include "os_task.h"

/* task queues used to send message from UART. */
extern T_GAP_CONN_STATE bt_datatrans_gap_conn_state;
extern void *bt_datatrans_evt_queue_handle;
extern void *bt_datatrans_io_queue_handle;

static serial_t bt_datatrans_sobj;
void *DataTransHandle = NULL;
void *DatatransUartRxSema = NULL;
char uart_rx_buf[8] = {0}; //read data from rx fifo
uint32_t rx_irq_flag = 0;

#define BT_DATATRANS_UART_TASK_PRIORITY          6			/* Task priorities. */
#define BT_DATATRANS_UART_TASK_STACK_SIZE        256 * 4	/* Task stack size. */

/* send message to bt_datatrans_app_main_task */
bool datatrans_send_msg_to_app(T_IO_MSG *p_msg)
{
    uint8_t event = EVENT_IO_TO_APP;

    if (os_msg_send(bt_datatrans_io_queue_handle, p_msg, 0) == false)
    {
        printf("send_msg_to_app fail2\r\n");
        return false;
    }
    else if (os_msg_send(bt_datatrans_evt_queue_handle, &event, 0) == false)
    {
        printf("send_msg_to_app fail\r\n");
        return false;
    }
    return true;
}


/* uart tx data */
void bt_datatrans_uart_tx_char(int ch)
{
	serial_putc(&bt_datatrans_sobj, ch);
}

/* handle uart rx data */
void bt_datatrans_uart_rcv_done(uint32_t id)
{
	T_IO_MSG uart_msg;

	transferConfigInfo.uart_idle = 0;

	if ((IO_Receive.WriteOffset + 8) <= RECEIVE_BUF_MAX_LENGTH)
	{
		memcpy(IO_Receive.buf + IO_Receive.WriteOffset, uart_rx_buf, 8);
		IO_Receive.WriteOffset += 8;
		if (IO_Receive.WriteOffset == RECEIVE_BUF_MAX_LENGTH)
		{
			IO_Receive.WriteOffset = 0;
		}

	}
	else
	{
		uint16_t len = 0;
		len = RECEIVE_BUF_MAX_LENGTH - IO_Receive.WriteOffset;
		memcpy(IO_Receive.buf + IO_Receive.WriteOffset, uart_rx_buf, len);
		IO_Receive.WriteOffset = 0;

		memcpy(IO_Receive.buf + IO_Receive.WriteOffset, uart_rx_buf+len, 8-len);
		IO_Receive.WriteOffset += (8 - len);
	}
	
	if ((8 + IO_Receive.datalen) > RECEIVE_BUF_MAX_LENGTH)	 /* Rx overrun */
	{
		IO_Receive.datalen = RECEIVE_BUF_MAX_LENGTH;
		IO_Receive.ReadOffset = IO_Receive.WriteOffset;
	}
	else
	{
		IO_Receive.datalen += 8;		 /* update length */
	}

    if (bt_datatrans_gap_conn_state == GAP_CONN_STATE_CONNECTED)
    {
    	rx_irq_flag++;			
		if (rx_irq_flag % 31 == 0)	// if IO_Receive.datalen >= MTU_SIZE - 3
        {
            uart_msg.type = IO_MSG_TYPE_UART;
            uart_msg.subtype = IO_MSG_UART_RX;
            datatrans_send_msg_to_app(&uart_msg);
        }
    }
		
    os_sem_give(DatatransUartRxSema);
	
}


/*  UART RX idle callback function, send uart msg to bt_datatrans_app_main_task */
void bt_datatrans_uart_rx_idle(void *arg)
{
	T_IO_MSG uart_msg;
    int rx_bytes;

    rx_bytes = serial_recv_stream_abort(&bt_datatrans_sobj);
	
    if (rx_bytes > 0) 
	{

		if ((IO_Receive.WriteOffset + rx_bytes) <= RECEIVE_BUF_MAX_LENGTH)
		{
			memcpy(IO_Receive.buf + IO_Receive.WriteOffset, uart_rx_buf, rx_bytes);
			IO_Receive.WriteOffset += rx_bytes;
			if (IO_Receive.WriteOffset == RECEIVE_BUF_MAX_LENGTH)
			{
				IO_Receive.WriteOffset = 0;
			}
		
		}
		else
		{
			uint16_t len = 0;
			len = RECEIVE_BUF_MAX_LENGTH - IO_Receive.WriteOffset;
			memcpy(IO_Receive.buf + IO_Receive.WriteOffset, uart_rx_buf, len);
			IO_Receive.WriteOffset = 0;
		
			memcpy(IO_Receive.buf + IO_Receive.WriteOffset, uart_rx_buf+len, rx_bytes-len);
			IO_Receive.WriteOffset += (rx_bytes - len);
		} 
		
		if ((rx_bytes + IO_Receive.datalen) > RECEIVE_BUF_MAX_LENGTH)  /* Rx overrun */
		{
			IO_Receive.datalen = RECEIVE_BUF_MAX_LENGTH;
			IO_Receive.ReadOffset = IO_Receive.WriteOffset;
		}
		else
		{
			IO_Receive.datalen += rx_bytes;		 /* update length */
		}

    }

	rx_irq_flag = 0;
	uart_msg.type = IO_MSG_TYPE_UART;
	uart_msg.subtype = IO_MSG_UART_RX_TIMEOUT;
	datatrans_send_msg_to_app(&uart_msg);
	transferConfigInfo.uart_idle = 1;	
	
	os_sem_give(DatatransUartRxSema);

}

void bt_datatrans_set_baud(void)
{
	/* dataTransInfo.device_mode.baudrateidx */
	switch (transferConfigInfo.baudrate)
	{
	case 2400:
		dataTransInfo.device_mode.baudrateidx = BAUD_2400;
		break;
	case 4800:
		dataTransInfo.device_mode.baudrateidx = BAUD_4800;
		break;
	case 9600:
		dataTransInfo.device_mode.baudrateidx = BAUD_9600;
		break;
	case 19200:
		dataTransInfo.device_mode.baudrateidx = BAUD_19200;
		break;
	case 38400:
		dataTransInfo.device_mode.baudrateidx = BAUD_38400;
		break;
	case 57600:
		dataTransInfo.device_mode.baudrateidx = BAUD_57600;
		break;
	case 115200:
		dataTransInfo.device_mode.baudrateidx = BAUD_115200;
		break;	
	case 921600:
		dataTransInfo.device_mode.baudrateidx = BAUD_921600;
		break;
	default:
		dataTransInfo.device_mode.baudrateidx = BAUD_9600;
		break;		
	}	
	
	serial_baud(&bt_datatrans_sobj, transferConfigInfo.baudrate);
	
	//WRTIE Baud rate idx.
	ftl_save(&dataTransInfo.device_mode, INFO_DEVICE_MODE_OFFSET, 4);

}

void bt_datatrans_uart_init(void)
{
	serial_init(&bt_datatrans_sobj, (PinName)PA_11, (PinName)PA_12);
	serial_baud(&bt_datatrans_sobj, transferConfigInfo.baudrate);
	serial_format(&bt_datatrans_sobj, 8, ParityNone, 1);
	serial_recv_comp_handler(&bt_datatrans_sobj, (void*)bt_datatrans_uart_rcv_done, (uint32_t)&bt_datatrans_sobj);
	hal_uart_rx_idle_timeout_en(&bt_datatrans_sobj.uart_adp, 200, bt_datatrans_uart_rx_idle, NULL);
}


void bt_datatrans_uart_task(void *p_param)
{
	int ret = 0;
	memset(uart_rx_buf, 0, 8);
	rx_irq_flag = 0;
	
	while(1)
	{		
        ret = serial_recv_stream(&bt_datatrans_sobj, uart_rx_buf, 8);
		
		if(ret)
		{
			printf("serial_recv_stream fail, error %d\r\n", ret);
		}
	
        if (os_sem_take(DatatransUartRxSema, 0xFFFFFFFF) == false) {
            printf("DatatransUartRxSema take fail\r\n");
        }	
	}
}


void bt_datatrans_uart_task_init(void)
{
	bt_datatrans_uart_init();
	
	os_sem_create(&DatatransUartRxSema, 0, 1);
    
	if(os_task_create(&DataTransHandle, "bt_datatrans_uart_task", bt_datatrans_uart_task, 0, BT_DATATRANS_UART_TASK_STACK_SIZE, 
		BT_DATATRANS_UART_TASK_PRIORITY) == false) {
		printf("bt_datatrans_uart_task create fail\r\n");
	}
	
}

void bt_datatrans_uart_deinit(void)
{
	serial_free(&bt_datatrans_sobj);
	memset(&bt_datatrans_sobj,0,sizeof(serial_t));
}

void bt_datatrans_uart_task_deinit(void)
{

	bt_datatrans_uart_deinit();
	
	if (DataTransHandle) {
		os_task_delete(DataTransHandle);
	}
	
	os_sem_delete(DatatransUartRxSema);

	DataTransHandle = NULL;
	DatatransUartRxSema = NULL;

}

#endif
