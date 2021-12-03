
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_auto_test_application.h"
#include "user_cmd.h"
#include "user_cmd_parse.h"
#include "profile_server.h"
#include "gap_adv.h"
#include "vendor_tp_service.h"
#include "os_sched.h"

#include <ble_auto_test_case.h>
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
#include <gap_vendor.h>
#endif

#include <tc_common.h>
#include <tc_200.h>



#if TC_200_SUPPORT

T_TP_TEST_CONFIG g_tp_test_config;

#define TP_TEST_PARAM_COUNT         12
T_TP_TEST_PARAM g_tp_test_param[TP_TEST_PARAM_COUNT];
uint8_t g_tp_cur_test_index = 0;
uint8_t g_tp_used_test_index = 0;
uint8_t g_tp_test_start = false;


#define MAX_TEST_COUNT 4000
static uint8_t tp_test_mode;



void tc_200_tp_notification_tx_start(void)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}
uint8_t tc_tp_notification_tx_01_fixed_len_array[] = {20, 150, 200, 236, 241};
uint8_t tc_tp_notification_tx_01_fixed_len_array_len =
    sizeof(tc_tp_notification_tx_01_fixed_len_array) / sizeof(
        tc_tp_notification_tx_01_fixed_len_array[0]);
uint8_t tc_tp_notification_tx_01_fixed_len_array_cur_index = 0;

void tc_200_tp_notification_tx_init_config(uint16_t con_interval, uint16_t length,
                                           uint8_t tx_octets, bool test_drop_acl_data)
{
    g_tp_test_config.con_interval = con_interval;
    g_tp_test_config.length = length;
    g_tp_test_config.tx_octets = tx_octets;
    g_tp_test_config.test_drop_acl_data = test_drop_acl_data;

    if (con_interval != 0)
    {
        tc_200_tp_notification_tx_config_with_fixed_interval(g_tp_test_config.con_interval);
    }
    else if (length != 0)

    {
        tc_200_tp_notification_tx_config_with_fixed_length(g_tp_test_config.length);
    }
    else if (con_interval == 0 && length == 0)
    {
        tc_200_tp_notification_tx_config_with_fixed_length(
            tc_tp_notification_tx_01_fixed_len_array[tc_tp_notification_tx_01_fixed_len_array_cur_index]);
    }


}

void tc_200_tp_notification_tx_config_with_fixed_interval(uint16_t con_interval)
{
    uint8_t i = 0;
    g_tp_test_param[i].con_interval = con_interval;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = 20;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = g_tp_test_config.con_interval;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = 150;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;

    i++;

    g_tp_test_param[i].con_interval = g_tp_test_config.con_interval;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = 200;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = g_tp_test_config.con_interval;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = 236;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;


    g_tp_test_param[i].con_interval = g_tp_test_config.con_interval;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = 244;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;




    g_tp_used_test_index = i;
}

void tc_200_tp_notification_tx_config_with_fixed_length(uint16_t length)
{
    uint8_t i = 0;
    g_tp_test_param[i].con_interval = 6;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 6;
    g_tp_test_param[i].conn_slave_latency = 50;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 8;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 8;
    g_tp_test_param[i].conn_slave_latency = 50;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 10;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 10;
    g_tp_test_param[i].conn_slave_latency = 50;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 12;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 12;
    g_tp_test_param[i].conn_slave_latency = 50;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;


    g_tp_test_param[i].con_interval = 15;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 15;
    g_tp_test_param[i].conn_slave_latency = 50;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

#if 0
    g_tp_test_param[i].con_interval = 18;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 19;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;
#endif

    g_tp_test_param[i].con_interval = 20;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 20;
    g_tp_test_param[i].conn_slave_latency = 50;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;

#if 0
    i++;

    g_tp_test_param[i].con_interval = 24;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
    i++;

    g_tp_test_param[i].con_interval = 80;
    g_tp_test_param[i].conn_slave_latency = 0;
    g_tp_test_param[i].conn_supervision_timeout = 1000;
    g_tp_test_param[i].length = length;
    g_tp_test_param[i].count = MAX_TEST_COUNT;
    g_tp_test_param[i].count_remain = g_tp_test_param[i].count;
    g_tp_test_param[i].initial_value = 0;
#endif


    g_tp_used_test_index = i;
}

void tc_200_tp_notification_tx_init_default_param(uint8_t conn_id)
{
    for (uint8_t i = 0; i <= g_tp_used_test_index; i++)
    {
        g_tp_test_param[g_tp_cur_test_index].conn_id = conn_id;
    }

}

void tc_200_tp_notification_tx_change_tx_octets(uint8_t conn_id, uint16_t tx_octets)
{
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    le_set_data_len(conn_id, tx_octets, 0x0848);
#endif
}

#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void tc_200_tp_notification_phy_update_event(uint8_t conn_id, uint16_t cause,
                                             T_GAP_PHYS_TYPE tx_phy, T_GAP_PHYS_TYPE rx_phy)
{
    if (cause != 0)
    {
        if (tp_test_mode == 4 || tp_test_mode == 5)
        {
            le_set_phy(g_tp_test_param[g_tp_cur_test_index].conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S2
                      );
        }
        else if (tp_test_mode == 6 || tp_test_mode == 7)
        {
            le_set_phy(g_tp_test_param[g_tp_cur_test_index].conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S8
                      );
        }
    }
}
#endif

void tc_200_tp_notification_tx_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("GAP_MSG_LE_CONN_PARAM_UPDATE update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (g_tp_test_param[g_tp_cur_test_index].con_interval ==  con_interval &&
        g_tp_test_param[g_tp_cur_test_index].conn_slave_latency ==  conn_slave_latency &&
        g_tp_test_param[g_tp_cur_test_index].conn_supervision_timeout == conn_supervision_timeout)
    {
        if (g_tp_cur_test_index == 0 && tc_tp_notification_tx_01_fixed_len_array_cur_index == 0)
        {
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
            if (tp_test_mode == 4 || tp_test_mode == 5)
            {
                le_set_phy(g_tp_test_param[g_tp_cur_test_index].conn_id,
                           0,
                           GAP_PHYS_PREFER_CODED_BIT,
                           GAP_PHYS_PREFER_CODED_BIT,
                           GAP_PHYS_OPTIONS_CODED_PREFER_S2
                          );
            }
            else if (tp_test_mode == 6 || tp_test_mode == 7)
            {
                le_set_phy(g_tp_test_param[g_tp_cur_test_index].conn_id,
                           0,
                           GAP_PHYS_PREFER_CODED_BIT,
                           GAP_PHYS_PREFER_CODED_BIT,
                           GAP_PHYS_OPTIONS_CODED_PREFER_S8
                          );
            }
#endif
        }

        g_tp_test_param[g_tp_cur_test_index].begin_time = os_sys_time_get();
        APP_PRINT_INFO1("tp test:notification:begin:begin time = %dms",
                        g_tp_test_param[g_tp_cur_test_index].begin_time);

        uint8_t credits = 10;
        while (credits)
        {
            if (g_tp_test_param[g_tp_cur_test_index].count_remain)
            {
                uint8_t value[250];
                memset(value, g_tp_test_param[g_tp_cur_test_index].initial_value,
                       g_tp_test_param[g_tp_cur_test_index].length);
                if (vendor_tp_service_v1_notification(conn_id, gSimpleProfileServiceId, value,
                                                      g_tp_test_param[g_tp_cur_test_index].length))
                {
                    credits--;
                    g_tp_test_param[g_tp_cur_test_index].initial_value++;
                    g_tp_test_param[g_tp_cur_test_index].count_remain--;
                }
                else
                {
                    APP_PRINT_ERROR0("profile callback PROFILE_EVT_SEND_DATA_COMPLETE, send failed");
                    break;
                }

            }
            else
            {
                break;
            }
        }
    }

}
static bool flag200 = false;
void tc_200_tp_notification_tx_tx_data_complete(uint8_t credits)
{
    while (credits)
    {
        if (g_tp_test_param[g_tp_cur_test_index].count_remain)
        {
            uint8_t value[250];
            memset(value, g_tp_test_param[g_tp_cur_test_index].initial_value,
                   g_tp_test_param[g_tp_cur_test_index].length);
            if (vendor_tp_service_v1_notification(g_tp_test_param[g_tp_cur_test_index].conn_id,
                                                  gSimpleProfileServiceId, value, g_tp_test_param[g_tp_cur_test_index].length))
            {
                credits--;
                g_tp_test_param[g_tp_cur_test_index].initial_value++;
                g_tp_test_param[g_tp_cur_test_index].count_remain--;
                if (g_tp_test_config.test_drop_acl_data)
                {
                    if ((g_tp_test_param[g_tp_cur_test_index].count_remain % 100) == 0)
                    {
#if 0
                        le_vendor_drop_acl_data(g_tp_test_param[g_tp_cur_test_index].conn_id, 0, 0, 0);
#endif
                    }
                }
            }
            else
            {
                APP_PRINT_ERROR0("profile callback PROFILE_EVT_SEND_DATA_COMPLETE, send failed");
                break;
            }
        }
        else
        {
            if (credits == 10)
            {
                g_tp_test_param[g_tp_cur_test_index].end_time = os_sys_time_get();
                uint32_t elapsed_time = os_time_get_elapsed(g_tp_test_param[g_tp_cur_test_index].begin_time,
                                                            g_tp_test_param[g_tp_cur_test_index].end_time);
                uint32_t data_rate =
                    g_tp_test_param[g_tp_cur_test_index].count * g_tp_test_param[g_tp_cur_test_index].length * 1000 /
                    (elapsed_time);

                APP_PRINT_INFO8("tp test:notification:end:conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                                g_tp_test_param[g_tp_cur_test_index].con_interval,
                                g_tp_test_param[g_tp_cur_test_index].conn_slave_latency,
                                g_tp_test_param[g_tp_cur_test_index].length,
                                g_tp_test_param[g_tp_cur_test_index].count,
                                g_tp_test_param[g_tp_cur_test_index].begin_time,
                                g_tp_test_param[g_tp_cur_test_index].end_time,
                                elapsed_time,
                                data_rate);
                if (false == flag200)
                {
                    data_uart_print(" conn_interval,     latency,     length,     data rate(Bytes/s)\r\n");
                    flag200 = true;
                }
                data_uart_print("     %d,              %d,              %d,                %d\r\n",
                                g_tp_test_param[g_tp_cur_test_index].con_interval,
                                g_tp_test_param[g_tp_cur_test_index].conn_slave_latency,
                                g_tp_test_param[g_tp_cur_test_index].length,
                                data_rate);
                if (g_tp_cur_test_index < g_tp_used_test_index)
                {
                    g_tp_cur_test_index++;
                    APP_PRINT_INFO2("g_tp_cur_test_index = %d, interval = %d", g_tp_cur_test_index,
                                    g_tp_test_param[g_tp_cur_test_index].con_interval);
                    le_update_conn_param(g_tp_test_param[g_tp_cur_test_index].conn_id,
                                         g_tp_test_param[g_tp_cur_test_index].con_interval,
                                         g_tp_test_param[g_tp_cur_test_index].con_interval,
                                         g_tp_test_param[g_tp_cur_test_index].conn_slave_latency,
                                         g_tp_test_param[g_tp_cur_test_index].conn_supervision_timeout,
                                         2 * (g_tp_test_param[g_tp_cur_test_index].con_interval - 1),
                                         2 * (g_tp_test_param[g_tp_cur_test_index].con_interval - 1)
                                        );

                }
                else
                {
                    g_tp_cur_test_index = 0;
                    g_tp_used_test_index = 0;
                    g_tp_test_start = true;
                    if (tc_tp_notification_tx_01_fixed_len_array_cur_index <
                        tc_tp_notification_tx_01_fixed_len_array_len - 1)
                    {
                        tc_tp_notification_tx_01_fixed_len_array_cur_index++;

                        APP_PRINT_INFO2("11tc_tp_notification_tx_01_fixed_len_array_cur_index = %d, tc_tp_notification_tx_01_fixed_len_array_len = %d",
                                        tc_tp_notification_tx_01_fixed_len_array_cur_index,
                                        tc_tp_notification_tx_01_fixed_len_array_len);

                        tc_200_tp_notification_tx_init_config(0,
                                                              tc_tp_notification_tx_01_fixed_len_array[tc_tp_notification_tx_01_fixed_len_array_cur_index],
                                                              251, g_tp_test_config.test_drop_acl_data);

                        le_update_conn_param(g_tp_test_param[g_tp_cur_test_index].conn_id,
                                             g_tp_test_param[g_tp_cur_test_index].con_interval,
                                             g_tp_test_param[g_tp_cur_test_index].con_interval,
                                             g_tp_test_param[g_tp_cur_test_index].conn_slave_latency,
                                             g_tp_test_param[g_tp_cur_test_index].conn_supervision_timeout,
                                             2 * (g_tp_test_param[g_tp_cur_test_index].con_interval - 1),
                                             2 * (g_tp_test_param[g_tp_cur_test_index].con_interval - 1)
                                            );
                    }
                    else
                    {
                        //all test end here
                        APP_PRINT_INFO2("tc_tp_notification_tx_01_fixed_len_array_cur_index = %d, tc_tp_notification_tx_01_fixed_len_array_len = %d",
                                        tc_tp_notification_tx_01_fixed_len_array_cur_index,
                                        tc_tp_notification_tx_01_fixed_len_array_len);
                        APP_PRINT_INFO0("tp test:notification:end: start next case\r\n");
                        data_uart_print("tp test:notification:end: start next case\a\r\n");
                        tc_tp_notification_tx_01_fixed_len_array_cur_index = 0;
                        tc_200_tp_notification_tx_init_config(0,
                                                              tc_tp_notification_tx_01_fixed_len_array[tc_tp_notification_tx_01_fixed_len_array_cur_index],
                                                              251, g_tp_test_config.test_drop_acl_data);

                    }


                }
            }
            break;
        }
    }
}

