/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      scatternet_app.c
   * @brief     This file handles BLE scatternet application routines.
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
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_FUZZ_TEST) && CONFIG_BT_FUZZ_TEST
#include <string.h>
#include <app_msg.h>
#include <trace_app.h>
#include <gap_scan.h>
#include <gap.h>
#include <gap_msg.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <bt_fuzz_test_app.h>
#include <bt_fuzz_test_link_mgr.h>
#include <bt_fuzz_test_user_cmd.h>
#include <user_cmd_parse.h>
#include <simple_ble_client.h>
#include <gaps_client.h>
#include <bas_client.h>
#include <profile_server.h>
#include <bt_fuzz_test_simple_ble_service.h>
#include <bas.h>
#include <bt_fuzz_test_app_flags.h>
#include <gap_callback_le.h>
#include <os_sched.h>
#include <gls.h>
#include <ias.h>
#include <hrs.h>
#if BT_FUZZ_TEST_APP_GENERAL_CLIENT_TEST
#include <gcs_client.h>
#endif
#include <bt_fuzz_test_at_cmd.h>
#include <platform/platform_stdlib.h>

/** @defgroup  SCATTERNET_APP Scatternet Application
    * @brief This file handles BLE scatternet application routines.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @addtogroup  SCATTERNET_CLIIENT_CALLBACK
    * @{
    */
T_CLIENT_ID   bt_fuzz_test_gaps_client_id;        /**< gap service client id*/
T_CLIENT_ID   bt_fuzz_test_bas_client_id;         /**< battery service client id*/
#if BT_FUZZ_TEST_APP_GENERAL_CLIENT_TEST
T_CLIENT_ID   bt_fuzz_test_gcs_client_id;         /**< general client id*/
#endif

/** @} */ /* End of group SCATTERNET_CLIIENT_CALLBACK */

/** @addtogroup  SCATTERNET_SEVER_CALLBACK
    * @{
    */
T_SERVER_ID bt_fuzz_test_simp_srv_id; /**< Simple ble service id*/
T_SERVER_ID bt_fuzz_test_bas_srv_id;  /**< Battery service id */
T_SERVER_ID bt_fuzz_test_gls_srv_id; /**< Glucose service id*/
T_SERVER_ID bt_fuzz_test_ias_srv_id;  /**< Immediate alert level service id */
T_SERVER_ID bt_fuzz_test_hrs_srv_id; /**< Heart rate service id*/
T_SERVER_ID bt_fuzz_test_dis_srv_id; /**< Device information service id*/
/** @} */ /* End of group SCATTERNET_SEVER_CALLBACK */

T_FUZZ_TESTSUITE testsuite;

TGATTDBdAddr g_cur_rembd;
bool m_start_pair = false;

uint16_t read_handle;
uint16_t write_handle;
uint16_t notify_handle = 0x21;   //characteristic UUID-16:0x2a36
uint16_t indicate_handle = 0x1e;   ////characteristic UUID-16:0x2a35



/** @defgroup  SCATTERNET_GAP_MSG GAP Message Handler
    * @brief Handle GAP Message
    * @{
    */
T_GAP_DEV_STATE bt_fuzz_test_gap_dev_state = {0, 0, 0, 0};                 /**< GAP device state */
/*============================================================================*
 *                              Functions
 *============================================================================*/
void bt_fuzz_test_app_discov_services(uint8_t conn_id, bool start);
void bt_fuzz_test_app_handle_gap_msg(T_IO_MSG  *p_gap_msg);
/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void bt_fuzz_test_app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;
    uint8_t rx_char;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {
            bt_fuzz_test_app_handle_gap_msg(&io_msg);
        }
        break;
#if	defined (CONFIG_BT_USER_COMMAND) && (CONFIG_BT_USER_COMMAND)
    case IO_MSG_TYPE_UART:
        /* We handle user command informations from Data UART in this branch. */
        rx_char = (uint8_t)io_msg.subtype;
        user_cmd_collect(&bt_fuzz_test_user_cmd_if, &rx_char, sizeof(rx_char), bt_fuzz_test_user_cmd_table);
        break;
#endif
    case IO_MSG_TYPE_AT_CMD:
    {
        uint16_t subtype = io_msg.subtype;
        void *arg = io_msg.u.buf;
        bt_fuzz_test_app_handle_at_cmd(subtype, arg);
    }
    break;
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
void bt_fuzz_test_app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO4("bt_fuzz_test_app_handle_dev_state_evt: init state  %d, adv state %d, scan state %d, cause 0x%x",
                    new_state.gap_init_state, new_state.gap_adv_state,
                    new_state.gap_scan_state, cause);
    if (bt_fuzz_test_gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            uint8_t bt_addr[6];
            APP_PRINT_INFO0("GAP stack ready");
            /*stack ready*/
            gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
            data_uart_print("local bd addr: 0x%2x:%2x:%2x:%2x:%2x:%2x\r\n",
                            bt_addr[5], bt_addr[4], bt_addr[3],
                            bt_addr[2], bt_addr[1], bt_addr[0]);
        }
    }

    if (bt_fuzz_test_gap_dev_state.gap_scan_state != new_state.gap_scan_state)
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
        }
    }

    if (bt_fuzz_test_gap_dev_state.gap_adv_state != new_state.gap_adv_state)
    {
        if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
            {
                APP_PRINT_INFO0("GAP adv stoped: because connection created");
				data_uart_print("GAP adv stoped: because connection created\r\n");
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
        }
    }

    bt_fuzz_test_gap_dev_state = new_state;
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_STATE_CHANGE
 * @note     All the gap conn state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New gap connection state
 * @param[in] disc_cause Use this cause when new_state is GAP_CONN_STATE_DISCONNECTED
 * @return   void
 */
