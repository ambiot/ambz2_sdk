/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      central_client_app.c
   * @brief     This file handles BLE central application routines.
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
#include "platform_opts_bt.h"
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT) && CONFIG_BT_OTA_CENTRAL_CLIENT
#include "platform_stdlib.h"
#include <app_msg.h>
#include <string.h>
#include <trace_app.h>
#include <gap_scan.h>
#include <gap.h>
#include <gap_msg.h>
#include <gap_bond_le.h>
#include <bt_ota_central_client_app.h>
#include <bt_ota_central_client_link_mgr.h>
#include "dfu_client.h"
#include "ota_client.h"
#include "gaps_client.h"
#include "bas_client.h"
#include "data_uart.h"
#include "bt_ota_central_client.h"
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT) && CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT
#include "bt_ota_central_client_at_cmd.h"
#endif
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
#include "gcs_client.h"
#include "insert_write.h"
#endif
/** @defgroup  CENTRAL_CLIENT_APP Central Client Application
    * @brief This file handles BLE central client application routines.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @addtogroup  CENTRAL_CLIIENT_CALLBACK
    * @{
    */
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
T_CLIENT_ID   bt_ota_central_client_gcs_client_id;         /**< general client id */
#endif
T_CLIENT_ID   bt_ota_central_client_gaps_client_id;        /**< Gaps Services client client id */
T_CLIENT_ID   bt_ota_central_client_bas_client_id;         /**< Battery Services client client id*/
T_CLIENT_ID   bt_ota_central_client_ota_client_id;         /**< OTA Services client client id */
T_CLIENT_ID   bt_ota_central_client_dfu_client_id;         /**< DFU Services client client id */
/** @} */ /* End of group CENTRAL_CLIIENT_CALLBACK */

/** @defgroup  CENTRAL_CLIENT_GAP_MSG GAP Message Handler
    * @brief Handle GAP Message
    * @{
    */
T_GAP_DEV_STATE bt_ota_central_client_gap_dev_state = {0, 0, 0, 0, 0};                /**< GAP device state */

bool is_entered_dfu_mode = false;
uint8_t g_conn_bd_addr[6] = {0};  /**< Bluetooth address of remote device. */
T_GAP_REMOTE_ADDR_TYPE g_conn_addr_type = GAP_REMOTE_ADDR_LE_ANONYMOUS;  /**< Address type of remote device. */

uint8_t g_ota_mac_addr[6];
uint32_t g_patch_version = 0;
uint32_t g_patch_ext_version = 0;
uint32_t g_app_version = 0;
T_OTA_DEVICE_INFO g_ota_device_info;

#if (SILENT_OTA == 1)
#if (RCU_SILENT_OTA == 1)
char remote_name[9] = {'B', 'e', 'e', '2', ' ', 'R', 'C', 'U', '\0'};//bee2 rcu device name
#else
char remote_name[11] = {'R', 'e', 'a', 'l', 'T', 'e', 'k', 'D', 'f', 'u', '\0'};
#endif
#else
char remote_name[8] = {'B', 'L', 'E', '_', 'O', 'T', 'A', '\0'};
char dfu_remote_name[7] = {'B', 'e', 'e', 'T', 'g', 't', '\0'};
#endif

/*============================================================================*
 *                              Functions
 *============================================================================*/
void bt_ota_central_client_app_handle_gap_msg(T_IO_MSG  *p_gap_msg);
void bt_ota_central_client_app_discov_services(uint8_t conn_id, bool start);

#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
/* user write request example */
int client_write_data_example(uint8_t conn_id, uint16_t handle)
{
    uint8_t data[3] = {1,2,3};
    int ret = -1;
    uint16_t length = 3;

    if(if_queue_in(0, conn_id, bt_ota_central_client_gcs_client_id, handle, length, data) == 0)
    {
        request_in_process_flag[conn_id] = 1;
        ret = client_attr_write(conn_id, bt_ota_central_client_gcs_client_id, GATT_WRITE_TYPE_REQ, handle, length, data);
        if(ret != 0) //if write fail
        {
            request_in_process_flag[conn_id] = 0;
            printf("(conn_id %d) user write request(handle %x) fail, please check! ret=%d\r\n", conn_id, handle, ret);
        }
    }

    return ret; // return -1 -- add write request info to queue, 0 -- send write request success, others -- send write request fail
}
#endif

/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void bt_ota_central_client_app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {
            bt_ota_central_client_app_handle_gap_msg(&io_msg);
        }
        break;
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT) && CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT
    case IO_MSG_TYPE_AT_CMD:
        {
            uint16_t subtype = io_msg.subtype;
            void *arg = io_msg.u.buf;
            bt_ota_central_client_app_handle_at_cmd(subtype, arg);
        }
        break;
