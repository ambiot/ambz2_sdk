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
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include <stdlib.h>
#include <os_sched.h>
#include <string.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_adv.h>
#include <gap_scan.h>
#include <gap_bond_le.h>
#include <profile_server.h>
#include <gap_msg.h>
#include <bt_datatrans_app_task.h>
#include <bt_datatrans_peripheral_application.h>
#include "bt_datatrans_app_flags.h"
#include "bt_datatrans_uart.h"
#include "bt_datatrans_module_param_config.h"
#include "bt_datatrans_at_hci_cmd_process.h"
#include "gap_config.h"
#include "os_timer.h"
#include "bt_datatrans_profile.h"
#if CENTRAL_MODE
#include "bt_datatrans_multilink_manager.h"
#include "bt_datatrans_client.h"
#include "profile_client.h"
#include "bt_datatrans_central_application.h"
#endif
#include "trace_uart.h"
#include <bte.h>
#include "wifi_constants.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "platform_stdlib.h"
#include <bt_flags.h>

/** @defgroup  PERIPH_DEMO_MAIN Peripheral Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */

/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @brief  Default minimum advertising interval when device is discoverable (units of 625us, 160=100ms) */
#define DEFAULT_ADVERTISING_INTERVAL_MIN            320
/** @brief  Default Maximum advertising interval */
#define DEFAULT_ADVERTISING_INTERVAL_MAX            320

/**< Default scan interval (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_INTERVAL     0x10
/**< Default scan window (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_WINDOW       0x10
/*============================================================================*
 *                              Variables
 *============================================================================*/

T_SERVER_ID  bt_datatrans_srv_id;
#if CENTRAL_MODE
T_CLIENT_ID   bt_datatrans_client_id;
#endif

transfer_info dataTransInfo;
config_info transferConfigInfo;

extern void datatrans_send_msg_to_app(T_IO_MSG *p_msg);
extern T_GAP_CONN_STATE bt_datatrans_gap_conn_state;
extern T_GAP_DEV_STATE bt_datatrans_gap_dev_state;
/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
  * @brief  Initialize peripheral and gap bond manager related parameters
  * @return void
  */
void bt_datatrans_bt_stack_config_init(void)
{
    gap_config_max_le_link_num(BT_DATATRANS_APP_MAX_LINKS);
    gap_config_max_le_paired_device(BT_DATATRANS_APP_MAX_LINKS);
}

void bt_datatrans_app_le_gap_init(void)
{
    //device name and device appearance
    uint8_t  gap_device_name[GAP_DEVICE_NAME_LEN] = DEFAULT_DEVICE_NAME;
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    uint8_t  slave_init_mtu_req = true;

    //advertising parameters
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    if (dataTransInfo.device_mode.role == ROLE_BEACON)
    {
        adv_evt_type = GAP_ADTYPE_ADV_NONCONN_IND;
    }
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = dataTransInfo.adv_interval;
    uint16_t adv_int_max = adv_int_min;

    //scan patameters
    uint8_t scan_mode = GAP_SCAN_MODE_ACTIVE;
    uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
    uint16_t scan_window = DEFAULT_SCAN_WINDOW;
    uint8_t scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

    //GAP Bond Manager parameters
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = dataTransInfo.pair_info.authenflag.value;
    uint8_t  auth_io_cap = dataTransInfo.pair_info.authen_iocap;
#if F_BT_LE_SMP_OOB_SUPPORT	
    uint8_t  auth_oob = false;
#endif
    uint8_t  auth_use_fix_passkey = true;
    uint32_t auth_fix_passkey = dataTransInfo.pincode;
    uint8_t  auth_sec_req_enalbe =  dataTransInfo.pair_info.auto_security_req;
    uint16_t auth_sec_req_flags = dataTransInfo.pair_info.authenflag.value;


    if (dataTransInfo.devicename_info.length <= 15)
    {
        memcpy(gap_device_name, dataTransInfo.devicename_info.device_name,
               dataTransInfo.devicename_info.length);
    }
    if (dataTransInfo.device_mode.advtype == GAP_ADTYPE_ADV_HDC_DIRECT_IND ||
        dataTransInfo.device_mode.advtype == GAP_ADTYPE_ADV_LDC_DIRECT_IND)
    {
        //for fast pair
        //memcpy(adv_direct_addr, datatrans_efuse.beacon_adv_direct_add, GAP_BD_ADDR_LEN);
    }

    if (transferConfigInfo.select_io != UART_AT) //HCI CMD mode, fix passkey false
    {
        auth_use_fix_passkey = false;
    }
    else   //AT CMD mode, set pairing config
    {
        auth_flags = GAP_AUTHEN_BIT_NONE;
        auth_sec_req_enalbe = true;
        auth_sec_req_flags = GAP_AUTHEN_BIT_NONE;

        switch (dataTransInfo.pair_info.pair_mode)
        {
        case NO_PASS_WORD:
            {
                auth_sec_req_enalbe = false;
            }
            break;
        case JUST_WORK:
            break;
        case PASS_WORD:
            {
                auth_flags = GAP_AUTHEN_BIT_MITM_FLAG;
                auth_sec_req_flags = GAP_AUTHEN_BIT_MITM_FLAG;
                auth_io_cap = GAP_IO_CAP_DISPLAY_ONLY;
            }
            break;
        case PASS_WORD_BOND:
            {
                auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_MITM_FLAG;
                auth_sec_req_flags = GAP_AUTHEN_BIT_MITM_FLAG;
                auth_io_cap = GAP_IO_CAP_DISPLAY_ONLY;
            }
            break;
        default:
            break;
        }
    }

    //Set device name and device appearance
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, gap_device_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
    le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
                     &slave_init_mtu_req);

    //Set advertising parameters
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);

    moduleParam_InitAdvAndScanRspData();
	
	// Set scan parameters
    le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                      &scan_filter_policy);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                      &scan_filter_duplicate);

    // Setup the GAP Bond Manager
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
#endif
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enalbe), &auth_sec_req_enalbe);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);


    /* register gap message callback */
    le_register_app_cb(bt_datatrans_app_gap_callback);
}


