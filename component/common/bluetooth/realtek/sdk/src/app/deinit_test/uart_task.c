/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_task.c
   * @brief     Routines to create App task and handle events & messages
   * @author    jane
   * @date      2017-06-02
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include <os_msg.h>
#include <os_task.h>
#include <uart_task.h>
#include <app_msg.h>
#include <user_cmd.h>
#include <gap_test_app.h>
#include <trace_app.h>
#include "app_task.h"
#include <os_timer.h>
#include <os_mem.h>

/*============================================================================*
 *                              Macros
 *============================================================================*/
//! Task priorities
#define UART_TASK_PRIORITY             1
//!Task stack size
#define UART_TASK_STACK_SIZE           256 * 8

/*============================================================================*
 *                              Variables
 *============================================================================*/
void *uart_task_handle;   //!< APP Task handle
void *uart_evt_queue_handle;  //!< Event queue handle
void *uart_io_queue_handle;   //!< IO queue handle
void *auto_test_timer;

/*============================================================================*
 *                              Functions
 *============================================================================*/

T_CUR_TEST_CASE test_case_id = TC_IDLE;
uint32_t test_cur_count = 0;
uint32_t test_max_count = 0;
uint32_t dump_memory_start;
uint32_t dump_memory_end;

void uart_main_task(void *p_param);

void app_send_msg_to_uart_app(T_TEST_PROC_SUBTYPE sub_type, uint16_t cause)
{
    uint8_t event = EVENT_IO_TO_APP;
    T_IO_MSG io_msg;
    io_msg.type = IO_MSG_TYPE_ANCS;
    io_msg.subtype = sub_type;
    io_msg.u.param = cause;

    if (os_msg_send(uart_io_queue_handle, &io_msg, 0) == false)
    {
        APP_PRINT_ERROR1("app_send_msg_to_uart_app msg fail: subtype 0x%x", io_msg.subtype);
    }
    else if (os_msg_send(uart_evt_queue_handle, &event, 0) == false)
    {
        APP_PRINT_ERROR1("app_send_msg_to_uart_app event fail: subtype 0x%x", io_msg.subtype);
    }
}

void app_handle_uart_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;
    uint8_t rx_char;

    switch (msg_type)
    {
    case IO_MSG_TYPE_UART:
        /* We handle user command informations from Data UART in this branch. */
        rx_char = (uint8_t)io_msg.subtype;
        user_cmd_collect(&user_cmd_if, &rx_char, sizeof(rx_char), user_cmd_table);
        break;
    case IO_MSG_TYPE_ANCS:
        switch (io_msg.subtype)
        {
        case TC_START:
            {
                APP_PRINT_ERROR2("test case[%d] TC_START: test_max_count %d",
                                 test_case_id, test_max_count);
                dump_memory_start = os_mem_peek(RAM_TYPE_DATA_ON);
                app_send_msg_to_uart_app(TC_DEL_STACK, 0);
            }
            break;

        case TC_DEL_STACK:
            {
                APP_PRINT_ERROR2("test case[%d] TC_DEL_STACK: test_cur_count %d",
                                 test_case_id, test_cur_count);
                app_deinit();
                upper_task_deinit();
                app_send_msg_to_uart_app(TC_ADD_STACK, 0);
            }
            break;

        case TC_ADD_STACK:
            {
                APP_PRINT_ERROR2("test case[%d] TC_ADD_STACK: test_cur_count %d",
                                 test_case_id, test_cur_count);
                upper_task_init();
                app_init();
            }
            break;

        case TC_STACK_STARTED:
            {
                test_cur_count++;
                APP_PRINT_ERROR2("test case[%d] TC_STACK_STARTED: test_cur_count %d",
                                 test_case_id, test_cur_count);
                if (test_cur_count < test_max_count)
                {
                    if (test_case_id == TC_0001_IDLE)
                    {
                        app_send_msg_to_uart_app(TC_DEL_STACK, 0);
                    }
                    else if (test_case_id == TC_0002_ADV)
                    {
                        app_send_msg(0);
                    }
                    else if (test_case_id == TC_0003_SCAN)
                    {
                        app_send_msg(1);
                    }
                    else if (test_case_id == TC_0005_CON_RX)
                    {
                        app_send_msg(0);
                    }
                    else if (test_case_id == TC_0004_CON_TX)
                    {
                        app_send_msg(0);
                    }
                }
                else
                {
                    dump_memory_end = os_mem_peek(RAM_TYPE_DATA_ON);
                    APP_PRINT_ERROR4("test case[%d]: complete test_max_count %d, dump_memory_start %d, dump_memory_end %d",
                                     test_case_id, test_max_count, dump_memory_start, dump_memory_end);
                }
            }
            break;
        case TC_START_ADV:
            APP_PRINT_ERROR2("test case[%d] TC_START_ADV: test_cur_count %d",
                             test_case_id, test_cur_count);
            if (test_case_id == TC_0002_ADV)
            {
                app_send_msg_to_uart_app(TC_DEL_STACK, 0);
            }
            break;

        case TC_START_SCAN:
            APP_PRINT_ERROR2("test case[%d] TC_START_SCAN: test_cur_count %d",
                             test_case_id, test_cur_count);
            if (test_case_id == TC_0003_SCAN)
            {
                app_send_msg_to_uart_app(TC_DEL_STACK, 0);
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

void auto_test_timeout(void *pxTimer)
{
    APP_PRINT_ERROR2("test case[%d] auto_test_timeout: test_cur_count %d",
                     test_case_id, test_cur_count);
    app_send_msg_to_uart_app(TC_DEL_STACK, 0);
}
/**
 * @brief  Initialize App task
 * @return void
 */
void uart_task_init()
{
    os_timer_create(&auto_test_timer, "auto test timer", 1, 1000, false, auto_test_timeout);
    os_task_create(&uart_task_handle, "uart app", uart_main_task, 0, UART_TASK_STACK_SIZE,
                   UART_TASK_PRIORITY);
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_params    Parameters sending to the task
 * @return       void
 */
void uart_main_task(void *p_param)
{
    uint8_t event;

    os_msg_queue_create(&uart_io_queue_handle, 0x20, sizeof(T_IO_MSG));
    os_msg_queue_create(&uart_evt_queue_handle, 0x20, sizeof(uint8_t));

    data_uart_init(uart_evt_queue_handle, uart_io_queue_handle);
    user_cmd_init(&user_cmd_if, "gap_test");

    while (true)
    {
        if (os_msg_recv(uart_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == EVENT_IO_TO_APP)
            {
                T_IO_MSG io_msg;
                if (os_msg_recv(uart_io_queue_handle, &io_msg, 0) == true)
                {
                    app_handle_uart_msg(io_msg);
                }
            }
        }
    }
}

