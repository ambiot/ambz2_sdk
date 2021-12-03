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
#include <stdlib.h>
#include <os_sched.h>
#include <string.h>
#include <bt_mesh_provisioner_multiple_profile_app_task.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <gap_msg.h>
#include <bte.h>
#include <gap_config.h>
#include <profile_client.h>
#include <gaps_client.h>
#include <gap_adv.h>
#include <profile_server.h>
#include <gatt_builtin_services.h>
#include <platform_utils.h>

#include "mesh_api.h"
#include "mesh_cmd.h"
#include "provisioner_multiple_profile_app.h"
#include "health.h"
#include "ping.h"
#include "ping_app.h"
#include "tp.h"
#include "generic_client_app.h"
#include "light_client_app.h"
#include "provision_client.h"
#include "proxy_client.h"
#include "datatrans_app.h"
#include "bt_mesh_provisioner_multiple_profile_app_flags.h"
#include "vendor_cmd.h"
#include "vendor_cmd_bt.h"
#include "wifi_constants.h"
#if defined(MESH_RPR) && MESH_RPR
#include "rmt_prov_client_app.h"
#endif
#if defined(MESH_DFU) && MESH_DFU
#include "dfu_distributor_app.h"
#endif
#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO
#include "bt_mesh_app_list_intf.h"
#endif

#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
#include "bt_mesh_provisioner_api.h"
#endif

#if defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL
#include <simple_ble_service.h>
#include <bas.h>
#include "platform_stdlib.h"
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "bas.h"
#endif

#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
#include <gcs_client.h>
#include <ble_central_link_mgr.h>
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include "platform_stdlib.h"
#include "ble_central_at_cmd.h"
#endif

#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
#include <gcs_client.h>
#include <ble_scatternet_link_mgr.h>
#include "trace_uart.h"
#include <gap_le_types.h>
#include <simple_ble_config.h>
#include <wifi/wifi_conf.h>
#include "rtk_coex.h"
#include <simple_ble_service.h>
#include "bas.h"
#endif

#define COMPANY_ID        0x005D
#define PRODUCT_ID        0x0000
#define VERSION_ID        0x0000

#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO

extern struct BT_MESH_LIB_PRIV bt_mesh_lib_priv;

void bt_mesh_example_device_info_cb(uint8_t bt_addr[6], uint8_t bt_addr_type, int8_t rssi, device_info_t *pinfo)
{
    /* avoid gcc compile warning */
    (void)bt_addr;
    (void)bt_addr_type;
    (void)rssi;
    uint8_t NULL_UUID[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (pinfo->type == DEVICE_INFO_UDB) {
        if (rtw_memcmp(pinfo->pbeacon_udb->dev_uuid, NULL_UUID, 16) == 0)
            add_unproed_dev(pinfo->pbeacon_udb->dev_uuid);
    }
}

void generic_on_off_client_subscribe(void)
{
    uint16_t sub_addr = 0xFEFF;
    mesh_model_p pmodel = model_goo_client.pmodel;

    if (pmodel == NULL || MESH_NOT_SUBSCRIBE_ADDR(sub_addr)) {
        return;
    } else {
        mesh_model_sub(pmodel, sub_addr);
    }  
}
#endif

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
/** @brief  Default minimum advertising interval when device is discoverable (units of 625us, 160=100ms) */
#define DEFAULT_ADVERTISING_INTERVAL_MIN            320
/** @brief  Default maximum advertising interval */
#define DEFAULT_ADVERTISING_INTERVAL_MAX            400
#endif

#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
/** @brief Default scan interval (units of 0.625ms, 0x520=820ms) */
#define DEFAULT_SCAN_INTERVAL     0x520
/** @brief Default scan window (units of 0.625ms, 0x520=820ms) */
#define DEFAULT_SCAN_WINDOW       0x520
#endif

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
/** @brief  GAP - scan response data (max size = 31 bytes) */
static const uint8_t scan_rsp_data[] =
{
    0x03,                             /* length */
    GAP_ADTYPE_APPEARANCE,            /* type="Appearance" */
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
};
plt_timer_t bt_mesh_multiple_profile_peripheral_adv_timer = NULL;
uint8_t le_adv_start_enable = 0;
uint16_t bt_mesh_peripheral_adv_interval = 352;
extern int array_count_of_adv_data;
#endif

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))

