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
#include <os_msg.h>
#include <os_task.h>
#include <app_task.h>
#include <app_msg.h>
#include <user_cmd.h>
#include <gap_test_app.h>
#include <trace_app.h>

/*============================================================================*
 *                              Macros
 *============================================================================*/
//! Task priorities
#define TEST_TASK_PRIORITY             1
//!Task stack size
#define TEST_TASK_STACK_SIZE           256 * 4

/*============================================================================*
 *                              Variables
 *============================================================================*/
void *test_task_handle;   //!< APP Task handle

/*============================================================================*
 *                              Functions
 *============================================================================*/
void test_main_task(void *p_param);

/**
 * @brief  Initialize App task
 * @return void
 */
void test_task_init()
{
    if (os_task_create(&test_task_handle, "test app", test_main_task, 0, TEST_TASK_STACK_SIZE,
                       TEST_TASK_PRIORITY))
    {
        APP_PRINT_INFO0("test_task_init: success");
    }
    else
    {
        APP_PRINT_INFO0("test_task_init: failed");
    }
}

void test_task_deinit()
{
    if (os_task_delete(test_task_handle))
    {
        APP_PRINT_INFO0("test_task_deinit: success");
    }
    else
    {
        APP_PRINT_INFO0("test_task_deinit: failed");
    }
    //test_task_handle = NULL;
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_params    Parameters sending to the task
 * @return       void
 */
void test_main_task(void *p_param)
{
    while (true)
    {
        //os_delay(1000);
        APP_PRINT_ERROR0("test task");
    }
}

