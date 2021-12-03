#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_throughput_app.h"
#include "ble_throughput_user_cmd.h"
#include "user_cmd_parse.h"
#include "profile_server.h"
#include "gap_adv.h"
#include "ble_throughput_vendor_tp_service.h"
#include "os_sched.h"

#include <ble_throughput_test_case.h>
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
#include <gap_vendor.h>
#endif

#include <ble_throughput_200.h>

extern uint8_t gSimpleProfileServiceId;
 
T_TP_TEST_PARAM g_206_tp_test_param;
TTP_PERFER_PARAM g_206_prefer_param;
bool g_206_phy_update = false;

void ble_throughput_206_tp_notification_tx_start(void)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void ble_throughput_206_tp_notification_tx_init_config(uint16_t con_interval, uint16_t slave_latency,
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

void ble_throughput_206_start_send_notification(uint8_t conn_id)
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

void ble_throughput_206_tp_notification_tx_conn_param_update_event(uint8_t conn_id)
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
        ble_throughput_206_start_send_notification(0);
    }
    else
    {
        data_uart_print("[206][TX] error: Invalid conn parameter\r\n");
        le_disconnect(0);
    }

}
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void ble_throughput_206_tp_notification_phy_update_event(uint8_t conn_id, uint16_t cause,
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

void ble_throughput_206_tp_notification_tx_tx_data_complete(uint8_t credits)
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
                g_206_tp_test_param.elapsed_time = ble_throughput_os_time_get_elapsed(g_206_tp_test_param.begin_time,
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

void ble_throughput_206_tp_notification_tx_tx_config(void *pdata)
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

void ble_throughput_206_dump_result(void)
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

void ble_throughput_206_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("Disc reason 0x%04x\r\n", reason);
    ble_throughput_206_dump_result();
}

T_TP_TEST_PARAM g_207_tp_test_param;
TTP_PERFER_PARAM g_207_prefer_param;

void ble_throughput_207_tp_rx_start(void)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void ble_throughput_207_tp_rx_init_config(uint16_t con_interval, uint16_t slave_latency,
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

void ble_throughput_207_tp_rx_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("GAP_MSG_LE_CONN_PARAM_UPDATE update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);
	data_uart_print("GAP_MSG_LE_CONN_PARAM_UPDATE update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);
}

#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void ble_throughput_207_tp_phy_update_event(uint8_t conn_id, uint16_t cause,
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

void ble_throughput_207_tp_handle_write_data(void *pdata)
{
    TTP_CALLBACK_DATA *p_simp_client_cb_data = pdata;
    APP_PRINT_INFO1("ble_throughput_207_tp_handle_write_data: length %d",
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
        g_207_tp_test_param.elapsed_time = ble_throughput_os_time_get_elapsed(g_207_tp_test_param.begin_time,
                                                               g_207_tp_test_param.end_time);
        g_207_tp_test_param.data_rate =
            g_207_tp_test_param.count * g_207_prefer_param.length * 1000 /
            (g_207_tp_test_param.elapsed_time);
        APP_PRINT_ERROR1("[207][RX]: :end time = %dms",
                         g_207_tp_test_param.end_time);
        le_disconnect(0);
    }
}


void ble_throughput_207_dump_result(void)
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

void ble_throughput_207_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("\r\nDisc reason 0x%04x\r\n", reason);
    ble_throughput_207_dump_result();
}
#endif
