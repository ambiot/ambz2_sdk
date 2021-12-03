/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      peripheral_app.c
   * @brief     This file handles BLE peripheral application routines.
   * @author    jane
   * @date      2017-06-06
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
#include <app_msg.h>
#include <os_sched.h>
#include "porting_test_app.h"
#include "app_flags.h"
#include <trace_app.h>
#if APP_TEST_RANDOM
#include "platform_utils.h"
#endif
#if APP_TEST_OSIF
#include <os_mem.h>
#endif
#if APP_TEST_FTL
#include "ftl_if.h"
#endif
#if APP_TEST_HCI_TP
#include "hci_if.h"
#endif

typedef enum
{
    APP_TC_START,
#if APP_TEST_RANDOM
    APP_TC_RANDOM,
#endif
#if APP_TEST_OSIF
    APP_TC_OSIF,
#endif
#if APP_TEST_FTL
    APP_TC_FTL,
#endif
#if APP_TEST_HCI_TP
    APP_TC_HCI_TP,
#endif
    APP_TC_END,
} T_APP_TEST_CASE;

uint8_t test_case = APP_TC_START;
uint8_t fail_count = 0;
#if APP_TEST_FTL
uint32_t ftl_test_count = APP_TEST_FTL_COUNT;
uint8_t cmd_data[20];
#endif
void app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;

    switch (msg_type)
    {
    case IO_MSG_TYPE_QDECODE:
        {
            app_run_test_case();
        }
        break;
    default:
        break;
    }
}

#if APP_TEST_HCI_TP
extern bool bte_sys_init(void);
extern unsigned char rtlbt_config[];
bool hci_if_callback(T_HCI_IF_EVT evt, bool status, uint8_t *p_buf, uint32_t len)
{
    bool result = true;
    bool ret = true;
    APP_PRINT_TRACE4("hci_if_callback: evt %d status %d, buf %p, len %u",
                     evt, status, p_buf, len);

    switch (evt)
    {
    case HCI_IF_EVT_DATA_IND:
        {
            APP_PRINT_INFO0("HCI_IF_EVT_DATA_IND");
            APP_PRINT_INFO1("Data: %b", TRACE_BINARY(len, p_buf));
            if(len == 13)
            {
                uint8_t status = p_buf[6];
                uint8_t bt_addr[6];
                if(status != 0)
                {
                    ret = false;
                    break;
                }
                memcpy(bt_addr, &(p_buf[7]), 6);
                APP_PRINT_INFO6("local bd addr: 0x%x:%x:%x:%x:%x:%x",
                            bt_addr[5], bt_addr[4], bt_addr[3],
                            bt_addr[2], bt_addr[1], bt_addr[0]);
#if 0
                if(memcmp(bt_addr, &(rtlbt_config[9]), 6) == 0)
                {
                    APP_PRINT_INFO0("APP_TC_HCI_TP: Success");
                }
                else
                {
                    ret = false;
                    APP_PRINT_ERROR0("APP_TC_HCI_TP: Check Address Failed");
                    break;
                }
#endif
            }
            else
            {
                ret = false;
                APP_PRINT_ERROR0("APP_TC_HCI_TP: Check Address Failed");
                break;
            }
            hci_if_confirm(p_buf);
            test_case++;
            app_send_msg(1);
        }
        break;

    case HCI_IF_EVT_DATA_XMIT:
        {
            APP_PRINT_INFO0("HCI_IF_EVT_DATA_XMIT");
        }
        break;

    case HCI_IF_EVT_ERROR:
        APP_PRINT_INFO0("HCI_IF_EVT_ERROR");
        break;

    case HCI_IF_EVT_OPENED:
        {
            APP_PRINT_INFO0("HCI_IF_EVT_OPENED");
            if(status == false)
            {
                ret = false;
            }
            else
            { 
                cmd_data[0] = 0x01; //read bt address
                cmd_data[1] = 0x09;
                cmd_data[2] = 0x10;
                cmd_data[3] = 0x00;
                if(hci_if_write(cmd_data, 4) == false)
                {
                    ret = false;
                }
            }
        }
        break;

    case HCI_IF_EVT_CLOSED:
        break;

    default:
        break;
    }

    if(ret == false)
    {
        APP_PRINT_ERROR0("APP_TC_HCI_TP: Failed 2");
        fail_count++;
        test_case++;
        app_send_msg(1);
    }

    return result;
}
#endif

