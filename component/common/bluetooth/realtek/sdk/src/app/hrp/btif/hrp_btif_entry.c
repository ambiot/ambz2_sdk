#include <trace_app.h>
#include <hrp_btif_entry.h>
#include <stddef.h>
#include <hrp_btif_system_api.h>
#include <hrp_btif_common_api.h>
#include <hrp_btif_le_api.h>
#include <os_msg.h>
#if F_BT_DLPS_EN
#include <hrp_dlps.h>
#endif

#if F_BT_LE_BTIF_SUPPORT

extern uint8_t btif_get_up_data_len(T_BTIF_UP_MSG *p_msg);
extern T_L2C_DATA_TRANS l2c_data_trans;
T_GATT_DEV_INFO  t_gatt_dev_info = {0};
T_GATT_SRV_TABLE  t_srv_table = {0};

const P_HRP_BTIF_HANDLER hrp_btif_handle_system[] =
{
    hrp_btif_system_reset,              /* 0x00, HRP_SYSTEM_RESET_REQ */
    NULL,                               /* 0x01, HRP_SYSTEM_RESET_RSP*/
    hrp_btif_system_init,               /* 0x02, HRP_SYSTEM_INIT_REQ*/
    NULL,                               /* 0x03, HRP_SYSTEM_INIT_RSP*/
    NULL,      /* 0x04, HRP_SYSTEM_L2C_DATA_TEST_REQ*/
    NULL,                               /* 0x05, HRP_SYSTEM_L2C_DATA_TEST_CMPL_INFO*/
    hrp_btif_system_le_cfg_passkey_value,  /* 0x06, HRP_SYSTEM_LE_CFG_PASSKEY_VALUE_REQ*/
    NULL,                               /* 0x07, HRP_SYSTEM_LE_CFG_PASSKEY_VALUE_RSP*/
    hrp_btif_system_event_ack,          /* 0x08, HRP_SYSTEM_EVENT_ACK*/
    NULL,                               /* 0x09, HRP_SYSTEM_CMD_ACK*/
    hrp_btif_system_enable_dlps_req,    /* 0x0A, HRP_SYSTEM_ENABLE_DLPS_REQ*/
    NULL,                               /* 0x0B, HRP_SYSTEM_ENABLE_DLPS_RSP*/
    hrp_btif_system_read_dlps_count_req,/* 0x0C, HRP_SYSTEM_READ_DLPS_COUNT_REQ*/
    NULL,                               /* 0x0D, HRP_SYSTEM_READ_DLPS_COUNT_RSP*/

};

const P_HRP_BTIF_HANDLER hrp_btif_handle_common[] =
{
    NULL,                             /* 0x00 */
    hrp_register_req,                 /* BTIF_MSG_REGISTER_REQ = 0x01, */
    NULL,                             /* BTIF_MSG_REGISTER_RSP, */
    hrp_release_req,                  /* BTIF_MSG_RELEASE_REQ, */
    NULL,                             /* BTIF_MSG_RELEASE_RSP, */
    NULL,                             /* BTIF_MSG_ACT_INFO, */
    hrp_dev_cfg_req,                  /* BTIF_MSG_DEV_CFG_REQ, */
    NULL,                             /* BTIF_MSG_DEV_CFG_RSP, */

    NULL,                             /* BTIF_MSG_ACL_STATUS_INFO, */
    NULL,         /* BTIF_MSG_ACL_PRIORITY_SET_REQ, */
    NULL,                             /* BTIF_MSG_ACL_PRIORITY_SET_RSP, */

    hrp_read_rssi_req,                /* BTIF_MSG_READ_RSSI_REQ, */
    NULL,                             /* BTIF_MSG_READ_RSSI_RSP, */
    hrp_vendor_cmd_req,               /* BTIF_MSG_VENDOR_CMD_REQ, */
    NULL,                             /* BTIF_MSG_VENDOR_CMD_RSP, */
    NULL,                             /* BTIF_MSG_VENDOR_EVT_INFO, */

    hrp_pairable_mode_set_req,        /* BTIF_MSG_PAIRABLE_MODE_SET_REQ, */
    NULL,                             /* BTIF_MSG_PAIRABLE_MODE_SET_RSP, */
    NULL,                             /* BTIF_MSG_USER_PASSKEY_REQ_IND, */
    hrp_user_passkey_req_cfm,         /* BTIF_MSG_USER_PASSKEY_REQ_CFM, */
    hrp_user_passkey_req_reply_req,   /* BTIF_MSG_USER_PASSKEY_REQ_REPLY_REQ, */
    NULL,                             /* BTIF_MSG_USER_PASSKEY_REQ_REPLY_RSP, */
    NULL,                             /* BTIF_MSG_USER_PASSKEY_NOTIF_INFO, */
    NULL,                             /* BTIF_MSG_AUTHEN_RESULT_IND, */
    hrp_authen_result_cfm,            /* BTIF_MSG_AUTHEN_RESULT_CFM, */
    NULL,                             /* BTIF_MSG_AUTHEN_KEY_REQ_IND, */
    hrp_authen_key_req_cfm,           /* BTIF_MSG_AUTHEN_KEY_REQ_CFM, */
    NULL,                             /* BTIF_MSG_USER_CFM_REQ_IND, */
    hrp_user_cfm_req_cfm,             /* BTIF_MSG_USER_CFM_REQ_CFM, */
#if F_BT_KEY_PRESS_SUPPORT
    hrp_keypress_notif_req,           /* BTIF_MSG_KEYPRESS_NOTIF_REQ, */
#else
    NULL,
#endif
    NULL,                             /* BTIF_MSG_KEYPRESS_NOTIF_RSP, */
    NULL,                             /* BTIF_MSG_KEYPRESS_NOTIF_INFO, */
    NULL,                             /* BTIF_MSG_HW_ERROR_INFO, */
};

