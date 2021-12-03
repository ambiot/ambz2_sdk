/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      ble_google_seamless_app.c
   * @brief     This file handles BLE google_seamless application routines.
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
#if defined(CONFIG_BT_GOOGLE_SEAMLESS) && CONFIG_BT_GOOGLE_SEAMLESS
#include "platform_stdlib.h"
#include <trace_app.h>
#include <string.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <profile_server.h>
#include <gap_msg.h>
#include <app_msg.h>
#include "os_mem.h"
#include "log_service.h"
#include "google_seamless.h"
#include "ble_google_seamless_app.h"
#include <gap_conn_le.h>
#include "platform_stdlib.h"
#include "app_common_flags.h"
#if (F_BT_LE_USE_RANDOM_ADDR==1)
#include "ftl_app.h"
#endif

/*============================================================================*
 *                              Constants
 *============================================================================*/
#if (F_BT_LE_USE_RANDOM_ADDR==1)
/** @brief  Define start offset of the flash to save static random address. */
#define BLE_PERIPHERAL_APP_STATIC_RANDOM_ADDR_OFFSET 0
#endif
/** @defgroup  PERIPH_APP Peripheral Application
    * @brief This file handles BLE google_seamless application routines.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @addtogroup  PERIPH_SEVER_CALLBACK Profile Server Callback Event Handler
    * @brief Handle profile server callback event
    * @{
    */
T_SERVER_ID     google_seamless_srv_id;  /**< Google seamless service id */
/** @} */ /* End of group PERIPH_SEVER_CALLBACK */
/** @defgroup  PERIPH_GAP_MSG GAP Message Handler
    * @brief Handle GAP Message
    * @{
    */
T_GAP_DEV_STATE google_seamless_gap_dev_state = {0, 0, 0, 0, 0};                 /**< GAP device state */
T_GAP_CONN_STATE google_seamless_gap_conn_state = GAP_CONN_STATE_DISCONNECTED; /**< GAP connection state */
extern bool google_seamless_provisioned;
/*============================================================================*
 *                              Functions
 *============================================================================*/
