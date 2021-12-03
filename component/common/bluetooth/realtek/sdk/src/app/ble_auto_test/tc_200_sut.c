
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


#include "os_sched.h"
#include <os_mem.h>
#include <ble_auto_test_case.h>


#include <tc_common.h>
#include <tc_200_sut.h>

#include <vendor_tp_client.h>
#include <vendor_tp_config.h>

#if TC_200_SUT_SUPPORT


typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint8_t remote_bd[6];
    uint8_t mode;/** 0 : 1M + w/o data length extension
                     1 : 1M + w data length extension
                     2 : 2M + w/o data length extension
                     3 : 2M + w data length extension
                     4 : Coded(s=2) + w/o data length extension
                     5 : Coded(s=2) + w data length extension
                     6 : Coded(s=8) + w/o data length extension
                     7 : Coded(s=8) + w data length extension
                    */
    uint8_t mode_end;
    T_TP_CONFIG_STATE cur_config_state;
    T_TP_CONFIG_OP cur_config_op;
    uint32_t total_notify_rx_count;
} TC_200_SUT_MGR;

TC_200_SUT_MGR *p_tc_200_sut_mgr = NULL;
#endif

#if TC_206_SUT_SUPPORT
typedef struct
{
    uint8_t initial_value;
    uint32_t total_test_count;
    uint8_t remote_bd[6];
    uint32_t total_notify_rx_count;
    uint32_t begin_time;
    uint32_t end_time;
    uint32_t elapsed_time;
    uint32_t data_rate;
} TC_206_SUT_MGR;

TC_206_SUT_MGR *p_tc_206_sut_mgr = NULL;
TTP_PERFER_PARAM g_206_sut_prefer_param;
#endif

#if TC_207_SUT_SUPPORT
typedef struct
{
    uint8_t initial_value;
    uint32_t total_test_count;
    uint8_t remote_bd[6];
    uint32_t count_remain;
    uint32_t begin_time;
    uint32_t end_time;
    uint32_t elapsed_time;
    uint32_t data_rate;
} TC_207_SUT_MGR;

TC_207_SUT_MGR *p_tc_207_sut_mgr = NULL;
TTP_PERFER_PARAM g_207_sut_prefer_param;
#endif

#if TC_208_SUT_SUPPORT
typedef struct
{
    uint8_t remote_bd[6];
    uint32_t rx_count;
    uint8_t  rx_initial_value;
    uint32_t rx_begin_time;
    uint32_t rx_end_time;
    uint32_t rx_elapsed_time;
    uint32_t rx_data_rate;

    uint32_t tx_count_remain;
    uint8_t  tx_initial_value;
    uint32_t tx_begin_time;
    uint32_t tx_end_time;
    uint32_t tx_elapsed_time;
    uint32_t tx_data_rate;
} TC_208_SUT_MGR;

TC_208_SUT_MGR *p_tc_208_sut_mgr = NULL;
TTP_PERFER_PARAM g_208_sut_prefer_param;
#endif

