/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      gap_test_app.c
   * @brief     Gap roles test application implementation.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include <string.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_le.h>
#include <gap_scan.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <gap_msg.h>
#include <app_msg.h>
#include <link_mgr.h>
#include <user_cmd.h>
#include <user_cmd_parse.h>
#include <complete_ble_service.h>
#include <gatt_builtin_services.h>
#include <gap_test_app.h>
#if F_BT_LE_GATT_CLIENT_SUPPORT
#include <complete_ble_client.h>
#include <gaps_client.h>
#endif
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
#include <gap_ext_scan.h>
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
#include <gap_ext_adv.h>
#endif
#if F_BT_LE_PRIVACY_SUPPORT
#include <privacy_mgnt.h>
#include <gap_privacy.h>
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
#include <gap_credit_based_conn.h>
#endif
#if F_BT_ANCS_CLIENT_SUPPORT
#include <ancs_client.h>
#include <ancs.h>
#endif
#include <hids_kb.h>
#include <uart_task.h>
#include <os_timer.h>
#include "os_mem.h"
/*============================================================================*
 *                              Variables
 *============================================================================*/
#if F_BT_LE_GATT_CLIENT_SUPPORT
T_CLIENT_ID     simple_ble_client_id = 0xf0;  /**< Simple ble service client id*/
T_CLIENT_ID     gaps_client_id = 0xf0;        /**< gap service client id*/
#endif
T_SERVER_ID     simp_srv_id = 0xff; /**< Simple ble service id*/
T_SERVER_ID     hid_srv_id = 0xff;

T_GAP_TEST_CASE gap_test_case = GAP_TC_00_NORMAL;
T_GAP_V3_NOTIF_TEST gap_v3_notif_test;

T_GAP_DEV_STATE gap_dev_state = {0, 0, 0, 0, 0};                 /**< GAP device state */
#if F_BT_LE_PRIVACY_SUPPORT
T_PRIVACY_STATE priv_state = PRIVACY_STATE_INIT;
#endif
/*============================================================================*
 *                              Functions
 *============================================================================*/
void app_handle_gap_msg(T_IO_MSG  *p_gap_msg);
void app_handle_bond_modify_msg(T_LE_BOND_MODIFY_TYPE type, T_LE_KEY_ENTRY *p_entry);
/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {
            app_handle_gap_msg(&io_msg);
        }
        break;
    case IO_MSG_TYPE_QDECODE:
        {
            if (io_msg.subtype == 0)
            {
                le_adv_start();
            }
            else if (io_msg.subtype == 1)
            {
                le_scan_start();
            }
        }
        break;
#if F_BT_ANCS_CLIENT_SUPPORT
    case IO_MSG_TYPE_ANCS:
        {
            ancs_handle_msg(&io_msg);
        }
        break;
#endif
    default:
        break;
    }
}

/**
 * @brief    Handle msg GAP_MSG_LE_DEV_STATE_CHANGE
 * @note     All the gap device state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] new_state  New gap device state
 * @param[in] cause GAP device state change cause
 * @return   void
 */
void app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO5("app_handle_dev_state_evt: init state  %d, adv state %d, scan state %d, conn state %d, cause 0x%x",
                    new_state.gap_init_state, new_state.gap_adv_state,
                    new_state.gap_scan_state, new_state.gap_conn_state, cause);
    if (gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            uint8_t bt_addr[6];
            data_uart_print("GAP stack ready\r\n");
            gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
            data_uart_print("local bd addr: 0x%2x:%2x:%2x:%2x:%2x:%2x\r\n",
                            bt_addr[5], bt_addr[4], bt_addr[3],
                            bt_addr[2], bt_addr[1], bt_addr[0]);
            if (test_case_id != TC_IDLE)
            {
                app_send_msg_to_uart_app(TC_STACK_STARTED, 0);
            }
#if F_BT_LE_PRIVACY_SUPPORT
            if (gap_test_case == GAP_TC_03_PRIVACY)
            {
                privacy_init_resolving_list(app_privacy_callback);
            }
#endif
            APP_PRINT_INFO1("GAP_INIT_STATE_STACK_READY: %d", os_mem_peek(RAM_TYPE_DATA_ON));
        }
    }

    if (gap_dev_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");
            data_uart_print("GAP scan stop\r\n");
        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");
            data_uart_print("GAP scan start\r\n");
            if (test_case_id == TC_0003_SCAN)
            {
                app_send_msg_to_uart_app(TC_START_SCAN, 0);
            }
        }
    }

    if (gap_dev_state.gap_adv_state != new_state.gap_adv_state)
    {
        if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
            {
                APP_PRINT_INFO0("GAP adv stoped: because connection created");
                data_uart_print("GAP adv stoped:because conn\r\n");
            }
            else
            {
                APP_PRINT_INFO0("GAP adv stoped");
                data_uart_print("GAP adv stoped\r\n");
            }
        }
        else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            APP_PRINT_INFO0("GAP adv start");
            data_uart_print("GAP adv start\r\n");
            if (test_case_id == TC_0002_ADV)
            {
                app_send_msg_to_uart_app(TC_START_ADV, 0);
            }
        }
    }

    if (gap_dev_state.gap_conn_state != new_state.gap_conn_state)
    {
        APP_PRINT_INFO2("conn state: %d -> %d",
                        gap_dev_state.gap_conn_state,
                        new_state.gap_conn_state);
    }
#if F_BT_LE_PRIVACY_SUPPORT
    if (gap_test_case == GAP_TC_03_PRIVACY)
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
    gap_dev_state = new_state;
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_STATE_CHANGE
 * @note     All the gap conn state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New gap connection state
 * @param[in] cause Use this cause when new_state is GAP_CONN_STATE_DISCONNECTED
 * @return   void
 */
