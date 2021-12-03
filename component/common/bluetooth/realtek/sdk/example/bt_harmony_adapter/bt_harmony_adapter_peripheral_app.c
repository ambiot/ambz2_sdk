/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      peripheral_app.c
   * @brief     This file handles BLE peripheral application routines.
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
#if defined(CONFIG_BT_HARMONY_ADAPTER) && CONFIG_BT_HARMONY_ADAPTER
#include "platform_stdlib.h"
#include <trace_app.h>
#include <string.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <profile_server.h>
#include <gap_msg.h>
#include <app_msg.h>
#include "bt_harmony_adapter_peripheral_app.h"
#include "ohos_bt_gatt.h"
#include "ohos_bt_gatt_server.h"
#include "ohos_bt_gatt_client.h"
#include <gap_conn_le.h>
#include "platform_stdlib.h"
#include <os_msg.h>
#include <os_mem.h>
#include <os_timer.h>
#include <os_sync.h>

/*============================================================================*
 *                              Variables
 *============================================================================*/
T_GAP_DEV_STATE bt_harmony_adapter_gap_dev_state = {0, 0, 0, 0, 0};                 /**< GAP device state */
T_GAP_CONN_STATE bt_harmony_adapter_gap_conn_state = GAP_CONN_STATE_DISCONNECTED; /**< GAP connection state */

extern void *bt_harmony_adapter_evt_queue_handle;
extern void *bt_harmony_adapter_io_queue_handle;
extern void *bt_harmony_adapter_callback_queue_handle;
extern BHA_SERVICE_INFO bt_harmony_adapter_srvs_head;

extern H_adv_param h_adv_param;
extern BtGattCallbacks BH_GattCallbacks ;
extern BtGattServerCallbacks BH_GattServerCallbacks;
extern uint8_t H_adv_data[31];
extern uint8_t H_scan_response_data[31];
void *bt_harmony_BleAdv_Timer = NULL;
int RT_duration = 0;

uint8_t  RT_remote_bd[OHOS_BD_ADDR_LEN] = {0};
extern void *send_indication_sem;
/*============================================================================*
 *                              Functions
 *============================================================================*/
bool bt_harmony_adapter_app_send_api_msg(T_BHA_API_MSG_TYPE sub_type, void *buf)
{
	T_IO_MSG io_msg;

	uint8_t event = EVENT_IO_TO_APP;

	io_msg.type = IO_MSG_TYPE_QDECODE;
	io_msg.subtype = sub_type;
	io_msg.u.buf = buf;

	if (bt_harmony_adapter_evt_queue_handle != NULL && bt_harmony_adapter_io_queue_handle != NULL) {
		if (os_msg_send(bt_harmony_adapter_io_queue_handle, &io_msg, 0) == false) {
			printf("[%s] send io queue fail! type = 0x%x\r\n", __FUNCTION__, io_msg.subtype);
			return false;
		} else if (os_msg_send(bt_harmony_adapter_evt_queue_handle, &event, 0) == false) {
			printf("[%s] send event queue fail! type = 0x%x\r\n", __FUNCTION__, io_msg.subtype);
			return false;
		}
	} else {
		printf("[%s] queue is empty! type = 0x%x\r\n", __FUNCTION__, io_msg.subtype);
		return false;
	}

	return true;
}

bool bt_harmony_adapter_app_send_callback_msg(T_BHA_CALLBACK_MSG_TYPE type, void *buf)
{
	T_BHA_CALLBACK_MSG callback_msg;

	callback_msg.type = type;
	callback_msg.buf = buf;

	if (bt_harmony_adapter_callback_queue_handle != NULL) {
		if (os_msg_send(bt_harmony_adapter_callback_queue_handle, &callback_msg, 0) == false) {
			printf("[%s] send callback queue fail! type = 0x%x\r\n", __FUNCTION__, callback_msg.type);
			return false;
		}
	} else {
		printf("[%s] queue is empty! type = 0x%x\r\n", __FUNCTION__, callback_msg.type);
		return false;
	}

	return true;
}
void bt_harmony_adapter_adv_timer_handler(void *timer_hdl)
{
///stop adv
	T_GAP_DEV_STATE new_state;
	le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	if (new_state.gap_adv_state != GAP_ADV_STATE_ADVERTISING) {
		printf("\n\r[%s]:adv already stopped\r\n", __func__);
	} else {
		bt_harmony_adapter_app_send_api_msg(BHA_API_MSG_STOP_ADV, NULL);
	}
}

