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
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST
#include <string.h>
#include <os_msg.h>
#include <os_task.h>
#include "ble_throughput_app_task.h"
#include "app_msg.h"
#include "gap.h"
#include "ble_throughput_user_cmd.h"
#include "gap_msg.h"

extern T_GAP_DEV_STATE ble_throughput_gap_dev_state;
extern void ble_throughput_app_handle_io_msg(T_IO_MSG io_msg);
void ble_throughput_app_main_task(void *p_param);

#define BLE_THROUGHPUT_MAX_NUMBER_OF_GAP_MESSAGE    0x20  //!<  GAP message queue size
#define BLE_THROUGHPUT_MAX_NUMBER_OF_IO_MESSAGE      0x20  //!<  IO message queue size
#define BLE_THROUGHPUT_MAX_NUMBER_OF_EVENT_MESSAGE   (BLE_THROUGHPUT_MAX_NUMBER_OF_GAP_MESSAGE + BLE_THROUGHPUT_MAX_NUMBER_OF_IO_MESSAGE) //!< Event message queue size


/**<  Handles of app task and queues. */
void *ble_throughput_app_task_handle;
void *ble_throughput_evt_queue_handle;
void *ble_throughput_io_queue_handle;

/**
  * @brief  application task.
  * @param  pvParameters
  * @retval none
  */
void ble_throughput_app_main_task(void *p_param)
{
    char event;

    os_msg_queue_create(&ble_throughput_io_queue_handle, BLE_THROUGHPUT_MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&ble_throughput_evt_queue_handle, BLE_THROUGHPUT_MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(unsigned char));

    gap_start_bt_stack(ble_throughput_evt_queue_handle, ble_throughput_io_queue_handle, BLE_THROUGHPUT_MAX_NUMBER_OF_GAP_MESSAGE);

    data_uart_init(ble_throughput_evt_queue_handle, ble_throughput_io_queue_handle);
    user_cmd_init(&user_cmd_if, "ble_throughput_test");

    while (true)
    {
        if (os_msg_recv(ble_throughput_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == EVENT_IO_TO_APP)
            {
                T_IO_MSG io_msg;
                if (os_msg_recv(ble_throughput_io_queue_handle, &io_msg, 0) == true)
                {
                    ble_throughput_app_handle_io_msg(io_msg);
                }
            }
            else
            {
                gap_handle_msg(event);
            }
        }
    }
}

/**
  * @brief  Create application task.
  * @retval none
  */
void ble_throughput_app_task_init()
{

    /* Create APP Task. */
    os_task_create(&ble_throughput_app_task_handle, "ble_throughput_app", ble_throughput_app_main_task, 0, 512 * 4, 1);
}

void ble_throughput_app_task_deinit(void)
{
	if (ble_throughput_app_task_handle) {
		os_task_delete(ble_throughput_app_task_handle);
	}
	if (ble_throughput_io_queue_handle) {
		os_msg_queue_delete(ble_throughput_io_queue_handle);
	}
	if (ble_throughput_evt_queue_handle) {
		os_msg_queue_delete(ble_throughput_evt_queue_handle);
	}
	ble_throughput_io_queue_handle = NULL;
	ble_throughput_evt_queue_handle = NULL;
	ble_throughput_app_task_handle = NULL;

	ble_throughput_gap_dev_state.gap_init_state = 0;
	ble_throughput_gap_dev_state.gap_adv_sub_state = 0;
	ble_throughput_gap_dev_state.gap_adv_state = 0;
	ble_throughput_gap_dev_state.gap_scan_state = 0;
	ble_throughput_gap_dev_state.gap_conn_state = 0;

}
#endif