void tc_20x_sut_client_result_callback(uint8_t conn_id, void *p_cb_data)
{
    T_TP_CB_DATA *p_cb = (T_TP_CB_DATA *)p_cb_data;
    APP_PRINT_INFO1("tc_20x_sut_client_result_callback: %d.", p_cb->cb_type);
    switch (p_cb->cb_type)
    {
    case TP_CLIENT_CB_TYPE_DISC_RESULT:
        APP_PRINT_INFO2("TP_CLIENT_CB_TYPE_DISC_RESULT: is_found %d, value_handle 0x%x",
                        p_cb->cb_content.disc_result.is_found,
                        p_cb->cb_content.disc_result.char_tp.value_handle);
        tp_client_write_cccd(conn_id, GATT_CLIENT_CHAR_CONFIG_NOTIFY);
        break;

    case TP_CLIENT_CB_TYPE_WRITE_RESULT:
        switch (p_cb->cb_content.write_result.type)
        {
        case TP_WRITE_CCCD:
#if TC_200_SUT_SUPPORT
            if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
            {
                T_TP_CONFIG_OP op = TP_CONFIG_OP_SET_LL_DATA_LEN_27;
                if (p_tc_200_sut_mgr->mode % 2 == 0)
                {
                    op = TP_CONFIG_OP_SET_LL_DATA_LEN_27;
                    data_uart_print("Off data length extension\r\n");
                }
                else if (p_tc_200_sut_mgr->mode % 2 == 1)
                {
                    op = TP_CONFIG_OP_SET_LL_DATA_LEN_251;
                    data_uart_print("On data length extension\r\n");
                }

                APP_PRINT_INFO1("TP_WRITE_CCCD: cause 0x%x.", p_cb->cb_content.write_result.cause);

                tp_client_write_value(conn_id, 1, &op);
                p_tc_200_sut_mgr->cur_config_state = TP_CONFIG_STATE_CONFIGING;
                p_tc_200_sut_mgr->cur_config_op = op;
            }
#endif
#if TC_206_SUT_SUPPORT
            if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
            {
                tp_client_read_prefer_param(conn_id);
            }
#endif
#if TC_207_SUT_SUPPORT
            if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
            {
                tp_client_read_prefer_param(conn_id);
            }
#endif
#if TC_208_SUT_SUPPORT
            if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
            {
                tp_client_read_prefer_param(conn_id);
            }
#endif
            break;
        case TP_WRITE_CHAR_VALUE:
            APP_PRINT_INFO1("TP_WRITE_CHAR_VALUE: cause 0x%x.", p_cb->cb_content.write_result.cause);
            if (p_cb->cb_content.write_result.write_type == GATT_WRITE_TYPE_REQ)
            {
#if TC_200_SUT_SUPPORT
                if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
                {
                    if (p_tc_200_sut_mgr->cur_config_state == TP_CONFIG_STATE_CONFIGING)
                    {
                        uint8_t data[2];
                        data[0] = TP_CONFIG_OP_START;
                        data[1] = p_tc_200_sut_mgr->mode;
                        tp_client_write_value(conn_id, 2, data);
                        p_tc_200_sut_mgr->cur_config_state = TP_CONFIG_STATE_STARTED;
                    }
                }
#endif
            }
            else if (p_cb->cb_content.write_result.write_type == GATT_WRITE_TYPE_CMD)
            {
                if (p_cb->cb_content.write_result.cause == 0)
                {
#if TC_207_SUT_SUPPORT
                    if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                    {
                        tc_207_sut_tx_data_complete(p_cb->cb_content.write_result.credits);
                    }
#endif
#if TC_208_SUT_SUPPORT
                    if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                    {
                        tc_208_sut_tx_data_complete(p_cb->cb_content.write_result.credits);
                    }
#endif
                }
            }
            break;
        default:
            break;
        }
        break;
    case TP_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
#if TC_200_SUT_SUPPORT
        if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
        {
            p_tc_200_sut_mgr->total_notify_rx_count++;
            APP_PRINT_INFO3("TP_CLIENT_CB_TYPE_NOTIF_IND_RESULT: is_notif 0x%x, value_size %d, total_notify_rx_count %d",
                            p_cb->cb_content.notif_ind_data.is_notif,
                            p_cb->cb_content.notif_ind_data.value_size,
                            p_tc_200_sut_mgr->total_notify_rx_count);
            /* MAX_TEST_COUNT * TP_TEST_PARAM_COUNT * tc_tp_notification_tx_01_fixed_len_array_len */
            if (p_tc_200_sut_mgr->mode < p_tc_200_sut_mgr->mode_end &&
                p_tc_200_sut_mgr->total_notify_rx_count == 4000 * 12 * 5)
            {
                p_tc_200_sut_mgr->mode++;
                p_tc_200_sut_mgr->total_notify_rx_count = 0;
                p_tc_200_sut_mgr->total_connect_count--;
                tc_200_sut_link_connected(conn_id);
            }
        }
#endif
#if TC_206_SUT_SUPPORT
        if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
        {
            APP_PRINT_INFO3("TP_CLIENT_CB_TYPE_NOTIF_IND_RESULT: is_notif 0x%x, value_size %d, total_notify_rx_count %d",
                            p_cb->cb_content.notif_ind_data.is_notif,
                            p_cb->cb_content.notif_ind_data.value_size,
                            p_tc_206_sut_mgr->total_notify_rx_count);
            if (g_206_sut_prefer_param.length == p_cb->cb_content.notif_ind_data.value_size)
            {
                if (p_tc_206_sut_mgr->total_notify_rx_count == 0)
                {
                    p_tc_206_sut_mgr->begin_time = os_sys_time_get();
                    APP_PRINT_ERROR1("[206 SUT]:start rx time = %dms",
                                     p_tc_206_sut_mgr->begin_time);
                }
                if (g_206_sut_prefer_param.data_check)
                {
                    uint16_t i;
                    uint8_t *p_value = p_cb->cb_content.notif_ind_data.pValue;
                    for (i = 0; i < g_206_sut_prefer_param.length; i++)
                    {
                        if (p_value[i] != p_tc_206_sut_mgr->initial_value)
                        {
                            APP_PRINT_ERROR4("[206 SUT][RX]: data check failed: p_value[%d] 0x%x, initial_value %d, total_notify_rx_count %d",
                                             i, p_value[i],
                                             p_tc_206_sut_mgr->initial_value,
                                             p_tc_206_sut_mgr->total_notify_rx_count);
                            data_uart_print("[206 SUT][RX]: data check failed\r\n");
                            le_disconnect(conn_id);
                            break;
                        }
                    }
                }
            }
            else
            {
                APP_PRINT_ERROR1("[206 SUT][RX]: Len check failed: length %d",
                                 p_cb->cb_content.notif_ind_data.value_size);
                data_uart_print("[206 SUT][RX]: Len check failed: length %d\r\n",
                                p_cb->cb_content.notif_ind_data.value_size);
                le_disconnect(conn_id);
                break;
            }
            p_tc_206_sut_mgr->total_notify_rx_count++;
            p_tc_206_sut_mgr->initial_value++;
            if (p_tc_206_sut_mgr->total_notify_rx_count == g_206_sut_prefer_param.count)
            {
                p_tc_206_sut_mgr->end_time = os_sys_time_get();
                p_tc_206_sut_mgr->elapsed_time = os_time_get_elapsed(p_tc_206_sut_mgr->begin_time,
                                                                     p_tc_206_sut_mgr->end_time);
                p_tc_206_sut_mgr->data_rate =
                    g_206_sut_prefer_param.count * g_206_sut_prefer_param.length * 1000 /
                    (p_tc_206_sut_mgr->elapsed_time);
                APP_PRINT_ERROR1("[206 SUT]:end rx time = %dms",
                                 p_tc_206_sut_mgr->end_time);
                le_disconnect(conn_id);
            }
        }
#endif
#if TC_208_SUT_SUPPORT
        if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
        {
            APP_PRINT_INFO3("TP_CLIENT_CB_TYPE_NOTIF_IND_RESULT: is_notif 0x%x, value_size %d, total_notify_rx_count %d",
                            p_cb->cb_content.notif_ind_data.is_notif,
                            p_cb->cb_content.notif_ind_data.value_size,
                            p_tc_208_sut_mgr->rx_count);
            if (g_208_sut_prefer_param.length == p_cb->cb_content.notif_ind_data.value_size)
            {
                if (p_tc_208_sut_mgr->rx_count == 0)
                {
                    p_tc_208_sut_mgr->rx_begin_time = os_sys_time_get();
                    APP_PRINT_ERROR1("[208 SUT]:start rx time = %dms",
                                     p_tc_208_sut_mgr->rx_begin_time);
                }
                if (g_208_sut_prefer_param.data_check)
                {
                    uint16_t i;
                    uint8_t *p_value = p_cb->cb_content.notif_ind_data.pValue;
                    for (i = 0; i < g_208_sut_prefer_param.length; i++)
                    {
                        if (p_value[i] != p_tc_208_sut_mgr->rx_initial_value)
                        {
                            APP_PRINT_ERROR4("[208 SUT][RX]: data check failed:  p_value[%d] 0x%x, initial_value %d, total_notify_rx_count %d",
                                             i, p_value[i],
                                             p_tc_208_sut_mgr->rx_initial_value,
                                             p_tc_208_sut_mgr->rx_count);
                            data_uart_print("[208 SUT][RX]: data check failed\r\n");
                            le_disconnect(conn_id);
                            break;
                        }
                    }
                }
            }
            else
            {
                APP_PRINT_ERROR1("[208 SUT][RX]: Len check failed: length %d",
                                 p_cb->cb_content.notif_ind_data.value_size);
                data_uart_print("[208 SUT][RX]: Len check failed: length %d\r\n",
                                p_cb->cb_content.notif_ind_data.value_size);
                le_disconnect(conn_id);
                break;
            }
            p_tc_208_sut_mgr->rx_count++;
            p_tc_208_sut_mgr->rx_initial_value++;
            if (p_tc_208_sut_mgr->rx_count == g_208_sut_prefer_param.count)
            {
                p_tc_208_sut_mgr->rx_end_time = os_sys_time_get();
                p_tc_208_sut_mgr->rx_elapsed_time = os_time_get_elapsed(p_tc_208_sut_mgr->rx_begin_time,
                                                                        p_tc_208_sut_mgr->rx_end_time);
                p_tc_208_sut_mgr->rx_data_rate =
                    g_208_sut_prefer_param.count * g_208_sut_prefer_param.length * 1000 /
                    (p_tc_208_sut_mgr->rx_elapsed_time);
                APP_PRINT_ERROR1("[208 SUT]:end rx time = %dms",
                                 p_tc_208_sut_mgr->rx_end_time);
                tc_208_sut_dump_rx_result();
            }
        }
#endif
        break;
    case TP_CLIENT_CB_TYPE_READ_RESULT:
        APP_PRINT_INFO2("TP_CLIENT_CB_TYPE_READ_RESULT: cause 0x%x, value_size %d",
                        p_cb->cb_content.read_result.cause,
                        p_cb->cb_content.read_result.value_size);
        switch (p_cb->cb_content.read_result.type)
        {
        case TP_READ_PREFER_PARAM:
#if TC_206_SUT_SUPPORT
            if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
            {
                if (p_cb->cb_content.read_result.cause == GAP_SUCCESS
                    && p_cb->cb_content.read_result.value_size == sizeof(TTP_PERFER_PARAM))
                {
                    memcpy(&g_206_sut_prefer_param, p_cb->cb_content.read_result.p_value, sizeof(TTP_PERFER_PARAM));
                    p_tc_206_sut_mgr->total_test_count = g_206_sut_prefer_param.count;
                    p_tc_206_sut_mgr->initial_value = g_206_sut_prefer_param.initial_value;
                    APP_PRINT_INFO6("TP_READ_PREFER_PARAM: interval 0x%x, latency 0x%x, length %d, mode %d, count %d, check %d",
                                    g_206_sut_prefer_param.con_interval, g_206_sut_prefer_param.conn_slave_latency,
                                    g_206_sut_prefer_param.length, g_206_sut_prefer_param.mode,
                                    g_206_sut_prefer_param.count, g_206_sut_prefer_param.data_check);

                    data_uart_print("TP_READ_PREFER_PARAM: interval 0x%x, latency 0x%x, length %d, mode %d, count %d, check %d\r\n",
                                    g_206_sut_prefer_param.con_interval, g_206_sut_prefer_param.conn_slave_latency,
                                    g_206_sut_prefer_param.length, g_206_sut_prefer_param.mode,
                                    g_206_sut_prefer_param.count, g_206_sut_prefer_param.data_check);
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
                    if (g_206_sut_prefer_param.mode != 8)
                    {
                        T_TP_CONFIG_OP op = TP_CONFIG_OP_SET_LL_DATA_LEN_27;
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
                        tc_206_sut_phy_update(0);
#endif
                        if (g_206_sut_prefer_param.mode % 2 == 0)
                        {
                            op = TP_CONFIG_OP_SET_LL_DATA_LEN_27;
                            data_uart_print("Off data length extension\r\n");
                        }
                        else if (g_206_sut_prefer_param.mode % 2 == 1)
                        {
                            op = TP_CONFIG_OP_SET_LL_DATA_LEN_251;
                            data_uart_print("On data length extension\r\n");
                        }
                        tp_client_write_value(conn_id, 1, &op);
                    }
                    else
#endif
                    {
                        tc_206_sut_update_conn_param(0);
                    }
                }
                else
                {
                    APP_PRINT_ERROR0("TP_CLIENT_CB_TYPE_READ_RESULT: TC_0206_TP_NOTIFICATION_TX_02 failed");
                }
            }
#endif
#if TC_207_SUT_SUPPORT
            if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
            {
                if (p_cb->cb_content.read_result.cause == GAP_SUCCESS
                    && p_cb->cb_content.read_result.value_size == sizeof(TTP_PERFER_PARAM))
                {
                    memcpy(&g_207_sut_prefer_param, p_cb->cb_content.read_result.p_value, sizeof(TTP_PERFER_PARAM));
                    p_tc_207_sut_mgr->total_test_count = g_207_sut_prefer_param.count;
                    p_tc_207_sut_mgr->initial_value = g_207_sut_prefer_param.initial_value;
                    p_tc_207_sut_mgr->count_remain = g_207_sut_prefer_param.count;
                    APP_PRINT_INFO6("TP_READ_PREFER_PARAM: interval 0x%x, latency 0x%x, length %d, mode %d, count %d, check %d",
                                    g_207_sut_prefer_param.con_interval, g_207_sut_prefer_param.conn_slave_latency,
                                    g_207_sut_prefer_param.length, g_207_sut_prefer_param.mode,
                                    g_207_sut_prefer_param.count, g_207_sut_prefer_param.data_check);

                    data_uart_print("TP_READ_PREFER_PARAM: interval 0x%x, latency 0x%x, length %d, mode %d, count %d, check %d\r\n",
                                    g_207_sut_prefer_param.con_interval, g_207_sut_prefer_param.conn_slave_latency,
                                    g_207_sut_prefer_param.length, g_207_sut_prefer_param.mode,
                                    g_207_sut_prefer_param.count, g_207_sut_prefer_param.data_check);
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
                    if (g_207_sut_prefer_param.mode != 8)
                    {
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
                        tc_207_sut_phy_update(0);
#endif
                        if (g_207_sut_prefer_param.mode % 2 == 0)
                        {
                            le_set_data_len(0, 27, 0x0848);
                            data_uart_print("Off data length extension\r\n");
                        }
                        else if (g_207_sut_prefer_param.mode % 2 == 1)
                        {
                            le_set_data_len(0, 251, 0x0848);
                            data_uart_print("On data length extension\r\n");
                        }
                    }
                    else
#endif
                    {
                        tc_207_sut_update_conn_param(0);
                    }
                }
                else
                {
                    APP_PRINT_ERROR0("TP_CLIENT_CB_TYPE_READ_RESULT: TC_0207_TP_NOTIFICATION_TX_02 failed");
                }
            }
#endif
#if TC_208_SUT_SUPPORT
            if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
            {
                if (p_cb->cb_content.read_result.cause == GAP_SUCCESS
                    && p_cb->cb_content.read_result.value_size == sizeof(TTP_PERFER_PARAM))
                {
                    memcpy(&g_208_sut_prefer_param, p_cb->cb_content.read_result.p_value, sizeof(TTP_PERFER_PARAM));
                    p_tc_208_sut_mgr->rx_initial_value = g_208_sut_prefer_param.initial_value;
                    p_tc_208_sut_mgr->tx_initial_value = g_208_sut_prefer_param.initial_value;
                    p_tc_208_sut_mgr->tx_count_remain = g_208_sut_prefer_param.count;
                    APP_PRINT_INFO6("TP_READ_PREFER_PARAM: interval 0x%x, latency 0x%x, length %d, mode %d, count %d, check %d",
                                    g_208_sut_prefer_param.con_interval, g_208_sut_prefer_param.conn_slave_latency,
                                    g_208_sut_prefer_param.length, g_208_sut_prefer_param.mode,
                                    g_208_sut_prefer_param.count, g_208_sut_prefer_param.data_check);

                    data_uart_print("TP_READ_PREFER_PARAM: interval 0x%x, latency 0x%x, length %d, mode %d, count %d, check %d\r\n",
                                    g_208_sut_prefer_param.con_interval, g_208_sut_prefer_param.conn_slave_latency,
                                    g_208_sut_prefer_param.length, g_208_sut_prefer_param.mode,
                                    g_208_sut_prefer_param.count, g_208_sut_prefer_param.data_check);
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
                    if (g_208_sut_prefer_param.mode != 8)
                    {
                        T_TP_CONFIG_OP op = TP_CONFIG_OP_SET_LL_DATA_LEN_27;
                        if (g_208_sut_prefer_param.mode % 2 == 0)
                        {
                            op = TP_CONFIG_OP_SET_LL_DATA_LEN_27;
                            le_set_data_len(0, 27, 0x0848);
                            data_uart_print("Off data length extension\r\n");
                        }
                        else if (g_208_sut_prefer_param.mode % 2 == 1)
                        {
                            op = TP_CONFIG_OP_SET_LL_DATA_LEN_251;
                            le_set_data_len(0, 251, 0x0848);
                            data_uart_print("On data length extension\r\n");
                        }
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
                        tc_208_sut_phy_update(0);
#endif
                        tp_client_write_value(conn_id, 1, &op);
                    }
                    else
#endif
                    {
                        tc_208_sut_update_conn_param(0);
                    }
                }
                else
                {
                    APP_PRINT_ERROR0("TP_CLIENT_CB_TYPE_READ_RESULT: TC_0207_TP_NOTIFICATION_TX_02 failed");
                }
            }
#endif
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

#if TC_200_SUT_SUPPORT
void tc_200_sut_start(uint32_t count, uint8_t mode, uint8_t mode_end, uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 1000;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (NULL == p_tc_200_sut_mgr)
    {
        p_tc_200_sut_mgr = (TC_200_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_200_SUT_MGR));
    }
    else
    {
        memset(p_tc_200_sut_mgr, 0, sizeof(TC_200_SUT_MGR));
    }
    p_tc_200_sut_mgr->total_test_count = count;
    if (mode > 7)
    {
        mode  = 7;
    }
    if (mode_end < mode)
    {
        mode_end = mode;
    }
    else if (mode_end > 7)
    {
        mode_end = 7;
    }
    p_tc_200_sut_mgr->mode = mode;
    p_tc_200_sut_mgr->mode_end = mode_end;
    memcpy(p_tc_200_sut_mgr->remote_bd, remote_bd, 6);

    le_connect(0, p_tc_200_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}

void tc_200_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_0200_TP_NOTIFICATION_TX_01, reason);
    tc_update_disc_reason(reason);

    os_delay(200);

    if (p_tc_200_sut_mgr->total_connect_count < p_tc_200_sut_mgr->total_test_count +
        g_ble_disconn_reason.reason_3e)
    {
        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint16_t scan_timeout = 1000;
        le_connect(0, p_tc_200_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                   scan_timeout
                  );
    }
    else
    {
        tc_200_sut_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0200_TP_NOTIFICATION_TX_01, 0, NULL);
        }
    }
}