void bt_fuzz_test_app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    if (conn_id >= BT_FUZZ_TEST_APP_MAX_LINKS)
    {
        return;
    }

    APP_PRINT_INFO4("bt_fuzz_test_app_handle_conn_state_evt: conn_id %d, conn_state(%d -> %d), disc_cause 0x%x",
                    conn_id, bt_fuzz_test_app_link_table[conn_id].conn_state, new_state, disc_cause);

    bt_fuzz_test_app_link_table[conn_id].conn_state = new_state;
    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTED:
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
            {
                APP_PRINT_ERROR2("bt_fuzz_test_app_handle_conn_state_evt: connection lost, conn_id %d, cause 0x%x", conn_id,
                                 disc_cause);
				//data_uart_print("bt_fuzz_test_app_handle_conn_state_evt: connection lost, conn_id %d, cause 0x%x\r\n", conn_id,
                                 //disc_cause);
            }
			
            switch (testsuite)
            {
            case FUZZ_TESTUITE_BTLE_SMP_510:
            case FUZZ_TESTUITE_BTLE_ATT_510:
            case FUZZ_TESTUITE_BTLE_PROFILES_510:
                le_adv_start();
                break;
            /* Ignore coverity issue of CID 14020. */
            case FUZZ_TESTUITE_BTLE_SMPC_510:
                if (m_start_pair)
                {
                    le_bond_clear_all_keys();
                }
            case FUZZ_TESTUITE_BTLE_ATTC_510:
            case FUZZ_TESTUITE_BTLE_HOGP_510:
            case FUZZ_TESTUITE_BTLE_HEALTH_510:
				os_delay(300);
                le_connect(GAP_PHYS_CONN_INIT_1M_BIT,
                           g_cur_rembd,
                           GAP_REMOTE_ADDR_LE_PUBLIC,
                           GAP_LOCAL_ADDR_LE_PUBLIC,
                           1000);
                break;

            default:
                break;
            }

            data_uart_print("Disconnect conn_id %d\r\n", conn_id);
            memset(&bt_fuzz_test_app_link_table[conn_id], 0, sizeof(T_APP_LINK));
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        {
            uint16_t conn_interval;
            uint16_t conn_latency;
            uint16_t conn_supervision_timeout;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
            le_get_conn_addr(conn_id, bt_fuzz_test_app_link_table[conn_id].bd_addr,
                             &bt_fuzz_test_app_link_table[conn_id].bd_type);
            APP_PRINT_INFO5("GAP_CONN_STATE_CONNECTED:remote_bd %s, remote_addr_type %d, conn_interval 0x%x, conn_latency 0x%x, conn_supervision_timeout 0x%x",
                            TRACE_BDADDR(bt_fuzz_test_app_link_table[conn_id].bd_addr), bt_fuzz_test_app_link_table[conn_id].bd_type,
                            conn_interval, conn_latency, conn_supervision_timeout);
            data_uart_print("Connected success conn_id %d\r\n", conn_id);
            switch (testsuite)
            {
            case FUZZ_TESTUITE_BTLE_SMPC_510:
                if (m_start_pair)
                {
                    le_bond_pair(conn_id);
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

/**
 * @brief    Handle msg GAP_MSG_LE_AUTHEN_STATE_CHANGE
 * @note     All the gap authentication state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New authentication state
 * @param[in] cause Use this cause when new_state is GAP_AUTHEN_STATE_COMPLETE
 * @return   void
 */
void bt_fuzz_test_app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO2("bt_fuzz_test_app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("bt_fuzz_test_app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
        }
        break;

    case GAP_AUTHEN_STATE_COMPLETE:
        {
            if (cause == GAP_SUCCESS)
            {
                data_uart_print("Pair success\r\n");
                APP_PRINT_INFO0("bt_fuzz_test_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");
            }else
            {
                data_uart_print("Pair failed: cause 0x%x\r\n", cause);
                APP_PRINT_INFO0("bt_fuzz_test_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");

            }
        }
        break;

    default:
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_handle_authen_state_evt: unknown newstate %d", new_state);
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
void bt_fuzz_test_app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("bt_fuzz_test_app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
	data_uart_print("bt_fuzz_test_app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
#if BT_FUZZ_TEST_APP_GENERAL_CLIENT_TEST
    switch (testsuite)
    {
    case FUZZ_TESTUITE_BTLE_ATTC_510:
        bt_fuzz_test_app_discov_services(conn_id, true);
        break;

    default:
        break;
    }
#endif
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note     All the connection parameter update change  events are pre-handled in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause Use this cause when status is GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 * @return   void
 */
void bt_fuzz_test_app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
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
            APP_PRINT_INFO4("bt_fuzz_test_app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
			data_uart_print("bt_fuzz_test_app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x\r\n",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            APP_PRINT_ERROR2("bt_fuzz_test_app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x",
                             conn_id, cause);
			data_uart_print("bt_fuzz_test_app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x",
                             conn_id, cause);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            APP_PRINT_INFO1("bt_fuzz_test_app_handle_conn_param_update_evt update pending: conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_handle_conn_param_update_evt update pending: conn_id %d", conn_id);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief    All the BT GAP MSG are pre-handled in this function.
 * @note     Then the event handling function shall be called according to the
 *           subtype of T_IO_MSG
 * @param[in] p_gap_msg Pointer to GAP msg
 * @return   void
 */
void bt_fuzz_test_app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    uint8_t conn_id;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    APP_PRINT_TRACE1("bt_fuzz_test_app_handle_gap_msg: subtype %d", p_gap_msg->subtype);
    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            bt_fuzz_test_app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                     gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            bt_fuzz_test_app_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                      (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                      gap_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            bt_fuzz_test_app_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                         gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            bt_fuzz_test_app_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
                                             gap_msg.msg_data.gap_conn_param_update.status,
                                             gap_msg.msg_data.gap_conn_param_update.cause);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            bt_fuzz_test_app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
                                        gap_msg.msg_data.gap_authen_state.new_state,
                                        gap_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_JUST_WORK");
			data_uart_print("GAP_MSG_LE_BOND_JUST_WORK\r\n");
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:  //passkey entry
        {
            uint32_t display_value = 000000;
            conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
            //le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn_id %d, passkey %d",
                            conn_id, display_value);
            data_uart_print("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn_id %d, passkey %d\r\n",
                            conn_id,
                            display_value);
			le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION: //numeric comperison
        {
            uint32_t display_value = 000000;
            conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
            //le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_USER_CONFIRMATION: conn_id %d, passkey %d",
                            conn_id, display_value);
            data_uart_print("GAP_MSG_LE_BOND_USER_CONFIRMATION: conn_id %d, passkey %d\r\n",
                            conn_id,
                            display_value);
            le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
            uint32_t passkey = 000000;
            conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d, key_press %d",
                            conn_id, gap_msg.msg_data.gap_bond_passkey_input.key_press);
            data_uart_print("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d\r\n", conn_id);
            le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
#if F_BT_LE_SMP_OOB_SUPPORT
    case GAP_MSG_LE_BOND_OOB_INPUT:
        {
            uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_OOB_INPUT: conn_id %d", conn_id);
            le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
            le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
#endif
    default:
        APP_PRINT_ERROR1("bt_fuzz_test_app_handle_gap_msg: unknown subtype %d", p_gap_msg->subtype);
        break;
    }
}
/** @} */ /* End of group SCATTERNET_GAP_MSG */

/** @defgroup  SCATTERNET_SCAN_MGR Scan Information manager
    * @brief Scan Information manager
    * @{
    */
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

        if ((length < 1) || (length >= 31))
        {
            return false;
        }

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
/** @} */ /* End of group SCATTERNET_SCAN_MGR */

void bt_fuzz_test_app_parse_scan_info(T_LE_SCAN_INFO *scan_info)
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

            APP_PRINT_TRACE2("bt_fuzz_test_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d", type,
                             length - 1);
//            BLE_PRINT("ble_scatternet_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d\n\r", type,
//                             length - 1);


            switch (type)
            {
            case GAP_ADTYPE_FLAGS:
                {
                    /* (flags & 0x01) -- LE Limited Discoverable Mode */
                    /* (flags & 0x02) -- LE General Discoverable Mode */
                    /* (flags & 0x04) -- BR/EDR Not Supported */
                    /* (flags & 0x08) -- Simultaneous LE and BR/EDR to Same Device Capable (Controller) */
                    /* (flags & 0x10) -- Simultaneous LE and BR/EDR to Same Device Capable (Host) */
                    uint8_t flags = scan_info->data[pos + 1];
                    APP_PRINT_INFO1("GAP_ADTYPE_FLAGS: 0x%x", flags);
					BLE_PRINT("GAP_ADTYPE_FLAGS: 0x%x\n\r", flags);

                }
                break;

            case GAP_ADTYPE_16BIT_MORE:
            case GAP_ADTYPE_16BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_16BIT:
                {
                    uint16_t *p_uuid = (uint16_t *)(buffer);
                    uint8_t i = length - 1;

                    while (i >= 2)
                    {
                        APP_PRINT_INFO1("GAP_ADTYPE_16BIT_XXX: 0x%x", *p_uuid);
                        BLE_PRINT("GAP_ADTYPE_16BIT_XXX: 0x%x\n\r", *p_uuid);
						p_uuid ++;
                        i -= 2;
                    }
                }
                break;

            case GAP_ADTYPE_32BIT_MORE:
            case GAP_ADTYPE_32BIT_COMPLETE:
                {
                    uint32_t *p_uuid = (uint32_t *)(buffer);
                    uint8_t    i     = length - 1;

                    while (i >= 4)
                    {
                        APP_PRINT_INFO1("GAP_ADTYPE_32BIT_XXX: 0x%x", *p_uuid);
                        BLE_PRINT("GAP_ADTYPE_32BIT_XXX: 0x%x\n\r", (unsigned int)*p_uuid);
						p_uuid ++;

                        i -= 4;
                    }
                }
                break;

            case GAP_ADTYPE_128BIT_MORE:
            case GAP_ADTYPE_128BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_128BIT:
                {
                    uint32_t *p_uuid = (uint32_t *)(buffer);
                    APP_PRINT_INFO4("GAP_ADTYPE_128BIT_XXX: 0x%8.8x%8.8x%8.8x%8.8x",
                                    p_uuid[3], p_uuid[2], p_uuid[1], p_uuid[0]);
					BLE_PRINT("GAP_ADTYPE_128BIT_XXX: 0x%8.8x%8.8x%8.8x%8.8x\n\r",
									(unsigned int)p_uuid[3], (unsigned int)p_uuid[2], (unsigned int)p_uuid[1], (unsigned int)p_uuid[0]);

                }
                break;

            case GAP_ADTYPE_LOCAL_NAME_SHORT:
            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                {
                    buffer[length - 1] = '\0';
                    APP_PRINT_INFO1("GAP_ADTYPE_LOCAL_NAME_XXX: %s", TRACE_STRING(buffer));
					BLE_PRINT("GAP_ADTYPE_LOCAL_NAME_XXX: %s\n\r", buffer);

                }
                break;

            case GAP_ADTYPE_POWER_LEVEL:
                {
                    APP_PRINT_INFO1("GAP_ADTYPE_POWER_LEVEL: 0x%x", scan_info->data[pos + 1]);
					BLE_PRINT("GAP_ADTYPE_POWER_LEVEL: 0x%x\n\r", scan_info->data[pos + 1]);

                }
                break;

            case GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE:
                {
                    uint16_t *p_min = (uint16_t *)(buffer);
                    uint16_t *p_max = p_min + 1;
                    APP_PRINT_INFO2("GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE: 0x%x - 0x%x", *p_min,
                                    *p_max);
					BLE_PRINT("GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE: 0x%x - 0x%x\n\r", *p_min,
									*p_max);

                }
                break;

            case GAP_ADTYPE_SERVICE_DATA:
                {
                    uint16_t *p_uuid = (uint16_t *)(buffer);
                    uint8_t data_len = length - 3;

                    APP_PRINT_INFO3("GAP_ADTYPE_SERVICE_DATA: UUID 0x%x, len %d, data %b", *p_uuid,
                                    data_len, TRACE_BINARY(data_len, &buffer[2]));
					BLE_PRINT("GAP_ADTYPE_SERVICE_DATA: UUID 0x%x, len %d\n\r", *p_uuid,
								data_len);

                }
                break;
            case GAP_ADTYPE_APPEARANCE:
                {
                    uint16_t *p_appearance = (uint16_t *)(buffer);
                    APP_PRINT_INFO1("GAP_ADTYPE_APPEARANCE: %d", *p_appearance);
					BLE_PRINT("GAP_ADTYPE_APPEARANCE: %d\n\r", *p_appearance);

                }
                break;

            case GAP_ADTYPE_MANUFACTURER_SPECIFIC:
                {
                    uint8_t data_len = length - 3;
                    uint16_t *p_company_id = (uint16_t *)(buffer);
                    APP_PRINT_INFO3("GAP_ADTYPE_MANUFACTURER_SPECIFIC: company_id 0x%x, len %d, data %b",
                                    *p_company_id, data_len, TRACE_BINARY(data_len, &buffer[2]));
					BLE_PRINT("GAP_ADTYPE_MANUFACTURER_SPECIFIC: company_id 0x%x, len %d\n\r",
									*p_company_id, data_len);

                }
                break;

            default:
                {
                    uint8_t i = 0;

                    for (i = 0; i < (length - 1); i++)
                    {
                        APP_PRINT_INFO1("  AD Data: Unhandled Data = 0x%x", scan_info->data[pos + i]);
//						BLE_PRINT("  AD Data: Unhandled Data = 0x%x\n\r", scan_info->data[pos + i]);

                    }
                }
                break;
            }
        }

        pos += length;
    }
}


/** @defgroup  SCATTERNET_GAP_CALLBACK GAP Callback Event Handler
    * @brief Handle GAP callback event
    * @{
    */
/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT bt_fuzz_test_app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;
	char adv_type[20];
	char remote_addr_type[10];

    switch (cb_type)
    {
    /* common msg*/
    case GAP_MSG_LE_READ_RSSI:
        APP_PRINT_INFO3("GAP_MSG_LE_READ_RSSI:conn_id %d cause 0x%x rssi %d",
                        p_data->p_le_read_rssi_rsp->conn_id,
                        p_data->p_le_read_rssi_rsp->cause,
                        p_data->p_le_read_rssi_rsp->rssi);
		data_uart_print("GAP_MSG_LE_READ_RSSI:conn_id %d cause 0x%x rssi %d\r\n",
                        p_data->p_le_read_rssi_rsp->conn_id,
                        p_data->p_le_read_rssi_rsp->cause,
                        p_data->p_le_read_rssi_rsp->rssi);
        break;
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        APP_PRINT_INFO3("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x",
                        p_data->p_le_data_len_change_info->conn_id,
                        p_data->p_le_data_len_change_info->max_tx_octets,
                        p_data->p_le_data_len_change_info->max_tx_time);
		data_uart_print("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x\r\n",
                        p_data->p_le_data_len_change_info->conn_id,
                        p_data->p_le_data_len_change_info->max_tx_octets,
                        p_data->p_le_data_len_change_info->max_tx_time);
        break;
#endif
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        APP_PRINT_INFO1("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
                        p_data->p_le_bond_modify_info->type);
		data_uart_print("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x\r\n",
                        p_data->p_le_bond_modify_info->type);
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
		APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x\r\n",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
        break;
    /* central reference msg*/
    case GAP_MSG_LE_SCAN_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_SCAN_INFO:adv_type 0x%x, bd_addr %s, remote_addr_type %d, rssi %d, data_len %d",
                        p_data->p_le_scan_info->adv_type,
                        TRACE_BDADDR(p_data->p_le_scan_info->bd_addr),
                        p_data->p_le_scan_info->remote_addr_type,
                        p_data->p_le_scan_info->rssi,
                        p_data->p_le_scan_info->data_len);
        /* If you want to parse the scan info, please reference function ble_central_app_parse_scan_info. */
		sprintf(adv_type,"%s",(p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_UNDIRECTED)? "CON_UNDIRECT":
							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_DIRECTED)? "CON_DIRECT":
							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_SCANNABLE)? "SCANABLE_UNDIRCT":
							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_NON_CONNECTABLE)? "NON_CONNECTABLE":
							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_SCAN_RSP)? "SCAN_RSP":"unknown");
		sprintf(remote_addr_type,"%s",(p_data->p_le_scan_info->remote_addr_type == GAP_REMOTE_ADDR_LE_PUBLIC)? "public":
							   (p_data->p_le_scan_info->remote_addr_type == GAP_REMOTE_ADDR_LE_RANDOM)? "random":"unknown");

		BLE_PRINT("ADVType\t\t\t| AddrType\t|%s\t\t\t|rssi\n\r","BT_Addr");
		BLE_PRINT("%s\t\t%s\t"BD_ADDR_FMT"\t%d\n\r",adv_type,remote_addr_type,BD_ADDR_ARG(p_data->p_le_scan_info->bd_addr),
												p_data->p_le_scan_info->rssi);

        bt_fuzz_test_app_parse_scan_info(p_data->p_le_scan_info);
        break;

    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO5("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
		data_uart_print("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x\r\n",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        break;

    case GAP_MSG_LE_SET_HOST_CHANN_CLASSIF:
        APP_PRINT_INFO1("GAP_MSG_LE_SET_HOST_CHANN_CLASSIF: cause 0x%x",
                        p_data->p_le_set_host_chann_classif_rsp->cause);
		data_uart_print("GAP_MSG_LE_SET_HOST_CHANN_CLASSIF: cause 0x%x\r\n",
                        p_data->p_le_set_host_chann_classif_rsp->cause);
        break;
    /* peripheral reference msg*/
    case GAP_MSG_LE_ADV_UPDATE_PARAM:
        APP_PRINT_INFO1("GAP_MSG_LE_ADV_UPDATE_PARAM: cause 0x%x",
                        p_data->p_le_adv_update_param_rsp->cause);
		data_uart_print("GAP_MSG_LE_ADV_UPDATE_PARAM: cause 0x%x\r\n",
                        p_data->p_le_adv_update_param_rsp->cause);
        break;

//    case GAP_MSG_LE_DISABLE_SLAVE_LATENCY:
//        APP_PRINT_INFO1("GAP_MSG_LE_DISABLE_SLAVE_LATENCY: cause 0x%x",
//                        p_data->p_le_disable_slave_latency_rsp->cause);
//        break;

//    case GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:
//        APP_PRINT_INFO1("GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:cause 0x%x",
//                        p_data->p_le_update_passed_chann_map_rsp->cause);
//        break;
    default:
        APP_PRINT_ERROR1("bt_fuzz_test_app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}
/** @} */ /* End of group SCATTERNET_GAP_CALLBACK */

/** @defgroup  CENTRAL_SRV_DIS GATT Services discovery and storage
    * @brief GATT Services discovery and storage
    * @{
    */
/**
 * @brief  Discovery GATT services
 * @param  conn_id connection ID.
 * @param  start first call. true - first call this function after conncection, false - not first
 * @retval None
 */
void bt_fuzz_test_app_discov_services(uint8_t conn_id, bool start)
{
    if (bt_fuzz_test_app_link_table[conn_id].conn_state != GAP_CONN_STATE_CONNECTED)
    {
        APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: conn_id %d not connected ", conn_id);
		data_uart_print("bt_fuzz_test_app_discov_services: conn_id %d not connected\r\n ", conn_id);
        return;
    }
#if BT_FUZZ_TEST_APP_GENERAL_CLIENT_TEST
    if (start)
    {
    	data_uart_print("\n\rDiscover all primary services\r\n");
        if (gcs_all_primary_srv_discovery(conn_id) == true)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover all primary services failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover all primary services failed conn_id %d\r\n", conn_id);
        }
        return;
    }
    if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_GAPS_FLAG) == 0)
    {
    	data_uart_print("\n\rDiscover gaps service\r\n");
        if (gaps_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover gaps failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover gaps failed conn_id %d\r\n", conn_id);
        }
    }
    else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_BAS_FLAG) == 0)
    {	
    	data_uart_print("\n\rDiscover bas service\r\n");
        if (bas_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover bas failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover bas failed conn_id %d\r\n", conn_id);
        }
    }
    else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_FLAG) == 0)
    {
    	data_uart_print("\n\rDiscover char\r\n");
        if (gcs_all_char_discovery(conn_id, 0x0001, 0xFFFF) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover char failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover char failed conn_id %d\r\n", conn_id);
        }
    }
    else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG1) == 0)
    {
    	data_uart_print("\n\rDiscover decriptor_0x0016.....\r\n");
        if (gcs_all_char_descriptor_discovery(conn_id, 0x0016, 0x0016) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d\r\n", conn_id);
        }
    }
    else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG2) == 0)
    {
    	data_uart_print("\n\rDiscover decriptor_0x001a.....\r\n");
        if (gcs_all_char_descriptor_discovery(conn_id, 0x001a, 0x001a) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d\r\n", conn_id);
        }
    }
	else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG3) == 0)
    {
    	data_uart_print("\n\rDiscover decriptor_0x001e.....\r\n");
        if (gcs_all_char_descriptor_discovery(conn_id, 0x001e, 0x001e) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d\r\n", conn_id);
        }
    }
	else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG4) == 0)
    {
    	data_uart_print("\n\rDiscover decriptor_0x0021.....\r\n");
        if (gcs_all_char_descriptor_discovery(conn_id, 0x0021, 0x0021) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: discover char descriptor failed conn_id %d\r\n", conn_id);
        }
    }
    else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_READ_FLAG) == 0)
    {
    	data_uart_print("\n\rRead.....\r\n");
        if (gcs_attr_read(conn_id, read_handle) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: read failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: read failed conn_id %d\r\n", conn_id);
        }
    }
    else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_WRITE_FLAG) == 0)
    {
    	data_uart_print("\n\rWrite.....\r\n");
        uint8_t data[1] = {0x1};
        if (gcs_attr_write(conn_id, GATT_WRITE_TYPE_REQ, write_handle, 1, data) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: write failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: write failed conn_id %d\r\n", conn_id);
        }
    }
	else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_NOTIFY_FLAG) == 0)
    {
    	data_uart_print("\n\rEnable notification.....\r\n");
        uint8_t data[1] = {0x01};
        if (gcs_attr_write(conn_id, GATT_WRITE_TYPE_REQ, notify_handle, 1, data) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: enable notification failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: enable notification failed conn_id %d\r\n", conn_id);
        }
    }
	else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_INDICATE_FLAG) == 0)
    {
    	data_uart_print("\n\rEnable indication.....\r\n");
        uint8_t data[1] = {0x02};
        if (gcs_attr_write(conn_id, GATT_WRITE_TYPE_REQ, indicate_handle, 1, data) != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bt_fuzz_test_app_discov_services: enable indication failed conn_id %d", conn_id);
			data_uart_print("bt_fuzz_test_app_discov_services: enable indication failed conn_id %d\r\n", conn_id);
        }
    }
    else
    {
        APP_PRINT_INFO2("bt_fuzz_test_app_discov_services: discover complete, conn_id %d, srv_found_flags 0x%x",
                        conn_id, bt_fuzz_test_app_link_table[conn_id].srv_found_flags);
		data_uart_print("bt_fuzz_test_app_discov_services: discover complete, conn_id %d, srv_found_flags 0x%x\r\n",
                        conn_id, bt_fuzz_test_app_link_table[conn_id].srv_found_flags);
    }
#endif
    return;
}
/** @} */ /* End of group CENTRAL_SRV_DIS */


/** @defgroup  SCATTERNET_CLIIENT_CALLBACK Profile Client Callback Event Handler
    * @brief Handle profile client callback event
    * @{
    */

/**
 * @brief  Callback will be called when data sent from profile client layer.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT bt_fuzz_test_app_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("bt_fuzz_test_app_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
    if (client_id == bt_fuzz_test_gaps_client_id)
    {
        T_GAPS_CLIENT_CB_DATA *p_gaps_cb_data = (T_GAPS_CLIENT_CB_DATA *)p_data;
        switch (p_gaps_cb_data->cb_type)
        {
        case GAPS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_gaps_cb_data->cb_content.disc_state)
            {
            case DISC_GAPS_DONE:
                bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_DISCOV_GAPS_FLAG;
                bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_DISCOV_GAPS_FLAG;
                bt_fuzz_test_app_discov_services(conn_id, false);
                /* Discovery Simple BLE service procedure successfully done. */
                APP_PRINT_INFO0("bt_fuzz_test_app_client_callback: discover gaps procedure done.");
				data_uart_print("bt_fuzz_test_app_client_callback: discover gaps procedure done.\r\n");
                break;
            case DISC_GAPS_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("bt_fuzz_test_app_client_callback: discover gaps request failed.");
				data_uart_print("bt_fuzz_test_app_client_callback: discover gaps request failed.\r\n");
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
					data_uart_print("GAPS_READ_DEVICE_NAME: device name %s.\r\n",
                                    TRACE_STRING(p_gaps_cb_data->cb_content.read_result.data.device_name.p_value));
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_DEVICE_NAME: failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
					data_uart_print("GAPS_READ_DEVICE_NAME: failded cause 0x%x\r\n",
                                    p_gaps_cb_data->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_APPEARANCE:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_APPEARANCE: appearance %d",
                                    p_gaps_cb_data->cb_content.read_result.data.appearance);
					data_uart_print("GAPS_READ_APPEARANCE: appearance %d\r\n",
                                    p_gaps_cb_data->cb_content.read_result.data.appearance);
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_APPEARANCE: failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
					data_uart_print("GAPS_READ_APPEARANCE: failded cause 0x%x\r\n",
                                    p_gaps_cb_data->cb_content.read_result.cause);
                }
                break;
            case GAPS_READ_CENTRAL_ADDR_RESOLUTION:
                if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: central_addr_res %d",
                                    p_gaps_cb_data->cb_content.read_result.data.central_addr_res);
					data_uart_print("GAPS_READ_CENTRAL_ADDR_RESOLUTION: central_addr_res %d\r\n",
                                    p_gaps_cb_data->cb_content.read_result.data.central_addr_res);
                }
                else
                {
                    APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: failded cause 0x%x",
                                    p_gaps_cb_data->cb_content.read_result.cause);
					data_uart_print("GAPS_READ_CENTRAL_ADDR_RESOLUTION: failded cause 0x%x\r\n",
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
    else if (client_id == bt_fuzz_test_bas_client_id)
    {
        T_BAS_CLIENT_CB_DATA *p_bas_cb_data = (T_BAS_CLIENT_CB_DATA *)p_data;
        switch (p_bas_cb_data->cb_type)
        {
        case BAS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_bas_cb_data->cb_content.disc_state)
            {
            case DISC_BAS_DONE:
                /* Discovery BAS procedure successfully done. */
                bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_DISCOV_BAS_FLAG;
                bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_DISCOV_BAS_FLAG;
                bt_fuzz_test_app_discov_services(conn_id, false);
                APP_PRINT_INFO0("bt_fuzz_test_app_client_callback: discover bas procedure done");
				data_uart_print("bt_fuzz_test_app_client_callback: discover bas procedure done\r\n");
                break;
            case DISC_BAS_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("bt_fuzz_test_app_client_callback: discover bas procedure failed");
				data_uart_print("bt_fuzz_test_app_client_callback: discover bas procedure failed\r\n");
                break;
            default:
                break;
            }
            break;
        case BAS_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_bas_cb_data->cb_content.read_result.type)
            {
            case BAS_READ_BATTERY_LEVEL:
                if (p_bas_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("BAS_READ_BATTERY_LEVEL: battery level %d",
                                    p_bas_cb_data->cb_content.read_result.data.battery_level);
					data_uart_print("BAS_READ_BATTERY_LEVEL: battery level %d\r\n",
                                    p_bas_cb_data->cb_content.read_result.data.battery_level);
                }
                else
                {
                    APP_PRINT_ERROR1("BAS_READ_BATTERY_LEVEL: failed cause 0x%x",
                                     p_bas_cb_data->cb_content.read_result.cause);
					data_uart_print("BAS_READ_BATTERY_LEVEL: failed cause 0x%x\r\n",
                                     p_bas_cb_data->cb_content.read_result.cause);
                }
                break;
            case BAS_READ_NOTIFY:
                if (p_bas_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("BAS_READ_NOTIFY: notify %d",
                                    p_bas_cb_data->cb_content.read_result.data.notify);
					data_uart_print("BAS_READ_NOTIFY: notify %d\r\n",
                                    p_bas_cb_data->cb_content.read_result.data.notify);
                }
                else
                {
                    APP_PRINT_ERROR1("BAS_READ_NOTIFY: failed cause 0x%x",
                                     p_bas_cb_data->cb_content.read_result.cause);
					data_uart_print("BAS_READ_NOTIFY: failed cause 0x%x\r\n",
                                     p_bas_cb_data->cb_content.read_result.cause);
                };
                break;

            default:
                break;
            }
            break;
        case BAS_CLIENT_CB_TYPE_WRITE_RESULT:
            switch (p_bas_cb_data->cb_content.write_result.type)
            {
            case BAS_WRITE_NOTIFY_ENABLE:
                APP_PRINT_INFO1("BAS_WRITE_NOTIFY_ENABLE: write result 0x%x",
                                p_bas_cb_data->cb_content.write_result.cause);
				data_uart_print("BAS_WRITE_NOTIFY_ENABLE: write result 0x%x\r\n",
                                p_bas_cb_data->cb_content.write_result.cause);
                break;
            case BAS_WRITE_NOTIFY_DISABLE:
                APP_PRINT_INFO1("BAS_WRITE_NOTIFY_DISABLE: write result 0x%x",
                                p_bas_cb_data->cb_content.write_result.cause);
				data_uart_print("BAS_WRITE_NOTIFY_DISABLE: write result 0x%x\r\n",
                                p_bas_cb_data->cb_content.write_result.cause);
                break;
            default:
                break;
            }
            break;
        case BAS_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
            APP_PRINT_INFO1("BAS_CLIENT_CB_TYPE_NOTIF_IND_RESULT: battery level %d",
                            p_bas_cb_data->cb_content.notify_data.battery_level);
			data_uart_print("BAS_CLIENT_CB_TYPE_NOTIF_IND_RESULT: battery level %d\r\n",
                            p_bas_cb_data->cb_content.notify_data.battery_level);
            break;

        default:
            break;
        }
    }

    return result;
}

