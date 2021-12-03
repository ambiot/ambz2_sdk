
#include <string.h>
#include "os_timer.h"
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"

#include "user_cmd.h"
#include "user_cmd_parse.h"
#include "gap_adv.h"


#include <os_mem.h>
#include <ble_auto_test_case.h>

#include <tc_common.h>
#include <tc_1200.h>
#include <link_mgr.h>

#if F_BT_LE_5_0_AE_SCAN_SUPPORT
#include <gap_ext_scan.h>
#endif


#if TC_1200_SUPPORT

void tc_1200_start(uint32_t count)
{
    uint16_t scan_interval = 400;
    uint16_t scan_window = 200;
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_start();
}

void tc_1200_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 1000;
    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 12;
    conn_param.conn_interval_max = 12;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

    le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void *g_tc_1200_timer_handle = NULL;

void tc_1200_conn_param_update_timeout_handler(void *pxTimer)
{
    uint8_t conn_id = 3;
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_1200_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (con_interval < 36)
    {
        con_interval++;

        le_update_conn_param(conn_id, con_interval, con_interval,
                             conn_slave_latency, conn_supervision_timeout,
                             2 * (con_interval + 1), 2 * (con_interval + 1));
    }
    else
    {
        //test case end here
        data_uart_print(" TC 1200 Test success\r\n");
        le_disconnect(0);
    }
}

void tc_1200_link_connected(uint8_t conn_id)
{
    data_uart_print("tc_1200_link_connected: conn_id %d\r\n", conn_id);
    uint8_t addr[6];
    T_GAP_REMOTE_ADDR_TYPE bd_type;
    le_get_conn_addr(conn_id, addr, &bd_type);
    data_uart_print("conn_id %d, bd_addr 0x%02x%02x%02x%02x%02x%02x\r\n",
                    conn_id, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    if (conn_id == 0)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[1].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[2].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[2].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[3].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[3].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {
        uint16_t con_interval;
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);

        os_timer_create(&g_tc_1200_timer_handle, "tc_1200_timer", 1, con_interval * 30, false,
                        tc_1200_conn_param_update_timeout_handler);
        os_timer_start(&g_tc_1200_timer_handle);
    }
}

void tc_1200_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("tc_1200_link_disconnected: conn_id %d, reason 0x%x\r\n", conn_id, reason);

    if (reason != (HCI_ERR_LOCAL_HOST_TERMINATE | HCI_ERR))
    {

        data_uart_print(" TC 1200 Test fail\r\n");
        os_timer_stop(&g_tc_1200_timer_handle);
        return;
    }

    if (conn_id == 0)
    {
        le_disconnect(1);
    }
    else if (conn_id == 1)
    {
        le_disconnect(2);
    }
    else if (conn_id == 2)
    {
        le_disconnect(3);
    }
    else if (conn_id == 3)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
        data_uart_print(" TC 1200 All link disconneced, Test again\r\n");
    }
}

void tc_1200_conn_param_update_evt(uint8_t conn_id)
{
    os_timer_start(&g_tc_1200_timer_handle);
}



void tc_1200_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1200_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif



#if TC_1201_SUPPORT

void tc_1201_start(uint32_t count)
{
    le_adv_start();
}

void tc_1201_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 1000;

    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 12;
    conn_param.conn_interval_max = 12;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

    le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void *g_tc_1201_timer_handle = NULL;

void tc_1201_conn_param_update_timeout_handler(void *pxTimer)
{
    uint8_t conn_id = 3;
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);


    APP_PRINT_INFO3("tc_1200_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);


    if (con_interval < 36)
    {
        con_interval++;

        le_update_conn_param(conn_id, con_interval, con_interval,
                             conn_slave_latency, conn_supervision_timeout,
                             2 * (con_interval + 1), 2 * (con_interval + 1));
    }
    else
    {
        //test case end here
        data_uart_print(" TC 1201 Test success\r\n");
        le_disconnect(1);
    }
}

void tc_1201_link_connected(uint8_t conn_id)
{
    data_uart_print("tc_1201_link_connected: conn_id %d\r\n", conn_id);
    uint8_t addr[6];
    T_GAP_REMOTE_ADDR_TYPE bd_type;
    le_get_conn_addr(conn_id, addr, &bd_type);
    data_uart_print("conn_id %d, bd_addr 0x%02x%02x%02x%02x%02x%02x\r\n",
                    conn_id, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    if (conn_id == 0)
    {
        uint16_t scan_interval = 400;
        uint16_t scan_window = 200;
        le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
        le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
        le_scan_start();
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[1].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[2].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[2].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {
        uint16_t con_interval;
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);

        os_timer_create(&g_tc_1201_timer_handle, "tc_1200_timer", 1, con_interval * 30, false,
                        tc_1201_conn_param_update_timeout_handler);
        os_timer_start(&g_tc_1201_timer_handle);
    }
}

void tc_1201_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("tc_1201_link_disconnected: conn_id %d, reason 0x%x\r\n", conn_id, reason);
    if (reason != (HCI_ERR_LOCAL_HOST_TERMINATE | HCI_ERR))
    {
        data_uart_print(" TC 1201 Test fail\r\n");
        os_timer_stop(&g_tc_1201_timer_handle);
        return;
    }

    if (conn_id == 0)
    {
        le_disconnect(1);
    }
    else if (conn_id == 1)
    {
        le_disconnect(2);
    }
    else if (conn_id == 2)
    {
        le_disconnect(3);
    }
    else if (conn_id == 3)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
        data_uart_print(" TC 1201 All link disconneced, Test again\r\n");
    }

}

void tc_1201_conn_param_update_evt(uint8_t conn_id)
{
    os_timer_start(&g_tc_1201_timer_handle);
}

void tc_1201_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1201_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif


#if TC_1202_SUPPORT

void tc_1202_start(uint32_t count)
{
    uint16_t scan_interval = 400;
    uint16_t scan_window = 200;
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_start();
}

void tc_1202_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 1000;
    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 6;
    conn_param.conn_interval_max = 12;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

    le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void *g_tc_1202_timer_handle = NULL;

void tc_1202_conn_param_update_timeout_handler(void *pxTimer)
{
    uint8_t conn_id = 3;
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_1200_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (con_interval < 36)
    {
        con_interval++;

        le_update_conn_param(conn_id, con_interval, con_interval,
                             conn_slave_latency, conn_supervision_timeout,
                             2 * (con_interval + 1), 2 * (con_interval + 1));
    }
    else
    {
        //test case end here
        data_uart_print(" TC 1200 Test success\r\n");
        le_disconnect(0);
    }
}