const P_HRP_BTIF_HANDLER hrp_btif_handle_ble[] =
{
    NULL,                               //BTIF_MSG_REMOTE_OOB_REQ_IND = 0x0100,
#if F_BT_LE_SMP_OOB_SUPPORT
    hrp_remote_oob_req_cfm,                               // BTIF_MSG_REMOTE_OOB_REQ_CFM,
#else
    NULL,
#endif
    hrp_gatt_srv_reg_req,               // BTIF_MSG_GATT_SRV_REG_REQ,
    NULL,                               // BTIF_MSG_GATT_SRV_REG_RSP,
    hrp_gatt_attr_update_req,            // BTIF_MSG_GATT_ATTR_UPDATE_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_UPDATE_RSP,
    NULL,                               // BTIF_MSG_GATT_ATTR_UPDATE_STATUS_IND,
    hrp_gatt_attr_update_status_cfm,                               // BTIF_MSG_GATT_ATTR_UPDATE_STATUS_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_READ_IND,
    hrp_gatt_attr_read_cfm,                               // BTIF_MSG_GATT_ATTR_READ_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_WRITE_REQ_IND,
    hrp_gatt_attr_write_req_cfm,                               // BTIF_MSG_GATT_ATTR_WRITE_REQ_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_PREP_WRITE_IND,
    hrp_gatt_attr_prep_write_cfm,                               // BTIF_MSG_GATT_ATTR_PREP_WRITE_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_EXEC_WRITE_IND,
    hrp_gatt_attr_exec_write_cfm,                               // BTIF_MSG_GATT_ATTR_EXEC_WRITE_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_WRITE_CMD_INFO,//0x110
    NULL,                               // BTIF_MSG_GATT_ATTR_CCCD_INFO,
#if F_BT_LE_GATT_CLIENT_SUPPORT
    hrp_gatt_discovery_req,                               // BTIF_MSG_GATT_DISCOVERY_REQ,
    NULL,                               // BTIF_MSG_GATT_DISCOVERY_RSP,
    NULL,                               // BTIF_MSG_GATT_DISCOVERY_IND,
    hrp_gatt_discovery_cfm,                               // BTIF_MSG_GATT_DISCOVERY_CFM,
    hrp_gatt_attr_read_req,                               // BTIF_MSG_GATT_ATTR_READ_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_READ_RSP,
#if F_BT_LE_ATT_READ_MULTI_SUPPORT
    hrp_gatt_attr_read_multi_req,                               // BTIF_MSG_GATT_ATTR_READ_MULTI_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_GATT_ATTR_READ_MULTI_RSP,
    hrp_gatt_attr_write_req,                               // BTIF_MSG_GATT_ATTR_WRITE_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_WRITE_RSP,
    hrp_gatt_attr_prep_write_req,                               // BTIF_MSG_GATT_ATTR_PREP_WRITE_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_PREP_WRITE_RSP,
    hrp_gatt_attr_exec_write_req,                               // BTIF_MSG_GATT_ATTR_EXEC_WRITE_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_EXEC_WRITE_RSP,

    NULL,                               // BTIF_MSG_GATT_ATTR_IND,//0x120
    hrp_gatt_attr_cfm,                               // BTIF_MSG_GATT_ATTR_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_NOTIF_INFO,
#else
    NULL,                               // BTIF_MSG_GATT_DISCOVERY_REQ,
    NULL,                               // BTIF_MSG_GATT_DISCOVERY_RSP,
    NULL,                               // BTIF_MSG_GATT_DISCOVERY_IND,
    NULL,                               // BTIF_MSG_GATT_DISCOVERY_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_READ_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_READ_RSP,
#if F_BT_LE_ATT_READ_MULTI_SUPPORT
    NULL,                               // BTIF_MSG_GATT_ATTR_READ_MULTI_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_GATT_ATTR_READ_MULTI_RSP,
    NULL,                               // BTIF_MSG_GATT_ATTR_WRITE_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_WRITE_RSP,
    NULL,                               // BTIF_MSG_GATT_ATTR_PREP_WRITE_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_PREP_WRITE_RSP,
    NULL,                               // BTIF_MSG_GATT_ATTR_EXEC_WRITE_REQ,
    NULL,                               // BTIF_MSG_GATT_ATTR_EXEC_WRITE_RSP,

    NULL,                               // BTIF_MSG_GATT_ATTR_IND,//0x120
    NULL,                               // BTIF_MSG_GATT_ATTR_CFM,
    NULL,                               // BTIF_MSG_GATT_ATTR_NOTIF_INFO,
#endif
    hrp_gatt_security_req,                               // BTIF_MSG_GATT_SECURITY_REQ,
    NULL,                               // BTIF_MSG_GATT_SECURITY_RSP,
    NULL,                               // BTIF_MSG_GATT_SERVER_STORE_IND,
    hrp_gatt_server_store_cfm,                               // BTIF_MSG_GATT_SERVER_STORE_CFM,
    NULL,                               // BTIF_MSG_GATT_MTU_SIZE_INFO,
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    hrp_le_conn_req,                               // BTIF_MSG_LE_CONN_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_CONN_RSP,
    NULL,                               // BTIF_MSG_LE_CONN_IND,
    hrp_le_conn_cfm,                               // BTIF_MSG_LE_CONN_CFM,
    NULL,                               // BTIF_MSG_LE_CONN_CMPL_INFO,
    hrp_le_disconn_req,                               // BTIF_MSG_LE_DISCONN_REQ,
    NULL,                               // BTIF_MSG_LE_DISCONN_RSP,
    NULL,                               // BTIF_MSG_LE_DISCONN_IND,
    hrp_le_disconn_cfm,                               // BTIF_MSG_LE_DISCONN_CFM,//0x130

    hrp_le_adv_req,                               // BTIF_MSG_LE_ADV_REQ,
    NULL,                               // BTIF_MSG_LE_ADV_RSP,
    hrp_le_adv_param_set_req,                               // BTIF_MSG_LE_ADV_PARAM_SET_REQ,
    NULL,                               // BTIF_MSG_LE_ADV_PARAM_SET_RSP,
    hrp_le_adv_data_set_req,                               // BTIF_MSG_LE_ADV_DATA_SET_REQ,
    NULL,                               // BTIF_MSG_LE_ADV_DATA_SET_RSP,
    hrp_le_scan_req,                               // BTIF_MSG_LE_SCAN_REQ,
    NULL,                               // BTIF_MSG_LE_SCAN_RSP,
    hrp_le_scan_param_set_req,                               // BTIF_MSG_LE_SCAN_PARAM_SET_REQ,
    NULL,                               // BTIF_MSG_LE_SCAN_PARAM_SET_RSP,
    NULL,                               // BTIF_MSG_LE_SCAN_INFO,
    hrp_le_modify_white_list_req,                               // BTIF_MSG_LE_MODIFY_WHITE_LIST_REQ,
    NULL,                               // BTIF_MSG_LE_MODIFY_WHITE_LIST_RSP,
    hrp_le_conn_update_req,                               // BTIF_MSG_LE_CONN_UPDATE_REQ,
    NULL,                               // BTIF_MSG_LE_CONN_UPDATE_RSP,
    NULL,                               // BTIF_MSG_LE_CONN_UPDATE_IND,//0x140
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    hrp_le_conn_update_cfm,                               // BTIF_MSG_LE_CONN_UPDATE_CFM,
#else
    NULL,                               // BTIF_MSG_LE_CONN_UPDATE_CFM,
#endif
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_INFO,
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_REQ_EVT_INFO,
#if F_BT_LE_4_2_CONN_PARAM_UPDATE_SUPPORT
    hrp_le_conn_param_req_reply_req,                               // BTIF_MSG_LE_CONN_PARAM_REQ_REPLY_REQ,
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_REQ_REPLY_RSP,
    hrp_le_conn_param_req_neg_reply_req,                               // BTIF_MSG_LE_CONN_PARAM_REQ_NEG_REPLY_REQ,
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_REQ_NEG_REPLY_RSP,
#else
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_REQ_REPLY_REQ,
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_REQ_REPLY_RSP,
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_REQ_NEG_REPLY_REQ,
    NULL,                               // BTIF_MSG_LE_CONN_PARAM_REQ_NEG_REPLY_RSP,
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
    hrp_le_credit_based_conn_req,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_IND,
    hrp_le_credit_based_conn_cfm,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_CFM,
    hrp_le_credit_based_disconn_req,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_IND,
    hrp_le_credit_based_disconn_cfm,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_CFM,
    hrp_le_send_credit_req,                               // BTIF_MSG_LE_SEND_CREDIT_REQ,//0x150
    NULL,                               // BTIF_MSG_LE_SEND_CREDIT_RSP,
    hrp_le_credit_based_data_req,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_IND,
    hrp_le_credit_based_data_cfm,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_CFM,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_CMPL_INFO,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_CREDIT_INFO,
    hrp_le_credit_based_security_reg_req,                               // BTIF_MSG_LE_CREDIT_BASED_SECURITY_REG_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_SECURITY_REG_RSP,
    hrp_le_credit_based_psm_reg_req,                               // BTIF_MSG_LE_CREDIT_BASED_PSM_REG_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_PSM_REG_RSP,
#else
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_IND,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_CFM,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_IND,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DISCONN_CFM,
    NULL,                               // BTIF_MSG_LE_SEND_CREDIT_REQ,//0x150
    NULL,                               // BTIF_MSG_LE_SEND_CREDIT_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_IND,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_DATA_CFM,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_CMPL_INFO,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_CONN_CREDIT_INFO,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_SECURITY_REG_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_SECURITY_REG_RSP,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_PSM_REG_REQ,
    NULL,                               // BTIF_MSG_LE_CREDIT_BASED_PSM_REG_RSP,
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    hrp_le_set_chann_classif_req,                               // BTIF_MSG_LE_SET_CHANN_CLASSIF_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_SET_CHANN_CLASSIF_RSP,
#if F_BT_LE_READ_CHANN_MAP
    hrp_le_read_chann_map_req,                               // BTIF_MSG_LE_READ_CHANN_MAP_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_READ_CHANN_MAP_RSP,
#if F_BT_LE_4_0_DTM_SUPPORT
    hrp_le_receiver_test_req,                               // BTIF_MSG_LE_RECEIVER_TEST_REQ,
    NULL,                               // BTIF_MSG_LE_RECEIVER_TEST_RSP,
    hrp_le_transmitter_test_req,                               // BTIF_MSG_LE_TRANSMITTER_TEST_REQ,//0x162
    NULL,                               // BTIF_MSG_LE_TRANSMITTER_TEST_RSP,
    hrp_le_test_end_req,                               // BTIF_MSG_LE_TEST_END_REQ,
    NULL,                               // BTIF_MSG_LE_TEST_END_RSP,
#else
    NULL,                               // BTIF_MSG_LE_RECEIVER_TEST_REQ,
    NULL,                               // BTIF_MSG_LE_RECEIVER_TEST_RSP,
    NULL,                               // BTIF_MSG_LE_TRANSMITTER_TEST_REQ,//0x162
    NULL,                               // BTIF_MSG_LE_TRANSMITTER_TEST_RSP,
    NULL,                               // BTIF_MSG_LE_TEST_END_REQ,
    NULL,                               // BTIF_MSG_LE_TEST_END_RSP,
#endif
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
    hrp_le_read_adv_tx_power_req,                               // BTIF_MSG_LE_READ_ADV_TX_POWER_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_READ_ADV_TX_POWER_RSP,
    hrp_le_set_rand_addr_req,                               // BTIF_MSG_LE_SET_RAND_ADDR_REQ,
    NULL,                               // BTIF_MSG_LE_SET_RAND_ADDR_RSP,
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    hrp_le_read_max_data_len_req,                               // BTIF_MSG_LE_READ_MAX_DATA_LEN_REQ,
    NULL,                               // BTIF_MSG_LE_READ_MAX_DATA_LEN_RSP,
    hrp_le_read_default_data_len_req,                               // BTIF_MSG_LE_READ_DEFAULT_DATA_LEN_REQ,
    NULL,                               // BTIF_MSG_LE_READ_DEFAULT_DATA_LEN_RSP,
    hrp_le_write_default_data_len_req,                               // BTIF_MSG_LE_WRITE_DEFAULT_DATA_LEN_REQ,
    NULL,                               // BTIF_MSG_LE_WRITE_DEFAULT_DATA_LEN_RSP,
    hrp_le_set_data_len_req,                               // BTIF_MSG_LE_SET_DATA_LEN_REQ,//0x172
#else
    NULL,                               // BTIF_MSG_LE_READ_MAX_DATA_LEN_REQ,
    NULL,                               // BTIF_MSG_LE_READ_MAX_DATA_LEN_RSP,
    NULL,                               // BTIF_MSG_LE_READ_DEFAULT_DATA_LEN_REQ,
    NULL,                               // BTIF_MSG_LE_READ_DEFAULT_DATA_LEN_RSP,
    NULL,                               // BTIF_MSG_LE_WRITE_DEFAULT_DATA_LEN_REQ,
    NULL,                               // BTIF_MSG_LE_WRITE_DEFAULT_DATA_LEN_RSP,
    NULL,                               // BTIF_MSG_LE_SET_DATA_LEN_REQ,//0x172
#endif
    NULL,                               // BTIF_MSG_LE_SET_DATA_LEN_RSP,
    NULL,                               // BTIF_MSG_LE_DATA_LEN_CHANGE_INFO,
#if F_BT_LE_PRIVACY_SUPPORT
    hrp_le_modify_resolv_list_req,                               // BTIF_MSG_LE_MODIFY_RESOLV_LIST_REQ,
    NULL,                               // BTIF_MSG_LE_MODIFY_RESOLV_LIST_RSP,
    hrp_le_read_peer_resolv_addr_req,                               // BTIF_MSG_LE_READ_PEER_RESOLV_ADDR_REQ,
    NULL,                               // BTIF_MSG_LE_READ_PEER_RESOLV_ADDR_RSP,
    hrp_le_read_local_resolv_addr_req,                               // BTIF_MSG_LE_READ_LOCAL_RESOLV_ADDR_REQ,
    NULL,                               // BTIF_MSG_LE_READ_LOCAL_RESOLV_ADDR_RSP,
    hrp_le_set_resolution_enable_req,                               // BTIF_MSG_LE_SET_RESOLUTION_ENABLE_REQ,
    NULL,                               // BTIF_MSG_LE_SET_RESOLUTION_ENABLE_RSP,
    hrp_le_set_resolv_priv_addr_tout_req,                               // BTIF_MSG_LE_SET_RESOLV_PRIV_ADDR_TOUT_REQ,
    NULL,                               // BTIF_MSG_LE_SET_RESOLV_PRIV_ADDR_TOUT_RSP,
#else
    NULL,                               // BTIF_MSG_LE_MODIFY_RESOLV_LIST_REQ,
    NULL,                               // BTIF_MSG_LE_MODIFY_RESOLV_LIST_RSP,
    NULL,                               // BTIF_MSG_LE_READ_PEER_RESOLV_ADDR_REQ,
    NULL,                               // BTIF_MSG_LE_READ_PEER_RESOLV_ADDR_RSP,
    NULL,                               // BTIF_MSG_LE_READ_LOCAL_RESOLV_ADDR_REQ,
    NULL,                               // BTIF_MSG_LE_READ_LOCAL_RESOLV_ADDR_RSP,
    NULL,                               // BTIF_MSG_LE_SET_RESOLUTION_ENABLE_REQ,
    NULL,                               // BTIF_MSG_LE_SET_RESOLUTION_ENABLE_RSP,
    NULL,                               // BTIF_MSG_LE_SET_RESOLV_PRIV_ADDR_TOUT_REQ,
    NULL,                               // BTIF_MSG_LE_SET_RESOLV_PRIV_ADDR_TOUT_RSP,
#endif
#if F_BT_LE_LOCAL_IRK_SETTING_SUPPORT
    hrp_le_config_local_irk_req,                               // BTIF_MSG_LE_CONFIG_LOCAL_IRK_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_CONFIG_LOCAL_IRK_RSP,
#if F_BT_LE_PRIVACY_SUPPORT
    hrp_le_set_privacy_mode_req,                               // BTIF_MSG_LE_SET_PRIVACY_MODE_REQ,
#else
    NULL,                               // BTIF_MSG_LE_SET_PRIVACY_MODE_REQ,
#endif
    NULL,                               // BTIF_MSG_LE_SET_PRIVACY_MODE_RSP,//0x182
    NULL,                               // BTIF_MSG_LE_DIRECT_ADV_INFO,
    NULL,                               // BTIF_MSG_LE_HIGH_DUTY_ADV_TOUT_INFO,
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    hrp_le_set_adv_set_rand_addr_req,                               // BTIF_MSG_LE_SET_ADV_SET_RAND_ADDR_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_SET_ADV_SET_RAND_ADDR_RSP,
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    hrp_le_ext_adv_param_set_req,                               // BTIF_MSG_LE_EXT_ADV_PARAM_SET_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_EXT_ADV_PARAM_SET_RSP,
#if  F_BT_LE_5_0_AE_ADV_SUPPORT
    hrp_le_ext_adv_data_set_req,                               // BTIF_MSG_LE_EXT_ADV_DATA_SET_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_EXT_ADV_DATA_SET_RSP,
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    hrp_le_ext_adv_enable_req,                               // BTIF_MSG_LE_EXT_ADV_ENABLE_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_EXT_ADV_ENABLE_RSP,
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
    hrp_le_ext_scan_param_set_req,                               // BTIF_MSG_LE_EXT_SCAN_PARAM_SET_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_EXT_SCAN_PARAM_SET_RSP,
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
    hrp_le_ext_scan_req,                               // BTIF_MSG_LE_EXT_SCAN_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_EXT_SCAN_RSP,//0x190
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
    hrp_le_set_default_phy_req,                               // BTIF_MSG_LE_SET_DEFAULT_PHY_REQ,
    NULL,                               // BTIF_MSG_LE_SET_DEFAULT_PHY_RSP,
    hrp_le_set_phy_req,                               // BTIF_MSG_LE_SET_PHY_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PHY_RSP,
#else
    NULL,                               // BTIF_MSG_LE_SET_DEFAULT_PHY_REQ,
    NULL,                               // BTIF_MSG_LE_SET_DEFAULT_PHY_RSP,
    NULL,                               // BTIF_MSG_LE_SET_PHY_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PHY_RSP,
#endif
#if F_BT_LE_5_0_DTM_SUPPORT
    hrp_le_enhanced_receiver_test_req,                               // BTIF_MSG_LE_ENHANCED_RECEIVER_TEST_REQ,
    NULL,                               // BTIF_MSG_LE_ENHANCED_RECEIVER_TEST_RSP,
    hrp_le_enhanced_transmitter_test_req,                               // BTIF_MSG_LE_ENHANCED_TRANSMITTER_TEST_REQ,
    NULL,                               // BTIF_MSG_LE_ENHANCED_TRANSMITTER_TEST_RSP,
#else
    NULL,                               // BTIF_MSG_LE_ENHANCED_RECEIVER_TEST_REQ,
    NULL,                               // BTIF_MSG_LE_ENHANCED_RECEIVER_TEST_RSP,
    NULL,                               // BTIF_MSG_LE_ENHANCED_TRANSMITTER_TEST_REQ,
    NULL,                               // BTIF_MSG_LE_ENHANCED_TRANSMITTER_TEST_RSP,
#endif
    hrp_le_modify_periodic_adv_list_req,                               // BTIF_MSG_LE_MODIFY_PERIODIC_ADV_LIST_REQ,
    NULL,                               // BTIF_MSG_LE_MODIFY_PERIODIC_ADV_LIST_RSP,
#if F_BT_LE_5_0_RF_PATH_SUPPORT
    hrp_le_read_rf_path_compensation_req,                               // BTIF_MSG_LE_READ_RF_PATH_COMPENSATION_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_READ_RF_PATH_COMPENSATION_RSP,
#if F_BT_LE_5_0_RF_PATH_SUPPORT
    hrp_le_write_rf_path_compensation_req,                               // BTIF_MSG_LE_WRITE_RF_PATH_COMPENSATION_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_WRITE_RF_PATH_COMPENSATION_RSP,
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    hrp_le_modify_adv_set_req,                               // BTIF_MSG_LE_MODIFY_ADV_SET_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_MODIFY_ADV_SET_RSP,//0x1A0
#if F_BT_LE_5_0_PERIODIC_ADV_SUPPORT
    hrp_le_set_periodic_adv_param_req,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_PARAM_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_PARAM_RSP,
    hrp_le_set_periodic_adv_data_req,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_DATA_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_DATA_RSP,
    hrp_le_set_periodic_adv_enable_req,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_ENABLE_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_ENABLE_RSP,
    hrp_le_periodic_adv_create_sync_req,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_REQ,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_RSP,
    hrp_le_periodic_adv_create_sync_cancel_req,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_CANCEL_REQ,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_CANCEL_RSP,
    hrp_le_periodic_adv_terminate_sync_req,                               // BTIF_MSG_LE_PERIODIC_ADV_TERMINATE_SYNC_REQ,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_TERMINATE_SYNC_RSP,
#else
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_PARAM_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_PARAM_RSP,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_DATA_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_DATA_RSP,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_ENABLE_REQ,
    NULL,                               // BTIF_MSG_LE_SET_PERIODIC_ADV_ENABLE_RSP,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_REQ,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_RSP,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_CANCEL_REQ,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_CREATE_SYNC_CANCEL_RSP,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_TERMINATE_SYNC_REQ,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_TERMINATE_SYNC_RSP,
#endif
    NULL,                               // BTIF_MSG_LE_PHY_UPDATE_INFO,
    NULL,                               // BTIF_MSG_LE_EXT_ADV_REPORT_INFO,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_SYNC_ESTABLISHED_INFO,
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_REPORT_INFO,//0x1B0
    NULL,                               // BTIF_MSG_LE_PERIODIC_ADV_SYNC_LOST_INFO,
    NULL,                               // BTIF_MSG_LE_SCAN_TIMEOUT_INFO,
    NULL,                               // BTIF_MSG_LE_ADV_SET_TERMINATED_INFO,
    NULL,                               // BTIF_MSG_LE_SCAN_REQ_RECEIVED_INFO,
    NULL,                               // BTIF_MSG_LE_LOCAL_DEV_INFO,
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    hrp_le_enable_ext_adv_mode_req,                               // BTIF_MSG_LE_ENABLE_EXT_ADV_MODE_REQ,
#else
    NULL,
#endif
    NULL,                               // BTIF_MSG_LE_ENABLE_EXT_ADV_MODE_RSP,
    NULL,                               // BTIF_MSG_LE_REMOTE_FEATS_INFO,
    NULL,                               // BTIF_MSG_GATT_ATTR_WRITE_CMD_SIGNED_INFO,
    NULL,                               // BTIF_MSG_LE_SIGNED_STATUS_INFO,
    NULL,                               // BTIF_MSG_JUST_WORK_REQ_IND,
    hrp_just_work_req_cfm,                               // BTIF_MSG_JUST_WORK_REQ_CFM,
    NULL,                               // BTIF_MSG_USER_PASSKEY_NOTIF_IND,
    hrp_user_passkey_notif_cfm,                               // BTIF_MSG_USER_PASSKEY_NOTIF_CFM,
};



