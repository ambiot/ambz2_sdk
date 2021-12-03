/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      datatrans_peripheral_application.c
* @brief     datatrans application implementation
* @details
* @author    parker
* @date      2017-7-4
* @version   v0.1
* *********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include <trace_app.h>
#include <string.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <profile_server.h>
#include <gap_msg.h>
#include "bt_datatrans_profile.h"
#include <app_msg.h>
#include <bt_datatrans_peripheral_application.h>
#include "bt_datatrans_module_param_config.h"
#include "bt_datatrans_uart.h"
#include <os_task.h>
#include "bt_datatrans_at_cmd.h"
#include "bt_datatrans_multilink_manager.h"
#include "bt_datatrans_at_hci_cmd_process.h"
#include "os_timer.h"
#include "aes_api.h"
#include "gap_callback_le.h"
#include "gap_conn_le.h"
#include "bt_datatrans_app_flags.h"
#include <platform/platform_stdlib.h>

//bool g_switch_into_ota_pending = false;
T_GAP_DEV_STATE bt_datatrans_gap_dev_state = {0, 0, 0, 0, 0};
T_GAP_CONN_STATE bt_datatrans_gap_conn_state = GAP_CONN_STATE_DISCONNECTED;
T_DATATRANS_REMOTE_BDADDR_INFO connect_dev;
extern T_SERVER_ID bt_datatrans_srv_id;
extern uint8_t sendbuffer[255];

void bt_datatrans_app_handle_gap_msg(T_IO_MSG  *p_gap_msg);
void DataTrans_HandleUART_Event(T_IO_MSG io_driver_msg_recv);

void AtCmdSendResponseConnect(uint8_t conn_id);
void AtCmdSendResponseDisconnect(void);

/******************************************************************
 * @fn          app_handle_io_msg
 * @brief      All the application events are pre-handled in this function.
 *                All the IO MSGs are sent to this function, Then the event handling function
 *                shall be called according to the MSG type.
 *
 * @param    io_msg  - bee io msg data
 * @return     void
 */
void bt_datatrans_app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {			
            bt_datatrans_app_handle_gap_msg(&io_msg);
        }
        break;
    case IO_MSG_TYPE_UART:
        {
            DataTrans_HandleUART_Event(io_msg);
        }
        break;
    default:
        break;
    }
}

void DataTrans_HandleUART_Event(T_IO_MSG io_driver_msg_recv)
{
    uint8_t subtype = io_driver_msg_recv.subtype;
    switch (subtype)
    {
    case IO_MSG_UART_RX:
    case IO_MSG_UART_RX_TIMEOUT:
        {
            if (transferConfigInfo.select_io == UART_AT && transferConfigInfo.at_dfu_mode == 0)
            {
                DataTrans_HandleATCMD();
            }
        }
        break;
    case IO_MSG_UART_RX_OVERFLOW:
        break;
    case IO_MSG_UART_RX_EMPTY:
        break;

    }

}

/******************************************************************
 * @fn          peripheral_HandleBtDevStateChangeEvt
 * @brief      All the gaprole_States_t events are pre-handled in this function.
 *                Then the event handling function shall be called according to the newState.
 *
 * @param    newState  - new gap state
 * @return     void
 */
void bt_datatrans_app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO4("bt_datatrans_app_handle_dev_state_evt: init state %d, adv state %d, conn state %d, cause 0x%x",
                    new_state.gap_init_state, new_state.gap_adv_state,
                    new_state.gap_conn_state, cause);
    if (bt_datatrans_gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO0("GAP stack ready");
			printf("[BT Datatrans] GAP stack ready\n\r");
			
            /*stack ready*/
            if (dataTransInfo.device_mode.adv_mode == AUTO_ADV &&
                dataTransInfo.device_mode.role != ROLE_CENTRAL)
            {
                le_adv_start();
            }

        }
    }

    if (bt_datatrans_gap_dev_state.gap_adv_state != new_state.gap_adv_state)
    {
        if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
            {
                APP_PRINT_INFO0("GAP adv stoped: because connection created");
				printf("GAP adv stoped: because connection created\n\r");
            }
            else
            {
                APP_PRINT_INFO0("GAP adv stoped");
				printf("GAP adv stopped\n\r");
                if (transferConfigInfo.adv_param_update)
                {
                    /*****Update adv data, adv interval******/
                    moduleParam_InitAdvAndScanRspData();
                    uint8_t DeviceName[GAP_DEVICE_NAME_LEN] = {0};
                    memcpy(DeviceName, dataTransInfo.devicename_info.device_name, dataTransInfo.devicename_info.length);
                    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, DeviceName);

                    uint16_t adv_int_min = dataTransInfo.adv_interval;
                    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
                    uint16_t adv_int_max = dataTransInfo.adv_interval;
                    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
                    transferConfigInfo.adv_param_update = false;
                }

                if (dataTransInfo.device_mode.adv_mode == AUTO_ADV &&
                    (dataTransInfo.device_mode.role == ROLE_PERIPHERAL ||
                     dataTransInfo.device_mode.role == ROLE_BEACON))
                {
                    le_adv_start();
                }
            }
        }
        else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            APP_PRINT_INFO0("GAP adv start");
			printf("GAP adv start\n\r");
        }
    }