#if BT_FUZZ_TEST_APP_GENERAL_CLIENT_TEST
void bt_fuzz_test_gcs_handle_discovery_result(uint8_t conn_id, T_GCS_DISCOVERY_RESULT discov_result)
{
    uint16_t i;
    T_GCS_DISCOV_RESULT *p_result_table;
    uint16_t    properties;
    switch (discov_result.discov_type)
    {
    case GCS_ALL_PRIMARY_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_PRIMARY_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
		data_uart_print("conn_id %d, GCS_ALL_PRIMARY_SRV_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_ALL_SRV_UUID16:
                APP_PRINT_INFO4("ALL SRV UUID16[%d]: service range: 0x%x-0x%x, uuid16 0x%x",
                                i, p_result_table->result_data.srv_uuid16_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.end_group_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.uuid16);
				data_uart_print("ALL SRV UUID16[%d]: service range: 0x%x-0x%x, uuid16 0x%x\r\n",
                                i, p_result_table->result_data.srv_uuid16_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.end_group_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.uuid16);
                break;
            case DISC_RESULT_ALL_SRV_UUID128:
                APP_PRINT_INFO4("ALL SRV UUID128[%d]: service range: 0x%x-0x%x, service=<%b>",
                                i, p_result_table->result_data.srv_uuid128_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid128_disc_data.end_group_handle,
                                TRACE_BINARY(16, p_result_table->result_data.srv_uuid128_disc_data.uuid128));
				data_uart_print("ALL SRV UUID128[%d]: service range: 0x%x-0x%x, service="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.srv_uuid128_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid128_disc_data.end_group_handle,
                                UUID_128(p_result_table->result_data.srv_uuid128_disc_data.uuid128));
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                break;
            }
        }
        bt_fuzz_test_app_discov_services(conn_id, false);

        break;

    case GCS_BY_UUID128_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID128_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
		data_uart_print("conn_id %d, GCS_BY_UUID128_SRV_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_SRV_DATA:
                APP_PRINT_INFO3("SRV DATA[%d]: service range: 0x%x-0x%x",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
				data_uart_print("SRV DATA[%d]: service range: 0x%x-0x%x\r\n",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                break;
            }
        }
        break;

    case GCS_BY_UUID_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
		data_uart_print("conn_id %d, GCS_BY_UUID_SRV_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_SRV_DATA:
                APP_PRINT_INFO3("SRV DATA[%d]: service range: 0x%x-0x%x",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
				data_uart_print("SRV DATA[%d]: service range: 0x%x-0x%x\r\n",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                break;
            }
        }
        break;

    case GCS_ALL_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
		data_uart_print("conn_id %d, GCS_ALL_CHAR_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_CHAR_UUID16:
                properties = p_result_table->result_data.char_uuid16_disc_data.properties;
                APP_PRINT_INFO5("CHAR UUID16[%d]: decl_handle 0x%x, properties 0x%x, value_handle 0x%x, uuid16 0x%x",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
				data_uart_print("CHAR UUID16[%d]: decl_handle 0x%x, properties 0x%x, value_handle 0x%x, uuid16 0x%x\r\n",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\r\n",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY);
				data_uart_print("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\r\n",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY);

                break;

            case DISC_RESULT_CHAR_UUID128:
                properties = p_result_table->result_data.char_uuid128_disc_data.properties;
                APP_PRINT_INFO5("CHAR UUID128[%d]:  decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_uuid128_disc_data.uuid128));
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
				data_uart_print("CHAR UUID128[%d]:  decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                UUID_128(p_result_table->result_data.char_uuid128_disc_data.uuid128));
                data_uart_print("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                break;
            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                break;
            }
        }

        bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_FLAG;
        bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_FLAG;
        bt_fuzz_test_app_discov_services(conn_id, false);
        
        break;

    case GCS_BY_UUID_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
		data_uart_print("conn_id %d, GCS_BY_UUID_CHAR_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_BY_UUID16_CHAR:
                properties = p_result_table->result_data.char_uuid16_disc_data.properties;
                APP_PRINT_INFO5("UUID16 CHAR[%d]: Characteristics by uuid16, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid16=<0x%x>",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
			    data_uart_print("UUID16 CHAR[%d]: Characteristics by uuid16, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid16=<0x%x>\r\n",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
				data_uart_print("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\r\n",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                break;
            }
        }
        break;

    case GCS_BY_UUID128_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID128_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
		data_uart_print("conn_id %d, GCS_BY_UUID128_CHAR_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_BY_UUID128_CHAR:
                properties = p_result_table->result_data.char_uuid128_disc_data.properties;
                APP_PRINT_INFO5("UUID128 CHAR[%d]: Characteristics by uuid128, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_uuid128_disc_data.uuid128));
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
			    data_uart_print("UUID128 CHAR[%d]: Characteristics by uuid128, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                UUID_128(p_result_table->result_data.char_uuid128_disc_data.uuid128));
                data_uart_print("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                break;
            }
        }
        break;

    case GCS_ALL_CHAR_DESC_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_CHAR_DESC_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
		data_uart_print("conn_id %d, GCS_ALL_CHAR_DESC_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_CHAR_DESC_UUID16:
                APP_PRINT_INFO3("DESC UUID16[%d]: Descriptors handle=0x%x, uuid16=<0x%x>",
                                i, p_result_table->result_data.char_desc_uuid16_disc_data.handle,
                                p_result_table->result_data.char_desc_uuid16_disc_data.uuid16);
				data_uart_print("DESC UUID16[%d]: Descriptors handle=0x%x, uuid16=<0x%x>\r\n",
                                i, p_result_table->result_data.char_desc_uuid16_disc_data.handle,
                                p_result_table->result_data.char_desc_uuid16_disc_data.uuid16);
                break;
            case DISC_RESULT_CHAR_DESC_UUID128:
                APP_PRINT_INFO3("DESC UUID128[%d]: Descriptors handle=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_desc_uuid128_disc_data.handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_desc_uuid128_disc_data.uuid128));
				data_uart_print("DESC UUID128[%d]: Descriptors handle=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_desc_uuid128_disc_data.handle,
                                UUID_128(p_result_table->result_data.char_desc_uuid128_disc_data.uuid128));
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                break;
            }
        }

        if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG1) == 0)
        {
            bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG1;
            bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG1;
        }
        else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG2) == 0)
        {
            bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG2;
            bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG2;
        }
		else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG3) == 0)
        {
            bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG3;
            bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG3;
        }
		else if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG4) == 0)
        {
            bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG4;
            bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_DISCOV_CHAR_DES_FLAG4;
        }
        bt_fuzz_test_app_discov_services(conn_id, false);
        
        break;

    default:
        APP_PRINT_ERROR2("Invalid disc type: conn_id %d, discov_type %d",
                         conn_id, discov_result.discov_type);
        break;
    }
}
/**
 * @brief  Callback will be called when data sent from profile client layer.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT bt_fuzz_test_gcs_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("bt_fuzz_test_gcs_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
	data_uart_print("bt_fuzz_test_gcs_client_callback: client_id %d, conn_id %d\r\n",
                    client_id, conn_id);
    if (client_id == bt_fuzz_test_gcs_client_id)
    {
        T_GCS_CLIENT_CB_DATA *p_gcs_cb_data = (T_GCS_CLIENT_CB_DATA *)p_data;
        switch (p_gcs_cb_data->cb_type)
        {
        case GCS_CLIENT_CB_TYPE_DISC_RESULT:
            bt_fuzz_test_gcs_handle_discovery_result(conn_id, p_gcs_cb_data->cb_content.discov_result);
            break;
        case GCS_CLIENT_CB_TYPE_READ_RESULT:
            APP_PRINT_INFO3("READ RESULT: cause 0x%x, handle 0x%x,  value_len %d",
                            p_gcs_cb_data->cb_content.read_result.cause,
                            p_gcs_cb_data->cb_content.read_result.handle,
                            p_gcs_cb_data->cb_content.read_result.value_size);
			data_uart_print("READ RESULT: cause 0x%x, handle 0x%x,  value_len %d\r\n",
                            p_gcs_cb_data->cb_content.read_result.cause,
                            p_gcs_cb_data->cb_content.read_result.handle,
                            p_gcs_cb_data->cb_content.read_result.value_size);
            APP_PRINT_INFO1("READ VALUE: %b",
                            TRACE_BINARY(p_gcs_cb_data->cb_content.read_result.value_size,
                                         p_gcs_cb_data->cb_content.read_result.p_value));
			data_uart_print("READ VALUE:\r\n");
				for(int i=0; i< p_gcs_cb_data->cb_content.read_result.value_size; i++)
					data_uart_print("0x%2X", *(p_gcs_cb_data->cb_content.read_result.p_value + i));
				data_uart_print("\n\r");
			if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_READ_FLAG) == 0)
            {
                bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_READ_FLAG;
                bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_READ_FLAG;
            }
            bt_fuzz_test_app_discov_services(conn_id, false);
            break;
        case GCS_CLIENT_CB_TYPE_WRITE_RESULT:
            APP_PRINT_INFO3("WRITE RESULT: cause 0x%x ,handle 0x%x, type %d",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);
			data_uart_print("WRITE RESULT: cause 0x%x ,handle 0x%x, type %d\r\n",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);
			if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_WRITE_FLAG) == 0)
            {
                bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_WRITE_FLAG;
                bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_WRITE_FLAG;
				bt_fuzz_test_app_discov_services(conn_id, false);
			}else if((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_NOTIFY_FLAG) == 0)
            {
                bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_NOTIFY_FLAG;
                bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_NOTIFY_FLAG;
				bt_fuzz_test_app_discov_services(conn_id, false);
            }
            break;
        case GCS_CLIENT_CB_TYPE_NOTIF_IND:
            if (p_gcs_cb_data->cb_content.notif_ind.notify == false)
            {
                APP_PRINT_INFO2("INDICATION: handle 0x%x, value_size %d",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
				data_uart_print("INDICATION: handle 0x%x, value_size %d",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                APP_PRINT_INFO1("INDICATION VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
				data_uart_print("INDICATION VALUE: %b\r\n",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
            }
            else
            {
                APP_PRINT_INFO2("NOTIFICATION: handle 0x%x, value_size %d",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
				data_uart_print("NOTIFICATION: handle 0x%x, value_size %d\r\n",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                APP_PRINT_INFO1("NOTIFICATION VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
				data_uart_print("NOTIFICATION VALUE: %b\r\n",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
            }
			//if ((bt_fuzz_test_app_link_table[conn_id].discovered_flags & BT_FUZZ_TEST_APP_NOTIFY_FLAG) == 0)
            //{
               // bt_fuzz_test_app_link_table[conn_id].discovered_flags |= BT_FUZZ_TEST_APP_NOTIFY_FLAG;
                //bt_fuzz_test_app_link_table[conn_id].srv_found_flags |= BT_FUZZ_TEST_APP_NOTIFY_FLAG;
				//bt_fuzz_test_app_discov_services(conn_id, false);
            //}
            break;
        default:
            break;
        }
    }

    return result;
}
#endif

/** @} */ /* End of group SCATTERNET_CLIENT_CALLBACK */
/** @defgroup  SCATTERNET_SEVER_CALLBACK Profile Server Callback Event Handler
    * @brief Handle profile server callback event
    * @{
    */
/**
    * @brief    All the BT Profile service callback events are handled in this function
    * @note     Then the event handling function shall be called according to the
    *           service_id
    * @param    service_id  Profile service ID
    * @param    p_data      Pointer to callback data
    * @return   T_APP_RESULT, which indicates the function call is successful or not
    * @retval   APP_RESULT_SUCCESS  Function run successfully
    * @retval   others              Function run failed, and return number indicates the reason
    */
T_APP_RESULT bt_fuzz_test_app_profile_callback(T_SERVER_ID service_id, void *p_data)
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
			data_uart_print("PROFILE_EVT_SRV_REG_COMPLETE: result %d\r\n",
                            p_param->event_data.service_reg_result);
            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
			data_uart_print("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d\r\n",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
            if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
				data_uart_print("PROFILE_EVT_SEND_DATA_COMPLETE success\r\n");
            }
            else
            {
                APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
				data_uart_print("PROFILE_EVT_SEND_DATA_COMPLETE failed\r\n");
            }
            break;

        default:
            break;
        }
    }
    else  if (service_id == bt_fuzz_test_simp_srv_id)
    {
        TSIMP_CALLBACK_DATA *p_simp_cb_data = (TSIMP_CALLBACK_DATA *)p_data;
        switch (p_simp_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_simp_cb_data->msg_data.notification_indification_index)
                {
                case BT_FUZZ_TEST_SIMP_NOTIFY_INDICATE_V3_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_ENABLE");
						data_uart_print("SIMP_NOTIFY_INDICATE_V3_ENABLE\r\n");
                    }
                    break;

                case BT_FUZZ_TEST_SIMP_NOTIFY_INDICATE_V3_DISABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_DISABLE");
						data_uart_print("SIMP_NOTIFY_INDICATE_V3_DISABLE\r\n");
                    }
                    break;
                case BT_FUZZ_TEST_SIMP_NOTIFY_INDICATE_V4_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_ENABLE");
						data_uart_print("SIMP_NOTIFY_INDICATE_V4_ENABLE\r\n");
                    }
                    break;
                case BT_FUZZ_TEST_SIMP_NOTIFY_INDICATE_V4_DISABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_DISABLE");
						data_uart_print("SIMP_NOTIFY_INDICATE_V4_DISABLE\r\n");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_simp_cb_data->msg_data.read_value_index == BT_FUZZ_TEST_SIMP_READ_V1)
                {
                    uint8_t value[1] = {0x01};
                    APP_PRINT_INFO0("SIMP_READ_V1");
					data_uart_print("SIMP_READ_V1\r\n");
                    bt_fuzz_test_simp_ble_service_set_parameter(SIMPLE_BLE_SERVICE_PARAM_V1_READ_CHAR_VAL, 1, &value);
                }
            }
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                switch (p_simp_cb_data->msg_data.write.opcode)
                {
                case BT_FUZZ_TEST_SIMP_WRITE_V2:
                    {
                        APP_PRINT_INFO2("SIMP_WRITE_V2: write type %d, len %d", p_simp_cb_data->msg_data.write.write_type,
                                        p_simp_cb_data->msg_data.write.len);
						data_uart_print("SIMP_WRITE_V2: write type %d, len %d\r\n", p_simp_cb_data->msg_data.write.write_type,
                                        p_simp_cb_data->msg_data.write.len);
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
    else if (service_id == bt_fuzz_test_bas_srv_id)
    {
        T_BAS_CALLBACK_DATA *p_bas_cb_data = (T_BAS_CALLBACK_DATA *)p_data;
        switch (p_bas_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_bas_cb_data->msg_data.notification_indification_index)
                {
                case BAS_NOTIFY_BATTERY_LEVEL_ENABLE:
                    {
                        APP_PRINT_INFO0("BAS_NOTIFY_BATTERY_LEVEL_ENABLE");
						data_uart_print("BAS_NOTIFY_BATTERY_LEVEL_ENABLE\r\n");
                    }
                    break;

                case BAS_NOTIFY_BATTERY_LEVEL_DISABLE:
                    {
                        APP_PRINT_INFO0("BAS_NOTIFY_BATTERY_LEVEL_DISABLE");
						data_uart_print("BAS_NOTIFY_BATTERY_LEVEL_DISABLE\r\n");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_bas_cb_data->msg_data.read_value_index == BAS_READ_BATTERY_LEVEL)
                {
                    uint8_t battery_level = 90;
                    APP_PRINT_INFO1("BAS_READ_BATTERY_LEVEL: battery_level %d", battery_level);
					data_uart_print("BAS_READ_BATTERY_LEVEL: battery_level %d\r\n", battery_level);
                    bas_set_parameter(BAS_PARAM_BATTERY_LEVEL, 1, &battery_level);
                }
            }
            break;


        default:
            break;
        }
    }
    else if (service_id == bt_fuzz_test_gls_srv_id)
    {
        T_GLS_CALLBACK_DATA *p_gls_cb_data = (T_GLS_CALLBACK_DATA *)p_data;
        switch (p_gls_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_gls_cb_data->msg_data.notify_indicate_index)
                {
                case GLS_EVT_GLC_MEASUREMENT_NOTIFY_ENABLE:
                    {
                        APP_PRINT_INFO0("GLS_EVT_GLC_MEASUREMENT_NOTIFY_ENABLE");
						data_uart_print("GLS_EVT_GLC_MEASUREMENT_NOTIFY_ENABLE\r\n");
                    }
                    break;

                case GLS_EVT_GLC_MEASUREMENT_NOTIFY_DISABLE:
                    {
                        APP_PRINT_INFO0("GLS_EVT_GLC_MEASUREMENT_NOTIFY_DISABLE");
						data_uart_print("GLS_EVT_GLC_MEASUREMENT_NOTIFY_DISABLE\r\n");
                    }
                    break;
                case GLS_EVT_GLC_MEASUREMENT_CONTEXT_NOTIFY_ENABLE:
                    {
                        APP_PRINT_INFO0("GLS_EVT_GLC_MEASUREMENT_CONTEXT_NOTIFY_ENABLE");
						data_uart_print("GLS_EVT_GLC_MEASUREMENT_CONTEXT_NOTIFY_ENABLE\r\n");
                    }
                    break;

                case GLS_EVT_GLC_MEASUREMENT_CONTEXT_NOTIFY_DISABLE:
                    {
                        APP_PRINT_INFO0("GLS_EVT_GLC_MEASUREMENT_CONTEXT_NOTIFY_DISABLE");
						data_uart_print("GLS_EVT_GLC_MEASUREMENT_CONTEXT_NOTIFY_DISABLE\r\n");
                    }
                    break;
                case GLS_EVT_GLC_RACP_INDICATE_ENABLE:
                    {
                        APP_PRINT_INFO0("GLS_EVT_GLC_RACP_INDICATE_ENABLE");
						data_uart_print("GLS_EVT_GLC_RACP_INDICATE_ENABLE\r\n");
                    }
                    break;

                case GLS_EVT_GLC_RACP_INDICATE_DISABLE:
                    {
                        APP_PRINT_INFO0("GLS_EVT_GLC_RACP_INDICATE_DISABLE");
						data_uart_print("GLS_EVT_GLC_RACP_INDICATE_DISABLE\r\n");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_gls_cb_data->msg_data.read_value_index == GLS_CHAR_GLC_FEATURE_INDEX)
                {
                    uint8_t glc_features = 90;
                    APP_PRINT_INFO1("GLS_PARAM_GLC_FEATURES: glc_features %d", glc_features);
					data_uart_print("GLS_PARAM_GLC_FEATURES: glc_features %d\r\n", glc_features);
                    gls_set_parameter(GLS_PARAM_GLC_FEATURES, 1, &glc_features);
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                APP_PRINT_INFO1("GLS_CHAR_GLC_RACP_INDEX: racp_value %d", p_gls_cb_data->msg_data.write);
				data_uart_print("GLS_CHAR_GLC_RACP_INDEX: racp_value %d\r\n", p_gls_cb_data->msg_data.write);
            }
            break;

        default:
            break;
        }
    }

    else if (service_id == bt_fuzz_test_ias_srv_id)
    {
        T_IAS_CALLBACK_DATA *p_ias_cb_data = (T_IAS_CALLBACK_DATA *)p_data;
        switch (p_ias_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                APP_PRINT_INFO1("GATT_SVC_PXP_IMMEDIATE_AlERT_VALUE_INDEX: alert_level %d",
                                p_ias_cb_data->msg_data.write_alert_level);
				data_uart_print("GATT_SVC_PXP_IMMEDIATE_AlERT_VALUE_INDEX: alert_level %d\r\n",
                                p_ias_cb_data->msg_data.write_alert_level);
            }
            break;

        default:
            break;
        }
    }
    else if (service_id == bt_fuzz_test_hrs_srv_id)
    {
        T_HRS_CALLBACK_DATA *p_hrs_cb_data = (T_HRS_CALLBACK_DATA *)p_data;
        switch (p_hrs_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_hrs_cb_data->msg_data.notification_indification_index)
                {
                case HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_ENABLE:
                    {
                        T_HEART_RATE_MEASUREMENT_VALUE_FLAG flag;
                        uint16_t measurement_value = 267;
                        uint16_t energy_expended = 5;
                        uint16_t rr_interval = 6;
                        flag.heart_rate_value_format_bit = 1;
                        flag.sensor_contact_status_bits = 3;
                        flag.energy_expended_status_bit = 1;
                        flag.rr_interval_bit = 1;
                        flag.rfu = 0;

                        hrs_set_parameter(HRS_HEART_RATE_MEASUREMENT_PARAM_FLAG, 1, &flag);
                        hrs_set_parameter(HRS_HEART_RATE_MEASUREMENT_PARAM_MEASUREMENT_VALUE, 2, &measurement_value);
                        hrs_set_parameter(HRS_HEART_RATE_MEASUREMENT_PARAM_ENERGY_EXPENDED, 2, &energy_expended);
                        hrs_set_parameter(HRS_HEART_RATE_MEASUREMENT_PARAM_RR_INTERVAL, 2, &rr_interval);
                        APP_PRINT_INFO4("HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_ENABLE: flag 0x%x, measurement_value 0x%x, energy_expended 0x%x, rr_interval 0x%x",
                                        flag, measurement_value, energy_expended, rr_interval);
						data_uart_print("HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_ENABLE: flag 0x%x, measurement_value 0x%x, energy_expended 0x%x, rr_interval 0x%x\r\n",
                                        flag, measurement_value, energy_expended, rr_interval);
                    }
                    break;

                case HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_DISABLE:
                    {
                        APP_PRINT_INFO0("HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_DISABLE");
						data_uart_print("HRS_NOTIFY_INDICATE_MEASUREMENT_VALUE_DISABLE\r\n");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_hrs_cb_data->msg_data.read_value_index == HRS_READ_BODY_SENSOR_LOCATION_VALUE)
                {
                    uint8_t hrs_body_sensor_location = 6;

                    hrs_set_parameter(HRS_BODY_SENSOR_LOCATION_PARAM_VALUE, 1, &hrs_body_sensor_location);
                    APP_PRINT_INFO1("HRS_READ_BODY_SENSOR_LOCATION_VALUE: hrs_body_sensor_location %d",
                                    hrs_body_sensor_location);
					 data_uart_print("HRS_READ_BODY_SENSOR_LOCATION_VALUE: hrs_body_sensor_location %d\r\n",
                                    hrs_body_sensor_location);
                }
            }
            break;


        default:
            break;
        }
    }

    return app_result;
}
#endif
/** @} */ /* End of group SCATTERNET_SEVER_CALLBACK */
/** @} */ /* End of group SCATTERNET_APP */