void app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    if (conn_id >= APP_MAX_LINKS)
    {
        return;
    }

    APP_PRINT_INFO4("app_handle_conn_state_evt: conn_id %d, conn_state(%d -> %d), disc_cause 0x%x",
                    conn_id, app_link_table[conn_id].conn_state, new_state, disc_cause);

    app_link_table[conn_id].conn_state = new_state;
    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTED:
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
            {
                APP_PRINT_ERROR2("app_handle_conn_state_evt: connection lost, conn_id %d, cause 0x%x", conn_id,
                                 disc_cause);
            }

            data_uart_print("Disconnect conn_id %d, cause 0x%x\r\n", conn_id, disc_cause);
            memset(&app_link_table[conn_id], 0, sizeof(T_APP_LINK));
            if (gap_v3_notif_test.v3_tx_num != 0 && gap_v3_notif_test.v3_tx_conn_id == conn_id)
            {
                memset(&gap_v3_notif_test, 0, sizeof(T_GAP_V3_NOTIF_TEST));
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        {
            uint8_t local_bd_type;
            uint8_t features[8];
            uint8_t remote_bd_type;
            le_get_conn_param(GAP_PARAM_CONN_LOCAL_BD_TYPE, &local_bd_type, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_BD_ADDR_TYPE, &remote_bd_type, conn_id);
            APP_PRINT_INFO3("GAP_CONN_STATE_CONNECTED: conn_id %d, local_bd_type %d, remote_bd_type %d",
                            conn_id, local_bd_type, remote_bd_type);
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
            {
                uint8_t tx_phy;
                uint8_t rx_phy;
                le_get_conn_param(GAP_PARAM_CONN_RX_PHY_TYPE, &rx_phy, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_TX_PHY_TYPE, &tx_phy, conn_id);
                APP_PRINT_INFO2("GAP_CONN_STATE_CONNECTED: tx_phy %d, rx_phy %d", tx_phy, rx_phy);
            }
#endif
#if F_BT_LE_READ_REMOTE_FEATS
            le_get_conn_param(GAP_PARAM_CONN_REMOTE_FEATURES, &features, conn_id);
#endif
#if F_BT_LE_5_0_CSA2_SUPPORT
            {
                uint8_t csa;
                le_get_conn_param(GAP_PARAM_CONN_CHANN_ALGORITHM, &csa, conn_id);
                APP_PRINT_INFO1("GAP_CONN_STATE_CONNECTED: csa %d", csa);
            }
#endif
            data_uart_print("connected success conn_id = %d, local_bd_type %d\r\n", conn_id,
                            local_bd_type);
            if (gap_test_case == GAP_TC_09_SLAVE_LATENCY)
            {
                le_update_conn_param(conn_id, 10, 10, 50, 500, 19, 19);
            }
            if (test_case_id == TC_0004_CON_TX
                || test_case_id == TC_0005_CON_RX)
            {
                os_timer_start(&auto_test_timer);
            }
        }
        break;

    default:
        break;

    }
}

/**
 * @brief    Handle msg GAP_MSG_LE_AUTHEN_STATE_CHANGE
 * @note     All the gap authentication state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New authentication state
 * @param[in] cause Use this cause when new_state is GAP_AUTHEN_STATE_COMPLETE
 * @return   void
 */
void app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO2("app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            data_uart_print("pair start\r\n");
            APP_PRINT_INFO0("app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
        }
        break;

    case GAP_AUTHEN_STATE_COMPLETE:
        {
            if (cause == GAP_SUCCESS)
            {
                data_uart_print("Pair success\r\n");
                APP_PRINT_INFO0("app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");
#if F_BT_GAP_KEY_MANAGER_SUPPORT
                {
                    uint8_t addr[6];
                    T_GAP_REMOTE_ADDR_TYPE bd_type;
                    uint8_t resolved_addr[6];
                    T_GAP_IDENT_ADDR_TYPE resolved_bd_type;
                    le_get_conn_addr(conn_id, addr, &bd_type);
                    if (bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
                    {
                        if (le_resolve_random_address(addr, resolved_addr, &resolved_bd_type))
                        {
                            APP_PRINT_INFO2("GAP_AUTHEN_STATE_COMPLETE: resolved_addr %s, resolved_addr_type %d",
                                            TRACE_BDADDR(resolved_addr), resolved_bd_type);
                        }
                        else
                        {
                            APP_PRINT_INFO0("GAP_AUTHEN_STATE_COMPLETE: resolved addr failed");
                        }
                    }
                }
#endif
            }
            else
            {
                data_uart_print("Pair failed: cause 0x%x\r\n", cause);
                APP_PRINT_INFO0("app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");
            }
        }
        break;

    default:
        {
            APP_PRINT_ERROR1("app_handle_authen_state_evt: unknown newstate %d", new_state);
        }
        break;
    }
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_MTU_INFO
 * @note     This msg is used to inform APP that exchange mtu procedure is completed.
 * @param[in] conn_id Connection ID
 * @param[in] mtu_size  New mtu size
 * @return   void
 */
void app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note     All the connection parameter update change  events are pre-handled in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause Use this cause when status is GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 * @return   void
 */
void app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
{
    switch (status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
        {
            uint16_t conn_interval;
            uint16_t conn_slave_latency;
            uint16_t conn_supervision_timeout;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
            data_uart_print("conn param update success\r\n");
            APP_PRINT_INFO4("app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            data_uart_print("conn param update fail\r\n");
            APP_PRINT_ERROR2("app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x",
                             conn_id, cause);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            APP_PRINT_INFO1("app_handle_conn_param_update_evt update pending: conn_id %d", conn_id);
        }
        break;
    default:
        break;
    }
}

#if F_BT_LE_5_0_AE_ADV_SUPPORT
void app_handle_ext_adv_state_evt(uint8_t adv_handle, T_GAP_EXT_ADV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO2("app_handle_ext_adv_state_evt: adv_handle = %d newState = %d",
                    adv_handle, new_state);
    switch (new_state)
    {
    /*device is disconnected.*/
    case EXT_ADV_STATE_IDLE:
        {
            APP_PRINT_INFO2("EXT_ADV_STATE_IDLE: adv_handle %d, cause 0x%x",
                            adv_handle, cause);
        }
        break;

    /*device is connected*/
    case EXT_ADV_STATE_ADVERTISING:
        {
            APP_PRINT_INFO2("EXT_ADV_STATE_ADVERTISING: adv_handle %d, cause 0x%x",
                            adv_handle, cause);
        }
        break;

    /*error comes here*/
    default:
        break;
    }
}
#endif
/**
 * @brief    All the BT GAP MSG are pre-handled in this function.
 * @note     Then the event handling function shall be called according to the
 *           subtype of T_IO_MSG
 * @param[in] p_gap_msg Pointer to GAP msg
 * @return   void
 */
void app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    uint8_t conn_id;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    APP_PRINT_TRACE1("app_handle_gap_msg: subtype %d", p_gap_msg->subtype);
    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                     gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            app_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                      (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                      gap_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            app_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                         gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            app_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
                                             gap_msg.msg_data.gap_conn_param_update.status,
                                             gap_msg.msg_data.gap_conn_param_update.cause);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
                                        gap_msg.msg_data.gap_authen_state.new_state,
                                        gap_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {
            uint32_t display_value = 0;
            conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn_id %d, passkey %d",
                            conn_id, display_value);
            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            data_uart_print("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn_id %d, passkey %d\r\n",
                            conn_id,
                            display_value);
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            uint32_t display_value = 0;
            conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_USER_CONFIRMATION: conn_id %d, passkey %d",
                            conn_id, display_value);
            data_uart_print("GAP_MSG_LE_BOND_USER_CONFIRMATION: conn_id %d, passkey %d\r\n",
                            conn_id,
                            display_value);
            //le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
            //uint32_t passkey = 888888;
            conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d, key_press %d",
                            conn_id, gap_msg.msg_data.gap_bond_passkey_input.key_press);
            data_uart_print("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d\r\n", conn_id);
            //le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