void bt_harmony_adapter_app_handle_api_msg(T_IO_MSG io_msg)
{
	//Handle API msg here
	uint16_t msg_type = io_msg.subtype;

	switch (msg_type) {
	case BHA_API_MSG_START_ADV: {
		H_adv_param *h_adv_param = io_msg.u.buf;
		le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(h_adv_param->H_adv_evt_type), &h_adv_param->H_adv_evt_type);
		le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(h_adv_param->H_adv_direct_type), &h_adv_param->H_adv_direct_type);
		le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(h_adv_param->H_local_addr_type), &h_adv_param->H_local_addr_type);
		le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(h_adv_param->H_adv_direct_addr), h_adv_param->H_adv_direct_addr);
		le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(h_adv_param->H_adv_chann_map), &h_adv_param->H_adv_chann_map);
		le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(h_adv_param->H_adv_filter_policy), &h_adv_param->H_adv_filter_policy);
		le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(h_adv_param->H_adv_int_min), &h_adv_param->H_adv_int_min);
		le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(h_adv_param->H_adv_int_max), &h_adv_param->H_adv_int_max);
		le_adv_set_param(GAP_PARAM_ADV_DATA, h_adv_param->adv_datalen, (void *)H_adv_data);
		le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, h_adv_param->scanrep_datalen, (void *)H_scan_response_data);

		memset(H_adv_data, 0, 31);
		memset(H_scan_response_data, 0, 31);
		///timer
		if (h_adv_param->H_duration != 0) {
			RT_duration = h_adv_param->H_duration;
			if (bt_harmony_BleAdv_Timer == NULL) {
				if (os_timer_create(&bt_harmony_BleAdv_Timer, "bt_harmony_BleAdv_Timer", 0, RT_duration * 10, false, bt_harmony_adapter_adv_timer_handler) == true) {
					printf("\n\r[%s]:create adv timer successful\r\n", __func__);
				} else {
					printf("\n\r[%s]:create adv timer failed\r\n", __func__);
				}

			}
		}
		if (h_adv_param) {
			os_mem_free(h_adv_param);
		}
		le_adv_start();
	}
	break;
	case BHA_API_MSG_STOP_ADV:
		le_adv_stop();
		break;
	case BHA_API_MSG_DISCONNECT:
		le_disconnect(0);
		break;
	case BHA_API_MSG_AUTH_RESPOND: {
		H_SecurityRsp_param *SecurityRsp_p = io_msg.u.buf;
		le_bond_user_confirm(0, SecurityRsp_p->accept);
		if (SecurityRsp_p) {
			os_mem_free(SecurityRsp_p);
		}
	}
	break;
	case BHA_API_MSG_ADD_SERVICE: {
		BHA_SERVICE_INFO *srv_info = io_msg.u.buf;
		bt_harmony_adapter_add_service(srv_info, (void *)bt_harmony_adapter_app_profile_callback);
	}
	break;

	case BHA_API_MSG_SEND_INDICATION: {
		BHA_INDICATION_PARAM *param = io_msg.u.buf;
		BHA_SERVICE_INFO *p_srv = bt_harmony_adapter_srvs_head.next;
		while (p_srv) {
			if (p_srv->srvId == param->srv_id) {
				break;
			} else {
				p_srv = p_srv->next;
			}
		}
		if (p_srv) {
			if (bt_harmony_adapter_send_indication(0, param->srv_id, param->att_handle, (uint8_t *)param->val, param->len, param->type)) {
				printf("\r\n[%s] send indication success", __FUNCTION__);
			} else {
				os_sem_give(send_indication_sem);
				printf("\r\n[%s] send indication fail", __FUNCTION__);
			}
		} else {
			os_sem_give(send_indication_sem);
			printf("\r\n[%s] Can not find service info", __FUNCTION__);
		}
		os_mem_free(param->val);
		os_mem_free(param);
	}
	break;
	default:
		break;
	}

}

