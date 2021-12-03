
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
#include <tc_310.h>

#if TC_310_SUPPORT


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
} TC_310_MGR;
TC_310_MGR *p_tc_310_mgr = NULL;

typedef struct
{
    uint8_t all_phys;
    uint8_t tx_phys;
    uint8_t rx_phys;
    T_GAP_PHYS_OPTIONS phy_options;
} TC_310_PHY_ARRAY;

typedef struct
{
    T_GAP_PHYS_TYPE tx_phy;
    T_GAP_PHYS_TYPE rx_phy;
} TC_310_PHY_ARRAY_RESULT;

TC_310_PHY_ARRAY tc_31x_phy_array[6] =
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

uint8_t tc_31x_phy_array_index = 0;


const TC_310_PHY_ARRAY_RESULT tc_31x_phy_array_result[6] =
{
    {GAP_PHYS_1M, GAP_PHYS_1M},
    {GAP_PHYS_1M, GAP_PHYS_1M},
    {GAP_PHYS_1M, GAP_PHYS_2M},
    {GAP_PHYS_CODED, GAP_PHYS_CODED},
    {GAP_PHYS_CODED, GAP_PHYS_CODED},
    {GAP_PHYS_CODED, GAP_PHYS_CODED}
};

void tc_310_adv_only_start(uint32_t count)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);

    if (NULL == p_tc_310_mgr)
    {
        p_tc_310_mgr = (TC_310_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_310_MGR));
    }
    else
    {
        memset(p_tc_310_mgr, 0, sizeof(TC_310_MGR));
    }
    p_tc_310_mgr->total_test_count = count;


    le_adv_start();
}



void tc_310_adv_only_link_connected(uint8_t conn_id)
{

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    p_tc_310_mgr->total_connect_count++;

    le_set_phy(conn_id, tc_31x_phy_array[tc_31x_phy_array_index].all_phys,
               tc_31x_phy_array[tc_31x_phy_array_index].tx_phys,
               tc_31x_phy_array[tc_31x_phy_array_index].rx_phys,
               tc_31x_phy_array[tc_31x_phy_array_index].phy_options);


}

void tc_310_phy_update_evt(uint8_t conn_id, uint16_t cause, T_GAP_PHYS_TYPE tx_phy,
                           T_GAP_PHYS_TYPE rx_phy)
{
    if ((cause == GAP_SUCCESS) &&
        (tc_31x_phy_array_result[tc_31x_phy_array_index].tx_phy == tx_phy) &&
        (tc_31x_phy_array_result[tc_31x_phy_array_index].rx_phy == rx_phy))
    {
        tc_31x_phy_array_index++;
        if (tc_31x_phy_array_index < (sizeof(tc_31x_phy_array_result) / sizeof(tc_31x_phy_array_result[0])))
        {
            le_set_phy(conn_id, tc_31x_phy_array[tc_31x_phy_array_index].all_phys,
                       tc_31x_phy_array[tc_31x_phy_array_index].tx_phys,
                       tc_31x_phy_array[tc_31x_phy_array_index].rx_phys,
                       tc_31x_phy_array[tc_31x_phy_array_index].phy_options);
        }
        else
        {
            /*one round test complete, start next round, wait for link disconnected*/
            tc_31x_phy_array_index = 0;
        }

    }
    else
    {

    }
}

void tc_310_adv_only_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    g_test_end_time = read_vendor_counter_no_display();

    tc_check_remote_disc_reason(TC_0310_2M_LONGRANGE_1, reason);
    tc_update_disc_reason(reason);

    if (p_tc_310_mgr->total_connect_count < p_tc_310_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_310_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0310_2M_LONGRANGE_1, 0, NULL);
        }
    }

}

void tc_310_dump_result(void)
{
    if (p_tc_310_mgr != NULL)
    {
        APP_PRINT_INFO2("tc_310_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_310_mgr->total_test_count, p_tc_310_mgr->total_connect_count);

        data_uart_print("tc_310_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_310_mgr->total_test_count, p_tc_310_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}


void tc_310_add_case(uint32_t max_count)
{

    T_TC_310_IN_PARAM_DATA *p_tc_310_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_310_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_310_IN_PARAM_DATA));

    p_tc_310_param_data->id = TC_0310_2M_LONGRANGE_1;
    p_tc_310_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_310_param_data;

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
} TC_311_MGR;
TC_311_MGR *p_tc_311_mgr = NULL;

void tc_311_adv_disc_start(uint32_t count)
{
    uint8_t secReqEnable = false;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);

    if (NULL == p_tc_311_mgr)
    {
        p_tc_311_mgr = (TC_311_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_311_MGR));
    }
    else
    {
        memset(p_tc_311_mgr, 0, sizeof(TC_311_MGR));
    }
    p_tc_311_mgr->total_test_count = count;


    le_adv_start();
}



void tc_311_adv_disc_link_connected(uint8_t conn_id)
{

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    reset_vendor_counter();
    g_test_end_time = read_vendor_counter_no_display();

    p_tc_311_mgr->total_connect_count++;

    le_disconnect(conn_id);
}

void tc_311_phy_update_evt(uint8_t conn_id, uint16_t cause, T_GAP_PHYS_TYPE tx_phy,
                           T_GAP_PHYS_TYPE rx_phy)
{
    if ((cause == GAP_SUCCESS) &&
        (tc_31x_phy_array_result[tc_31x_phy_array_index].tx_phy == tx_phy) &&
        (tc_31x_phy_array_result[tc_31x_phy_array_index].rx_phy == rx_phy))
    {
        tc_31x_phy_array_index++;
        if (tc_31x_phy_array_index < sizeof(tc_31x_phy_array_result) / sizeof(tc_31x_phy_array_result[0]))
        {

        }
        else
        {
            /*one round test complete, start next round, wait for link disconnected*/
            tc_31x_phy_array_index = 0;
        }

    }
    else
    {
        APP_PRINT_ERROR6("tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d"
                         "cause 0x%04x: tx_phy %d rx_phy %d index %d\r\n",
                         p_tc_311_mgr->total_test_count, p_tc_311_mgr->total_connect_count,
                         cause, tx_phy, rx_phy, tc_31x_phy_array_index);

        data_uart_print("!!!tc_310_sut_phy_update_evt: total_test_count %d total_connect_count %d"
                        "cause 0x%04x: tx_phy %d rx_phy %d index %d\r\n",
                        p_tc_311_mgr->total_test_count, p_tc_311_mgr->total_connect_count,
                        cause, tx_phy, rx_phy, tc_31x_phy_array_index);
    }
}

void tc_311_adv_disc_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_0311_2M_LONGRANGE_2, reason);
    tc_update_disc_reason(reason);

    if (p_tc_311_mgr->total_connect_count < p_tc_311_mgr->total_test_count)
    {
        le_adv_start();
    }
    else
    {
        tc_311_dump_result();
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0311_2M_LONGRANGE_2, 0, NULL);
        }
    }
}

void tc_311_dump_result(void)
{
    if (p_tc_311_mgr != NULL)
    {
        APP_PRINT_INFO2("tc_311_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_311_mgr->total_test_count, p_tc_311_mgr->total_connect_count);

        data_uart_print("tc_311_dump_result: total_test_count %d total_connect_count %d\r\n",
                        p_tc_311_mgr->total_test_count, p_tc_311_mgr->total_connect_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_311_add_case(uint32_t max_count)
{
    T_TC_311_IN_PARAM_DATA *p_tc_311_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_311_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_311_IN_PARAM_DATA));

    p_tc_311_param_data->id = TC_0311_2M_LONGRANGE_2;
    p_tc_311_param_data->total_test_count = max_count;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_311_param_data;

    os_queue_in(&tc_q, p_tc_param);


}

#endif