extern void *bt_mesh_provisioner_multiple_profile_evt_queue_handle;  //!< Event queue handle
extern void *bt_mesh_provisioner_multiple_profile_io_queue_handle;   //!< IO queue handle
extern void	rtw_set_timer(_timer *ptimer,u32 delay_time);
extern void	rtw_init_timer(_timer *ptimer, void *adapter, TIMER_FUN pfunc,void* cntx, const char *name);

void bt_mesh_multiple_profile_peripheral_adv_timer_handler(void *FunctionContext)
{
    /* avoid gcc compile warning */
    (void)FunctionContext;
    uint8_t event = EVENT_IO_TO_APP;
    T_IO_MSG io_msg;
    uint16_t adv_interval = bt_mesh_peripheral_adv_interval * 625 / 1000;

    io_msg.type = IO_MSG_TYPE_ADV;

    if (os_msg_send(bt_mesh_provisioner_multiple_profile_io_queue_handle, &io_msg, 0) == false)
    {
    }
    else if (os_msg_send(bt_mesh_provisioner_multiple_profile_evt_queue_handle, &event, 0) == false)
    {
    }
    if (le_adv_start_enable) {
        plt_timer_change_period(bt_mesh_multiple_profile_peripheral_adv_timer, adv_interval, 0xFFFFFFFF);
    }
}

void mesh_le_adv_start(void)
{
    uint16_t adv_interval = bt_mesh_peripheral_adv_interval * 625 / 1000;

    le_adv_start_enable = 1;
    plt_timer_change_period(bt_mesh_multiple_profile_peripheral_adv_timer, adv_interval, 0xFFFFFFFF);
}

void mesh_le_adv_stop(void)
{
    le_adv_start_enable = 0;
}
#endif

/******************************************************************
 * @fn          Initial gap parameters
 * @brief      Initialize peripheral and gap bond manager related parameters
 *
 * @return     void
 */
void bt_mesh_provisioner_multiple_profile_stack_init(void)
{
    /** set ble stack log level, disable nonsignificant log */
    // log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 0);
    // log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 0);
    // log_module_trace_set(TRACE_MODULE_LOWERSTACK, TRACE_LEVEL_ERROR, 0);
	// log_module_trace_set(TRACE_MODULE_SNOOP, TRACE_LEVEL_ERROR, 0);

    /** set mesh stack log level, default all on, disable the log of level LEVEL_TRACE */
    uint32_t module_bitmap[MESH_LOG_LEVEL_SIZE] = {0};
    diag_level_set(TRACE_LEVEL_TRACE, module_bitmap);

    /** mesh stack needs rand seed */
    plt_srand(platform_random(0xffffffff));

    /** configure provisioning parameters */
    prov_params_set(PROV_PARAMS_CALLBACK_FUN, (void *)prov_cb, sizeof(prov_cb_pf));

    /** config node parameters */
    mesh_node_features_t features =
    {
        .role = MESH_ROLE_PROVISIONER,
        .relay = 1,
        .proxy = 1,
        .fn = 1,
        .lpn = 2,
        .prov = 1,
        .udb = 0,
        .snb = 1,
        .bg_scan = 1,
        .flash = 1,
        .flash_rpl = 1
    };

    mesh_node_cfg_t node_cfg =
    {
        .dev_key_num = 20,
        .net_key_num = 10,
        .app_key_num = 3,
        .vir_addr_num = 3,
        .rpl_num = 20,
        .sub_addr_num = 5,
        .proxy_num = 1
    };

#if defined(CONFIG_BT_MESH_TEST) && CONFIG_BT_MESH_TEST
	features.relay = 0;
	features.flash_rpl = 0;
	node_cfg.dev_key_num = 50;
	node_cfg.rpl_num = 50;
	node_cfg.sub_addr_num = 0;
#endif

	node_cfg.proxy_interval = 5;
	mesh_node_cfg(features, &node_cfg);   
	mesh_node.net_trans_count = 6;
    mesh_node.relay_retrans_count = 2;
    mesh_node.trans_retrans_count = 4;
    mesh_node.ttl = 5;

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))

    le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);