#if F_BT_LE_SMP_OOB_SUPPORT
    case GAP_MSG_LE_BOND_OOB_INPUT:
        {
            conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_OOB_INPUT: conn_id %d", conn_id);
            data_uart_print("GAP_MSG_LE_BOND_OOB_INPUT conn id=%d\r\n", conn_id);
            //uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            //le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
            //le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE:
        {
            app_handle_ext_adv_state_evt(gap_msg.msg_data.gap_ext_adv_state_change.adv_handle,
                                         (T_GAP_EXT_ADV_STATE)gap_msg.msg_data.gap_ext_adv_state_change.new_state,
                                         gap_msg.msg_data.gap_ext_adv_state_change.cause);
        }
        break;
#endif
    default:
        APP_PRINT_ERROR1("app_handle_gap_msg: unknown subtype %d", p_gap_msg->subtype);
        break;
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

#if F_BT_LE_APP_KEY_MANAGER
void app_handle_authen_result_ind(T_LE_AUTHEN_RESULT_IND *auth_ind)
{
    APP_PRINT_INFO5("app_handle_authen_result_ind: bd_addr %s, remote_addr_type %d, key_len %d, key_type %d, cause 0x%x",
                    TRACE_BDADDR(auth_ind->bd_addr),
                    auth_ind->remote_addr_type,
                    auth_ind->key_len,
                    auth_ind->key_type,
                    auth_ind->cause);
    le_bond_authen_result_confirm(auth_ind->bd_addr, auth_ind->remote_addr_type,
                                  auth_ind->key_type, GAP_CFM_CAUSE_ACCEPT);
}
void app_handle_authen_key_req_ind(T_LE_AUTHEN_KEY_REQ_IND *key_req_ind)
{
    APP_PRINT_INFO3("app_handle_authen_key_req_ind: bd_addr %s, remote_addr_type %d, key_type %d",
                    TRACE_BDADDR(key_req_ind->bd_addr),
                    key_req_ind->remote_addr_type,
                    key_req_ind->key_type);
    le_bond_authen_key_req_confirm(key_req_ind->bd_addr,
                                   key_req_ind->remote_addr_type,
                                   0,
                                   NULL,
                                   key_req_ind->key_type,
                                   GAP_CFM_CAUSE_REJECT
                                  );
}
void app_handle_gatt_server_store_ind(T_LE_GATT_SERVER_STORE_IND *store_ind)
{
    APP_PRINT_INFO4("app_handle_gatt_server_store_ind: bd_addr %s, remote_addr_type %d, op %d, data_len %d",
                    TRACE_BDADDR(store_ind->bd_addr),
                    store_ind->remote_addr_type,
                    store_ind->op,
                    store_ind->data_len);
    le_bond_gatt_server_store_confirm(store_ind->op,
                                      store_ind->bd_addr,
                                      store_ind->remote_addr_type,
                                      0,
                                      NULL,
                                      GAP_CFM_CAUSE_REJECT
                                     );
}
#endif

/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
    /* common msg*/
    case GAP_MSG_LE_READ_RSSI:
        APP_PRINT_INFO3("GAP_MSG_LE_READ_RSSI:conn_id 0x%x cause 0x%x rssi %d",
                        p_data->p_le_read_rssi_rsp->conn_id,
                        p_data->p_le_read_rssi_rsp->cause,
                        p_data->p_le_read_rssi_rsp->rssi);
        break;
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, max_tx_octets 0x%x, max_tx_time 0x%x, max_rx_octets 0x%x, max_rx_time 0x%x",
                        p_data->p_le_data_len_change_info->conn_id,
                        p_data->p_le_data_len_change_info->max_tx_octets,
                        p_data->p_le_data_len_change_info->max_tx_time,
                        p_data->p_le_data_len_change_info->max_rx_octets,
                        p_data->p_le_data_len_change_info->max_rx_time);
        break;

    case GAP_MSG_LE_SET_DATA_LEN:
        APP_PRINT_INFO2("GAP_MSG_LE_SET_DATA_LEN: conn_id 0x%x, cause 0x%x",
                        p_data->p_le_set_data_len_rsp->conn_id,
                        p_data->p_le_set_data_len_rsp->cause);
        break;
