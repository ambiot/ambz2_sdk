#include <ble_auto_test_case.h>
#if TC_500_SUT_SUPPORT
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


#include <tc_common.h>
#include <tc_500_sut.h>

#include <vendor_pxpext_client.h>


typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_read_count;
    uint32_t total_write_count;
    uint8_t remote_bd[6];
} TC_500_SUT_MGR;

TC_500_SUT_MGR *p_tc_500_sut_mgr = NULL;


P_FUN_PROFILE_CLIENT_RESULT_CB tc_500_sut_client_result_cb;



void tc_50x_sut_client_result_callback(uint8_t conn_id, void *p_cb_data)
{
    T_PXP_CB_DATA *p_cb = (T_PXP_CB_DATA *)p_cb_data;
    APP_PRINT_INFO1("vendor_pxp_client_id: %d.", p_cb->cb_type);
    switch (p_cb->cb_type)
    {
    case PXP_CLIENT_CB_TYPE_DISC_RESULT:
        APP_PRINT_INFO2("PXP_CLIENT_CB_TYPE_DISC_RESULT: is_found %d, value_handle 0x%x",
                        p_cb->cb_content.disc_result.is_found,
                        p_cb->cb_content.disc_result.pxp_char.value_handle);
        if (p_cb->cb_content.disc_result.is_found)
        {
            uint8_t data[2] = {0x12, 0x34};
            vendor_pxpext_write_value(conn_id, sizeof(data), data);
        }
        break;

    case PXP_CLIENT_CB_TYPE_WRITE_RESULT:
        APP_PRINT_INFO1("PXP_CLIENT_CB_TYPE_WRITE_RESULT: cause 0x%x.",
                        p_cb->cb_content.write_result.cause);
        vendor_pxpext_read_value(conn_id);
        break;

    case PXP_CLIENT_CB_TYPE_READ_RESULT:
        APP_PRINT_INFO2("PXP_CLIENT_CB_TYPE_READ_RESULT: cause 0x%x, value_size %d",
                        p_cb->cb_content.read_result.cause,
                        p_cb->cb_content.read_result.value_size);
        if (p_cb->cb_content.read_result.cause == GAP_CAUSE_SUCCESS)
        {
            uint8_t data[2] = {0x12, 0x34};
            if ((p_cb->cb_content.read_result.value_size == sizeof(data)) &&
                (0 == memcmp(p_cb->cb_content.read_result.pValue, data, sizeof(data))))
            {
                os_delay(1000);
                le_disconnect(conn_id);
            }
        }
        break;
    default:
        break;
    }
}



//TC_0500_SLAVE_AUTO_ADV
void tc_500_sut_start(uint32_t count, uint8_t remote_bd[6])
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

    if (NULL == p_tc_500_sut_mgr)
    {
        p_tc_500_sut_mgr = (TC_500_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_500_SUT_MGR));
    }
    else
    {
        memset(p_tc_500_sut_mgr, 0, sizeof(TC_500_SUT_MGR));
    }
    p_tc_500_sut_mgr->total_test_count = count;
    memcpy(p_tc_500_sut_mgr->remote_bd, remote_bd, 6);

    le_connect(0, p_tc_500_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}

void tc_500_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_local_disc_reason(TC_0500_SLAVE_AUTO_ADV, reason);
    tc_update_disc_reason(reason);

    if (p_tc_500_sut_mgr->total_connect_count < p_tc_500_sut_mgr->total_test_count +
        g_ble_disconn_reason.reason_3e)
    {
        os_delay(200);

        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint16_t scan_timeout = 1000;
        le_connect(0, p_tc_500_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                   scan_timeout
                  );
    }
    else
    {
        tc_500_sut_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0500_SLAVE_AUTO_ADV, 0, NULL);
        }
    }
}

void tc_500_sut_link_connected(uint8_t conn_id)
{
    p_tc_500_sut_mgr->total_connect_count++;
    //gatt write /read
    vendor_pxpext_start_discovery(conn_id);

}

void tc_500_sut_dump_result(void)
{
    if (p_tc_500_sut_mgr != NULL)
    {

        APP_PRINT_INFO4("tc_500_sut_dump_result: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_500_sut_mgr->total_test_count, p_tc_500_sut_mgr->total_connect_count,
                        p_tc_500_sut_mgr->total_read_count, p_tc_500_sut_mgr->total_write_count);

        data_uart_print("tc_500_sut_dump_result: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_500_sut_mgr->total_test_count, p_tc_500_sut_mgr->total_connect_count,
                        p_tc_500_sut_mgr->total_read_count, p_tc_500_sut_mgr->total_write_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }


}
void tc_500_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_500_SUT_IN_PARAM_DATA *p_tc_500_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_500_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_500_SUT_IN_PARAM_DATA));

    p_tc_500_sut_param_data->id = TC_0500_SLAVE_AUTO_ADV;
    p_tc_500_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_500_sut_param_data->remote_bd, remote_bd, 6);


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_500_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}



//TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ
/***/
typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_read_count;
    uint32_t total_write_count;
    uint8_t remote_bd[6];
} TC_501_SUT_MGR;

TC_501_SUT_MGR *p_tc_501_sut_mgr = NULL;

void tc_501_sut_start(uint32_t count, uint8_t remote_bd[6])
{
    uint8_t secReqEnable = true;
    uint16_t scan_timeout = 1000;
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;

    if (NULL == p_tc_501_sut_mgr)
    {
        p_tc_501_sut_mgr = (TC_501_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_501_SUT_MGR));
    }
    else
    {
        memset(p_tc_501_sut_mgr, 0, sizeof(TC_501_SUT_MGR));
    }

    p_tc_501_sut_mgr->total_test_count = count;
    memcpy(p_tc_501_sut_mgr->remote_bd, remote_bd, 6);


    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);

    le_connect(0, remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}

void tc_501_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_local_disc_reason(TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ, reason);
    tc_update_disc_reason(reason);

    if (p_tc_501_sut_mgr->total_connect_count < p_tc_501_sut_mgr->total_test_count +
        g_ble_disconn_reason.reason_3e)
    {
        os_delay(200);

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

        le_connect(0, p_tc_501_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                   scan_timeout
                  );

    }
    else
    {
        tc_501_sut_dump_result();

        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ, 0, NULL);
        }
    }
}

void tc_501_sut_link_connected(uint8_t conn_id)
{
    p_tc_501_sut_mgr->total_connect_count ++;
    //gatt write/read
    vendor_pxpext_start_discovery(conn_id);
}

void tc_501_sut_dump_result(void)
{
    if (p_tc_501_sut_mgr != NULL)
    {
        APP_PRINT_INFO4("tc_501_sut_dump_result: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_501_sut_mgr->total_test_count, p_tc_501_sut_mgr->total_connect_count,
                        p_tc_501_sut_mgr->total_read_count, p_tc_501_sut_mgr->total_write_count);

        data_uart_print("tc_501_sut_dump_result: end: total_test_count %d total_connect_count %d total_read_count %d"
                        "total_write_count %d\r\n",
                        p_tc_501_sut_mgr->total_test_count, p_tc_501_sut_mgr->total_connect_count,
                        p_tc_501_sut_mgr->total_read_count, p_tc_501_sut_mgr->total_write_count);

    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_501_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_501_SUT_IN_PARAM_DATA *p_tc_501_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_501_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_501_SUT_IN_PARAM_DATA));

    p_tc_501_sut_param_data->id = TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ;
    p_tc_501_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_501_sut_param_data->remote_bd, remote_bd, 6);


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_501_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif

