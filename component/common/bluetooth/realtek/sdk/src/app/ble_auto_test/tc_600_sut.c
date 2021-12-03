
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_bond_le.h"

#include "user_cmd.h"
#include "user_cmd_parse.h"


#include "os_sched.h"
#include <os_mem.h>
#include <ble_auto_test_case.h>
#include <os_mem.h>

#include <tc_common.h>
#include <tc_600_sut.h>

#if TC_600_SUT_SUPPORT

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_pair_start_count;
    uint32_t total_pair_success_count;
    uint32_t total_pair_fail_count;
    uint8_t  remote_bd[6];
} TC_600_SUT_MGR;

TC_600_SUT_MGR *p_tc_600_sut_mgr = NULL;


void tc_600_sut_iop_android_legacl_pair_start(uint32_t count, uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 10000;

    uint8_t sec_req_enable = true;
    uint16_t sec_req_auth = GAP_AUTHEN_BIT_MITM_FLAG |
                            GAP_AUTHEN_BIT_FORCE_BONDING_FLAG;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_auth);


    if (NULL == p_tc_600_sut_mgr)
    {
        p_tc_600_sut_mgr = (TC_600_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_600_SUT_MGR));
    }
    else
    {
        memset(p_tc_600_sut_mgr, 0, sizeof(TC_600_SUT_MGR));
    }

    p_tc_600_sut_mgr->total_test_count = count;
    memcpy(p_tc_600_sut_mgr->remote_bd, remote_bd, 6);

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);


    le_connect(0, remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );

}

void tc_600_sut_iop_android_legacl_pair_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_local_disc_reason(conn_id, reason);
    tc_update_disc_reason(reason);

    if (p_tc_600_sut_mgr->total_connect_count < p_tc_600_sut_mgr->total_test_count +
        g_ble_disconn_reason.reason_3e)
    {
        os_delay(200);

        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint16_t scan_timeout = 10000;

        le_connect(0, p_tc_600_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                   scan_timeout
                  );

    }
    else
    {
        tc_600_sut_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0600_IOP_PAIR_LEGACL, 0, NULL);
        }
    }
}
void tc_600_sut_iop_android_legacl_pair_link_connected(uint8_t conn_id)
{
    p_tc_600_sut_mgr->total_connect_count++;
    APP_PRINT_INFO1("tc_600_sut_iop_android_legacl_pair_link_connected total_connect_count %d\r\n",
                    p_tc_600_sut_mgr->total_connect_count);
    //initiate pairing
    //le_bond_pair(conn_id);
}

void tc_600_sut_iop_android_legacl_pair_state_to_start(uint8_t conn_id)
{

    p_tc_600_sut_mgr->total_pair_start_count++;
}
void tc_600_sut_iop_android_legacl_pair_state_to_success(uint8_t conn_id)
{
    p_tc_600_sut_mgr->total_pair_success_count++;
    os_delay(1000);
    le_disconnect(conn_id);
}

void tc_600_sut_iop_android_legacl_pair_state_to_fail(uint8_t conn_id, uint16_t reason)
{
    p_tc_600_sut_mgr->total_pair_fail_count++;
    APP_PRINT_INFO2("tc_600_sut_iop_android_legacl_pair_state_to_fail: reason 0x%04x, total_connect_count %d\r\n",
                    reason, p_tc_600_sut_mgr->total_connect_count);

    data_uart_print("tc_600_iop_android_legacl_pair_state_to_fail: reason 0x%04x, total_connect_count %d\r\n",
                    reason, p_tc_600_sut_mgr->total_connect_count);

}

void tc_600_sut_dump_result(void)
{
    if (p_tc_600_sut_mgr != NULL)
    {
        APP_PRINT_INFO5("tc 600: end: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d\r\n",
                        p_tc_600_sut_mgr->total_test_count, p_tc_600_sut_mgr->total_connect_count,
                        p_tc_600_sut_mgr->total_pair_start_count, p_tc_600_sut_mgr->total_pair_success_count,
                        p_tc_600_sut_mgr->total_pair_fail_count);

        data_uart_print("tc 600: end: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d\r\n",
                        p_tc_600_sut_mgr->total_test_count, p_tc_600_sut_mgr->total_connect_count,
                        p_tc_600_sut_mgr->total_pair_start_count, p_tc_600_sut_mgr->total_pair_success_count,
                        p_tc_600_sut_mgr->total_pair_fail_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_600_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_600_SUT_IN_PARAM_DATA *p_tc_600_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_600_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_600_SUT_IN_PARAM_DATA));

    p_tc_600_sut_param_data->id = TC_0600_IOP_PAIR_LEGACL;
    p_tc_600_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_600_sut_param_data->remote_bd, remote_bd, 6);


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_600_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif

/****************************************************************************
TC
****************************************************************************/

#if TC_601_SUT_SUPPORT

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_pair_start_count;
    uint32_t total_pair_success_count;
    uint32_t total_pair_fail_count;
    uint8_t  remote_bd[6];
} TC_601_MGR;

TC_601_MGR *p_tc_601_sut_mgr = NULL;

void tc_601_sut_iop_android_sc_pair_start(uint32_t count, uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 100;

    uint8_t sec_req_enable = false;
    uint16_t sec_req_auth = GAP_AUTHEN_BIT_MITM_FLAG | GAP_AUTHEN_BIT_SC_FLAG |
                            GAP_AUTHEN_BIT_FORCE_BONDING_FLAG;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_auth);
    gap_set_pairable_mode();

    if (NULL == p_tc_601_sut_mgr)
    {
        p_tc_601_sut_mgr = (TC_601_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_601_MGR));
    }
    else
    {
        memset(p_tc_601_sut_mgr, 0, sizeof(TC_601_MGR));
    }

    p_tc_601_sut_mgr->total_test_count = count;
    memcpy(p_tc_601_sut_mgr->remote_bd, remote_bd, 6);


    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    le_bond_clear_all_keys();

    le_connect(0, p_tc_601_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}

void tc_601_sut_iop_android_sc_pair_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_local_disc_reason(conn_id, reason);
    tc_update_disc_reason(reason);

    if (p_tc_601_sut_mgr->total_connect_count < p_tc_601_sut_mgr->total_test_count +
        g_ble_disconn_reason.reason_3e)
    {
        os_delay(200);

        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        T_GAP_LE_CONN_REQ_PARAM conn_req_param;
        uint16_t scan_timeout = 100;

        conn_req_param.scan_interval = 0x10;
        conn_req_param.scan_window = 0x10;
        conn_req_param.conn_interval_min = 6;
        conn_req_param.conn_interval_max = 6;
        conn_req_param.conn_latency = 0;
        conn_req_param.supv_tout = 300;
        conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
        conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
        le_bond_clear_all_keys();
        le_connect(0, p_tc_601_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                   scan_timeout
                  );

    }
    else
    {
        tc_600_sut_dump_result();

        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0601_IOP_PAIR_SC, 0, NULL);
        }
    }
}

void tc_601_sut_iop_android_sc_pair_link_connected(uint8_t conn_id)
{
    p_tc_601_sut_mgr->total_connect_count++;
    APP_PRINT_INFO1("tc_601_iop_android_sc_pair_link_connected total_connect_count %d\r\n",
                    p_tc_601_sut_mgr->total_connect_count);

    //initiate pairing
    //le_bond_pair(conn_id);

}

void tc_601_sut_iop_android_sc_pair_state_to_start(uint8_t conn_id)
{

    p_tc_601_sut_mgr->total_pair_start_count++;
}
void tc_601_sut_iop_android_sc_pair_state_to_success(uint8_t conn_id)
{
    p_tc_601_sut_mgr->total_pair_success_count++;
    os_delay(1000);
    le_disconnect(conn_id);
}

void tc_601_sut_iop_android_sc_pair_state_to_fail(uint8_t conn_id)
{
    p_tc_601_sut_mgr->total_pair_fail_count++;

    APP_PRINT_INFO1("tc_601_iop_android_sc_pair_state_to_fail at total_connect_count %d\r\n",
                    p_tc_601_sut_mgr->total_connect_count);

    data_uart_print("tc_601_iop_android_sc_pair_state_to_fail at total_connect_count %d\r\n",
                    p_tc_601_sut_mgr->total_connect_count);

}

void tc_601_sut_dump_result(void)
{
    if (p_tc_601_sut_mgr != NULL)
    {
        APP_PRINT_INFO5("tc 601: end: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d\r\n",
                        p_tc_601_sut_mgr->total_test_count, p_tc_601_sut_mgr->total_connect_count,
                        p_tc_601_sut_mgr->total_pair_start_count, p_tc_601_sut_mgr->total_pair_success_count,
                        p_tc_601_sut_mgr->total_pair_fail_count);

        data_uart_print("tc 601: end: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d\r\n",
                        p_tc_601_sut_mgr->total_test_count, p_tc_601_sut_mgr->total_connect_count,
                        p_tc_601_sut_mgr->total_pair_start_count, p_tc_601_sut_mgr->total_pair_success_count,
                        p_tc_601_sut_mgr->total_pair_fail_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_601_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_601_SUT_IN_PARAM_DATA *p_tc_601_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_601_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_601_SUT_IN_PARAM_DATA));

    p_tc_601_sut_param_data->id = TC_0601_IOP_PAIR_SC;
    p_tc_601_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_601_sut_param_data->remote_bd, remote_bd, 6);


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_601_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}

#endif

