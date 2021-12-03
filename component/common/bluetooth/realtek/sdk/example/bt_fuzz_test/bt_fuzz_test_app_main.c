/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      main.c
   * @brief     Source file for BLE scatternet project, mainly used for initialize modules
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
#if defined(CONFIG_BT_FUZZ_TEST) && CONFIG_BT_FUZZ_TEST
#include <os_sched.h>
#include <string.h>
#include <bt_fuzz_test_app_task.h>
//#include <trace.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_config.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <gap_msg.h>
#include <profile_client.h>
#include <simple_ble_client.h>
#include <gaps_client.h>
#include <bas_client.h>
#include <bt_fuzz_test_link_mgr.h>
#include <bt_fuzz_test_app.h>
#include <gap_adv.h>
#include <profile_server.h>
#include <bt_fuzz_test_simple_ble_service.h>
#include <bas.h>
#include "trace_uart.h"
#include <bte.h>
#include "wifi_constants.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "platform_stdlib.h"
#include <gap_adv.h>
#include <bt_flags.h>
#include <gls.h>
#include <ias.h>
#include <hrs.h>
#include <dis.h>
#if BT_FUZZ_TEST_F_BT_DLPS_EN
#include <dlps.h>
#include <data_uart_dlps.h>
#endif
#if BT_FUZZ_TEST_APP_GENERAL_CLIENT_TEST
#include <gcs_client.h>
#endif
#include <bt_fuzz_test_app_flags.h>

extern bool bt_trace_uninit(void);
/** @defgroup  SCATTERNET_DEMO_MAIN Scatternet Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */

/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @brief Default scan interval (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_INTERVAL     0x20
/** @brief Default scan window (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_WINDOW       0x10

/** @brief  Default minimum advertising interval when device is discoverable (units of 625us, 160=100ms) */
#define DEFAULT_ADVERTISING_INTERVAL_MIN            320
/** @brief  Default Maximum advertising interval */
#define DEFAULT_ADVERTISING_INTERVAL_MAX            320

/** @brief  GAP - scan response data (max size = 31 bytes) */
static const uint8_t scan_rsp_data[] =
{
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
};

/** @brief  GAP - Advertisement data (max size = 31 bytes, best kept short to conserve power) */
static const uint8_t adv_data[] =
{
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    0x03,
    GAP_ADTYPE_16BIT_COMPLETE,
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),

    0x0A,
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'F', 'U', 'Z', 'Z', '_', 'T', 'E', 'S', 'T',
};

/*============================================================================*
 *                              Functions
 *============================================================================*/

/**
  * @brief  Initialize gap related parameters
  * @return void
  */
void bt_fuzz_test_app_le_gap_init(void)
{
    /* Device name and device appearance */
    uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "FUZZ_TEST";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

    /* Scan parameters */
    uint8_t  scan_mode = GAP_SCAN_MODE_ACTIVE;
    uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
    uint16_t scan_window = DEFAULT_SCAN_WINDOW;
    uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_DISABLE;

    /* advertising parameters */
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = DEFAULT_ADVERTISING_INTERVAL_MIN;
    uint16_t adv_int_max = DEFAULT_ADVERTISING_INTERVAL_MIN;

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = true;
    uint32_t auth_fix_passkey = 000000;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    /* Set device name and device appearance */
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

    /* Set scan parameters */
    le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                      &scan_filter_policy);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                      &scan_filter_duplicate);


    /* Set advertising parameters */
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
    le_adv_set_param(GAP_PARAM_ADV_DATA, sizeof(adv_data), (void *)adv_data);
    le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);

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
    le_register_app_cb(bt_fuzz_test_app_gap_callback);
}

/**
 * @brief  Add GATT services, clients and register callbacks
 * @return void
 */