void tc_1202_link_connected(uint8_t conn_id)
{
    data_uart_print("tc_1202_link_connected: conn_id %d\r\n", conn_id);
    uint8_t addr[6];
    T_GAP_REMOTE_ADDR_TYPE bd_type;
    le_get_conn_addr(conn_id, addr, &bd_type);
    data_uart_print("conn_id %d, bd_addr 0x%02x%02x%02x%02x%02x%02x\r\n",
                    conn_id, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    if (conn_id == 0)
    {
        uint16_t scan_timeout = 1000;

        T_GAP_LE_CONN_REQ_PARAM conn_param;

        conn_param.scan_interval = 0x10;
        conn_param.scan_window = 0x10;
        conn_param.conn_interval_min = 9;
        conn_param.conn_interval_max = 18;
        conn_param.conn_latency = 0;
        conn_param.supv_tout = 1000;
        conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
        conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

        le_connect(0, dev_list[1].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        T_GAP_LE_CONN_REQ_PARAM conn_param;

        conn_param.scan_interval = 0x10;
        conn_param.scan_window = 0x10;
        conn_param.conn_interval_min = 24;
        conn_param.conn_interval_max = 24;
        conn_param.conn_latency = 0;
        conn_param.supv_tout = 1000;
        conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
        conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

        le_connect(0, dev_list[2].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        T_GAP_LE_CONN_REQ_PARAM conn_param;

        conn_param.scan_interval = 0x10;
        conn_param.scan_window = 0x10;
        conn_param.conn_interval_min = 12;
        conn_param.conn_interval_max = 12;
        conn_param.conn_latency = 0;
        conn_param.supv_tout = 1000;
        conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
        conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

        le_connect(0, dev_list[3].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {
        uint16_t con_interval;
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);

        os_timer_create(&g_tc_1202_timer_handle, "tc_1200_timer", 1, con_interval * 30, false,
                        tc_1202_conn_param_update_timeout_handler);
        os_timer_start(&g_tc_1202_timer_handle);
    }
}

void tc_1202_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("tc_1202_link_disconnected: conn_id %d, reason 0x%x\r\n", conn_id, reason);

    if (reason != (HCI_ERR_LOCAL_HOST_TERMINATE | HCI_ERR))
    {
        data_uart_print(" TC 1200 Test fail\r\n");
        os_timer_stop(&g_tc_1202_timer_handle);
        return;
    }

    if (conn_id == 0)
    {
        le_disconnect(1);
    }
    else if (conn_id == 1)
    {
        le_disconnect(2);
    }
    else if (conn_id == 2)
    {
        le_disconnect(3);
    }
    else if (conn_id == 3)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
        data_uart_print(" TC 1202 All link disconneced, Test again\r\n");
    }

}

void tc_1202_conn_param_update_evt(uint8_t conn_id)
{
    os_timer_start(&g_tc_1202_timer_handle);
}

void tc_1202_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1202_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif


#if TC_1203_SUPPORT

void tc_1203_start(uint32_t count)
{
    uint16_t scan_interval = 400;
    uint16_t scan_window = 200;
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_start();
}

void tc_1203_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 1000;
    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 9;
    conn_param.conn_interval_max = 18;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

    le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void *g_tc_1203_timer_handle = NULL;

void tc_1203_conn_param_update_timeout_handler(void *pxTimer)
{
    uint8_t conn_id = 3;
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_1200_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (con_interval < 36)
    {
        con_interval++;

        le_update_conn_param(conn_id, con_interval, con_interval,
                             conn_slave_latency, conn_supervision_timeout,
                             2 * (con_interval + 1), 2 * (con_interval + 1));
    }
    else
    {
        //test case end here
        data_uart_print(" TC 1203 Test success\r\n");
        le_disconnect(0);
    }
}

void tc_1203_link_connected(uint8_t conn_id)
{
    data_uart_print("tc_1203_link_connected: conn_id %d\r\n", conn_id);
    uint8_t addr[6];
    T_GAP_REMOTE_ADDR_TYPE bd_type;
    le_get_conn_addr(conn_id, addr, &bd_type);
    data_uart_print("conn_id %d, bd_addr 0x%02x%02x%02x%02x%02x%02x\r\n",
                    conn_id, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    if (conn_id == 0)
    {
        uint16_t scan_timeout = 1000;

        T_GAP_LE_CONN_REQ_PARAM conn_param;

        conn_param.scan_interval = 0x10;
        conn_param.scan_window = 0x10;
        conn_param.conn_interval_min = 6;
        conn_param.conn_interval_max = 12;
        conn_param.conn_latency = 0;
        conn_param.supv_tout = 1000;
        conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
        conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

        le_connect(0, dev_list[1].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        T_GAP_LE_CONN_REQ_PARAM conn_param;

        conn_param.scan_interval = 0x10;
        conn_param.scan_window = 0x10;
        conn_param.conn_interval_min = 24;
        conn_param.conn_interval_max = 24;
        conn_param.conn_latency = 0;
        conn_param.supv_tout = 1000;
        conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
        conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

        le_connect(0, dev_list[2].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        T_GAP_LE_CONN_REQ_PARAM conn_param;

        conn_param.scan_interval = 0x10;
        conn_param.scan_window = 0x10;
        conn_param.conn_interval_min = 12;
        conn_param.conn_interval_max = 12;
        conn_param.conn_latency = 0;
        conn_param.supv_tout = 1000;
        conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
        conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

        le_connect(0, dev_list[3].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {
        uint16_t con_interval;
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);

        os_timer_create(&g_tc_1203_timer_handle, "tc_1200_timer", 1, con_interval * 30, false,
                        tc_1203_conn_param_update_timeout_handler);
        os_timer_start(&g_tc_1203_timer_handle);
    }
}

void tc_1203_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("tc_1203_link_disconnected: conn_id %d, reason 0x%x\r\n", conn_id, reason);

    if (reason != (HCI_ERR_LOCAL_HOST_TERMINATE | HCI_ERR))
    {
        data_uart_print(" TC 1203 Test fail\r\n");
        os_timer_stop(&g_tc_1203_timer_handle);
        return;
    }

    if (conn_id == 0)
    {
        le_disconnect(1);
    }
    else if (conn_id == 1)
    {
        le_disconnect(2);
    }
    else if (conn_id == 2)
    {
        le_disconnect(3);
    }
    else if (conn_id == 3)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
        data_uart_print(" TC 1203 All link disconneced, Test again\r\n");
    }

}

void tc_1203_conn_param_update_evt(uint8_t conn_id)
{
    os_timer_start(&g_tc_1203_timer_handle);
}

void tc_1203_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1203_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif


#if TC_1204_SUPPORT

void tc_1204_start(uint32_t count)
{
    uint8_t  ext_scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT | GAP_EXT_SCAN_PHYS_CODED_BIT;

    link_mgr_clear_device_list();

    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(ext_scan_filter_policy),
                          &ext_scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(ext_scan_filter_duplicate),
                          &ext_scan_filter_duplicate);
    le_ext_scan_start();

}

void tc_1204_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 1000;
    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 12;
    conn_param.conn_interval_max = 12;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

    le_connect(GAP_PHYS_CONN_INIT_1M_BIT, dev_list[0].bd_addr,
               (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void *g_tc_1204_timer_handle = NULL;

void tc_1204_conn_param_update_timeout_handler(void *pxTimer)
{
    uint8_t conn_id = 3;
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_1200_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (con_interval < 36)
    {
        con_interval++;

        le_update_conn_param(conn_id, con_interval, con_interval,
                             conn_slave_latency, conn_supervision_timeout,
                             2 * (con_interval + 1), 2 * (con_interval + 1));
    }
    else
    {
        //test case end here
        data_uart_print(" TC 1200 Test success\r\n");
    }
}

void tc_1204_link_connected(uint8_t conn_id)
{
    if (conn_id == 0)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_1M_BIT, dev_list[1].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_1M_BIT, dev_list[2].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_1M_BIT, dev_list[3].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {
        uint16_t con_interval;
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);

        os_timer_create(&g_tc_1204_timer_handle, "tc_1200_timer", 1, con_interval * 30, false,
                        tc_1204_conn_param_update_timeout_handler);
        os_timer_start(&g_tc_1204_timer_handle);
    }
}

void tc_1204_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    data_uart_print("tc_1204_link_disconnected: conn_id %d, reason 0x%x\r\n", conn_id, reason);
    tc_update_disc_reason(reason);
    data_uart_print(" TC 1204 Test fail\r\n");
    {
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_1204_MULTI_LINK_4_MASTER, 0, NULL);
        }
    }

}

void tc_1204_conn_param_update_evt(uint8_t conn_id)
{
    os_timer_start(&g_tc_1204_timer_handle);
}



void tc_1204_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1204_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif

#if TC_1205_SUPPORT

void tc_1205_start(uint32_t count)
{
    uint8_t  ext_scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT | GAP_EXT_SCAN_PHYS_CODED_BIT;

    link_mgr_clear_device_list();

    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(ext_scan_filter_policy),
                          &ext_scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(ext_scan_filter_duplicate),
                          &ext_scan_filter_duplicate);
    le_ext_scan_start();
}

