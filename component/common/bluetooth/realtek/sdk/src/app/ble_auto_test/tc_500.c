
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

#include <tc_common.h>
#include <tc_500.h>

#if TC_500_SUPPORT

typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_read_count;
    uint32_t total_write_count;
} TC_500_MGR;

TC_500_MGR *p_tc_500_mgr = NULL;


/***/
typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_read_count;
    uint32_t total_write_count;
} TC_501_MGR;

TC_501_MGR *p_tc_501_mgr = NULL;


//TC_0500_SLAVE_AUTO_ADV
void tc_500_salve_auto_adv_start(uint32_t count)
{
    uint8_t secReqEnable = false;

    if (NULL == p_tc_500_mgr)
    {
        p_tc_500_mgr = (TC_500_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_500_MGR));
    }
    else
    {
        memset(p_tc_500_mgr, 0, sizeof(TC_500_MGR));
    }

    p_tc_500_mgr->total_test_count = count;

    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_500_salve_auto_adv_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_0500_SLAVE_AUTO_ADV, reason);
    tc_update_disc_reason(reason);

    if (p_tc_500_mgr->total_connect_count < p_tc_500_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_500_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0500_SLAVE_AUTO_ADV, 0, NULL);
        }
    }
}

void tc_500_salve_auto_adv_link_connected(uint8_t conn_id)
{
    p_tc_500_mgr->total_connect_count++;


}

void tc_500_dump_result(void)
{
    if (p_tc_500_mgr != NULL)
    {

        APP_PRINT_INFO4("tc 500: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_500_mgr->total_test_count, p_tc_500_mgr->total_connect_count,
                        p_tc_500_mgr->total_read_count, p_tc_500_mgr->total_write_count);

        data_uart_print("tc 500: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_500_mgr->total_test_count, p_tc_500_mgr->total_connect_count,
                        p_tc_500_mgr->total_read_count, p_tc_500_mgr->total_write_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }


}

void tc_500_add_case(uint32_t max_count)
{
    T_TC_500_IN_PARAM_DATA *p_tc_500_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_500_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_500_IN_PARAM_DATA));

    p_tc_500_param_data->id = TC_0500_SLAVE_AUTO_ADV;
    p_tc_500_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_500_param_data;

    os_queue_in(&tc_q, p_tc_param);

}

//TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ
void tc_501_salve_auto_adv_with_sec_req_start(uint32_t count)
{
    uint8_t secReqEnable = true;

    if (NULL == p_tc_501_mgr)
    {
        p_tc_501_mgr = (TC_501_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_501_MGR));
    }
    else
    {
        memset(p_tc_501_mgr, 0, sizeof(TC_501_MGR));
    }

    p_tc_501_mgr->total_test_count = count;

    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_501_salve_auto_adv_with_sec_req_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ, reason);
    tc_update_disc_reason(reason);

    if (p_tc_501_mgr->total_connect_count < p_tc_501_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_501_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ, 0, NULL);
        }
    }
}

void tc_501_salve_auto_adv_with_sec_req_link_connected(uint8_t conn_id)
{
    p_tc_501_mgr->total_connect_count ++;
}

void tc_501_dump_result(void)
{
    if (p_tc_501_mgr != NULL)
    {
        APP_PRINT_INFO4("tc 501: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_501_mgr->total_test_count, p_tc_501_mgr->total_connect_count,
                        p_tc_501_mgr->total_read_count, p_tc_501_mgr->total_write_count);

        data_uart_print("tc 501: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_501_mgr->total_test_count, p_tc_501_mgr->total_connect_count,
                        p_tc_501_mgr->total_read_count, p_tc_501_mgr->total_write_count);

    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_501_add_case(uint32_t max_count)
{
    T_TC_501_IN_PARAM_DATA *p_tc_501_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_501_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_501_IN_PARAM_DATA));

    p_tc_501_param_data->id = TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ;
    p_tc_501_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_501_param_data;

    os_queue_in(&tc_q, p_tc_param);

}


typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_read_count;
    uint32_t total_write_count;
} TC_502_MGR;

TC_502_MGR *p_tc_502_mgr = NULL;

//TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV
void tc_502_salve_auto_adv_start(uint32_t count)
{
    uint8_t secReqEnable = false;

    if (NULL == p_tc_502_mgr)
    {
        p_tc_502_mgr = (TC_502_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_502_MGR));
    }
    else
    {
        memset(p_tc_502_mgr, 0, sizeof(TC_502_MGR));
    }

    p_tc_502_mgr->total_test_count = count;

    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_adv_start();
}

void tc_502_salve_auto_adv_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV, reason);
    tc_update_disc_reason(reason);

    if (p_tc_502_mgr->total_connect_count < p_tc_502_mgr->total_test_count)
    {

        le_adv_start();
    }
    else
    {
        tc_502_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV, 0, NULL);
        }
    }
}

void tc_502_salve_auto_adv_link_connected(uint8_t conn_id)
{
    p_tc_502_mgr->total_connect_count++;

    le_adv_start();

}

void tc_502_dump_result(void)
{
    if (p_tc_502_mgr != NULL)
    {

        APP_PRINT_INFO4("tc 502: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_502_mgr->total_test_count, p_tc_502_mgr->total_connect_count,
                        p_tc_502_mgr->total_read_count, p_tc_502_mgr->total_write_count);

        data_uart_print("tc 502: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_502_mgr->total_test_count, p_tc_502_mgr->total_connect_count,
                        p_tc_502_mgr->total_read_count, p_tc_502_mgr->total_write_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }


}

void tc_502_add_case(uint32_t max_count)
{
    T_TC_502_IN_PARAM_DATA *p_tc_502_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_502_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_502_IN_PARAM_DATA));

    p_tc_502_param_data->id = TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV;
    p_tc_502_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_502_param_data;

    os_queue_in(&tc_q, p_tc_param);

}

#endif

