
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

#include <os_mem.h>
#include <ble_auto_test_case.h>
#include <os_mem.h>

#include <tc_common.h>
#include <tc_800.h>

#if TC_800_SUPPORT

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t disc_cmpl_fail_cnt; \
    uint32_t total_pair_start_count;
    uint32_t total_pair_success_count;
    uint32_t total_pair_fail_count;
} TC_800_MGR;

TC_800_MGR *p_tc_800_mgr = NULL;


void tc_800_iop_android_legacl_pair_start(uint32_t count)
{

    uint8_t secReqEnable = true;
    if (NULL == p_tc_800_mgr)
    {
        p_tc_800_mgr = (TC_800_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_800_MGR));
    }
    else
    {
        memset(p_tc_800_mgr, 0, sizeof(TC_800_MGR));
    }

    p_tc_800_mgr->total_test_count = count;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_800_iop_android_legacl_pair_link_disconnected(uint8_t conn_id, uint16_t reason)
{

    if ((reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) &&
        (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        p_tc_800_mgr->disc_cmpl_fail_cnt++;
        APP_PRINT_ERROR2("tc_600_iop_android_legacl_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                         reason, p_tc_800_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc_600_iop_android_legacl_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                        reason, p_tc_800_mgr->disc_cmpl_fail_cnt);

    }

    if (p_tc_800_mgr->total_connect_count < p_tc_800_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_800_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0600_IOP_PAIR_LEGACL, 0, NULL);
        }
    }
}

void tc_800_iop_android_legacl_pair_link_connected(uint8_t conn_id)
{
    p_tc_800_mgr->total_connect_count++;
    APP_PRINT_INFO1("tc_800_iop_android_legacl_pair_link_connected total_connect_count %d\r\n",
                    p_tc_800_mgr->total_connect_count);
}

void tc_800_iop_android_legacl_pair_state_to_start(uint8_t conn_id)
{

    p_tc_800_mgr->total_pair_start_count++;
}
void tc_800_iop_android_legacl_pair_state_to_success(uint8_t conn_id)
{
    p_tc_800_mgr->total_pair_success_count++;
}

void tc_800_iop_android_legacl_pair_state_to_fail(uint8_t conn_id, uint16_t reason)
{
    p_tc_800_mgr->total_pair_fail_count++;
    APP_PRINT_ERROR2("tc_800_iop_android_legacl_pair_state_to_fail: reason 0x%04x, total_connect_count %d\r\n",
                     reason, p_tc_800_mgr->total_connect_count);

    data_uart_print("tc_800_iop_android_legacl_pair_state_to_fail: reason 0x%04x, total_connect_count %d\r\n",
                    reason, p_tc_800_mgr->total_connect_count);
}

void tc_800_dump_result(void)
{
    if (p_tc_800_mgr != NULL)
    {
        APP_PRINT_INFO6("tc_800_dump_result: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d disc_cmpl_fail_cnt %d\r\n",
                        p_tc_800_mgr->total_test_count, p_tc_800_mgr->total_connect_count,
                        p_tc_800_mgr->total_pair_start_count, p_tc_800_mgr->total_pair_success_count,
                        p_tc_800_mgr->total_pair_fail_count, p_tc_800_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc_800_dump_result: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d disc_cmpl_fail_cnt %d\r\n",
                        p_tc_800_mgr->total_test_count, p_tc_800_mgr->total_connect_count,
                        p_tc_800_mgr->total_pair_start_count, p_tc_800_mgr->total_pair_success_count,
                        p_tc_800_mgr->total_pair_fail_count, p_tc_800_mgr->disc_cmpl_fail_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_800_add_case(uint32_t max_count)
{
    T_TC_800_IN_PARAM_DATA *p_tc_800_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_800_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_800_IN_PARAM_DATA));

    p_tc_800_param_data->id = TC_0800_IOP_PAIR_LEGACL;
    p_tc_800_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_800_param_data;

    os_queue_in(&tc_q, p_tc_param);

}
#endif

/****************************************************************************
TC
****************************************************************************/

#if TC_801_SUPPORT
typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t disc_cmpl_fail_cnt;
    uint32_t total_pair_start_count;
    uint32_t total_pair_success_count;
    uint32_t total_pair_fail_count;
} TC_801_MGR;