#endif
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
    case IO_MSG_TYPE_QDECODE:
        {
            printf("at cmd send data\r\n");
            uint8_t conn_id = io_msg.subtype;
            uint16_t handle = io_msg.u.param;
            client_write_data_example(conn_id, handle);
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
void bt_ota_central_client_app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO3("bt_ota_central_client_app_handle_dev_state_evt: init state  %d, scan state %d, cause 0x%x",
                    new_state.gap_init_state,
                    new_state.gap_scan_state, cause);
    if (bt_ota_central_client_gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            uint8_t bt_addr[6];
            APP_PRINT_INFO0("GAP stack ready");
            /*stack ready*/
            gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
            data_uart_print("local bd addr: 0x%2x:%2x:%2x:%2x:%2x:%2x\r\n",
                            bt_addr[5],
                            bt_addr[4],
                            bt_addr[3],
                            bt_addr[2],
                            bt_addr[1],
                            bt_addr[0]);
            APP_PRINT_INFO6("bt addr 0x%x:%x:%x:%x:%x:%x",
                            bt_addr[5], bt_addr[4], bt_addr[3],
                            bt_addr[2], bt_addr[1], bt_addr[0]);
            bt_ota_central_client_link_mgr_clear_device_list();
#if (!(defined(CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT) && CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT))
            le_scan_start();
#endif
        }
    }

    if (bt_ota_central_client_gap_dev_state.gap_scan_state != new_state.gap_scan_state)
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

    bt_ota_central_client_gap_dev_state = new_state;
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
void bt_ota_central_client_app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    if (conn_id >= BT_OTA_CENTRAL_CLIENT_APP_MAX_LINKS)
    {
        return;
    }

    APP_PRINT_INFO4("bt_ota_central_client_app_handle_conn_state_evt: conn_id %d, conn_state(%d -> %d), disc_cause 0x%x",
                    conn_id, bt_ota_central_client_app_link_table[conn_id].conn_state, new_state, disc_cause);

    bt_ota_central_client_app_link_table[conn_id].conn_state = new_state;
    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTED:
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
            {
                APP_PRINT_ERROR2("bt_ota_central_client_app_handle_conn_state_evt: connection lost, conn_id %d, cause 0x%x", conn_id,
                                 disc_cause);
            }

            data_uart_print("Disconnect conn_id %d\r\n", conn_id);
            memset(&bt_ota_central_client_app_link_table[conn_id], 0, sizeof(T_APP_LINK));

            /* Clear all key information */
            le_bond_clear_all_keys();

#if (SILENT_OTA == 1)
#if (ENABLE_SILENT_OTA_PRESS_TEST == 1)
            T_GAP_CAUSE status = le_scan_start();
            if (GAP_CAUSE_SUCCESS != status)
            {
                APP_PRINT_INFO1("LE Scan start status: 0x%x", status);
            }
#endif
#else
            T_GAP_CAUSE status = le_scan_start();
            if (GAP_CAUSE_SUCCESS != status)
            {
                APP_PRINT_INFO1("LE Scan start status: 0x%x", status);
            }
#endif
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
            /* if disconnected during ota process, then need queue out and free memory */
            disconnect_and_queue_out(conn_id);