#endif

    /** create elements and register models */
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    cfg_client_reg();  
    generic_client_models_init();
#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO
    init_bt_mesh_priv();
#else
    tp_control_reg(tp_reveive);
    ping_control_reg(ping_app_ping_cb, pong_receive);
    trans_ping_pong_init(ping_app_ping_cb, pong_receive);
    light_client_models_init();
	datatrans_model_init();
#endif
#if defined(MESH_DFU) && MESH_DFU
    dfu_dist_models_init();
#endif
#if defined(MESH_RPR) && MESH_RPR
    rmt_prov_client_init();
#endif 
#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    bt_mesh_multiple_profile_peripheral_adv_timer = plt_timer_create("bt_mesh_multiple_profile_peripheral_adv_timer", 0xFFFFFFFF, FALSE, NULL, bt_mesh_multiple_profile_peripheral_adv_timer_handler);
    if (!bt_mesh_multiple_profile_peripheral_adv_timer) {
        printf("[BT Mesh Provisioner] Create adv timer failed\n\r");
    }
#endif

    compo_data_page0_header_t compo_data_page0_header = {COMPANY_ID, PRODUCT_ID, VERSION_ID};
    compo_data_page0_gen(&compo_data_page0_header);

    /** init mesh stack */
    mesh_init();

    /** register udb/provision adv/proxy adv callback */
#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO	
	device_info_cb_reg(bt_mesh_example_device_info_cb);
    hb_init(hb_cb);
    generic_on_off_client_subscribe();
#else	
    device_info_cb_reg(device_info_cb);
    hb_init(hb_cb);
#endif
}

/**
  * @brief  Initialize gap related parameters
  * @return void
  */
