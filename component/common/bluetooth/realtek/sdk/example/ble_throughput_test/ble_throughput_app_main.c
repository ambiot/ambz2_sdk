/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    main.c
  * @brief   This is the entry of user code which the main function resides in.
  * @details
  * @author  jane
  * @date    2016-02-18
  * @version v0.1
  ******************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2016 Realtek Semiconductor Corporation</center></h2>
  ******************************************************************************
  */

/** Add Includes here **/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST
#include <string.h>
#include "os_sched.h"
#include "ble_throughput_app_task.h"
#include "trace_app.h"
#include "gap.h"
#include "gap_bond_le.h"
#include "gap_scan.h"
#include <gap_config.h>
#include <bte.h>
#include <wifi_constants.h>
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"

#if F_BT_LE_GATT_CLIENT_SUPPORT
#include "profile_client.h"
#endif
#include "gap_msg.h"
#include "ble_throughput_app.h"
#include "ble_throughput_user_cmd.h"
#include "data_uart.h"

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
#include "gap_adv.h"
#endif

#if F_BT_LE_GATT_SERVER_SUPPORT
#include "profile_server.h"
#include "ble_throughput_vendor_tp_service.h"
#endif

#if F_BT_LE_GATT_CLIENT_SUPPORT
#include <gaps_client.h>
#include "ble_throughput_vendor_tp_client.h"
#endif

#include "trace_uart.h"
#include "ble_throughput_app_flags.h"

/**< Default scan interval (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_INTERVAL     0x10
/**< Default scan window (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_WINDOW       0x10

/**<  Client ID for Simple BLE Client module, get from lower layer when add Simple BLE Client module. */
#if F_BT_LE_GATT_CLIENT_SUPPORT
T_CLIENT_ID   vendor_tp_client_id;
#endif

uint8_t gSimpleProfileServiceId;


#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL_MIN            320        //0x20 /* 20ms */
#define DEFAULT_ADVERTISING_INTERVAL_MAX            320        //0x30 /* 30ms */
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     80
// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     800
// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0
// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scan_rsp_data[] =
{
    0x03,           /* length     */
    0x03,           /* type="More 16-bit UUIDs available" */
    0x12,
    0x18,
    /* place holder for Local Name, filled by BT stack. if not present */
    /* BT stack appends Local Name.                                    */
    0x03,           /* length     */
    0x19,           /* type="Appearance" */
    0xc1, 0x03,     /* Keyboard */
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8_t adv_data[] =
{
    /* Core spec. Vol. 3, Part C, Chapter 18 */
    /* Flags */
    0x02,            /* length     */
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
    0xc1, 0x03,     /* Keyboard */
    0x0D,           /* length     */
    0x09,           /* type="Complete local name" */
    'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', '_', 'Q', 'W', 'E' /* Keyboard Name */
};
#endif

/**
 * @brief  Config bt stack related feature
 *
 * NOTE: This function shall be called before @ref bte_init is invoked.
 * @return void
 */
void ble_throughput_bt_stack_config_init(void)
{
    gap_config_max_le_link_num(APP_MAX_LINKS);
    gap_config_max_le_paired_device(APP_MAX_LINKS);
}
/**
  * @brief  Initiate GAP parameters, these params will affect the local device's behavior.
  * @retval none.
  */
void ble_throughput_app_le_gap_init()
{
    uint8_t device_name[GAP_DEVICE_NAME_LEN] = "Bee_GapTest";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

#if F_BT_LE_GAP_SCAN_SUPPORT
    uint8_t scan_mode = GAP_SCAN_MODE_ACTIVE;
    uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
    uint16_t scan_window = DEFAULT_SCAN_WINDOW;
    uint8_t scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
#endif

    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint32_t passkey = 0; /* passkey "000000"*/
    uint8_t fix_passkey_enable = false;
    uint8_t secReqEnable = false;
    uint16_t secReqRequirement = GAP_AUTHEN_BIT_BONDING_FLAG;
#if F_BT_LE_PRIVACY_SUPPORT
    uint16_t privacy_timeout = 0x384;
#endif

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
    //advertising parameters
    uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
    uint8_t  advDirectType = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  advDirectAddr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;
    uint16_t advIntMin = DEFAULT_ADVERTISING_INTERVAL_MIN;
    uint16_t advIntMax = DEFAULT_ADVERTISING_INTERVAL_MIN;
#endif

    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);
#endif
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

#if F_BT_LE_GAP_SCAN_SUPPORT
    le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                      &scan_filter_policy);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                      &scan_filter_duplicate);
#endif

    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(uint32_t), &passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(uint8_t), &fix_passkey_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &secReqRequirement);
#if F_BT_LE_PRIVACY_SUPPORT
    le_privacy_set_param(GAP_PARAM_PRIVACY_TIMEOUT, sizeof(uint16_t), &privacy_timeout);
#endif

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
    //Set advertising parameters
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(advDirectType), &advDirectType);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(advDirectAddr), advDirectAddr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);

    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_set_param(GAP_PARAM_ADV_DATA, sizeof(adv_data), (void *)adv_data);
    le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);
#endif
}

/**
  * @brief  Initiate Profile.
  * @retval none.
  */
void ble_throughput_app_le_profile_init(void)
{
#if F_BT_LE_GATT_SERVER_SUPPORT
    server_init(1);
    gSimpleProfileServiceId = vendor_tp_service_add((void *)ble_throughput_app_profile_callback);
    server_register_app_cb(ble_throughput_app_profile_callback);
#endif

    /* Register app callback--App_CentralRoleCallback to handle events from GAP Central Role. */
    le_register_app_cb(ble_throughput_app_gap_callback);

#if F_BT_LE_GATT_CLIENT_SUPPORT
    client_init(1);
    vendor_tp_client_id = tp_client_add(ble_throughput_client_callback);
#endif

#if F_BT_LE_PRIVACY_SUPPORT
    le_privacy_register_cb(App_PrivacyCallback);
#endif
}

/**
  * @brief  PwrMgr_Init() contains the setting about power mode.
  * @retval  none.
  */
void pwrmgr_init()
{
#if F_BT_DLPS_EN
    data_uart_dlps_init();
    lps_mode_set(LPM_DLPS_MODE);
    lps_mode_pause();
#endif
}

/**
  * @brief  Task_Init() contains the initialization of all the tasks.
  *
  * NOTE: There are four tasks are initiated.
  * Lowerstack task and upperstack task are used by bluetooth stack.
  * Application task is task which user application code resides in.
  * Emergency task is reserved.
  *
  * @retval  none.
  */
void ble_throughput_task_init()
{
    ble_throughput_app_task_init();
}

/**
  * @brief  main() is the entry of user code.
  * @retval  none.
  */
int ble_throughput_app_main(void)
{
    bt_trace_init();
    ble_throughput_bt_stack_config_init();
    bte_init();
    le_gap_init(APP_MAX_LINKS);
    ble_throughput_app_le_gap_init();
    ble_throughput_app_le_profile_init();
    pwrmgr_init();
    ble_throughput_task_init();

	log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 0);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 0);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_WARN, 0);
	
    return 0;
}

extern void wifi_btcoex_set_bt_on(void);
int ble_throughput_app_init(void)
{
	int bt_stack_already_on = 0;
	T_GAP_DEV_STATE new_state;

	/*Wait WIFI init complete*/
	while( !wifi_is_up(RTW_STA_INTERFACE) ) {
		os_delay(1000);
	}
	
	//wifi_disable_powersave();

	le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
		bt_stack_already_on = 1;
		printf("[BLE Central]BT Stack already on\n\r");
		return 0;
	}
	else
		ble_throughput_app_main();
	
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

void ble_throughput_app_deinit(void)
{
	ble_throughput_app_task_deinit();
	T_GAP_DEV_STATE state;
	le_get_gap_param(GAP_PARAM_DEV_STATE , &state);
	if (state.gap_init_state != GAP_INIT_STATE_STACK_READY) {
	}
#if F_BT_DEINIT
	else {
		bte_deinit();
		bt_trace_uninit();
	}
#endif
}
#endif