void tc_200_sut_link_connected(uint8_t conn_id)
{
    p_tc_200_sut_mgr->total_connect_count++;
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
    if (p_tc_200_sut_mgr->mode == 2 || p_tc_200_sut_mgr->mode == 3)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_NO
                  );
        data_uart_print("2M link\r\n");
    }
    else if (p_tc_200_sut_mgr->mode == 4 || p_tc_200_sut_mgr->mode == 5)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
        data_uart_print("Coded (s = 2) link\r\n");
    }
    else if (p_tc_200_sut_mgr->mode == 6 || p_tc_200_sut_mgr->mode == 7)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S8
                  );
        data_uart_print("Coded (s = 8) link\r\n");
    }
    else
    {
        data_uart_print("1M link\r\n");
    }
#endif
    tp_client_start_discovery(conn_id);
}

void tc_200_sut_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO4("tc_200_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x, total_notify_rx_count %d",
                    con_interval, conn_slave_latency, conn_supervision_timeout,
                    p_tc_200_sut_mgr->total_notify_rx_count);

    data_uart_print("tc_200_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x, total_notify_rx_count %d\r\n",
                    con_interval, conn_slave_latency, conn_supervision_timeout,
                    p_tc_200_sut_mgr->total_notify_rx_count);


}

void tc_200_sut_dump_result(void)
{
    if (p_tc_200_sut_mgr != NULL)
    {

        APP_PRINT_INFO2("tc_200_sut_dump_result: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_200_sut_mgr->total_test_count, p_tc_200_sut_mgr->total_connect_count);

        data_uart_print("tc_200_sut_dump_result: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_200_sut_mgr->total_test_count, p_tc_200_sut_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }


}
void tc_200_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_200_SUT_IN_PARAM_DATA *p_tc_200_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_200_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_200_SUT_IN_PARAM_DATA));

    p_tc_200_sut_param_data->id = TC_0200_TP_NOTIFICATION_TX_01;
    p_tc_200_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_200_sut_param_data->remote_bd, remote_bd, 6);


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_200_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}