/**
 * @brief  Add GATT services and register callbacks
 * @return void
 */

void bt_datatrans_app_le_profile_init(void)
{
    server_init(1);
    bt_datatrans_srv_id = Datatrans_AddService((void*)bt_datatrans_app_profile_callback);
    server_register_app_cb(bt_datatrans_app_profile_callback);

#if CENTRAL_MODE
    client_init(1);
    bt_datatrans_client_id = datatrans_add_client(bt_datatrans_app_client_callback, BT_DATATRANS_APP_MAX_LINKS);
    client_register_general_client_cb(bt_datatrans_app_client_callback);
#endif
}


/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Peripheral APP, thus only one APP task is init here
 * @return   void
 */
void bt_datatrans_task_init(void)
{
    DataTransApplicationInit();
	bt_datatrans_uart_task_init();
    bt_datatrans_app_task_init();
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int bt_datatrans_app_main(void)
{      
	bt_trace_init();
	bt_datatrans_bt_stack_config_init();
	bte_init();
    le_gap_init(BT_DATATRANS_APP_MAX_LINKS);
	readconfig();	
    bt_datatrans_app_le_gap_init();
    if (dataTransInfo.device_mode.role != ROLE_BEACON)
    {
        bt_datatrans_app_le_profile_init();
    }	
    bt_datatrans_task_init();

    return 0;
}
/** @} */ /* End of group PERIPH_DEMO_MAIN */

extern void wifi_btcoex_set_bt_on(void);
int bt_datatrans_app_init(void)
{
	T_GAP_DEV_STATE new_state;
	
	/*Wait WIFI init complete*/
	while(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
		os_delay(1000);
	}

	//judge BT DATATRANS is already on
	le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
		//bt_stack_already_on = 1;
		printf("[BT Datatrans]BT Stack already on\n\r");
		return 0;
	}
	else
		bt_datatrans_app_main();

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

extern bool bt_trace_uninit(void);
void bt_datatrans_app_deinit(void)
{
	bt_datatrans_app_task_deinit();
	DataTransApplicationDeinit();
	bt_datatrans_uart_task_deinit();
	
	T_GAP_DEV_STATE state;
	le_get_gap_param(GAP_PARAM_DEV_STATE , &state);
	if (state.gap_init_state != GAP_INIT_STATE_STACK_READY) {
		printf("[BT Datatrans]BT Stack is not running\n\r");		
	}
#if F_BT_DEINIT
	else {
		bte_deinit();
		bt_trace_uninit();
		bt_datatrans_delete_client();
		memset(&bt_datatrans_gap_dev_state, 0, sizeof(T_GAP_DEV_STATE));
		printf("[BT Datatrans]BT Stack deinitalized\n\r");		
	}
#endif
}

#endif //end: #if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS