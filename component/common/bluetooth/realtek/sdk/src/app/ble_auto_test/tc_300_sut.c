
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "user_cmd.h"
#include "user_cmd_parse.h"
#include "os_sched.h"
#include <os_mem.h>
#include <ble_auto_test_case.h>
#include <tc_common.h>
#include <tc_300_sut.h>


#if TC_300_SUT_SUPPORT
/**

*/

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t conn_supervision_timeout;
    uint8_t remote_bd[6];

} TC_300_SUT_MGR;

static TC_300_SUT_MGR *p_tc_300_sut_mgr = NULL;

void tc_300_sut_start(uint32_t count, uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 10000;

    if (NULL == p_tc_300_sut_mgr)
    {
        p_tc_300_sut_mgr = (TC_300_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_300_SUT_MGR));
    }
    else
    {
        memset(p_tc_300_sut_mgr, 0, sizeof(TC_300_SUT_MGR));
    }
    p_tc_300_sut_mgr->total_test_count = count;
    memcpy(p_tc_300_sut_mgr->remote_bd, remote_bd, 6);

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    le_connect(0, p_tc_300_sut_mgr->remote_bd, remote_bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout
              );

}

void tc_300_sut_link_connected(uint8_t conn_id)
{
    APP_PRINT_TRACE1("tc_300_sut_link_connected: conn_id %d", conn_id);
    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    reset_vendor_counter();
    g_test_end_time = read_vendor_counter_no_display();

    p_tc_300_sut_mgr->total_connect_count++;

    os_delay(200);

    le_disconnect(conn_id);
}

void tc_300_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    APP_PRINT_TRACE2("tc_300_sut_link_disconnected: conn_id %d, reason 0x%04x", conn_id, reason);

    g_test_end_time = read_vendor_counter_no_display();


    tc_check_local_disc_reason(TC_0300_ADV_ONLY, reason);
    tc_update_disc_reason(reason);

#if 0
    if ((reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) &&
        (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        data_uart_print(
            "tc_300_sut_link_disconnected fail: reason 0x%4x, begin time= %d,end time=%d,elapase time=%dus, conn_interval: %d,conn_latency: %d, conn_supervision_timeout: %d\r\n",
            reason,
            g_test_begin_time,
            g_test_end_time,
            (g_test_end_time - g_test_begin_time) / TEST_CPU_CLOCK,
            conn_interval,
            conn_latency,
            conn_supervision_timeout);
    }
#endif


    {
        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint16_t scan_timeout = 10000;

        if (p_tc_300_sut_mgr->total_connect_count  < p_tc_300_sut_mgr->total_test_count +
            g_ble_disconn_reason.reason_3e)
        {
            os_delay(200);
            le_connect(0, p_tc_300_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                       scan_timeout
                      );
        }
        else
        {
            tc_300_sut_dump_result();
            if (p_tc_result_cb != NULL)
            {
                p_tc_result_cb(TC_0300_ADV_ONLY, 0, NULL);
            }
        }
    }
}
void tc_300_sut_dump_result(void)
{
    if (p_tc_300_sut_mgr != NULL)
    {
        APP_PRINT_INFO2("tc_300_sut_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_300_sut_mgr->total_test_count, p_tc_300_sut_mgr->total_connect_count);

        data_uart_print("tc_300_sut_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_300_sut_mgr->total_test_count, p_tc_300_sut_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_300_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_300_SUT_IN_PARAM_DATA *p_tc_300_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_300_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_300_SUT_IN_PARAM_DATA));

    p_tc_300_sut_param_data->id = TC_0300_ADV_ONLY;
    p_tc_300_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_300_sut_param_data->remote_bd, remote_bd, 6);

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_300_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}



/**
    TC_0301_ADV_DISC,
*/

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t conn_supervision_timeout;
    uint8_t remote_bd[6];
} TC_301_SUT_MGR;

static TC_301_SUT_MGR *p_tc_301_sut_mgr = NULL;
void tc_301_sut_start(uint32_t count, uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 10000;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (NULL == p_tc_301_sut_mgr)
    {
        p_tc_301_sut_mgr = (TC_301_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_301_SUT_MGR));
    }
    else
    {
        memset(p_tc_301_sut_mgr, 0, sizeof(TC_301_SUT_MGR));
    }

    p_tc_301_sut_mgr->total_test_count = count;


    memcpy(p_tc_301_sut_mgr->remote_bd, remote_bd, 6);
    le_connect(0, p_tc_301_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}


void tc_301_sut_link_connected(uint8_t conn_id)
{

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    reset_vendor_counter();
    g_test_end_time = read_vendor_counter_no_display();
    p_tc_301_sut_mgr->total_connect_count++;

}

void tc_301_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    g_test_end_time = read_vendor_counter_no_display();

    tc_check_remote_disc_reason(TC_0301_ADV_DISC, reason);
    tc_update_disc_reason(reason);

#if 0
    if (reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE) &&
        reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
    {
        data_uart_print(
            "tc_301_sut_link_disconnected fail: reason 0x%4x, begin time= %d,end time=%d,elapase time=%dus, conn_interval: %d,conn_latency: %d, conn_supervision_timeout: %d\r\n",
            reason,
            g_test_begin_time,
            g_test_end_time,
            (g_test_end_time - g_test_begin_time) / TEST_CPU_CLOCK,
            conn_interval,
            conn_latency,
            conn_supervision_timeout);
    }
#endif

    {
        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint16_t scan_timeout = 1000;



        if (p_tc_301_sut_mgr->total_connect_count < p_tc_301_sut_mgr->total_test_count +
            g_ble_disconn_reason.reason_3e)
        {
            os_delay(200);
            le_connect(0, p_tc_301_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                       scan_timeout
                      );
        }
        else
        {
            tc_301_sut_dump_result();
            if (p_tc_result_cb != NULL)
            {
                p_tc_result_cb(TC_0301_ADV_DISC, 0, NULL);
            }
        }
    }

}

void tc_301_sut_dump_result(void)
{
    if (p_tc_301_sut_mgr != NULL)
    {
        APP_PRINT_INFO2("tc_301_sut_dump_result: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_301_sut_mgr->total_test_count, p_tc_301_sut_mgr->total_connect_count);

        data_uart_print("tc_301_sut_dump_result: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_301_sut_mgr->total_test_count, p_tc_301_sut_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_301_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_301_SUT_IN_PARAM_DATA *p_tc_301_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_301_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_301_SUT_IN_PARAM_DATA));

    p_tc_301_sut_param_data->id = TC_0301_ADV_DISC;
    p_tc_301_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_301_sut_param_data->remote_bd, remote_bd, 6);


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_301_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}

#endif