#endif
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
        break;

    case GAP_MSG_LE_SCAN_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_SCAN_INFO:adv_type 0x%x, bd_addr %s, remote_addr_type %d, rssi %d, data_len %d",
                        p_data->p_le_scan_info->adv_type,
                        TRACE_BDADDR(p_data->p_le_scan_info->bd_addr),
                        p_data->p_le_scan_info->remote_addr_type,
                        p_data->p_le_scan_info->rssi,
                        p_data->p_le_scan_info->data_len);

        /* User can split interested information by using the function as follow. */
        if (filter_scan_info_by_uuid(GATT_UUID_SIMPLE_PROFILE, p_data->p_le_scan_info))
        {
            APP_PRINT_INFO0("Found simple ble service");
            link_mgr_add_device(p_data->p_le_scan_info->bd_addr, p_data->p_le_scan_info->remote_addr_type);
        }
        /* If you want to parse the scan info, please reference function app_parse_scan_info in observer app. */
        break;
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO5("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        if (p_data->p_le_conn_update_ind->conn_latency > 60)//only used for test
        {
            result = APP_RESULT_REJECT;
        }
        else
        {
            result = APP_RESULT_ACCEPT;
        }
        break;

    case GAP_MSG_LE_SET_HOST_CHANN_CLASSIF:
        APP_PRINT_INFO1("GAP_MSG_LE_SET_HOST_CHANN_CLASSIF: cause 0x%x",
                        p_data->p_le_set_host_chann_classif_rsp->cause);
        break;
#endif
#if F_BT_LE_READ_CHANN_MAP
    case GAP_MSG_LE_READ_CHANN_MAP:
        APP_PRINT_INFO7("GAP_MSG_LE_READ_CHANN_MAP: conn_id 0x%x, cause 0x%x, map[0x%x:0x%x:0x%x:0x%x:0x%x]",
                        p_data->p_le_read_chann_map_rsp->conn_id,
                        p_data->p_le_read_chann_map_rsp->cause,
                        p_data->p_le_read_chann_map_rsp->channel_map[0],
                        p_data->p_le_read_chann_map_rsp->channel_map[1],
                        p_data->p_le_read_chann_map_rsp->channel_map[2],
                        p_data->p_le_read_chann_map_rsp->channel_map[3],
                        p_data->p_le_read_chann_map_rsp->channel_map[4]);
        break;
#endif
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
    case GAP_MSG_LE_ADV_READ_TX_POWER:
        APP_PRINT_INFO2("GAP_MSG_LE_ADV_READ_TX_POWER: cause 0x%x, tx_power_level 0x%x",
                        p_data->p_le_adv_read_tx_power_rsp->cause,
                        p_data->p_le_adv_read_tx_power_rsp->tx_power_level);
        break;
#endif

    case GAP_MSG_LE_SET_RAND_ADDR:
        APP_PRINT_INFO1("GAP_MSG_LE_SET_RAND_ADDR: cause 0x%x",
                        p_data->p_le_set_rand_addr_rsp->cause);
        break;

    case GAP_MSG_LE_ADV_UPDATE_PARAM:
        APP_PRINT_INFO1("GAP_MSG_LE_ADV_UPDATE_PARAM: cause 0x%x",
                        p_data->p_le_adv_update_param_rsp->cause);
        break;
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_DISABLE_SLAVE_LATENCY:
        APP_PRINT_INFO1("GAP_MSG_LE_DISABLE_SLAVE_LATENCY: cause 0x%x",
                        p_data->p_le_disable_slave_latency_rsp->cause);
        break;
#endif
    case GAP_MSG_LE_CREATE_CONN_IND:
        APP_PRINT_INFO0("GAP_MSG_LE_CREATE_CONN_IND");
        result = APP_RESULT_ACCEPT;
        break;
#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
    case GAP_MSG_LE_KEYPRESS_NOTIFY:
        APP_PRINT_INFO2("GAP_MSG_LE_KEYPRESS_NOTIFY:conn %d, cause 0x%x",
                        p_data->p_le_keypress_notify_rsp->conn_id, p_data->p_le_keypress_notify_rsp->cause);
        break;

    case GAP_MSG_LE_KEYPRESS_NOTIFY_INFO:
        APP_PRINT_INFO2("GAP_MSG_LE_KEYPRESS_NOTIFY_INFO:conn %d, type 0x%x",
                        p_data->p_le_keypress_notify_info->conn_id, p_data->p_le_keypress_notify_info->event_type);
        break;
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:
        APP_PRINT_INFO1("GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:cause 0x%x",
                        p_data->p_le_update_passed_chann_map_rsp->cause);
        break;
#endif
#if F_BT_LE_READ_REMOTE_FEATS
    case GAP_MSG_LE_REMOTE_FEATS_INFO:
        {
            uint8_t  remote_feats[8];
            APP_PRINT_INFO3("GAP_MSG_LE_REMOTE_FEATS_INFO: conn id %d, cause 0x%x, remote_feats %b",
                            p_data->p_le_remote_feats_info->conn_id,
                            p_data->p_le_remote_feats_info->cause,
                            TRACE_BINARY(8, p_data->p_le_remote_feats_info->remote_feats));
            if (p_data->p_le_remote_feats_info->cause == GAP_SUCCESS)
            {
                memcpy(remote_feats, p_data->p_le_remote_feats_info->remote_feats, 8);
                if (remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] & LE_SUPPORT_FEATURES_LE_2M_MASK_BIT)
                {
                    APP_PRINT_INFO0("GAP_MSG_LE_REMOTE_FEATS_INFO: support 2M");
                }
                if (remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] & LE_SUPPORT_FEATURES_LE_CODED_PHY_MASK_BIT)
                {
                    APP_PRINT_INFO0("GAP_MSG_LE_REMOTE_FEATS_INFO: support CODED");
                }
            }
        }
        break;
#endif
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
    case GAP_MSG_LE_PHY_UPDATE_INFO:
        APP_PRINT_INFO4("GAP_MSG_LE_PHY_UPDATE_INFO:conn_id %d, cause 0x%x, rx_phy %d, tx_phy %d",
                        p_data->p_le_phy_update_info->conn_id,
                        p_data->p_le_phy_update_info->cause,
                        p_data->p_le_phy_update_info->rx_phy,
                        p_data->p_le_phy_update_info->tx_phy);
        break;