void tc_200_tp_notification_tx_tx_config(void *pdata)
{
    TTP_CALLBACK_DATA *p_simp_client_cb_data = pdata;

    switch (p_simp_client_cb_data->msg_data.write.opcode)
    {
    case VENDOR_TP_OP_CONFIG_NOTIFY_PARAM1:
        {
            //ThroughputTestSize = p_simp_client_cb_data->msg_data.write.u.notify_param.count;
            //ThroughputTestNum = p_simp_client_cb_data->msg_data.write.u.notify_param.length;

            //APP_PRINT_INFO2("tp test:notification:config:count = %d, length = %d", ThroughputTestSize, ThroughputTestNum);

            //ThroughputTestSize = 20;
            //ThroughputTestNum = 10000;
            //ThroughputTestValue = 0;
            tc_200_tp_notification_tx_change_tx_octets(g_tp_test_param[g_tp_cur_test_index].conn_id, 27);
        }
        break;
    case VENDOR_TP_OP_CONFIG_NOTIFY_PARAM2:
        {
            tc_200_tp_notification_tx_change_tx_octets(g_tp_test_param[g_tp_cur_test_index].conn_id, 251);
        }
        break;

    case VENDOR_TP_OP_NOTIFY_START_TEST:
        {
            g_tp_test_start = true;
            g_tp_cur_test_index = 0;
            tc_tp_notification_tx_01_fixed_len_array_cur_index = 0;
            tp_test_mode = p_simp_client_cb_data->msg_data.write.u.mode;
            le_update_conn_param(g_tp_test_param[g_tp_cur_test_index].conn_id,
                                 g_tp_test_param[g_tp_cur_test_index].con_interval,
                                 g_tp_test_param[g_tp_cur_test_index].con_interval,
                                 g_tp_test_param[g_tp_cur_test_index].conn_slave_latency,
                                 g_tp_test_param[g_tp_cur_test_index].conn_supervision_timeout,
                                 2 * (g_tp_test_param[g_tp_cur_test_index].con_interval - 1),
                                 2 * (g_tp_test_param[g_tp_cur_test_index].con_interval - 1));
        }
        break;
    default:
        break;
    }

}