void bt_harmony_adapter_app_handle_gap_msg(T_IO_MSG *p_gap_msg);

/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void bt_harmony_adapter_app_handle_io_msg(T_IO_MSG io_msg)
{
	uint16_t msg_type = io_msg.type;

	switch (msg_type) {
	case IO_MSG_TYPE_BT_STATUS: {
		bt_harmony_adapter_app_handle_gap_msg(&io_msg);
	}
	break;
	case IO_MSG_TYPE_QDECODE: {
		bt_harmony_adapter_app_handle_api_msg(io_msg);
	}
	break;
	default:
		break;
	}
}

void bt_harmony_adapter_app_handle_callback_msg(T_BHA_CALLBACK_MSG callback_msg)
{
	//Handle callback msg here
	switch (callback_msg.type) {
	case BHA_CALLBACK_MSG_ADV_STARTED: {
		AdvEnableCallback pfunc_advT = BH_GattCallbacks.advEnableCb;
		if (pfunc_advT) {
			pfunc_advT(0, OHOS_BT_STATUS_SUCCESS);
		}
		break;
	}
	case BHA_CALLBACK_MSG_ADV_STOPED: {
		AdvDisableCallback pfunc_advS = BH_GattCallbacks.advDisableCb;
		if (pfunc_advS) {
			pfunc_advS(0, OHOS_BT_STATUS_SUCCESS);
		}
		break;
	}
	case BHA_CALLBACK_MSG_CONNECTED: {
		T_BHA_CONNECTED_CALLBACK_DATA *conn_data_C = callback_msg.buf;
		ConnectServerCallback pfunc_C = BH_GattServerCallbacks.connectServerCb;
		if (pfunc_C) {
			pfunc_C(conn_data_C->conn_id, conn_data_C->serverID, &(conn_data_C->bt_addr));
		}
		if (conn_data_C) {
			os_mem_free(conn_data_C);
		}
		break;
	}
	case BHA_CALLBACK_MSG_DISCONNECTED: {
		T_BHA_CONNECTED_CALLBACK_DATA *conn_data_D = callback_msg.buf;
		DisconnectServerCallback pfunc_D = BH_GattServerCallbacks.disconnectServerCb;
		if (pfunc_D) {
			pfunc_D(conn_data_D->conn_id, conn_data_D->serverID, &(conn_data_D->bt_addr));
		}
		if (conn_data_D) {
			os_mem_free(conn_data_D);
		}
		break;
	}
	case BHA_CALLBACK_MSG_AUTH_REQUEST: {
		BdAddr bt_addr;
		memcpy(bt_addr.addr, callback_msg.buf, OHOS_BD_ADDR_LEN);
		SecurityRespondCallback pfunc_A = BH_GattCallbacks.securityRespondCb;
		if (pfunc_A) {
			pfunc_A(&bt_addr);
		}
		break;
	}
	case BHA_CALLBACK_MSG_MTU_CHANGED: {
		T_BHA_MTU_CALLBACK_DATA *mtu_data = callback_msg.buf;
		MtuChangeCallback pfunc_M = BH_GattServerCallbacks.mtuChangeCb;
		if (pfunc_M) {
			pfunc_M(mtu_data->conn_id, mtu_data->mtu_size);
		}
		if (mtu_data) {
			os_mem_free(mtu_data);
		}
		break;
	}
	case BHA_CALLBACK_MSG_SERVICE_ADDED: {
		ServiceStartCallback p_func = BH_GattServerCallbacks.serviceStartCb;
		T_BHA_SRV_ADDED_CALLBACK_DATA *p_data = callback_msg.buf;
		BHA_SERVICE_INFO *p_srv = bt_harmony_adapter_srvs_head.next;
		while (p_srv) {
			if (p_srv->srvId == p_data->srv_id && p_srv->status == BHA_SERVICE_REG_SUCCESS) {
				break;
			} else {
				p_srv = p_srv->next;
			}
		}
		int status = OHOS_BT_STATUS_FAIL;
		int srv_id = 0xffff;
		int srv_handle = 0;
		if (p_srv) {
			if (p_data->result == GATT_SERVER_SUCCESS) {
				status = OHOS_BT_STATUS_SUCCESS;
			}
			srv_id = p_data->srv_id;
			srv_handle = p_srv->start_handle;
		} else {
			printf("\r\n[%s] Can not find service info", __FUNCTION__);
		}

		if (p_func) {
			p_func(status, srv_id, srv_handle);
		} else {
			printf("\r\n[%s]  serviceStartCallback callback is NULL\n", __FUNCTION__);
		}
		os_mem_free(p_data);
	}
	break;

	case BHA_CALLBACK_MSG_INDICATION_SENT: {
		T_BHA_IND_SENT_CALLBACK_DATA *p_data = callback_msg.buf;
		BHA_SERVICE_INFO *p_srv = bt_harmony_adapter_srvs_head.next;
		while (p_srv) {
			if (p_srv->srvId == p_data->srv_id && p_srv->status == BHA_SERVICE_REG_SUCCESS) {
				break;
			} else {
				p_srv = p_srv->next;
			}
		}
		if (p_srv) {
			BleGattServiceIndicate pfunc = (p_srv->cbInfo[p_data->att_index]).func.indicate;
			if (pfunc) {
				pfunc(&(p_data->result), 1);
			} else {
				printf("\r\n[%s] BleGattServiceIndicate callback is NULL", __FUNCTION__);
			}
		} else {
			printf("\r\n[%s] Can not find service info", __FUNCTION__);
		}
		os_mem_free(p_data);
	}
	break;
	case BHA_CALLBACK_MSG_WRITE_SENT: {
		T_BHA_WRITE_DATA *p_data = callback_msg.buf;
		BleGattServiceWrite p_func = p_data->write_cb;
		if (p_func) {
			p_func(p_data->write_value, p_data->write_len);
		} else {
			printf("\r\n[%s] BleGattServiceWrite callback is NULL", __FUNCTION__);
		}
		os_mem_free(p_data);
	}
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
void bt_harmony_adapter_app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
	APP_PRINT_INFO3("bt_harmony_adapter_app_handle_dev_state_evt: init state %d, adv state %d, cause 0x%x",
					new_state.gap_init_state, new_state.gap_adv_state, cause);
	if (bt_harmony_adapter_gap_dev_state.gap_init_state != new_state.gap_init_state) {
		if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
			APP_PRINT_INFO0("GAP stack ready");
			printf("GAP stack ready\r\n");
		}
	}

	if (bt_harmony_adapter_gap_dev_state.gap_adv_state != new_state.gap_adv_state) {
		if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE) {
			if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN) {
				APP_PRINT_INFO0("GAP adv stoped: because connection created");
				printf("GAP adv stoped: because connection created\r\n");
			} else {
				APP_PRINT_INFO0("GAP adv stoped");
				printf("GAP adv stopped\r\n");
				bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_ADV_STOPED, NULL);
			}
		} else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING) {
			APP_PRINT_INFO0("GAP adv start");
			printf("GAP adv start\r\n");

			if (RT_duration != 0) {
				os_timer_start(&bt_harmony_BleAdv_Timer);
				RT_duration = 0;
			}
			bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_ADV_STARTED, NULL);
		}
	}

	bt_harmony_adapter_gap_dev_state = new_state;
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
void bt_harmony_adapter_app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
	APP_PRINT_INFO4("bt_harmony_adapter_app_handle_conn_state_evt: conn_id %d old_state %d new_state %d, disc_cause 0x%x",
					conn_id, bt_harmony_adapter_gap_conn_state, new_state, disc_cause);
	switch (new_state) {
	case GAP_CONN_STATE_DISCONNECTED: {
		if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
			&& (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))) {
			APP_PRINT_ERROR1("bt_harmony_adapter_app_handle_conn_state_evt: connection lost cause 0x%x", disc_cause);
		}
		printf("BT Disconnected\r\n");
		T_BHA_CONNECTED_CALLBACK_DATA *conn_data = (T_BHA_CONNECTED_CALLBACK_DATA *) os_mem_alloc(0, sizeof(T_BHA_CONNECTED_CALLBACK_DATA));
		memset(conn_data, 0, sizeof(T_BHA_CONNECTED_CALLBACK_DATA));
		conn_data->conn_id = conn_id;
		conn_data->serverID = 0;
		memcpy(conn_data->bt_addr.addr, RT_remote_bd, OHOS_BD_ADDR_LEN);
		if(bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_DISCONNECTED, conn_data) != true){
			os_mem_free(conn_data);
		}
		memset(RT_remote_bd, 0, OHOS_BD_ADDR_LEN);
	}
	break;

	case GAP_CONN_STATE_CONNECTED: {
		uint16_t conn_interval;
		uint16_t conn_latency;
		uint16_t conn_supervision_timeout;
		T_GAP_REMOTE_ADDR_TYPE remote_bd_type;

		le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
		le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
		le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
		le_get_conn_addr(conn_id, RT_remote_bd, (void *)&remote_bd_type);
		APP_PRINT_INFO5("GAP_CONN_STATE_CONNECTED: remote_bd %s, remote_addr_type %d, conn_interval 0x%x, conn_latency 0x%x, conn_supervision_timeout 0x%x",
						TRACE_BDADDR(RT_remote_bd), remote_bd_type, conn_interval, conn_latency, conn_supervision_timeout);
		printf("BT Connected\r\n");
		uint8_t tmp = 0;
		for (int i = 0; i < OHOS_BD_ADDR_LEN / 2; ++i) {
			tmp = RT_remote_bd[OHOS_BD_ADDR_LEN - 1 - i];
			RT_remote_bd[OHOS_BD_ADDR_LEN - 1 - i] = RT_remote_bd[i];
			RT_remote_bd[i] = tmp;
		}
		T_BHA_CONNECTED_CALLBACK_DATA *conn_data = (T_BHA_CONNECTED_CALLBACK_DATA *)os_mem_alloc(0, sizeof(T_BHA_CONNECTED_CALLBACK_DATA));
		memset(conn_data, 0, sizeof(T_BHA_CONNECTED_CALLBACK_DATA));
		conn_data->conn_id = conn_id;
		conn_data->serverID = 0;
		memcpy(conn_data->bt_addr.addr, RT_remote_bd, OHOS_BD_ADDR_LEN);
		if(bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_CONNECTED, conn_data) != true){
			os_mem_free(conn_data);
		}
	}
	break;

	default:
		break;
	}

	bt_harmony_adapter_gap_conn_state = new_state;
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
void bt_harmony_adapter_app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
	APP_PRINT_INFO2("bt_harmony_adapter_app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

	switch (new_state) {
	case GAP_AUTHEN_STATE_STARTED: {
		APP_PRINT_INFO0("bt_harmony_adapter_app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
	}
	break;

	case GAP_AUTHEN_STATE_COMPLETE: {
		if (cause == GAP_SUCCESS) {
			printf("Pair success\r\n");
			APP_PRINT_INFO0("bt_harmony_adapter_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");

		} else {
			printf("Pair failed: cause 0x%x\r\n", cause);
			APP_PRINT_INFO0("bt_harmony_adapter_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");
		}
	}
	break;

	default: {
		APP_PRINT_ERROR1("bt_harmony_adapter_app_handle_authen_state_evt: unknown newstate %d", new_state);
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
void bt_harmony_adapter_app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
	APP_PRINT_INFO2("app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
	T_BHA_MTU_CALLBACK_DATA *mtu_data = (T_BHA_MTU_CALLBACK_DATA *)os_mem_alloc(0, sizeof(T_BHA_MTU_CALLBACK_DATA));
	mtu_data->conn_id = conn_id;
	mtu_data->mtu_size = mtu_size;
	if(bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_MTU_CHANGED, mtu_data) != true){
		os_mem_free(mtu_data);
	}
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note     All the connection parameter update change  events are pre-handled in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause Use this cause when status is GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 * @return   void
 */
void bt_harmony_adapter_app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
{
	switch (status) {
	case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS: {
		uint16_t conn_interval;
		uint16_t conn_slave_latency;
		uint16_t conn_supervision_timeout;

		le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
		le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
		le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
		APP_PRINT_INFO3("bt_harmony_adapter_app_handle_conn_param_update_evt update success: conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
						conn_interval, conn_slave_latency, conn_supervision_timeout);
		printf("bt_harmony_adapter_app_handle_conn_param_update_evt update success: conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x\r\n",
			   conn_interval, conn_slave_latency, conn_supervision_timeout);
	}
	break;

	case GAP_CONN_PARAM_UPDATE_STATUS_FAIL: {
		APP_PRINT_ERROR1("bt_harmony_adapter_app_handle_conn_param_update_evt update failed: cause 0x%x", cause);
		printf("bt_harmony_adapter_app_handle_conn_param_update_evt update failed: cause 0x%x\r\n", cause);
	}
	break;

	case GAP_CONN_PARAM_UPDATE_STATUS_PENDING: {
		APP_PRINT_INFO0("bt_harmony_adapter_app_handle_conn_param_update_evt update pending");
		printf("bt_harmony_adapter_app_handle_conn_param_update_evt update pending\r\n");
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
void bt_harmony_adapter_app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
	T_LE_GAP_MSG gap_msg;
	uint8_t conn_id;
	memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

	APP_PRINT_TRACE1("bt_harmony_adapter_app_handle_gap_msg: subtype %d", p_gap_msg->subtype);
	switch (p_gap_msg->subtype) {
	case GAP_MSG_LE_DEV_STATE_CHANGE: {
		bt_harmony_adapter_app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
				gap_msg.msg_data.gap_dev_state_change.cause);
	}
	break;

	case GAP_MSG_LE_CONN_STATE_CHANGE: {
		bt_harmony_adapter_app_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
				(T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
				gap_msg.msg_data.gap_conn_state_change.disc_cause);
	}
	break;

	case GAP_MSG_LE_CONN_MTU_INFO: {
		bt_harmony_adapter_app_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
				gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
	}
	break;

	case GAP_MSG_LE_CONN_PARAM_UPDATE: {
		bt_harmony_adapter_app_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
				gap_msg.msg_data.gap_conn_param_update.status,
				gap_msg.msg_data.gap_conn_param_update.cause);
	}
	break;

	case GAP_MSG_LE_AUTHEN_STATE_CHANGE: {
		bt_harmony_adapter_app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
				gap_msg.msg_data.gap_authen_state.new_state,
				gap_msg.msg_data.gap_authen_state.status);
	}
	break;

	case GAP_MSG_LE_BOND_JUST_WORK: {
		conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
		le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
		APP_PRINT_INFO0("GAP_MSG_LE_BOND_JUST_WORK");
	}
	break;

	case GAP_MSG_LE_BOND_PASSKEY_DISPLAY: {
		uint32_t display_value = 0;
		conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
		le_bond_get_display_key(conn_id, &display_value);
		APP_PRINT_INFO1("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: passkey %d", display_value);
		le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
		printf("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: passkey %d\r\n", display_value);
	}
	break;

	case GAP_MSG_LE_BOND_USER_CONFIRMATION: {
		uint32_t display_value = 0;
		conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
		le_bond_get_display_key(conn_id, &display_value);
		APP_PRINT_INFO1("GAP_MSG_LE_BOND_USER_CONFIRMATION: passkey %d", display_value);
		printf("GAP_MSG_LE_BOND_USER_CONFIRMATION: passkey %d\r\n", display_value);
		bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_AUTH_REQUEST, RT_remote_bd);
	}
	break;

	case GAP_MSG_LE_BOND_PASSKEY_INPUT: {
		conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
		APP_PRINT_INFO1("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d", conn_id);
		printf("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d\r\n", conn_id);
	}
	break;
#if F_BT_LE_SMP_OOB_SUPPORT
	case GAP_MSG_LE_BOND_OOB_INPUT: {
		uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
		APP_PRINT_INFO0("GAP_MSG_LE_BOND_OOB_INPUT");
		le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
		le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
	}
	break;
#endif
	default:
		APP_PRINT_ERROR1("bt_harmony_adapter_app_handle_gap_msg: unknown subtype %d", p_gap_msg->subtype);
		break;
	}
}

/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT bt_harmony_adapter_app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
	T_APP_RESULT result = APP_RESULT_SUCCESS;
	T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

	switch (cb_type) {
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
	case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
		APP_PRINT_INFO3("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x",
						p_data->p_le_data_len_change_info->conn_id,
						p_data->p_le_data_len_change_info->max_tx_octets,
						p_data->p_le_data_len_change_info->max_tx_time);
		break;
#endif
	case GAP_MSG_LE_MODIFY_WHITE_LIST:
		APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
						p_data->p_le_modify_white_list_rsp->operation,
						p_data->p_le_modify_white_list_rsp->cause);
		break;

	default:
		APP_PRINT_ERROR1("app_gap_callback: unhandled cb_type 0x%x", cb_type);
		break;
	}
	return result;
}

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
T_APP_RESULT bt_harmony_adapter_app_profile_callback(T_SERVER_ID service_id, void *p_data)
{
	T_APP_RESULT app_result = APP_RESULT_SUCCESS;
	if (service_id == SERVICE_PROFILE_GENERAL_ID) {
		T_SERVER_APP_CB_DATA *p_param = (T_SERVER_APP_CB_DATA *)p_data;
		switch (p_param->eventId) {
		case PROFILE_EVT_SRV_REG_COMPLETE:
			APP_PRINT_INFO1("PROFILE_EVT_SRV_REG_COMPLETE: result %d", p_param->event_data.service_reg_result);
			break;
		case PROFILE_EVT_SRV_REG_AFTER_INIT_COMPLETE: {
			printf("\r\nT_SERVER_REG_AFTER_INIT_RESULT: result %d service_id %d cause %d",
				   p_param->event_data.server_reg_after_init_result.result,
				   p_param->event_data.server_reg_after_init_result.service_id,
				   p_param->event_data.server_reg_after_init_result.cause);
			T_BHA_SRV_ADDED_CALLBACK_DATA *callback_data = (T_BHA_SRV_ADDED_CALLBACK_DATA *) os_mem_alloc(0, sizeof(T_BHA_SRV_ADDED_CALLBACK_DATA));
			callback_data->result = p_param->event_data.server_reg_after_init_result.result;
			callback_data->srv_id = p_param->event_data.server_reg_after_init_result.service_id;
			if(bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_SERVICE_ADDED, callback_data) != true){
				os_mem_free(callback_data);
			}
		}
		break;
		case PROFILE_EVT_SEND_DATA_COMPLETE:
			APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
							p_param->event_data.send_data_result.conn_id,
							p_param->event_data.send_data_result.cause,
							p_param->event_data.send_data_result.service_id,
							p_param->event_data.send_data_result.attrib_idx,
							p_param->event_data.send_data_result.credits);
			if (p_param->event_data.send_data_result.cause == GAP_SUCCESS) {
				APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
			} else {
				APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
			}
			os_sem_give(send_indication_sem);
			T_BHA_IND_SENT_CALLBACK_DATA *callback_data = (T_BHA_IND_SENT_CALLBACK_DATA *) os_mem_alloc(0, sizeof(T_BHA_IND_SENT_CALLBACK_DATA));
			callback_data->result = p_param->event_data.send_data_result.cause;
			callback_data->srv_id = p_param->event_data.send_data_result.service_id;
			callback_data->att_index = p_param->event_data.send_data_result.attrib_idx;
			if(bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_INDICATION_SENT, callback_data) != true){
				os_mem_free(callback_data);
			}
			break;

		default:
			break;
		}
	} else {
		T_HARMONY_CALLBACK_DATA *p_harmony_cb_data = p_data;

		switch (p_harmony_cb_data->msg_type) {
		case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION: {
			if (p_harmony_cb_data->msg_data.cccd.ccc_val & GATT_CLIENT_CHAR_CONFIG_NOTIFY) {
				printf("\r\n[%s] cccd 0x%x update : notify enable", __FUNCTION__, p_harmony_cb_data->msg_data.cccd.attr_index);
			} else {
				printf("\r\n[%s] cccd 0x%x update : notify disable", __FUNCTION__, p_harmony_cb_data->msg_data.cccd.attr_index);
			}
			if (p_harmony_cb_data->msg_data.cccd.ccc_val & GATT_CLIENT_CHAR_CONFIG_INDICATE) {
				printf("\r\n[%s] cccd 0x%x update : indicate enable", __FUNCTION__, p_harmony_cb_data->msg_data.cccd.attr_index);
			} else {
				printf("\r\n[%s] cccd 0x%x update : indicate disable", __FUNCTION__, p_harmony_cb_data->msg_data.cccd.attr_index);
			}
			break;
		}
		case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE: {
			/* call user defined callback */
			if (p_harmony_cb_data->msg_data.read.read_cb) {
				BleGattServiceRead p_func = p_harmony_cb_data->msg_data.read.read_cb;
				p_func(p_harmony_cb_data->msg_data.read.p_value, p_harmony_cb_data->msg_data.read.p_len);
			} else {
				printf("\r\n[%s] User's read callback is NULL", __FUNCTION__);
			}
			printf("\r\n[%s] read len %d data 0x", __FUNCTION__,
				   *(p_harmony_cb_data->msg_data.read.p_len));
			for (int i = 0; i < * (p_harmony_cb_data->msg_data.read.p_len); i++) {
				printf("%x ", *(p_harmony_cb_data->msg_data.read.p_value + i));
			}
			break;
		}
		case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE: {
			printf("\r\n[%s] write_type %d len %d data 0x", __FUNCTION__,
				   p_harmony_cb_data->msg_data.write.write_type,
				   p_harmony_cb_data->msg_data.write.len);
			if (p_harmony_cb_data->msg_data.write.write_cb) {
				T_BHA_WRITE_DATA *MSG_DATA = (T_BHA_WRITE_DATA *)os_mem_alloc(0, sizeof(T_BHA_WRITE_DATA));
				memcpy(MSG_DATA->write_value, p_harmony_cb_data->msg_data.write.p_value, p_harmony_cb_data->msg_data.write.len);
				MSG_DATA->write_len = p_harmony_cb_data->msg_data.write.len;
				MSG_DATA->write_cb = p_harmony_cb_data->msg_data.write.write_cb;
				if(bt_harmony_adapter_app_send_callback_msg(BHA_CALLBACK_MSG_WRITE_SENT, MSG_DATA) != true){
					os_mem_free(MSG_DATA);
				}
			} else {
				printf("\r\n[%s] User's write callback is NULL", __FUNCTION__);
			}

			break;
		}
		default:
			break;
		}
	}

	return app_result;
}

#endif