#endif
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
    case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
        APP_PRINT_INFO6("GAP_MSG_LE_EXT_ADV_REPORT_INFO:connectable %d, scannable %d, direct %d, scan response %d, legacy %d, data status 0x%x",
                        p_data->p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_CONNECTABLE_ADV,
                        p_data->p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_SCANNABLE_ADV,
                        p_data->p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_DIRECTED_ADV,
                        p_data->p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_SCAN_RESPONSE,
                        p_data->p_le_ext_adv_report_info->event_type & GAP_EXT_ADV_REPORT_BIT_USE_LEGACY_ADV,
                        p_data->p_le_ext_adv_report_info->data_status);
        APP_PRINT_INFO5("GAP_MSG_LE_EXT_ADV_REPORT_INFO:event_type 0x%x, bd_addr %s, addr_type %d, rssi %d, data_len %d",
                        p_data->p_le_ext_adv_report_info->event_type,
                        TRACE_BDADDR(p_data->p_le_ext_adv_report_info->bd_addr),
                        p_data->p_le_ext_adv_report_info->addr_type,
                        p_data->p_le_ext_adv_report_info->rssi,
                        p_data->p_le_ext_adv_report_info->data_len);
        APP_PRINT_INFO5("GAP_MSG_LE_EXT_ADV_REPORT_INFO:primary_phy %d, secondary_phy %d, adv_sid %d, tx_power %d, peri_adv_interval %d",
                        p_data->p_le_ext_adv_report_info->primary_phy,
                        p_data->p_le_ext_adv_report_info->secondary_phy,
                        p_data->p_le_ext_adv_report_info->adv_sid,
                        p_data->p_le_ext_adv_report_info->tx_power,
                        p_data->p_le_ext_adv_report_info->peri_adv_interval);
        APP_PRINT_INFO2("GAP_MSG_LE_EXT_ADV_REPORT_INFO:direct_addr_type 0x%x, direct_addr %s",
                        p_data->p_le_ext_adv_report_info->direct_addr_type,
                        TRACE_BDADDR(p_data->p_le_ext_adv_report_info->direct_addr));
        link_mgr_add_device(p_data->p_le_ext_adv_report_info->bd_addr,
                            p_data->p_le_ext_adv_report_info->addr_type);
        break;
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        APP_PRINT_INFO3("GAP_MSG_LE_EXT_ADV_START_SETTING:cause 0x%x, flag 0x%x, adv_handle %d",
                        p_data->p_le_ext_adv_start_setting_rsp->cause,
                        p_data->p_le_ext_adv_start_setting_rsp->flag,
                        p_data->p_le_ext_adv_start_setting_rsp->adv_handle);
        break;
    case GAP_MSG_LE_EXT_ADV_REMOVE_SET:
        APP_PRINT_INFO2("GAP_MSG_LE_EXT_ADV_REMOVE_SET:cause 0x%x, adv_handle %d",
                        p_data->p_le_ext_adv_remove_set_rsp->cause,
                        p_data->p_le_ext_adv_remove_set_rsp->adv_handle);
        break;
    case GAP_MSG_LE_EXT_ADV_CLEAR_SET:
        APP_PRINT_INFO1("GAP_MSG_LE_EXT_ADV_CLEAR_SET:cause 0x%x",
                        p_data->p_le_ext_adv_clear_set_rsp->cause);
        break;
    case GAP_MSG_LE_EXT_ADV_ENABLE:
        APP_PRINT_INFO1("GAP_MSG_LE_EXT_ADV_ENABLE:cause 0x%x",
                        p_data->le_cause.cause);
        break;
    case GAP_MSG_LE_EXT_ADV_DISABLE:
        APP_PRINT_INFO1("GAP_MSG_LE_EXT_ADV_DISABLE:cause 0x%x",
                        p_data->le_cause.cause);
        break;
    case GAP_MSG_LE_SCAN_REQ_RECEIVED_INFO:
        APP_PRINT_INFO3("GAP_MSG_LE_SCAN_REQ_RECEIVED_INFO:adv_handle %d, scanner_addr_type 0x%x, scanner_addr %s",
                        p_data->p_le_scan_req_received_info->adv_handle,
                        p_data->p_le_scan_req_received_info->scanner_addr_type,
                        TRACE_BDADDR(p_data->p_le_scan_req_received_info->scanner_addr));
        break;
#endif
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        APP_PRINT_INFO2("GAP_MSG_LE_BOND_MODIFY_INFO: 0x%x, p_entry %p",
                        p_data->p_le_bond_modify_info->type, p_data->p_le_bond_modify_info->p_entry);
#if F_BT_LE_PRIVACY_SUPPORT
        if (gap_test_case == GAP_TC_03_PRIVACY)
        {
            app_handle_bond_modify_msg(p_data->p_le_bond_modify_info->type,
                                       p_data->p_le_bond_modify_info->p_entry);
        }
#endif
        break;
#if F_BT_LE_ATT_SIGNED_WRITE_SUPPORT
    case GAP_MSG_LE_GATT_SIGNED_STATUS_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_GATT_SIGNED_STATUS_INFO:conn_id %d, cause 0x%x, update_local %d, local_sign_count %d,remote_sign_count %d",
                        p_data->p_le_gatt_signed_status_info->conn_id,
                        p_data->p_le_gatt_signed_status_info->cause,
                        p_data->p_le_gatt_signed_status_info->update_local,
                        p_data->p_le_gatt_signed_status_info->local_sign_count,
                        p_data->p_le_gatt_signed_status_info->remote_sign_count);
        break;
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_VENDOR_ADV_3_DATA_ENABLE:
        APP_PRINT_INFO1("GAP_MSG_LE_VENDOR_ADV_3_DATA_ENABLE: cause 0x%x",
                        p_data->le_cause.cause);
        break;
    case GAP_MSG_LE_VENDOR_ADV_3_DATA_SET:
        APP_PRINT_INFO2("GAP_MSG_LE_VENDOR_ADV_3_DATA_SET: type %d, cause 0x%x",
                        p_data->p_le_vendor_adv_3_data_set_rsp->type,
                        p_data->p_le_vendor_adv_3_data_set_rsp->cause);
        break;
#endif
#if F_BT_LE_GAP_MSG_INFO_WAY
    case GAP_MSG_LE_GAP_STATE_MSG:
        APP_PRINT_INFO0("GAP_MSG_LE_GAP_STATE_MSG");
        app_handle_gap_msg(p_data->p_gap_state_msg);
        break;
#endif
#if F_BT_LE_APP_KEY_MANAGER
    case GAP_MSG_LE_AUTHEN_RESULT_IND:
        app_handle_authen_result_ind(p_data->p_le_authen_result_ind);
        break;
    case GAP_MSG_LE_AUTHEN_KEY_REQ_IND:
        app_handle_authen_key_req_ind(p_data->p_le_authen_key_req_ind);
        break;
    case GAP_MSG_LE_GATT_SERVER_STORE_IND:
        app_handle_gatt_server_store_ind(p_data->p_le_gatt_server_store_ind);
        break;