void hrp_btif_handle_req(uint8_t cmd_group, uint16_t cmd_index,
                         uint16_t param_list_len, uint8_t *p_param_list)
{
    P_HRP_BTIF_HANDLER p_handle_req = NULL;

    if ((system_status == HRP_STATUS_ACTIVE) && (active_module !=  HRP_MODULE_UPPER_STACK))
    {
        APP_PRINT_ERROR2("wrong module status: system_status= %d  active_module= %d",
                         system_status, active_module);
        return ;
    }

#if F_BT_DLPS_EN
    if (hrp_dlps_status == HRP_DLPS_STATUS_ACTIVED)
    {
        if (!((cmd_group == HRP_BTIF_CMD_GROUP_SYSTEM) && (cmd_index == HRP_SYSTEM_EVENT_ACK)))
        {
            hrp_system_send_cmd_ack(cmd_group, cmd_index, HRP_SYSTEM_COMMAND_COMPLETE);
        }
        hrp_dlps_allow_enter(true);
        APP_PRINT_INFO0("hrp_btif_handle_req allow enter dlps");
    }

#endif

    switch (cmd_group)
    {
    case HRP_BTIF_CMD_GROUP_STACK:
        if (cmd_index >= 0x1000)
        {
            APP_PRINT_ERROR1("HRP_BTIF_CMD_GROUP_STACK: wrong cmd_index! cmd_index = 0x%x",
                             cmd_index);
            return;
        }
        if (cmd_index < (sizeof(hrp_btif_handle_common) / sizeof(P_HRP_BTIF_HANDLER)))
        {
            p_handle_req = hrp_btif_handle_common[cmd_index];
        }
        else if (cmd_index >= 0x0100 &&
                 cmd_index < (0x0100 + sizeof(hrp_btif_handle_ble) / sizeof(P_HRP_BTIF_HANDLER)))
        {
            p_handle_req = hrp_btif_handle_ble[cmd_index - 0x0100];
        }
        break;
    case HRP_BTIF_CMD_GROUP_SYSTEM:
        if (cmd_index >= 0x0d)
        {
            APP_PRINT_ERROR1("HRP_BTIF_CMD_GROUP_SYSTEM: wrong cmd_index! cmd_index = 0x%x",
                             cmd_index);
            return;
        }
        p_handle_req = hrp_btif_handle_system[cmd_index];
        break;

    default:
        p_handle_req = NULL;
        break;
    }

    if (p_handle_req != NULL)
    {
        p_handle_req(param_list_len, p_param_list);
    }
    else
    {
        APP_PRINT_ERROR2("the function handle is NULL, cmd_group = 0x%x, cmd_index = 0x%x",
                         cmd_group, cmd_index);
    }
}