void bt_mesh_provisioner_multiple_profile_app_le_gap_init(void)
{
    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    uint16_t scan_window = 0x100; /* 160ms */
    uint16_t scan_interval = 0x120; /* 180ms */

    gap_sched_params_set(GAP_SCHED_PARAMS_INTERWAVE_SCAN_WINDOW, &scan_window, sizeof(scan_window));
    gap_sched_params_set(GAP_SCHED_PARAMS_INTERWAVE_SCAN_INTERVAL, &scan_interval, sizeof(scan_interval));
    gap_sched_params_set(GAP_SCHED_PARAMS_SCAN_WINDOW, &scan_window, sizeof(scan_window));
    gap_sched_params_set(GAP_SCHED_PARAMS_SCAN_INTERVAL, &scan_interval, sizeof(scan_interval));
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

#if F_BT_LE_5_0_SET_PHY_SUPPORT
    uint8_t phys_prefer = GAP_PHYS_PREFER_ALL;
    uint8_t tx_phys_prefer = GAP_PHYS_PREFER_1M_BIT;
    uint8_t rx_phys_prefer = GAP_PHYS_PREFER_1M_BIT;
    le_set_gap_param(GAP_PARAM_DEFAULT_PHYS_PREFER, sizeof(phys_prefer), &phys_prefer);
    le_set_gap_param(GAP_PARAM_DEFAULT_TX_PHYS_PREFER, sizeof(tx_phys_prefer), &tx_phys_prefer);
    le_set_gap_param(GAP_PARAM_DEFAULT_RX_PHYS_PREFER, sizeof(rx_phys_prefer), &rx_phys_prefer);
#endif

#if defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL
    {
        /* Device name and device appearance */
        uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_PERIPHERAL";
        uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
        uint8_t  slave_init_mtu_req = false;


        /* Advertising parameters */
        uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
        uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
        uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
        uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
        uint16_t adv_int_min = DEFAULT_ADVERTISING_INTERVAL_MIN;
        uint16_t adv_int_max = DEFAULT_ADVERTISING_INTERVAL_MAX;

        /* GAP Bond Manager parameters */
        uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
        uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
        uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#if F_BT_LE_SMP_OOB_SUPPORT
        uint8_t  auth_oob = false;
#endif
        uint8_t  auth_use_fix_passkey = false;
        uint32_t auth_fix_passkey = 0;
        uint8_t  auth_sec_req_enable = false;
        uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

        /* Set device name and device appearance */
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
        le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
        le_set_gap_param(GAP_PARAM_SLAVE_INIT_GATT_MTU_REQ, sizeof(slave_init_mtu_req),
                         &slave_init_mtu_req);
#if 1
        /* Set advertising parameters */
        le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
        le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
        le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
        le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
        le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
        le_adv_set_param(GAP_PARAM_ADV_DATA, array_count_of_adv_data, (void *)adv_data);
        // le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);
#endif
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
    }
#endif

#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
    {
        /* Device name and device appearance */
        uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_CENTRAL_CLIENT";
        uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

        /* Scan parameters */
        //uint8_t  scan_mode = GAP_SCAN_MODE_ACTIVE;
        //uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
        //uint16_t scan_window = DEFAULT_SCAN_WINDOW;
        //uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
        //uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

        /* GAP Bond Manager parameters */
        uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
        uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
        uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#if F_BT_LE_SMP_OOB_SUPPORT
        uint8_t  auth_oob = false;
#endif
        uint8_t  auth_use_fix_passkey = false;
        uint32_t auth_fix_passkey = 0;
        uint8_t  auth_sec_req_enable = false;
        uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

        /* Set device name and device appearance */
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
        le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);
#if 0
        /* Set scan parameters */
        le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
        le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
        le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
        le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                          &scan_filter_policy);
        le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                          &scan_filter_duplicate);
#endif

#if 1
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
#endif
#if F_BT_LE_USE_STATIC_RANDOM_ADDR
        T_APP_STATIC_RANDOM_ADDR random_addr;
        bool gen_addr = true;
        uint8_t local_bd_type = GAP_LOCAL_ADDR_LE_RANDOM;
        if (ble_central_app_load_static_random_address(&random_addr) == 0)
        {
            if (random_addr.is_exist == true)
            {
                gen_addr = false;
            }
        }
        if (gen_addr)
        {
            if (le_gen_rand_addr(GAP_RAND_ADDR_STATIC, random_addr.bd_addr) == GAP_CAUSE_SUCCESS)
            {
                random_addr.is_exist = true;
                ble_central_app_save_static_random_address(&random_addr);
            }
        }
        le_cfg_local_identity_address(random_addr.bd_addr, GAP_IDENT_ADDR_RAND);
        le_set_gap_param(GAP_PARAM_RANDOM_ADDR, 6, random_addr.bd_addr);
        //only for peripheral,broadcaster
        //le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
        //only for central,observer
#if 0
        le_scan_set_param(GAP_PARAM_SCAN_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
#endif
#endif
    }
#endif