void tc_1205_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 1000;
    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 12;
    conn_param.conn_interval_max = 12;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_2M, &conn_param);

    le_connect(GAP_PHYS_CONN_INIT_2M_BIT, dev_list[0].bd_addr,
               (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void *g_tc_1205_timer_handle = NULL;

void tc_1205_conn_param_update_timeout_handler(void *pxTimer)
{
    uint8_t conn_id = 3;
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_1205_conn_param_update_timeout_handler update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (con_interval < 36)
    {
        con_interval++;

        le_update_conn_param(conn_id, con_interval, con_interval,
                             conn_slave_latency, conn_supervision_timeout,
                             2 * (con_interval + 1), 2 * (con_interval + 1));
    }
    else
    {
        //test case end here
        data_uart_print(" TC 1205 Test success\r\n");
    }
}

void tc_1205_link_connected(uint8_t conn_id)
{
    if (conn_id == 0)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_2M_BIT, dev_list[1].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_2M_BIT, dev_list[2].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_2M_BIT, dev_list[3].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {
        uint16_t con_interval;
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);

        os_timer_create(&g_tc_1205_timer_handle, "tc_1200_timer", 1, con_interval * 30, false,
                        tc_1205_conn_param_update_timeout_handler);
        os_timer_start(&g_tc_1205_timer_handle);
    }
}

void tc_1205_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1205_MULTI_LINK_4_MASTER, reason);
    tc_update_disc_reason(reason);
    data_uart_print(" TC 1205 Test fail\r\n");
    {
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_1205_MULTI_LINK_4_MASTER, 0, NULL);
        }
    }

}

