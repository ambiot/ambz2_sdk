
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
#include <tc_310_sut.h>


#if TC_310_SUT_SUPPORT

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

} TC_310_SUT_MGR;

static TC_310_SUT_MGR *p_tc_310_sut_mgr = NULL;

typedef struct
{
    uint8_t all_phys;
    uint8_t tx_phys;
    uint8_t rx_phys;
    T_GAP_PHYS_OPTIONS phy_options;
} TC_310_SUT_PHY_ARRAY;

typedef struct
{
    T_GAP_PHYS_TYPE tx_phy;
    T_GAP_PHYS_TYPE rx_phy;
} TC_310_SUT_PHY_ARRAY_RESULT;

TC_310_SUT_PHY_ARRAY tc_31x_sut_phy_array[6] =
{
    {
        GAP_PHYS_NO_PREFER_TX_BIT |
        GAP_PHYS_NO_PREFER_RX_BIT,

        GAP_PHYS_PREFER_1M_BIT |
        GAP_PHYS_PREFER_2M_BIT |
        GAP_PHYS_PREFER_CODED_BIT,

        GAP_PHYS_PREFER_1M_BIT |
        GAP_PHYS_PREFER_2M_BIT |
        GAP_PHYS_PREFER_CODED_BIT,

        GAP_PHYS_OPTIONS_CODED_PREFER_NO
    },

    {
        0,
        GAP_PHYS_PREFER_1M_BIT,
        GAP_PHYS_PREFER_1M_BIT,
        GAP_PHYS_OPTIONS_CODED_PREFER_NO
    },

    {
        0,
        GAP_PHYS_PREFER_2M_BIT,
        GAP_PHYS_PREFER_2M_BIT,
        GAP_PHYS_OPTIONS_CODED_PREFER_NO
    },

    {
        0,
        GAP_PHYS_PREFER_CODED_BIT,
        GAP_PHYS_PREFER_CODED_BIT,
        GAP_PHYS_OPTIONS_CODED_PREFER_NO
    },

    {
        0,
        GAP_PHYS_PREFER_CODED_BIT,
        GAP_PHYS_PREFER_CODED_BIT,
        GAP_PHYS_OPTIONS_CODED_PREFER_S2
    },

    {
        0,
        GAP_PHYS_PREFER_CODED_BIT,
        GAP_PHYS_PREFER_CODED_BIT,
        GAP_PHYS_OPTIONS_CODED_PREFER_S8
    }
};

uint8_t tc_31x_sut_phy_array_index = 0;


const TC_310_SUT_PHY_ARRAY_RESULT tc_31x_sut_phy_array_result[6] =
{
    {GAP_PHYS_1M, GAP_PHYS_1M},
    {GAP_PHYS_1M, GAP_PHYS_1M},
    {GAP_PHYS_1M, GAP_PHYS_2M},
    {GAP_PHYS_CODED, GAP_PHYS_CODED},
    {GAP_PHYS_CODED, GAP_PHYS_CODED},
    {GAP_PHYS_CODED, GAP_PHYS_CODED}
};


void tc_310_sut_start(uint32_t count, uint8_t remote_bd[6])
{
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 1000;

    if (NULL == p_tc_310_sut_mgr)
    {
        p_tc_310_sut_mgr = (TC_310_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_310_SUT_MGR));
    }
    else
    {
        memset(p_tc_310_sut_mgr, 0, sizeof(TC_310_SUT_MGR));
    }
    p_tc_310_sut_mgr->total_test_count = count;
    memcpy(p_tc_310_sut_mgr->remote_bd, remote_bd, 6);

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    le_connect(0, p_tc_310_sut_mgr->remote_bd, remote_bd_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout
              );

}

void tc_310_sut_link_connected(uint8_t conn_id)
{
    APP_PRINT_TRACE1("tc_310_sut_link_connected: conn_id %d", conn_id);
    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    p_tc_310_sut_mgr->total_connect_count++;

}

void tc_310_sut_phy_update_evt(uint8_t conn_id, uint16_t cause, T_GAP_PHYS_TYPE tx_phy,
                               T_GAP_PHYS_TYPE rx_phy)
{
    if ((cause == GAP_SUCCESS) &&
        (tc_31x_sut_phy_array_result[tc_31x_sut_phy_array_index].tx_phy == tx_phy) &&
        (tc_31x_sut_phy_array_result[tc_31x_sut_phy_array_index].rx_phy == rx_phy))
    {
        tc_31x_sut_phy_array_index++;
        if (tc_31x_sut_phy_array_index < sizeof(tc_31x_sut_phy_array_result) / sizeof(
                tc_31x_sut_phy_array_result[0]))
        {
            APP_PRINT_INFO6("tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d"
                            "cause 0x%04x tx_phy %d rx_phy %d index %d\r\n",
                            p_tc_310_sut_mgr->total_test_count,
                            p_tc_310_sut_mgr->total_connect_count,
                            cause,
                            tx_phy,
                            rx_phy,
                            tc_31x_sut_phy_array_index);
        }
        else
        {
            /*one round test complete, start next round, disconect link */
            tc_31x_sut_phy_array_index = 0;
            le_disconnect(conn_id);
        }
    }
    else
    {
        APP_PRINT_ERROR6("tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d"
                         "cause 0x%04x: tx_phy %d rx_phy %d index %d\r\n",
                         p_tc_310_sut_mgr->total_test_count, p_tc_310_sut_mgr->total_connect_count,
                         cause, tx_phy, rx_phy, tc_31x_sut_phy_array_index);

        data_uart_print("!!!tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d"
                        "cause 0x%04x: tx_phy %d rx_phy %d index %d\r\n",
                        p_tc_310_sut_mgr->total_test_count, p_tc_310_sut_mgr->total_connect_count,
                        cause, tx_phy, rx_phy, tc_31x_sut_phy_array_index);
    }
}

void tc_310_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    APP_PRINT_TRACE2("tc_300_sut_link_disconnected: conn_id %d, reason 0x%04x", conn_id, reason);

    tc_check_local_disc_reason(TC_0310_2M_LONGRANGE_1, reason);
    tc_update_disc_reason(reason);

    {
        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint16_t scan_timeout = 1000;

        if (p_tc_310_sut_mgr->total_connect_count < p_tc_310_sut_mgr->total_test_count +
            g_ble_disconn_reason.reason_3e)
        {
            os_delay(200);
            le_connect(0, p_tc_310_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                       scan_timeout
                      );
        }
        else
        {
            tc_310_sut_dump_result();
            if (p_tc_result_cb != NULL)
            {
                p_tc_result_cb(TC_0310_2M_LONGRANGE_1, 0, NULL);
            }
        }
    }
}
void tc_310_sut_dump_result(void)
{
    if (p_tc_310_sut_mgr != NULL)
    {
        APP_PRINT_INFO2("tc_300_sut_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_310_sut_mgr->total_test_count, p_tc_310_sut_mgr->total_connect_count);

        data_uart_print("tc_300_sut_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_310_sut_mgr->total_test_count, p_tc_310_sut_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_310_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_310_SUT_IN_PARAM_DATA *p_tc_310_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_310_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_310_SUT_IN_PARAM_DATA));

    p_tc_310_sut_param_data->id = TC_0310_2M_LONGRANGE_1;
    p_tc_310_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_310_sut_param_data->remote_bd, remote_bd, 6);

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_310_sut_param_data;

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
} TC_311_SUT_MGR;

static TC_311_SUT_MGR *p_tc_311_sut_mgr = NULL;
void tc_311_sut_start(uint32_t count, uint8_t remote_bd[6])
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

    if (NULL == p_tc_311_sut_mgr)
    {
        p_tc_311_sut_mgr = (TC_311_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_311_SUT_MGR));
    }
    else
    {
        memset(p_tc_311_sut_mgr, 0, sizeof(TC_311_SUT_MGR));
    }

    p_tc_311_sut_mgr->total_test_count = count;


    memcpy(p_tc_311_sut_mgr->remote_bd, remote_bd, 6);
    le_connect(0, p_tc_311_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
               scan_timeout
              );
}


void tc_311_sut_link_connected(uint8_t conn_id)
{

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    reset_vendor_counter();
    g_test_end_time = read_vendor_counter_no_display();
    p_tc_311_sut_mgr->total_connect_count++;
}

void tc_311_sut_phy_update_evt(uint8_t conn_id, uint16_t cause, T_GAP_PHYS_TYPE tx_phy,
                               T_GAP_PHYS_TYPE rx_phy)
{
    if ((cause == GAP_SUCCESS) &&
        (tc_31x_sut_phy_array_result[tc_31x_sut_phy_array_index].tx_phy == tx_phy) &&
        (tc_31x_sut_phy_array_result[tc_31x_sut_phy_array_index].rx_phy == rx_phy))
    {
        tc_31x_sut_phy_array_index++;
        if (tc_31x_sut_phy_array_index < sizeof(tc_31x_sut_phy_array_result) / sizeof(
                tc_31x_sut_phy_array_result[0]))
        {
            APP_PRINT_INFO6("tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d"
                            "cause 0x%04x: tx_phy %d rx_phy %d index %d\r\n",
                            p_tc_310_sut_mgr->total_test_count,
                            p_tc_310_sut_mgr->total_connect_count,
                            cause,
                            tx_phy,
                            rx_phy,
                            tc_31x_sut_phy_array_index);

            le_set_phy(conn_id, tc_31x_sut_phy_array[tc_31x_sut_phy_array_index].all_phys,
                       tc_31x_sut_phy_array[tc_31x_sut_phy_array_index].tx_phys,
                       tc_31x_sut_phy_array[tc_31x_sut_phy_array_index].rx_phys,
                       tc_31x_sut_phy_array[tc_31x_sut_phy_array_index].phy_options);

        }
        else
        {
            /*one round test complete, start next round, disconect link */
            tc_31x_sut_phy_array_index = 0;
            le_disconnect(conn_id);
        }
    }
    else
    {
        APP_PRINT_ERROR6("tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d"
                         "cause 0x%04x: tx_phy %d rx_phy %d index %d\r\n",
                         p_tc_310_sut_mgr->total_test_count, p_tc_310_sut_mgr->total_connect_count,
                         cause, tx_phy, rx_phy, tc_31x_sut_phy_array_index);

        data_uart_print("!!!tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d",
                        "cause 0x%04x: tx_phy %d rx_phy %d index %d\r\n",
                        p_tc_310_sut_mgr->total_test_count, p_tc_310_sut_mgr->total_connect_count,
                        cause, tx_phy, rx_phy, tc_31x_sut_phy_array_index);
    }
}

void tc_311_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    g_test_end_time = read_vendor_counter_no_display();
    if (reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE) &&
        reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
    {
        data_uart_print(
            "tc_311_sut_link_disconnected fail: reason 0x%4x, begin time= %d,end time=%d,elapase time=%dus, conn_interval: %d,conn_latency: %d, conn_supervision_timeout: %d\r\n",
            reason,
            g_test_begin_time,
            g_test_end_time,
            (g_test_end_time - g_test_begin_time) / TEST_CPU_CLOCK,
            conn_interval,
            conn_latency,
            conn_supervision_timeout);
    }

    {
        T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint16_t scan_timeout = 1000;

        if (p_tc_311_sut_mgr->total_connect_count < p_tc_311_sut_mgr->total_test_count +
            g_ble_disconn_reason.reason_3e)
        {
            os_delay(200);
            le_connect(0, p_tc_311_sut_mgr->remote_bd, remote_bd_type, GAP_LOCAL_ADDR_LE_PUBLIC,
                       scan_timeout
                      );
        }
        else
        {
            tc_310_sut_dump_result();
            if (p_tc_result_cb != NULL)
            {
                p_tc_result_cb(TC_0311_2M_LONGRANGE_2, 0, NULL);
            }
        }
    }

}

void tc_311_sut_dump_result(void)
{
    if (p_tc_311_sut_mgr != NULL)
    {
        APP_PRINT_INFO2("tc_301_sut_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_311_sut_mgr->total_test_count, p_tc_311_sut_mgr->total_connect_count);

        data_uart_print("tc_301_sut_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_311_sut_mgr->total_test_count, p_tc_311_sut_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_311_sut_add_case(uint32_t max_count, uint8_t remote_bd[6])
{
    T_TC_311_SUT_IN_PARAM_DATA *p_tc_311_sut_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_311_sut_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_311_SUT_IN_PARAM_DATA));

    p_tc_311_sut_param_data->id = TC_0311_2M_LONGRANGE_2;
    p_tc_311_sut_param_data->total_test_count = max_count;
    memcpy(p_tc_311_sut_param_data->remote_bd, remote_bd, 6);


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_311_sut_param_data;

    os_queue_in(&tc_q, p_tc_param);
}

#endif