#endif
    default:
        APP_PRINT_ERROR1("app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}


/**
  * @brief Callback for gap common module to notify app
  * @param[in] cb_type callback msy type @ref GAP_COMMON_MSG_TYPE.
  * @param[in] p_cb_data point to callback data @ref T_GAP_CB_DATA.
  * @retval void
  */
void app_gap_common_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_CB_DATA));
    APP_PRINT_INFO1("app_gap_common_callback: cb_type = %d", cb_type);
    switch (cb_type)
    {
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_WRITE_AIRPLAN_MODE:
        APP_PRINT_INFO1("GAP_MSG_WRITE_AIRPLAN_MODE: cause 0x%x",
                        cb_data.p_gap_write_airplan_mode_rsp->cause);
        break;
    case GAP_MSG_READ_AIRPLAN_MODE:
        APP_PRINT_INFO2("GAP_MSG_READ_AIRPLAN_MODE: cause 0x%x, mode %d",
                        cb_data.p_gap_read_airplan_mode_rsp->cause,
                        cb_data.p_gap_read_airplan_mode_rsp->mode);
        break;
#endif
#if F_BT_CONTROLLER_POWER_CONTROL
    case GAP_MSG_BT_POWER_ON_RSP:
        APP_PRINT_INFO1("GAP_MSG_BT_POWER_ON_RSP: cause 0x%x",
                        cb_data.p_gap_bt_power_on_rsp->cause);
        break;
    case GAP_MSG_BT_POWER_OFF_RSP:
        APP_PRINT_INFO1("GAP_MSG_BT_POWER_OFF_RSP: cause 0x%x",
                        cb_data.p_gap_bt_power_off_rsp->cause);
        break;
#endif
    default:
        break;
    }
    return;
}
#if 0
void app_gap_vendor_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_VENDOR_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_VENDOR_CB_DATA));
    APP_PRINT_INFO1("app_gap_common_callback: cb_type = %d", cb_type);
    switch (cb_type)
    {
    case GAP_MSG_VENDOR_CMD_RSP:
        APP_PRINT_INFO4("GAP_MSG_VENDOR_CMD_RSP: command 0x%x, cause 0x%x, is_cmpl_evt %d, param_len %d",
                        cb_data.p_gap_vendor_cmd_rsp->command,
                        cb_data.p_gap_vendor_cmd_rsp->cause,
                        cb_data.p_gap_vendor_cmd_rsp->is_cmpl_evt,
                        cb_data.p_gap_vendor_cmd_rsp->param_len);
        break;
    case GAP_MSG_VENDOR_EVT_INFO:
        APP_PRINT_INFO1("GAP_MSG_VENDOR_EVT_INFO: param_len %d",
                        cb_data.p_gap_vendor_evt_info->param_len);
        break;
    default:
        break;
    }
    return;
}
#endif
#if F_BT_GAPS_CHAR_WRITEABLE
/**
 * @brief    All the BT GAP service callback events are handled in this function
 * @param[in] service_id  Profile service ID
 * @param[in] p_data      Pointer to callback data
 * @return   Indicates the function call is successful or not
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT gap_service_callback(T_SERVER_ID service_id, void *p_para)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GAPS_CALLBACK_DATA *p_gap_data = (T_GAPS_CALLBACK_DATA *)p_para;
    APP_PRINT_INFO2("gap_service_callback conn_id = %d msg_type = %d\n", p_gap_data->conn_id,
                    p_gap_data->msg_type);
    if (p_gap_data->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
    {
        switch (p_gap_data->msg_data.opcode)
        {
        case GAPS_WRITE_DEVICE_NAME:
            {
                T_LOCAL_NAME device_name;
                memcpy(device_name.local_name, p_gap_data->msg_data.p_value, p_gap_data->msg_data.len);
                device_name.local_name[p_gap_data->msg_data.len] = 0;
                flash_save_local_name(&device_name);
            }
            break;

        case GAPS_WRITE_APPEARANCE:
            {
                uint16_t appearance_val;
                T_LOCAL_APPEARANCE appearance;

                LE_ARRAY_TO_UINT16(appearance_val, p_gap_data->msg_data.p_value);
                appearance.local_appearance = appearance_val;
                flash_save_local_appearance(&appearance);
            }
            break;

        default:
            break;
        }
    }
    return result;
}
#endif

/**
 * @brief  Callback will be called when data sent from profile client layer.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */
#if F_BT_LE_GATT_CLIENT_SUPPORT
T_APP_RESULT app_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("app_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
    if (client_id == CLIENT_PROFILE_GENERAL_ID)
    {
        T_CLIENT_APP_CB_DATA *p_client_app_cb_data = (T_CLIENT_APP_CB_DATA *)p_data;
        switch (p_client_app_cb_data->cb_type)
        {
        case CLIENT_APP_CB_TYPE_DISC_STATE:
            if (p_client_app_cb_data->cb_content.disc_state_data.disc_state == DISC_STATE_SRV_DONE)
            {
                APP_PRINT_INFO0("Discovery All Service Procedure Done.");
            }
            else
            {
                APP_PRINT_INFO0("Discovery state send to application directly.");
            }
            break;
        case CLIENT_APP_CB_TYPE_DISC_RESULT:
            if (p_client_app_cb_data->cb_content.disc_result_data.result_type == DISC_RESULT_ALL_SRV_UUID16)
            {
                APP_PRINT_INFO3("Discovery All Primary Service: UUID16 0x%x, start handle 0x%x, end handle 0x%x.",
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

    }
    else if (client_id == gaps_client_id)
    {
        T_GAPS_CLIENT_CB_DATA *p_gaps_cb_data = (T_GAPS_CLIENT_CB_DATA *)p_data;
        switch (p_gaps_cb_data->cb_type)
        {
        case GAPS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_gaps_cb_data->cb_content.disc_state)
            {
            case DISC_GAPS_DONE:
                /* Discovery Simple BLE service procedure successfully done. */
                APP_PRINT_INFO0("app_client_callback: discover gaps procedure done.");
                break;
            case DISC_GAPS_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("app_client_callback: discover gaps request failed.");
                break;
            default:
                break;
            }
            break;
        case GAPS_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_gaps_cb_data->cb_content.read_result.type)
            {
            case GAPS_READ_DEVICE_NAME:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_DEVICE_NAME: device name %s.",
                                    TRACE_STRING(p_gaps_cb_data->cb_content.read_result.data.device_name.p_value));
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_DEVICE_NAME: failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_APPEARANCE:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_APPEARANCE: appearance %d",
                                    p_gaps_cb_data->cb_content.read_result.data.appearance);
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_APPEARANCE: failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_CENTRAL_ADDR_RESOLUTION:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: central_addr_res %d",
                                    p_gaps_cb_data->cb_content.read_result.data.central_addr_res);
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
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
    else if (client_id == simple_ble_client_id)
    {
        T_SIMP_CLIENT_CB_DATA *p_simp_client_cb_data = (T_SIMP_CLIENT_CB_DATA *)p_data;
        uint16_t value_size;
        uint8_t *p_value;
        switch (p_simp_client_cb_data->cb_type)
        {
        case SIMP_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_simp_client_cb_data->cb_content.disc_state)
            {
            case DISC_SIMP_DONE:
                APP_PRINT_INFO0("app_client_callback: discover simp procedure done.");
                break;
            case DISC_SIMP_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("app_client_callback: discover simp request failed.");
                break;
            default:
                break;
            }
            break;
        case SIMP_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_simp_client_cb_data->cb_content.read_result.type)
            {
            case SIMP_READ_V1_READ:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    value_size = p_simp_client_cb_data->cb_content.read_result.data.v1_read.value_size;
                    p_value = p_simp_client_cb_data->cb_content.read_result.data.v1_read.p_value;
                    APP_PRINT_INFO2("SIMP_READ_V1_READ: value_size %d, value %b",
                                    value_size, TRACE_BINARY(value_size, p_value));
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V1_READ: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }
                break;
            case SIMP_READ_V3_NOTIFY_CCCD:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("SIMP_READ_V3_NOTIFY_CCCD: notify %d",
                                    p_simp_client_cb_data->cb_content.read_result.data.v3_notify_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V3_NOTIFY_CCCD: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                };
                break;
            case SIMP_READ_V4_INDICATE_CCCD:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("SIMP_READ_V4_INDICATE_CCCD: indicate %d",
                                    p_simp_client_cb_data->cb_content.read_result.data.v4_indicate_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V4_INDICATE_CCCD: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                };
                break;

            case SIMP_READ_V8_CCCD:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("SIMP_READ_V8_CCCD: cccd 0x%x",
                                    p_simp_client_cb_data->cb_content.read_result.data.v8_notify_ind_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V8_CCCD: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                };
                break;

            case SIMP_READ_V7_READ_LONG:
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    value_size = p_simp_client_cb_data->cb_content.read_result.data.v7_read.value_size;
                    p_value = p_simp_client_cb_data->cb_content.read_result.data.v7_read.p_value;
                    APP_PRINT_INFO2("SIMP_READ_V7_READ_LONG: value_size %d, value %b",
                                    value_size, TRACE_BINARY(value_size, p_value));
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V7_READ_LONG: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }
                break;

            default:
                break;
            }
            break;

        case SIMP_CLIENT_CB_TYPE_WRITE_RESULT:
            switch (p_simp_client_cb_data->cb_content.write_result.type)
            {
            case SIMP_WRITE_V2_WRITE:
                APP_PRINT_INFO1("SIMP_WRITE_V2_WRITE: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            case SIMP_WRITE_V3_NOTIFY_CCCD:
                APP_PRINT_INFO1("SIMP_WRITE_V3_NOTIFY_CCCD: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            case SIMP_WRITE_V4_INDICATE_CCCD:
                APP_PRINT_INFO1("SIMP_WRITE_V4_INDICATE_CCCD: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            case SIMP_WRITE_V6_WRITE_LONG:
                APP_PRINT_INFO1("SIMP_WRITE_V6_WRITE_LONG: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            case SIMP_WRITE_V8_CCCD:
                APP_PRINT_INFO1("SIMP_WRITE_V8_CCCD: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                break;
            default:
                break;
            }
            break;

        case SIMP_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
            value_size = p_simp_client_cb_data->cb_content.notif_ind_data.data.value_size;
            p_value = p_simp_client_cb_data->cb_content.notif_ind_data.data.p_value;
            switch (p_simp_client_cb_data->cb_content.notif_ind_data.type)
            {
            case SIMP_V3_NOTIFY:
                APP_PRINT_INFO2("SIMP_V3_NOTIFY: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                if (gap_v3_notif_test.v3_tx_conn_id == conn_id)
                {
                    gap_v3_notif_test.v3_rx_num++;
                }
                break;
            case SIMP_V4_INDICATE:
                APP_PRINT_INFO2("SIMP_V4_INDICATE: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                break;
            case SIMP_V8_INDICATE:
                APP_PRINT_INFO2("SIMP_V8_INDICATE: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                break;
            case SIMP_V8_NOTIFY:
                if (gap_v3_notif_test.v3_tx_conn_id == conn_id)
                {
                    gap_v3_notif_test.v8_rx_num++;
                }
                APP_PRINT_INFO2("SIMP_V8_NOTIFY: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
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
#endif
/**
 * @brief    All the BT Profile service callback events are handled in this function
 * @note     Then the event handling function shall be called according to the
 *           service_id.
 * @param[in] service_id  Profile service ID
 * @param[in] p_data      Pointer to callback data
 * @return   Indicates the function call is successful or not
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT app_profile_callback(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    if (service_id == SERVICE_PROFILE_GENERAL_ID)
    {
        T_SERVER_APP_CB_DATA *p_param = (T_SERVER_APP_CB_DATA *)p_data;
        switch (p_param->eventId)
        {
        case PROFILE_EVT_SRV_REG_COMPLETE:// srv register result event.
            APP_PRINT_INFO1("PROFILE_EVT_SRV_REG_COMPLETE: result %d",
                            p_param->event_data.service_reg_result);
            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
            if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
                if (test_case_id == TC_0004_CON_TX)
                {
                    uint8_t data[100] = {0, 1, 2};
                    simp_ble_service_send_v3_notify(0, simp_srv_id, data, 100);
                }
#if 0
                if (gap_v3_notif_test.v3_tx_conn_id == p_param->event_data.send_data_result.conn_id)
                {
                    uint8_t credit = p_param->event_data.send_data_result.credits;
                    uint8_t notif_val[244];
                    gap_v3_notif_test.v3_tx_cmp_num++;

                    for (; credit > 0; credit--)
                    {
                        if (gap_v3_notif_test.v3_tx_num == 0)
                        {
                            break;
                        }
                        memset(notif_val, gap_v3_notif_test.v3_tx_idx, gap_v3_notif_test.v3_tx_len);
                        if (gap_v3_notif_test.v3_tx_num % 2 == 0 || gap_test_case != GAP_TC_13_V3_V8_TX)
                        {

                            if (simp_ble_service_send_v3_notify(gap_v3_notif_test.v3_tx_conn_id, simp_srv_id,
                                                                &notif_val,
                                                                gap_v3_notif_test.v3_tx_len))
                            {
                                gap_v3_notif_test.v3_tx_idx++;
                                gap_v3_notif_test.v3_tx_num--;
                                gap_v3_notif_test.v3_tx_cnt++;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {

                            if (simp_ble_service_simple_v8_notify(gap_v3_notif_test.v3_tx_conn_id, simp_srv_id,
                                                                  &notif_val,
                                                                  gap_v3_notif_test.v3_tx_len))
                            {
                                gap_v3_notif_test.v3_tx_idx++;
                                gap_v3_notif_test.v3_tx_num--;
                                gap_v3_notif_test.v8_tx_cnt++;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
#endif
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
    else  if (service_id == simp_srv_id)
    {
        TSIMP_CALLBACK_DATA *p_simp_cb_data = (TSIMP_CALLBACK_DATA *)p_data;
        switch (p_simp_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_simp_cb_data->msg_data.notification_indification_index)
                {
                case SIMP_NOTIFY_INDICATE_V3_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_ENABLE");
                        if (test_case_id == TC_0004_CON_TX)
                        {
                            uint8_t data[100] = {0, 1, 2};
                            uint8_t credits = 0;
                            le_get_gap_param(GAP_PARAM_LE_REMAIN_CREDITS, &credits);
                            for (; credits > 0; credits--)
                            {
                                simp_ble_service_send_v3_notify(0, simp_srv_id, data, 100);
                            }
                        }
                    }
                    break;

                case SIMP_NOTIFY_INDICATE_V3_DISABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_DISABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_ENABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_DISABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_DISABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V8_NOTIFY_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_NOTIFY_ENABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V8_INDICATE_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_INDICATE_ENABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V8_NOTIFY_INDICATE_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_NOTIFY_INDICATE_ENABLE");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V8_DISABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_DISABLE");
                    }
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_simp_cb_data->msg_data.read_value_index == SIMP_READ_V1)
                {
                    uint8_t value = 0x88;
                    APP_PRINT_INFO1("SIMP_READ_V1: 0x%x", value);
                    simp_ble_service_set_parameter(SIMPLE_BLE_SERVICE_PARAM_V1_READ_CHAR_VAL, 1, &value);
                }
            }
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                switch (p_simp_cb_data->msg_data.write.opcode)
                {
                case SIMP_WRITE_V2:
                    {
                        APP_PRINT_INFO2("SIMP_WRITE_V2: write type %d, len %d", p_simp_cb_data->msg_data.write.write_type,
                                        p_simp_cb_data->msg_data.write.len);
                    }
                    break;

                case SIMP_WRITE_V6:
                    {
                        APP_PRINT_INFO1("SIMP_WRITE_V6: len = 0x%x", p_simp_cb_data->msg_data.write.len);
                        data_uart_print("V6 len = %d\r\n", p_simp_cb_data->msg_data.write.len);
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        default:
            break;
        }
    }
#if APP_HID_TEST
    else  if (service_id == hid_srv_id)
    {
        T_HID_CALLBACK_DATA *p_hid_cb_data = (T_HID_CALLBACK_DATA *)p_data;

        switch (p_hid_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                APP_PRINT_INFO2("SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION: index %d, value %d",
                                p_hid_cb_data->msg_data.not_ind_data.index,
                                p_hid_cb_data->msg_data.not_ind_data.value);
            }
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                switch (p_hid_cb_data->msg_data.read_value_index)
                {
                case GATT_SVC_HID_PROTOCOL_MODE_INDEX:
                    {
                        APP_PRINT_INFO1("HID_WRITE_PROTOCOL MODE %d\n",
                                        p_hid_cb_data->msg_data.write_msg.write_parameter.protocol_mode);
                        break;
                    }
                case GATT_SVC_HID_REPORT_OUTPUT_INDEX:
                    {
                        //callback data definition need to modify.
                        APP_PRINT_INFO1("HID_OUTPUT value %d\n", p_hid_cb_data->msg_data.write_msg.write_parameter.output);
                        break;
                    }
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
    }
#endif
    return app_result;
}


#if F_BT_LE_PRIVACY_SUPPORT
void app_privacy_callback(T_PRI_CB_TYPE type, uint8_t status)
{
    if (type == PRIVACY_STATE_MSGTYPE)
    {
        APP_PRINT_INFO2("app_privacy_callback: state (%d->%d)", priv_state, status);
        priv_state = (T_PRIVACY_STATE)status;
    }
}

void app_handle_bond_modify_msg(T_LE_BOND_MODIFY_TYPE type, T_LE_KEY_ENTRY *p_entry)
{
    APP_PRINT_INFO1("app_handle_bond_modify_msg  GAP_MSG_LE_BOND_MODIFY_INFO:type=0x%x",
                    type);
    if (type == LE_BOND_CLEAR)
    {
        privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_CLEAR, GAP_IDENT_ADDR_PUBLIC,
                                           NULL);
        privacy_handle_pending_resolving_list();
    }
    else if (type == LE_BOND_ADD)
    {
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
    else if (type == LE_BOND_DELETE)
    {
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
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
T_APP_RESULT app_credit_based_conn_callback(uint8_t cbc_type, void *p_cbc_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CBC_DATA cb_data;
    memcpy(&cb_data, p_cbc_data, sizeof(T_LE_CBC_DATA));
    APP_PRINT_TRACE1("app_credit_based_conn_callback: cbc_type = %d", cbc_type);
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
            le_cbc_get_chann_param(CBC_CHANN_PARAM_MTU, &mtu, cb_data.p_le_chann_state->cid);
            APP_PRINT_INFO1("GAP_CHANN_STATE_CONNECTED: mtu %d", mtu);
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
        break;

    case GAP_CBC_MSG_LE_RECEIVE_DATA:
        APP_PRINT_INFO3("GAP_CBC_MSG_LE_RECEIVE_DATA: conn_id %d, cid 0x%x, value_len %d",
                        cb_data.p_le_receive_data->conn_id,
                        cb_data.p_le_receive_data->cid,
                        cb_data.p_le_receive_data->value_len);
        break;

    default:
        break;
    }
    return result;
}
#endif