#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
    {
        /* Device name and device appearance */
        uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_SCATTERNET";
        uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

        /* Advertising parameters */
        uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
        uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
        uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
        uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
        uint16_t adv_int_min = DEFAULT_ADVERTISING_INTERVAL_MIN;
        uint16_t adv_int_max = DEFAULT_ADVERTISING_INTERVAL_MAX;

        /* Scan parameters */
        //uint8_t  scan_mode = GAP_SCAN_MODE_ACTIVE;
        //uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
        //uint16_t scan_window = DEFAULT_SCAN_WINDOW;
        //uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
        //uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

        /* GAP Bond Manager parameters */
        uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
        uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
        uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#if F_BT_LE_SMP_OOB_SUPPORT
        uint8_t  auth_oob = false;
#endif
        uint8_t  auth_use_fix_passkey = false;
        uint32_t auth_fix_passkey = 0;
        uint8_t  auth_sec_req_enable = false;
        uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

        /* Set device name and device appearance */
        le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
        le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

    	/* Set advertising parameters */
    	le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    	le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    	le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    	le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    	le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    	le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    	le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);
    	le_adv_set_param(GAP_PARAM_ADV_DATA, array_count_of_adv_data, (void *)adv_data);
    	// le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data), (void *)scan_rsp_data);

#if 0
        /* Set scan parameters */
        le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
        le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
        le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
        le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                          &scan_filter_policy);
        le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                          &scan_filter_duplicate);
#endif
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

#if F_BT_LE_USE_STATIC_RANDOM_ADDR
    	T_APP_STATIC_RANDOM_ADDR random_addr;
    	bool gen_addr = true;
    	uint8_t local_bd_type = GAP_LOCAL_ADDR_LE_RANDOM;
    	if (ble_scatternet_app_load_static_random_address(&random_addr) == 0)
    	{
    		if (random_addr.is_exist == true)
    		{
    			gen_addr = false;
    		}
    	}
    	if (gen_addr)
    	{
    		if (le_gen_rand_addr(GAP_RAND_ADDR_STATIC, random_addr.bd_addr) == GAP_CAUSE_SUCCESS)
    		{
    			random_addr.is_exist = true;
    			ble_scatternet_app_save_static_random_address(&random_addr);
    		}
    	}
    	le_cfg_local_identity_address(random_addr.bd_addr, GAP_IDENT_ADDR_RAND);
    	le_set_gap_param(GAP_PARAM_RANDOM_ADDR, 6, random_addr.bd_addr);
    	//only for peripheral,broadcaster
    	le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
    	//only for central,observer
#if 0
    	le_scan_set_param(GAP_PARAM_SCAN_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
#endif
#endif
#if F_BT_GAPS_CHAR_WRITEABLE
    	uint8_t appearance_prop = GAPS_PROPERTY_WRITE_ENABLE;
    	uint8_t device_name_prop = GAPS_PROPERTY_WRITE_ENABLE;
    	T_LOCAL_APPEARANCE appearance_local;
    	T_LOCAL_NAME local_device_name;
    	if (flash_load_local_appearance(&appearance_local) == 0)
    	{
    		gaps_set_parameter(GAPS_PARAM_APPEARANCE, sizeof(uint16_t), &appearance_local.local_appearance);
    	}

    	if (flash_load_local_name(&local_device_name) == 0)
    	{
    		gaps_set_parameter(GAPS_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, local_device_name.local_name);
    	}
    	gaps_set_parameter(GAPS_PARAM_APPEARANCE_PROPERTY, sizeof(appearance_prop), &appearance_prop);
    	gaps_set_parameter(GAPS_PARAM_DEVICE_NAME_PROPERTY, sizeof(device_name_prop), &device_name_prop);
    	gatt_register_callback((void*)bt_mesh_scatternet_gap_service_callback);
#endif
    }
#endif

    vendor_cmd_init(app_vendor_callback);
    /* register gap message callback */
    le_register_app_cb(bt_mesh_provisioner_multiple_profile_app_gap_callback);
}

/**
 * @brief  Add GATT services, clients and register callbacks
 * @return void
 */