#endif

#if TC_206_SUPPORT
T_TP_TEST_PARAM g_206_tp_test_param;
TTP_PERFER_PARAM g_206_prefer_param;
bool g_206_phy_update = false;

void tc_206_tp_notification_tx_start(void)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_206_tp_notification_tx_init_config(uint16_t con_interval, uint16_t slave_latency,
                                           uint16_t length, uint8_t mode, uint32_t max_count, uint8_t data_check)
{
    memset(&g_206_tp_test_param, 0, sizeof(g_206_tp_test_param));
    g_206_prefer_param.con_interval = con_interval;
    g_206_prefer_param.conn_slave_latency = slave_latency;
    g_206_prefer_param.conn_supervision_timeout = 1000;
    g_206_prefer_param.count = max_count;
    g_206_prefer_param.length = length;
    g_206_prefer_param.mode = mode;
    g_206_prefer_param.initial_value = 0;
    g_206_prefer_param.data_check = data_check;

    g_206_tp_test_param.length = length;
    g_206_tp_test_param.count = max_count;
    g_206_tp_test_param.count_remain = max_count;
    g_206_tp_test_param.initial_value = 0;

    g_206_phy_update = false;

    vendor_tp_service_config_param(g_206_prefer_param);
}

void tc_206_start_send_notification(uint8_t conn_id)
{
    uint8_t credits = 10;
    g_206_tp_test_param.begin_time = os_sys_time_get();
    APP_PRINT_ERROR1("[206][TX]:begin time = %dms",
                     g_206_tp_test_param.begin_time);

    while (credits)
    {
        if (g_206_tp_test_param.count_remain)
        {
            uint8_t value[250];
            memset(value, g_206_tp_test_param.initial_value,
                   g_206_tp_test_param.length);
            if (vendor_tp_service_v1_notification(conn_id, gSimpleProfileServiceId, value,
                                                  g_206_tp_test_param.length))
            {
                credits--;
                g_206_tp_test_param.initial_value++;
                g_206_tp_test_param.count_remain--;
            }
            else
            {
                APP_PRINT_INFO0("profile callback PROFILE_EVT_SEND_DATA_COMPLETE, send failed");
                break;
            }

        }
        else
        {
            break;
        }
    }
}

void tc_206_tp_notification_tx_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("GAP_MSG_LE_CONN_PARAM_UPDATE update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (g_206_prefer_param.con_interval ==  con_interval &&
        g_206_prefer_param.conn_slave_latency ==  conn_slave_latency &&
        g_206_prefer_param.conn_supervision_timeout == conn_supervision_timeout)
    {
        tc_206_start_send_notification(0);
    }
    else
    {
        data_uart_print("[206][TX] error: Invalid conn parameter\r\n");
        le_disconnect(0);
    }

}
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void tc_206_tp_notification_phy_update_event(uint8_t conn_id, uint16_t cause,
                                             T_GAP_PHYS_TYPE tx_phy, T_GAP_PHYS_TYPE rx_phy)
{
    if (g_206_phy_update)
    {
        return;
    }
    else
    {
        g_206_phy_update = true;
    }
    if (cause == 0)
    {
        if (g_206_prefer_param.mode == 4 || g_206_prefer_param.mode == 5)
        {
            le_set_phy(conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S2
                      );
        }
        else if (g_206_prefer_param.mode == 6 || g_206_prefer_param.mode == 7)
        {
            le_set_phy(conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S8
                      );
        }
    }
}
#endif