#endif

#if TC_206_SUT_SUPPORT

void tc_206_sut_start(uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 0;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (NULL == p_tc_206_sut_mgr)
    {
        p_tc_206_sut_mgr = (TC_206_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_206_SUT_MGR));
    }
    else
    {
        memset(p_tc_206_sut_mgr, 0, sizeof(TC_206_SUT_MGR));
    }

    memcpy(p_tc_206_sut_mgr->remote_bd, remote_bd, 6);

    le_connect(0, p_tc_206_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}

void tc_206_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("Disc reason 0x%04x\r\n", reason);

    tc_206_sut_dump_result();
    if (p_tc_result_cb != NULL)
    {
        p_tc_result_cb(TC_0206_TP_NOTIFICATION_TX_02, 0, NULL);
    }
}

void tc_206_sut_link_connected(uint8_t conn_id)
{
    tp_client_start_discovery(conn_id);
}

void tc_206_sut_update_conn_param(uint8_t conn_id)
{
    le_update_conn_param(conn_id,
                         g_206_sut_prefer_param.con_interval,
                         g_206_sut_prefer_param.con_interval,
                         g_206_sut_prefer_param.conn_slave_latency,
                         g_206_sut_prefer_param.conn_supervision_timeout,
                         2 * (g_206_sut_prefer_param.con_interval - 1),
                         2 * (g_206_sut_prefer_param.con_interval - 1)
                        );
}

