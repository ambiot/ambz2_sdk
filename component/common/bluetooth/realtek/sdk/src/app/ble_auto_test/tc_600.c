
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

#include <os_mem.h>
#include <ble_auto_test_case.h>
#include "privacy_mgnt.h"
#include <os_mem.h>

#include <tc_common.h>
#include <tc_600.h>
#include "hids_kb.h"


#if TC_600_SUPPORT

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t disc_cmpl_fail_cnt;
    uint32_t total_pair_start_count;
    uint32_t total_pair_success_count;
    uint32_t total_pair_fail_count;
} TC_600_MGR;

TC_600_MGR *p_tc_600_mgr = NULL;


void tc_600_iop_android_legacl_pair_start(uint32_t count)
{
    // hids_add_service(app_profile_callback);

    uint8_t secReqEnable = true;
    if (NULL == p_tc_600_mgr)
    {
        p_tc_600_mgr = (TC_600_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_600_MGR));
    }
    else
    {
        memset(p_tc_600_mgr, 0, sizeof(TC_600_MGR));
    }
    memset(&g_ble_disconn_reason, 0, sizeof(g_ble_disconn_reason));
    p_tc_600_mgr->total_test_count = count;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_600_iop_android_legacl_pair_link_disconnected(uint8_t conn_id, uint16_t reason)
{

    tc_check_remote_disc_reason(TC_0600_IOP_PAIR_LEGACL, reason);
    tc_update_disc_reason(reason);

    if ((reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) &&
        (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        p_tc_600_mgr->disc_cmpl_fail_cnt++;
        APP_PRINT_ERROR2("tc_600_iop_android_legacl_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                         reason, p_tc_600_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc_600_iop_android_legacl_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                        reason, p_tc_600_mgr->disc_cmpl_fail_cnt);

    }

    if (p_tc_600_mgr->total_connect_count < p_tc_600_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_600_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0600_IOP_PAIR_LEGACL, 0, NULL);
        }
    }
}

void tc_600_iop_android_legacl_pair_link_connected(uint8_t conn_id)
{
    p_tc_600_mgr->total_connect_count++;
    APP_PRINT_INFO1("tc_600_iop_android_legacl_pair_link_connected total_connect_count %d\r\n",
                    p_tc_600_mgr->total_connect_count);
}

void tc_600_iop_android_legacl_pair_state_to_start(uint8_t conn_id)
{

    p_tc_600_mgr->total_pair_start_count++;
}
void tc_600_iop_android_legacl_pair_state_to_success(uint8_t conn_id)
{
    p_tc_600_mgr->total_pair_success_count++;
}

void tc_600_iop_android_legacl_pair_state_to_fail(uint8_t conn_id, uint16_t reason)
{
    p_tc_600_mgr->total_pair_fail_count++;
    APP_PRINT_ERROR2("tc_600_iop_android_legacl_pair_state_to_fail: reason 0x%04x, total_connect_count %d\r\n",
                     reason, p_tc_600_mgr->total_connect_count);

    data_uart_print("tc_600_iop_android_legacl_pair_state_to_fail: reason 0x%04x, total_connect_count %d\r\n",
                    reason, p_tc_600_mgr->total_connect_count);
}

void tc_600_dump_result(void)
{
    if (p_tc_600_mgr != NULL)
    {
        APP_PRINT_INFO6("tc 600: end: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d disc_cmpl_fail_cnt %d\r\n",
                        p_tc_600_mgr->total_test_count, p_tc_600_mgr->total_connect_count,
                        p_tc_600_mgr->total_pair_start_count, p_tc_600_mgr->total_pair_success_count,
                        p_tc_600_mgr->total_pair_fail_count, p_tc_600_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc 600: end: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d disc_cmpl_fail_cnt %d\r\n",
                        p_tc_600_mgr->total_test_count, p_tc_600_mgr->total_connect_count,
                        p_tc_600_mgr->total_pair_start_count, p_tc_600_mgr->total_pair_success_count,
                        p_tc_600_mgr->total_pair_fail_count, p_tc_600_mgr->disc_cmpl_fail_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_600_add_case(uint32_t max_count)
{
    T_TC_600_IN_PARAM_DATA *p_tc_600_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_600_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_600_IN_PARAM_DATA));

    p_tc_600_param_data->id = TC_0600_IOP_PAIR_LEGACL;
    p_tc_600_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_600_param_data;

    os_queue_in(&tc_q, p_tc_param);

}
#endif

/****************************************************************************
TC
****************************************************************************/

#if TC_601_SUPPORT
typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t disc_cmpl_fail_cnt;
    uint32_t total_pair_start_count;
    uint32_t total_pair_success_count;
    uint32_t total_pair_fail_count;
} TC_601_MGR;

