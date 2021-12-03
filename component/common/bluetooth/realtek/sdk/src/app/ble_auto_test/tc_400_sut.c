
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"

#include "user_cmd.h"
#include "user_cmd_parse.h"


#include <os_mem.h>
#include <ble_auto_test_case.h>


#include <tc_common.h>
#include <tc_400_sut.h>

#if TC_400_SUT_SUPPORT

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t conn_supervision_timeout;
    uint8_t remote_bd[6];
} TC_400_SUT_MGR;
TC_400_SUT_MGR *p_tc_400_sut_mgr = NULL;


void tc_400_sut_start(uint32_t count, uint8_t remote_bd[6])
{
    uint16_t scan_timeout = 1000;
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (NULL == p_tc_400_sut_mgr)
    {
        p_tc_400_sut_mgr = (TC_400_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_400_SUT_MGR));
    }
    else
    {
        memset(p_tc_400_sut_mgr, 0, sizeof(TC_400_SUT_MGR));
    }

    p_tc_400_sut_mgr->total_test_count = count;
    memcpy(p_tc_400_sut_mgr->remote_bd, remote_bd, 6);

    le_connect(0, remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void tc_400_sut_link_connected(uint8_t conn_id)
{
    p_tc_400_sut_mgr->total_connect_count++;
}

void tc_400_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_0400_CONN_PARAM_UPDATE_SLAVE, reason);
    tc_update_disc_reason(reason);

    if (p_tc_400_sut_mgr->total_connect_count < p_tc_400_sut_mgr->total_test_count)
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

        le_connect(0, p_tc_400_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                   scan_timeout
                  );
    }
    else
    {
        tc_400_sut_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0400_CONN_PARAM_UPDATE_SLAVE, 0, NULL);
        }
    }
}

void tc_400_sut_conn_param_update_evt(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_400_sut_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    p_tc_400_sut_mgr->conn_interval = con_interval;
    p_tc_400_sut_mgr->conn_latency = conn_slave_latency;
    p_tc_400_sut_mgr->conn_supervision_timeout = conn_supervision_timeout;
}

void tc_400_sut_dump_result(void)
{
    if (p_tc_400_sut_mgr != NULL)
    {
        APP_PRINT_INFO3("tc_400_sut_dump_result: total_test_count %d total_connect_count %d conn_interval_min 0x%04x\r\n",
                        p_tc_400_sut_mgr->total_test_count, p_tc_400_sut_mgr->total_connect_count,
                        p_tc_400_sut_mgr->conn_interval);

        data_uart_print("tc_400_sut_dump_result: total_test_count %d total_connect_count %d conn_interval_min 0x%04x\r\n",
                        p_tc_400_sut_mgr->total_test_count, p_tc_400_sut_mgr->total_connect_count,
                        p_tc_400_sut_mgr->conn_interval);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }

}

void tc_400_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_400_SUT_IN_PARAM_DATA *p_tc_400_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_400_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_400_SUT_IN_PARAM_DATA));

    p_tc_400_sut_param_data->id = TC_0400_CONN_PARAM_UPDATE_SLAVE;
    p_tc_400_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_400_sut_param_data->remote_bd, remote_bd, 6);

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_400_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif

#if TC_401_SUT_SUPPORT
typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint16_t conn_interval_min;
    uint16_t conn_interval_max;
    uint16_t conn_latency;
    uint16_t conn_supervision_timeout;
    uint8_t remote_bd[6];
} TC_401_SUT_MGR;
TC_401_SUT_MGR *p_tc_401_sut_mgr = NULL;


void tc_401_sut_start(uint32_t count, uint8_t remote_bd[6])
{
    uint16_t scan_timeout = 1000;
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (NULL == p_tc_401_sut_mgr)
    {
        p_tc_401_sut_mgr = (TC_401_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_401_SUT_MGR));
    }
    else
    {
        memset(p_tc_401_sut_mgr, 0, sizeof(TC_401_SUT_MGR));
    }

    p_tc_401_sut_mgr->total_test_count = count;
    memcpy(p_tc_401_sut_mgr->remote_bd, remote_bd, 6);

    le_connect(0, remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout);
}

void tc_401_sut_link_connected(uint8_t conn_id)
{
    p_tc_401_sut_mgr->total_connect_count++;

    le_update_conn_param(conn_id, p_tc_401_sut_mgr->conn_interval_min,
                         p_tc_401_sut_mgr->conn_interval_max,
                         p_tc_401_sut_mgr->conn_latency, p_tc_401_sut_mgr->conn_supervision_timeout,
                         2 * (p_tc_401_sut_mgr->conn_interval_min - 1),
                         2 * (p_tc_401_sut_mgr->conn_interval_max - 1));
}

void tc_401_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_0401_CONN_PARAM_UPDATE_SLAVE_01, reason);
    tc_update_disc_reason(reason);

    if (p_tc_401_sut_mgr->total_connect_count < p_tc_401_sut_mgr->total_test_count)
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

        le_connect(0, p_tc_401_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                   scan_timeout
                  );
    }
    else
    {
        tc_401_sut_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0401_CONN_PARAM_UPDATE_SLAVE_01, 0, NULL);
        }
    }
}

void tc_401_sut_conn_param_update_evt(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("tc_401_sut_conn_param_update_evt update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (con_interval < 0x4000)
    {
        p_tc_401_sut_mgr->conn_interval_min++;
        p_tc_401_sut_mgr->conn_interval_max++;

        le_update_conn_param(conn_id, p_tc_401_sut_mgr->conn_interval_min,
                             p_tc_401_sut_mgr->conn_interval_max,
                             p_tc_401_sut_mgr->conn_latency, p_tc_401_sut_mgr->conn_supervision_timeout,
                             2 * (p_tc_401_sut_mgr->conn_interval_min - 1),
                             2 * (p_tc_401_sut_mgr->conn_interval_max - 1));
    }
    else
    {
        p_tc_401_sut_mgr->conn_interval_min = 0x0020;
        p_tc_401_sut_mgr->conn_interval_max = 0x0020;
    }



}

void tc_401_sut_dump_result(void)
{
    if (p_tc_401_sut_mgr != NULL)
    {
        APP_PRINT_INFO3("tc_401_sut_dump_result: total_test_count %d total_connect_count %d conn_interval_min 0x%04x\r\n",
                        p_tc_401_sut_mgr->total_test_count, p_tc_401_sut_mgr->total_connect_count,
                        p_tc_401_sut_mgr->conn_interval_min);

        data_uart_print("tc_401_sut_dump_result: total_test_count %d total_connect_count %d conn_interval_min 0x%04x\r\n",
                        p_tc_401_sut_mgr->total_test_count, p_tc_401_sut_mgr->total_connect_count,
                        p_tc_401_sut_mgr->conn_interval_min);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }

}

void tc_401_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_401_SUT_IN_PARAM_DATA *p_tc_401_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_401_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_401_SUT_IN_PARAM_DATA));

    p_tc_401_sut_param_data->id = TC_0401_CONN_PARAM_UPDATE_SLAVE_01;
    p_tc_401_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_401_sut_param_data->remote_bd, remote_bd, 6);

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_401_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}

#endif