void tc_206_tp_notification_tx_tx_data_complete(uint8_t credits)
{
    while (credits)
    {
        if (g_206_tp_test_param.count_remain)
        {
            uint8_t value[250];
            memset(value, g_206_tp_test_param.initial_value,
                   g_206_tp_test_param.length);
            if (vendor_tp_service_v1_notification(0, gSimpleProfileServiceId, value,
                                                  g_206_tp_test_param.length))
            {
                credits--;
                g_206_tp_test_param.initial_value++;
                g_206_tp_test_param.count_remain--;
            }
            else
            {
                APP_PRINT_ERROR0("profile callback PROFILE_EVT_SEND_DATA_COMPLETE, send failed");
                break;
            }
        }
        else
        {
            if (credits == 10)
            {
                g_206_tp_test_param.end_time = os_sys_time_get();
                g_206_tp_test_param.elapsed_time = os_time_get_elapsed(g_206_tp_test_param.begin_time,
                                                                       g_206_tp_test_param.end_time);
                g_206_tp_test_param.data_rate =
                    g_206_tp_test_param.count * g_206_tp_test_param.length * 1000 /
                    (g_206_tp_test_param.elapsed_time);
                APP_PRINT_ERROR1("[206][TX]:end time = %dms",
                                 g_206_tp_test_param.end_time);
            }
            break;
        }
    }
}

void tc_206_tp_notification_tx_tx_config(void *pdata)
{
    TTP_CALLBACK_DATA *p_simp_client_cb_data = pdata;

    switch (p_simp_client_cb_data->msg_data.write.opcode)
    {
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case VENDOR_TP_OP_CONFIG_NOTIFY_PARAM1:
        {
            le_set_data_len(0, 27, 0x0848);
        }
        break;
    case VENDOR_TP_OP_CONFIG_NOTIFY_PARAM2:
        {
            le_set_data_len(0, 251, 0x0848);
        }
        break;
#endif
    case VENDOR_TP_OP_NOTIFY_START_TEST:
        break;

    default:
        break;
    }

}

void tc_206_dump_result(void)
{
    APP_PRINT_ERROR8("[206][TX]: conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                     g_206_prefer_param.con_interval,
                     g_206_prefer_param.conn_slave_latency,
                     g_206_prefer_param.length,
                     g_206_tp_test_param.count,
                     g_206_tp_test_param.begin_time,
                     g_206_tp_test_param.end_time,
                     g_206_tp_test_param.elapsed_time,
                     g_206_tp_test_param.data_rate);
    data_uart_print("[206][TX]: conn_interval %d, latency %d, length %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                    g_206_prefer_param.con_interval,
                    g_206_prefer_param.conn_slave_latency,
                    g_206_prefer_param.length,
                    g_206_tp_test_param.count,
                    g_206_tp_test_param.begin_time,
                    g_206_tp_test_param.end_time,
                    g_206_tp_test_param.elapsed_time,
                    g_206_tp_test_param.data_rate);
    APP_PRINT_ERROR2("[206][TX]: count %d count_remain %d",
                     g_206_prefer_param.count, g_206_tp_test_param.count_remain);

    data_uart_print("[206][TX]: count %d count_remain %d\r\n",
                    g_206_prefer_param.count, g_206_tp_test_param.count_remain);
}

void tc_206_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("Disc reason 0x%04x\r\n", reason);
    tc_206_dump_result();
}
#endif

#if TC_207_SUPPORT
T_TP_TEST_PARAM g_207_tp_test_param;
TTP_PERFER_PARAM g_207_prefer_param;

void tc_207_tp_rx_start(void)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_207_tp_rx_init_config(uint16_t con_interval, uint16_t slave_latency,
                              uint16_t length, uint8_t mode, uint32_t max_count, uint8_t data_check)
{
    memset(&g_207_tp_test_param, 0, sizeof(g_207_tp_test_param));
    g_207_prefer_param.con_interval = con_interval;
    g_207_prefer_param.conn_slave_latency = slave_latency;
    g_207_prefer_param.conn_supervision_timeout = 1000;
    g_207_prefer_param.count = max_count;
    g_207_prefer_param.length = length;
    g_207_prefer_param.mode = mode;
    g_207_prefer_param.initial_value = 0;
    g_207_prefer_param.data_check = data_check;

    g_207_tp_test_param.length = length;
    g_207_tp_test_param.count = 0;
    g_207_tp_test_param.count_remain = 0;
    g_207_tp_test_param.initial_value = 0;

    vendor_tp_service_config_param(g_207_prefer_param);
}

void tc_207_tp_rx_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("GAP_MSG_LE_CONN_PARAM_UPDATE update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);
}

#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void tc_207_tp_phy_update_event(uint8_t conn_id, uint16_t cause,
                                T_GAP_PHYS_TYPE tx_phy, T_GAP_PHYS_TYPE rx_phy)
{
    if (cause == 0)
    {
        if (g_207_prefer_param.mode == 4 || g_207_prefer_param.mode == 5)
        {
            le_set_phy(conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S2
                      );
        }
        else if (g_207_prefer_param.mode == 6 || g_207_prefer_param.mode == 7)
        {
            le_set_phy(conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S8
                      );
        }
    }
}
#endif