#endif
            dfu_client_disconnect_cb(conn_id);
        }
        break;

	case GAP_CONN_STATE_CONNECTING:
		{
			APP_PRINT_INFO0("Connecting!");
			data_uart_print("Connecting! \r\n");
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
            le_get_conn_addr(conn_id, bt_ota_central_client_app_link_table[conn_id].bd_addr,
                             (void *)&bt_ota_central_client_app_link_table[conn_id].bd_type);
            APP_PRINT_INFO5("GAP_CONN_STATE_CONNECTED:remote_bd %s, remote_addr_type %d, conn_interval 0x%x, conn_latency 0x%x, conn_supervision_timeout 0x%x",
                            TRACE_BDADDR(bt_ota_central_client_app_link_table[conn_id].bd_addr), bt_ota_central_client_app_link_table[conn_id].bd_type,
                            conn_interval, conn_latency, conn_supervision_timeout);
            data_uart_print("GAP_CONN_STATE_CONNECTED: Connected success conn_id %d\r\n", conn_id);

#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
            /* after connected, init flag and queue*/
            connect_and_init(conn_id);
#endif

#if F_BT_LE_5_0_SET_PHY_SUPPORT
			{
			uint8_t tx_phy;
			uint8_t rx_phy;
			le_get_conn_param(GAP_PARAM_CONN_RX_PHY_TYPE, &rx_phy, conn_id);
			le_get_conn_param(GAP_PARAM_CONN_TX_PHY_TYPE, &tx_phy, conn_id);
			APP_PRINT_INFO2("GAP_CONN_STATE_CONNECTED: tx_phy %d, rx_phy %d", tx_phy, rx_phy);
			data_uart_print("GAP_CONN_STATE_CONNECTED: tx_phy %d, rx_phy %d\r\n", tx_phy, rx_phy);
			}
#endif

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
void bt_ota_central_client_app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO2("bt_ota_central_client_app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("bt_ota_central_client_app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
        }
        break;

    case GAP_AUTHEN_STATE_COMPLETE:
        {
            if (cause == GAP_SUCCESS)
            {
                data_uart_print("Pair success\r\n");
                APP_PRINT_INFO0("bt_ota_central_client_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");

            }
            else
            {
                data_uart_print("Pair failed: cause 0x%x\r\n", cause);
                APP_PRINT_INFO0("bt_ota_central_client_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");
            }
        }
        break;

    default:
        {
            APP_PRINT_ERROR1("bt_ota_central_client_app_handle_authen_state_evt: unknown newstate %d", new_state);
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
void bt_ota_central_client_app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("bt_ota_central_client_app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
	data_uart_print("bt_ota_central_client_app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d\r\n", conn_id, mtu_size);
#if (RCU_SILENT_OTA == 1)
	le_bond_pair(conn_id);
#endif
#if (!(defined(CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT) && CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT))
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
	if(if_queue_in(1, conn_id, 0, 0, 0, NULL) == 0)
#endif
	{
		bt_ota_central_client_app_discov_services(conn_id, true);
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
void bt_ota_central_client_app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
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
            APP_PRINT_INFO4("bt_ota_central_client_app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
			data_uart_print("\n\rbt_ota_central_client_app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x \r\n",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            APP_PRINT_ERROR2("bt_ota_central_client_app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x",
                             conn_id, cause);
			data_uart_print("\n\rbt_ota_central_client_app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x\r\n",
                             conn_id, cause);

        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            APP_PRINT_INFO1("bt_ota_central_client_app_handle_conn_param_update_evt update pending: conn_id %d", conn_id);
			data_uart_print("\n\rbt_ota_central_client_app_handle_conn_param_update_evt update pending: conn_id %d\r\n", conn_id);

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
void bt_ota_central_client_app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    uint8_t conn_id;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    APP_PRINT_TRACE1("bt_ota_central_client_app_handle_gap_msg: subtype %d", p_gap_msg->subtype);
    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            bt_ota_central_client_app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                     gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            bt_ota_central_client_app_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                      (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                      gap_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            bt_ota_central_client_app_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                         gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            bt_ota_central_client_app_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
                                             gap_msg.msg_data.gap_conn_param_update.status,
                                             gap_msg.msg_data.gap_conn_param_update.cause);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            bt_ota_central_client_app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
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
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d", conn_id);
            data_uart_print("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d\r\n", conn_id);
            //le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_OOB_INPUT:
        {
#if F_BT_LE_SMP_OOB_SUPPORT
            uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
            conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_OOB_INPUT: conn_id %d", conn_id);
#if F_BT_LE_SMP_OOB_SUPPORT
            le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
            le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
#endif
        }
        break;

    default:
        APP_PRINT_ERROR1("bt_ota_central_client_app_handle_gap_msg: unknown subtype %d", p_gap_msg->subtype);
        break;
    }
}
/** @} */ /* End of group CENTRAL_CLIENT_GAP_MSG */

#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT) && CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT
void bt_ota_central_client_app_parse_scan_info(T_LE_SCAN_INFO *scan_info)
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

            APP_PRINT_TRACE2("ble_central_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d", type,
                             length - 1);
//            BLE_PRINT("ble_central_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d\n\r", type,
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
//                        BLE_PRINT("  AD Data: Unhandled Data = 0x%x\n\r", scan_info->data[pos + i]);

                    }
                }
                break;
            }
        }

        pos += length;
    }
}
#endif

