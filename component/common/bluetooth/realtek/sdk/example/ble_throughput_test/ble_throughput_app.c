#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include <os_mem.h>
#include "gap_scan.h"
#include "gap.h"
#include "gap_adv.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_throughput_app.h"
#include "ble_throughput_link_mgr.h"
#include "ble_throughput_user_cmd.h"
#include "user_cmd_parse.h"
#include <ble_throughput_200_sut.h>
#include "os_sched.h"

#if F_BT_LE_GATT_SERVER_SUPPORT
#include "profile_server.h"
#include "ble_throughput_vendor_tp_service.h"
#endif

#include <ble_throughput_test_case.h>

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
#include "gap_adv.h"
#endif

#if F_BT_LE_GATT_CLIENT_SUPPORT
#include <gaps_client.h>
#include <ble_throughput_vendor_tp_client.h>
#include <simple_ble_client.h>
#endif

#if F_BT_LE_5_0_AE_SCAN_SUPPORT
#include <gap_ext_scan.h>
#endif

#if F_BT_LE_5_0_AE_ADV_SUPPORT
#include <gap_ext_adv.h>
#endif

extern uint8_t gSimpleProfileServiceId;
//extern bool g_206_phy_update;


bool V3NotifyEnable = false;
/* Client ID of Simple BLE Client Module, generated when add this module in main.c. Can't be modified by user. */
/* Device state maintained in application. */
T_GAP_DEV_STATE ble_throughput_gap_dev_state = {0, 0, 0, 0, 0};
#if F_BT_LE_GATT_CLIENT_SUPPORT
extern T_CLIENT_ID   vendor_tp_client_id;
#endif

void ble_throughput_app_handle_gap_msg(T_IO_MSG *pBeeIoMsg);

/**
* @brief  All the application events are pre-handled in this function.
*
* All the IO MSGs are sent to this function.
* Then the event handling function shall be called according to the MSG type.
*
* @param   io_msg  The T_IO_MSG from peripherals or BT stack state machine.
* @return  void
*/
extern int ble_throughput_app_handle_at_cmd(uint16_t subtype, void *arg);
void ble_throughput_app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;
    uint8_t rx_char;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        ble_throughput_app_handle_gap_msg(&io_msg);
        break;
    case IO_MSG_TYPE_UART:
        /* We handle user command informations from Data UART in this branch. */
        rx_char = (uint8_t)io_msg.subtype;
        user_cmd_collect(&user_cmd_if, &rx_char, sizeof(rx_char), user_cmd_table);
        break;
	case IO_MSG_TYPE_AT_CMD:
        {
            uint16_t subtype = io_msg.subtype;
            void *arg = io_msg.u.buf;
            ble_throughput_app_handle_at_cmd(subtype, arg);
        }
        break;
	case IO_MSG_TYPE_QDECODE:
		{
			le_disconnect(0);
			data_uart_print("\r\n[207 SUT][TX]: test complete\r\n");
		}
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
void ble_throughput_app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO4("ble_throughput_app_handle_dev_state_evt: init state %d scan state %d adv state %d conn state %d",
                    new_state.gap_init_state,
                    new_state.gap_scan_state, new_state.gap_adv_state, new_state.gap_conn_state);
    if (ble_throughput_gap_dev_state.gap_init_state != new_state.gap_init_state)
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

    if (ble_throughput_gap_dev_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");
			data_uart_print("GAP scan stop");
        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");
			data_uart_print("GAP scan start");
        }
    }

    if (ble_throughput_gap_dev_state.gap_conn_state != new_state.gap_conn_state)
    {
        /*
            APP_PRINT_INFO2("Conn state: %d -> %d",
                       ble_throughput_gap_dev_state.gap_conn_state,
                       new_state.gap_conn_state);
                       */
    }
	
    ble_throughput_gap_dev_state = new_state;
}