TC_601_MGR *p_tc_601_mgr = NULL;

void tc_601_iop_android_sc_pair_start(uint32_t count)
{
    uint8_t sec_req_enable = true;
    uint16_t sec_req_auth = GAP_AUTHEN_BIT_MITM_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;

    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_auth);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_pairable_mode();

    if (NULL == p_tc_601_mgr)
    {
        p_tc_601_mgr = (TC_601_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_601_MGR));
    }
    else
    {
        memset(p_tc_601_mgr, 0, sizeof(TC_601_MGR));
    }

    memset(&g_ble_disconn_reason, 0, sizeof(g_ble_disconn_reason));
    p_tc_601_mgr->total_test_count = count;

    le_adv_start();
}

void tc_601_iop_android_sc_pair_link_disconnected(uint8_t conn_id, uint16_t reason)
{

    tc_check_remote_disc_reason(TC_0600_IOP_PAIR_LEGACL, reason);
    tc_update_disc_reason(reason);

    if ((reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) &&
        (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        p_tc_601_mgr->disc_cmpl_fail_cnt++;
        APP_PRINT_ERROR2("tc_601_iop_android_sc_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                         reason, p_tc_601_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc_601_iop_android_sc_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                        reason, p_tc_601_mgr->disc_cmpl_fail_cnt);

    }

    if (p_tc_601_mgr->total_connect_count < p_tc_601_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_601_dump_result();

        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0601_IOP_PAIR_SC, 0, NULL);
        }
    }
}

void tc_601_iop_android_sc_pair_link_connected(uint8_t conn_id)
{
    p_tc_601_mgr->total_connect_count++;
    APP_PRINT_INFO1("tc_601_iop_android_sc_pair_link_connected total_connect_count %d\r\n",
                    p_tc_601_mgr->total_connect_count);

}

void tc_601_iop_android_sc_pair_state_to_start(uint8_t conn_id)
{

    p_tc_601_mgr->total_pair_start_count++;
}
void tc_601_iop_android_sc_pair_state_to_success(uint8_t conn_id)
{
    p_tc_601_mgr->total_pair_success_count++;
}

void tc_601_iop_android_sc_pair_state_to_fail(uint8_t conn_id)
{
    p_tc_601_mgr->total_pair_fail_count++;

    APP_PRINT_INFO1("tc_601_iop_android_sc_pair_state_to_fail at total_connect_count %d\r\n",
                    p_tc_601_mgr->total_connect_count);

    data_uart_print("tc_601_iop_android_sc_pair_state_to_fail at total_connect_count %d\r\n",
                    p_tc_601_mgr->total_connect_count);

}

void tc_601_dump_result(void)
{
    if (p_tc_601_mgr != NULL)
    {
        APP_PRINT_INFO6("tc_601_dump_result: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d disc_cmpl_fail_cnt %d\r\n",
                        p_tc_601_mgr->total_test_count, p_tc_601_mgr->total_connect_count,
                        p_tc_601_mgr->total_pair_start_count, p_tc_601_mgr->total_pair_success_count,
                        p_tc_601_mgr->total_pair_fail_count, p_tc_601_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc_601_dump_result: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d, disc_cmpl_fail_cnt %d\r\n",
                        p_tc_601_mgr->total_test_count, p_tc_601_mgr->total_connect_count,
                        p_tc_601_mgr->total_pair_start_count, p_tc_601_mgr->total_pair_success_count,
                        p_tc_601_mgr->total_pair_fail_count, p_tc_601_mgr->disc_cmpl_fail_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_601_add_case(uint32_t max_count)
{
    T_TC_601_IN_PARAM_DATA *p_tc_601_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_601_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_601_IN_PARAM_DATA));

    p_tc_601_param_data->id = TC_0601_IOP_PAIR_SC;
    p_tc_601_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_601_param_data;

    os_queue_in(&tc_q, p_tc_param);

}

#endif