bool filter_scan_info_by_local_name(T_LE_SCAN_INFO *scan_info, uint8_t *p_remote_name)
{
	uint8_t buffer[32];
	uint8_t pos = 0;
	APP_PRINT_INFO0("filter_scan_info_by_local_name");
	data_uart_print("filter_scan_info_by_local_name.\r\n");

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
			case GAP_ADTYPE_LOCAL_NAME_SHORT:
			case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
				{
					buffer[length - 1] = '\0';

					if (memcmp(buffer, p_remote_name, length - 1) == 0)
					{
						APP_PRINT_INFO1("GAP_ADTYPE_LOCAL_NAME_XXX: %s", TRACE_STRING(buffer));
						data_uart_print("GAP_ADTYPE_LOCAL_NAME_XXX: %s.\r\n", buffer);
						return true;
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

bool filter_scan_info_by_mac_addr(T_LE_SCAN_INFO *scan_info, uint8_t *p_mac_addr)
{
    APP_PRINT_INFO0("filter_scan_info_by_mac_addr");
	data_uart_print("filter_scan_info_by_mac_addr.\r\n");
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
            case GAP_ADTYPE_MANUFACTURER_SPECIFIC:
                {
                    if (memcmp(buffer + 2, p_mac_addr, length - 3) == 0)
                    {
                        APP_PRINT_INFO1("GAP_ADTYPE_MANUFACTURER_SPECIFIC: %s", TRACE_BDADDR(buffer + 2));
						data_uart_print("GAP_ADTYPE_MANUFACTURER_SPECIFIC: [%02x:%02x:%02x:%02x:%02x:%02x].\r\n", 
							buffer[7], buffer[6], buffer[5], buffer[4], buffer[3], buffer[2]);
                        return true;
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


/** @defgroup  CENTRAL_CLIENT_GAP_CALLBACK GAP Callback Event Handler
    * @brief Handle GAP callback event
    * @{
    */
/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT bt_ota_central_client_app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;
    char adv_type[20];
    char remote_addr_type[10];

    switch (cb_type)
    {
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

#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT) && CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT
        BLE_PRINT("ADVType\t\t\t| AddrType\t|%s\t\t\t|rssi\n\r","BT_Addr");
        BLE_PRINT("%s\t\t%s\t"BD_ADDR_FMT"\t%d\n\r",adv_type,remote_addr_type,BD_ADDR_ARG(p_data->p_le_scan_info->bd_addr),
                                                p_data->p_le_scan_info->rssi);

        bt_ota_central_client_app_parse_scan_info(p_data->p_le_scan_info);
        break;
#endif

#if (!(defined(CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT) && CONFIG_BT_OTA_CENTRAL_CLIENT_SPLIT))
        /*get first scan info: silent ota and normal ota is same but remote name is different*/
        if (!is_entered_dfu_mode)
        {
            if (filter_scan_info_by_local_name(p_data->p_le_scan_info, (uint8_t *)remote_name))
            {
                APP_PRINT_INFO0("first scaned target!");
                APP_PRINT_INFO1("Found %s", TRACE_STRING(remote_name));
                le_scan_stop();
				ota_client_connect_device(p_data->p_le_scan_info);
                bt_ota_central_client_link_mgr_add_device(p_data->p_le_scan_info->bd_addr, p_data->p_le_scan_info->remote_addr_type);
            }
        }
        else
        {
            //entered dfu mode
            /*
            0:      filter by device addr in adv data and device name in scan response data;
            1:      filter by device addr in adv data;
            others: filter by device name in scan response data.
            */
#if (DFU_MODE_FILTER_SCAN_INFO_STRATEGY == 0)
			APP_PRINT_INFO0("entered dfu mode 0.");
			
            if (filter_scan_info_by_mac_addr(p_data->p_le_scan_info, g_ota_mac_addr))
            {
                APP_PRINT_INFO2("Found BDADDR:%s, remote_addr_type=%d", TRACE_BDADDR(g_ota_mac_addr),
                                p_data->p_le_scan_info->remote_addr_type);
                //record connect bd addr and addr type
                memcpy(g_conn_bd_addr, p_data->p_le_scan_info->bd_addr, 6);
                g_conn_addr_type = p_data->p_le_scan_info->remote_addr_type;
            }

            if (g_conn_addr_type != GAP_REMOTE_ADDR_LE_ANONYMOUS)
            {
                if ((memcmp(p_data->p_le_scan_info->bd_addr, g_conn_bd_addr, 6) == 0) &&
                    g_conn_addr_type == p_data->p_le_scan_info->remote_addr_type)
                {
                    if (filter_scan_info_by_local_name(p_data->p_le_scan_info, (uint8_t *)dfu_remote_name))
                    {
                        APP_PRINT_INFO1("Found %s", TRACE_STRING(dfu_remote_name));
                        APP_PRINT_INFO0("Found OTA device in DFU Mode!");
                        le_scan_stop();
                        dfu_client_connect_device(p_data->p_le_scan_info);
                        bt_ota_central_client_link_mgr_add_device(p_data->p_le_scan_info->bd_addr, p_data->p_le_scan_info->remote_addr_type);
                    }
                }
            }

#elif (DFU_MODE_FILTER_SCAN_INFO_STRATEGY == 1)
			APP_PRINT_INFO0("entered dfu mode 1.");

            if (filter_scan_info_by_mac_addr(p_data->p_le_scan_info, g_ota_mac_addr))
            {
                APP_PRINT_INFO2("Found BDADDR:%s, remote_addr_type=%d", TRACE_BDADDR(g_ota_mac_addr),
                                p_data->p_le_scan_info->remote_addr_type);
                APP_PRINT_INFO0("Found OTA device in DFU Mode!");
                le_scan_stop();
                dfu_client_connect_device(p_data->p_le_scan_info);
                is_entered_dfu_mode = false;
                bt_ota_central_client_link_mgr_add_device(p_data->p_le_scan_info->bd_addr, p_data->p_le_scan_info->remote_addr_type);
            }
#else
			APP_PRINT_INFO0("entered dfu mode others.");
            if (filter_scan_info_by_local_name(p_data->p_le_scan_info, (uint8_t *)dfu_remote_name))
            {
                APP_PRINT_INFO1("Found %s", TRACE_STRING(dfu_remote_name));
                APP_PRINT_INFO0("Found OTA device in DFU Mode!");
                le_scan_stop();
                dfu_client_connect_device(p_data->p_le_scan_info);
                bt_ota_central_client_link_mgr_add_device(p_data->p_le_scan_info->bd_addr, p_data->p_le_scan_info->remote_addr_type);
            }
#endif
        }
        break;
#endif

    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO5("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
		data_uart_print("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        break;

#if F_BT_LE_5_0_SET_PHY_SUPPORT
	case GAP_MSG_LE_PHY_UPDATE_INFO:
		APP_PRINT_INFO4("GAP_MSG_LE_PHY_UPDATE_INFO:conn_id %d, cause 0x%x, rx_phy %d, tx_phy %d",
						p_data->p_le_phy_update_info->conn_id,
						p_data->p_le_phy_update_info->cause,
						p_data->p_le_phy_update_info->rx_phy,
						p_data->p_le_phy_update_info->tx_phy);
		data_uart_print("GAP_MSG_LE_PHY_UPDATE_INFO:conn_id %d, cause 0x%x, rx_phy %d, tx_phy %d\r\n",
						p_data->p_le_phy_update_info->conn_id,
						p_data->p_le_phy_update_info->cause,
						p_data->p_le_phy_update_info->rx_phy,
						p_data->p_le_phy_update_info->tx_phy);
		break;

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
					data_uart_print("GAP_MSG_LE_REMOTE_FEATS_INFO: support 2M\r\n");
				}
				if (remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] & LE_SUPPORT_FEATURES_LE_CODED_PHY_MASK_BIT)
				{
					APP_PRINT_INFO0("GAP_MSG_LE_REMOTE_FEATS_INFO: support CODED");
					data_uart_print("GAP_MSG_LE_REMOTE_FEATS_INFO: support CODED\r\n");
				}
			}
		}
		break;
#endif

	case GAP_MSG_LE_MODIFY_WHITE_LIST:
		APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation  0x%x, cause 0x%x",
					p_data->p_le_modify_white_list_rsp->operation,
					p_data->p_le_modify_white_list_rsp->cause);
		data_uart_print("GAP_MSG_LE_MODIFY_WHITE_LIST: operation  0x%x, cause 0x%x\r\n",
					p_data->p_le_modify_white_list_rsp->operation,
					p_data->p_le_modify_white_list_rsp->cause);
		break;

    case GAP_MSG_LE_SET_HOST_CHANN_CLASSIF:
        APP_PRINT_INFO1("GAP_MSG_LE_SET_HOST_CHANN_CLASSIF: cause 0x%x",
                        p_data->p_le_set_host_chann_classif_rsp->cause);
        break;

    default:
        APP_PRINT_ERROR1("bt_ota_central_client_app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}
/** @} */ /* End of group CENTRAL_CLIENT_GAP_CALLBACK */




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
void bt_ota_central_client_app_discov_services(uint8_t conn_id, bool start)
{
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
    request_in_process_flag[conn_id] = 1;  //protect discovery service process
#endif
    if (bt_ota_central_client_app_link_table[conn_id].conn_state != GAP_CONN_STATE_CONNECTED)
    {
        //DBG_DIRECT("ota_central_client_app_discov_services conn_id %d not connected ", conn_id);
        APP_PRINT_ERROR1("bt_ota_central_client_app_discov_services: conn_id %d not connected ", conn_id);
        data_uart_print("bt_ota_central_client_app_discov_services: conn_id %d not connected \n\r", conn_id);
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
        request_in_process_flag[conn_id] = 0;
#endif
        return;
    }
    if (start)
    {
#if F_BT_GATT_SRV_HANDLE_STORAGE
		bool is_disc = true;
		T_APP_SRVS_HDL_TABLE app_srvs_table;
		if (bt_ota_central_client_app_load_srvs_hdl_table(&app_srvs_table) == 0)
		{
			if ((app_srvs_table.srv_found_flags != 0) &&
				(app_srvs_table.bd_type == bt_ota_central_client_app_link_table[conn_id].bd_type) &&
				(memcmp(app_srvs_table.bd_addr, bt_ota_central_client_app_link_table[conn_id].bd_addr, GAP_BD_ADDR_LEN) == 0))
			{
				APP_PRINT_INFO1("bt_ota_central_client_app_discov_services: load from flash, srv_found_flags 0x%x",
								app_srvs_table.srv_found_flags);
				bt_ota_central_client_app_link_table[conn_id].srv_found_flags = app_srvs_table.srv_found_flags;
				if (app_srvs_table.srv_found_flags & APP_DISCOV_OTA_FLAG)
				{
					ota_client_set_hdl_cache(conn_id, app_srvs_table.ota_hdl_cache,
											 sizeof(uint16_t) * HDL_OTA_CACHE_LEN);
				}
				if (app_srvs_table.srv_found_flags & APP_DISCOV_DFU_FLAG)
				{
					dfu_client_set_hdl_cache(conn_id, app_srvs_table.dfu_hdl_cache,
											 sizeof(uint16_t) * HDL_DFU_CACHE_LEN);
				}
				if (app_srvs_table.srv_found_flags & APP_DISCOV_BAS_FLAG)
				{
					bas_set_hdl_cache(conn_id, app_srvs_table.bas_hdl_cache, sizeof(uint16_t) * HDL_BAS_CACHE_LEN);
				}
				is_disc = false;
			}
		}
		else
		{
			APP_PRINT_ERROR0("bt_ota_central_client_app_load_srvs_hdl_table: failed");
		}

		if (is_disc)
		{
			if (gaps_start_discovery(conn_id) == false)
			{
				APP_PRINT_ERROR1("bt_ota_central_client_app_discov_services: discover gaps failed conn_id %d", conn_id);
			}
		}
#else
        if (gaps_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("bt_ota_central_client_app_discov_services: discover gaps failed conn_id %d", conn_id);
			data_uart_print("bt_ota_central_client_app_discov_services: discover gaps failed conn_id %d\n\r", conn_id);
        }
#endif
        return;
    }

    if ((bt_ota_central_client_app_link_table[conn_id].discovered_flags & APP_DISCOV_BAS_FLAG) == 0)
    {
        if (bas_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("bt_ota_central_client_app_discov_services: discover bas failed conn_id %d", conn_id);
			data_uart_print("bt_ota_central_client_app_discov_services: discover bas failed conn_id %d\n\r", conn_id);
        }
    }
    else if ((bt_ota_central_client_app_link_table[conn_id].discovered_flags & APP_DISCOV_OTA_FLAG) == 0)
    {
        if (ota_client_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("bt_ota_central_client_app_discov_services: discover ota failed conn_id %d", conn_id);
			data_uart_print("bt_ota_central_client_app_discov_services: discover ota failed conn_id %d\n\r", conn_id);
        }
    }
    else if ((bt_ota_central_client_app_link_table[conn_id].discovered_flags & APP_DISCOV_DFU_FLAG) == 0)
    {
        if (dfu_client_start_discovery(conn_id) == false)
        {
            APP_PRINT_ERROR1("bt_ota_central_client_app_discov_services: discover dfu failed conn_id %d", conn_id);
			data_uart_print("bt_ota_central_client_app_discov_services: discover dfu failed conn_id %d\n\r", conn_id);
        }
    }
    else
    {
        APP_PRINT_INFO2("bt_ota_central_client_app_discov_services: discover complete, conn_id %d, srv_found_flags 0x%x",
                        conn_id, bt_ota_central_client_app_link_table[conn_id].srv_found_flags);
		data_uart_print("bt_ota_central_client_app_discov_services: discover complete, conn_id %d, srv_found_flags 0x%x\n\r",
                        conn_id, bt_ota_central_client_app_link_table[conn_id].srv_found_flags);
#if F_BT_GATT_SRV_HANDLE_STORAGE
        if (bt_ota_central_client_app_link_table[conn_id].srv_found_flags != 0)
        {
            T_APP_SRVS_HDL_TABLE app_srvs_table;
            memset(&app_srvs_table, 0, sizeof(T_APP_SRVS_HDL_TABLE));
            app_srvs_table.bd_type = bt_ota_central_client_app_link_table[conn_id].bd_type;
            app_srvs_table.srv_found_flags = bt_ota_central_client_app_link_table[conn_id].srv_found_flags;
            memcpy(app_srvs_table.bd_addr, bt_ota_central_client_app_link_table[conn_id].bd_addr, GAP_BD_ADDR_LEN);
            ota_client_get_hdl_cache(conn_id, app_srvs_table.ota_hdl_cache,
                                     sizeof(uint16_t) * HDL_OTA_CACHE_LEN);
            dfu_client_get_hdl_cache(conn_id, app_srvs_table.dfu_hdl_cache,
                                     sizeof(uint16_t) * HDL_DFU_CACHE_LEN);
            bas_get_hdl_cache(conn_id, app_srvs_table.bas_hdl_cache, sizeof(uint16_t) * HDL_BAS_CACHE_LEN);
            if (bt_ota_central_client_app_save_srvs_hdl_table(&app_srvs_table) != 0)
            {
                APP_PRINT_ERROR0("bt_ota_central_client_app_save_srvs_hdl_table: failed");
            }
        }
#endif		
    }

    return;
}

/**
 * @brief  Callback will be called when data sent from profile client layer.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT bt_ota_central_client_app_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("bt_ota_central_client_app_client_callback: client_id %d, conn_id %d",
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
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
    else if (client_id == bt_ota_central_client_gcs_client_id)
    {
        T_GCS_CLIENT_CB_DATA *p_gcs_cb_data = (T_GCS_CLIENT_CB_DATA *)p_data;
        switch (p_gcs_cb_data->cb_type)
        {
        case GCS_CLIENT_CB_TYPE_WRITE_RESULT:
            APP_PRINT_INFO3("(GCS_CLIENT) WRITE RESULT: cause 0x%x, handle 0x%x, type %d",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);
            data_uart_print("(GCS_CLIENT) WRITE RESULT: cause 0x%x, handle 0x%x, type %d\n\r",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);

            if (p_gcs_cb_data->cb_content.write_result.type == GATT_WRITE_TYPE_REQ)
            {
                request_in_process_flag[conn_id] = 0;
                if_queue_out_and_send(conn_id);
            }
            break;
        default:
            break;
        }
    }
#endif
    else if (client_id == bt_ota_central_client_gaps_client_id)
    {
        T_GAPS_CLIENT_CB_DATA *p_gaps_cb_data = (T_GAPS_CLIENT_CB_DATA *)p_data;
        switch (p_gaps_cb_data->cb_type)
        {
        case GAPS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_gaps_cb_data->cb_content.disc_state)
            {
            case DISC_GAPS_DONE:
                bt_ota_central_client_app_link_table[conn_id].discovered_flags |= APP_DISCOV_GAPS_FLAG;
                bt_ota_central_client_app_link_table[conn_id].srv_found_flags |= APP_DISCOV_GAPS_FLAG;
                bt_ota_central_client_app_discov_services(conn_id, false);
                /* Discovery Simple BLE service procedure successfully done. */
                APP_PRINT_INFO0("bt_ota_central_client_app_client_callback: discover gaps procedure done.");
                break;
            case DISC_GAPS_FAILED:
                bt_ota_central_client_app_link_table[conn_id].discovered_flags |= APP_DISCOV_GAPS_FLAG;
                bt_ota_central_client_app_discov_services(conn_id, false);
                /* Discovery Request failed. */
                APP_PRINT_INFO0("bt_ota_central_client_app_client_callback: discover gaps request failed.");
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
    else if (client_id == bt_ota_central_client_bas_client_id)
    {
        T_BAS_CLIENT_CB_DATA *p_bas_cb_data = (T_BAS_CLIENT_CB_DATA *)p_data;
        switch (p_bas_cb_data->cb_type)
        {
        case BAS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_bas_cb_data->cb_content.disc_state)
            {
            case DISC_BAS_DONE:
                /* Discovery BAS procedure successfully done. */
                bt_ota_central_client_app_link_table[conn_id].discovered_flags |= APP_DISCOV_BAS_FLAG;
                bt_ota_central_client_app_link_table[conn_id].srv_found_flags |= APP_DISCOV_BAS_FLAG;
                bt_ota_central_client_app_discov_services(conn_id, false);
                APP_PRINT_INFO0("bt_ota_central_client_app_client_callback: discover bas procedure done");
                break;
            case DISC_BAS_FAILED:
                /* Discovery Request failed. */
                bt_ota_central_client_app_link_table[conn_id].discovered_flags |= APP_DISCOV_BAS_FLAG;
                bt_ota_central_client_app_discov_services(conn_id, false);
                APP_PRINT_INFO0("bt_ota_central_client_app_client_callback: discover bas procedure failed");
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
                }
                else
                {
                    APP_PRINT_ERROR1("BAS_READ_BATTERY_LEVEL: failed cause 0x%x",
                                     p_bas_cb_data->cb_content.read_result.cause);
                }
                break;
            case BAS_READ_NOTIFY:
                if (p_bas_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("BAS_READ_NOTIFY: notify %d",
                                    p_bas_cb_data->cb_content.read_result.data.notify);
                }
                else
                {
                    APP_PRINT_ERROR1("BAS_READ_NOTIFY: failed cause 0x%x",
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
                break;
            case BAS_WRITE_NOTIFY_DISABLE:
                APP_PRINT_INFO1("BAS_WRITE_NOTIFY_DISABLE: write result 0x%x",
                                p_bas_cb_data->cb_content.write_result.cause);
                break;
            default:
                break;
            }
            break;
        case BAS_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
            APP_PRINT_INFO1("BAS_CLIENT_CB_TYPE_NOTIF_IND_RESULT: battery level %d",
                            p_bas_cb_data->cb_content.notify_data.battery_level);
            break;

        default:
            break;
        }
    }
    else if (client_id == bt_ota_central_client_ota_client_id)
    {
        T_OTA_CLIENT_CB_DATA *p_ota_client_cb_data = (T_OTA_CLIENT_CB_DATA *)p_data;
        uint16_t read_value_size = p_ota_client_cb_data->cb_content.read_result.data.value_size;
        uint8_t *p_read_value = p_ota_client_cb_data->cb_content.read_result.data.p_value;
        switch (p_ota_client_cb_data->cb_type)
        {
        case OTA_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_ota_client_cb_data->cb_content.disc_state)
            {
            case DISC_OTA_DONE:
                /* Discovery Simple BLE service procedure successfully done. */
                bt_ota_central_client_app_link_table[conn_id].discovered_flags |= APP_DISCOV_OTA_FLAG;
                bt_ota_central_client_app_link_table[conn_id].srv_found_flags |= APP_DISCOV_OTA_FLAG;
                APP_PRINT_INFO0("bt_ota_central_client_app_client_callback: discover ota procedure done.");
				
                //start read ota target device info
                ota_client_read_by_handle(conn_id, OTA_READ_DEVICE_MAC);
                //app_discov_services(conn_id, false); //start find dfu service

                break;
            case DISC_OTA_FAILED:
                /* Discovery Request failed. */
                bt_ota_central_client_app_link_table[conn_id].discovered_flags |= APP_DISCOV_OTA_FLAG;
                bt_ota_central_client_app_discov_services(conn_id, false);
                APP_PRINT_INFO0("bt_ota_central_client_app_client_callback: discover ota request failed.");
                break;
            default:
                break;
            }
            break;
        case OTA_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_ota_client_cb_data->cb_content.read_result.type)
            {
            case OTA_READ_DEVICE_MAC:
                if (p_ota_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    if (read_value_size == sizeof(g_ota_mac_addr))
                    {
                        memcpy(g_ota_mac_addr, p_read_value, read_value_size);
                        LE_ARRAY_TO_UINT32(g_patch_version, p_read_value);
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA Target MAC ADDR=%s", TRACE_BDADDR(g_ota_mac_addr));
                        ota_client_read_by_handle(conn_id, OTA_READ_DEVICE_INFO);
                    }
                    else
                    {
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA_READ_DEVICE_MAC invalid read value size=%d",
                                        read_value_size);
                    }
                }
                else
                {
                    APP_PRINT_ERROR1("bt_ota_central_client_app_client_callback: Type OTA_READ_DEVICE_MAC failed cause 0x%x",
                                     p_ota_client_cb_data->cb_content.read_result.cause);
                }
                break;
            case OTA_READ_PATCH_VER:
                if (p_ota_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    if (read_value_size == sizeof(g_patch_version))
                    {
                        LE_ARRAY_TO_UINT32(g_patch_version, p_read_value);
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA Target patch version=0x%x", g_patch_version);
                    }
                    else
                    {
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA_READ_PATCH_VER invalid read value size=%d",
                                        read_value_size);
                    }
                }
                else
                {
                    APP_PRINT_ERROR1("bt_ota_central_client_app_client_callback: Type OTA_READ_PATCH_VER failed cause 0x%x",
                                     p_ota_client_cb_data->cb_content.read_result.cause);
                };
                break;
            case OTA_READ_APP_VER:
                if (p_ota_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    if (read_value_size == sizeof(g_app_version))
                    {
                        LE_ARRAY_TO_UINT32(g_app_version, p_read_value);
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback:  OTA Target app version=0x%x", g_app_version);
                    }
                    else
                    {
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA_READ_APP_VER invalid read value size=%d",
                                        read_value_size);
                    }
                }
                else
                {
                    APP_PRINT_ERROR1("bt_ota_central_client_app_client_callback: Type OTA_READ_APP_VER failed cause 0x%x",
                                     p_ota_client_cb_data->cb_content.read_result.cause);
                };
                break;
            case OTA_READ_PATCH_EXT_VER:
                if (p_ota_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    if (read_value_size == sizeof(g_patch_ext_version))
                    {
                        LE_ARRAY_TO_UINT32(g_patch_ext_version, p_read_value);
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA Target patch ext version=0x%x", g_patch_ext_version);
                    }
                    else
                    {
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA_READ_PATCH_EXT_VER invalid read value size=%d",
                                        read_value_size);
                    }
                }
                else
                {
                    APP_PRINT_ERROR1("bt_ota_central_client_app_client_callback: Type OTA_READ_PATCH_EXT_VER failed cause 0x%x",
                                     p_ota_client_cb_data->cb_content.read_result.cause);
                };
                break;
            case OTA_READ_DEVICE_INFO:
                if (p_ota_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    if (read_value_size == sizeof(g_ota_device_info))
                    {
                        memcpy((uint8_t *)&g_ota_device_info, p_read_value, read_value_size);
                        APP_PRINT_INFO4("bt_ota_central_client_app_client_callback: aesflg=%d, aesmode=%d,buffercheck=%d, maxbuffersize=%d",
                                        g_ota_device_info.mode.aesflg, g_ota_device_info.mode.aesmode,
                                        g_ota_device_info.mode.buffercheck, g_ota_device_info.maxbuffersize);
                        g_dfu_ctx.mode.value = g_ota_device_info.mode.value;
#if (SILENT_OTA == 1)
                        g_dfu_ctx.maxbuffersize = g_ota_device_info.maxbuffersize;
                        bt_ota_central_client_app_discov_services(conn_id, false); //start find dfu service
#else
                        g_dfu_ctx.maxbuffersize = NORMAL_OTA_MAX_BUFFER_SIZE;
                        if (ota_client_write_char(conn_id, OTA_WRITE_CMD))
                        {
                            is_entered_dfu_mode = true;
                            APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: is_entered_dfu_mode=%d", is_entered_dfu_mode);
                        }
#endif
                    }
                    else
                    {
                        APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: OTA_READ_DEVICE_INFO invalid read value size=%d",
                                        read_value_size);
                    }
                }
                else
                {
                    APP_PRINT_ERROR1("bt_ota_central_client_app_client_callback: Type OTA_READ_DEVICE_INFO failed cause 0x%x",
                                     p_ota_client_cb_data->cb_content.read_result.cause);
                };
                break;
            case OTA_READ_IMG_VER:
                if (p_ota_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: g_app_version 0x%x", g_app_version);
                }
                else
                {
                    APP_PRINT_ERROR1("bt_ota_central_client_app_client_callback: Type OTA_READ_IMG_VER failed cause 0x%x",
                                     p_ota_client_cb_data->cb_content.read_result.cause);
                };
                break;

            default:
                break;
            }
            break;
        case OTA_CLIENT_CB_TYPE_WRITE_RESULT:
            switch (p_ota_client_cb_data->cb_content.write_result.type)
            {
            case OTA_WRITE_CMD:
                APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: Type OTA_WRITE_CMD write result 0x%x",
                                p_ota_client_cb_data->cb_content.write_result.cause);

                break;
            case OTA_WRITE_TEST_MODE:
                APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: Type OTA_WRITE_TEST_MODE write result 0x%x",
                                p_ota_client_cb_data->cb_content.write_result.cause);
                break;
            case OTA_WRITE_IMG_COUNTER:
                APP_PRINT_INFO1("bt_ota_central_client_app_client_callback: Type OTA_WRITE_IMG_COUNTER write result 0x%x",
                                p_ota_client_cb_data->cb_content.write_result.cause);
                break;
            default:
                break;
            }
            break;
        case OTA_CLIENT_CB_TYPE_INVALID:
            APP_PRINT_ERROR0("BT_OTA_CENTRAL_CLIENT_CB_TYPE_INVALID!");
        }
    }
    else if (client_id == bt_ota_central_client_dfu_client_id)
    {
    	APP_PRINT_INFO0("bt_ota_central_client_app_client_callback: dfu_client.");
        T_DFU_CB_MSG *pmsg = (T_DFU_CB_MSG *)p_data;
        switch (pmsg->type)
        {
        case DFU_CB_START:
			APP_PRINT_INFO0("bt_ota_central_client_app_client_callback--dfu_client_start.");
            //dfu_client_start();  //just notify app do something
            break;
        case DFU_CB_FAIL:
			APP_PRINT_INFO0("bt_ota_central_client_app_client_callback--dfu_client_fail.");
            //dfu_client_fail();
            break;
        case DFU_CB_END:
			APP_PRINT_INFO0("bt_ota_central_client_app_client_callback--dfu_client_end.");
            //dfu_client_end();
            break;
        default:
            break;
        }
    }

    return result;
}
#endif
/** @} */ /* End of group CENTRAL_CLIENT_APP */