TC_801_MGR *p_tc_801_mgr = NULL;

void tc_801_iop_android_sc_pair_start(uint32_t count)
{
    uint8_t sec_req_enable = true;
    uint16_t sec_req_auth = GAP_AUTHEN_BIT_MITM_FLAG;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &sec_req_auth);

    if (NULL == p_tc_801_mgr)
    {
        p_tc_801_mgr = (TC_801_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_801_MGR));
    }
    else
    {
        memset(p_tc_801_mgr, 0, sizeof(TC_801_MGR));
    }

    p_tc_801_mgr->total_test_count = count;

    le_adv_start();
}

void tc_801_iop_android_sc_pair_link_disconnected(uint8_t conn_id, uint16_t reason)
{

    if ((reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE)) &&
        (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
    {
        p_tc_801_mgr->disc_cmpl_fail_cnt++;
        APP_PRINT_ERROR2("tc_801_iop_android_sc_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                         reason, p_tc_801_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc_801_iop_android_sc_pair_link_disconnected: reason 0x%04x, disc_cmpl_fail_cnt %d\r\n",
                        reason, p_tc_801_mgr->disc_cmpl_fail_cnt);

    }


    if (p_tc_801_mgr->total_connect_count < p_tc_801_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_801_dump_result();

        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0601_IOP_PAIR_SC, 0, NULL);
        }
    }
}

void tc_801_iop_android_sc_pair_link_connected(uint8_t conn_id)
{
    p_tc_801_mgr->total_connect_count++;
    APP_PRINT_INFO1("tc_801_iop_android_sc_pair_link_connected total_connect_count %d\r\n",
                    p_tc_801_mgr->total_connect_count);

}

void tc_801_iop_android_sc_pair_state_to_start(uint8_t conn_id)
{

    p_tc_801_mgr->total_pair_start_count++;
}
void tc_801_iop_android_sc_pair_state_to_success(uint8_t conn_id)
{
    p_tc_801_mgr->total_pair_success_count++;
}

void tc_801_iop_android_sc_pair_state_to_fail(uint8_t conn_id)
{
    p_tc_801_mgr->total_pair_fail_count++;

    APP_PRINT_INFO1("tc_801_iop_android_sc_pair_state_to_fail at total_connect_count %d\r\n",
                    p_tc_801_mgr->total_connect_count);

    data_uart_print("tc_801_iop_android_sc_pair_state_to_fail at total_connect_count %d\r\n",
                    p_tc_801_mgr->total_connect_count);

}

void tc_801_dump_result(void)
{
    if (p_tc_801_mgr != NULL)
    {
        APP_PRINT_INFO6("tc_801_dump_result: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d disc_cmpl_fail_cnt %d\r\n",
                        p_tc_801_mgr->total_test_count, p_tc_801_mgr->total_connect_count,
                        p_tc_801_mgr->total_pair_start_count, p_tc_801_mgr->total_pair_success_count,
                        p_tc_801_mgr->total_pair_fail_count, p_tc_801_mgr->disc_cmpl_fail_cnt);

        data_uart_print("tc_801_dump_result: total_test_count %d total_connect_count %d total_pair_start_count %d"
                        "total_pair_success_count %d total_pair_fail_count %d, disc_cmpl_fail_cnt %d\r\n",
                        p_tc_801_mgr->total_test_count, p_tc_801_mgr->total_connect_count,
                        p_tc_801_mgr->total_pair_start_count, p_tc_801_mgr->total_pair_success_count,
                        p_tc_801_mgr->total_pair_fail_count, p_tc_801_mgr->disc_cmpl_fail_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_801_add_case(uint32_t max_count)
{
    T_TC_801_IN_PARAM_DATA *p_tc_801_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_801_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_801_IN_PARAM_DATA));

    p_tc_801_param_data->id = TC_0801_IOP_PAIR_SC;
    p_tc_801_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_801_param_data;

    os_queue_in(&tc_q, p_tc_param);

}

#endif