void bt_mesh_provisioner_multiple_profile_app_le_profile_init(void)
{
    server_init(MESH_GATT_SERVER_COUNT + 2);
    /* Add Server Module */

    /* Register Server Callback */
    server_register_app_cb(bt_mesh_provisioner_multiple_profile_app_profile_callback);

    client_init(MESH_GATT_CLIENT_COUNT + 3);
    /* Add Client Module */
    prov_client_add(bt_mesh_provisioner_multiple_profile_app_client_callback);
    proxy_client_add(bt_mesh_provisioner_multiple_profile_app_client_callback);

    /* Register Client Callback--App_ClientCallback to handle events from Profile Client layer. */
    client_register_general_client_cb(bt_mesh_provisioner_multiple_profile_app_client_callback);

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    bt_mesh_simp_srv_id = simp_ble_service_add_service((void *)bt_mesh_provisioner_multiple_profile_app_profile_callback);
    bt_mesh_bas_srv_id  = bas_add_service((void *)bt_mesh_provisioner_multiple_profile_app_profile_callback);
#endif
#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
    bt_mesh_central_gcs_client_id = gcs_add_client(bt_mesh_central_gcs_client_callback, BLE_CENTRAL_APP_MAX_LINKS, BLE_CENTRAL_APP_MAX_DISCOV_TABLE_NUM);
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
    bt_mesh_scatternet_gcs_client_id = gcs_add_client(bt_mesh_scatternet_gcs_client_callback, BLE_SCATTERNET_APP_MAX_LINKS, BLE_SCATTERNET_APP_MAX_DISCOV_TABLE_NUM);
#endif

}

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void bt_mesh_provisioner_multiple_profile_board_init(void)
{

}

/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
void bt_mesh_provisioner_multiple_profile_driver_init(void)
{

}

/**
 * @brief    Contains the power mode settings
 * @return   void
 */