void tc_207_tp_handle_write_data(void *pdata)
{
    TTP_CALLBACK_DATA *p_simp_client_cb_data = pdata;
    APP_PRINT_INFO1("tc_207_tp_handle_write_data: length %d",
                    p_simp_client_cb_data->msg_data.write.u.write_data.length);
    if (g_207_prefer_param.length == p_simp_client_cb_data->msg_data.write.u.write_data.length)
    {
        if (g_207_tp_test_param.count == 0)
        {
            g_207_tp_test_param.begin_time = os_sys_time_get();
            APP_PRINT_ERROR1("[207][RX]: :begin time = %dms",
                             g_207_tp_test_param.begin_time);
        }
        if (g_207_prefer_param.data_check)
        {
            uint16_t i;
            uint8_t *p_value = p_simp_client_cb_data->msg_data.write.u.write_data.p_value;
            for (i = 0; i < g_207_prefer_param.length; i++)
            {
                if (p_value[i] != g_207_tp_test_param.initial_value)
                {
                    APP_PRINT_ERROR4("[207][RX]: data check failed: p_value[%d] 0x%x, initial_value %d, count %d",
                                     i, p_value[i],
                                     g_207_tp_test_param.initial_value,
                                     g_207_tp_test_param.count);
                    data_uart_print("[207][RX]: data check failed\r\n");
                    le_disconnect(0);
                    return;
                }
            }
        }
    }
    else
    {
        APP_PRINT_ERROR1("[207][RX]: Len check failed: length %d",
                         p_simp_client_cb_data->msg_data.write.u.write_data.length);
        data_uart_print("[207][RX]: Len check failed: length %d\r\n",
                        p_simp_client_cb_data->msg_data.write.u.write_data.length);
        le_disconnect(0);
        return;
    }
    g_207_tp_test_param.count++;
    g_207_tp_test_param.initial_value++;
    if (g_207_tp_test_param.count == g_207_prefer_param.count)
    {
        g_207_tp_test_param.end_time = os_sys_time_get();
        g_207_tp_test_param.elapsed_time = os_time_get_elapsed(g_207_tp_test_param.begin_time,
                                                               g_207_tp_test_param.end_time);
        g_207_tp_test_param.data_rate =
            g_207_tp_test_param.count * g_207_prefer_param.length * 1000 /
            (g_207_tp_test_param.elapsed_time);
        APP_PRINT_ERROR1("[207][RX]: :end time = %dms",
                         g_207_tp_test_param.end_time);
        le_disconnect(0);
    }
}

void tc_207_dump_result(void)
{
    APP_PRINT_ERROR7("[207][RX]: conn_interval = %d,conn_latency = %d, length = %d, begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %d Bytes/s",
                     g_207_prefer_param.con_interval,
                     g_207_prefer_param.conn_slave_latency,
                     g_207_prefer_param.length,
                     g_207_tp_test_param.begin_time,
                     g_207_tp_test_param.end_time,
                     g_207_tp_test_param.elapsed_time,
                     g_207_tp_test_param.data_rate);
    data_uart_print("[207][RX]: conn_interval %d, latency %d, length %d, begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                    g_207_prefer_param.con_interval,
                    g_207_prefer_param.conn_slave_latency,
                    g_207_prefer_param.length,
                    g_207_tp_test_param.begin_time,
                    g_207_tp_test_param.end_time,
                    g_207_tp_test_param.elapsed_time,
                    g_207_tp_test_param.data_rate);
    APP_PRINT_ERROR2("[207][RX]: count %d rx_count %d",
                     g_207_prefer_param.count, g_207_tp_test_param.count);

    data_uart_print("[207][RX]: count %d rx_count %d\r\n",
                    g_207_prefer_param.count, g_207_tp_test_param.count);
}

void tc_207_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("Disc reason 0x%04x\r\n", reason);
    tc_207_dump_result();
}
#endif

#if TC_208_SUPPORT
typedef struct
{
    uint8_t  tx_initial_value;
    uint32_t tx_count;
    uint32_t tx_count_remain;

    uint32_t tx_begin_time;
    uint32_t tx_end_time;
    uint32_t tx_elapsed_time;
    uint32_t tx_data_rate;

    uint32_t rx_count;
    uint8_t  rx_initial_value;
    uint32_t rx_begin_time;
    uint32_t rx_end_time;
    uint32_t rx_elapsed_time;
    uint32_t rx_data_rate;
} T_TP_208_PARAM;
T_TP_208_PARAM g_208_tp_test_param;
TTP_PERFER_PARAM g_208_prefer_param;

void tc_208_tp_trx_start(void)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_208_tp_trx_init_config(uint16_t con_interval, uint16_t slave_latency,
                               uint16_t length, uint8_t mode, uint32_t max_count, uint8_t data_check)
{
    memset(&g_208_tp_test_param, 0, sizeof(g_208_tp_test_param));
    g_208_prefer_param.con_interval = con_interval;
    g_208_prefer_param.conn_slave_latency = slave_latency;
    g_208_prefer_param.conn_supervision_timeout = 1000;
    g_208_prefer_param.count = max_count;
    g_208_prefer_param.length = length;
    g_208_prefer_param.mode = mode;
    g_208_prefer_param.initial_value = 0;
    g_208_prefer_param.data_check = data_check;

    g_208_tp_test_param.tx_count = 0;
    g_208_tp_test_param.tx_count_remain = max_count;
    g_208_tp_test_param.tx_initial_value = 0;
    g_208_tp_test_param.rx_count = 0;
    g_208_tp_test_param.rx_initial_value = 0;
    vendor_tp_service_config_param(g_208_prefer_param);
}