void tc_206_sut_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_206_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    data_uart_print("tc_206_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x\r\n",
                    con_interval, conn_slave_latency, conn_supervision_timeout);


}

#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void tc_206_sut_phy_update(uint8_t conn_id)
{
    if (g_206_sut_prefer_param.mode == 0 || g_206_sut_prefer_param.mode == 1)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_1M_BIT,
                   GAP_PHYS_PREFER_1M_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_206_sut_prefer_param.mode == 2 || g_206_sut_prefer_param.mode == 3)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_206_sut_prefer_param.mode == 4 || g_206_sut_prefer_param.mode == 5)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_206_sut_prefer_param.mode == 6 || g_206_sut_prefer_param.mode == 7)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S8
                  );
    }
}

void tc_206_sut_notification_phy_update_event(uint8_t conn_id, uint16_t cause,
                                              T_GAP_PHYS_TYPE tx_phy, T_GAP_PHYS_TYPE rx_phy)
{
    bool phy_result = false;
    if (cause == 0)
    {
        if (g_206_sut_prefer_param.mode == 0 || g_206_sut_prefer_param.mode == 1)
        {
            if (tx_phy == GAP_PHYS_1M && rx_phy == GAP_PHYS_1M)
            {
                phy_result = true;
            }
        }
        else if (g_206_sut_prefer_param.mode == 2 || g_206_sut_prefer_param.mode == 3)
        {
            if (tx_phy == GAP_PHYS_2M && rx_phy == GAP_PHYS_2M)
            {
                phy_result = true;
            }
        }
        else if (g_206_sut_prefer_param.mode == 4 || g_206_sut_prefer_param.mode == 5 ||
                 g_206_sut_prefer_param.mode == 6 || g_206_sut_prefer_param.mode == 7)
        {
            if (tx_phy == GAP_PHYS_CODED && rx_phy == GAP_PHYS_CODED)
            {
                phy_result = true;
            }
        }
    }
    if (phy_result == false)
    {
        APP_PRINT_ERROR0("[206 SUT]: Update phy failed");

        data_uart_print("[206 SUT]: Update phy failed\r\n");
        le_disconnect(0);
    }
    else
    {
        tc_206_sut_update_conn_param(0);
    }
}
#endif

