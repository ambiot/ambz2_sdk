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
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include <osdep_service.h>
#include <os_msg.h>
#include <os_task.h>
#include <gap.h>
#include <gap_le.h>
#include <trace_app.h>
#include <app_msg.h>
#include <bt_datatrans_app_task.h>
#include <bt_datatrans_peripheral_application.h>
#include <basic_types.h>
#include <gap_msg.h>

/** @defgroup  PERIPH_APP_TASK Peripheral App Task
    * @brief This file handles the implementation of application task related functions.
    *
    * Create App task and handle events & messages
    * @{
    */
/*============================================================================*
 *                              Macros
 *============================================================================*/
#define BT_DATATRANS_APP_TASK_PRIORITY             4         //!< Task priorities
#define BT_DATATRANS_APP_TASK_STACK_SIZE           256 * 4   //!<  Task stack size
#define BT_DATATRANS_MAX_NUMBER_OF_GAP_MESSAGE     0x20      //!<  GAP message queue size
#define BT_DATATRANS_MAX_NUMBER_OF_IO_MESSAGE      0x20      //!<  IO message queue size
#define BT_DATATRANS_MAX_NUMBER_OF_EVENT_MESSAGE   (BT_DATATRANS_MAX_NUMBER_OF_GAP_MESSAGE + BT_DATATRANS_MAX_NUMBER_OF_IO_MESSAGE)    //!< Event message queue size

/*============================================================================*
 *                              Variables
 *============================================================================*/
void *bt_datatrans_app_task_handle;   //!< APP Task handle
void *bt_datatrans_evt_queue_handle;  //!< Event queue handle
void *bt_datatrans_io_queue_handle;   //!< IO queue handle

extern T_GAP_DEV_STATE bt_datatrans_gap_dev_state;

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_param    Parameters sending to the task
 * @return       void
 */
void bt_datatrans_app_main_task(void *p_param)
{
    uint8_t event;
    os_msg_queue_create(&bt_datatrans_io_queue_handle, BT_DATATRANS_MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&bt_datatrans_evt_queue_handle, BT_DATATRANS_MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(uint8_t));

    gap_start_bt_stack(bt_datatrans_evt_queue_handle, bt_datatrans_io_queue_handle, BT_DATATRANS_MAX_NUMBER_OF_GAP_MESSAGE);

    while (true)
    {
        if (os_msg_recv(bt_datatrans_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == EVENT_IO_TO_APP)
            {
                T_IO_MSG io_msg;
                if (os_msg_recv(bt_datatrans_io_queue_handle, &io_msg, 0) == true)
                {
                    bt_datatrans_app_handle_io_msg(io_msg);
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
 * @brief  Initialize App task
 * @return void
 */
void bt_datatrans_app_task_init()
{
    os_task_create(&bt_datatrans_app_task_handle, "bt_datatrans_app", bt_datatrans_app_main_task, 0, BT_DATATRANS_APP_TASK_STACK_SIZE,
                   BT_DATATRANS_APP_TASK_PRIORITY);
}


void bt_datatrans_app_task_deinit(void)
{
	if (bt_datatrans_app_task_handle) {
		os_task_delete(bt_datatrans_app_task_handle);
	}
	if (bt_datatrans_io_queue_handle) {
		os_msg_queue_delete(bt_datatrans_io_queue_handle);
	}
	if (bt_datatrans_evt_queue_handle) {
		os_msg_queue_delete(bt_datatrans_evt_queue_handle);
	}
	bt_datatrans_io_queue_handle = NULL;
	bt_datatrans_evt_queue_handle = NULL;
	bt_datatrans_app_task_handle = NULL;

	bt_datatrans_gap_dev_state.gap_init_state = 0;
	bt_datatrans_gap_dev_state.gap_adv_sub_state = 0;
	bt_datatrans_gap_dev_state.gap_adv_state = 0;
	bt_datatrans_gap_dev_state.gap_scan_state = 0;
	bt_datatrans_gap_dev_state.gap_conn_state = 0;

}

/** @} */ /* End of group PERIPH_APP_TASK */
#endif