void tc_208_start_send_notification(uint8_t conn_id)
{
    uint8_t credits = 10;
    g_208_tp_test_param.tx_begin_time = os_sys_time_get();
    APP_PRINT_ERROR1("[208]:tx begin time = %dms",
                     g_208_tp_test_param.tx_begin_time);

    while (credits)
    {
        if (g_208_tp_test_param.tx_count_remain)
        {
            uint8_t value[250];
            memset(value, g_208_tp_test_param.tx_initial_value,
                   g_208_prefer_param.length);
            if (vendor_tp_service_v1_notification(conn_id, gSimpleProfileServiceId, value,
                                                  g_208_prefer_param.length))
            {
                credits--;
                g_208_tp_test_param.tx_initial_value++;
                g_208_tp_test_param.tx_count_remain--;
            }
            else
            {
                APP_PRINT_ERROR0("profile callback PROFILE_EVT_SEND_DATA_COMPLETE, send failed");
                break;
            }

        }
        else
        {
            break;
        }
    }
}

void tc_208_tp_notification_tx_data_complete(uint8_t credits)
{
    while (credits)
    {
        if (g_208_tp_test_param.tx_count_remain)
        {
            uint8_t value[250];
            memset(value, g_208_tp_test_param.tx_initial_value,
                   g_208_prefer_param.length);
            if (vendor_tp_service_v1_notification(0, gSimpleProfileServiceId, value,
                                                  g_208_prefer_param.length))
            {
                credits--;
                g_208_tp_test_param.tx_initial_value++;
                g_208_tp_test_param.tx_count_remain--;
            }
            else
            {
                APP_PRINT_ERROR0("profile callback PROFILE_EVT_SEND_DATA_COMPLETE, send failed");
                break;
            }
        }
        else
        {
            if (credits == 10)
            {
                g_208_tp_test_param.tx_end_time = os_sys_time_get();
                g_208_tp_test_param.tx_elapsed_time = os_time_get_elapsed(g_208_tp_test_param.tx_begin_time,
                                                                          g_208_tp_test_param.tx_end_time);
                g_208_tp_test_param.tx_data_rate =
                    g_208_prefer_param.count * g_208_prefer_param.length * 1000 /
                    (g_208_tp_test_param.tx_elapsed_time);
                APP_PRINT_ERROR1("[208]:tx end time = %dms",
                                 g_208_tp_test_param.tx_end_time);

                tc_208_dump_tx_result();
                //le_disconnect(0);
            }
            break;
        }
    }
}

void tc_208_tp_trx_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("GAP_MSG_LE_CONN_PARAM_UPDATE update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);
    if (g_208_prefer_param.con_interval ==  con_interval &&
        g_208_prefer_param.conn_slave_latency ==  conn_slave_latency &&
        g_208_prefer_param.conn_supervision_timeout == conn_supervision_timeout)
    {
        tc_208_start_send_notification(0);
    }
    else
    {
        data_uart_print("[208] error: Invalid conn parameter\r\n");
        le_disconnect(0);
    }
}

#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void tc_208_tp_phy_update_event(uint8_t conn_id, uint16_t cause,
                                T_GAP_PHYS_TYPE tx_phy, T_GAP_PHYS_TYPE rx_phy)
{
    if (cause == 0)
    {
        if (g_208_prefer_param.mode == 4 || g_208_prefer_param.mode == 5)
        {
            le_set_phy(conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S2
                      );
        }
        else if (g_208_prefer_param.mode == 6 || g_208_prefer_param.mode == 7)
        {
            le_set_phy(conn_id,
                       0,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_PREFER_CODED_BIT,
                       GAP_PHYS_OPTIONS_CODED_PREFER_S8
                      );
        }
    }
}
#endif

void tc_208_tp_notification_tx_tx_config(void *pdata)
{
    TTP_CALLBACK_DATA *p_simp_client_cb_data = pdata;

    switch (p_simp_client_cb_data->msg_data.write.opcode)
    {
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case VENDOR_TP_OP_CONFIG_NOTIFY_PARAM1:
        {
            le_set_data_len(0, 27, 0x0848);
        }
        break;
    case VENDOR_TP_OP_CONFIG_NOTIFY_PARAM2:
        {
            le_set_data_len(0, 251, 0x0848);
        }
        break;
#endif
    case VENDOR_TP_OP_NOTIFY_START_TEST:
        break;

    default:
        break;
    }

}

void tc_208_tp_handle_write_data(void *pdata)
{
    TTP_CALLBACK_DATA *p_simp_client_cb_data = pdata;
    APP_PRINT_INFO1("tc_208_tp_handle_write_data: length %d",
                    p_simp_client_cb_data->msg_data.write.u.write_data.length);
    if (g_208_prefer_param.length == p_simp_client_cb_data->msg_data.write.u.write_data.length)
    {
        if (g_208_tp_test_param.rx_count == 0)
        {
            g_208_tp_test_param.rx_begin_time = os_sys_time_get();
            APP_PRINT_ERROR1("[208]: rx begin time = %dms",
                             g_208_tp_test_param.rx_begin_time);
        }
        if (g_208_prefer_param.data_check)
        {
            uint16_t i;
            uint8_t *p_value = p_simp_client_cb_data->msg_data.write.u.write_data.p_value;
            for (i = 0; i < g_208_prefer_param.length; i++)
            {
                if (p_value[i] != g_208_tp_test_param.rx_initial_value)
                {
                    APP_PRINT_ERROR4("[208][RX]: data check failed: p_value[%d] 0x%x, initial_value %d, count %d",
                                     i, p_value[i],
                                     g_208_tp_test_param.rx_initial_value,
                                     g_208_tp_test_param.rx_count);
                    data_uart_print("[208][RX]: data check failed\r\n");
                    le_disconnect(0);
                    return;
                }
            }
        }
    }
    else
    {
        APP_PRINT_ERROR1("[208][RX]: Len check failed: length %d",
                         p_simp_client_cb_data->msg_data.write.u.write_data.length);
        data_uart_print("[208][RX]: Len check failed: length %d\r\n",
                        p_simp_client_cb_data->msg_data.write.u.write_data.length);
        le_disconnect(0);
        return;
    }
    g_208_tp_test_param.rx_count++;
    g_208_tp_test_param.rx_initial_value++;
    if (g_208_tp_test_param.rx_count == g_208_prefer_param.count)
    {
        g_208_tp_test_param.rx_end_time = os_sys_time_get();
        g_208_tp_test_param.rx_elapsed_time = os_time_get_elapsed(g_208_tp_test_param.rx_begin_time,
                                                                  g_208_tp_test_param.rx_end_time);
        g_208_tp_test_param.rx_data_rate =
            g_208_tp_test_param.rx_count * g_208_prefer_param.length * 1000 /
            (g_208_tp_test_param.rx_elapsed_time);
        APP_PRINT_ERROR1("[208]:rx end time = %dms",
                         g_208_tp_test_param.rx_end_time);

        tc_208_dump_rx_result();
    }
}

void tc_208_dump_tx_result(void)
{
    APP_PRINT_ERROR8("[208][TX]: conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                     g_208_prefer_param.con_interval,
                     g_208_prefer_param.conn_slave_latency,
                     g_208_prefer_param.length,
                     g_208_prefer_param.count,
                     g_208_tp_test_param.tx_begin_time,
                     g_208_tp_test_param.tx_end_time,
                     g_208_tp_test_param.tx_elapsed_time,
                     g_208_tp_test_param.tx_data_rate);
    data_uart_print("[208][TX]: conn_interval %d, latency %d, length %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                    g_208_prefer_param.con_interval,
                    g_208_prefer_param.conn_slave_latency,
                    g_208_prefer_param.length,
                    g_208_prefer_param.count,
                    g_208_tp_test_param.tx_begin_time,
                    g_208_tp_test_param.tx_end_time,
                    g_208_tp_test_param.tx_elapsed_time,
                    g_208_tp_test_param.tx_data_rate);
    APP_PRINT_ERROR2("[208][TX]: count %d tx_count_remain %d",
                     g_208_prefer_param.count, g_208_tp_test_param.tx_count_remain);

    data_uart_print("[208][TX]: count %d tx_count_remain %d\r\n",
                    g_208_prefer_param.count, g_208_tp_test_param.tx_count_remain);
}

void tc_208_dump_rx_result(void)
{
    APP_PRINT_ERROR8("[208][RX]: conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                     g_208_prefer_param.con_interval,
                     g_208_prefer_param.conn_slave_latency,
                     g_208_prefer_param.length,
                     g_208_prefer_param.count,
                     g_208_tp_test_param.rx_begin_time,
                     g_208_tp_test_param.rx_end_time,
                     g_208_tp_test_param.rx_elapsed_time,
                     g_208_tp_test_param.rx_data_rate);
    data_uart_print("[208][RX]: conn_interval %d, latency %d, length %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                    g_208_prefer_param.con_interval,
                    g_208_prefer_param.conn_slave_latency,
                    g_208_prefer_param.length,
                    g_208_prefer_param.count,
                    g_208_tp_test_param.rx_begin_time,
                    g_208_tp_test_param.rx_end_time,
                    g_208_tp_test_param.rx_elapsed_time,
                    g_208_tp_test_param.rx_data_rate);
    APP_PRINT_ERROR2("[208][RX]: count %d rx_count %d",
                     g_208_prefer_param.count, g_208_tp_test_param.rx_count);

    data_uart_print("[208][RX]: count %d rx_count %d\r\n",
                    g_208_prefer_param.count, g_208_tp_test_param.rx_count);
}

void tc_208_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("Disc reason 0x%04x\r\n", reason);
    tc_208_dump_tx_result();
    tc_208_dump_rx_result();
}
#endif