void tc_206_sut_dump_result(void)
{
    if (p_tc_206_sut_mgr != NULL)
    {
        APP_PRINT_ERROR8("[206 SUT][RX]: conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                         g_206_sut_prefer_param.con_interval,
                         g_206_sut_prefer_param.conn_slave_latency,
                         g_206_sut_prefer_param.length,
                         g_206_sut_prefer_param.count,
                         p_tc_206_sut_mgr->begin_time,
                         p_tc_206_sut_mgr->end_time,
                         p_tc_206_sut_mgr->elapsed_time,
                         p_tc_206_sut_mgr->data_rate);
        data_uart_print("[206 SUT][RX]: conn_interval %d, latency %d, length %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                        g_206_sut_prefer_param.con_interval,
                        g_206_sut_prefer_param.conn_slave_latency,
                        g_206_sut_prefer_param.length,
                        g_206_sut_prefer_param.count,
                        p_tc_206_sut_mgr->begin_time,
                        p_tc_206_sut_mgr->end_time,
                        p_tc_206_sut_mgr->elapsed_time,
                        p_tc_206_sut_mgr->data_rate);
        APP_PRINT_ERROR3("[206 SUT][RX]: total_test_count %d total_notify_rx_count %d, initial_value 0x%x\r\n",
                         p_tc_206_sut_mgr->total_test_count, p_tc_206_sut_mgr->total_notify_rx_count,
                         p_tc_206_sut_mgr->initial_value);

        data_uart_print("[206 SUT][RX]: total_test_count %d total_notify_rx_count %d, initial_value 0x%x\r\n",
                        p_tc_206_sut_mgr->total_test_count, p_tc_206_sut_mgr->total_notify_rx_count,
                        p_tc_206_sut_mgr->initial_value);

    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

#endif

#if TC_207_SUT_SUPPORT

void tc_207_sut_start(uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 0;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (NULL == p_tc_207_sut_mgr)
    {
        p_tc_207_sut_mgr = (TC_207_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_207_SUT_MGR));
    }
    else
    {
        memset(p_tc_207_sut_mgr, 0, sizeof(TC_207_SUT_MGR));
    }

    memcpy(p_tc_207_sut_mgr->remote_bd, remote_bd, 6);

    le_connect(0, p_tc_207_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}

void tc_207_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("Disc reason 0x%04x\r\n", reason);

    tc_207_sut_dump_result();
    if (p_tc_result_cb != NULL)
    {
        p_tc_result_cb(TC_0207_TP_WRITE_COMMAND_RX_02, 0, NULL);
    }
}

void tc_207_sut_link_connected(uint8_t conn_id)
{
    tp_client_start_discovery(conn_id);
}

void tc_207_sut_update_conn_param(uint8_t conn_id)
{
    le_update_conn_param(conn_id,
                         g_207_sut_prefer_param.con_interval,
                         g_207_sut_prefer_param.con_interval,
                         g_207_sut_prefer_param.conn_slave_latency,
                         g_207_sut_prefer_param.conn_supervision_timeout,
                         2 * (g_207_sut_prefer_param.con_interval - 1),
                         2 * (g_207_sut_prefer_param.con_interval - 1)
                        );
}

void tc_207_sut_start_send_write_command(uint8_t conn_id)
{
    uint8_t credits = 10;
    p_tc_207_sut_mgr->begin_time = os_sys_time_get();
    APP_PRINT_ERROR1("[207 SUT]:begin time = %dms",
                     p_tc_207_sut_mgr->begin_time);

    while (credits)
    {
        if (p_tc_207_sut_mgr->count_remain)
        {
            uint8_t value[250];
            memset(value, p_tc_207_sut_mgr->initial_value,
                   g_207_sut_prefer_param.length);
            if (tp_client_write_command(conn_id, g_207_sut_prefer_param.length, value))
            {
                credits--;
                p_tc_207_sut_mgr->initial_value++;
                p_tc_207_sut_mgr->count_remain--;
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

void tc_207_sut_tx_data_complete(uint8_t credits)
{
    while (credits)
    {
        if (p_tc_207_sut_mgr->count_remain)
        {
            uint8_t value[250];
            memset(value, p_tc_207_sut_mgr->initial_value,
                   g_207_sut_prefer_param.length);
            if (tp_client_write_command(0, g_207_sut_prefer_param.length, value))
            {
                credits--;
                p_tc_207_sut_mgr->initial_value++;
                p_tc_207_sut_mgr->count_remain--;
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
                p_tc_207_sut_mgr->end_time = os_sys_time_get();
                p_tc_207_sut_mgr->elapsed_time = os_time_get_elapsed(p_tc_207_sut_mgr->begin_time,
                                                                     p_tc_207_sut_mgr->end_time);
                p_tc_207_sut_mgr->data_rate =
                    g_207_sut_prefer_param.count * g_207_sut_prefer_param.length * 1000 /
                    (p_tc_207_sut_mgr->elapsed_time);
                APP_PRINT_ERROR1("[207 SUT]:end time = %dms",
                                 p_tc_207_sut_mgr->end_time);
                le_disconnect(0);
            }
            break;
        }
    }
}

void tc_207_sut_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_207_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    data_uart_print("tc_207_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x\r\n",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (g_207_sut_prefer_param.con_interval ==  con_interval &&
        g_207_sut_prefer_param.conn_slave_latency ==  conn_slave_latency &&
        g_207_sut_prefer_param.conn_supervision_timeout == conn_supervision_timeout)
    {
        tc_207_sut_start_send_write_command(0);
    }
    else
    {
        data_uart_print("[207 SUT] error: Invalid conn parameter\r\n");
        le_disconnect(0);
    }
}

#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void tc_207_sut_phy_update(uint8_t conn_id)
{
    if (g_207_sut_prefer_param.mode == 0 || g_207_sut_prefer_param.mode == 1)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_1M_BIT,
                   GAP_PHYS_PREFER_1M_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_207_sut_prefer_param.mode == 2 || g_207_sut_prefer_param.mode == 3)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_207_sut_prefer_param.mode == 4 || g_207_sut_prefer_param.mode == 5)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_207_sut_prefer_param.mode == 6 || g_207_sut_prefer_param.mode == 7)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S8
                  );
    }
}

void tc_207_sut_notification_phy_update_event(uint8_t conn_id, uint16_t cause,
                                              T_GAP_PHYS_TYPE tx_phy, T_GAP_PHYS_TYPE rx_phy)
{
    bool phy_result = false;
    if (cause == 0)
    {
        if (g_207_sut_prefer_param.mode == 0 || g_207_sut_prefer_param.mode == 1)
        {
            if (tx_phy == GAP_PHYS_1M && rx_phy == GAP_PHYS_1M)
            {
                phy_result = true;
            }
        }
        else if (g_207_sut_prefer_param.mode == 2 || g_207_sut_prefer_param.mode == 3)
        {
            if (tx_phy == GAP_PHYS_2M && rx_phy == GAP_PHYS_2M)
            {
                phy_result = true;
            }
        }
        else if (g_207_sut_prefer_param.mode == 4 || g_207_sut_prefer_param.mode == 5 ||
                 g_207_sut_prefer_param.mode == 6 || g_207_sut_prefer_param.mode == 7)
        {
            if (tx_phy == GAP_PHYS_CODED && rx_phy == GAP_PHYS_CODED)
            {
                phy_result = true;
            }
        }
    }
    if (phy_result == false)
    {
        APP_PRINT_INFO0("[207 SUT]: Update phy failed");

        data_uart_print("[207 SUT]: Update phy failed\r\n");
        le_disconnect(0);
    }
    else
    {
        tc_207_sut_update_conn_param(0);
    }
}
#endif