void bt_fuzz_test_app_le_profile_init(void)
{
    server_init(6);
    bt_fuzz_test_bas_srv_id  = bas_add_service((void *)bt_fuzz_test_app_profile_callback);
	bt_fuzz_test_gls_srv_id = gls_add_service((void *)bt_fuzz_test_app_profile_callback);

    bt_fuzz_test_ias_srv_id  = ias_add_service((void *)bt_fuzz_test_app_profile_callback);
    bt_fuzz_test_hrs_srv_id = hrs_add_service((void *)bt_fuzz_test_app_profile_callback);

    bt_fuzz_test_dis_srv_id = dis_add_service((void *)bt_fuzz_test_app_profile_callback);
    bt_fuzz_test_simp_srv_id = bt_fuzz_test_simp_ble_service_add_service((void *)bt_fuzz_test_app_profile_callback);
    server_register_app_cb(bt_fuzz_test_app_profile_callback);

    client_init(3);
    bt_fuzz_test_gaps_client_id  = gaps_add_client(bt_fuzz_test_app_client_callback, BT_FUZZ_TEST_APP_MAX_LINKS);
#if BT_FUZZ_TEST_APP_GENERAL_CLIENT_TEST
    bt_fuzz_test_gcs_client_id = gcs_add_client(bt_fuzz_test_gcs_client_callback, BT_FUZZ_TEST_APP_MAX_LINKS, BT_FUZZ_TEST_APP_MAX_DISCOV_TABLE_NUM);
#endif
    bt_fuzz_test_bas_client_id = bas_add_client(bt_fuzz_test_app_client_callback, BT_FUZZ_TEST_APP_MAX_LINKS);
    client_register_general_client_cb(bt_fuzz_test_app_client_callback);
}

void bt_fuzz_test_stack_config_init(void)
{
	gap_config_max_le_link_num(BT_FUZZ_TEST_APP_MAX_LINKS);
	gap_config_max_le_paired_device(BT_FUZZ_TEST_APP_MAX_LINKS);
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Central Client APP, thus only one APP task is init here
 * @return   void
 */
void bt_fuzz_test_task_init(void)
{
    bt_fuzz_test_app_task_init();
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int bt_fuzz_test_app_main(void)
{
    bt_trace_init();
	bt_fuzz_test_stack_config_init();
    bte_init();
    le_gap_init(BT_FUZZ_TEST_APP_MAX_LINKS);
    bt_fuzz_test_app_le_gap_init();
    bt_fuzz_test_app_le_profile_init();
    bt_fuzz_test_task_init();	

    return 0;
}

extern void wifi_btcoex_set_bt_on(void);
int bt_fuzz_test_app_init(void)
{
	//int bt_stack_already_on = 0;
	//(void) bt_stack_already_on;
	T_GAP_DEV_STATE new_state;

	/*Wait WIFI init complete*/
	while(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
		os_delay(1000);
	}

	//judge BLE central is already on
	le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
		//bt_stack_already_on = 1;
		printf("[BT Fuzz Test]BT Stack already on\n\r");
		return 0;
	}
	else
		bt_fuzz_test_app_main();
	
	bt_coex_init();

	/*Wait BT init complete*/
	do {
		os_delay(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	}while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

	/*Start BT WIFI coexistence*/
	wifi_btcoex_set_bt_on();
	return 0;

}

extern void bt_fuzz_test_task_deinit(void);
extern void gcs_delete_client(void);
extern void gaps_delete_client(void);
extern void bas_delete_client(void);
void bt_fuzz_test_app_deinit(void)
{
	bt_fuzz_test_task_deinit();
	T_GAP_DEV_STATE state;
	le_get_gap_param(GAP_PARAM_DEV_STATE , &state);
	if (state.gap_init_state != GAP_INIT_STATE_STACK_READY) {
		printf("[BT Fuzz Test]BT Stack is not running\n\r");
	}
#if F_BT_DEINIT
	else {
		gcs_delete_client();
		gaps_delete_client();
		bas_delete_client();
		bte_deinit();
		bt_trace_uninit();
		printf("[BT Fuzz Test]BT Stack deinitalized\n\r");
	}
#endif
}
#endif
/** @} */ /* End of group SCATTERNET_DEMO_MAIN */

