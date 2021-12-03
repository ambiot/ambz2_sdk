
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "ble_auto_test_application.h"

#include "user_cmd.h"
#include "user_cmd_parse.h"
#if F_BT_LE_GATT_SERVER_SUPPORT
#include "profile_server.h"
#endif

#include <os_mem.h>
#include <ble_auto_test_case.h>


void *g_test_timer_handle = NULL;
uint32_t  g_test_begin_time = 0;
uint32_t  g_test_end_time = 0;



uint16_t conn_interval = 0;
uint16_t conn_latency = 0;
uint16_t conn_supervision_timeout = 0;

BLE_DISCONN_REASON g_ble_disconn_reason;


P_FUN_TC_RESULT_CB p_tc_result_cb = NULL;

void tc_check_remote_disc_reason(uint16_t case_id, uint16_t reason)
{
    if (reason != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
    {
        data_uart_print("WARNING: tc %d: disc reason 0x%04x\r\n", case_id, reason);
    }
}

void tc_check_local_disc_reason(uint16_t case_id, uint16_t reason)
{
    if (reason != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
    {
        data_uart_print("WARNING: tc %d: disc reason 0x%04x\r\n", case_id, reason);
    }
}


void tc_update_disc_reason(uint16_t reason)
{
    switch (reason)
    {
    case HCI_ERR_CONN_TIMEOUT|HCI_ERR:
        g_ble_disconn_reason.reason_08++;
        break;

    case HCI_ERR_REMOTE_USER_TERMINATE|HCI_ERR:
        g_ble_disconn_reason.reason_13++;
        break;

    case HCI_ERR_REMOTE_POWER_OFF|HCI_ERR:
        g_ble_disconn_reason.reason_15++;
        break;

    case HCI_ERR_LOCAL_HOST_TERMINATE|HCI_ERR:
        g_ble_disconn_reason.reason_16++;
        break;

    case HCI_ERR_LMP_RESPONSE_TIMEOUT|HCI_ERR:
        g_ble_disconn_reason.reason_22++;
        break;

    case HCI_ERR_INSTANT_PASSED|HCI_ERR:
        g_ble_disconn_reason.reason_28++;
        break;

    case HCI_ERR_MIC_FAILURE|HCI_ERR:
        g_ble_disconn_reason.reason_3d++;
        break;

    case HCI_ERR_FAIL_TO_ESTABLISH_CONN|HCI_ERR:
        g_ble_disconn_reason.reason_3e++;
        break;

    default:
        g_ble_disconn_reason.reason_others++;
        break;
    }
}


void tc_dump_disc_reason(void)
{
    APP_PRINT_ERROR8("disc_reason: err_08  %d, err_13  %d, err_16  %d, err_22  %d, err_28  %d, err_3d  %d, err_3e  %d, err_others %d",
                    g_ble_disconn_reason.reason_08,
                    g_ble_disconn_reason.reason_13,
                    g_ble_disconn_reason.reason_16,
                    g_ble_disconn_reason.reason_22,
                    g_ble_disconn_reason.reason_28,
                    g_ble_disconn_reason.reason_3d,
                    g_ble_disconn_reason.reason_3e,
                    g_ble_disconn_reason.reason_others);
    data_uart_print("disc_reason: err_08, err_13, err_15, err_16, err_22, err_28, err_3d, err_3e, err_others\r\n");
    data_uart_print("              %d,       %d,     %d,     %d,       %d,      %d,     %d,      %d,         %d\r\n",
                    g_ble_disconn_reason.reason_08,
                    g_ble_disconn_reason.reason_13,
                    g_ble_disconn_reason.reason_15,
                    g_ble_disconn_reason.reason_16,
                    g_ble_disconn_reason.reason_22,
                    g_ble_disconn_reason.reason_28,
                    g_ble_disconn_reason.reason_3d,
                    g_ble_disconn_reason.reason_3e,
                    g_ble_disconn_reason.reason_others);
}


void tc_reg_result_callback(P_FUN_TC_RESULT_CB tc_result_cb)
{
    p_tc_result_cb = tc_result_cb;
}


