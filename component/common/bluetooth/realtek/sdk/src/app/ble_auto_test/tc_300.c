
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_bond_le.h"

#include "user_cmd.h"
#include "user_cmd_parse.h"
#include "gap_adv.h"

#include "os_sched.h"
#include <os_mem.h>
#include <ble_auto_test_case.h>

#include <tc_common.h>
#include <tc_300.h>



#if TC_300_SUPPORT
/**
    TC_0301_ADV_DISC,
*/

typedef struct
{
    uint16_t id;
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t conn_supervision_timeout;
} TC_300_MGR;
TC_300_MGR *p_tc_300_mgr = NULL;

void tc_300_adv_only_start(uint32_t count)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);

    if (NULL == p_tc_300_mgr)
    {
        p_tc_300_mgr = (TC_300_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_300_MGR));
    }
    else
    {
        memset(p_tc_300_mgr, 0, sizeof(TC_300_MGR));
    }
    p_tc_300_mgr->total_test_count = count;


    le_adv_start();
}



void tc_300_adv_only_link_connected(uint8_t conn_id)
{

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    reset_vendor_counter();
    g_test_end_time = read_vendor_counter_no_display();
    p_tc_300_mgr->total_connect_count++;

}

void tc_300_adv_only_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    g_test_end_time = read_vendor_counter_no_display();

    tc_check_remote_disc_reason(TC_0300_ADV_ONLY,  reason);
    tc_update_disc_reason(reason);

#if 0
    if ((reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) &&
        (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        data_uart_print(
            "tc_300_adv_only_link_disconnected fail: reason 0x%4x, begin time= %d,end time=%d,elapase time=%dus, conn_interval: %d,conn_latency: %d, conn_supervision_timeout: %d\r\n",
            reason,
            g_test_begin_time,
            g_test_end_time,
            (g_test_end_time - g_test_begin_time) / TEST_CPU_CLOCK,
            conn_interval,
            conn_latency,
            conn_supervision_timeout);
    }
#endif

    if (p_tc_300_mgr->total_connect_count < p_tc_300_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_300_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0300_ADV_ONLY, 0, NULL);
        }
    }

}

void tc_300_dump_result(void)
{
    if (p_tc_300_mgr != NULL)
    {
        APP_PRINT_INFO2("tc 300 sut: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_300_mgr->total_test_count, p_tc_300_mgr->total_connect_count);

        data_uart_print("tc 300 sut: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_300_mgr->total_test_count, p_tc_300_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}


void tc_300_add_case(uint32_t max_count)
{

    T_TC_300_IN_PARAM_DATA *p_tc_300_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_300_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_300_IN_PARAM_DATA));

    p_tc_300_param_data->id = TC_0300_ADV_ONLY;
    p_tc_300_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_300_param_data;

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
} TC_301_MGR;
TC_301_MGR *p_tc_301_mgr = NULL;

void tc_301_adv_disc_start(uint32_t count)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);

    if (NULL == p_tc_301_mgr)
    {
        p_tc_301_mgr = (TC_301_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_301_MGR));
    }
    else
    {
        memset(p_tc_301_mgr, 0, sizeof(TC_301_MGR));
    }
    p_tc_301_mgr->total_test_count = count;


    le_adv_start();
}



void tc_301_adv_disc_link_connected(uint8_t conn_id)
{

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    reset_vendor_counter();
    g_test_end_time = read_vendor_counter_no_display();

    p_tc_301_mgr->total_connect_count++;
    os_delay(200);
    le_disconnect(conn_id);
}

void tc_301_adv_disc_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    g_test_end_time = read_vendor_counter_no_display();

    tc_check_local_disc_reason(TC_0301_ADV_DISC, reason);
    tc_update_disc_reason(reason);

#if 0
    if (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
    {
        data_uart_print(
            "tc_301_adv_disc_link_disconnected fail: reason 0x%4x, begin time= %d,end time=%d,elapase time=%dus, conn_interval: %d,conn_latency: %d, conn_supervision_timeout: %d\r\n",
            reason,
            g_test_begin_time,
            g_test_end_time,
            (g_test_end_time - g_test_begin_time) / TEST_CPU_CLOCK,
            conn_interval,
            conn_latency,
            conn_supervision_timeout);
    }
#endif

    if (p_tc_301_mgr->total_connect_count < p_tc_301_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_301_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0301_ADV_DISC, 0, NULL);
        }
    }
}

void tc_301_dump_result(void)
{
    if (p_tc_301_mgr != NULL)
    {
        APP_PRINT_INFO2("tc_301_dump_result: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_301_mgr->total_test_count, p_tc_301_mgr->total_connect_count);

        data_uart_print("tc_301_dump_result: end: total_test_count %d total_connect_count %d\r\n",
                        p_tc_301_mgr->total_test_count, p_tc_301_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_301_add_case(uint32_t max_count)
{
    T_TC_301_IN_PARAM_DATA *p_tc_301_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_301_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_301_IN_PARAM_DATA));

    p_tc_301_param_data->id = TC_0301_ADV_DISC;
    p_tc_301_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_301_param_data;

    os_queue_in(&tc_q, p_tc_param);


}
#endif