#if CENTRAL_MODE
    if (bt_datatrans_gap_dev_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");
			printf("GAP scan stop\r\n");
            if (transferConfigInfo.stop_scan_then_adv == true)
            {
                le_adv_start();
            }
            if (transferConfigInfo.create_connection)
            {
                if (transferConfigInfo.connect_by_add == false)
                {
                    le_connect(0, DT_DevList[transferConfigInfo.connect_dev_num].bd_addr,
                               (T_GAP_REMOTE_ADDR_TYPE)DT_DevList[transferConfigInfo.connect_dev_num].bd_type,
                               GAP_LOCAL_ADDR_LE_PUBLIC,
                               1000);
                }
                else
                {
                    le_connect(0, transferConfigInfo.connect_dev_add,
                               GAP_REMOTE_ADDR_LE_PUBLIC,
                               GAP_LOCAL_ADDR_LE_PUBLIC,
                               1000);
                }
                os_timer_start(&TimersConnTimeOut);
                transferConfigInfo.create_connection = false;
            }
            else if (dataTransInfo.device_mode.adv_mode == AUTO_ADV &&
                     dataTransInfo.device_mode.role == ROLE_PERIPHERAL)
            {
                le_adv_start();
            }
        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");
			printf("GAP scan start\r\n");
        }
    }
#endif

    bt_datatrans_gap_dev_state = new_state;
}


void bt_datatrans_app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state,
                                            uint16_t disc_cause)
{
    APP_PRINT_INFO3("bt_datatrans_app_handle_conn_state_evt: conn_id = %d old_state = %d new_state = %d",
                    conn_id, bt_datatrans_gap_conn_state, new_state);
	
    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTED:
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
            {
                APP_PRINT_ERROR1("connection lost: cause 0x%x", disc_cause);
				printf("connection lost: cause 0x%x\r\n", disc_cause);
            }
			printf("[BT DATATRANS] BT Disconnected\n\r");
            if (dataTransInfo.device_mode.adv_mode == AUTO_ADV &&
                (dataTransInfo.device_mode.role == ROLE_PERIPHERAL ||
                 dataTransInfo.device_mode.role == ROLE_BEACON))
            {
                 le_adv_start();
            }
#if CENTRAL_MODE
            else if (dataTransInfo.device_mode.role == ROLE_CENTRAL && conn_id < BT_DATATRANS_APP_MAX_LINKS)
            {
                memset(&DataTransLinkTable[conn_id], 0, sizeof(TAppLinkCB));
            }
#endif

			if (transferConfigInfo.select_io == UART_AT)
            {
                AtCmdSendResponseDisconnect();
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        {
			if (transferConfigInfo.select_io == UART_AT)
            {
#if CENTRAL_MODE
                if (dataTransInfo.device_mode.role == ROLE_CENTRAL)
                {
                    os_timer_stop(&TimersConnTimeOut);
                }
#endif
				printf("[BT DATATRANS] BT Connected\n\r"); 
                AtCmdSendResponseConnect(conn_id);//,dataTransInfo.device_mode.role);
                os_timer_start(&TimersConnParamUpdate);
            }

            CON_ID = conn_id;
           
        }
        break;

    default:
        break;
    }

	bt_datatrans_gap_conn_state = new_state;

#if CENTRAL_MODE
    if (dataTransInfo.device_mode.role == ROLE_CENTRAL && conn_id < BT_DATATRANS_APP_MAX_LINKS)
    {
        DataTransLinkTable[conn_id].conn_state = new_state;
    }
#endif

}

/******************************************************************
 * @fn          peripheral_HandleBtGapAuthenStateChangeEvt
 * @brief      All the bonding state change  events are pre-handled in this function.
 *                Then the event handling function shall be called according to the newState.
 *
 * @param    newState  - new bonding state
 * @return     void
 */
void bt_datatrans_app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO1("bt_datatrans_app_handle_authen_state_evt:conn_id %d", conn_id);
	
    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("GAP_AUTHEN_STATE_STARTED");
        }
        break;

    case GAP_AUTHEN_STATE_COMPLETE:
        {
            //APP_PRINT_INFO0("GAP_AUTHEN_STATE_COMPLETE");
            if (cause == 0)
            {
                APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE pair success");
            }
            else
            {
                APP_PRINT_INFO0("GAP_MSG_LE_AUTHEN_STATE_CHANGE pair failed");
                //memset(&g_ble_key_info, 0, sizeof(T_DATATRANS_KEY_INFO));
                le_disconnect(CON_ID);
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

/******************************************************************
 * @fn          peripheral_HandleBtGapConnParaChangeEvt
 * @brief      All the connection parameter update change  events are pre-handled in this function.
 *                Then the event handling function shall be called according to the status.
 *
 * @param    status  - connection parameter result, 0 - success, otherwise fail.
 * @return     void
 */
void bt_datatrans_app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
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
            APP_PRINT_INFO3("GAP_MSG_LE_CONN_STATE_CHANGE update success:conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
                            conn_interval, conn_slave_latency, conn_supervision_timeout);
			printf("GAP_MSG_LE_CONN_STATE_CHANGE update success:conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x\r\n",
                            conn_interval, conn_slave_latency, conn_supervision_timeout);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            APP_PRINT_ERROR1("GAP_MSG_LE_CONN_STATE_CHANGE failed: cause 0x%x", cause);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            APP_PRINT_INFO0("bt_datatrans_app_handle_conn_param_update_evt update pending.");
			printf("bt_datatrans_app_handle_conn_param_update_evt update pending.\r\n");
        }
        break;

    default:
        break;
    }
}

/******************************************************************
 * @fn          peripheral_HandleBtGapMessage
 * @brief      All the bt gap msg  events are pre-handled in this function.
 *                Then the event handling function shall be called according to the subType
 *                of T_IO_MSG.
 *
 * @param    pBeeIoMsg  - pointer to bee io msg
 * @return     void
 */
void bt_datatrans_app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    uint8_t conn_id;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    APP_PRINT_TRACE1("bt_datatrans_app_handle_gap_msg subType = %d", p_gap_msg->subtype);
	printf("\n\rbt_datatrans_app_handle_gap_msg subType = %d\n", p_gap_msg->subtype);
    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            bt_datatrans_app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                                  gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            bt_datatrans_app_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                                   (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                                   gap_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            bt_datatrans_app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
                                                     gap_msg.msg_data.gap_authen_state.new_state,
                                                     gap_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("LE_GAP_MSG_TYPE_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {
            uint32_t display_value = 0;
            conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO1("LE_GAP_MSG_TYPE_BOND_PASSKEY_DISPLAY:passkey %d", display_value);
            AtCmdSendResponsePinDis();
			
            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            uint32_t display_value = 0;
            conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO1("LE_GAP_MSG_TYPE_BOND_USER_CONFIRMATION:passkey %d", display_value);
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
//            uint32_t passkey = 888888;
            conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
//            APP_PRINT_INFO0("LE_GAP_MSG_TYPE_BOND_PASSKEY_INPUT");
//            le_bond_passkey_input_confirm(conn_id, passkey, BTIF_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_OOB_INPUT:
        {
#if F_BT_LE_SMP_OOB_SUPPORT
            uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
            conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
            APP_PRINT_INFO0("LE_GAP_MSG_TYPE_BOND_OOB_INPUT");
#if F_BT_LE_SMP_OOB_SUPPORT
            le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
            le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
#endif
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            bt_datatrans_app_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
                                                   gap_msg.msg_data.gap_conn_param_update.status,
                                                   gap_msg.msg_data.gap_conn_param_update.cause);
        }
        break;
    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            MTU_SIZE = gap_msg.msg_data.gap_conn_mtu_info.mtu_size;

            APP_PRINT_INFO2("GAP_MSG_LE_CONN_MTU_INFO MTU SIZE =  %x,conn id is %x", MTU_SIZE,
                            gap_msg.msg_data.gap_conn_mtu_info.conn_id);
			printf("GAP_MSG_LE_CONN_MTU_INFO MTU SIZE =  %x,conn id is %x\n", MTU_SIZE,
                            gap_msg.msg_data.gap_conn_mtu_info.conn_id); 
#if CENTRAL_MODE
            if (dataTransInfo.device_mode.role == ROLE_CENTRAL)
            {
            	APP_PRINT_INFO0("client discovery primary service!");
            	printf("client discovery primary service!\n"); 
                client_all_primary_srv_discovery(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                                 CLIENT_PROFILE_GENERAL_ID);
            }
#endif
        }
        break;
    default:
        break;
    }
}