void bt_mesh_provisioner_multiple_profile_pwr_mgr_init(void)
{
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Scatternet APP, thus only one APP task is init here
 * @return   void
 */
void bt_mesh_provisioner_multiple_profile_task_init(void)
{
    bt_mesh_provisioner_multiple_profile_app_task_init();
}

void bt_mesh_provisioner_multiple_profile_task_deinit(void)
{
    bt_mesh_provisioner_multiple_profile_app_task_deinit();
}

void bt_mesh_provisioner_multiple_profile_stack_config_init(void)
{
#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL 
    gap_config_max_le_link_num(BLE_CENTRAL_APP_MAX_LINKS);
    gap_config_max_le_paired_device(BLE_CENTRAL_APP_MAX_LINKS);
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
    gap_config_max_le_link_num(BLE_SCATTERNET_APP_MAX_LINKS);
    gap_config_max_le_paired_device(BLE_SCATTERNET_APP_MAX_LINKS);
#else
    gap_config_max_le_link_num(APP_MAX_LINKS);
    gap_config_max_le_paired_device(APP_MAX_LINKS);
#endif
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int bt_mesh_provisioner_multiple_profile_app_main(void)
{
	bt_trace_init();
#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    bt_mesh_provisioner_multiple_profile_stack_config_init();
#endif
    bte_init();
    bt_mesh_provisioner_multiple_profile_board_init();
    bt_mesh_provisioner_multiple_profile_driver_init();
#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL 
    le_gap_init(BLE_CENTRAL_APP_MAX_LINKS);
#elif defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL
    le_gap_init(APP_MAX_LINKS);
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
    le_gap_init(BLE_SCATTERNET_APP_MAX_LINKS);
#else
    le_gap_init(APP_MAX_LINKS);
#endif
    bt_mesh_provisioner_multiple_profile_app_le_gap_init();
    bt_mesh_provisioner_multiple_profile_app_le_profile_init();
    bt_mesh_provisioner_multiple_profile_stack_init();
    bt_mesh_provisioner_multiple_profile_pwr_mgr_init();
    bt_mesh_provisioner_multiple_profile_task_init();

    return 0;
}

extern void wifi_btcoex_set_bt_on(void);

int bt_mesh_provisioner_multiple_profile_app_init(void)
{
    int bt_stack_already_on = 0;
	T_GAP_DEV_STATE new_state;
	/*Wait WIFI init complete*/
	while(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
		os_delay(1000);
	}

	//judge BLE central is already on
	le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
		bt_stack_already_on = 1;
		printf("[BT Mesh Provisioner]BT Stack already on\n\r");
		return 0;
	}
	else
		bt_mesh_provisioner_multiple_profile_app_main();
	bt_coex_init();

	/*Wait BT init complete*/
	do {
		os_delay(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	}while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

	/*Start BT WIFI coexistence*/
	wifi_btcoex_set_bt_on();

    if (bt_stack_already_on) {
        uint8_t bt_addr[6];
        uint8_t net_key[16] = MESH_NET_KEY;
        uint8_t net_key1[16] = MESH_NET_KEY1;
        uint8_t app_key[16] = MESH_APP_KEY;
        uint8_t app_key1[16] = MESH_APP_KEY1;
        gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
        data_uart_debug("bt addr: 0x%02x%02x%02x%02x%02x%02x\r\n>",
                        bt_addr[5], bt_addr[4], bt_addr[3],
                        bt_addr[2], bt_addr[1], bt_addr[0]);

        /** configure provisioner */
        mesh_node.node_state = PROV_NODE;
        mesh_node.unicast_addr = bt_addr[0] % 99 + 1;
        memcpy(&net_key[10], bt_addr, sizeof(bt_addr));
        memcpy(&net_key1[10], bt_addr, sizeof(bt_addr));
        memcpy(&app_key[10], bt_addr, sizeof(bt_addr));
        memcpy(&app_key1[10], bt_addr, sizeof(bt_addr));
        uint16_t net_key_index = net_key_add(0, net_key);
        app_key_add(net_key_index, 0, app_key);
        uint8_t net_key_index1 = net_key_add(1, net_key1);
        app_key_add(net_key_index1, 1, app_key1);
    	mesh_model_bind_all_key();
    }

#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
    if (bt_mesh_provisioner_api_init()) {
        printf("[BT Mesh Provisioner] bt_mesh_provisioner_api_init fail ! \n\r");
        return 1;
    }
#endif

	return 0;
}

extern void mesh_deinit(void);
extern bool mesh_initial_state;
extern bool bt_trace_uninit(void);
#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
extern void gcs_delete_client(void);
#endif

extern T_GAP_DEV_STATE bt_mesh_provisioner_multiple_profile_gap_dev_state;

void bt_mesh_provisioner_multiple_profile_app_deinit(void)
{
    T_GAP_DEV_STATE new_state;

#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
	bt_mesh_provisioner_api_deinit();
#endif
    bt_mesh_provisioner_multiple_profile_task_deinit();
    le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	if (new_state.gap_init_state != GAP_INIT_STATE_STACK_READY) {
		printf("[BT Mesh Provisioner] BT Stack is not running\n\r");
        mesh_initial_state = FALSE;
        bt_mesh_provisioner_multiple_profile_gap_dev_state.gap_init_state =  GAP_INIT_STATE_INIT;
        return;
	}
#if F_BT_DEINIT
	else {
#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
        gcs_delete_client();
#endif
		bte_deinit();
		printf("[BT Mesh Provisioner] BT Stack deinitalized\n\r");
	}
#endif
    mesh_deinit();
    bt_trace_uninit();

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    plt_timer_delete(bt_mesh_multiple_profile_peripheral_adv_timer, 0xFFFFFFFF);
	bt_mesh_multiple_profile_peripheral_adv_timer = NULL;
#endif
    mesh_initial_state = FALSE;
	bt_mesh_provisioner_multiple_profile_gap_dev_state.gap_init_state =  GAP_INIT_STATE_INIT;
}