void tc_207_sut_dump_result(void)
{
    if (p_tc_207_sut_mgr != NULL)
    {
        APP_PRINT_ERROR8("[207 SUT][TX]: conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                         g_207_sut_prefer_param.con_interval,
                         g_207_sut_prefer_param.conn_slave_latency,
                         g_207_sut_prefer_param.length,
                         g_207_sut_prefer_param.count,
                         p_tc_207_sut_mgr->begin_time,
                         p_tc_207_sut_mgr->end_time,
                         p_tc_207_sut_mgr->elapsed_time,
                         p_tc_207_sut_mgr->data_rate);
        data_uart_print("[207 SUT][TX]: conn_interval %d, latency %d, length %d, count = %d, begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                        g_207_sut_prefer_param.con_interval,
                        g_207_sut_prefer_param.conn_slave_latency,
                        g_207_sut_prefer_param.length,
                        g_207_sut_prefer_param.count,
                        p_tc_207_sut_mgr->begin_time,
                        p_tc_207_sut_mgr->end_time,
                        p_tc_207_sut_mgr->elapsed_time,
                        p_tc_207_sut_mgr->data_rate);
        APP_PRINT_ERROR3("[207 SUT][TX]: total_test_count %d count_remain %d, initial_value %d\r\n",
                         p_tc_207_sut_mgr->total_test_count, p_tc_207_sut_mgr->count_remain,
                         p_tc_207_sut_mgr->initial_value);

        data_uart_print("[207 SUT][TX]: total_test_count %d count_remain %d, initial_value %d\r\n",
                        p_tc_207_sut_mgr->total_test_count, p_tc_207_sut_mgr->count_remain,
                        p_tc_207_sut_mgr->initial_value);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

#endif

#if TC_208_SUT_SUPPORT

void tc_208_sut_start(uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 0;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (NULL == p_tc_208_sut_mgr)
    {
        p_tc_208_sut_mgr = (TC_208_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_208_SUT_MGR));
    }
    else
    {
        memset(p_tc_208_sut_mgr, 0, sizeof(TC_208_SUT_MGR));
    }

    memcpy(p_tc_208_sut_mgr->remote_bd, remote_bd, 6);

    le_connect(0, p_tc_208_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}

void tc_208_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("Disc reason 0x%04x\r\n", reason);

    tc_208_sut_dump_tx_result();
    tc_208_sut_dump_rx_result();
    if (p_tc_result_cb != NULL)
    {
        p_tc_result_cb(TC_0208_TP_NOTIF_WRITE_CMD_TRX_02, 0, NULL);
    }
}

void tc_208_sut_link_connected(uint8_t conn_id)
{
    tp_client_start_discovery(conn_id);
}

void tc_208_sut_update_conn_param(uint8_t conn_id)
{
    le_update_conn_param(conn_id,
                         g_208_sut_prefer_param.con_interval,
                         g_208_sut_prefer_param.con_interval,
                         g_208_sut_prefer_param.conn_slave_latency,
                         g_208_sut_prefer_param.conn_supervision_timeout,
                         2 * (g_208_sut_prefer_param.con_interval - 1),
                         2 * (g_208_sut_prefer_param.con_interval - 1)
                        );
}

void tc_208_sut_start_send_write_command(uint8_t conn_id)
{
    uint8_t credits = 10;
    p_tc_208_sut_mgr->tx_begin_time = os_sys_time_get();
    APP_PRINT_ERROR1("[208 SUT]:begin tx time = %dms",
                     p_tc_208_sut_mgr->tx_begin_time);

    while (credits)
    {
        if (p_tc_208_sut_mgr->tx_count_remain)
        {
            uint8_t value[250];
            memset(value, p_tc_208_sut_mgr->tx_initial_value,
                   g_208_sut_prefer_param.length);
            if (tp_client_write_command(conn_id, g_208_sut_prefer_param.length, value))
            {
                credits--;
                p_tc_208_sut_mgr->tx_initial_value++;
                p_tc_208_sut_mgr->tx_count_remain--;
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

void tc_208_sut_tx_data_complete(uint8_t credits)
{
    while (credits)
    {
        if (p_tc_208_sut_mgr->tx_count_remain)
        {
            uint8_t value[250];
            memset(value, p_tc_208_sut_mgr->tx_initial_value,
                   g_208_sut_prefer_param.length);
            if (tp_client_write_command(0, g_208_sut_prefer_param.length, value))
            {
                credits--;
                p_tc_208_sut_mgr->tx_initial_value++;
                p_tc_208_sut_mgr->tx_count_remain--;
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
                p_tc_208_sut_mgr->tx_end_time = os_sys_time_get();
                p_tc_208_sut_mgr->tx_elapsed_time = os_time_get_elapsed(p_tc_208_sut_mgr->tx_begin_time,
                                                                        p_tc_208_sut_mgr->tx_end_time);
                p_tc_208_sut_mgr->tx_data_rate =
                    g_208_sut_prefer_param.count * g_208_sut_prefer_param.length * 1000 /
                    (p_tc_208_sut_mgr->tx_elapsed_time);

                APP_PRINT_ERROR1("[208 SUT]:end tx time = %dms",
                                 p_tc_208_sut_mgr->tx_end_time);
                tc_208_sut_dump_tx_result();

                //le_disconnect(0);
            }
            break;
        }
    }
}

void tc_208_sut_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_208_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    data_uart_print("tc_208_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x\r\n",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (g_208_sut_prefer_param.con_interval ==  con_interval &&
        g_208_sut_prefer_param.conn_slave_latency ==  conn_slave_latency &&
        g_208_sut_prefer_param.conn_supervision_timeout == conn_supervision_timeout)
    {
        tc_208_sut_start_send_write_command(0);
    }
    else
    {
        data_uart_print("[208 SUT] error: Invalid conn parameter\r\n");
        le_disconnect(0);
    }
}

#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void tc_208_sut_phy_update(uint8_t conn_id)
{
    if (g_208_sut_prefer_param.mode == 0 || g_208_sut_prefer_param.mode == 1)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_1M_BIT,
                   GAP_PHYS_PREFER_1M_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_208_sut_prefer_param.mode == 2 || g_208_sut_prefer_param.mode == 3)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_PREFER_2M_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_208_sut_prefer_param.mode == 4 || g_208_sut_prefer_param.mode == 5)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S2
                  );
    }
    else if (g_208_sut_prefer_param.mode == 6 || g_208_sut_prefer_param.mode == 7)
    {
        le_set_phy(conn_id,
                   0,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_PREFER_CODED_BIT,
                   GAP_PHYS_OPTIONS_CODED_PREFER_S8
                  );
    }
}

void tc_208_sut_notification_phy_update_event(uint8_t conn_id, uint16_t cause,
                                              T_GAP_PHYS_TYPE tx_phy, T_GAP_PHYS_TYPE rx_phy)
{
    bool phy_result = false;
    if (cause == 0)
    {
        if (g_208_sut_prefer_param.mode == 0 || g_208_sut_prefer_param.mode == 1)
        {
            if (tx_phy == GAP_PHYS_1M && rx_phy == GAP_PHYS_1M)
            {
                phy_result = true;
            }
        }
        else if (g_208_sut_prefer_param.mode == 2 || g_208_sut_prefer_param.mode == 3)
        {
            if (tx_phy == GAP_PHYS_2M && rx_phy == GAP_PHYS_2M)
            {
                phy_result = true;
            }
        }
        else if (g_208_sut_prefer_param.mode == 4 || g_208_sut_prefer_param.mode == 5 ||
                 g_208_sut_prefer_param.mode == 6 || g_208_sut_prefer_param.mode == 7)
        {
            if (tx_phy == GAP_PHYS_CODED && rx_phy == GAP_PHYS_CODED)
            {
                phy_result = true;
            }
        }
    }
    if (phy_result == false)
    {
        APP_PRINT_INFO0("[208 SUT]: Update phy failed");

        data_uart_print("[208 SUT]: Update phy failed\r\n");
        le_disconnect(0);
    }
    else
    {
        tc_208_sut_update_conn_param(0);
    }
}
#endif

void tc_208_sut_dump_rx_result(void)
{
    if (p_tc_208_sut_mgr != NULL)
    {
        APP_PRINT_ERROR8("[208 SUT][RX]:conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                         g_208_sut_prefer_param.con_interval,
                         g_208_sut_prefer_param.conn_slave_latency,
                         g_208_sut_prefer_param.length,
                         g_208_sut_prefer_param.count,
                         p_tc_208_sut_mgr->rx_begin_time,
                         p_tc_208_sut_mgr->rx_end_time,
                         p_tc_208_sut_mgr->rx_elapsed_time,
                         p_tc_208_sut_mgr->rx_data_rate);
        data_uart_print("[208 SUT][RX]:conn_interval %d, latency %d, length %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                        g_208_sut_prefer_param.con_interval,
                        g_208_sut_prefer_param.conn_slave_latency,
                        g_208_sut_prefer_param.length,
                        g_208_sut_prefer_param.count,
                        p_tc_208_sut_mgr->rx_begin_time,
                        p_tc_208_sut_mgr->rx_end_time,
                        p_tc_208_sut_mgr->rx_elapsed_time,
                        p_tc_208_sut_mgr->rx_data_rate);
        APP_PRINT_ERROR2("[208 SUT][RX]: count %d rx_count %d\r\n",
                         g_208_sut_prefer_param.count, p_tc_208_sut_mgr->rx_count);

        data_uart_print("[208 SUT][RX]: count %d rx_count %d\r\n",
                        g_208_sut_prefer_param.count, p_tc_208_sut_mgr->rx_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_208_sut_dump_tx_result(void)
{
    if (p_tc_208_sut_mgr != NULL)
    {
        APP_PRINT_ERROR8("[208 SUT][TX]:conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                         g_208_sut_prefer_param.con_interval,
                         g_208_sut_prefer_param.conn_slave_latency,
                         g_208_sut_prefer_param.length,
                         g_208_sut_prefer_param.count,
                         p_tc_208_sut_mgr->tx_begin_time,
                         p_tc_208_sut_mgr->tx_end_time,
                         p_tc_208_sut_mgr->tx_elapsed_time,
                         p_tc_208_sut_mgr->tx_data_rate);
        data_uart_print("[208 SUT][TX]:conn_interval %d, latency %d, length %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate(Bytes/s) %d\r\n",
                        g_208_sut_prefer_param.con_interval,
                        g_208_sut_prefer_param.conn_slave_latency,
                        g_208_sut_prefer_param.length,
                        g_208_sut_prefer_param.count,
                        p_tc_208_sut_mgr->tx_begin_time,
                        p_tc_208_sut_mgr->tx_end_time,
                        p_tc_208_sut_mgr->tx_elapsed_time,
                        p_tc_208_sut_mgr->tx_data_rate);
        APP_PRINT_ERROR2("[208 SUT][TX]: count %d tx_count_remain %d\r\n",
                         g_208_sut_prefer_param.count, p_tc_208_sut_mgr->tx_count_remain);

        data_uart_print("[208 SUT][TX]: count %d tx_count_remain %d\r\n",
                        g_208_sut_prefer_param.count, p_tc_208_sut_mgr->tx_count_remain);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

#endif