void tc_1205_conn_param_update_evt(uint8_t conn_id)
{
    os_timer_start(&g_tc_1205_timer_handle);
}



void tc_1205_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1205_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif

#if TC_1206_SUPPORT

void tc_1206_start(uint32_t count)
{
    uint8_t  ext_scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT | GAP_EXT_SCAN_PHYS_CODED_BIT;

    link_mgr_clear_device_list();

    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(ext_scan_filter_policy),
                          &ext_scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(ext_scan_filter_duplicate),
                          &ext_scan_filter_duplicate);
    le_ext_scan_start();
}

void tc_1206_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 1000;
    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 12;
    conn_param.conn_interval_max = 12;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_CODED, &conn_param);

    le_connect(GAP_PHYS_CONN_INIT_CODED_BIT, dev_list[0].bd_addr,
               (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void *g_tc_1206_timer_handle = NULL;

void tc_1206_conn_param_update_timeout_handler(void *pxTimer)
{
    uint8_t conn_id = 3;
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_1200_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (con_interval < 36)
    {
        con_interval++;

        le_update_conn_param(conn_id, con_interval, con_interval,
                             conn_slave_latency, conn_supervision_timeout,
                             2 * (con_interval + 1), 2 * (con_interval + 1));
    }
    else
    {
        //test case end here
        data_uart_print(" TC 1200 Test success\r\n");
    }
}

void tc_1206_link_connected(uint8_t conn_id)
{
    if (conn_id == 0)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_CODED_BIT, dev_list[1].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_CODED_BIT, dev_list[2].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        le_connect(GAP_PHYS_CONN_INIT_CODED_BIT, dev_list[3].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {
        uint16_t con_interval;
        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);

        os_timer_create(&g_tc_1206_timer_handle, "tc_1200_timer", 1, con_interval * 30, false,
                        tc_1206_conn_param_update_timeout_handler);
        os_timer_start(&g_tc_1206_timer_handle);
    }
}

void tc_1206_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1206_MULTI_LINK_4_MASTER, reason);
    tc_update_disc_reason(reason);
    data_uart_print(" TC 1206 Test fail\r\n");
    {
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_1206_MULTI_LINK_4_MASTER, 0, NULL);
        }
    }

}

void tc_1206_conn_param_update_evt(uint8_t conn_id)
{
    os_timer_start(&g_tc_1206_timer_handle);
}



void tc_1206_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1206_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif

#if TC_1207_SUPPORT

void tc_1207_start(uint32_t count)
{
    uint16_t scan_interval = 400;
    uint16_t scan_window = 200;
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_start();
}

void tc_1207_scan_state_change_to_idle()
{
    uint16_t scan_timeout = 0;//no timeout
    T_GAP_LE_CONN_REQ_PARAM conn_param;

    conn_param.scan_interval = 0x10;
    conn_param.scan_window = 0x10;
    conn_param.conn_interval_min = 6;
    conn_param.conn_interval_max = 6;
    conn_param.conn_latency = 0;
    conn_param.supv_tout = 1000;
    conn_param.ce_len_min = 2 * (conn_param.conn_interval_min - 1);
    conn_param.ce_len_max = 2 * (conn_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_param);

    le_connect(0, dev_list[0].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[0].bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void tc_1207_link_connected(uint8_t conn_id)
{
    data_uart_print("tc_1207_link_connected: conn_id %d\r\n", conn_id);

    if (conn_id == 0)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[1].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 1)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[2].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else if (conn_id == 2)
    {
        uint16_t scan_timeout = 1000;
        le_connect(0, dev_list[3].bd_addr, (T_GAP_REMOTE_ADDR_TYPE)dev_list[1].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
    }
    else
    {

    }
}

void tc_1207_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1207_MULTI_LINK_4_MASTER, reason);
    tc_update_disc_reason(reason);
    data_uart_print("tc_1207_link_disconnected: conn_id %d\r\n", conn_id);
    {
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_1207_MULTI_LINK_4_MASTER, 0, NULL);
        }
    }

}

void tc_1207_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1207_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif
