
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
#include <os_timer.h>
#include <ble_auto_test_case.h>
#include "privacy_mgnt.h"


#include <tc_common.h>
#include <tc_700.h>


#if TC_700_SUPPORT

void *g_tc_stable_notification_tx_timer_handle = NULL;


void tc_700_stable_notification_tx_01_start(void)
{
    le_adv_start();
}


void tc_700_stable_notification_tx_01_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    le_adv_start();
}


void tc_700_stable_notification_tx_01_link_connected(uint8_t conn_id)
{

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
    data_uart_print(
        "TC_0700_STABLE_NOTIFICATION_TX_01: conn_interval: %d,conn_latency: %d, conn_supervision_timeout: %d\r\n",
        conn_interval,
        conn_latency,
        conn_supervision_timeout);
}

void tc_stable_notification_TimeoutHandler(void *pxTimer)
{
    uint8_t value[244] = {0};
    uint32_t conn_id;
    os_timer_id_get(&g_tc_stable_notification_tx_timer_handle, &conn_id);
    APP_PRINT_INFO1("tc_stable_notification_TimeoutHandler conn_id = %d", conn_id);

    vendor_tp_service_v1_notification((uint8_t)conn_id, gSimpleProfileServiceId, value, 20);
}

void tc_stable_indication_tx_start_auto_notification_timer(uint8_t conn_id,
                                                           uint16_t connection_interval)
{
    if (NULL == g_tc_stable_notification_tx_timer_handle)
    {

        os_timer_create(&g_tc_stable_notification_tx_timer_handle, "tc_stable_indication_timer", conn_id,
                        (connection_interval * 125) / 100, true, tc_stable_notification_TimeoutHandler);
    }

    os_timer_start(&g_tc_stable_notification_tx_timer_handle);

}
void tc_700_stable_notification_tx_01_cccd_enable(uint8_t conn_id, bool bEnable)
{
    if (bEnable)
    {
#if 0
        uint16_t con_interval = 12;
        uint16_t conn_slave_latency = 4;
        uint16_t conn_supervision_timeout = 300;
        le_update_conn_param(conn_id,
                             con_interval,
                             con_interval,
                             conn_slave_latency,
                             conn_supervision_timeout,
                             2 * (con_interval - 1),
                             2 * (con_interval - 1),
                             0);

#endif

        uint16_t con_interval;
        uint16_t conn_slave_latency;
        uint16_t conn_supervision_timeout;

        le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
        le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
        le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

        APP_PRINT_INFO4("tc_700_stable_notification_tx_01_cccd_enable, conn_id = %d, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                        conn_id, con_interval, conn_slave_latency, conn_supervision_timeout);

        tc_stable_indication_tx_start_auto_notification_timer(conn_id, con_interval);

    }
    else
    {
        if (g_tc_stable_notification_tx_timer_handle != NULL)
        {
            os_timer_stop(&g_tc_stable_notification_tx_timer_handle);
        }
    }

}

void tc_700_stable_notification_tx_01_tx_data_complete(uint8_t credits)
{
    //do nothing
}


void tc_700_stable_notification_tx_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO4("tc_700_stable_notification_tx_conn_param_update_event conn_id = %d, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    conn_id, con_interval, conn_slave_latency, conn_supervision_timeout);

    tc_stable_indication_tx_start_auto_notification_timer(conn_id, con_interval);
}


#endif

