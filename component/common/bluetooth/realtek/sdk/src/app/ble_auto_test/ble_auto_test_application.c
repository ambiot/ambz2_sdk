
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include <os_mem.h>
#include "gap_scan.h"
#include "gap.h"
#include "gap_adv.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_auto_test_application.h"
#include "link_mgr.h"
#include "user_cmd.h"
#include "user_cmd_parse.h"

#if F_BT_LE_GATT_SERVER_SUPPORT
#include "profile_server.h"
#include "vendor_tp_service.h"
#endif

#include <ble_auto_test_case.h>

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
#include "gap_adv.h"
#endif

#if F_BT_LE_PRIVACY_SUPPORT
#include "privacy_mgnt.h"
#endif

#if F_BT_LE_GATT_CLIENT_SUPPORT
#include <gaps_client.h>
#include <vendor_tp_client.h>
#include <vendor_pxpext_client.h>
#include <simple_ble_client.h>
#endif

#if F_BT_LE_5_0_AE_SCAN_SUPPORT
#include <gap_ext_scan.h>
#endif

#if F_BT_LE_5_0_AE_ADV_SUPPORT
#include <gap_ext_adv.h>
#endif

#if F_BT_LE_4_1_CBC_SUPPORT
#include "gap_credit_based_conn.h"
#endif

bool V3NotifyEnable = false;
/* Client ID of Simple BLE Client Module, generated when add this module in main.c. Can't be modified by user. */
/* Device state maintained in application. */
T_GAP_DEV_STATE gap_dev_state = {0, 0, 0, 0, 0};
extern uint8_t simple_srv_id;
#if F_BT_LE_GATT_CLIENT_SUPPORT
extern T_CLIENT_ID   GapClientID;
extern T_CLIENT_ID   vendor_tp_client_id;
extern T_CLIENT_ID   vendor_pxp_client_id;
#endif
// GAP - SCAN RSP data (max size = 31 bytes)
const uint8_t scan_rsp_data[] =
{
    0x03,           /* length     */
    0x03,           /* type="More 16-bit UUIDs available" */
    0x12,
    0x18,
    0x03,           /* length     */
    0x19,           /* type="Appearance" */
    0xc2, 0x03,     /* Mouse */
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
const uint8_t adv_data[] =
{
    0x02,            /* length     */
    //XXXXMJMJ 0x01, 0x06,      /* type="flags", data="bit 1: LE General Discoverable Mode", BR/EDR not supp. */
    0x01, 0x05,      /* type="flags", data="bit 1: LE General Discoverable Mode" */
    /* Service */
    0x03,           /* length     */
    0x03,           /* type="More 16-bit UUIDs available" */
    0x12,
    0x18,
    /* place holder for Local Name, filled by BT stack. if not present */
    /* BT stack appends Local Name.                                    */
    0x03,           /* length     */
    0x19,           /* type="Appearance" */
    0xc2, 0x03,     /* Mouse */
    0x0C,           /* length     */
    0x09,           /* type="Complete local name" */
//    0x42, 0x65, 0x65, 0x5F, 0x6D, 0x6F, 0x75, 0x73, 0x65  /* Bee_perip */
    'B', 'e', 'e', '_', 'G', 'a', 'p', 'T', 'e', 's', 't' /* Bee_perip */
};


void app_handle_gap_msg(T_IO_MSG *pBeeIoMsg);

/**
* @brief  All the application events are pre-handled in this function.
*
* All the IO MSGs are sent to this function.
* Then the event handling function shall be called according to the MSG type.
*
* @param   io_msg  The T_IO_MSG from peripherals or BT stack state machine.
* @return  void
*/
void app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;
    uint8_t rx_char;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        app_handle_gap_msg(&io_msg);
        break;
    case IO_MSG_TYPE_UART:
        /* We handle user command informations from Data UART in this branch. */
        rx_char = (uint8_t)io_msg.subtype;
        user_cmd_collect(&user_cmd_if, &rx_char, sizeof(rx_char), user_cmd_table);
        break;
    default:
        break;
    }
}

/**
  * @brief  handle messages indicate that GAP device state has changed.
  * @param  new_state: GAP state.
  * @retval none
  */
void app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO4("app_handle_dev_state_evt: init state %d scan state %d adv state %d conn state %d",
                    new_state.gap_init_state,
                    new_state.gap_scan_state, new_state.gap_adv_state, new_state.gap_conn_state);
    if (gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO0("GAP stack ready");
            uint8_t bt_addr[6];
            gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
            data_uart_print("local bd addr: 0x%2x:%2x:%2x:%2x:%2x:%2x\r\n",
                            bt_addr[5], bt_addr[4], bt_addr[3],
                            bt_addr[2], bt_addr[1], bt_addr[0]);

            /*stack ready*/
        }
    }

    if (gap_dev_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");

#if TC_100_SUPPORT
            if (app_get_cur_test_case() == TC_0100_SCAN_PERFORMANCE)
            {
                tc_0100_scan_state_change_to_idle();
            }
            else if (app_get_cur_test_case() == TC_0101_SCAN_STRESS_ENABLE_DISABLE)
            {
                tc_0101_scan_stress_enable_disable_state_change_to_idle();
            }
#endif

#if TC_1200_SUPPORT
            if (app_get_cur_test_case() == TC_1200_MULTI_LINK_4_MASTER)
            {
                tc_1200_scan_state_change_to_idle();
            }
#endif

#if TC_1201_SUPPORT
            if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
            {
                tc_1201_scan_state_change_to_idle();
            }
#endif
#if TC_1202_SUPPORT
            if (app_get_cur_test_case() == TC_1202_MULTI_LINK_4_MASTER)
            {
                tc_1202_scan_state_change_to_idle();
            }
#endif
#if TC_1203_SUPPORT
            if (app_get_cur_test_case() == TC_1203_MULTI_LINK_4_MASTER)
            {
                tc_1203_scan_state_change_to_idle();
            }
#endif
#if TC_1204_SUPPORT
            if (app_get_cur_test_case() == TC_1204_MULTI_LINK_4_MASTER)
            {
                tc_1204_scan_state_change_to_idle();
            }
#endif
#if TC_1205_SUPPORT
            if (app_get_cur_test_case() == TC_1205_MULTI_LINK_4_MASTER)
            {
                tc_1205_scan_state_change_to_idle();
            }
#endif
#if TC_1206_SUPPORT
            if (app_get_cur_test_case() == TC_1206_MULTI_LINK_4_MASTER)
            {
                tc_1206_scan_state_change_to_idle();
            }
#endif
#if TC_1207_SUPPORT
            if (app_get_cur_test_case() == TC_1207_MULTI_LINK_4_MASTER)
            {
                tc_1207_scan_state_change_to_idle();
            }
#endif

        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");

#if TC_100_SUPPORT

            if (app_get_cur_test_case() == TC_0100_SCAN_PERFORMANCE)
            {
                tc_0100_scan_state_change_to_scaning();
            }
            else if (app_get_cur_test_case() == TC_0101_SCAN_STRESS_ENABLE_DISABLE)
            {
                tc_0101_scan_stress_enable_disable_state_change_to_scaning();
            }
#endif
        }
    }

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
    if (gap_dev_state.gap_adv_state != new_state.gap_adv_state)
    {
        if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            if (app_get_cur_test_case() == TC_0001_ADV_PERFORMANCE)
            {
#if TC_0001_SUPPORT
                tc_0001_adv_adv_state_change_to_idle();
#endif
            }
            else if (app_get_cur_test_case() == TC_0002_ADV_STRESS_START_STOP)
            {
#if TC_0002_SUPPORT
                tc_0002_adv_start_stop_adv_state_change_to_idle();
#endif
            }
        }
        else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            if (app_get_cur_test_case() == TC_0001_ADV_PERFORMANCE)
            {
#if TC_0001_SUPPORT
                tc_0001_adv_adv_state_change_to_advertising();
#endif
            }
            else if (app_get_cur_test_case() == TC_0002_ADV_STRESS_START_STOP)
            {
#if TC_0002_SUPPORT
                tc_0002_adv_start_stop_adv_state_change_to_advertising();
#endif
            }

        }
    }
#endif

    if (gap_dev_state.gap_conn_state != new_state.gap_conn_state)
    {
        /*
            APP_PRINT_INFO2("Conn state: %d -> %d",
                       gap_dev_state.gap_conn_state,
                       new_state.gap_conn_state);
                       */
    }

#if TC_900_SUPPORT
#if F_BT_LE_PRIVACY_SUPPORT
    if ((app_get_cur_test_case() == TC_0900_PRIVACY_TEST_SLAVE)
        || (app_get_cur_test_case() == TC_0901_PRIVACY_TEST_MASTER))
    {
        if ((new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
            && (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
            && (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
            && (new_state.gap_conn_state == GAP_CONN_DEV_STATE_IDLE))
        {
            privacy_handle_pending_resolving_list();
        }
    }
#endif
#endif

    gap_dev_state = new_state;
}

/**
  * @brief  handle messages indicate that GAP connection state has changed.
  * @param  conn_id: connection ID.
  * @param  new_state: new connection state.
  * @param  disc_cause: when new_state=GAP_CONN_STATE_DISCONNECTED, this value is valid.
  * @retval none
  */
void app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    if (conn_id >= APP_MAX_LINKS)
    {
        return;
    }
    APP_PRINT_INFO3("app_handle_conn_state_evt: conn_id %d oldState %d new_state %d",
                    conn_id, app_link_table[conn_id].conn_state, new_state);

    app_link_table[conn_id].conn_state = new_state;
    switch (new_state)
    {
    /*device is disconnected.*/
    case GAP_CONN_STATE_DISCONNECTED:
        {
            APP_PRINT_INFO2("app_handle_conn_state_evt: conn_id %d disc_cause 0x%04x",
                            conn_id, disc_cause);
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
                if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
                {

                }
                else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
#if TC_206_SUPPORT
                    tc_206_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
#if TC_207_SUPPORT
                    tc_207_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                {
#if TC_208_SUPPORT
                    tc_208_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0300_ADV_ONLY)
                {
#if TC_300_SUPPORT
                    tc_300_adv_only_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0301_ADV_DISC)
                {
#if TC_301_SUPPORT
                    tc_301_adv_disc_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0400_CONN_PARAM_UPDATE_SLAVE)
                {
#if TC_400_SUPPORT
                    tc_400_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0401_CONN_PARAM_UPDATE_SLAVE_01)
                {
#if TC_401_SUPPORT
                    tc_401_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0500_SLAVE_AUTO_ADV)
                {
#if TC_500_SUPPORT
                    tc_500_salve_auto_adv_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ)
                {
#if TC_501_SUPPORT
                    tc_501_salve_auto_adv_with_sec_req_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV)
                {
#if TC_502_SUPPORT
                    tc_502_salve_auto_adv_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                {
#if TC_600_SUPPORT
                    tc_600_iop_android_legacl_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                {
#if TC_601_SUPPORT
                    tc_601_iop_android_sc_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
                {
#if TC_700_SUPPORT
                    tc_700_stable_notification_tx_01_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                {
#if TC_800_SUPPORT
                    tc_800_iop_android_legacl_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                {
#if TC_801_SUPPORT
                    tc_801_iop_android_sc_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0900_PRIVACY_TEST_SLAVE)
                {
                    le_adv_start();
                }
                else if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUPPORT
                    tc_1100_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUPPORT
                    tc_1101_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
                {
#if TC_1102_SUPPORT
                    tc_1102_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1200_MULTI_LINK_4_MASTER)
                {
#if TC_1200_SUPPORT
                    tc_1200_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
                {
#if TC_1201_SUPPORT
                    tc_1201_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1202_MULTI_LINK_4_MASTER)
                {
#if TC_1202_SUPPORT
                    tc_1202_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
                {
#if TC_1203_SUPPORT
                    tc_1203_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
                {
#if TC_1204_SUPPORT
                    tc_1204_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
                {
#if TC_1205_SUPPORT
                    tc_1205_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
                {
#if TC_1206_SUPPORT
                    tc_1206_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
                {
#if TC_1207_SUPPORT
                    tc_1207_link_disconnected(conn_id, disc_cause);
#endif
                }
                else
                {
                    data_uart_print("Disconnect conn_id %d\r\n", conn_id);
                }
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
                if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
                {
#if TC_200_SUT_SUPPORT
                    tc_200_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
#if TC_206_SUT_SUPPORT
                    tc_206_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
#if TC_207_SUT_SUPPORT
                    tc_207_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                {
#if TC_208_SUT_SUPPORT
                    tc_208_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0300_ADV_ONLY)
                {
#if TC_300_SUT_SUPPORT
                    tc_300_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0301_ADV_DISC)
                {
#if TC_301_SUT_SUPPORT
                    tc_301_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0400_CONN_PARAM_UPDATE_SLAVE)
                {
#if TC_400_SUT_SUPPORT
                    tc_400_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0401_CONN_PARAM_UPDATE_SLAVE_01)
                {
#if TC_401_SUT_SUPPORT
                    tc_401_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0500_SLAVE_AUTO_ADV)
                {
#if TC_500_SUT_SUPPORT
                    tc_500_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ)
                {
#if TC_501_SUT_SUPPORT
                    tc_501_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV)
                {
#if TC_502_SUT_SUPPORT

#endif
                }
                else if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                {
#if TC_600_SUT_SUPPORT
                    tc_600_sut_iop_android_legacl_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                {
#if TC_601_SUT_SUPPORT
                    tc_601_sut_iop_android_sc_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
                {
#if TC_700_SUPPORT

#endif
                }
#if F_BT_LE_5_0_SUPPORT
                else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                {
#if TC_800_SUT_SUPPORT
                    tc_800_sut_iop_android_legacl_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                {
#if TC_801_SUT_SUPPORT
                    tc_801_sut_iop_android_sc_pair_link_disconnected(conn_id, disc_cause);
#endif
                }
#endif
                else if (app_get_cur_test_case() == TC_0900_PRIVACY_TEST_SLAVE)
                {

                }
                else if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUT_SUPPORT
                    tc_1100_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUT_SUPPORT
                    tc_1101_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
                {
#if TC_1102_SUT_SUPPORT
                    tc_1102_sut_link_disconnected(conn_id, disc_cause);
#endif
                }
                else
                {
                    data_uart_print("Disconnect conn_id = %d\r\n", conn_id);
                }
            }


            if (disc_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT))
            {
                APP_PRINT_INFO1("Test_HandleBtConnStateChangeEvt: connection lost", conn_id);
            }
            memset(&app_link_table[conn_id], 0, sizeof(T_APP_LINK));
        }
        break;
    /*device is disconnected.*/
    case GAP_CONN_STATE_CONNECTING:
        {

        }
        break;

    /*device is connected*/
    case GAP_CONN_STATE_CONNECTED:
        {
            uint16_t mtu_size;
            le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, conn_id);
            data_uart_print("Connected conn_id = %d, mtu size = %d\r\n", conn_id, mtu_size);
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
                if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
                {
#if TC_200_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    tc_200_tp_notification_tx_init_default_param(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
#if TC_206_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    //tc_206_tp_notification_tx_init_default_param(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
#if TC_207_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                {
#if TC_208_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0300_ADV_ONLY)
                {
#if TC_300_SUPPORT
                    tc_300_adv_only_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0301_ADV_DISC)
                {
#if TC_301_SUPPORT
                    tc_301_adv_disc_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0400_CONN_PARAM_UPDATE_SLAVE)
                {
#if TC_400_SUPPORT
                    tc_400_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0401_CONN_PARAM_UPDATE_SLAVE_01)
                {
#if TC_401_SUPPORT
                    tc_401_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
                {
#if TC_700_SUPPORT
                    tc_700_stable_notification_tx_01_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0500_SLAVE_AUTO_ADV)
                {
#if TC_500_SUPPORT
                    tc_500_salve_auto_adv_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ)
                {
#if TC_501_SUPPORT
                    tc_501_salve_auto_adv_with_sec_req_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV)
                {
#if TC_502_SUPPORT
                    tc_502_salve_auto_adv_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                {

#if TC_600_SUPPORT
                    tc_600_iop_android_legacl_pair_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                {

#if TC_601_SUPPORT
                    tc_601_iop_android_sc_pair_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                {
#if TC_800_SUPPORT
                    tc_800_iop_android_legacl_pair_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                {
#if TC_801_SUPPORT
                    tc_801_iop_android_sc_pair_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUPPORT
                    tc_1100_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUPPORT
                    tc_1101_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1200_MULTI_LINK_4_MASTER)
                {
#if TC_1200_SUPPORT
                    tc_1200_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
                {
#if TC_1201_SUPPORT
                    tc_1201_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1202_MULTI_LINK_4_MASTER)
                {
#if TC_1202_SUPPORT
                    tc_1202_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1203_MULTI_LINK_4_MASTER)
                {
#if TC_1203_SUPPORT
                    tc_1203_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1204_MULTI_LINK_4_MASTER)
                {
#if TC_1204_SUPPORT
                    tc_1204_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1205_MULTI_LINK_4_MASTER)
                {
#if TC_1205_SUPPORT
                    tc_1205_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1206_MULTI_LINK_4_MASTER)
                {
#if TC_1206_SUPPORT
                    tc_1206_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1207_MULTI_LINK_4_MASTER)
                {
#if TC_1207_SUPPORT
                    tc_1207_link_connected(conn_id);
#endif
                }
                else
                {
                    data_uart_print("Conn conn_id = %d\r\n", conn_id);
                }
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
                if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
                {
#if TC_200_SUT_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    tc_200_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
#if TC_206_SUT_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    tc_206_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
#if TC_207_SUT_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    tc_207_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                {
#if TC_208_SUT_SUPPORT
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    tc_208_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0300_ADV_ONLY)
                {
#if TC_300_SUT_SUPPORT
                    tc_300_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0301_ADV_DISC)
                {
#if TC_301_SUT_SUPPORT
                    tc_301_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0400_CONN_PARAM_UPDATE_SLAVE)
                {
#if TC_400_SUT_SUPPORT
                    tc_400_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0401_CONN_PARAM_UPDATE_SLAVE_01)
                {
#if TC_401_SUT_SUPPORT
                    tc_401_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
                {

                }
                else if (app_get_cur_test_case() == TC_0500_SLAVE_AUTO_ADV)
                {
#if TC_500_SUT_SUPPORT
                    tc_500_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ)
                {
#if TC_501_SUT_SUPPORT
                    tc_501_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV)
                {
#if TC_502_SUT_SUPPORT
                    //tc_502_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                {
#if TC_600_SUT_SUPPORT
                    tc_600_sut_iop_android_legacl_pair_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                {
#if TC_601_SUT_SUPPORT
                    tc_601_sut_iop_android_sc_pair_link_connected(conn_id);
#endif
                }
#if F_BT_LE_5_0_SUPPORT
                else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                {
#if TC_800_SUT_SUPPORT
                    tc_800_sut_iop_android_legacl_pair_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                {
#if TC_801_SUT_SUPPORT
                    tc_801_sut_iop_android_sc_pair_link_connected(conn_id);
#endif
                }
#endif
                else if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUT_SUPPORT
                    tc_1100_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUT_SUPPORT
                    tc_1101_sut_link_connected(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
                {
#if TC_1102_SUT_SUPPORT
                    tc_1102_sut_link_connected(conn_id);
#endif
                }
                else
                {
                    data_uart_print("Conn conn_id %d\r\n", conn_id);
                }
            }
        }
        break;

    /*error comes here*/
    default:
        {

        }
        break;

    }
}

/**
  * @brief  handle messages indicate that GAP bond state has changed.
  * @param  conn_id: connection ID.
  * @param  new_state: GAP bond state.
  * @param  status: pairing complete status.
  *             @arg: 0 - success.
  *             @arg: other - failed.
  * @retval none
  */
void app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t status)
{
    APP_PRINT_INFO3("app_handle_authen_state_evt: conn_id %d, new_state %d, status 0x%x", conn_id,
                    new_state, status);
    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE:(GAP_AUTHEN_STATE_STARTED)");

            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
                if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                {
#if TC_600_SUT_SUPPORT
                    tc_600_iop_android_legacl_pair_state_to_start(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                {
#if TC_601_SUT_SUPPORT
                    tc_601_iop_android_sc_pair_state_to_start(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                {
#if TC_800_SUPPORT
                    tc_800_iop_android_legacl_pair_state_to_start(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                {
#if TC_801_SUPPORT
                    tc_801_iop_android_sc_pair_state_to_start(conn_id);
#endif
                }
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
                if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                {
#if TC_600_SUT_SUPPORT
                    tc_600_sut_iop_android_legacl_pair_state_to_start(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                {
#if TC_601_SUT_SUPPORT
                    tc_601_sut_iop_android_sc_pair_state_to_start(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                {
#if TC_800_SUT_SUPPORT
                    tc_800_sut_iop_android_legacl_pair_state_to_start(conn_id);
#endif
                }
                else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                {
#if TC_801_SUT_SUPPORT
                    tc_801_sut_iop_android_sc_pair_state_to_start(conn_id);
#endif
                }

            }
        }
        break;
    case GAP_AUTHEN_STATE_COMPLETE:
        {
            APP_PRINT_INFO1("GAP_MSG_LE_AUTHEN_STATE_CHANGE:(GAP_AUTHEN_STATE_COMPLETE) status 0x%x",
                            status);
            if (status == 0)
            {


                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
                    if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                    {
#if TC_600_SUPPORT
                        tc_600_iop_android_legacl_pair_state_to_success(conn_id);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                    {
#if TC_601_SUPPORT
                        tc_601_iop_android_sc_pair_state_to_success(conn_id);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                    {
#if TC_800_SUPPORT
                        tc_800_iop_android_legacl_pair_state_to_success(conn_id);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                    {
#if TC_801_SUPPORT
                        tc_801_iop_android_sc_pair_state_to_success(conn_id);
#endif
                    }
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
                    if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                    {
#if TC_600_SUT_SUPPORT
                        tc_600_sut_iop_android_legacl_pair_state_to_success(conn_id);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                    {
#if TC_601_SUT_SUPPORT
                        tc_601_sut_iop_android_sc_pair_state_to_success(conn_id);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                    {
#if TC_800_SUT_SUPPORT
                        tc_800_sut_iop_android_legacl_pair_state_to_success(conn_id);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                    {
#if TC_801_SUT_SUPPORT
                        tc_801_sut_iop_android_sc_pair_state_to_success(conn_id);
#endif
                    }
                }


                APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE pair success");
            }
            else
            {
                APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE pair failed");
                data_uart_print("pair failed conn_id = %d\r\n", conn_id);

                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {

                    if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                    {
#if TC_600_SUPPORT
                        tc_600_iop_android_legacl_pair_state_to_fail(conn_id, status);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                    {
#if TC_601_SUPPORT
                        tc_601_iop_android_sc_pair_state_to_fail(conn_id);
#endif
                    }

                    else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                    {
#if TC_800_SUPPORT
                        tc_800_iop_android_legacl_pair_state_to_fail(conn_id, status);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                    {
#if TC_801_SUPPORT
                        tc_801_iop_android_sc_pair_state_to_fail(conn_id);
#endif
                    }
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
                    if (app_get_cur_test_case() == TC_0600_IOP_PAIR_LEGACL)
                    {
#if TC_600_SUT_SUPPORT
                        tc_600_sut_iop_android_legacl_pair_state_to_fail(conn_id, status);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0601_IOP_PAIR_SC)
                    {
#if TC_601_SUT_SUPPORT
                        tc_601_sut_iop_android_sc_pair_state_to_fail(conn_id);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0800_IOP_PAIR_LEGACL)
                    {
#if TC_800_SUT_SUPPORT
                        tc_800_sut_iop_android_legacl_pair_state_to_fail(conn_id, status);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0801_IOP_PAIR_SC)
                    {
#if TC_801_SUT_SUPPORT
                        tc_801_sut_iop_android_sc_pair_state_to_fail(conn_id);
#endif
                    }

                }

            }
        }
        break;

    default:
        {
            APP_PRINT_INFO1("GAP_MSG_LE_AUTHEN_STATE_CHANGE:(unknown newstate: %d)", new_state);
        }
        break;
    }

}

/**
  * @brief  handle messages indicate that connection parameters has changed.
  * @param  conn_id: connection ID.
  * @param  status: change status.
  * @retval none
  */
void app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status)
{
    switch (status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
        {
            if (app_get_cur_test_case() == TC_0400_CONN_PARAM_UPDATE_SLAVE)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_400_SUPPORT
                    tc_400_conn_param_update_evt(conn_id);
#endif
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
#if TC_400_SUT_SUPPORT
                    tc_400_sut_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_0400_CONN_PARAM_UPDATE_SLAVE)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_401_SUPPORT
                    tc_401_conn_param_update_evt(conn_id);
#endif
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
#if TC_401_SUT_SUPPORT
                    tc_401_sut_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_200_SUPPORT
                    tc_200_tp_notification_tx_conn_param_update_event(conn_id);
#endif
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
#if TC_200_SUT_SUPPORT
                    tc_200_sut_conn_param_update_event(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_206_SUPPORT
                    tc_206_tp_notification_tx_conn_param_update_event(conn_id);
#endif
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
#if TC_206_SUT_SUPPORT
                    tc_206_sut_conn_param_update_event(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_207_SUPPORT
                    tc_207_tp_rx_conn_param_update_event(conn_id);
#endif
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
#if TC_207_SUT_SUPPORT
                    tc_207_sut_conn_param_update_event(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_208_SUPPORT
                    tc_208_tp_trx_conn_param_update_event(conn_id);
#endif
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
#if TC_208_SUT_SUPPORT
                    tc_208_sut_conn_param_update_event(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
            {
#if TC_700_SUT_SUPPORT
                tc_700_stable_notification_tx_conn_param_update_event(conn_id);
#endif
            }
            else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1102_SUPPORT
                    tc_1102_conn_param_update_event(conn_id);
#endif
                }
                else if (app_get_cur_tc_role() == TC_ROLE_SUT)
                {
#if TC_1102_SUT_SUPPORT
                    tc_1102_sut_conn_param_update_event(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1200_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1200_SUPPORT
                    tc_1200_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1201_SUPPORT
                    tc_1201_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1202_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1202_SUPPORT
                    tc_1202_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1203_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1203_SUPPORT
                    tc_1203_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1204_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1204_SUPPORT
                    tc_1204_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1205_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1205_SUPPORT
                    tc_1205_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1206_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1206_SUPPORT
                    tc_1206_conn_param_update_evt(conn_id);
#endif
                }
            }


        }
        break;
    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_CONN_PARAM_UPDATE failed.");
            data_uart_print("LE_CONN_PARAM_UPDATE failed\r\n");
            if (app_get_cur_test_case() == TC_1200_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1200_SUPPORT
                    tc_1200_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1201_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1201_SUPPORT
                    tc_1201_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1202_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1202_SUPPORT
                    tc_1202_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1203_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1203_SUPPORT
                    tc_1203_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1204_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1204_SUPPORT
                    tc_1204_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1205_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1205_SUPPORT
                    tc_1205_conn_param_update_evt(conn_id);
#endif
                }
            }
            else if (app_get_cur_test_case() == TC_1206_MULTI_LINK_4_MASTER)
            {
                if (app_get_cur_tc_role() == TC_ROLE_DUT)
                {
#if TC_1206_SUPPORT
                    tc_1206_conn_param_update_evt(conn_id);
#endif
                }
            }

        }
        break;
    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_CONN_PARAM_UPDATE param request pending.");
        }
        break;
    default:
        break;
    }
}

void app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
    if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
    {
        if (app_get_cur_tc_role() == TC_ROLE_DUT)
        {
#if TC_1102_SUPPORT
            tc_1102_mtu_size_info(conn_id);
#endif
        }
    }
}
/**
  * @brief  handle messages from GAP layer.
  * @param  pBeeIoMsg: message from GAP layer.
  * @retval none
  */
void app_handle_gap_msg(T_IO_MSG *p_io_msg)
{
    T_LE_GAP_MSG bt_msg;
    uint8_t conn_id;

    memcpy(&bt_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_handle_dev_state_evt(bt_msg.msg_data.gap_dev_state_change.new_state,
                                     bt_msg.msg_data.gap_dev_state_change.cause);
        }
        break;
    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_handle_conn_state_evt(bt_msg.msg_data.gap_conn_state_change.conn_id,
                                      (T_GAP_CONN_STATE)bt_msg.msg_data.gap_conn_state_change.new_state,
                                      bt_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;
    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            app_handle_conn_mtu_info_evt(bt_msg.msg_data.gap_conn_mtu_info.conn_id,
                                         bt_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;
    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_handle_authen_state_evt(bt_msg.msg_data.gap_authen_state.conn_id,
                                        bt_msg.msg_data.gap_authen_state.new_state,
                                        bt_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            uint8_t conn_id = bt_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {
            uint32_t display_value = 0;
            uint8_t conn_id = bt_msg.msg_data.gap_bond_passkey_display.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_PASSKEY_DISPLAY:passkey %d", display_value);
            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            data_uart_print("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn id=%d value=%d\r\n",
                            conn_id, display_value);
        }
        break;
    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            uint32_t displayValue = 0;
            uint8_t conn_id = bt_msg.msg_data.gap_bond_user_conf.conn_id;
            le_bond_get_display_key(conn_id, &displayValue);
            if (app_get_cur_test_case() >= TC_0801_IOP_PAIR_SC)
            {
                data_uart_print("GAP_MSG_LE_BOND_USER_CONFIRMATION conn id=%d value=%d\r\n",
                                conn_id, displayValue);
            }
            else
            {

                APP_PRINT_INFO1("GAP_MSG_LE_BOND_USER_CONFIRMATION: %d", displayValue);
                le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            }
        }
        break;
    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
            conn_id = bt_msg.msg_data.gap_bond_passkey_input.conn_id;
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_PASSKEY_INPUT");
            if (app_get_cur_test_case() >= TC_0801_IOP_PAIR_SC)
            {
                data_uart_print("GAP_MSG_LE_BOND_PASSKEY_INPUT conn id=%d\r\n", conn_id);
            }
            else
            {
                uint32_t passKey = 888888;
                le_bond_passkey_input_confirm(conn_id, passKey, GAP_CFM_CAUSE_ACCEPT);
            }
        }
        break;
#if F_BT_LE_SMP_OOB_SUPPORT
    case GAP_MSG_LE_BOND_OOB_INPUT:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_OOB_INPUT");
            uint8_t ooBData[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, ooBData);
            conn_id = bt_msg.msg_data.gap_bond_oob_input.conn_id;
            le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
#endif

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            app_handle_conn_param_update_evt(bt_msg.msg_data.gap_conn_param_update.conn_id,
                                             bt_msg.msg_data.gap_conn_param_update.status);
        }
        break;
    default:
        APP_PRINT_ERROR1("app_handle_gap_msg unknown subtype", p_io_msg->subtype);
        break;
    }

}

static void app_parse_scan_info(T_LE_SCAN_INFO *pLeScanInfo)
{
    uint8_t buffer[32];
    uint8_t pos = 0;

    while (pos < pLeScanInfo->data_len)
    {
        /* Length of the AD structure. */
        uint8_t length = pLeScanInfo->data[pos++];
        uint8_t type;

        if (length < 0x02 || (pos + length) > 0x1F)
        {
            continue;
        }

        /* Copy the AD Data to buffer. */
        memcpy(buffer, pLeScanInfo->data + pos + 1, length - 1);
        /* AD Type, one octet. */
        type = pLeScanInfo->data[pos];

        //APP_PRINT_INFO2("  AD Structure Info: AD type = 0x%x, AD Data Length = %d", type, length -1);

        switch (type)
        {
        case GAP_ADTYPE_FLAGS:
            {
                /* (flags & 0x01) -- LE Limited Discoverable Mode */
                /* (flags & 0x02) -- LE General Discoverable Mode */
                /* (flags & 0x04) -- BR/EDR Not Supported */
                /* (flags & 0x08) -- Simultaneous LE and BR/EDR to Same Device Capable (Controller) */
                /* (flags & 0x10) -- Simultaneous LE and BR/EDR to Same Device Capable (Host) */
                uint8_t flags = pLeScanInfo->data[pos + 1];
                APP_PRINT_INFO1("  AD Data: Flags = 0x%x", flags);
            }
            break;

        case GAP_ADTYPE_16BIT_MORE:
        case GAP_ADTYPE_16BIT_COMPLETE:
        case GAP_ADTYPE_SERVICES_LIST_16BIT:
            {
                uint16_t *pUUID = (uint16_t *)(buffer);
                uint8_t i = length - 1;

                while (i >= 2)
                {
                    APP_PRINT_INFO2("  AD Data: UUID16 List Item %d = 0x%x", i / 2, *pUUID++);
                    i -= 2;
                }
            }
            break;

        case GAP_ADTYPE_32BIT_MORE:
        case GAP_ADTYPE_32BIT_COMPLETE:
            {
                uint32_t *pUUID = (uint32_t *)(buffer);
                uint8_t    i     = length - 1;

                while (i >= 4)
                {
                    APP_PRINT_INFO2("  AD Data: UUID32 List Item %d = 0x%x", i / 4, *pUUID++);
                    i -= 4;
                }
            }
            break;

        case GAP_ADTYPE_128BIT_MORE:
        case GAP_ADTYPE_128BIT_COMPLETE:
        case GAP_ADTYPE_SERVICES_LIST_128BIT:
            {
                uint32_t *pUUID = (uint32_t *)(buffer);
                APP_PRINT_INFO4("  AD Data: UUID128 value: 0x%8.8x%8.8x%8.8x%8.8x",
                                pUUID[3], pUUID[2], pUUID[1], pUUID[0]);
            }
            break;

        case GAP_ADTYPE_LOCAL_NAME_SHORT:
        case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
            {
                buffer[length - 1] = '\0';
                APP_PRINT_INFO1("  AD Data: Local Name (%s)", TRACE_STRING(buffer));
            }
            break;
        default:
            break;
        }

        pos += length;
    }
}

/**
  * @brief Use 16 bit uuid to filter scan information
  * @param[in] uuid 16 bit UUID.
  * @param[in] scan_info point to scan information data.
  * @return filter result
  * @retval true found success
  * @retval false not found
  */
bool filter_scan_info_by_uuid(uint16_t uuid, T_LE_SCAN_INFO *scan_info)
{
    uint8_t buffer[32];
    uint8_t pos = 0;

    while (pos < scan_info->data_len)
    {
        /* Length of the AD structure. */
        uint8_t length = scan_info->data[pos++];
        uint8_t type;

        if ((length > 0x01) && ((pos + length) <= 31))
        {
            /* Copy the AD Data to buffer. */
            memcpy(buffer, scan_info->data + pos + 1, length - 1);
            /* AD Type, one octet. */
            type = scan_info->data[pos];

            switch (type)
            {
            case GAP_ADTYPE_16BIT_MORE:
            case GAP_ADTYPE_16BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_16BIT:
                {
                    uint16_t *p_uuid = (uint16_t *)(buffer);
                    uint8_t i = length - 1;

                    while (i >= 2)
                    {
                        APP_PRINT_INFO2("  AD Data: UUID16 List Item %d = 0x%x", i / 2, *p_uuid);
                        if (*p_uuid == uuid)
                        {
                            return true;
                        }
                        p_uuid++;
                        i -= 2;
                    }
                }
                break;

            default:
                break;
            }
        }

        pos += length;
    }
    return false;
}

/**
  * @brief  This callback will be called when advertising or scan response data received.
  * @param  msg_type: type of the message sent from GAP Central Role layer.
  * @param  cb_data: message sent from GAP Central Role layer.
  * @retval None
  */
T_APP_RESULT app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_LE_CB_DATA));
    APP_PRINT_INFO1("app_gap_callback: cb_type %d", cb_type);
    switch (cb_type)
    {
#if F_BT_LE_GAP_SCAN_SUPPORT
    case GAP_MSG_LE_SCAN_INFO:
        APP_PRINT_TRACE5("GAP_MSG_LE_SCAN_INFO: bd_addr %s, bdtype=%d, event=0x%x, rssi=%d, len=%d",
                         TRACE_BDADDR(cb_data.p_le_scan_info->bd_addr),
                         cb_data.p_le_scan_info->remote_addr_type,
                         cb_data.p_le_scan_info->adv_type,
                         cb_data.p_le_scan_info->rssi,
                         cb_data.p_le_scan_info->data_len);
        app_parse_scan_info(cb_data.p_le_scan_info);
        /* User can split interested information by using the function as follow. */
        //if (filter_scan_info_by_uuid(GATT_UUID_SIMPLE_PROFILE, cb_data.p_le_scan_info))
        if (filter_scan_info_by_uuid(0xA00A, cb_data.p_le_scan_info))
        {
            //APP_PRINT_INFO0("found simple profile");
            if (dev_list_count < 4)
            {
                link_mgr_add_device(cb_data.p_le_scan_info->bd_addr, cb_data.p_le_scan_info->remote_addr_type);
            }
            else
            {
                le_scan_stop();
            }
        }
        break;
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO4("  GAP_MSG_LE_CONN_UPDATE_IND: max_interval=0x%x, min_interval=0x%x, Latency=0x%x,timeOut=0x%x",
                        cb_data.p_le_conn_update_ind->conn_interval_max,
                        cb_data.p_le_conn_update_ind->conn_interval_min,
                        cb_data.p_le_conn_update_ind->conn_latency,
                        cb_data.p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        break;
    case GAP_MSG_LE_SET_HOST_CHANN_CLASSIF:
        APP_PRINT_INFO1("  GAP_MSG_LE_SET_HOST_CHANN_CLASSIF: cause=0x%x",
                        cb_data.p_le_set_host_chann_classif_rsp->cause);
        break;
#endif
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("  GAP_MSG_LE_MODIFY_WHITE_LIST: operation = 0x%x cause=0x%x",
                        cb_data.p_le_modify_white_list_rsp->operation,
                        cb_data.p_le_modify_white_list_rsp->cause);
        break;
#if F_BT_LE_READ_CHANN_MAP
    case GAP_MSG_LE_READ_CHANN_MAP:
        APP_PRINT_INFO7("  GAP_MSG_LE_READ_CHANN_MAP: conn_id=0x%x, cause=0x%x, map[0x%x:0x%x:0x%x:0x%x:0x%x]",
                        cb_data.p_le_read_chann_map_rsp->conn_id,
                        cb_data.p_le_read_chann_map_rsp->cause,
                        cb_data.p_le_read_chann_map_rsp->channel_map[0],
                        cb_data.p_le_read_chann_map_rsp->channel_map[1],
                        cb_data.p_le_read_chann_map_rsp->channel_map[2],
                        cb_data.p_le_read_chann_map_rsp->channel_map[3],
                        cb_data.p_le_read_chann_map_rsp->channel_map[4]);
        break;
#endif
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
    case GAP_MSG_LE_ADV_READ_TX_POWER:
        APP_PRINT_INFO2("  GAP_MSG_LE_ADV_READ_TX_POWER: cause=0x%x, tx=0x%x",
                        cb_data.p_le_adv_read_tx_power_rsp->cause,
                        cb_data.p_le_adv_read_tx_power_rsp->tx_power_level);
        break;
#endif
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case GAP_MSG_LE_SET_DATA_LEN:
        APP_PRINT_INFO2("  GAP_MSG_LE_SET_DATA_LEN: conn_id=0x%x, cause=0x%x",
                        cb_data.p_le_set_data_len_rsp->conn_id,
                        cb_data.p_le_set_data_len_rsp->cause);
        break;
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        APP_PRINT_INFO5("  GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id=0x%x MaxTxOctets=0x%x MaxTxTime=0x%x MaxRxOctets=0x%x MaxRxTime=0x%x",
                        cb_data.p_le_data_len_change_info->conn_id,
                        cb_data.p_le_data_len_change_info->max_tx_octets,
                        cb_data.p_le_data_len_change_info->max_tx_time,
                        cb_data.p_le_data_len_change_info->max_rx_octets,
                        cb_data.p_le_data_len_change_info->max_rx_time);
        data_uart_print("LE data length: con id %d, tx %d octets, %d ms - rx %d octets, %d ms\r\n",
                        cb_data.p_le_data_len_change_info->conn_id,
                        cb_data.p_le_data_len_change_info->max_tx_octets,
                        cb_data.p_le_data_len_change_info->max_tx_time,
                        cb_data.p_le_data_len_change_info->max_rx_octets,
                        cb_data.p_le_data_len_change_info->max_rx_time);
        break;
#endif
    case GAP_MSG_LE_SET_RAND_ADDR:
        APP_PRINT_INFO1("  GAP_MSG_LE_SET_RAND_ADDR: cause=0x%x",
                        cb_data.p_le_set_rand_addr_rsp->cause);
        break;
    case GAP_MSG_LE_READ_RSSI:
        APP_PRINT_INFO3("  GAP_MSG_LE_READ_RSSI:conn_id=0x%x cause=0x%x rssi=%01d",
                        cb_data.p_le_read_rssi_rsp->conn_id,
                        cb_data.p_le_read_rssi_rsp->cause,
                        cb_data.p_le_read_rssi_rsp->rssi);
        break;
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
    case GAP_MSG_LE_ADV_UPDATE_PARAM:
        APP_PRINT_INFO1("  GAP_MSG_LE_ADV_UPDATE_PARAM:cause=0x%x",
                        cb_data.p_le_adv_update_param_rsp->cause);
        break;
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_DISABLE_SLAVE_LATENCY:
        APP_PRINT_INFO1("  GAP_MSG_LE_DISABLE_SLAVE_LATENCY:cause=0x%x",
                        cb_data.p_le_disable_slave_latency_rsp->cause);
        break;
#endif
    case GAP_MSG_LE_CREATE_CONN_IND:
        APP_PRINT_INFO0("  GAP_MSG_LE_CREATE_CONN_IND");
        result = APP_RESULT_ACCEPT;
        break;
#endif
#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
    case GAP_MSG_LE_KEYPRESS_NOTIFY:
        APP_PRINT_INFO2("  GAP_MSG_LE_KEYPRESS_NOTIFY:conn %d, cause 0x%x",
                        cb_data.p_le_keypress_notify_rsp->conn_id, cb_data.p_le_keypress_notify_rsp->cause);
        break;

    case GAP_MSG_LE_KEYPRESS_NOTIFY_INFO:
        APP_PRINT_INFO2("  GAP_MSG_LE_KEYPRESS_NOTIFY_INFO:conn %d, type 0x%x",
                        cb_data.p_le_keypress_notify_info->conn_id, cb_data.p_le_keypress_notify_info->event_type);
        break;
#endif
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
    case GAP_MSG_LE_PHY_UPDATE_INFO:
        APP_PRINT_INFO4("  GAP_MSG_LE_PHY_UPDATE_INFO:conn %d, cause 0x%x, rx_phy %d, tx_phy %d",
                        cb_data.p_le_phy_update_info->conn_id,
                        cb_data.p_le_phy_update_info->cause,
                        cb_data.p_le_phy_update_info->rx_phy,
                        cb_data.p_le_phy_update_info->tx_phy);
        data_uart_print("LE phy update info: con id %d, cause = 0x%x, rx_phy = %d, tx_phy = %d\r\n",
                        cb_data.p_le_phy_update_info->conn_id,
                        cb_data.p_le_phy_update_info->cause,
                        cb_data.p_le_phy_update_info->rx_phy,
                        cb_data.p_le_phy_update_info->tx_phy);
        if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
        {
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if TC_200_SUPPORT
                tc_200_tp_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                        cb_data.p_le_phy_update_info->cause,
                                                        cb_data.p_le_phy_update_info->tx_phy,
                                                        cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
        }
        else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
        {
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if TC_206_SUPPORT
                tc_206_tp_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                        cb_data.p_le_phy_update_info->cause,
                                                        cb_data.p_le_phy_update_info->tx_phy,
                                                        cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
#if TC_206_SUT_SUPPORT
                tc_206_sut_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                         cb_data.p_le_phy_update_info->cause,
                                                         cb_data.p_le_phy_update_info->tx_phy,
                                                         cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
        }
        else if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
        {
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if TC_207_SUPPORT
                tc_207_tp_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                           cb_data.p_le_phy_update_info->cause,
                                           cb_data.p_le_phy_update_info->tx_phy,
                                           cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
#if TC_207_SUT_SUPPORT
                tc_207_sut_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                         cb_data.p_le_phy_update_info->cause,
                                                         cb_data.p_le_phy_update_info->tx_phy,
                                                         cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
        }
        else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
        {
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if TC_208_SUPPORT
                tc_208_tp_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                           cb_data.p_le_phy_update_info->cause,
                                           cb_data.p_le_phy_update_info->tx_phy,
                                           cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
#if TC_208_SUT_SUPPORT
                tc_208_sut_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                         cb_data.p_le_phy_update_info->cause,
                                                         cb_data.p_le_phy_update_info->tx_phy,
                                                         cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
        }
        else if (app_get_cur_test_case() == TC_0310_2M_LONGRANGE_1)
        {
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if TC_310_SUPPORT
                tc_310_phy_update_evt(cb_data.p_le_phy_update_info->conn_id,
                                      cb_data.p_le_phy_update_info->cause,
                                      cb_data.p_le_phy_update_info->tx_phy,
                                      cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
#if TC_310_SUT_SUPPORT
                tc_310_sut_phy_update_evt(cb_data.p_le_phy_update_info->conn_id,
                                          cb_data.p_le_phy_update_info->cause,
                                          cb_data.p_le_phy_update_info->tx_phy,
                                          cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
        }
        else if (app_get_cur_test_case() == TC_0311_2M_LONGRANGE_2)
        {
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
#if TC_311_SUPPORT
                tc_311_phy_update_evt(cb_data.p_le_phy_update_info->conn_id,
                                      cb_data.p_le_phy_update_info->cause,
                                      cb_data.p_le_phy_update_info->tx_phy,
                                      cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
#if TC_311_SUT_SUPPORT
                tc_311_sut_phy_update_evt(cb_data.p_le_phy_update_info->conn_id,
                                          cb_data.p_le_phy_update_info->cause,
                                          cb_data.p_le_phy_update_info->tx_phy,
                                          cb_data.p_le_phy_update_info->rx_phy);
#endif
            }
        }
        break;
#endif
#if F_BT_LE_READ_REMOTE_FEATS
    case GAP_MSG_LE_REMOTE_FEATS_INFO:
        APP_PRINT_INFO3("  GAP_MSG_LE_REMOTE_FEATS_INFO:conn id %d, cause 0x%x, remote_feats %b",
                        cb_data.p_le_remote_feats_info->conn_id,
                        cb_data.p_le_remote_feats_info->cause,
                        TRACE_BINARY(8, cb_data.p_le_remote_feats_info->remote_feats));
        break;
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:
        APP_PRINT_INFO1("  GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:cause 0x%x",
                        cb_data.p_le_update_passed_chann_map_rsp->cause);
        break;
#endif
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
    case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
        APP_PRINT_INFO6("GAP_MSG_LE_EXT_ADV_REPORT_INFO:connectable %d, scannable %d, direct %d, scan response %d, legacy %d, data status 0x%x",
                        cb_data.p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_CONNECTABLE_ADV,
                        cb_data.p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_SCANNABLE_ADV,
                        cb_data.p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_DIRECTED_ADV,
                        cb_data.p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_SCAN_RESPONSE,
                        cb_data.p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_USE_LEGACY_ADV,
                        cb_data.p_le_ext_adv_report_info->data_status);
        APP_PRINT_INFO5("GAP_MSG_LE_EXT_ADV_REPORT_INFO:event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d",
                        cb_data.p_le_ext_adv_report_info->event_type,
                        TRACE_BDADDR(cb_data.p_le_ext_adv_report_info->bd_addr),
                        cb_data.p_le_ext_adv_report_info->addr_type,
                        cb_data.p_le_ext_adv_report_info->rssi,
                        cb_data.p_le_ext_adv_report_info->data_len);
        APP_PRINT_INFO5("GAP_MSG_LE_EXT_ADV_REPORT_INFO:primary_phy %d, secondary_phy %d, adv_sid %d, tx_power %d, peri_adv_interval %d",
                        cb_data.p_le_ext_adv_report_info->primary_phy,
                        cb_data.p_le_ext_adv_report_info->secondary_phy,
                        cb_data.p_le_ext_adv_report_info->adv_sid,
                        cb_data.p_le_ext_adv_report_info->tx_power,
                        cb_data.p_le_ext_adv_report_info->peri_adv_interval);
        APP_PRINT_INFO2("GAP_MSG_LE_EXT_ADV_REPORT_INFO:direct_addr_type 0x%x, direct_addr %s",
                        cb_data.p_le_ext_adv_report_info->direct_addr_type,
                        TRACE_BDADDR(cb_data.p_le_ext_adv_report_info->direct_addr));

        if (1)
        {
            if (dev_list_count < 4)
            {
                link_mgr_add_device(cb_data.p_le_scan_info->bd_addr, cb_data.p_le_scan_info->remote_addr_type);
            }
            else
            {
                le_scan_stop();
            }
        }
        break;
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        APP_PRINT_INFO3("GAP_MSG_LE_EXT_ADV_START_SETTING:cause 0x%x, flag 0x%x, adv_handle %d",
                        cb_data.p_le_ext_adv_start_setting_rsp->cause,
                        cb_data.p_le_ext_adv_start_setting_rsp->flag,
                        cb_data.p_le_ext_adv_start_setting_rsp->adv_handle);
        if (app_get_cur_tc_role() == TC_ROLE_SUT)
        {
#if TC_1204_SUPPORT
            if (app_get_cur_test_case() == TC_1204_MULTI_LINK_4_MASTER)
            {
                le_ext_adv_enable(1, &cb_data.p_le_ext_adv_start_setting_rsp->adv_handle);
            }
#endif
#if TC_1205_SUPPORT
            if (app_get_cur_test_case() == TC_1205_MULTI_LINK_4_MASTER)
            {
                le_ext_adv_enable(1, &cb_data.p_le_ext_adv_start_setting_rsp->adv_handle);
            }
#endif
#if TC_1206_SUPPORT
            if (app_get_cur_test_case() == TC_1206_MULTI_LINK_4_MASTER)
            {
                le_ext_adv_enable(1, &cb_data.p_le_ext_adv_start_setting_rsp->adv_handle);
            }
#endif
        }

        break;
    case GAP_MSG_LE_EXT_ADV_REMOVE_SET:
        APP_PRINT_INFO2("GAP_MSG_LE_EXT_ADV_REMOVE_SET:cause 0x%x, adv_handle %d",
                        cb_data.p_le_ext_adv_remove_set_rsp->cause,
                        cb_data.p_le_ext_adv_remove_set_rsp->adv_handle);
        break;
    case GAP_MSG_LE_EXT_ADV_CLEAR_SET:
        APP_PRINT_INFO1("GAP_MSG_LE_EXT_ADV_CLEAR_SET:cause 0x%x",
                        cb_data.p_le_ext_adv_clear_set_rsp->cause);
        break;
    case GAP_MSG_LE_EXT_ADV_ENABLE:
        APP_PRINT_INFO1("GAP_MSG_LE_EXT_ADV_ENABLE:cause 0x%x",
                        cb_data.le_cause.cause);
        break;
    case GAP_MSG_LE_EXT_ADV_DISABLE:
        APP_PRINT_INFO1("GAP_MSG_LE_EXT_ADV_DISABLE:cause 0x%x",
                        cb_data.le_cause.cause);
        break;
    case GAP_MSG_LE_SCAN_REQ_RECEIVED_INFO:
        APP_PRINT_INFO3("GAP_MSG_LE_SCAN_REQ_RECEIVED_INFO:adv_handle %d, scanner_addr_type 0x%x, scanner_addr %s",
                        cb_data.p_le_scan_req_received_info->adv_handle,
                        cb_data.p_le_scan_req_received_info->scanner_addr_type,
                        TRACE_BDADDR(cb_data.p_le_scan_req_received_info->scanner_addr));
        break;
#endif
#if F_BT_LE_PRIVACY_SUPPORT
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        {
            APP_PRINT_INFO1("  GAP_MSG_LE_BOND_MODIFY_INFO:type=0x%x",
                            cb_data.p_le_bond_modify_info->type);
            if (cb_data.p_le_bond_modify_info->type == LE_BOND_CLEAR)
            {
                privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_CLEAR, GAP_IDENT_ADDR_PUBLIC,
                                                   NULL);
                privacy_handle_pending_resolving_list();
            }
            else if (cb_data.p_le_bond_modify_info->type == LE_BOND_ADD)
            {
                T_LE_KEY_ENTRY *p_entry = cb_data.p_le_bond_modify_info->p_entry;
                if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
                {
                    privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_ADD,
                                                       (T_GAP_IDENT_ADDR_TYPE)p_entry->resolved_remote_bd.remote_bd_type,
                                                       p_entry->resolved_remote_bd.addr);
                }
                else if (p_entry->flags & LE_KEY_STORE_LOCAL_IRK_BIT)
                {
                    privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_ADD,
                                                       (T_GAP_IDENT_ADDR_TYPE)p_entry->remote_bd.remote_bd_type,
                                                       p_entry->remote_bd.addr);
                }
                else
                {
                }
                privacy_handle_pending_resolving_list();
            }
            else if (cb_data.p_le_bond_modify_info->type == LE_BOND_DELETE)
            {
                T_LE_KEY_ENTRY *p_entry = cb_data.p_le_bond_modify_info->p_entry;
                if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
                {
                    privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_REMOVE,
                                                       (T_GAP_IDENT_ADDR_TYPE)p_entry->resolved_remote_bd.remote_bd_type,
                                                       p_entry->resolved_remote_bd.addr);
                }
                else if (p_entry->flags & LE_KEY_STORE_LOCAL_IRK_BIT)
                {
                    privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_REMOVE,
                                                       (T_GAP_IDENT_ADDR_TYPE)p_entry->remote_bd.remote_bd_type,
                                                       p_entry->remote_bd.addr);
                }
                else
                {
                }
                privacy_handle_pending_resolving_list();
            }
            else
            {
            }
        }
        break;
#endif

#if 0
    case GAP_MSG_LE_VENDOR_DROP_ACL_DATA:
        APP_PRINT_INFO1("GAP_MSG_LE_VENDOR_DROP_ACL_DATA: cause 0x%x",
                        cb_data.le_cause.cause);
        break;
#endif
    default:
        break;
    }
    return result;
}

#if F_BT_LE_GATT_CLIENT_SUPPORT
/**
  * @brief  Callback will be called when data sent from profile client layer.
  * @param  client_id: the ID distinguish which module sent the data.
  * @param  conn_id: connection ID.
  * @param  p_data: pointer to data.
  * @retval T_APP_RESULT
  */
T_APP_RESULT app_general_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_CLIENT_APP_CB_DATA *p_client_app_cb_data = (T_CLIENT_APP_CB_DATA *)p_data;

    switch (p_client_app_cb_data->cb_type)
    {
    case CLIENT_APP_CB_TYPE_DISC_STATE:
        if (p_client_app_cb_data->cb_content.disc_state_data.disc_state == DISC_STATE_SRV_DONE)
        {
            APP_PRINT_INFO0("Discovery Service Procedure Done.");
        }
        else
        {
            APP_PRINT_INFO0("Discovery state send to application directly.");
        }
        break;
    case CLIENT_APP_CB_TYPE_DISC_RESULT:
        if (p_client_app_cb_data->cb_content.disc_result_data.result_type == DISC_RESULT_ALL_SRV_UUID16)
        {
            APP_PRINT_INFO3("Discovery All Primary Service: UUID16=0x%x,StartHdl=0x%x,EndHdl=0x%x.",
                            p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->uuid16,
                            p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->att_handle,
                            p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->end_group_handle);
        }
        else
        {
            APP_PRINT_INFO0("Discovery result send to application directly.");
        }
        break;
    default:
        break;
    }
    return result;
}

T_APP_RESULT app_gap_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;

    if (client_id == GapClientID)
    {
        T_GAPS_CLIENT_CB_DATA *pGapCallbackData = (T_GAPS_CLIENT_CB_DATA *)p_data;
        APP_PRINT_INFO1("GAp BLE Client CB Type: %d.", pGapCallbackData->cb_type);
        switch (pGapCallbackData->cb_type)
        {
        case GAPS_CLIENT_CB_TYPE_DISC_STATE:
            switch (pGapCallbackData->cb_content.disc_state)
            {
            case DISC_GAPS_DONE:
                /* Discovery Simple BLE service procedure successfully done. */
                APP_PRINT_INFO0("GAP BLE Client CB: discover procedure done.");
                break;
            case DISC_GAPS_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("GAP BLE Client CB: discover request failed.");
                break;
            default:
                break;
            }
            break;
        case GAPS_CLIENT_CB_TYPE_READ_RESULT:
            switch (pGapCallbackData->cb_content.read_result.type)
            {
            case GAPS_READ_DEVICE_NAME:
                if (pGapCallbackData->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_DEVICE_NAME: device name %s.",
                                    TRACE_STRING(pGapCallbackData->cb_content.read_result.data.device_name.p_value));
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_DEVICE_NAME: failded cause 0x%x",
                                    pGapCallbackData->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_APPEARANCE:
                if (pGapCallbackData->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_APPEARANCE: appearance name %d",
                                    pGapCallbackData->cb_content.read_result.data.appearance);
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_APPEARANCE: failded cause 0x%x",
                                    pGapCallbackData->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_CENTRAL_ADDR_RESOLUTION:
                if (pGapCallbackData->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: central_addr_res %d",
                                    pGapCallbackData->cb_content.read_result.data.central_addr_res);
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: failded cause 0x%x",
                                    pGapCallbackData->cb_content.read_result.cause);
                }
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
    }

    return result;
}


T_APP_RESULT test_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;

    if (client_id == vendor_tp_client_id)
    {
        T_TP_CB_DATA *p_cb = (T_TP_CB_DATA *)p_data;
        tc_20x_sut_client_result_callback(conn_id, p_cb);
    }
    else if (client_id == vendor_pxp_client_id)
    {
#if (TC_500_SUT_SUPPORT||TC_501_SUT_SUPPORT||TC_502_SUT_SUPPORT)

        T_PXP_CB_DATA *p_cb = (T_PXP_CB_DATA *)p_data;
        tc_50x_sut_client_result_callback(conn_id, p_cb);
#endif
    }

    return result;
}
#endif

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
/******************************************************************
 * @fn          app_profile_callback
 * @brief      All the bt profile callbacks are handled in this function.
 *                Then the event handling function shall be called according to the serviceID
 *                of T_IO_MSG.
 *
 * @param    serviceID  -  service id of profile
 * @param    p_data  - pointer to callback data
 * @return     void
 */
T_APP_RESULT app_profile_callback(T_SERVER_ID serviceID, void *p_data)
{
    T_APP_RESULT appResult = APP_RESULT_SUCCESS;
    if (serviceID == SERVICE_PROFILE_GENERAL_ID)
    {
        T_SERVER_APP_CB_DATA *pPara = (T_SERVER_APP_CB_DATA *)p_data;
        switch (pPara->eventId)
        {
        case PROFILE_EVT_SRV_REG_COMPLETE:// srv register result event.
            APP_PRINT_INFO1("profile callback PROFILE_EVT_SRV_REG_COMPLETE result",
                            pPara->event_data.service_reg_result);
            {
                //peripheral_Init_StartAdvertising();
            }
            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            APP_PRINT_INFO2("profile callback PROFILE_EVT_SEND_DATA_COMPLETE,result = %d wCredits = %d",
                            pPara->event_data.send_data_result.cause,
                            pPara->event_data.send_data_result.credits);
            if (pPara->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
                {
#if TC_200_SUPPORT
                    tc_200_tp_notification_tx_tx_data_complete(pPara->event_data.send_data_result.credits);
#endif
                }
                else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
#if TC_206_SUPPORT
                    tc_206_tp_notification_tx_tx_data_complete(pPara->event_data.send_data_result.credits);
#endif
                }
                else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                {
                    if (app_get_cur_tc_role() == TC_ROLE_DUT)
                    {
#if TC_208_SUPPORT
                        tc_208_tp_notification_tx_data_complete(pPara->event_data.send_data_result.credits);
#endif
                    }
                }
                else if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
                {
#if TC_700_SUPPORT
                    tc_700_stable_notification_tx_01_tx_data_complete(
                        pPara->event_data.send_data_result.credits);
#endif
                }
            }
            else
            {
                APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
            }
            break;

        default:
            break;
        }
    }
    else  if (serviceID == gSimpleProfileServiceId)
    {
        TTP_CALLBACK_DATA *pTpCallbackData = (TTP_CALLBACK_DATA *)p_data;
        APP_PRINT_INFO1("gSimpleProfileServiceId conn_id = %d", pTpCallbackData->conn_id);
        switch (pTpCallbackData->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                APP_PRINT_INFO0("SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION");
                switch (pTpCallbackData->msg_data.notification_indification_index)
                {
                case VENDOR_TP_SERVICE_V1_INDICATION_ENABLE:
                    {
                        APP_PRINT_INFO0("VENDOR_TP_SERVICE_V1_INDICATION_ENABLE");
                    }
                    break;

                case VENDOR_TP_SERVICE_V1_INDICATION_DISABLE:
                    {
                        APP_PRINT_INFO0("VENDOR_TP_SERVICE_V1_INDICATION_DISABLE");
                    }
                    break;
                case VENDOR_TP_SERVICE_V1_NOTIFICATION_ENABLE:
                    {
                        APP_PRINT_INFO0("VENDOR_TP_SERVICE_V1_NOTIFICATION_ENABLE");
                        if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
                        {
#if TC_700_SUPPORT
                            tc_700_stable_notification_tx_01_cccd_enable(pTpCallbackData->conn_id, true);
#endif
                        }
                    }
                    break;
                case VENDOR_TP_SERVICE_V1_NOTIFICATION_DISABLE:
                    {
                        APP_PRINT_INFO0("VENDOR_TP_SERVICE_V1_NOTIFICATION_DISABLE");
                        if (app_get_cur_test_case() == TC_0700_STABLE_NOTIFICATION_TX_01)
                        {
#if TC_700_SUPPORT
                            tc_700_stable_notification_tx_01_cccd_enable(pTpCallbackData->conn_id, false);
#endif
                        }
                    }
                    break;

                }
            }
            break;


        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                APP_PRINT_INFO0("SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE");
            }
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                APP_PRINT_INFO0("SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE");
                if (pTpCallbackData->msg_data.write.write_type == WRITE_REQUEST)
                {
                    if (app_get_cur_test_case() == TC_0200_TP_NOTIFICATION_TX_01)
                    {
#if TC_200_SUPPORT
                        tc_200_tp_notification_tx_tx_config(p_data);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                    {
#if TC_206_SUPPORT
                        tc_206_tp_notification_tx_tx_config(p_data);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                    {
#if TC_208_SUPPORT
                        tc_208_tp_notification_tx_tx_config(p_data);
#endif
                    }
                }
                else
                {
                    if (app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                    {
#if TC_207_SUPPORT
                        tc_207_tp_handle_write_data(p_data);
#endif
                    }
                    else if (app_get_cur_test_case() == TC_0208_TP_NOTIF_WRITE_CMD_TRX_02)
                    {
#if TC_208_SUPPORT
                        tc_208_tp_handle_write_data(p_data);
#endif
                    }
                }
            }
        }
    }

    return appResult;
}
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
T_APP_RESULT app_credit_based_conn_callback(uint8_t cbc_type, void *p_cbc_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CBC_DATA cb_data;
    memcpy(&cb_data, p_cbc_data, sizeof(T_LE_CBC_DATA));
    APP_PRINT_TRACE1("app_credit_based_conn_callback: msgType = %d", cbc_type);
    switch (cbc_type)
    {
    case GAP_CBC_MSG_LE_CHANN_STATE:
        APP_PRINT_INFO4("GAP_CBC_MSG_LE_CHANN_STATE: conn_id %d, cid 0x%x, conn_state %d, cause 0x%x",
                        cb_data.p_le_chann_state->conn_id,
                        cb_data.p_le_chann_state->cid,
                        cb_data.p_le_chann_state->conn_state,
                        cb_data.p_le_chann_state->cause);
        if (cb_data.p_le_chann_state->conn_state == GAP_CHANN_STATE_CONNECTED)
        {
            uint16_t mtu;
            uint16_t credit;
            le_cbc_get_chann_param(CBC_CHANN_PARAM_CUR_CREDITS, &credit, cb_data.p_le_chann_state->cid);
            le_cbc_get_chann_param(CBC_CHANN_PARAM_MTU, &mtu, cb_data.p_le_chann_state->cid);
            APP_PRINT_INFO2("GAP_CHANN_STATE_CONNECTED: mtu %d, credit %d", mtu, credit);
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
                if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUPPORT
                    tc_1100_chann_connected(cb_data.p_le_chann_state->cid);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUPPORT
                    tc_1101_chann_connected(cb_data.p_le_chann_state->cid,
                                            mtu,
                                            credit);
#endif
                }
                else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
                {
#if TC_1102_SUPPORT
                    tc_1102_chann_connected(cb_data.p_le_chann_state->cid,
                                            mtu,
                                            credit);
#endif
                }
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
                if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUT_SUPPORT
                    tc_1100_sut_chann_connected(cb_data.p_le_chann_state->cid);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUT_SUPPORT
                    tc_1101_sut_chann_connected(cb_data.p_le_chann_state->cid, mtu,
                                                credit);
#endif
                }
                else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
                {
#if TC_1102_SUT_SUPPORT
                    tc_1102_sut_chann_connected(cb_data.p_le_chann_state->cid, mtu,
                                                credit);
#endif
                }
            }
        }
        else if (cb_data.p_le_chann_state->conn_state == GAP_CHANN_STATE_DISCONNECTED)
        {
            if (app_get_cur_tc_role() == TC_ROLE_DUT)
            {
                if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUPPORT
                    tc_1100_chann_disconnected(cb_data.p_le_chann_state->cid, cb_data.p_le_chann_state->cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUPPORT
                    tc_1101_chann_disconnected(cb_data.p_le_chann_state->cid, cb_data.p_le_chann_state->cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
                {
#if TC_1102_SUPPORT
                    tc_1102_chann_disconnected(cb_data.p_le_chann_state->cid, cb_data.p_le_chann_state->cause);
#endif
                }
            }
            else if (app_get_cur_tc_role() == TC_ROLE_SUT)
            {
                if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
                {
#if TC_1100_SUT_SUPPORT
                    tc_1100_sut_chann_disconnected(cb_data.p_le_chann_state->cid, cb_data.p_le_chann_state->cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
                {
#if TC_1101_SUT_SUPPORT
                    tc_1101_sut_chann_disconnected(cb_data.p_le_chann_state->cid, cb_data.p_le_chann_state->cause);
#endif
                }
                else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
                {
#if TC_1102_SUT_SUPPORT
                    tc_1102_sut_chann_disconnected(cb_data.p_le_chann_state->cid, cb_data.p_le_chann_state->cause);
#endif
                }
            }
        }
        break;

    case GAP_CBC_MSG_LE_REG_PSM:
        APP_PRINT_INFO2("GAP_CBC_MSG_LE_REG_PSM: le_psm 0x%x, cause 0x%x",
                        cb_data.p_le_reg_psm_rsp->le_psm,
                        cb_data.p_le_reg_psm_rsp->cause);
        break;

    case GAP_CBC_MSG_LE_SET_PSM_SECURITY:
        APP_PRINT_INFO1("GAP_CBC_MSG_LE_SET_PSM_SECURITY: cause 0x%x",
                        cb_data.p_le_set_psm_security_rsp->cause);
        break;

    case GAP_CBC_MSG_LE_SEND_DATA:
        APP_PRINT_INFO4("GAP_CBC_MSG_LE_SEND_DATA: conn_id %d, cid 0x%x, cause 0x%x, credit %d",
                        cb_data.p_le_send_data->conn_id,
                        cb_data.p_le_send_data->cid,
                        cb_data.p_le_send_data->cause,
                        cb_data.p_le_send_data->credit);
        if (app_get_cur_tc_role() == TC_ROLE_DUT)
        {
            if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
            {
#if TC_1101_SUPPORT
                tx_1101_send_data_cmpl(cb_data.p_le_send_data->cause, cb_data.p_le_send_data->credit);
#endif
            }
            if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
            {
#if TC_1102_SUPPORT
                tc_1102_send_data_cmpl(cb_data.p_le_send_data->cause, cb_data.p_le_send_data->credit);
#endif
            }
        }
        else if (app_get_cur_tc_role() == TC_ROLE_SUT)
        {
            if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
            {
#if TC_1100_SUT_SUPPORT
                tc_1100_sut_chann_data_send_cmpl(cb_data.p_le_send_data->cause);
#endif
            }
            else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
            {
#if TC_1101_SUT_SUPPORT
                tc_1101_sut_chann_data_send_cmpl(cb_data.p_le_send_data->cause, cb_data.p_le_send_data->credit);
#endif
            }
        }
        break;

    case GAP_CBC_MSG_LE_RECEIVE_DATA:
        APP_PRINT_INFO3("GAP_CBC_MSG_LE_RECEIVE_DATA: conn_id %d, cid 0x%x, value_len %d",
                        cb_data.p_le_receive_data->conn_id,
                        cb_data.p_le_receive_data->cid,
                        cb_data.p_le_receive_data->value_len);
        if (app_get_cur_tc_role() == TC_ROLE_DUT)
        {
            if (app_get_cur_test_case() == TC_1100_BT41_CONN_TEST)
            {
#if TC_1100_SUPPORT
                tc_1100_receive_data(cb_data.p_le_receive_data->cid);
#endif
            }
            else if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
            {
#if TC_1101_SUPPORT
                tc_1101_receive_data(cb_data.p_le_receive_data->cid, cb_data.p_le_receive_data->value_len);
#endif
            }
        }
        else if (app_get_cur_tc_role() == TC_ROLE_SUT)
        {
            if (app_get_cur_test_case() == TC_1101_BT41_TRX_STRESS_TEST)
            {
#if TC_1101_SUT_SUPPORT
                tc_1101_sut_receive_data(cb_data.p_le_receive_data->cid, cb_data.p_le_receive_data->value_len);
#endif
            }
            else if (app_get_cur_test_case() == TC_1102_BT41_TP_TEST)
            {
#if TC_1102_SUT_SUPPORT
                tc_1102_sut_receive_data(cb_data.p_le_receive_data->cid, cb_data.p_le_receive_data->value_len);
#endif
            }
        }
        break;

    default:
        break;
    }
    return result;
}
#endif
void tc_add_case(uint32_t count)
{
    T_CUR_DEVICE_ROLE role = app_get_cur_tc_role();

    switch (role)
    {
    case TC_ROLE_DUT:
        {


#if TC_0001_SUPPORT
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
            tc_0001_add_case(0x80, 0x80);
#endif
#endif

#if TC_0002_SUPPORT
            tc_0002_add_case(0x80, 0x80, 1000);
#endif
            /**
            format:
            tc 101 [scan_mode] [interval] [window] [filter_policy] [filter_dup] [count]

            sample:
            tc 101 1 100 100 0 0 1000

            */
#if TC_101_SUPPORT
            tc_0101_add_case(GAP_SCAN_MODE_ACTIVE, 100, 100, GAP_SCAN_FILTER_ANY, 0, 10);
#endif
#if TC_300_SUPPORT
            tc_300_add_case(count);
#endif
#if TC_301_SUPPORT
            tc_301_add_case(count);
#endif
#if TC_500_SUPPORT
            tc_500_add_case(count);
#endif
#if TC_501_SUPPORT
            tc_501_add_case(count);
#endif
#if TC_600_SUPPORT
            tc_600_add_case(count);
#endif
#if TC_601_SUPPORT
            tc_601_add_case(count);
#endif


        }
        break;

    case TC_ROLE_SUT:
        {
#if 1
            /**
            format:
            tc 101 [scan_mode] [interval] [window] [filter_policy] [filter_dup] [count]
            sample:
            tc 101 1 100 100 0 0 1000
            */
#if TC_101_SUPPORT
            tc_0101_add_case(GAP_SCAN_MODE_ACTIVE, 100, 100, GAP_SCAN_FILTER_ANY, 0, 10);
#endif

#if TC_300_SUT_SUPPORT
            tc_300_sut_add_case(count, g_cur_rembd);
#endif

#if TC_301_SUT_SUPPORT
            tc_301_sut_add_case(count, g_cur_rembd);
#endif

#if TC_500_SUT_SUPPORT
            tc_500_sut_add_case(count, g_cur_rembd);
#endif

#if TC_501_SUT_SUPPORT
            tc_501_sut_add_case(count, g_cur_rembd);
#endif

#if TC_600_SUT_SUPPORT
            tc_600_sut_add_case(count, g_cur_rembd);
#endif
#if TC_601_SUT_SUPPORT
            tc_601_sut_add_case(count, g_cur_rembd);
#endif

#endif

        }
        break;
    case TC_ROLE_UNDEFINED:

        break;
    default:
        break;
    }
}

void tc_result_cb(uint16_t case_id, uint16_t result, void *p_cb_data)
{
    data_uart_print("tc %d, result 0x%04x\r\n", case_id, result);

    tc_dump_disc_reason();
    memset(&g_ble_disconn_reason, 0, sizeof(g_ble_disconn_reason));

    if (case_id)
    {
        switch (case_id)
        {
        case TC_0001_ADV_PERFORMANCE:
            break;

        case TC_0002_ADV_STRESS_START_STOP:

            break;

#if TC_100_SUPPORT
        case TC_0100_SCAN_PERFORMANCE:
            {

            }
            break;

        case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
            {

            }
            break;
#endif

        case TC_0200_TP_NOTIFICATION_TX_01:
            {
            }

            break;
        case TC_0201_TP_INDICATION_TX_01:

            break;
        case TC_0202_TP_WRITE_COMMAND_TX_01:
            break;
        case TC_0203_TP_NOTIFICATION_RX_01:
            break;
        case TC_0204_TP_INDICATION_RX_01:
            break;
        case TC_0205_TP_WRITE_COMMAND_RX_01:
            break;
        case TC_0300_ADV_ONLY:
            {

            }
            break;
        case TC_0301_ADV_DISC:
            {
            }
            break;
        case TC_0400_CONN_PARAM_UPDATE_SLAVE:
            break;
        case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
            break;
        case TC_0500_SLAVE_AUTO_ADV:
            {
            }
            break;
        case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
            {
            }
            break;
        case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
            {
            }
            break;
        case TC_0600_IOP_PAIR_LEGACL:
            {
            }
            break;
        case TC_0601_IOP_PAIR_SC:
            {
            }
            break;
        case TC_0700_STABLE_NOTIFICATION_TX_01:
            break;

        case TC_0800_IOP_PAIR_LEGACL:
            break;
        case TC_0801_IOP_PAIR_SC:
            break;

        case TC_0900_PRIVACY_TEST_SLAVE:
            break;
        case TC_0901_PRIVACY_TEST_MASTER:
            break;

        default:
            break;

        }
    }

    tc_start_next_case();
}




void tc_dut_start_next_case(uint16_t id, T_TC_PARAM_DATA *p_tc_param)
{
    switch (id)
    {
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT

#if TC_0001_SUPPORT
    case TC_0001_ADV_PERFORMANCE:
        {
            T_TC_0001_PARAM_DATA *p_tc_0001_param_data;
            p_tc_0001_param_data = (T_TC_0001_PARAM_DATA *)p_tc_param;
            tc_0001_adv_start(p_tc_0001_param_data->adv_interval_min, p_tc_0001_param_data->adv_interval_max);
        }
        break;
#endif

#if TC_0002_SUPPORT
    case TC_0002_ADV_STRESS_START_STOP:
        {
            T_TC_0002_PARAM_DATA *p_tc_0002_param_data;
            p_tc_0002_param_data = (T_TC_0002_PARAM_DATA *)p_tc_param;

            tc_0002_adv_start_stop_start(p_tc_0002_param_data->adv_interval_min,
                                         p_tc_0002_param_data->adv_interval_max,
                                         p_tc_0002_param_data->max_count);
        }
        break;
#endif

#if TC_100_SUPPORT

    case TC_0100_SCAN_PERFORMANCE:
        {

        }
        break;
#endif
#endif

#if TC_100_SUPPORT
    case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
        {

            T_TC_0101_IN_PARAM_DATA *p_tc_0101_in_param_data;
            p_tc_0101_in_param_data = (T_TC_0101_IN_PARAM_DATA *)p_tc_param;
            tc_0101_scan_stress_enable_disable_start(p_tc_0101_in_param_data->mode,
                                                     p_tc_0101_in_param_data->interval,
                                                     p_tc_0101_in_param_data->window,
                                                     p_tc_0101_in_param_data->filter_policy,
                                                     p_tc_0101_in_param_data->filter_duplicates,
                                                     p_tc_0101_in_param_data->max_count);
        }
        break;
#endif

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT

#if TC_200_SUPPORT
    case TC_0200_TP_NOTIFICATION_TX_01:
        {
            uint16_t interval = 0x80;
            uint16_t length = 20;
            uint8_t tx_octets = 27;
            tc_200_tp_notification_tx_init_config(interval, length, tx_octets, false);
            tc_200_tp_notification_tx_start();
        }
        break;
#endif
    case TC_0201_TP_INDICATION_TX_01:

        break;
    case TC_0202_TP_WRITE_COMMAND_TX_01:
        break;
    case TC_0203_TP_NOTIFICATION_RX_01:
        break;
    case TC_0204_TP_INDICATION_RX_01:
        break;
    case TC_0205_TP_WRITE_COMMAND_RX_01:
        break;

#if TC_300_SUPPORT
    case TC_0300_ADV_ONLY:
        {
            T_TC_300_IN_PARAM_DATA *p_tc_300_in_param_data;
            p_tc_300_in_param_data = (T_TC_300_IN_PARAM_DATA *)p_tc_param;

            tc_300_adv_only_start(p_tc_300_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_301_SUPPORT
    case TC_0301_ADV_DISC:
        {
            T_TC_301_IN_PARAM_DATA *p_tc_301_in_param_data;
            p_tc_301_in_param_data = (T_TC_301_IN_PARAM_DATA *)p_tc_param;

            tc_301_adv_disc_start(p_tc_301_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_400_SUPPORT
    case TC_0400_CONN_PARAM_UPDATE_SLAVE:
        {
            T_TC_400_IN_PARAM_DATA *p_tc_400_in_param_data;
            p_tc_400_in_param_data = (T_TC_400_IN_PARAM_DATA *)p_tc_param;

            tc_400_start(p_tc_400_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_401_SUPPORT
    case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
        {
            T_TC_401_IN_PARAM_DATA *p_tc_401_in_param_data;
            p_tc_401_in_param_data = (T_TC_401_IN_PARAM_DATA *)p_tc_param;

            tc_401_start(p_tc_401_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_500_SUPPORT
    case TC_0500_SLAVE_AUTO_ADV:
        {
            T_TC_500_IN_PARAM_DATA *p_tc_500_in_param_data;
            p_tc_500_in_param_data = (T_TC_500_IN_PARAM_DATA *)p_tc_param;
            tc_500_salve_auto_adv_start(p_tc_500_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_501_SUPPORT
    case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
        {
            T_TC_501_IN_PARAM_DATA *p_tc_501_in_param_data;
            p_tc_501_in_param_data = (T_TC_501_IN_PARAM_DATA *)p_tc_param;

            tc_501_salve_auto_adv_with_sec_req_start(p_tc_501_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_502_SUPPORT
    case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
        {
            T_TC_502_IN_PARAM_DATA *p_tc_502_in_param_data;
            p_tc_502_in_param_data = (T_TC_502_IN_PARAM_DATA *)p_tc_param;
            tc_502_salve_auto_adv_start(p_tc_502_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_600_SUPPORT
    case TC_0600_IOP_PAIR_LEGACL:
        {
            T_TC_600_IN_PARAM_DATA *p_tc_600_in_param_data;
            p_tc_600_in_param_data = (T_TC_600_IN_PARAM_DATA *)p_tc_param;

            tc_600_iop_android_legacl_pair_start(p_tc_600_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_601_SUPPORT
    case TC_0601_IOP_PAIR_SC:
        {
            T_TC_601_IN_PARAM_DATA *p_tc_601_in_param_data;
            p_tc_601_in_param_data = (T_TC_601_IN_PARAM_DATA *)p_tc_param;
            tc_601_iop_android_sc_pair_start(p_tc_601_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_700_SUPPORT
    case TC_0700_STABLE_NOTIFICATION_TX_01:
        tc_700_stable_notification_tx_01_start();
        break;

#endif

#if TC_800_SUPPORT
    case TC_0800_IOP_PAIR_LEGACL:
        {
            T_TC_800_IN_PARAM_DATA *p_tc_800_in_param_data;
            p_tc_800_in_param_data = (T_TC_800_IN_PARAM_DATA *)p_tc_param;

            tc_800_iop_android_legacl_pair_start(p_tc_800_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_801_SUPPORT
    case TC_0801_IOP_PAIR_SC:
        {
            T_TC_801_IN_PARAM_DATA *p_tc_801_in_param_data;
            p_tc_801_in_param_data = (T_TC_801_IN_PARAM_DATA *)p_tc_param;
            tc_801_iop_android_sc_pair_start(p_tc_801_in_param_data->total_test_count);
        }
        break;
#endif

#if TC_900_SUPPORT
    case TC_0900_PRIVACY_TEST_SLAVE:
        tc_900_privacy_slave();
        break;
#endif

#if TC_901_SUPPORT
    case TC_0901_PRIVACY_TEST_MASTER:
        tc_901_privacy_master();
        break;
#endif

#endif

#if TC_1000_SUPPORT
#if F_CP_TEST_SUPPORT
    case TC_1000_CP_TEST:
        tc_1000_hw_init();
        tc_1000_start();
        break;
    case TC_1001_CP_TEST:
        tc_1001_start();
        break;
    case TC_1002_CP_TEST:
        tc_1002_start();
        break;
#endif
#endif
    default:
        return;
    }
}


void tc_sut_start_next_case(uint16_t id, T_TC_PARAM_DATA *p_tc_param)
{
    switch (id)
    {


#if TC_0001_SUPPORT
    case TC_0001_ADV_PERFORMANCE:
        {
            T_TC_0001_PARAM_DATA *p_tc_0001_param_data;
            p_tc_0001_param_data = (T_TC_0001_PARAM_DATA *)p_tc_param;
            tc_0001_adv_start(p_tc_0001_param_data->adv_interval_min, p_tc_0001_param_data->adv_interval_max);
        }
        break;
#endif


#if TC_0002_SUPPORT
    case TC_0002_ADV_STRESS_START_STOP:
        {
            T_TC_0002_PARAM_DATA *p_tc_0002_param_data;
            p_tc_0002_param_data = (T_TC_0002_PARAM_DATA *)p_tc_param;

            tc_0002_adv_start_stop_start(p_tc_0002_param_data->adv_interval_min,
                                         p_tc_0002_param_data->adv_interval_max,
                                         p_tc_0002_param_data->max_count);
        }
        break;
#endif
#if TC_100_SUPPORT
    case TC_0100_SCAN_PERFORMANCE:
        {

        }
        break;
#endif

#if TC_100_SUPPORT
    case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
        {

            T_TC_0101_IN_PARAM_DATA *p_tc_0101_in_param_data;
            p_tc_0101_in_param_data = (T_TC_0101_IN_PARAM_DATA *)p_tc_param;
            tc_0101_scan_stress_enable_disable_start(p_tc_0101_in_param_data->mode,
                                                     p_tc_0101_in_param_data->interval,
                                                     p_tc_0101_in_param_data->window,
                                                     p_tc_0101_in_param_data->filter_policy,
                                                     p_tc_0101_in_param_data->filter_duplicates,
                                                     p_tc_0101_in_param_data->max_count);
        }
        break;
#endif

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT



#if TC_200_SUT_SUPPORT
    case TC_0200_TP_NOTIFICATION_TX_01:
        {
            uint16_t interval = 0x80;
            uint16_t length = 20;
            uint8_t tx_octets = 27;
            tc_200_tp_notification_tx_init_config(interval, length, tx_octets, false);
            tc_200_tp_notification_tx_start();
        }
        break;
#endif

    case TC_0201_TP_INDICATION_TX_01:

        break;
    case TC_0202_TP_WRITE_COMMAND_TX_01:
        break;
    case TC_0203_TP_NOTIFICATION_RX_01:
        break;
    case TC_0204_TP_INDICATION_RX_01:
        break;
    case TC_0205_TP_WRITE_COMMAND_RX_01:
        break;

#if TC_300_SUT_SUPPORT
    case TC_0300_ADV_ONLY:
        {
            T_TC_300_SUT_IN_PARAM_DATA *p_tc_300_sut_in_param_data;
            p_tc_300_sut_in_param_data = (T_TC_300_SUT_IN_PARAM_DATA *)p_tc_param;

            tc_300_sut_start(p_tc_300_sut_in_param_data->total_test_count,
                             p_tc_300_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_301_SUT_SUPPORT
    case TC_0301_ADV_DISC:
        {
            T_TC_301_SUT_IN_PARAM_DATA *p_tc_301_sut_in_param_data;
            p_tc_301_sut_in_param_data = (T_TC_301_SUT_IN_PARAM_DATA *)p_tc_param;

            tc_301_sut_start(p_tc_301_sut_in_param_data->total_test_count,
                             p_tc_301_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_400_SUT_SUPPORT
    case TC_0400_CONN_PARAM_UPDATE_SLAVE:
        {
            T_TC_400_SUT_IN_PARAM_DATA *p_tc_400_sut_in_param_data;
            p_tc_400_sut_in_param_data = (T_TC_400_SUT_IN_PARAM_DATA *)p_tc_param;

            tc_400_sut_start(p_tc_400_sut_in_param_data->total_test_count,
                             p_tc_400_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_401_SUT_SUPPORT
    case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
        {
            T_TC_401_SUT_IN_PARAM_DATA *p_tc_401_sut_in_param_data;
            p_tc_401_sut_in_param_data = (T_TC_401_SUT_IN_PARAM_DATA *)p_tc_param;

            tc_401_sut_start(p_tc_401_sut_in_param_data->total_test_count,
                             p_tc_401_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_500_SUT_SUPPORT
    case TC_0500_SLAVE_AUTO_ADV:
        {
            T_TC_500_SUT_IN_PARAM_DATA *p_tc_500_sut_in_param_data;
            p_tc_500_sut_in_param_data = (T_TC_500_SUT_IN_PARAM_DATA *)p_tc_param;
            tc_500_sut_start(p_tc_500_sut_in_param_data->total_test_count,
                             p_tc_500_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_501_SUT_SUPPORT
    case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
        {
            T_TC_501_SUT_IN_PARAM_DATA *p_tc_501_sut_in_param_data;
            p_tc_501_sut_in_param_data = (T_TC_501_SUT_IN_PARAM_DATA *)p_tc_param;
            tc_501_sut_start(p_tc_501_sut_in_param_data->total_test_count,
                             p_tc_501_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_502_SUT_SUPPORT
    case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
        {

        }
        break;
#endif

#if TC_600_SUT_SUPPORT
    case TC_0600_IOP_PAIR_LEGACL:
        {
            T_TC_600_SUT_IN_PARAM_DATA *p_tc_600_sut_in_param_data;
            p_tc_600_sut_in_param_data = (T_TC_600_SUT_IN_PARAM_DATA *)p_tc_param;
            tc_600_sut_iop_android_legacl_pair_start(p_tc_600_sut_in_param_data->total_test_count,
                                                     p_tc_600_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_601_SUT_SUPPORT
    case TC_0601_IOP_PAIR_SC:
        {
            T_TC_601_SUT_IN_PARAM_DATA *p_tc_601_sut_in_param_data;
            p_tc_601_sut_in_param_data = (T_TC_601_SUT_IN_PARAM_DATA *)p_tc_param;
            tc_601_sut_iop_android_sc_pair_start(p_tc_601_sut_in_param_data->total_test_count,
                                                 p_tc_601_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_700_SUT_SUPPORT
    case TC_0700_STABLE_NOTIFICATION_TX_01:
        tc_700_stable_notification_tx_01_start();
        break;


#endif

#if TC_800_SUT_SUPPORT
    case TC_0800_IOP_PAIR_LEGACL:
        {
            T_TC_800_SUT_IN_PARAM_DATA *p_tc_800_sut_in_param_data;
            p_tc_800_sut_in_param_data = (T_TC_800_SUT_IN_PARAM_DATA *)p_tc_param;
            tc_800_sut_iop_android_legacl_pair_start(p_tc_800_sut_in_param_data->total_test_count,
                                                     p_tc_800_sut_in_param_data->remote_bd);
        }
        break;
#endif

#if TC_801_SUT_SUPPORT
    case TC_0801_IOP_PAIR_SC:
        {
            T_TC_801_SUT_IN_PARAM_DATA *p_tc_801_sut_in_param_data;
            p_tc_801_sut_in_param_data = (T_TC_801_SUT_IN_PARAM_DATA *)p_tc_param;
            tc_801_sut_iop_android_sc_pair_start(p_tc_801_sut_in_param_data->total_test_count,
                                                 p_tc_801_sut_in_param_data->remote_bd);
        }
        break;
#endif

#endif

#if TC_900_SUT_SUPPORT
    case TC_0900_PRIVACY_TEST_SLAVE:
        tc_900_privacy_slave();
        break;

#endif

#if TC_901_SUT_SUPPORT
    case TC_0901_PRIVACY_TEST_MASTER:
        tc_901_privacy_master();
        break;
#endif


#if TC_1000_SUPPORT

    case TC_1000_CP_TEST:
        tc_1000_hw_init();
        tc_1000_start();
        break;
    case TC_1001_CP_TEST:
        tc_1001_start();
        break;
    case TC_1002_CP_TEST:
        tc_1002_start();
        break;

#endif

    default:
        return;
    }
}


void tc_start_next_case(void)
{
    T_CUR_DEVICE_ROLE role = app_get_cur_tc_role();

    tc_reg_result_callback(tc_result_cb);

    //start next case
    T_TC_PARAM *p_tc_param = os_queue_out(&tc_q);
    T_TC_PARAM_DATA *p_tc_param_data;

    if (p_tc_param == NULL)
    {
        data_uart_print("queue out failed");
        return;
    }

    p_tc_param_data = (T_TC_PARAM_DATA *)(p_tc_param->p_data);

    app_set_cur_case((T_CUR_TEST_CASE)p_tc_param_data->id);

    switch (role)
    {
    case TC_ROLE_DUT:
        tc_dut_start_next_case(p_tc_param_data->id, p_tc_param_data);
        break;

    case TC_ROLE_SUT:
        tc_sut_start_next_case(p_tc_param_data->id, p_tc_param_data);
        break;

    default:
        break;
    }

    os_mem_free(p_tc_param->p_data);
    os_mem_free(p_tc_param);

}

