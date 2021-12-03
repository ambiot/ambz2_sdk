/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      main.c
   * @brief     Source file for BLE peripheral project, mainly used for initialize modules
   * @author    jane
   * @date      2017-06-12
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
#include <os_sched.h>
#include <string.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <profile_server.h>
#include <gap_msg.h>
#include <bte.h>
#include <gap_config.h>
#include <bt_flags.h>
#include "bt_harmony_adapter_app_task.h"
#include "bt_harmony_adapter_peripheral_app.h"
#include "bt_harmony_adapter_app_flags.h"
#include "platform_stdlib.h"
#include "wifi_constants.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "ohos_bt_gatt_server.h"
#include "ohos_bt_gatt_client.h"
#include "os_timer.h"
#include <os_sync.h>


/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @brief  Default minimum advertising interval when device is discoverable (units of 625us, 160=100ms) */
#define DEFAULT_ADVERTISING_INTERVAL_MIN            352 //220ms
/** @brief  Default maximum advertising interval */
#define DEFAULT_ADVERTISING_INTERVAL_MAX            384 //240ms


extern uint16_t H_auth_flags;
extern uint8_t  H_auth_io_cap;

extern void *bt_harmony_BleAdv_Timer;
extern void *send_indication_sem;
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief  Config bt stack related feature
 *
 * NOTE: This function shall be called before @ref bte_init is invoked.
 * @return void
 */
//extern void gap_config_hci_task_secure_context(uint32_t size);
extern void gap_config_deinit_flow(uint8_t deinit_flow);
void bt_harmony_adapter_stack_config_init(void)
{
	gap_config_cccd_not_check(CONFIG_GATT_CCCD_NOT_CHECK);
	gap_config_max_le_link_num(BT_HARMONY_ADAPTER_APP_MAX_LINKS);
	gap_config_max_le_paired_device(BT_HARMONY_ADAPTER_APP_MAX_LINKS);
	//gap_config_hci_task_secure_context(280);
	gap_config_deinit_flow(1);
}

/**
  * @brief  Initialize peripheral and gap bond manager related parameters
  * @return void
  */
void bt_harmony_adapter_app_le_gap_init(void)
{
	/* Device name and device appearance */
	uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BT_HARMONY_ADAPTER";
	uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
	uint8_t  slave_init_mtu_req = false;


	/* GAP Bond Manager parameters */
	uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
	uint16_t auth_flags = H_auth_flags;
	uint8_t  auth_io_cap = H_auth_io_cap;
#if F_BT_LE_SMP_OOB_SUPPORT
	uint8_t  auth_oob = false;
#endif
	uint8_t  auth_use_fix_passkey = false;
	uint32_t auth_fix_passkey = 0;
	uint8_t  auth_sec_req_enable = false;
	uint16_t auth_sec_req_flags = H_auth_flags;

	/* Set device name and device appearance */
	le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
	le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
	le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
					 &slave_init_mtu_req);


	/* Setup the GAP Bond Manager */
	gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
	gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
	gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
	gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
#endif
	le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
	le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
					  &auth_use_fix_passkey);
	le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
	le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
					  &auth_sec_req_flags);

	/* register gap message callback */
	le_register_app_cb(bt_harmony_adapter_app_gap_callback);
}

/**
 * @brief  Add GATT services and register callbacks
 * @return void
 */
void bt_harmony_adapter_app_le_profile_init(void)
{
	server_builtin_service_reg(false);

	server_init(BT_HARMONY_ADAPTER_SERVICE_MAX_NUM);

	server_register_app_cb(bt_harmony_adapter_app_profile_callback);
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Peripheral APP, thus only one APP task is init here
 * @return   void
 */
void bt_harmony_adapter_task_init(void)
{
	bt_harmony_adapter_app_task_init();
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int bt_harmony_adapter_app_main(void)
{
	bt_trace_init();
	bt_harmony_adapter_stack_config_init();
	bte_init();
	le_gap_init(BT_HARMONY_ADAPTER_APP_MAX_LINKS);
	bt_harmony_adapter_app_le_gap_init();
	bt_harmony_adapter_app_le_profile_init();
	bt_harmony_adapter_task_init();

	return 0;
}

int bt_harmony_adapter_app_init(void)
{
	T_GAP_DEV_STATE new_state;

	/*Wait WIFI init complete*/
	while (!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
		os_delay(1000);
	}

	//judge BT Harmony Adapter is already on
	le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
		printf("[BT Harmony Adapter] BT Stack already on\r\n");
		return 0;
	} else {
		bt_harmony_adapter_app_main();
	}

	bt_coex_init();

	/*Wait BT init complete*/
	do {
		os_delay(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	} while (new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

	return 0;
}

extern bool bt_trace_uninit(void);
extern T_GAP_DEV_STATE bt_harmony_adapter_gap_dev_state;

void bt_harmony_adapter_app_deinit(void)
{
	if (bt_harmony_BleAdv_Timer) {
		if (os_timer_delete(&bt_harmony_BleAdv_Timer) == true) {
			printf("\n\r[%s]:delete adv timer success\r\n", __func__);
		}
	}
	if (send_indication_sem) {
		os_sem_delete(send_indication_sem);
	}
	bt_harmony_adapter_app_task_deinit();

	T_GAP_DEV_STATE state;
	le_get_gap_param(GAP_PARAM_DEV_STATE, &state);
	if (state.gap_init_state != GAP_INIT_STATE_STACK_READY) {
		printf("[BT Harmony Adapter] BT Stack is not running\r\n");
	}
#if F_BT_DEINIT
	else {
		bte_deinit();
		bt_trace_uninit();
		memset(&bt_harmony_adapter_gap_dev_state, 0, sizeof(T_GAP_DEV_STATE));
		printf("[BT Harmony Adapter] BT Stack deinitalized\r\n");
	}
#endif
}
#endif
