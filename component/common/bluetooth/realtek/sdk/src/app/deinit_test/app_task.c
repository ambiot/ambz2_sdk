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
#include <stdint.h>
#include <os_msg.h>
#include <os_task.h>
#include <gap.h>
#include <app_task.h>
#include <app_msg.h>
#include <gap_test_app.h>
#include <trace_app.h>

/*============================================================================*
 *                              Macros
 *============================================================================*/
//! Task priorities
#define APP_TASK_PRIORITY             1
//!Task stack size
#define APP_TASK_STACK_SIZE           256 * 8

//!IO message queue size
#define MAX_NUMBER_OF_IO_MESSAGE      0x20
//!Event message queue size
#define MAX_NUMBER_OF_EVENT_MESSAGE   (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE)

/*============================================================================*
 *                              Variables
 *============================================================================*/
void *app_task_handle;   //!< APP Task handle
void *evt_queue_handle;  //!< Event queue handle
void *io_queue_handle;   //!< IO queue handle

/*============================================================================*
 *                              Functions
 *============================================================================*/
void app_main_task(void *p_param);

/**
 * @brief  Initialize App task
 * @return void
 */
void app_task_init()
{
    os_task_create(&app_task_handle, "app", app_main_task, 0, APP_TASK_STACK_SIZE,
                   APP_TASK_PRIORITY);
}

void app_task_deinit()
{
    if (io_queue_handle)
    {
        os_msg_queue_delete(io_queue_handle);
    }
    if (evt_queue_handle)
    {
        os_msg_queue_delete(evt_queue_handle);
    }
    if (app_task_handle)
    {
        os_task_delete(app_task_handle);
    }
    io_queue_handle = NULL;
    evt_queue_handle = NULL;
    app_task_handle = NULL;
    gap_dev_state.gap_init_state = 0;
    gap_dev_state.gap_adv_state = 0;
    gap_dev_state.gap_scan_state = 0;
    gap_dev_state.gap_conn_state = 0;
}

void app_send_msg(uint16_t sub_type)
{
    uint8_t event = EVENT_IO_TO_APP;
    T_IO_MSG io_msg;
    io_msg.type = IO_MSG_TYPE_QDECODE;
    io_msg.subtype = sub_type;

    if (os_msg_send(io_queue_handle, &io_msg, 0) == false)
    {
        APP_PRINT_ERROR1("app_send_msg msg fail: subtype 0x%x", io_msg.subtype);
    }
    else if (os_msg_send(evt_queue_handle, &event, 0) == false)
    {
        APP_PRINT_ERROR1("app_send_msg event fail: subtype 0x%x", io_msg.subtype);
    }
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_params    Parameters sending to the task
 * @return       void
 */
void app_main_task(void *p_param)
{
    uint8_t event;

    APP_PRINT_INFO0("app_main_task");

    os_msg_queue_create(&io_queue_handle, MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&evt_queue_handle, MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(uint8_t));

    gap_start_bt_stack(evt_queue_handle, io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == EVENT_IO_TO_APP)
            {
                T_IO_MSG io_msg;
                if (os_msg_recv(io_queue_handle, &io_msg, 0) == true)
                {
                    app_handle_io_msg(io_msg);
                }
            }
            else
            {
                gap_handle_msg(event);
            }
        }
    }
}