T_APP_RESULT bt_datatrans_app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

    switch (cb_type)
    {
#if CENTRAL_MODE
    case GAP_MSG_LE_SCAN_INFO:
        APP_PRINT_TRACE5("GAP_MSG_LE_SCAN_INFO: bd_addr %s, bdtype=%d, event=0x%x, rssi=%d, len=%d",
                         TRACE_BDADDR(p_data->p_le_scan_info->bd_addr),
                         p_data->p_le_scan_info->remote_addr_type,
                         p_data->p_le_scan_info->adv_type,
                         p_data->p_le_scan_info->rssi,
                         p_data->p_le_scan_info->data_len);
        /* User can split interested information by using the function as follow. */

        if (DataTrans_Multilink_FilterScanInfoByUuid(dataTransInfo.uuid_info.uuid, p_data->p_le_scan_info))
        {
            DataTrans_Multilink_AddDeviceInfo(p_data->p_le_scan_info->bd_addr,
                                              p_data->p_le_scan_info->remote_addr_type);
            APP_PRINT_INFO0("found datatrans device");
        }
        break;
#endif

    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO4("  GAP_MSG_LE_CONN_UPDATE_IND: max_interval=0x%x, min_interval=0x%x, Latency=0x%x,timeOut=0x%x",
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        break;

	case GAP_MSG_LE_READ_RSSI:
        APP_PRINT_INFO3("  GAP_MSG_LE_READ_RSSI:conn_id=0x%x cause=0x%x rssi=%d",
                        p_data->p_le_read_rssi_rsp->conn_id,
                        p_data->p_le_read_rssi_rsp->cause,
                        p_data->p_le_read_rssi_rsp->rssi);
        break;

    case GAP_MSG_LE_BOND_MODIFY_INFO:
        APP_PRINT_INFO1("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
                        p_data->p_le_bond_modify_info->type);
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
        break;
	
#if F_BT_LE_5_0_SET_PHY_SUPPORT
    case GAP_MSG_LE_PHY_UPDATE_INFO:
        APP_PRINT_INFO4("GAP_MSG_LE_PHY_UPDATE_INFO:conn_id %d, cause 0x%x, rx_phy %d, tx_phy %d",
                        p_data->p_le_phy_update_info->conn_id,
                        p_data->p_le_phy_update_info->cause,
                        p_data->p_le_phy_update_info->rx_phy,
                        p_data->p_le_phy_update_info->tx_phy);
        if (p_data->p_le_phy_update_info->rx_phy == GAP_PHYS_2M &&
            p_data->p_le_phy_update_info->tx_phy == GAP_PHYS_2M)
        {
            APP_PRINT_INFO0("Set 2M Phy success.");
        }
        else if (p_data->p_le_phy_update_info->rx_phy == GAP_PHYS_1M &&
                 p_data->p_le_phy_update_info->tx_phy == GAP_PHYS_1M)
        {
            APP_PRINT_INFO0("Set 1M Phy success.");
        }
        break;
#endif

#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT		
    case GAP_MSG_LE_SET_DATA_LEN:
        APP_PRINT_INFO2("GAP_MSG_LE_SET_DATA_LEN: conn_id 0x%x, cause 0x%x",
                        p_data->p_le_set_data_len_rsp->conn_id,
                        p_data->p_le_set_data_len_rsp->cause);
        break;

    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id 0x%x, max_tx_octets 0x%x, max_tx_time 0x%x, max_rx_octets 0x%x, max_rx_time 0x%x",
                        p_data->p_le_data_len_change_info->conn_id,
                        p_data->p_le_data_len_change_info->max_tx_octets,
                        p_data->p_le_data_len_change_info->max_tx_time,
                        p_data->p_le_data_len_change_info->max_rx_octets,
                        p_data->p_le_data_len_change_info->max_rx_time);
        break;
#endif

    default:
        APP_PRINT_INFO1("bt_datatrans_app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}

/******************************************************************
 * @fn          app_profile_callback
 * @brief      All the bt profile callbacks are handled in this function.
 *                Then the event handling function shall be called according to the serviceID
 *                of T_IO_MSG.
 *
 * @param    serviceID  -  service id of profile
 * @param    pData  - pointer to callback data
 * @return     void
 */

T_APP_RESULT bt_datatrans_app_profile_callback(T_SERVER_ID service_id, void *p_data)
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
            APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits = %d",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
            if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
                if (IO_Receive.datalen >= MTU_SIZE - 3) //buffer length > MTU size - 3, send immediately
                {
                    if (transferConfigInfo.select_io == UART_AT)
                    {
                        Setsendbuffer(MTU_SIZE - 3);
                        server_send_data(CON_ID, bt_datatrans_srv_id, GATT_UUID_CHAR_DATA_NOTIFY_INDEX, sendbuffer,
                                         MTU_SIZE - 3, GATT_PDU_TYPE_ANY);
                    }
                }
                else if (IO_Receive.datalen > 0)    //buffer length < MTU size - 3
                {
                    if (transferConfigInfo.select_io == UART_AT && transferConfigInfo.uart_idle)
                    {
                        uint16_t len = IO_Receive.datalen;
                        Setsendbuffer(IO_Receive.datalen);
                        server_send_data(CON_ID, bt_datatrans_srv_id, GATT_UUID_CHAR_DATA_NOTIFY_INDEX, sendbuffer, len,
                                         GATT_PDU_TYPE_ANY);
                        transferConfigInfo.uart_idle = false;
                    }
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
    else  if (service_id == bt_datatrans_srv_id)
    {
        T_DTS_CALLBACK_DATA *p_dts_cb_data = (T_DTS_CALLBACK_DATA *)p_data;
        switch (p_dts_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
				//APP_PRINT_INFO1("SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE: read_index %d\n", p_dts_cb_data->msg_index.read_index); 
                if (p_dts_cb_data->msg_index.read_index == DTS_FLOW_READ_PARA)
                {
                    uint8_t feature[2] = {0x22, 0x02};
                    dts_set_flow_info_parameter(DTS_FLOW_INFO, 2, feature); //length max 32 bytes here
                }
            }
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
				APP_PRINT_INFO1("SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE: write_index %d\n",p_dts_cb_data->msg_index.write_index); 
                switch (p_dts_cb_data->msg_index.write_index)
                {
                case DTS_WRITE_CHAR_DATA:
                    if (dataTransInfo.device_mode.bt_flowctrl)
                    {
                        transferConfigInfo.bt_buf_free++;
                    }
                    app_result = HandleBTReceiveData(0, p_dts_cb_data->msg_data.len, p_dts_cb_data->msg_data.data);
					//printf("DTS_DATA_WRITE: value_size %d\r\n", p_dts_cb_data->msg_data.len);//debug
                    break;
                case DTS_WRITE_FLOW_PARA:
                    dataTransInfo.device_mode.bt_flowctrl = p_dts_cb_data->msg_data.data[0];
                    break;
                default:
                    break;
                }
            }
            break;
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_dts_cb_data->msg_index.notification_indification_index)
                {
                case DTS_DATA_NOTIFY_ENABLE:
                    {
                        APP_PRINT_INFO0("DTS_DATA_NOTIFY_ENABLE");
                    }
                    break;
                case DTS_DATA_NOTIFY_DISABLE:
                    {
                        APP_PRINT_INFO0("DTS_DATA_NOTIFY_DISABLE");
                    }
                    break;
                case DTS_FLOW_NOTIFY_ENABLE:
                    {
                        APP_PRINT_INFO0("DTS_FLOW_NOTIFY_ENABLE");
                        uint8_t info[11] = {0x02, 0x09, 0x17, 0x00, 0x22, 0x01, 0x01};
                        info[2] = MTU_SIZE;
                        info[3] = MTU_SIZE >> 8;
                        info[6] = dataTransInfo.device_mode.bt_flowctrl;
                        info[7] = transferConfigInfo.baudrate;
                        info[8] = (transferConfigInfo.baudrate) >> 8;
                        info[9] = (transferConfigInfo.baudrate) >> 16;
                        info[10] = (transferConfigInfo.baudrate) >> 24;
                        server_send_data(CON_ID, bt_datatrans_srv_id, GATT_UUID_CHAR_FLOW_NOTIFY_INDEX,
                                         info, 11, GATT_PDU_TYPE_ANY);
                    }
                    break;

                case DTS_FLOW_NOTIFY_DISABLE:
                    {
                        APP_PRINT_INFO0("DTS_FLOW_NOTIFY_DISABLE");
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
    return app_result;
}

#endif
