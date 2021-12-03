/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    application.c
  * @brief   application task.
  * @details
  * @author  ranhui
  * @date    2016-02-18
  * @version v0.1
  ******************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2016 Realtek Semiconductor Corporation</center></h2>
  ******************************************************************************
  */

/** Add Includes here **/
#include <os_msg.h>
#include <os_task.h>
#include "app_task.h"
#include "app_msg.h"
#include "gap.h"
#include <user_cmd.h>


extern void app_handle_io_msg(T_IO_MSG io_msg);
void app_main_task(void *p_param);

#define MAX_NUMBER_OF_GAP_MESSAGE    0x20  //!<  GAP message queue size
#define MAX_NUMBER_OF_IO_MESSAGE      0x20  //!<  IO message queue size
#define MAX_NUMBER_OF_EVENT_MESSAGE   (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE) //!< Event message queue size


/**<  Handles of app task and queues. */
void *app_task_handle;
void *evt_queue_handle;
void *io_queue_handle;

/**
  * @brief  Create application task.
  * @retval none
  */
void app_task_init()
{

    /* Create APP Task. */
    os_task_create(&app_task_handle, "app", app_main_task, 0, 512 * 4, 1);
}

/**
  * @brief  application task.
  * @param  pvParameters
  * @retval none
  */
void app_main_task(void *p_param)
{
    char event;

    os_msg_queue_create(&io_queue_handle, MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&evt_queue_handle, MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(unsigned char));

    gap_start_bt_stack(evt_queue_handle, io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    data_uart_init(evt_queue_handle, io_queue_handle);
    user_cmd_init(&user_cmd_if, "ble_auto_test");

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