void google_seamless_handle_gap_msg(T_IO_MSG  *p_gap_msg);
/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void google_seamless_handle_io_msg(T_IO_MSG io_msg)
{
	uint16_t msg_type = io_msg.type;
	switch (msg_type)
	{
	case IO_MSG_TYPE_BT_STATUS:
		{
			google_seamless_handle_gap_msg(&io_msg);
		}
		break;
	case IO_MSG_TYPE_QDECODE:
		{
			if (io_msg.subtype == 0) {
				le_adv_stop();
			} else if (io_msg.subtype == 1) {
				le_adv_start();
			}else if(io_msg.subtype == GOOGLE_SEAMLESS_MSG_SEND_INDICATION){
				void *arg = io_msg.u.buf;
				GOOGLE_SEAMLESS_NOTIFICATION_PARAM *param = io_msg.u.buf;
				server_send_data(param->conn_id, param->srv_id, param->attrib_index, (uint8_t *)param->val, param->len, param->type);
				os_mem_free(param->val);
				os_mem_free(param);
			}
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
extern void google_seamless_get_adv_profile_infor(uint8_t **adv_profile, uint8_t* length);
void google_seamless_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
	APP_PRINT_INFO3("google_seamless_handle_dev_state_evt: init state %d, adv state %d, cause 0x%x",
					new_state.gap_init_state, new_state.gap_adv_state, cause);
	if (google_seamless_gap_dev_state.gap_init_state != new_state.gap_init_state)
	{
		if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
		{
			APP_PRINT_INFO0("GAP stack ready");
			printf("\n\r[BLE Google Seamless] GAP stack ready\n\r");
			uint8_t bt_addr[6]={0};
#if (F_BT_LE_USE_RANDOM_ADDR==2) && F_BT_LE_LOCAL_IRK_SETTING_SUPPORT
			T_GAP_RAND_ADDR_TYPE rand_addr_type = GAP_RAND_ADDR_RESOLVABLE;
			T_GAP_CAUSE result;
			result = le_gen_rand_addr(rand_addr_type, bt_addr);
			printf("[%s]le_gen_rand_addr result = %x\n\r",__func__,result);
			result = le_set_rand_addr(bt_addr);
			printf("[%s]le_set_rand_addr result = %x\n\r",__func__,result);
			memset(bt_addr,0,sizeof(uint8_t)*6);
#endif
			/*stack ready*/
			gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
			printf("local bd addr: 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n",
							bt_addr[5],
							bt_addr[4],
							bt_addr[3],
							bt_addr[2],
							bt_addr[1],
							bt_addr[0]);
			/*update adv data*/
			uint8_t* adv_profile;
			uint8_t length;
			google_seamless_get_adv_profile_infor(&adv_profile, &length);
			uint8_t* adv_data_send = (uint8_t *)os_mem_alloc(0, length);
			if(adv_data_send == NULL)
			{
				os_mem_free(adv_data_send);
				printf("Set provision advertisement package fail\n\r");
			}
			else
			{
				memcpy(adv_data_send, &(*adv_profile), length);
				if(google_seamless_provisioned == true)
				{
					*(adv_data_send+5)=LO_WORD(GATT_UUID_PROVISIONED_PROFILE);//the provision byte location is at 5th byte
					*(adv_data_send+6)=HI_WORD(GATT_UUID_PROVISIONED_PROFILE);//the provision byte location is at 6th byte
				}
				le_adv_set_param(GAP_PARAM_ADV_DATA, length, (void *)adv_data_send);
				os_mem_free(adv_data_send);
			}
			le_adv_start();
		}
	}

	if (google_seamless_gap_dev_state.gap_adv_state != new_state.gap_adv_state)
	{
		if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
		{
			if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
			{
				APP_PRINT_INFO0("GAP adv stoped: because connection created");
				printf("\n\rGAP adv stoped: because connection created\n\r");
			}
			else
			{
				APP_PRINT_INFO0("GAP adv stoped");
				printf("\n\rGAP adv stopped\n\r");
			}
		}
		else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
		{
			APP_PRINT_INFO0("GAP adv start");
			printf("\n\rGAP adv start\n\r");
		}
	}

	google_seamless_gap_dev_state = new_state;
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
void google_seamless_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
	APP_PRINT_INFO4("google_seamless_handle_conn_state_evt: conn_id %d old_state %d new_state %d, disc_cause 0x%x",
					conn_id, google_seamless_gap_conn_state, new_state, disc_cause);
	switch (new_state)
	{
	case GAP_CONN_STATE_DISCONNECTED:
		{
			if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
				&& (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
			{
				APP_PRINT_ERROR1("google_seamless_handle_conn_state_evt: connection lost cause 0x%x", disc_cause);
			}
			printf("\n\r[BLE Google Seamless] BT Disconnected, cause 0x%x, start ADV\n\r", disc_cause);
			/*update adv data*/
			uint8_t* adv_profile;
			uint8_t length;
			google_seamless_get_adv_profile_infor(&adv_profile, &length);
			uint8_t* adv_data_send = (uint8_t *)os_mem_alloc(0, length);
			if(adv_data_send == NULL)
			{
				os_mem_free(adv_data_send);
				if(google_seamless_provisioned == true)
				{
					printf("Set provision advertisement package fail\n\r");
				}
				else
				{
					printf("Set un-provision advertisement package fail\n\r");
				}
			}
			else
			{
				memcpy(adv_data_send, &(*adv_profile), length);
				if(google_seamless_provisioned == true)
				{
					*(adv_data_send+5)=LO_WORD(GATT_UUID_PROVISIONED_PROFILE);//the provision byte location is at 5th byte
					*(adv_data_send+6)=HI_WORD(GATT_UUID_PROVISIONED_PROFILE);//the provision byte location is at 6th byte
				}
				le_adv_set_param(GAP_PARAM_ADV_DATA, length, (void *)adv_data_send);
				os_mem_free(adv_data_send);
			}
			le_adv_start();
		}
		break;

	case GAP_CONN_STATE_CONNECTED:
		{
			uint16_t conn_interval;
			uint16_t conn_latency;
			uint16_t conn_supervision_timeout;
			uint8_t  remote_bd[6];
			T_GAP_REMOTE_ADDR_TYPE remote_bd_type;

			le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
			le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
			le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
			le_get_conn_addr(conn_id, remote_bd, (void *)&remote_bd_type);
			APP_PRINT_INFO5("GAP_CONN_STATE_CONNECTED:remote_bd %s, remote_addr_type %d, conn_interval 0x%x, conn_latency 0x%x, conn_supervision_timeout 0x%x",
							TRACE_BDADDR(remote_bd), remote_bd_type,
							conn_interval, conn_latency, conn_supervision_timeout);
			printf("\n\r[BLE Google Seamless] BT Connected\n\r");
		}
		break;

	default:
		break;
	}
	google_seamless_gap_conn_state = new_state;
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
void google_seamless_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
	APP_PRINT_INFO2("google_seamless_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

	switch (new_state)
	{
	case GAP_AUTHEN_STATE_STARTED:
		{
			APP_PRINT_INFO0("google_seamless_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
		}
		break;

	case GAP_AUTHEN_STATE_COMPLETE:
		{
			if (cause == GAP_SUCCESS)
			{
				printf("Pair success\r\n");
				APP_PRINT_INFO0("google_seamless_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");

			}
			else
			{
				printf("Pair failed: cause 0x%x\r\n", cause);
				APP_PRINT_INFO0("google_seamless_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");
			}
		}
		break;

	default:
		{
			APP_PRINT_ERROR1("google_seamless_handle_authen_state_evt: unknown newstate %d", new_state);
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
void google_seamless_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
	APP_PRINT_INFO2("google_seamless_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note     All the connection parameter update change  events are pre-handled in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause Use this cause when status is GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 * @return   void
 */
void google_seamless_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
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
			APP_PRINT_INFO3("google_seamless_handle_conn_param_update_evt update success:conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
							conn_interval, conn_slave_latency, conn_supervision_timeout);
			printf("google_seamless_handle_conn_param_update_evt update success:conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x\r\n",
							conn_interval, conn_slave_latency, conn_supervision_timeout);
		}
		break;

	case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
		{
			APP_PRINT_ERROR1("google_seamless_handle_conn_param_update_evt update failed: cause 0x%x", cause);
			printf("google_seamles_handle_conn_param_update_evt update failed: cause 0x%x\r\n", cause);
		}
		break;

	case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
		{
			APP_PRINT_INFO0("google_seamless_handle_conn_param_update_evt update pending.");
			printf("\n\rgoogle_seamles_handle_conn_param_update_evt update pending: conn_id %d\r\n", conn_id);
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
void google_seamless_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
	T_LE_GAP_MSG gap_msg;
	uint8_t conn_id;
	memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

	APP_PRINT_TRACE1("google_seamless_handle_gap_msg: subtype %d", p_gap_msg->subtype);
	switch (p_gap_msg->subtype)
	{
		case GAP_MSG_LE_DEV_STATE_CHANGE:
		{
			google_seamless_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
									 gap_msg.msg_data.gap_dev_state_change.cause);
		}
		break;

		case GAP_MSG_LE_CONN_STATE_CHANGE:
		{
			google_seamless_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
									  (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
									  gap_msg.msg_data.gap_conn_state_change.disc_cause);
		}
		break;

		case GAP_MSG_LE_CONN_MTU_INFO:
		{
			google_seamless_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
										 gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
		}
		break;

		case GAP_MSG_LE_CONN_PARAM_UPDATE:
		{
			google_seamless_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
											 gap_msg.msg_data.gap_conn_param_update.status,
											 gap_msg.msg_data.gap_conn_param_update.cause);
		}
		break;

		case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
		{
			google_seamless_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
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
			APP_PRINT_INFO1("GAP_MSG_LE_BOND_PASSKEY_DISPLAY:passkey %d", display_value);
			le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
				printf("GAP_MSG_LE_BOND_PASSKEY_DISPLAY:passkey %d\r\n", display_value);
		}
		break;

		case GAP_MSG_LE_BOND_USER_CONFIRMATION:
		{
			uint32_t display_value = 0;
			conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
			le_bond_get_display_key(conn_id, &display_value);
			APP_PRINT_INFO1("GAP_MSG_LE_BOND_USER_CONFIRMATION: passkey %d", display_value);
			printf("GAP_MSG_LE_BOND_USER_CONFIRMATION: passkey %d\r\n", display_value);
			//le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
		}
		break;

		case GAP_MSG_LE_BOND_PASSKEY_INPUT:
		{
			//uint32_t passkey = 888888;
			conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
			APP_PRINT_INFO1("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d", conn_id);
			//le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
				printf("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d\r\n", conn_id);
		}
		break;
#if F_BT_LE_SMP_OOB_SUPPORT
		case GAP_MSG_LE_BOND_OOB_INPUT:
		{
			uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
			APP_PRINT_INFO0("GAP_MSG_LE_BOND_OOB_INPUT");
			le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
			le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
		}
		break;
#endif
		default:
		{
			APP_PRINT_ERROR1("google_seamless_handle_gap_msg: unknown subtype %d", p_gap_msg->subtype);
			break;
		}
	}
}
/** @} */ /* End of group PERIPH_GAP_MSG */

/** @defgroup  PERIPH_GAP_CALLBACK GAP Callback Event Handler
    * @brief Handle GAP callback event
    * @{
    */
/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT google_seamless_gap_callback(uint8_t cb_type, void *p_cb_data)
{
	T_APP_RESULT result = APP_RESULT_SUCCESS;
	T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;

	switch (cb_type)
	{
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
		case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
		{
			APP_PRINT_INFO3("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x",
							p_data->p_le_data_len_change_info->conn_id,
							p_data->p_le_data_len_change_info->max_tx_octets,
							p_data->p_le_data_len_change_info->max_tx_time);
			break;
		}
#endif
		case GAP_MSG_LE_MODIFY_WHITE_LIST:
		{
			APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
							p_data->p_le_modify_white_list_rsp->operation,
							p_data->p_le_modify_white_list_rsp->cause);
			break;
		}

		default:
		{
			APP_PRINT_ERROR1("google_seamless_gap_callback: unhandled cb_type 0x%x", cb_type);
			break;
		}
	}
	return result;
}
/** @} */ /* End of group PERIPH_GAP_CALLBACK */

/** @defgroup  PERIPH_SEVER_CALLBACK Profile Server Callback Event Handler
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
T_APP_RESULT google_seamless_profile_callback(T_SERVER_ID service_id, void *p_data)
{
	T_APP_RESULT app_result = APP_RESULT_SUCCESS;
	if (service_id == SERVICE_PROFILE_GENERAL_ID)
	{
		T_SERVER_APP_CB_DATA *p_param = (T_SERVER_APP_CB_DATA *)p_data;
		switch (p_param->eventId)
		{
			case PROFILE_EVT_SRV_REG_COMPLETE:// srv register result event.
			{
				APP_PRINT_INFO1("PROFILE_EVT_SRV_REG_COMPLETE: result %d",
								p_param->event_data.service_reg_result);
				break;
			}

			case PROFILE_EVT_SEND_DATA_COMPLETE:
			{
				APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
								p_param->event_data.send_data_result.conn_id,
								p_param->event_data.send_data_result.cause,
								p_param->event_data.send_data_result.service_id,
								p_param->event_data.send_data_result.attrib_idx,
								p_param->event_data.send_data_result.credits);
				printf("\n\rPROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d\r\n",
								p_param->event_data.send_data_result.conn_id,
								p_param->event_data.send_data_result.cause,
								p_param->event_data.send_data_result.service_id,
								p_param->event_data.send_data_result.attrib_idx,
								p_param->event_data.send_data_result.credits);
				if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
				{
					APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
					printf("PROFILE_EVT_SEND_DATA_COMPLETE success\r\n");
				}
				else
				{
					APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
					printf("PROFILE_EVT_SEND_DATA_COMPLETE failed\r\n");
				}
				break;
			}

			default:
				break;
		}
	}
	else if (service_id == google_seamless_srv_id)
	{
		T_GOOGLE_SEAMLESS_CALLBACK_DATA *p_google_seamless_cb_data = (T_GOOGLE_SEAMLESS_CALLBACK_DATA *)p_data;
		switch (p_google_seamless_cb_data->msg_type)
		{
			case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
			{
				switch (p_google_seamless_cb_data->msg_data.notification_indification_index)
				{
					case GOOGLE_SEAMLESS_NOTIFY_INDICATE_V6_ENABLE:
					{
						printf("GOOGLE_NOTIFY_INDICATE_V6_ENABLE\r\n");
					}
					break;
					case GOOGLE_SEAMLESS_NOTIFY_INDICATE_V6_DISABLE:
					{
						printf("GOOGLE_NOTIFY_INDICATE_V6_DISABLE\r\n");
					}
					break;
					default:
						break;
				}
			}
			break;

			case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
			{
				switch (p_google_seamless_cb_data->msg_data.write.opcode)
				{
					case GOOGLE_SEAMLESS_WRITE_V5:
					{
						APP_PRINT_INFO2("GOOGLE_SEAMLESS_WRITE_V5: write type %d, len %d", p_google_seamless_cb_data->msg_data.write.write_type,
								p_google_seamless_cb_data->msg_data.write.len);
						google_seamless_set_parameter(GOOGLE_SEAMLESS_BLE_SERVICE_PARAM_V5_WRITE_CHAR_VAL, p_google_seamless_cb_data->msg_data.write.len, p_google_seamless_cb_data->msg_data.write.p_value);
					}
					break;
					case GOOGLE_SEAMLESS_WRITE_V6:
					{
						APP_PRINT_INFO2("GOOGLE_SEAMLESS_WRITE_V6: write type %d, len %d", p_google_seamless_cb_data->msg_data.write.write_type,
								p_google_seamless_cb_data->msg_data.write.len);
						google_seamless_set_parameter(GOOGLE_SEAMLESS_BLE_SERVICE_PARAM_V6_WRITE_CHAR_VAL, p_google_seamless_cb_data->msg_data.write.len, p_google_seamless_cb_data->msg_data.write.p_value);
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

/** @} */ /* End of group PERIPH_SEVER_CALLBACK */
/** @} */
/** @addtogroup  PERIPHERAL_APP
    * @{
    */
/** @defgroup  PERIPHERAL_RANDOM Static Random Address Storage
    * @brief Use @ref F_BT_LE_USE_RANDOM_ADDR to open
    * @{
    */
#if (F_BT_LE_USE_RANDOM_ADDR==1)
/**
 * @brief   Save static random address information into flash.
 * @param[in] p_addr Pointer to the buffer for saving data.
 * @retval 0 Save success.
 * @retval other Failed.
 */
uint32_t google_seamless_save_static_random_address(T_APP_STATIC_RANDOM_ADDR *p_addr)
{
	APP_PRINT_INFO0("google_seamless_save_static_random_address");
	return ftl_save(p_addr, BLE_PERIPHERAL_APP_STATIC_RANDOM_ADDR_OFFSET, sizeof(T_APP_STATIC_RANDOM_ADDR));
}
/**
  * @brief  Load static random address information from storage.
  * @param[out]  p_addr Pointer to the buffer for loading data.
  * @retval 0 Load success.
  * @retval other Failed.
  */
uint32_t google_seamless_load_static_random_address(T_APP_STATIC_RANDOM_ADDR *p_addr)
{
	uint32_t result;
	result = ftl_load(p_addr, BLE_PERIPHERAL_APP_STATIC_RANDOM_ADDR_OFFSET,
					  sizeof(T_APP_STATIC_RANDOM_ADDR));
	APP_PRINT_INFO1("google_seamless_load_static_random_address: result 0x%x", result);
	if (result)
	{
		memset(p_addr, 0, sizeof(T_APP_STATIC_RANDOM_ADDR));
	}
	return result;
}
#endif
/** @} */
/** @} */
/** @} */ /* End of group PERIPH_APP */
#endif