void app_run_test_case(void)
{
    switch (test_case)
    {
    case APP_TC_START:
        {
            APP_PRINT_INFO0("APP_TC_START");
        }
        break;

#if APP_TEST_RANDOM
    case APP_TC_RANDOM:
        {
            uint32_t random_1;
            uint32_t random_2;
            uint8_t i = 0;
            APP_PRINT_INFO0("APP_TC_RANDOM");
            for(i=0;i<10;i++)
            {
                random_1 = platform_random(0);
                random_2 = platform_random(0);
                APP_PRINT_INFO2("APP_TC_RANDOM: random_1 0x%x, random_2 0x%x", random_1, random_2);
                if(random_1 == random_2)
                {
                     APP_PRINT_ERROR0("APP_TC_RANDOM: Failed");
                     fail_count++;
                     break;
                }
            }
            if(i == 10)
            {
                APP_PRINT_INFO0("APP_TC_RANDOM: Success");
            }
        }
        break;
#endif

#if APP_TEST_OSIF
    case APP_TC_OSIF:
        {
            void *p_buffer;
            uint32_t heap_size_before = os_mem_peek(1);
            uint32_t heap_size_after;
            APP_PRINT_INFO0("APP_TC_OSIF");
            APP_PRINT_INFO1("Heap size: %d", heap_size_before);
            p_buffer = os_mem_alloc(1, 1000);
            if(p_buffer != NULL)
            {
                os_mem_free(p_buffer);
                heap_size_after = os_mem_peek(1);
                if(heap_size_after == heap_size_before)
                {
                    APP_PRINT_INFO0("APP_TC_OSIF: Success");
                    break;
                }
            }
            APP_PRINT_ERROR0("APP_TC_OSIF: Failed");
            fail_count++;
        }
        break;
#endif 

#if APP_TEST_FTL
    case APP_TC_FTL:
        {
            uint8_t data[80];
            uint8_t data_load[80];
            uint32_t ret;
            APP_PRINT_INFO1("APP_TC_FTL: ftl_test_count %d", ftl_test_count);
            memset(data, ftl_test_count, 80);
            ret = ftl_save_to_storage(data, 1024, 80);
            if(ret == 0)
            {
                ftl_load_from_storage(data_load, 1024, 80);
                if(memcmp(data, data_load, 80) == 0)
                {
                    ftl_test_count--;
                    if(ftl_test_count == 0)
                    {
                        APP_PRINT_INFO0("APP_TC_FTL: Success");
                        break;
                    }
                    app_send_msg(1);
                    return;
                }
            }
            APP_PRINT_ERROR0("APP_TC_FTL: Failed");
            fail_count++;
        }
        break;
#endif
#if APP_TEST_HCI_TP       
    case APP_TC_HCI_TP:
        {
            APP_PRINT_INFO0("APP_TC_HCI_TP");
            if(bte_sys_init() == true)
            {
                if (hci_if_open(hci_if_callback) == true)
                {
                    return;
                }
            }
            APP_PRINT_ERROR0("APP_TC_HCI_TP: Failed 1");
            fail_count++;
        }
        break;
#endif
    case APP_TC_END:
        {
            APP_PRINT_INFO1("APP_TC_END: fail_count %d", fail_count);
            return;
        }
        break;
    default:
        {
            APP_PRINT_ERROR1("app_run_test_case: unknown test_case %d", test_case);
        }
        break;
    }
    test_case++;
    os_delay(100);
    app_send_msg(1);
}