/**
  * @brief  handle messages indicate that GAP connection state has changed.
  * @param  conn_id: connection ID.
  * @param  new_state: new connection state.
  * @param  disc_cause: when new_state=GAP_CONN_STATE_DISCONNECTED, this value is valid.
  * @retval none
  */

extern TC_206_SUT_MGR *p_tc_206_sut_mgr;
extern TC_207_SUT_MGR *p_tc_207_sut_mgr;
extern TTP_PERFER_PARAM g_206_sut_prefer_param;
extern TTP_PERFER_PARAM g_207_sut_prefer_param;
extern T_TP_TEST_PARAM g_207_tp_test_param;

void ble_throughput_app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    if (conn_id >= APP_MAX_LINKS)
    {
        return;
    }
    APP_PRINT_INFO3("ble_throughput_app_handle_conn_state_evt: conn_id %d oldState %d new_state %d",
                    conn_id, ble_throughput_app_link_table[conn_id].conn_state, new_state);

    ble_throughput_app_link_table[conn_id].conn_state = new_state;
    switch (new_state)
    {
    /*device is disconnected.*/
    case GAP_CONN_STATE_DISCONNECTED:
        {
            APP_PRINT_INFO2("ble_throughput_app_handle_conn_state_evt: conn_id %d disc_cause 0x%04x",
                            conn_id, disc_cause);
            if (ble_throughput_app_get_cur_role() == TC_ROLE_DUT)
            {
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
                
                if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
                    ble_throughput_206_link_disconnected(conn_id, disc_cause);
                }
                else if (ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
                	if(disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
                	{
                		g_207_tp_test_param.end_time = os_sys_time_get();
       				 	g_207_tp_test_param.elapsed_time = ble_throughput_os_time_get_elapsed(g_207_tp_test_param.begin_time,
                                                               								g_207_tp_test_param.end_time);
        				g_207_tp_test_param.data_rate =
            			g_207_tp_test_param.count * g_207_tp_test_param.length * 1000 / (g_207_tp_test_param.elapsed_time);
            										
                	}
                    ble_throughput_207_link_disconnected(conn_id, disc_cause);
                }
                else
                {
                    data_uart_print("Disconnect conn_id %d\r\n", conn_id);
                }
#endif
            }
            else if (ble_throughput_app_get_cur_role() == TC_ROLE_SUT)
            {    
                if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
                	p_tc_206_sut_mgr->end_time = os_sys_time_get();
               		p_tc_206_sut_mgr->elapsed_time = ble_throughput_os_time_get_elapsed(p_tc_206_sut_mgr->begin_time,
                                                                     p_tc_206_sut_mgr->end_time);
                	p_tc_206_sut_mgr->data_rate = p_tc_206_sut_mgr->total_notify_rx_count * g_206_sut_prefer_param.length * 1000 /
                    												(p_tc_206_sut_mgr->elapsed_time);
                    ble_throughput_206_sut_link_disconnected(conn_id, disc_cause);
                }
                else if (ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
                	int sut_tx_count = g_207_sut_prefer_param.count - p_tc_207_sut_mgr->count_remain;
                	p_tc_207_sut_mgr->end_time = os_sys_time_get();
                	p_tc_207_sut_mgr->elapsed_time = ble_throughput_os_time_get_elapsed(p_tc_207_sut_mgr->begin_time,
                                                                     p_tc_207_sut_mgr->end_time);
                	p_tc_207_sut_mgr->data_rate = sut_tx_count * g_207_sut_prefer_param.length * 1000 /
                    												(p_tc_207_sut_mgr->elapsed_time);
                    ble_throughput_207_sut_link_disconnected(conn_id, disc_cause);
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
            memset(&ble_throughput_app_link_table[conn_id], 0, sizeof(T_APP_LINK));
        }
        break;
    /*device is disconnected.*/
    case GAP_CONN_STATE_CONNECTING:
        break;

    /*device is connected*/
    case GAP_CONN_STATE_CONNECTED:
        {
            uint16_t mtu_size;
            le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, conn_id);
            //data_uart_print("Connected conn_id = %d, mtu size = %d\r\n", conn_id, mtu_size);
            if (ble_throughput_app_get_cur_role() == TC_ROLE_DUT)
            {
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
                if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                }
                else if (ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                }
                else
                {
                    data_uart_print("Conn conn_id = %d\r\n", conn_id);
                }
#endif
            }
            else if (ble_throughput_app_get_cur_role() == TC_ROLE_SUT)
            {
                if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    ble_throughput_206_sut_link_connected(conn_id);
                }
                else if (ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                {
                    data_uart_print("connected success conn_id %d\r\n", conn_id);
                    ble_throughput_207_sut_link_connected(conn_id);
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
void ble_throughput_app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t status)
{
    APP_PRINT_INFO3("ble_throughput_app_handle_authen_state_evt: conn_id %d, new_state %d, status 0x%x", conn_id,
                    new_state, status);
    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE:(GAP_AUTHEN_STATE_STARTED)");
        }
        break;
    case GAP_AUTHEN_STATE_COMPLETE:
        {
            APP_PRINT_INFO1("GAP_MSG_LE_AUTHEN_STATE_CHANGE:(GAP_AUTHEN_STATE_COMPLETE) status 0x%x",
                            status);
            if (status == 0)
            {
                APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE pair success");
            }
            else
            {
                APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE pair failed");
                data_uart_print("pair failed conn_id = %d\r\n", conn_id);
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
void ble_throughput_app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status)
{
    switch (status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
        {
            if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
            {
                if (ble_throughput_app_get_cur_role() == TC_ROLE_DUT)
                {
                    ble_throughput_206_tp_notification_tx_conn_param_update_event(conn_id);
                }
                else if (ble_throughput_app_get_cur_role() == TC_ROLE_SUT)
                {
                    ble_throughput_206_sut_conn_param_update_event(conn_id);
                }
            }
            else if (ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
            {
                if (ble_throughput_app_get_cur_role() == TC_ROLE_DUT)
                {
                    ble_throughput_207_tp_rx_conn_param_update_event(conn_id);
                }
                else if (ble_throughput_app_get_cur_role() == TC_ROLE_SUT)
                {
                    ble_throughput_207_sut_conn_param_update_event(conn_id);
                }
            }   
        }
        break;
    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_CONN_PARAM_UPDATE failed.");
            data_uart_print("LE_CONN_PARAM_UPDATE failed\r\n");
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

void ble_throughput_app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("ble_throughput_app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
    printf("ble_throughput_app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d\r\n", conn_id, mtu_size);
}
/**
  * @brief  handle messages from GAP layer.
  * @param  pBeeIoMsg: message from GAP layer.
  * @retval none
  */
void ble_throughput_app_handle_gap_msg(T_IO_MSG *p_io_msg)
{
    T_LE_GAP_MSG bt_msg;
    uint8_t conn_id;

    memcpy(&bt_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            ble_throughput_app_handle_dev_state_evt(bt_msg.msg_data.gap_dev_state_change.new_state,
                                     bt_msg.msg_data.gap_dev_state_change.cause);
        }
        break;
    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            ble_throughput_app_handle_conn_state_evt(bt_msg.msg_data.gap_conn_state_change.conn_id,
                                      (T_GAP_CONN_STATE)bt_msg.msg_data.gap_conn_state_change.new_state,
                                      bt_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;
    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            ble_throughput_app_handle_conn_mtu_info_evt(bt_msg.msg_data.gap_conn_mtu_info.conn_id,
                                         bt_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;
    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            ble_throughput_app_handle_authen_state_evt(bt_msg.msg_data.gap_authen_state.conn_id,
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
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_USER_CONFIRMATION: %d", displayValue);
            le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
            conn_id = bt_msg.msg_data.gap_bond_passkey_input.conn_id;
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_PASSKEY_INPUT");
			
            uint32_t passKey = 888888;
            le_bond_passkey_input_confirm(conn_id, passKey, GAP_CFM_CAUSE_ACCEPT);
            
        }
        break;
#if F_BT_LE_SMP_OOB_SUPPORT
    case GAP_MSG_LE_BOND_OOB_INPUT:
        {
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_OOB_INPUT");
            uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
            conn_id = bt_msg.msg_data.gap_bond_oob_input.conn_id;
            le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
#endif

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            ble_throughput_app_handle_conn_param_update_evt(bt_msg.msg_data.gap_conn_param_update.conn_id,
                                             bt_msg.msg_data.gap_conn_param_update.status);
        }
        break;
    default:
        APP_PRINT_ERROR1("ble_throughput_app_handle_gap_msg unknown subtype", p_io_msg->subtype);
        break;
    }

}

/**
  * @brief  This callback will be called when advertising or scan response data received.
  * @param  msg_type: type of the message sent from GAP Central Role layer.
  * @param  cb_data: message sent from GAP Central Role layer.
  * @retval None
  */
T_APP_RESULT ble_throughput_app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_LE_CB_DATA));
    APP_PRINT_INFO1("app_gap_callback: cb_type %d", cb_type);
    switch (cb_type)
    {
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
        
        if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
        {
            if (ble_throughput_app_get_cur_role() == TC_ROLE_DUT)
            {
                ble_throughput_206_tp_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                        cb_data.p_le_phy_update_info->cause,
                                                        cb_data.p_le_phy_update_info->tx_phy,
                                                        cb_data.p_le_phy_update_info->rx_phy);
            }
            else if (ble_throughput_app_get_cur_role() == TC_ROLE_SUT)
            {
                ble_throughput_206_sut_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                         cb_data.p_le_phy_update_info->cause,
                                                         cb_data.p_le_phy_update_info->tx_phy,
                                                         cb_data.p_le_phy_update_info->rx_phy);
            }
        }
        else if (ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
        {
            if (ble_throughput_app_get_cur_role() == TC_ROLE_DUT)
            {
                ble_throughput_207_tp_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                           cb_data.p_le_phy_update_info->cause,
                                           cb_data.p_le_phy_update_info->tx_phy,
                                           cb_data.p_le_phy_update_info->rx_phy);
            }
            else if (ble_throughput_app_get_cur_role() == TC_ROLE_SUT)
            {
                ble_throughput_207_sut_notification_phy_update_event(cb_data.p_le_phy_update_info->conn_id,
                                                         cb_data.p_le_phy_update_info->cause,
                                                         cb_data.p_le_phy_update_info->tx_phy,
                                                         cb_data.p_le_phy_update_info->rx_phy);
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

    default:
        break;
    }
    return result;
}

#if F_BT_LE_GATT_CLIENT_SUPPORT
T_APP_RESULT ble_throughput_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;

    if (client_id == vendor_tp_client_id)
    {
        T_TP_CB_DATA *p_cb = (T_TP_CB_DATA *)p_data;
        ble_throughput_20x_sut_client_result_callback(conn_id, p_cb);
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
T_APP_RESULT ble_throughput_app_profile_callback(T_SERVER_ID serviceID, void *p_data)
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
                if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                {
                    ble_throughput_206_tp_notification_tx_tx_data_complete(pPara->event_data.send_data_result.credits);
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
                    }
                    break;
                case VENDOR_TP_SERVICE_V1_NOTIFICATION_DISABLE:
                    {
                        APP_PRINT_INFO0("VENDOR_TP_SERVICE_V1_NOTIFICATION_DISABLE");
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
                    if (ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
                    {
                        ble_throughput_206_tp_notification_tx_tx_config(p_data);
                    }
                }
                else
                {
                    if (ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02)
                    {
                        ble_throughput_207_tp_handle_write_data(p_data);
                    }
                }
            }
        }
    }

    return appResult;
}
#endif
#endif