/**
@
@
@return : true - need send msg to uart, false - no need to send data to uart
*/
bool hrp_btif_handle_special_msg(uint8_t *cmd_group, uint16_t *cmd_index,
                                 T_BTIF_UP_MSG_DATA *p_param_list, uint16_t *param_list_len)
{
    uint8_t *p_param = (uint8_t *)p_param_list;
    switch (*cmd_index)
    {
    case BTIF_MSG_SDP_ATTR_IND:
        {
            T_BTIF_SDP_ATTR_IND param = *((T_BTIF_SDP_ATTR_IND *)p_param_list);
            uint16_t pos = 0;

            APP_PRINT_INFO1("sizeof(T_BTIF_SDP_ATTR_IND = %d", sizeof(T_BTIF_SDP_ATTR_IND));

            memcpy(p_param, param.bd_addr, 6); pos += 6;
            LE_UINT8_TO_ARRAY(p_param + pos, param.server_channel); pos ++;
            LE_UINT8_TO_ARRAY(p_param + pos, param.supported_repos); pos++;
            LE_UINT16_TO_ARRAY(p_param + pos, param.l2c_psm); pos += 2;
            LE_UINT8_TO_ARRAY(p_param + pos, param.srv_class_uuid_type); pos++;

            switch (param.srv_class_uuid_type)
            {
            case BTIF_UUID16:
                LE_UINT16_TO_ARRAY(p_param + pos, param.srv_class_uuid_data.uuid_16);
                pos += 2;
                break;
            case BTIF_UUID32:
                LE_UINT32_TO_ARRAY(p_param + pos, param.srv_class_uuid_data.uuid_32);
                pos += 4;
                break;
            case BTIF_UUID128:
                memcpy(p_param + pos, param.srv_class_uuid_data.uuid_128, 16);
                pos += 16;
                break;
            default:
                break;
            }

            LE_UINT16_TO_ARRAY(p_param + pos, param.remote_version); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.supported_feat); pos += 2;
            if (param.p_ext_data != NULL)
            {
                uint16_t len = sizeof(param.p_ext_data);    /* Note: p_param may not enough */
                memcpy(p_param + pos, param.p_ext_data, len); pos += len;
                *param_list_len = pos;
            }
            else
            {
                *param_list_len = pos;
            }
        }
        break;

    case BTIF_MSG_L2C_DATA_IND:
        {
            T_BTIF_L2C_DATA_IND data_ind = *((T_BTIF_L2C_DATA_IND *)p_param_list);

            *param_list_len = offsetof(T_BTIF_L2C_DATA_IND, data) + data_ind.gap + data_ind.length;
            if (l2c_data_trans.is_testing)
            {
                bool dataRight = true;
                //check data


                //update data
                if (dataRight)
                {
                    l2c_data_trans.rx_rcved_count++;
                    l2c_data_trans.rx_next_byte = ((uint8_t *)p_param_list)[*param_list_len - 1] + 1;
                }
                else
                {
                    //data test cmpl info    cause = wrong data
                }
                if (l2c_data_trans.rx_rcved_count == l2c_data_trans.rx_total_count)
                {
                    *cmd_group = HRP_BTIF_CMD_GROUP_SYSTEM;
                    *cmd_index = HRP_SYSTEM_L2C_DATA_TEST_CMPL_INFO;
                    //uint8_t *p_buffer = p_param_list;
                    uint16_t pos = 0;
                    LE_UINT16_TO_ARRAY(p_param + pos, 0); pos += 2; /* cause */
                    LE_UINT16_TO_ARRAY(p_param + pos, l2c_data_trans.rx_rcved_count); pos += 2; /* rx_count */
                    LE_UINT16_TO_ARRAY(p_param + pos, l2c_data_trans.tx_sent_count); pos += 2; /* tx_count */
                    *param_list_len = 6;
                    return true;
                }
                return false;
            }
        }
        break;

    case BTIF_MSG_L2C_DATA_RSP:
        {
            uint8_t event = LTP_EVENT_BTIF_L2C_DATA_RSP;

            if (!l2c_data_trans.is_testing)
            {
                return false;
            }

            l2c_data_trans.tx_available_credits++;
            if (l2c_data_trans.tx_sent_count >= l2c_data_trans.tx_total_count)
            {
                *cmd_group = HRP_BTIF_CMD_GROUP_SYSTEM;
                *cmd_index = HRP_SYSTEM_L2C_DATA_TEST_CMPL_INFO;
                uint16_t pos = 0;
                LE_UINT16_TO_ARRAY(p_param + pos, 0); pos += 2; /* cause */
                LE_UINT16_TO_ARRAY(p_param + pos, l2c_data_trans.rx_rcved_count); pos += 2; /* rx_count */
                LE_UINT16_TO_ARRAY(p_param + pos, l2c_data_trans.tx_sent_count); pos += 2; /* tx_count */
                *param_list_len = 6;
                l2c_data_trans.is_testing = false;
                return true;
            }
            else
            {
                if (os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleEvent, &event, 0) == false)
                {
                    APP_PRINT_ERROR0("hrp_btif_handle_special_msg:BTIF_MSG_L2C_DATA_RSP Send msg error");
                }
            }
            return false;
        }

    case BTIF_MSG_LE_SCAN_INFO:
        {
            T_BTIF_LE_SCAN_INFO param = *((T_BTIF_LE_SCAN_INFO *)p_param_list);
            uint16_t pos = 0;

            memcpy(p_param + pos, param.bd_addr, 6); pos += 6;
            LE_UINT8_TO_ARRAY(p_param + pos, param.remote_addr_type);
            pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.adv_type);
            pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.rssi);
            pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.data_len);
            pos += 1;
            memcpy(p_param + pos, &(param.data), param.data_len);
            pos += param.data_len;
            *param_list_len = pos;
            break;
        }
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    case BTIF_MSG_LE_EXT_ADV_REPORT_INFO:
        {
            T_BTIF_LE_EXT_ADV_REPORT_INFO param = *((T_BTIF_LE_EXT_ADV_REPORT_INFO *)p_param_list);
            uint16_t pos = 0;
            LE_UINT16_TO_ARRAY(p_param + pos, param.event_type);
            pos += 2;
            LE_UINT8_TO_ARRAY(p_param + pos, param.data_status);
            pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.addr_type);
            pos += 1;

            memcpy(p_param + pos, param.bd_addr, 6); pos += 6;

            LE_UINT8_TO_ARRAY(p_param + pos, param.primary_phy);
            pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.secondary_phy);
            pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.adv_sid);
            pos += 1;

            LE_UINT8_TO_ARRAY(p_param + pos, param.tx_power);
            pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.rssi);
            pos += 1;

            LE_UINT16_TO_ARRAY(p_param + pos, param.peri_adv_interval);
            pos += 2;
            LE_UINT8_TO_ARRAY(p_param + pos, param.direct_addr_type);
            pos += 1;
            memcpy(p_param + pos, param.direct_addr, 6); pos += 6;
            LE_UINT8_TO_ARRAY(p_param + pos, param.data_len);
            pos += 1;
            memcpy(p_param + pos, param.p_data, param.data_len);

            pos += param.data_len;

            *param_list_len = pos;
            break;
        }
#endif
    case BTIF_MSG_GATT_ATTR_CCCD_INFO:
        {
            T_BTIF_GATT_ATTR_CCCD_INFO param = *((T_BTIF_GATT_ATTR_CCCD_INFO *)p_param_list);
            uint16_t pos = 0;
            LE_UINT16_TO_ARRAY(p_param + pos, param.link_id); pos += 2;
            memcpy(p_param + pos, param.p_srv_handle, 4);
            pos += 4;
            LE_UINT16_TO_ARRAY(p_param + pos, param.count); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.gap); pos += 2;
            pos = pos + param.gap;
            memcpy(p_param + pos, param.data, 4 * param.count);
            pos += (4 * param.count);
            *param_list_len = pos;
            break;
        }
    case BTIF_MSG_GATT_ATTR_UPDATE_RSP:
        {


        }
        break;
    case BTIF_MSG_GATT_ATTR_WRITE_RSP:
        {
            T_BTIF_GATT_ATTR_WRITE_RSP param = *((T_BTIF_GATT_ATTR_WRITE_RSP *)p_param_list);

            uint16_t pos = 0;

            LE_UINT16_TO_ARRAY(p_param + pos, param.link_id); pos += 2;
            LE_UINT8_TO_ARRAY(p_param + pos, param.write_type); pos += 1;
            LE_UINT16_TO_ARRAY(p_param + pos, param.attr_handle); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.cause); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.attr_len); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.gap); pos += 2;

            pos = pos + param.gap;

            memcpy(p_param + pos, &param.data, param.attr_len); pos += param.attr_len;

            *param_list_len = pos;

            //*param_list_len = offsetof(T_BTIF_GATT_ATTR_WRITE_RSP, data) + param.gap+param.attr_len;

        }
        break;

    case BTIF_MSG_GATT_ATTR_IND:
        {
            T_BTIF_GATT_ATTR_IND param = *((T_BTIF_GATT_ATTR_IND *)p_param_list);
            uint16_t pos = 0;

            LE_UINT16_TO_ARRAY(p_param + pos, param.link_id); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.attr_handle); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.attr_len); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.gap); pos += 2;

            pos = pos + param.gap;

            memcpy(p_param + pos, param.data, param.attr_len); pos += param.attr_len;

            *param_list_len = pos;
            //*param_list_len = offsetof(T_BTIF_GATT_ATTR_IND, data) + param.gap + param.attr_len;

        }
        break;
    case BTIF_MSG_GATT_DISCOVERY_IND:
        {

            T_BTIF_GATT_DISCOVERY_IND param = *((T_BTIF_GATT_DISCOVERY_IND *)p_param_list);
            uint16_t pos = 0;

            LE_UINT16_TO_ARRAY(p_param + pos, param.link_id); pos += 2;
            LE_UINT8_TO_ARRAY(p_param + pos, param.type); pos += 1;
            LE_UINT16_TO_ARRAY(p_param + pos, param.cause); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.elem_cnt); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.elem_len); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.gap); pos += 2;

            pos = pos + param.gap;

            memcpy(p_param + pos, param.list, param.elem_cnt * param.elem_len);
            pos += (param.elem_cnt * param.elem_len);

            *param_list_len = pos;

        }
        break;
    case BTIF_MSG_GATT_SRV_REG_RSP:
        {

            T_BTIF_GATT_SRV_REG_RSP param = *((T_BTIF_GATT_SRV_REG_RSP *)p_param_list);
            uint16_t pos = 0;
            memcpy(p_param + pos, param.p_srv_handle, 1); pos += 4;
            LE_UINT16_TO_ARRAY(p_param + pos, param.cause); pos += 2;

            t_srv_table.p_srv_handle = (uint8_t *)&param.p_srv_handle;
            *param_list_len = pos;

        }
        break;
    case BTIF_MSG_GATT_SERVER_STORE_IND:
        {

            T_BTIF_GATT_SERVER_STORE_IND param = *((T_BTIF_GATT_SERVER_STORE_IND *)p_param_list);
            uint16_t pos = 0;

            LE_UINT8_TO_ARRAY(p_param + pos, param.op); pos += 1;
            LE_UINT8_TO_ARRAY(p_param + pos, param.remote_addr_type); pos += 1;
            memcpy(p_param + pos, param.bd_addr, 6); pos += 6;
            LE_UINT8_TO_ARRAY(p_param + pos, param.data_len); pos += 1;
            memcpy(p_param + pos, param.p_data, param.data_len); pos += param.data_len;
            *param_list_len = pos;
        }
        break;

    case BTIF_MSG_GATT_ATTR_WRITE_REQ_IND:
        {

            T_BTIF_GATT_ATTR_WRITE_IND param = *((T_BTIF_GATT_ATTR_WRITE_IND *)p_param_list);
            uint16_t pos = 0;

            LE_UINT16_TO_ARRAY(p_param + pos, param.link_id); pos += 2;
            memcpy(p_param + pos, param.p_srv_handle, 4); pos += 4;

            LE_UINT16_TO_ARRAY(p_param + pos, param.attr_index); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.attr_len); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.handle); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.write_offset); pos += 2;
            LE_UINT16_TO_ARRAY(p_param + pos, param.gap); pos += 2;
            pos += param.gap;
            memcpy(p_param + pos, param.data, param.attr_len); pos += param.attr_len;
            *param_list_len = pos;

        }
        break;
    case BTIF_MSG_LE_LOCAL_DEV_INFO:
        {

            T_BTIF_LE_LOCAL_DEV_INFO dev_info = *((T_BTIF_LE_LOCAL_DEV_INFO *)p_param_list);
            t_gatt_dev_info.le_ds_pool_id = dev_info.le_ds_pool_id;
            t_gatt_dev_info.le_ds_data_offset = dev_info.le_ds_data_offset;
            t_gatt_dev_info.le_ds_credits = dev_info.le_ds_credits;
            APP_PRINT_TRACE3("Dev Info: le_ds_pool_id=%d, le_ds_data_offset=%d ,le_ds_credits=%d ",
                             dev_info.le_ds_pool_id,
                             dev_info.le_ds_data_offset, dev_info.le_ds_credits);
        }
        break;
    case BTIF_MSG_USER_PASSKEY_NOTIF_IND:
        {
            T_BTIF_USER_PASSKEY_NOTIF_IND param = *((T_BTIF_USER_PASSKEY_NOTIF_IND *)p_param_list);
            uint16_t pos = 0;
            memcpy(p_param + pos, param.bd_addr, 6); pos += 6;
            LE_UINT8_TO_ARRAY(p_param + pos, param.remote_addr_type); pos += 1;
            LE_UINT32_TO_ARRAY(p_param + pos, param.display_value); pos += 4;
            *param_list_len = pos;
        }
        break;
    case BTIF_MSG_LE_ADV_RSP:
        {
            T_BTIF_LE_ADV_RSP param = *((T_BTIF_LE_ADV_RSP *)p_param_list);
            uint16_t pos = 0;
            LE_UINT8_TO_ARRAY(p_param + pos, param.adv_mode); pos += 1;
            LE_UINT16_TO_ARRAY(p_param + pos, param.cause); pos += 2;
            *param_list_len = pos;
        }
        break;
    default:
        break;
    }
    return true;
}

void hrp_btif_handle_msg(T_BTIF_UP_MSG *pMsg)
{
    // process upstream events
    HRP_MODULE_ID module_id = HRP_MODULE_UPPER_STACK;
    uint8_t cmd_group = HRP_BTIF_CMD_GROUP_STACK;
    uint16_t cmd_index = pMsg->command;
    uint16_t param_list_len =  btif_get_up_data_len(pMsg);
    T_BTIF_UP_MSG_DATA *p_param_list = & pMsg->p;

    /*1. handle systemMsg; 2. handle some message contains poiter, commit every parameters to p_param_list */
    if (hrp_btif_handle_special_msg(&cmd_group, &cmd_index, p_param_list, &param_list_len))
    {
        hrp_handle_upperstream_events(module_id,  cmd_group,
                                      cmd_index,  param_list_len, (uint8_t *)p_param_list);
    }

    btif_buffer_put(pMsg);

}

#endif
