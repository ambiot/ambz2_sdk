/**
*********************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hrp_gap_le.c
* @brief
* @details   none.
* @author    Lorna
* @date      2017-11-24
* @version   v0.1
* *********************************************************************************************************
*/
#include "../../../../inc/bluetooth/gap/gap_callback_le.h"
#include "trace_app.h"
//#include <gap_le.h>
//#include <gap_callback_le.h>
#include <hrp_gap_le.h>

#include <hrp_profile_system_api.h>
#include <gap_scan.h>
#include <gap_adv.h>
#include <hrp_profile_entry.h>
#include <os_mem.h>
#include <gap_conn_le.h>
#if F_BT_LE_GATT_CLIENT_SUPPORT
#include <profile_client.h>
#endif
#include <profile_server.h>
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
#include <gap_ext_scan.h>
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
#include <gap_ext_adv.h>
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
#include "gap_credit_based_conn.h"
#endif

#if F_BT_LE_PRIVACY_SUPPORT
#include <privacy_mgnt.h>
#include <gap_privacy.h>
#endif
#include <gatt_builtin_services.h>
#include <os_timer.h>
#include <os_msg.h>

T_GAP_DEV_STATE hrp_dev_state = {0, 0, 0, 0, 0};                 /**< GAP device state */
#if F_BT_LE_PRIVACY_SUPPORT
T_PRIVACY_STATE app_privacy_state = PRIVACY_STATE_INIT;
T_PRIVACY_ADDR_RESOLUTION_STATE app_privacy_resolution_state = PRIVACY_ADDR_RESOLUTION_DISABLED;
#endif
//#define HRP_GAP_LE_PROFILE_HRS_MASK 1   /* Define le profile mask here*/
uint8_t adv_handle = 0;
void *g_check_lps_wakeup_timer_handle = NULL;

#if F_BT_LE_GATT_CLIENT_SUPPORT
T_CLIENT_ID     simple_ble_client_id;  /**< Simple ble service client id*/
T_CLIENT_ID     gaps_client_id;        /**< gap service client id*/
#endif
T_SERVER_ID     simp_srv_id; /**< Simple ble service id*/

#define HRP_CHANN_NUM 3

#if F_BT_LE_PRIVACY_SUPPORT
bool hrp_test_privacy = false;
#endif

extern bool gap_set_lps_bootup_active_time(uint16_t active_time);
extern void lps_get_wakeup_time(uint32_t *p_wakeup_count, uint32_t *p_wakeup_time,
                                uint32_t *p_total_time);

void hrp_profile_gap_le_event(uint16_t cmd_index, uint16_t param_list_len,
                              uint8_t *p_param_list)
{
    uint8_t cmd_group = HRP_PROFILE_CMD_GROUP_EVENT_LE;

    hrp_profile_evet(cmd_group, cmd_index,  param_list_len, p_param_list);
}
static void hrp_profile_gap_le_cmd_result(uint16_t leCause)
{
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_RESULT, sizeof(uint16_t), (uint8_t *)&leCause);
}
#if 0
static void hrp_profile_gap_le_check_lps_wakeup_time_result(uint16_t leCause)
{
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CHECK_LPS_WAKEUP_TIME_RSP, sizeof(uint16_t),
                             (uint8_t *)&leCause);
}
#endif
static void hrp_profile_general_id_event(uint16_t cmd_index, uint16_t param_list_len,
                                         uint8_t *p_param_list)
{
    uint8_t cmd_group = HRP_PROFILE_CMD_GROUP_EVENT_GENERAL_ID;

    hrp_profile_evet(cmd_group, cmd_index,  param_list_len, p_param_list);
}


void hrp_gap_le_handle_io_msg(T_IO_MSG io_msg)
{

    uint16_t msg_type = io_msg.type;
    APP_PRINT_INFO1("hrp_gap_le_handle_io_msg :msg_type=%d ", msg_type);
    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        hrp_app_handle_gap_le_msg(&io_msg);
        break;
#if F_BT_DLPS_EN
    case IO_MSG_TYPE_TIMER:
        hrp_gap_le_check_lps_wakeup_time_handler(&io_msg);
        break;
#endif
    default:
        break;

    }
}

void hrp_app_handle_dev_state_evt(T_GAP_DEV_STATE new_state)
{
    APP_PRINT_INFO4("hrp_app_handle_dev_state_evt: init state %d scan state %d adv state %d conn state %d",
                    new_state.gap_init_state,
                    new_state.gap_scan_state, new_state.gap_adv_state, new_state.gap_conn_state);
#if F_BT_LE_PRIVACY_SUPPORT
    // if (hrp_test_privacy == true)
    {
        if ((new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
            && (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
            && (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
            && (new_state.gap_conn_state == GAP_CONN_DEV_STATE_IDLE))
        {
            privacy_handle_resolv_list();
        }
    }
#endif
    if (hrp_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO0("GAP stack ready");
            hrp_profile_handle_initCmplete(
                HRP_GAP_LE);   /* must call this function after le gap initiate completed*/
            /*stack ready*/
        }
    }
    if (hrp_dev_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");
        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");
        }
    }

    if (hrp_dev_state.gap_adv_state != new_state.gap_adv_state)
    {
        if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
            {
                APP_PRINT_INFO0("GAP adv stoped: because connection created");
            }
            else
            {
                APP_PRINT_INFO0("GAP adv stoped");
            }
        }
        else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            APP_PRINT_INFO0("GAP adv start");
        }
    }

    if (hrp_dev_state.gap_conn_state != new_state.gap_conn_state)
    {
        APP_PRINT_INFO2("conn state: %d -> %d",
                        hrp_dev_state.gap_conn_state,
                        new_state.gap_conn_state);
    }

    hrp_dev_state = new_state;
}
void hrp_app_handle_gap_le_msg(T_IO_MSG *p_io_msg)
{
    T_LE_GAP_MSG bt_msg;
    uint8_t evt_param[100];
    memcpy(&bt_msg, &p_io_msg->u.param, sizeof(p_io_msg->u.param));

    APP_PRINT_TRACE1("hrp_app_handle_gap_le_msg: sub_type %d", p_io_msg->subtype);

    switch (p_io_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            hrp_app_handle_dev_state_evt(bt_msg.msg_data.gap_dev_state_change.new_state);
            T_GAP_DEV_STATE_CHANGE *tmpPtr = &bt_msg.msg_data.gap_dev_state_change;
            uint16_t pos = 0;
            hrp_app_handle_dev_state_evt(tmpPtr->new_state);

            memcpy(evt_param + pos, &(tmpPtr->new_state), sizeof(T_GAP_DEV_STATE));
            pos += sizeof(T_GAP_DEV_STATE);

            //hrp_profile_fetch_buf(evt_param, &pos, tmpPtr, sizeof(T_GAP_DEV_STATE));

            LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_DEV_STATE_CHANGE, pos, evt_param);
        }
        break;
    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {

            T_GAP_CONN_STATE_CHANGE *tmpPtr = &bt_msg.msg_data.gap_conn_state_change;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->new_state); pos += 1;
            LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->disc_cause); pos += 2;
            APP_PRINT_INFO3("app_handle_conn_state_evt: conn_id %d, conn_state %d, disc_cause 0x%x",
                            tmpPtr->conn_id, tmpPtr->new_state, tmpPtr->disc_cause);


            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CONN_STATE_CHANGE, pos, evt_param);
        }
        break;
    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {

            T_GAP_CONN_PARAM_UPDATE *tmpPtr = &bt_msg.msg_data.gap_conn_param_update;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->status); pos += 1;
            LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CONN_PARAM_UPDATE, pos, evt_param);
        }
        break;
    case GAP_MSG_LE_CONN_MTU_INFO:
        {

            T_GAP_CONN_MTU_INFO *tmpPtr = &bt_msg.msg_data.gap_conn_mtu_info;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->mtu_size); pos += 2;
            APP_PRINT_INFO2("app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", tmpPtr->conn_id,
                            tmpPtr->mtu_size);
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CONN_MTU_INFO, pos, evt_param);
        }
        break;
    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {

            T_GAP_AUTHEN_STATE *tmpPtr = &bt_msg.msg_data.gap_authen_state;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->new_state); pos += 1;
            LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->status); pos += 2;

            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_AUTHEN_STATE_CHANGE, pos, evt_param);
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {

            T_GAP_BOND_PASSKEY_DISPLAY *tmpPtr = &bt_msg.msg_data.gap_bond_passkey_display;
            int pos = 0;
            uint32_t display_value = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            le_bond_get_display_key(tmpPtr->conn_id, &display_value);
            LE_UINT32_TO_ARRAY(evt_param + pos, display_value); pos += 4;
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn_id %d, display_value %d", tmpPtr->conn_id,
                            display_value);

            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_BOND_PASSKEY_DISPLAY, pos, evt_param);
        }
        break;
    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {

            T_GAP_BOND_PASSKEY_INPUT *tmpPtr = &bt_msg.msg_data.gap_bond_passkey_input;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->key_press); pos += 1;
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_BOND_PASSKEY_INPUT, pos, evt_param);
        }
        break;
    case GAP_MSG_LE_BOND_OOB_INPUT:
        {

            T_GAP_BOND_OOB_INPUT *tmpPtr = &bt_msg.msg_data.gap_bond_oob_input;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_BOND_OOB_INPUT, pos, evt_param);
        }

        break;
    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {

            T_GAP_BOND_USER_CONF *tmpPtr = &bt_msg.msg_data.gap_bond_user_conf;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_BOND_USER_CONFIRMATION, pos, evt_param);
        }

        break;
    case GAP_MSG_LE_BOND_JUST_WORK:
        {

            T_GAP_BOND_JUST_WORK_CONF *tmpPtr = &bt_msg.msg_data.gap_bond_just_work_conf;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_BOND_JUST_WORK, pos, evt_param);
        }

        break;
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    case GAP_MSG_LE_EXT_ADV_STATE_CHANGE:
        {

            T_GAP_EXT_ADV_STATE_CHANGE *tmpPtr = &bt_msg.msg_data.gap_ext_adv_state_change;
            int pos = 0;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->adv_handle); pos += 1;
            LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->new_state); pos += 1;
            LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_EXT_ADV_STATE_CHANGE, pos, evt_param);
        }

        break;
#endif
    }
}

#if F_BT_LE_PRIVACY_SUPPORT
void app_privacy_callback(T_PRIVACY_CB_TYPE type, T_PRIVACY_CB_DATA cb_data)
{
    APP_PRINT_INFO1("app_privacy_callback: type %d", type);
    switch (type)
    {
    case PRIVACY_STATE_MSGTYPE:
        app_privacy_state = cb_data.privacy_state;
        APP_PRINT_INFO1("PRIVACY_STATE_MSGTYPE: status %d", app_privacy_state);
        break;

    case PRIVACY_RESOLUTION_STATUS_MSGTYPE:
        {
            uint8_t param[1] = {0};
            int pos = 0;
            app_privacy_resolution_state = cb_data.resolution_state;
            APP_PRINT_INFO1("PRIVACY_RESOLUTION_STATUS_MSGTYPE: status %d", app_privacy_resolution_state);
            LE_UINT8_TO_ARRAY(param + pos, app_privacy_resolution_state); pos += 1;
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_PRIVACY_RESOLUTION_STATUS_INFO, pos, param);
        }
        break;

    case PRIVACY_READ_PEER_RESOLV_ADDR_MSGTYPE:
        {
            uint8_t param[8] = {0};
            int pos = 0;
            APP_PRINT_INFO2("PRIVACY_READ_PEER_RESOLV_ADDR_MSGTYPE: cause 0x%x, addr %b",
                            cb_data.p_privacy_read_resolv_addr_rsp->cause,
                            TRACE_BDADDR(cb_data.p_privacy_read_resolv_addr_rsp->resolv_addr));
            LE_UINT16_TO_ARRAY(param + pos, cb_data.p_privacy_read_resolv_addr_rsp->cause); pos += 2;
            hrp_profile_commit_buf(param, &pos, cb_data.p_privacy_read_resolv_addr_rsp->resolv_addr, 6);
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_PRIVACY_READ_PEER_RESOLV_ADDR, pos, param);
        }
        break;

    case PRIVACY_READ_LOCAL_RESOLV_ADDR_MSGTYPE:
        {
            uint8_t param[8] = {0};
            int pos = 0;
            APP_PRINT_INFO2("PRIVACY_READ_LOCAL_RESOLV_ADDR_MSGTYPE: cause 0x%x, addr %b",
                            cb_data.p_privacy_read_resolv_addr_rsp->cause,
                            TRACE_BDADDR(cb_data.p_privacy_read_resolv_addr_rsp->resolv_addr));
            LE_UINT16_TO_ARRAY(param + pos, cb_data.p_privacy_read_resolv_addr_rsp->cause); pos += 2;
            hrp_profile_commit_buf(param, &pos, cb_data.p_privacy_read_resolv_addr_rsp->resolv_addr, 6);
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_PRIVACY_READ_LOCAL_RESOLV_ADDR, pos, param);
        }
        break;

    case PRIVACY_GEN_PRIV_ADDR_INTERVAL_MSGTYPE:
        APP_PRINT_INFO1("PRIVACY_GEN_PRIV_ADDR_INTERVAL_MSGTYPE: cause 0x%x", cb_data.cause);
        break;

    case PRIVACY_SET_PEER_MODE_MSGTYPE:
        {
            uint8_t param[2] = {0};
            int pos = 0;
            APP_PRINT_INFO1("PRIVACY_SET_PEER_MODE_MSGTYPE: cause 0x%x", cb_data.cause);

            LE_UINT16_TO_ARRAY(param + pos, cb_data.cause); pos += 2;
            hrp_profile_gap_le_event(HRP_GAP_MSG_LE_PRIVACY_SET_MODE, pos, param);
        }

        break;

    default:
        break;
    }
}
#endif
/***********************set le param**********************************/
T_GAP_CAUSE hrp_le_set_gap_param(uint16_t type, uint8_t len, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_set_gap_param: type:%x= %b", type, TRACE_BINARY(len, p_value));
    return le_set_gap_param((T_GAP_LE_PARAM_TYPE)type, len, p_value);
}
T_GAP_CAUSE hrp_le_set_bond_param(uint16_t type, uint8_t len, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_set_bond_param: type:%x = %b", type, TRACE_BINARY(len, p_value));
    return le_bond_set_param((T_LE_BOND_PARAM_TYPE)type, len, p_value);
}
#if F_BT_LE_GAP_SCAN_SUPPORT
T_GAP_CAUSE hrp_le_set_scan_param(uint16_t type, uint8_t len, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_set_scan_param: type:%x = %b", type, TRACE_BINARY(len, p_value));
    return le_scan_set_param((T_LE_SCAN_PARAM_TYPE)type, len, p_value);
}
#endif
#if F_BT_LE_PRIVACY_SUPPORT
T_GAP_CAUSE hrp_le_set_privacy_param(uint16_t type, uint8_t len, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_set_privacy_param: type:%x = %b", type, TRACE_BINARY(len, p_value));
    return le_privacy_set_param((T_LE_PRIVACY_PARAM_TYPE)type, len, p_value);
}
#endif
T_GAP_CAUSE hrp_le_set_adv_param(uint16_t type, uint8_t len, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_set_adv_param: type:%x = %b", type, TRACE_BINARY(len, p_value));
    return le_adv_set_param((T_LE_ADV_PARAM_TYPE)type, len, p_value);
}

/****************************get le param*********************************/
T_GAP_CAUSE hrp_le_get_gap_param(uint16_t type, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_get_gap_param: type:%x = %b", type, TRACE_STRING(p_value));
    return le_get_gap_param((T_GAP_LE_PARAM_TYPE)type, p_value);
}

T_GAP_CAUSE hrp_le_get_bond_param(uint16_t type, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_get_bond_param: type:%x = %b", type, TRACE_STRING(p_value));
    return le_bond_get_param((T_LE_BOND_PARAM_TYPE)type, p_value);
}
#if F_BT_LE_GAP_SCAN_SUPPORT
T_GAP_CAUSE hrp_le_get_scan_param(uint16_t type, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_get_scan_param: type:%x = %b", type, TRACE_STRING(p_value));
    return le_scan_get_param((T_LE_SCAN_PARAM_TYPE)type, p_value);
}
#endif
#if F_BT_LE_PRIVACY_SUPPORT
T_GAP_CAUSE hrp_le_get_privacy_param(uint16_t type, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_get_privacy_param: type:%x = %b", type, TRACE_STRING(p_value));
    return le_privacy_get_param((T_LE_PRIVACY_PARAM_TYPE)type, p_value);
}
#endif
T_GAP_CAUSE hrp_le_get_adv_param(uint16_t type, void *p_value)
{
    APP_PRINT_INFO2("hrp_le_get_privacy_param: type:%x = %b", type, TRACE_STRING(p_value));
    return le_adv_get_param((T_LE_ADV_PARAM_TYPE)type, p_value);
}





/**********************start handle event fun************************************/


void hrp_profile_handle_set_rand_addr(void *data)
{
    T_LE_SET_RAND_ADDR_RSP *tmpPtr = (T_LE_SET_RAND_ADDR_RSP *)data;
    uint8_t evt_param[2];
    int pos = 0;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_SET_RAND_ADDR, pos, evt_param);
}
#if F_BT_LE_GAP_CENTRAL_SUPPORT
void hrp_profile_handle_set_host_chann_classif(void *data)
{
    T_LE_SET_HOST_CHANN_CLASSIF_RSP *tmpPtr = (T_LE_SET_HOST_CHANN_CLASSIF_RSP *)data;
    uint8_t evt_param[2];
    int pos = 0;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_SET_HOST_CHANN_CLASSIF, pos, evt_param);
}
#endif

void hrp_profile_handle_read_rssi(void *data)
{
    T_LE_READ_RSSI_RSP *tmpPtr = (T_LE_READ_RSSI_RSP *)data;
    uint8_t evt_param[4];
    int pos = 0;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->rssi); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_READ_RSSI, pos, evt_param);
}
#if F_BT_LE_READ_CHANN_MAP
void hrp_profile_handle_read_chann_map(void *data)
{
    T_LE_READ_CHANN_MAP_RSP *tmpPtr = (T_LE_READ_CHANN_MAP_RSP *)data;
    uint8_t evt_param[100];
    int pos = 0;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->channel_map, 5);

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_READ_CHANN_MAP, pos, evt_param);
}
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
void hrp_profile_handle_disable_slave_latency(void *data)
{
    T_LE_DISABLE_SLAVE_LATENCY_RSP *tmpPtr = (T_LE_DISABLE_SLAVE_LATENCY_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_DISABLE_SLAVE_LATENCY, pos, evt_param);
}
#endif
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
void hrp_profile_handle_set_data_len(void *data)
{
    T_LE_SET_DATA_LEN_RSP *tmpPtr = (T_LE_SET_DATA_LEN_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_SET_DATA_LEN, pos, evt_param);
}

void hrp_profile_handle_data_len_change_info(void *data)
{
    T_LE_DATA_LEN_CHANGE_INFO *tmpPtr = (T_LE_DATA_LEN_CHANGE_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->max_tx_octets); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->max_tx_time); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->max_rx_octets); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->max_rx_time); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_DATA_LEN_CHANGE_INFO, pos, evt_param);
}
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
void hrp_profile_handle_conn_update_ind(void *data)
{
    T_LE_CONN_UPDATE_IND *tmpPtr = (T_LE_CONN_UPDATE_IND *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->conn_interval_min); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->conn_interval_max); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->conn_latency); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->supervision_timeout); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CONN_UPDATE_IND, pos, evt_param);
}
#endif
void hrp_profile_handle_create_conn_ind(void *data)
{
    T_LE_CREATE_CONN_IND *tmpPtr = (T_LE_CREATE_CONN_IND *)data;
    int pos = 0;
    uint8_t evt_param[100];
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->bd_addr, 6);

    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->remote_addr_type); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CREATE_CONN_IND, pos, evt_param);
}
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void hrp_profile_handle_phy_update_info(void *data)
{
    T_LE_PHY_UPDATE_INFO *tmpPtr = (T_LE_PHY_UPDATE_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->tx_phy); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->rx_phy); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_PHY_UPDATE_INFO, pos, evt_param);
}
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
void hrp_profile_handle_update_passed_chann_map(void *data)
{
    T_LE_UPDATE_PASSED_CHANN_MAP_RSP *tmpPtr = (T_LE_UPDATE_PASSED_CHANN_MAP_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP, pos, evt_param);
}
#endif
#if F_BT_LE_READ_REMOTE_FEATS
void hrp_profile_handle_remote_feats_info(void *data)
{
    T_LE_REMOTE_FEATS_INFO *tmpPtr = (T_LE_REMOTE_FEATS_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->remote_feats, 8);

    APP_PRINT_INFO3("hrp_profile_handle_remote_feats_info: conn_id %d, cause=%d, remote_feats =%b",
                    tmpPtr->conn_id, tmpPtr->cause, TRACE_BINARY(8, tmpPtr->remote_feats));
    if (tmpPtr->remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] &
        LE_SUPPORT_FEATURES_LE_2M_MASK_BIT)
    {
        APP_PRINT_INFO0("GAP_MSG_LE_REMOTE_FEATS_INFO: support 2M");
    }
    if (tmpPtr->remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] &
        LE_SUPPORT_FEATURES_LE_CODED_PHY_MASK_BIT)
    {
        APP_PRINT_INFO0("GAP_MSG_LE_REMOTE_FEATS_INFO: support CODED");
    }
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_REMOTE_FEATS_INFO, pos, evt_param);
}
#endif
void hrp_profile_handle_bond_modify_info(void *data)
{
    T_LE_BOND_MODIFY_INFO *tmpPtr = (T_LE_BOND_MODIFY_INFO *)data;
    APP_PRINT_INFO2("GAP_MSG_LE_BOND_MODIFY_INFO: 0x%x, p_entry %p",
                    tmpPtr->type, tmpPtr->p_entry);
#if F_BT_LE_PRIVACY_SUPPORT
    privacy_handle_bond_modify_msg(tmpPtr->type, tmpPtr->p_entry, true);
#endif
    if (tmpPtr->type == LE_BOND_CLEAR)
    {
        return;
    }
    else if (tmpPtr->type == LE_BOND_KEY_MISSING)
    {
        le_bond_delete_by_idx(tmpPtr->p_entry->idx);
    }
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->type); pos += 1;

    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->p_entry->is_used); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->p_entry->idx); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->p_entry->flags); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->p_entry->local_bd_type); pos += 1;
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->p_entry->reserved, 2);

    hrp_profile_commit_buf(evt_param, &pos, &tmpPtr->p_entry->remote_bd, 8/*sizeof(T_LE_REMOTE_BD)*/);

    hrp_profile_commit_buf(evt_param, &pos, &tmpPtr->p_entry->resolved_remote_bd,
                           8/* sizeof(T_LE_REMOTE_BD)*/);

    //hrp_profile_gap_le_event(HRP_GAP_MSG_LE_BOND_MODIFY_INFO, pos, evt_param);
}

#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
void hrp_profile_handle_keypress_notify(void *data)
{
    T_LE_KEYPRESS_NOTIFY_RSP *tmpPtr = (T_LE_KEYPRESS_NOTIFY_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_KEYPRESS_NOTIFY, pos, evt_param);
}

void hrp_profile_handle_keypress_notify_info(void *data)
{
    T_LE_KEYPRESS_NOTIFY_INFO *tmpPtr = (T_LE_KEYPRESS_NOTIFY_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->event_type); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_KEYPRESS_NOTIFY_INFO, pos, evt_param);
}
#endif
#if F_BT_LE_ATT_SIGNED_WRITE_SUPPORT
void hrp_profile_handle_gatt_signed_status_info(void *data)
{
    T_LE_GATT_SIGNED_STATUS_INFO *tmpPtr = (T_LE_GATT_SIGNED_STATUS_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->update_local); pos += 1;
    LE_UINT32_TO_ARRAY(evt_param + pos, tmpPtr->local_sign_count); pos += 4;
    LE_UINT32_TO_ARRAY(evt_param + pos, tmpPtr->remote_sign_count); pos += 4;
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_GATT_SIGNED_STATUS_INFO, pos, evt_param);
}
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
void hrp_profile_handle_scan_info(void *data)
{
    T_LE_SCAN_INFO *tmpPtr = (T_LE_SCAN_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->bd_addr, 6);

    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->remote_addr_type); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->adv_type); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->rssi); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->data_len); pos += 1;
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->data, 31);

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_SCAN_INFO, pos, evt_param);
}
#if F_BT_LE_PRIVACY_SUPPORT
void hrp_profile_handle_direct_adv_info(void *data)
{
    T_LE_DIRECT_ADV_INFO *tmpPtr = (T_LE_DIRECT_ADV_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->bd_addr, 6);
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->remote_addr_type); pos += 1;
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->direct_bd_addr, 6);
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->direct_addr_type); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->direct_adv_type); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->rssi); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_DIRECT_ADV_INFO, pos, evt_param);
}
#endif
#endif
void hrp_profile_handle_adv_update_param(void *data)
{
    T_LE_ADV_UPDATE_PARAM_RSP *tmpPtr = (T_LE_ADV_UPDATE_PARAM_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_ADV_UPDATE_PARAM, pos, evt_param);
}
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
void hrp_profile_handle_adv_read_tx_power(void *data)
{
    T_LE_ADV_READ_TX_POWER_RSP *tmpPtr = (T_LE_ADV_READ_TX_POWER_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->tx_power_level); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_ADV_READ_TX_POWER, pos, evt_param);
}
#endif
#if 0
void hrp_profile_handle_dtm_receiver_test(void *data)
{
    T_LE_DTM_TEST_END_RSP *tmpPtr = (T_LE_DTM_TEST_END_RSP *)data;
    int pos = 0;

    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->num_pkts); pos += 2;

    hrp_profile_gap_le_event(GAP_MSG_LE_DTM_RECEIVER_TEST, pos, evt_param);
}
void hrp_profile_handle_dtm_transmitter_test(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(GAP_MSG_LE_DTM_TRANSMITTER_TEST, pos, evt_param);
}

void hrp_profile_handle_dtm_test_end(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(GAP_MSG_LE_DTM_TEST_END, pos, evt_param);
}
void hrp_profile_handle_dtm_enhanced_receiver_test(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(GAP_MSG_LE_DTM_ENHANCED_RECEIVER_TEST, pos, evt_param);
}
void hrp_profile_handle_dtm_enhanced_transmitter_test(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(GAP_MSG_LE_DTM_ENHANCED_TRANSMITTER_TEST, pos, evt_param);
}
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
void hrp_profile_handle_vendor_adv_3_data_enable(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_VENDOR_ADV_3_DATA_ENABLE, pos, evt_param);
}


void hrp_profile_handle_vendor_adv_3_data_set(void *data)
{
    T_LE_VENDOR_ADV_3_DATA_SET_RSP *tmpPtr = (T_LE_VENDOR_ADV_3_DATA_SET_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->type); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_VENDOR_ADV_3_DATA_SET, pos, evt_param);
}

void hrp_profile_handle_set_lps_bootup_active_time(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    uint8_t evt_param[10];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_SET_LPS_BOOTUP_ACTIVE_TIME, pos, evt_param);
}
#endif
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
void hrp_profile_handle_ext_adv_report_info(void *data)
{
    T_LE_EXT_ADV_REPORT_INFO *tmpPtr = (T_LE_EXT_ADV_REPORT_INFO *)data;
    int pos = 0;
    uint8_t param[500];

    LE_UINT16_TO_ARRAY(param + pos, tmpPtr->event_type); pos += 2;
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->data_status); pos += 1;
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->addr_type); pos += 1;
    hrp_profile_commit_buf(param, &pos, tmpPtr->bd_addr, 6);
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->primary_phy); pos += 1;

    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->secondary_phy); pos += 1;
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->adv_sid); pos += 1;
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->tx_power); pos += 1;
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->rssi); pos += 1;
    LE_UINT16_TO_ARRAY(param + pos, tmpPtr->peri_adv_interval); pos += 2;
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->direct_addr_type); pos += 1;
    hrp_profile_commit_buf(param, &pos, tmpPtr->direct_addr, 6);
    LE_UINT8_TO_ARRAY(param + pos, tmpPtr->data_len); pos += 1;
    hrp_profile_commit_buf(param, &pos, tmpPtr->p_data, tmpPtr->data_len);

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_EXT_ADV_REPORT_INFO, pos, param);
}
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
void hrp_profile_handle_ext_adv_start_setting(void *data)
{
    T_LE_EXT_ADV_START_SETTING_RSP *tmpPtr = (T_LE_EXT_ADV_START_SETTING_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->adv_handle); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->flag); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_EXT_ADV_START_SETTING, pos, evt_param);
}
void hrp_profile_handle_ext_adv_remove_set(void *data)
{
    T_LE_EXT_ADV_REMOVE_SET_RSP *tmpPtr = (T_LE_EXT_ADV_REMOVE_SET_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->adv_handle); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_EXT_ADV_REMOVE_SET, pos, evt_param);
}

void hrp_profile_handle_ext_adv_clear_set(void *data)
{
    T_LE_EXT_ADV_CLEAR_SET_RSP *tmpPtr = (T_LE_EXT_ADV_CLEAR_SET_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_EXT_ADV_CLEAR_SET, pos, evt_param);
}
void hrp_profile_handle_ext_adv_enable(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_EXT_ADV_ENABLE, pos, evt_param);
}

void hrp_profile_handle_ext_adv_disable(void *data)
{
    T_LE_CAUSE *tmpPtr = (T_LE_CAUSE *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_EXT_ADV_DISABLE, pos, evt_param);
}
void hrp_profile_handle_scan_req_received_info(void *data)
{
    T_LE_SCAN_REQ_RECEIVED_INFO *tmpPtr = (T_LE_SCAN_REQ_RECEIVED_INFO *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->adv_handle); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->scanner_addr_type); pos += 1;
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->scanner_addr, 6);
    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_SCAN_REQ_RECEIVED_INFO, pos, evt_param);
}
#endif
void hrp_profile_handle_modify_white_list(void *data)
{
    T_LE_MODIFY_WHITE_LIST_RSP *tmpPtr = (T_LE_MODIFY_WHITE_LIST_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->operation); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_MODIFY_WHITE_LIST, pos, evt_param);
}




/*********************end handle event fun******************************/



T_APP_RESULT hrp_gap_le_cb(uint8_t cb_type, void *p_cb_data)
{
    T_LE_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_LE_CB_DATA));
    APP_PRINT_INFO1("hrp_gap_le_cb: cb_type %d", cb_type);
    switch (cb_type)
    {
//gap_le.h
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        hrp_profile_handle_modify_white_list(cb_data.p_le_modify_white_list_rsp);
        break;
    case GAP_MSG_LE_SET_RAND_ADDR:
        hrp_profile_handle_set_rand_addr(cb_data.p_le_set_rand_addr_rsp);
        break;
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    case GAP_MSG_LE_SET_HOST_CHANN_CLASSIF:
        hrp_profile_handle_set_host_chann_classif(cb_data.p_le_set_host_chann_classif_rsp);
        break;
#endif
//gap_conn_le.h
    case GAP_MSG_LE_READ_RSSI:
        hrp_profile_handle_read_rssi(cb_data.p_le_read_rssi_rsp);
        break;
#if F_BT_LE_READ_CHANN_MAP
    case GAP_MSG_LE_READ_CHANN_MAP:
        hrp_profile_handle_read_chann_map(cb_data.p_le_read_chann_map_rsp);
        break;
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_DISABLE_SLAVE_LATENCY:
        hrp_profile_handle_disable_slave_latency(cb_data.p_le_disable_slave_latency_rsp);
        break;
#endif
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case GAP_MSG_LE_SET_DATA_LEN:
        hrp_profile_handle_set_data_len(cb_data.p_le_set_data_len_rsp);
        break;
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        hrp_profile_handle_data_len_change_info(cb_data.p_le_data_len_change_info);
        break;
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    case GAP_MSG_LE_CONN_UPDATE_IND:
        hrp_profile_handle_conn_update_ind(cb_data.p_le_conn_update_ind);
        break;
#endif
    case GAP_MSG_LE_CREATE_CONN_IND:
        hrp_profile_handle_create_conn_ind(cb_data.p_le_create_conn_ind);
        break;
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
    case GAP_MSG_LE_PHY_UPDATE_INFO:
        hrp_profile_handle_phy_update_info(cb_data.p_le_phy_update_info);
        break;
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:
        hrp_profile_handle_update_passed_chann_map(cb_data.p_le_update_passed_chann_map_rsp);
        break;
#endif
#if F_BT_LE_READ_REMOTE_FEATS
    case GAP_MSG_LE_REMOTE_FEATS_INFO:
        hrp_profile_handle_remote_feats_info(cb_data.p_le_remote_feats_info);
        break;
#endif
//gap_bond_le.h
    case GAP_MSG_LE_BOND_MODIFY_INFO:
        hrp_profile_handle_bond_modify_info(cb_data.p_le_bond_modify_info);
        break;
#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
    case GAP_MSG_LE_KEYPRESS_NOTIFY:
        hrp_profile_handle_keypress_notify(cb_data.p_le_keypress_notify_rsp);
        break;
    case GAP_MSG_LE_KEYPRESS_NOTIFY_INFO:
        hrp_profile_handle_keypress_notify_info(cb_data.p_le_keypress_notify_info);
        break;
#endif
#if F_BT_LE_ATT_SIGNED_WRITE_SUPPORT
    case GAP_MSG_LE_GATT_SIGNED_STATUS_INFO:
        hrp_profile_handle_gatt_signed_status_info(cb_data.p_le_gatt_signed_status_info);
        break;
#endif
//gap_scan.h
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    case GAP_MSG_LE_SCAN_INFO:
        hrp_profile_handle_scan_info(cb_data.p_le_scan_info);
        break;
#if F_BT_LE_PRIVACY_SUPPORT
    case GAP_MSG_LE_DIRECT_ADV_INFO:
        hrp_profile_handle_direct_adv_info(cb_data.p_le_direct_adv_info);
        break;
#endif
#endif
//gap_adv.h
    case GAP_MSG_LE_ADV_UPDATE_PARAM:
        hrp_profile_handle_adv_update_param(cb_data.p_le_adv_update_param_rsp);
        break;
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
    case GAP_MSG_LE_ADV_READ_TX_POWER:
        hrp_profile_handle_adv_read_tx_power(cb_data.p_le_adv_read_tx_power_rsp);
        break;
#endif
//gap_dtm.h
#if 0
    case HRP_GAP_MSG_LE_DTM_RECEIVER_TEST:
        hrp_profile_handle_dtm_receiver_test(p_cb_data);
        break;
    case HRP_GAP_MSG_LE_DTM_TRANSMITTER_TEST:
        hrp_profile_handle_dtm_transmitter_test(p_cb_data);
        break;
    case HRP_GAP_MSG_LE_DTM_TEST_END:
        hrp_profile_handle_dtm_test_end(p_cb_data);
        break;
    case HRP_GAP_MSG_LE_DTM_ENHANCED_RECEIVER_TEST:
        hrp_profile_handle_dtm_enhanced_receiver_test(p_cb_data);
        break;
    case HRP_GAP_MSG_LE_DTM_ENHANCED_TRANSMITTER_TEST:
        hrp_profile_handle_dtm_enhanced_transmitter_test(p_cb_data);
        break;
#endif
//gap_vendor.h
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    case GAP_MSG_LE_VENDOR_ADV_3_DATA_ENABLE:
        hrp_profile_handle_vendor_adv_3_data_enable(&cb_data.le_cause);
        break;
    case GAP_MSG_LE_VENDOR_ADV_3_DATA_SET:
        hrp_profile_handle_vendor_adv_3_data_set(cb_data.p_le_vendor_adv_3_data_set_rsp);
        break;
    case GAP_MSG_GAP_SET_LPS_BOOTUP_ACTIVE_TIME:
        hrp_profile_handle_set_lps_bootup_active_time(&cb_data.le_cause);
        break;
#endif
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
//gap_ext_scan.h
    case GAP_MSG_LE_EXT_ADV_REPORT_INFO:
        hrp_profile_handle_ext_adv_report_info(cb_data.p_le_ext_adv_report_info);
        break;
#endif

#if F_BT_LE_5_0_AE_ADV_SUPPORT
//gap_ext_adv.h
    case GAP_MSG_LE_EXT_ADV_START_SETTING:
        hrp_profile_handle_ext_adv_start_setting(cb_data.p_le_ext_adv_start_setting_rsp);
        break;
    case GAP_MSG_LE_EXT_ADV_REMOVE_SET:
        hrp_profile_handle_ext_adv_remove_set(cb_data.p_le_ext_adv_remove_set_rsp);
        break;
    case GAP_MSG_LE_EXT_ADV_CLEAR_SET:
        hrp_profile_handle_ext_adv_clear_set(cb_data.p_le_ext_adv_clear_set_rsp);
        break;
    case GAP_MSG_LE_EXT_ADV_ENABLE:
        hrp_profile_handle_ext_adv_enable(&cb_data.le_cause);
        break;
    case GAP_MSG_LE_EXT_ADV_DISABLE:
        hrp_profile_handle_ext_adv_disable(&cb_data.le_cause);
        break;
    case GAP_MSG_LE_SCAN_REQ_RECEIVED_INFO:
        hrp_profile_handle_scan_req_received_info(cb_data.p_le_scan_req_received_info);
        break;
#endif

    default:
        break;
    }
    return APP_RESULT_SUCCESS;
}


void  hrp_profile_server_init(uint16_t le_profile_sever_mask)
{
    server_init(7);
    APP_PRINT_INFO1("hrp_profile_server_init: le_profile_sever_mask %d",
                    le_profile_sever_mask);

    if (SIMPLE_BLE_SERVICE & le_profile_sever_mask)
    {

        simp_srv_id = simp_ble_service_add_service((void *)hrp_profile_callback);
    }
    if (SIMPLE_BLE_ADD_SERVICE & le_profile_sever_mask)
    {
        for (uint8_t i = 1; i <= 6; i++)
        {
            simp_ble_service_add_service((void *)hrp_profile_callback);
            APP_PRINT_INFO1("hrp_profile_server_init :add service  %d  times.",  i);
        }

    }

    server_register_app_cb(hrp_profile_callback);

}

/****************************hrp service event function **************************************/

void hrp_profile_handle_profile_evt_srv_reg_complete(T_SERVER_RESULT     service_reg_result)
{
    int pos = 0;
    uint8_t evt_param[100];

    LE_UINT8_TO_ARRAY(evt_param + pos, service_reg_result); pos += 1;

    hrp_profile_general_id_event(HRP_PROFILE_EVT_SRV_REG_COMPLETE, pos, evt_param);
}
void  hrp_profile_handle_profile_evt_send_data_complete(uint8_t conn_id, uint16_t  cause,
                                                        T_SERVER_ID service_id,
                                                        uint16_t  attrib_idx, uint16_t  credits)
{
    int pos = 0;
    uint8_t evt_param[100];

    LE_UINT16_TO_ARRAY(evt_param + pos, credits); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, conn_id); pos += 1;
    LE_UINT8_TO_ARRAY(evt_param + pos, service_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, attrib_idx); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, cause); pos += 2;

    hrp_profile_general_id_event(HRP_PROFILE_EVT_SRV_SEND_DATA_COMPLETE, pos, evt_param);
}


T_APP_RESULT hrp_profile_callback(T_SERVER_ID service_id, void *p_data)
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
                hrp_profile_handle_profile_evt_srv_reg_complete(p_param->event_data.service_reg_result);
            }

            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            {
                APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
                                p_param->event_data.send_data_result.conn_id,
                                p_param->event_data.send_data_result.cause,
                                p_param->event_data.send_data_result.service_id,
                                p_param->event_data.send_data_result.attrib_idx,
                                p_param->event_data.send_data_result.credits);
                if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
                }
                else
                {
                    APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
                }

                hrp_profile_handle_profile_evt_send_data_complete(p_param->event_data.send_data_result.conn_id,
                                                                  p_param->event_data.send_data_result.cause,
                                                                  p_param->event_data.send_data_result.service_id,
                                                                  p_param->event_data.send_data_result.attrib_idx,
                                                                  p_param->event_data.send_data_result.credits);
            }

            break;

        default:
            break;
        }
    }
    else  if (service_id == simp_srv_id)
    {
        TSIMP_CALLBACK_DATA *p_simp_cb_data = (TSIMP_CALLBACK_DATA *)p_data;
        hrp_profile_server_simp_ble_callback(p_simp_cb_data);
    }
    return app_result;
}

#if F_BT_LE_GATT_CLIENT_SUPPORT
void  hrp_profile_client_init(uint16_t le_profile_client_mask)
{
    client_init(3);
    gaps_client_id = gaps_add_client(hrp_client_callback, 4);
    APP_PRINT_INFO1("hrp_profile_client_init: le_profile_client_mask %d",
                    le_profile_client_mask);

    if (SIMPLE_BLE_CLIENT & le_profile_client_mask)
    {
        simple_ble_client_id = simp_ble_add_client(hrp_client_callback, 4);
    }

}
#endif
#if F_BT_LE_PRIVACY_SUPPORT
void  hrp_privacy_init()
{
    hrp_test_privacy = false;

    uint8_t central_address_resolution = 1;
    privacy_manage_mode(false);
    privacy_init(app_privacy_callback, false);
    gaps_set_parameter(GAPS_PARAM_CENTRAL_ADDRESS_RESOLUTION, sizeof(central_address_resolution),
                       &central_address_resolution);
}
#endif
/**
 * @brief  Callback will be called when data sent from profile client layer.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */

/****************************hrp client event function **************************************/
#if F_BT_LE_GATT_CLIENT_SUPPORT
void  hrp_profile_handle_client_app_cb_type_disc_state(T_DISCOVERY_STATE disc_state)
{
    int pos = 0;
    uint8_t evt_param[1];

    LE_UINT8_TO_ARRAY(evt_param + pos, disc_state); pos += 1;

    hrp_profile_general_id_event(HRP_PROFILE_EVT_CLIENT_DISC_STATE, pos, evt_param);
}
void  hrp_profile_handle_client_app_cb_type_disc_result(T_DISCOVERY_RESULT_TYPE result_type)
{
    int pos = 0;
    uint8_t evt_param[1];

    LE_UINT8_TO_ARRAY(evt_param + pos, result_type); pos += 1;

    hrp_profile_general_id_event(HRP_PROFILE_EVT_CLIENT_DISC_RESULT, pos, evt_param);
}

T_APP_RESULT hrp_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("app_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
    if (client_id == CLIENT_PROFILE_GENERAL_ID)
    {
        T_CLIENT_APP_CB_DATA *p_client_app_cb_data = (T_CLIENT_APP_CB_DATA *)p_data;
        switch (p_client_app_cb_data->cb_type)
        {
        case CLIENT_APP_CB_TYPE_DISC_STATE:
            {

                if (p_client_app_cb_data->cb_content.disc_state_data.disc_state == DISC_STATE_SRV_DONE)
                {
                    APP_PRINT_INFO0("Discovery All Service Procedure Done.");
                }
                else
                {
                    APP_PRINT_INFO0("Discovery state send to application directly.");
                }

                hrp_profile_handle_client_app_cb_type_disc_state(
                    p_client_app_cb_data->cb_content.disc_state_data.disc_state);
            }

            break;
        case CLIENT_APP_CB_TYPE_DISC_RESULT:
            {

                if (p_client_app_cb_data->cb_content.disc_result_data.result_type == DISC_RESULT_ALL_SRV_UUID16)
                {
                    APP_PRINT_INFO3("Discovery All Primary Service: UUID16 0x%x, start handle 0x%x, end handle 0x%x.",
                                    p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->uuid16,
                                    p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->att_handle,
                                    p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->end_group_handle);
                }
                else
                {
                    APP_PRINT_INFO0("Discovery result send to application directly.");
                }

                hrp_profile_handle_client_app_cb_type_disc_result(
                    p_client_app_cb_data->cb_content.disc_result_data.result_type);
            }

            break;
        default:
            break;
        }

    }
    else if (client_id == gaps_client_id)
    {
        T_GAPS_CLIENT_CB_DATA *p_gaps_cb_data = (T_GAPS_CLIENT_CB_DATA *)p_data;
        hrp_profile_client_gaps_callback(p_gaps_cb_data);

    }
    else if (client_id == simple_ble_client_id)
    {
        T_SIMP_CLIENT_CB_DATA *p_simp_client_cb_data = (T_SIMP_CLIENT_CB_DATA *)p_data;
        hrp_profile_client_simp_ble_callback(p_simp_client_cb_data);

    }

    return result;
}
#endif
void hrp_gap_le_init(uint16_t le_profile_sever_mask, uint16_t le_profile_client_mask)
{
    bool sign_flag = true;
    APP_PRINT_TRACE1("hrp_gap_le_init:link_num: %d", APP_MAX_LINKS);
    le_gap_init(APP_MAX_LINKS);
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    gap_lib_init();
#endif
    le_register_app_cb(hrp_gap_le_cb);
    le_bond_set_param(GAP_PARAM_BOND_SIGN_KEY_FLAG, 1, &sign_flag);
    // init server profiles
    hrp_profile_server_init(le_profile_sever_mask);
    // init client profiles
#if F_BT_LE_GATT_CLIENT_SUPPORT
    hrp_profile_client_init(le_profile_client_mask);
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
    le_cbc_init(HRP_CHANN_NUM);
    le_cbc_register_app_cb(hrp_credit_based_conn_callback);
#endif
#if F_BT_LE_PRIVACY_SUPPORT
    hrp_privacy_init();
#endif

}

#if F_BT_LE_4_1_CBC_SUPPORT
void hrp_gap_handle_cbc_le_chann_state(void *data)
{
    T_LE_CBC_CHANN_STATE *tmpPtr = (T_LE_CBC_CHANN_STATE *)data;
    int pos = 0;
    uint8_t evt_param[100];

    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cid); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_state); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CBC_CHANN_STATE, pos, evt_param);
}
void hrp_gap_handle_cbc_le_get_mtu(uint16_t mtu)
{
    int pos = 0;
    uint8_t evt_param[2];
    LE_UINT16_TO_ARRAY(evt_param + pos, mtu); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CBC_MTU_INFO, pos, evt_param);
}
void hrp_gap_handle_cbc_le_reg_psm(void *data)
{
    T_LE_CBC_CREDIT_BASED_PSM_REG_RSP *tmpPtr = (T_LE_CBC_CREDIT_BASED_PSM_REG_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];

    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->le_psm); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CBC_REG_PSM, pos, evt_param);
}

void hrp_gap_handle_cbc_le_set_psm_security(void *data)
{
    T_LE_CBC_CREDIT_BASED_SECURITY_REG_RSP *tmpPtr = (T_LE_CBC_CREDIT_BASED_SECURITY_REG_RSP *)data;
    int pos = 0;
    uint8_t evt_param[100];

    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CBC_SET_PSM_SECURITY, pos, evt_param);
}

void hrp_gap_handle_cbc_le_send_data(void *data)
{
    T_LE_CBC_SEND_DATA *tmpPtr = (T_LE_CBC_SEND_DATA *)data;
    int pos = 0;
    uint8_t evt_param[100];

    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cid); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cause); pos += 2;
    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->credit); pos += 1;

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CBC_SEND_DATA, pos, evt_param);
}

void hrp_gap_handle_cbc_le_receive_data(void *data)
{
    T_LE_CBC_RECEIVE_DATA *tmpPtr = (T_LE_CBC_RECEIVE_DATA *)data;
    int pos = 0;
    uint8_t *evt_param;
    evt_param = os_mem_alloc(RAM_TYPE_DATA_ON, 1500);

    LE_UINT8_TO_ARRAY(evt_param + pos, tmpPtr->conn_id); pos += 1;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->cid); pos += 2;
    LE_UINT16_TO_ARRAY(evt_param + pos, tmpPtr->value_len); pos += 2;
    hrp_profile_commit_buf(evt_param, &pos, tmpPtr->p_data, tmpPtr->value_len);

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_CBC_RECEIVE_DATA, pos, evt_param);

    APP_PRINT_INFO2("hrp_gap_handle_cbc_le_receive_data: len:%d  pdata= %b", tmpPtr->value_len,
                    TRACE_BINARY(tmpPtr->value_len, tmpPtr->p_data));

    os_mem_free(evt_param);
}



T_APP_RESULT hrp_credit_based_conn_callback(uint8_t cbc_type, void *p_cbc_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CBC_DATA cb_data;
    memcpy(&cb_data, p_cbc_data, sizeof(T_LE_CBC_DATA));
    APP_PRINT_TRACE1("hrp_credit_based_conn_callback: cbc_type = %d", cbc_type);
    switch (cbc_type)
    {
    case GAP_CBC_MSG_LE_CHANN_STATE:
        {
            APP_PRINT_INFO4("GAP_CBC_MSG_LE_CHANN_STATE: conn_id %d, cid 0x%x, conn_state %d, cause 0x%x",
                            cb_data.p_le_chann_state->conn_id,
                            cb_data.p_le_chann_state->cid,
                            cb_data.p_le_chann_state->conn_state,
                            cb_data.p_le_chann_state->cause);
            if (cb_data.p_le_chann_state->conn_state == GAP_CHANN_STATE_CONNECTED)
            {
                uint16_t mtu;
                le_cbc_get_chann_param(CBC_CHANN_PARAM_MTU, &mtu, cb_data.p_le_chann_state->cid);
                APP_PRINT_INFO1("GAP_CHANN_STATE_CONNECTED: mtu %d", mtu);
                hrp_gap_handle_cbc_le_get_mtu(mtu);
            }
            hrp_gap_handle_cbc_le_chann_state(cb_data.p_le_chann_state);
        }

        break;

    case GAP_CBC_MSG_LE_REG_PSM:
        {
            APP_PRINT_INFO2("GAP_CBC_MSG_LE_REG_PSM: le_psm 0x%x, cause 0x%x",
                            cb_data.p_le_reg_psm_rsp->le_psm,
                            cb_data.p_le_reg_psm_rsp->cause);
            hrp_gap_handle_cbc_le_reg_psm(cb_data.p_le_reg_psm_rsp);
        }

        break;

    case GAP_CBC_MSG_LE_SET_PSM_SECURITY:
        {
            APP_PRINT_INFO1("GAP_CBC_MSG_LE_SET_PSM_SECURITY: cause 0x%x",
                            cb_data.p_le_set_psm_security_rsp->cause);
            hrp_gap_handle_cbc_le_set_psm_security(cb_data.p_le_set_psm_security_rsp);
        }

        break;

    case GAP_CBC_MSG_LE_SEND_DATA:
        {
            APP_PRINT_INFO4("GAP_CBC_MSG_LE_SEND_DATA: conn_id %d, cid 0x%x, cause 0x%x, credit %d",
                            cb_data.p_le_send_data->conn_id,
                            cb_data.p_le_send_data->cid,
                            cb_data.p_le_send_data->cause,
                            cb_data.p_le_send_data->credit);
            hrp_gap_handle_cbc_le_send_data(cb_data.p_le_send_data);
        }

        break;

    case GAP_CBC_MSG_LE_RECEIVE_DATA:
        {
            APP_PRINT_INFO3("GAP_CBC_MSG_LE_RECEIVE_DATA: conn_id %d, cid 0x%x, value_len %d",
                            cb_data.p_le_receive_data->conn_id,
                            cb_data.p_le_receive_data->cid,
                            cb_data.p_le_receive_data->value_len);
            hrp_gap_handle_cbc_le_receive_data(cb_data.p_le_receive_data);
        }

        break;

    default:
        break;
    }
    return result;
}
#endif


//================================cmd =============================//

static void hrp_gap_le_bond_delete_by_bd(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t bd_addr[6] = {0};
    uint8_t  bd_type;

    T_GAP_CAUSE tgap_cause;
    memcpy(bd_addr, p_param_list + pos, 6); pos += 6;
    LE_ARRAY_TO_UINT8(bd_type, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_delete_by_bd(bd_addr, (T_GAP_REMOTE_ADDR_TYPE)bd_type);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}

static void hrp_gap_le_bond_delete_by_idx(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t idx;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(idx, p_param_list + pos); pos += 1;
    tgap_cause = le_bond_delete_by_idx(idx);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}

static void hrp_gap_le_set_gap_param(uint16_t len, uint8_t *p_param_list)
{
    uint8_t pos = 0;
    uint8_t param_type;
    uint8_t data_len;
    uint16_t data_type;
    uint8_t *p_value;
    T_GAP_CAUSE tgap_cause;

    while (pos < len)
    {
        param_type = p_param_list[pos++];
        /* parse data field: len(1 byte) + type(GAP_DATA_TYPE_LEN byte) + value (len - GAP_DATA_TYPE_LEN)*/
        data_len = p_param_list[pos++];
        LE_ARRAY_TO_UINT16(data_type, p_param_list); pos += 2;
        p_value = p_param_list + pos;
        pos += data_len - GAP_DATA_TYPE_LEN;
        switch (param_type)
        {
        case LE_SET_PARAM_TYPE_GAP:
            tgap_cause = hrp_le_set_gap_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            hrp_profile_gap_le_cmd_result(tgap_cause);
            break;
        case LE_SET_PARAM_TYPE_BOND:
            tgap_cause = hrp_le_set_bond_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            hrp_profile_gap_le_cmd_result(tgap_cause);
            break;
#if F_BT_LE_GAP_SCAN_SUPPORT
        case LE_SET_PARAM_TYPE_SCAN:
            tgap_cause = hrp_le_set_scan_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            hrp_profile_gap_le_cmd_result(tgap_cause);
            break;
#endif
#if F_BT_LE_PRIVACY_SUPPORT
        case LE_SET_PARAM_TYPE_PRIVACY:
            tgap_cause = hrp_le_set_privacy_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            hrp_profile_gap_le_cmd_result(tgap_cause);
            break;
#endif
        case LE_SET_PARAM_TYPE_ADV:
            tgap_cause = hrp_le_set_adv_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            hrp_profile_gap_le_cmd_result(tgap_cause);
            break;
        default:
            break;
        }
    }

}




static void hrp_gap_le_get_gap_param(uint16_t len, uint8_t *p_param_list)
{
    uint8_t pos = 0;
    uint8_t param_type;
    uint16_t data_type;
    uint8_t *p_value;
    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, 6);

    param_type = p_param_list[pos++];

    /* parse data field:  type(GAP_DATA_TYPE_LEN byte) + value (len - GAP_DATA_TYPE_LEN-param_type)*/
    LE_ARRAY_TO_UINT16(data_type, p_param_list); pos += 2;
    // p_value = p_param_list + pos;

    switch (param_type)
    {
    case LE_GET_PARAM_TYPE_GAP:
        hrp_le_get_gap_param(data_type, (void *)p_value);
        break;
    case LE_GET_PARAM_TYPE_BOND:
        hrp_le_get_bond_param(data_type, (void *)p_value);
        break;
#if F_BT_LE_GAP_SCAN_SUPPORT
    case LE_GET_PARAM_TYPE_SCAN:
        hrp_le_get_scan_param(data_type, (void *)p_value);
        break;
#endif
#if F_BT_LE_PRIVACY_SUPPORT
    case LE_GET_PARAM_TYPE_PRIVACY:
        hrp_le_get_privacy_param(data_type, (void *)p_value);
        break;
#endif
    case LE_GET_PARAM_TYPE_ADV:
        hrp_le_get_adv_param(data_type, (void *)p_value);
        break;
    default:
        break;
    }
    os_mem_free(p_value);
}
static void hrp_gap_le_bond_clear_all_keys(uint16_t len, uint8_t *p_param_list)
{
    le_bond_clear_all_keys();
    hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);

}
#if F_BT_LE_SMP_SC_OOB_SUPPORT
static void hrp_gap_le_bond_sc_peer_oob_init(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE tgap_cause;
    T_GAP_LE_PEER_OOB_DATA *p_peer_oob_data = 0;
    p_peer_oob_data = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(T_GAP_LE_PEER_OOB_DATA));
    hrp_profile_fetch_buf(p_peer_oob_data, &pos, p_param_list, sizeof(T_GAP_LE_PEER_OOB_DATA));
    tgap_cause = le_bond_sc_peer_oob_init(p_peer_oob_data);

    hrp_profile_gap_le_cmd_result(tgap_cause);
    os_mem_free(p_peer_oob_data);
}

static void hrp_gap_le_bond_sc_local_oob_init(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    APP_PRINT_INFO0("hrp_profile_le_bond_sc_local_oob_init");
    uint64_t ecc_rand_in[4] = {0x1111111111111111, 0x1111111111111111, 0x1111111111111111, 0x1111111111111111};
    uint8_t local_rand[16] = {0x11, 0x11};
    T_GAP_LE_LOCAL_OOB_DATA *local_oob_data;
    local_oob_data = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(T_GAP_LE_LOCAL_OOB_DATA));
    T_GAP_CAUSE tgap_cause;
    hrp_profile_fetch_buf(ecc_rand_in, &pos, p_param_list, 32);
    hrp_profile_fetch_buf(local_rand, &pos, p_param_list, 16);
    hrp_profile_fetch_buf(local_oob_data, &pos, p_param_list, sizeof(T_GAP_LE_LOCAL_OOB_DATA));
    tgap_cause = le_bond_sc_local_oob_init(ecc_rand_in, local_rand, local_oob_data);

    hrp_profile_gap_le_cmd_result(tgap_cause);
    os_mem_free(local_oob_data);

}

#endif
static void hrp_gap_le_bond_cfg_local_key_distribute(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t init_dist;
    uint8_t rsp_dist;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(init_dist, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(rsp_dist, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_cfg_local_key_distribute(init_dist, rsp_dist);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}

#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
static void hrp_gap_le_bond_keypress_notify(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    uint8_t type;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(type, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_keypress_notify(conn_id, (T_GAP_KEYPRESS_NOTIF_TYPE)type);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
static void hrp_gap_le_bond_user_confirm(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    uint8_t  cause;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_user_confirm(conn_id, (T_GAP_CFM_CAUSE)cause);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_bond_passkey_display_confirm(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    uint8_t  cause;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_passkey_display_confirm(conn_id, (T_GAP_CFM_CAUSE)cause);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_bond_just_work_confirm(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    uint8_t  cause;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_just_work_confirm(conn_id, (T_GAP_CFM_CAUSE)cause);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#if F_BT_LE_SMP_OOB_SUPPORT
static void hrp_gap_le_bond_oob_input_confirm(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    uint8_t  cause;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_oob_input_confirm(conn_id, (T_GAP_CFM_CAUSE)cause);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
static void hrp_gap_le_bond_passkey_input_confirm(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    uint32_t passcode;
    uint8_t  cause;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT32(passcode, p_param_list + pos); pos += 4;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_passkey_input_confirm(conn_id, passcode, (T_GAP_CFM_CAUSE)cause);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_bond_get_display_key(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    uint32_t *p_key;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_get_display_key(conn_id, p_key);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_bond_pair(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;

    T_GAP_CAUSE tgap_cause;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;

    tgap_cause = le_bond_pair(conn_id);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_scan_stop(uint16_t len, uint8_t *p_param_list)
{

    T_GAP_CAUSE tgap_cause;

    tgap_cause = le_scan_stop();

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_scan_start(uint16_t len, uint8_t *p_param_list)
{

    T_GAP_CAUSE tgap_cause;

    tgap_cause = le_scan_start();

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
static void hrp_gap_le_adv_read_tx_power(uint16_t len, uint8_t *p_param_list)
{

    T_GAP_CAUSE tgap_cause;

    tgap_cause = le_adv_read_tx_power();

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
static void hrp_gap_le_adv_update_param(uint16_t len, uint8_t *p_param_list)
{


    T_GAP_CAUSE tgap_cause;

    tgap_cause = le_adv_update_param();

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_adv_stop(uint16_t len, uint8_t *p_param_list)
{


    T_GAP_CAUSE tgap_cause;

    tgap_cause = le_adv_stop();

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_adv_start(uint16_t len, uint8_t *p_param_list)
{

    T_GAP_CAUSE tgap_cause;

    tgap_cause = le_adv_start();

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
static void hrp_gap_le_update_conn_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;
    uint16_t conn_interval_min;
    uint16_t conn_interval_max;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
    uint16_t ce_length_min;
    uint16_t ce_length_max;

    T_GAP_CAUSE tgap_cause;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(conn_interval_min, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(conn_interval_max, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(conn_latency, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(supervision_timeout, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(ce_length_min, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(ce_length_max, p_param_list + pos); pos += 2;

    tgap_cause = le_update_conn_param(conn_id, conn_interval_min, conn_interval_max, conn_latency,
                                      supervision_timeout, ce_length_min, ce_length_max);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#if 0
static void hrp_gap_le_disable_slave_latency(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;
    bool disable;

    T_GAP_CAUSE tgap_cause;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(disable, p_param_list + pos); pos += 1;
    tgap_cause = le_disable_slave_latency(conn_id, disable);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
static void hrp_gap_le_update_passed_chann_map(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    bool enable;

    T_GAP_CAUSE tgap_cause;

    LE_ARRAY_TO_UINT8(enable, p_param_list + pos); pos += 1;

    tgap_cause = le_update_passed_chann_map(enable);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
static void hrp_gap_le_set_conn_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t  type;
    T_GAP_LE_CONN_REQ_PARAM *p_conn_param = NULL;
    T_GAP_CAUSE tgap_cause;
    p_conn_param = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(T_GAP_LE_CONN_REQ_PARAM));

    LE_ARRAY_TO_UINT8(type, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(p_conn_param->scan_interval, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(p_conn_param->scan_window, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(p_conn_param->conn_interval_min, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(p_conn_param->conn_interval_max, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(p_conn_param->conn_latency, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(p_conn_param->supv_tout, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(p_conn_param->ce_len_min, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(p_conn_param->ce_len_max, p_param_list + pos); pos += 2;

    tgap_cause = le_set_conn_param((T_GAP_CONN_PARAM_TYPE)type, p_conn_param);

    hrp_profile_gap_le_cmd_result(tgap_cause);
    os_mem_free(p_conn_param);

}
static void hrp_gap_le_connect(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t init_phys;
    uint8_t remote_bd[6] = {0};
    uint8_t remote_bd_type;
    uint8_t  local_bd_type;
    uint16_t scan_timeout;

    T_GAP_CAUSE tgap_cause;

    LE_ARRAY_TO_UINT8(init_phys, p_param_list + pos); pos += 1;
    memcpy(remote_bd, p_param_list + pos, 6); pos += 6;

    LE_ARRAY_TO_UINT8(remote_bd_type, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(local_bd_type, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(scan_timeout, p_param_list + pos); pos += 2;

    tgap_cause = le_connect(init_phys, remote_bd, (T_GAP_REMOTE_ADDR_TYPE)remote_bd_type,
                            (T_GAP_LOCAL_ADDR_TYPE)local_bd_type, scan_timeout);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
static void hrp_gap_le_set_phy(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;
    uint8_t all_phys;
    uint8_t tx_phys;
    uint8_t rx_phys;
    uint8_t phy_options;

    T_GAP_CAUSE tgap_cause;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(all_phys, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(tx_phys, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(rx_phys, p_param_list + pos); pos += 1;
    phy_options =   p_param_list[pos++];
    tgap_cause = le_set_phy(conn_id, all_phys, tx_phys, rx_phys, (T_GAP_PHYS_OPTIONS)phy_options);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
static void hrp_gap_le_set_data_len(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;
    uint16_t tx_octets;
    uint16_t tx_time;

    T_GAP_CAUSE tgap_cause;

    conn_id = p_param_list[pos++];
    LE_ARRAY_TO_UINT16(tx_octets, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(tx_time, p_param_list + pos); pos += 2;

    tgap_cause = le_set_data_len(conn_id, tx_octets, tx_time);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
#if F_BT_LE_READ_CHANN_MAP
static void hrp_gap_le_read_chann_map(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;

    T_GAP_CAUSE tgap_cause;

    conn_id = p_param_list[pos++];

    tgap_cause = le_read_chann_map(conn_id);

    hrp_profile_gap_le_cmd_result(tgap_cause);

}
#endif
static void hrp_gap_le_read_rssi(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;

    T_GAP_CAUSE tgap_status;

    conn_id = p_param_list[pos++];

    tgap_status = le_read_rssi(conn_id);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
static void hrp_gap_le_disconnect(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;

    T_GAP_CAUSE tgap_status;

    conn_id = p_param_list[pos++];

    tgap_status = le_disconnect(conn_id);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
static void hrp_gap_le_get_idle_link_num(uint16_t len, uint8_t *p_param_list)
{
    le_get_idle_link_num();
    hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);
}

static void hrp_gap_le_get_active_link_num(uint16_t len, uint8_t *p_param_list)
{
    le_get_active_link_num();
    hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);
}

static void hrp_gap_le_get_conn_addr(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;
    uint8_t bd_addr[6] = {0};;
    uint8_t *bd_type;

    conn_id = p_param_list[pos++];

    if (le_get_conn_addr(conn_id, bd_addr, bd_type))
    {
        hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);
    }
    else
    {
        hrp_profile_gap_le_cmd_result(GAP_CAUSE_NON_CONN);
    }
}
#if F_BT_DLPS_EN
static void hrp_gap_le_set_lps_bootup_active_time(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint16_t active_time;

    LE_ARRAY_TO_UINT16(active_time, p_param_list + pos); pos += 2;

    if (gap_set_lps_bootup_active_time(active_time))
    {
        hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);
    }
    else
    {
        hrp_profile_gap_le_cmd_result(GAP_CAUSE_SEND_REQ_FAILED);
    }
}

static void hrp_gap_le_check_lps_wakeup_time_enable(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint16_t check_time;
    uint8_t  enable;
    LE_ARRAY_TO_UINT16(check_time, p_param_list + pos); pos += 2;

    LE_ARRAY_TO_UINT8(enable, p_param_list + pos); pos += 1;

    if (enable)
    {
        if (NULL == g_check_lps_wakeup_timer_handle)
        {
            os_timer_create(&g_check_lps_wakeup_timer_handle, "CheckTimer", 1, check_time, true,
                            hrp_gap_le_check_lps_event_to_app);
        }
        APP_PRINT_INFO0("check lps wakeup time Enable");
        //start total timer
        os_timer_start(&g_check_lps_wakeup_timer_handle);

    }
    else
    {
        APP_PRINT_INFO0("check lps wakeup time Disable");
        os_timer_stop(&g_check_lps_wakeup_timer_handle);

    }

}
extern void *hIoQueueHandle;

void hrp_gap_le_check_lps_event_to_app(void *pxTimer)
{
    T_IO_MSG io_driver_msg_send;
    uint8_t event  = EVENT_IO_TO_APP;
    io_driver_msg_send.type = IO_MSG_TYPE_TIMER;
    //APP_PRINT_INFO0("hrp_gap_le_check_lps_event_to_app");
    if (os_msg_send(hIoQueueHandle, &io_driver_msg_send, 0) == false)
    {
        return;
    }
    else if (os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleEvent, &event, 0) == false)
    {
        return;
    }
    //APP_PRINT_INFO0("hrp_gap_le_check_lps_event_to_app: success");
}
static void hrp_gap_le_check_lps_wakeup_time_disable(uint16_t len, uint8_t *p_param_list)
{
    APP_PRINT_INFO0("check lps wakeup time Disable");
    os_timer_stop(&g_check_lps_wakeup_timer_handle);
}
void hrp_gap_le_check_lps_wakeup_time_handler(void *pxTimer)
{

    //tell how long has not been in DLPS mode
    static uint32_t wakeCountLast = 0;
    static uint32_t totalwakeuptimeLast = 0;
    static uint32_t totaltimeLast = 0;


    uint32_t wakeupCount = 0;  //sleep_mode_param.lps_wakeup_count
    uint32_t totalwakeuptime = 0; //sleep_mode_param.lps_wakeup_time
    uint32_t totaltime = 0; //sleep_mode_param.lps_total_time

    lps_get_wakeup_time(&wakeupCount, &totalwakeuptime, &totaltime);

    if (wakeCountLast == wakeupCount &&
        totalwakeuptimeLast == totalwakeuptime &&
        totaltimeLast == totaltime
       )
    {
        hrp_profile_gap_le_check_lps_wakeup_time_result(GAP_CAUSE_SEND_REQ_FAILED);
        APP_PRINT_INFO0("NOT in DLPS");
    }
    else
    {
        APP_PRINT_INFO6("wakeupCount: %d <->%d ,totalwakeuptime:%d <->%d ,totaltime: %d<->%d",
                        wakeCountLast, wakeupCount, totalwakeuptimeLast, totalwakeuptime, totaltimeLast, totaltime);
        //hrp_profile_gap_le_check_lps_wakeup_time_result(GAP_CAUSE_SUCCESS);
        APP_PRINT_INFO0(" Success in DLPS ");
    }

    wakeCountLast = wakeupCount;
    totalwakeuptimeLast = totalwakeuptime;
    totaltimeLast = totaltime;

}
#endif
static void hrp_gap_le_get_conn_info(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t conn_id;
    T_GAP_CONN_INFO *p_conn_info;

    conn_id = p_param_list[pos++];

    if (le_get_conn_info(conn_id, p_conn_info))
    {
        hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);
    }
    else

    {
        hrp_profile_gap_le_cmd_result(GAP_CAUSE_NON_CONN);
    }


}
static void hrp_gap_le_get_conn_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint16_t param;
    void *p_value;
    uint8_t conn_id;

    T_GAP_CAUSE tgap_status;
    LE_ARRAY_TO_UINT16(param, p_param_list + pos); pos += 2;

    conn_id = p_param_list[pos++];

    tgap_status = le_get_conn_param((T_LE_CONN_PARAM_TYPE)param, p_value, conn_id);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
#if F_BT_LE_GAP_CENTRAL_SUPPORT
static void hrp_gap_le_set_host_chann_classif(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t *p_channel_map;

    T_GAP_CAUSE tgap_status;

    p_channel_map = &p_param_list[pos];

    tgap_status = le_set_host_chann_classif(p_channel_map);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
#endif
static void hrp_gap_le_cfg_local_identity_address(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t addr[6] = {0};
    uint8_t  type;
    T_GAP_CAUSE tgap_status;

    hrp_profile_fetch_buf(addr, &pos, p_param_list, 6);
    type = p_param_list[pos++];

    tgap_status = le_cfg_local_identity_address(addr, (T_GAP_IDENT_ADDR_TYPE)type);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
static void hrp_gap_le_set_rand_addr(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t random_bd[6] = {0};

    T_GAP_CAUSE tgap_status;

    hrp_profile_fetch_buf(random_bd, &pos, p_param_list, 6);

    tgap_status = le_set_rand_addr(random_bd);

    hrp_profile_gap_le_cmd_result(tgap_status);
}
static void hrp_gap_le_gen_rand_addr(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t rand_addr_type;
    uint8_t random_bd[6] = {0};

    T_GAP_CAUSE tgap_status;

    rand_addr_type = p_param_list[pos++];

    //hrp_profile_fetch_buf(random_bd, &pos, p_param_list, 6);

    tgap_status = le_gen_rand_addr((T_GAP_RAND_ADDR_TYPE)rand_addr_type, random_bd);

    hrp_profile_gap_le_event(HRP_GAP_MSG_LE_GEN_RAND_ADDR_RSP, 6, random_bd);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
static void hrp_gap_le_modify_white_list(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;

    uint8_t  operation;
    uint8_t bd_addr[6] = {0};
    uint8_t bd_type;

    T_GAP_CAUSE tgap_status;

    operation = p_param_list[pos++];

    hrp_profile_fetch_buf(bd_addr, &pos, p_param_list, 6);
    bd_type = p_param_list[pos++];

    tgap_status = le_modify_white_list((T_GAP_WHITE_LIST_OP)operation, bd_addr,
                                       (T_GAP_REMOTE_ADDR_TYPE)bd_type);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
static void hrp_gap_le_get_max_link_num(uint16_t len, uint8_t *p_param_list)
{

    uint8_t link_num = le_get_max_link_num();

    hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);

}
//=================bt 5 cmd=========================
#if F_BT_LE_5_0_AE_ADV_SUPPORT
static void hrp_gap_le_ext_adv_get_param(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;

    uint32_t param;
    void *p_value;
    T_GAP_CAUSE tgap_status;

    LE_ARRAY_TO_UINT16(param, p_param_list + pos); pos += 2;

    tgap_status = le_ext_adv_get_param((T_LE_EXT_ADV_PARAM_TYPE)param, p_value);

    hrp_profile_gap_le_cmd_result(tgap_status);

}
static void hrp_gap_le_ext_adv_set_adv_param(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;
    T_GAP_CAUSE cause;
    uint16_t adv_event_prop = (uint16_t) LE_EXT_ADV_EXTENDED_ADV_CONN_UNDIRECTED;
    uint32_t primary_adv_interval_min = 320;
    uint32_t primary_adv_interval_max = 320;
    uint8_t  primary_adv_channel_map = GAP_ADVCHAN_ALL;
    uint8_t own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    uint8_t peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t p_peer_address[6] = {0};
    uint8_t filter_policy = 0;
    uint8_t tx_power = 10;
    uint8_t primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
    uint8_t secondary_adv_max_skip = 0x00;
    uint8_t secondary_adv_phy = GAP_PHYS_CODED;
    uint8_t adv_sid = 0;
    bool scan_req_notification_enable = false;

    adv_handle = le_ext_adv_create_adv_handle();

    LE_ARRAY_TO_UINT16(adv_event_prop, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT32(primary_adv_interval_min, p_param_list + pos); pos += 4;
    LE_ARRAY_TO_UINT32(primary_adv_interval_max, p_param_list + pos); pos += 4;
    LE_ARRAY_TO_UINT8(primary_adv_channel_map, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(own_address_type, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(peer_address_type, p_param_list + pos); pos += 1;
    hrp_profile_fetch_buf(p_peer_address, &pos, p_param_list, 6);
    LE_ARRAY_TO_UINT8(filter_policy, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(tx_power, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(primary_adv_phy, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(secondary_adv_max_skip, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(secondary_adv_phy, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(adv_sid, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(scan_req_notification_enable, p_param_list + pos); pos += 1;


    cause =    le_ext_adv_set_adv_param(adv_handle,
                                        adv_event_prop,
                                        primary_adv_interval_min,
                                        primary_adv_interval_max,
                                        primary_adv_channel_map,
                                        (T_GAP_LOCAL_ADDR_TYPE)own_address_type,
                                        (T_GAP_REMOTE_ADDR_TYPE)peer_address_type,
                                        p_peer_address,
                                        (T_GAP_ADV_FILTER_POLICY)filter_policy,
                                        tx_power,
                                        (T_GAP_PHYS_PRIM_ADV_TYPE)primary_adv_phy,
                                        secondary_adv_max_skip,
                                        (T_GAP_PHYS_TYPE)secondary_adv_phy,
                                        adv_sid,
                                        scan_req_notification_enable);

    hrp_profile_gap_le_cmd_result(cause);

}

static void hrp_gap_le_ext_adv_set_adv_data(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;

    uint16_t adv_data_len;
    uint8_t *p_adv_data;
    T_GAP_CAUSE cause;
    LE_ARRAY_TO_UINT16(adv_data_len, p_param_list + pos); pos += 2;
    p_adv_data = os_mem_alloc(RAM_TYPE_DATA_ON, adv_data_len);

    hrp_profile_fetch_buf(p_adv_data, &pos, p_param_list, adv_data_len);
    cause = le_ext_adv_set_adv_data(adv_handle, adv_data_len, p_adv_data);
    APP_PRINT_INFO2("hrp_gap_le_ext_adv_set_adv_data: len:%d  pdata= %b", adv_data_len,
                    TRACE_BINARY(adv_data_len, p_adv_data));
    hrp_profile_gap_le_cmd_result(cause);
    os_mem_free(p_adv_data);

}
static void hrp_gap_le_ext_adv_set_scan_response_data(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;

    uint16_t scan_data_len;
    uint8_t *p_scan_data;
    T_GAP_CAUSE cause;
    LE_ARRAY_TO_UINT16(scan_data_len, p_param_list + pos); pos += 2;
    p_scan_data = os_mem_alloc(RAM_TYPE_DATA_ON, scan_data_len);
    hrp_profile_fetch_buf(p_scan_data, &pos, p_param_list, scan_data_len);

    cause = le_ext_adv_set_scan_response_data(adv_handle, scan_data_len, p_scan_data);
    APP_PRINT_INFO2("hrp_gap_le_ext_adv_set_scan_response_data: len:%d  pdata= %b", scan_data_len,
                    TRACE_BINARY(scan_data_len, p_scan_data));
    hrp_profile_gap_le_cmd_result(cause);
    os_mem_free(p_scan_data);

}
static void hrp_gap_le_ext_adv_set_random(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;

    uint8_t random_address[6] = {0};
    T_GAP_CAUSE cause;

    hrp_profile_fetch_buf(random_address, &pos, p_param_list, 6);

    cause = le_ext_adv_set_random(adv_handle, random_address);

    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_ext_adv_start_setting(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t update_flags;
    T_GAP_CAUSE cause;

    LE_ARRAY_TO_UINT8(update_flags, p_param_list + pos); pos += 1;
    cause = le_ext_adv_start_setting(adv_handle, update_flags);
    APP_PRINT_TRACE3("hrp_gap_le_ext_adv_start_setting :adv_handle: %d,update_flags:%d, Return Cause=%x ",
                     adv_handle, update_flags, cause);
    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_ext_adv_set_adv_enable_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint16_t duration;
    uint8_t max_ext_adv_evt;
    uint8_t adv_handle;
    T_GAP_CAUSE cause;

    LE_ARRAY_TO_UINT16(duration, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT8(max_ext_adv_evt, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(adv_handle, p_param_list + pos); pos += 1;
    cause = le_ext_adv_set_adv_enable_param(adv_handle, duration, max_ext_adv_evt);

    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_ext_adv_enable(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t i;

    T_GAP_CAUSE cause;
    uint8_t num_of_sets;
    uint8_t adv_handle[4] = {0};
    LE_ARRAY_TO_UINT8(num_of_sets, p_param_list + pos); pos += 1;
    for (i = 0; i < num_of_sets; i++)
    {
        adv_handle[i] = i;
    }

    cause = le_ext_adv_enable(num_of_sets, adv_handle);

    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_ext_adv_disable(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t i;

    T_GAP_CAUSE cause;
    uint8_t num_of_sets;
    uint8_t adv_handle[4] = {0};
    LE_ARRAY_TO_UINT8(num_of_sets, p_param_list + pos); pos += 1;
    for (i = 0; i < num_of_sets; i++)
    {
        adv_handle[i] = i;
    }

    cause = le_ext_adv_disable(num_of_sets, adv_handle);

    hrp_profile_gap_le_cmd_result(cause);
}
static void hrp_gap_le_ext_adv_clear_set(uint16_t len, uint8_t *p_param_list)
{

    T_GAP_CAUSE cause;

    cause = le_ext_adv_clear_set();

    hrp_profile_gap_le_cmd_result(cause);
}
static void hrp_gap_le_ext_adv_remove_set(uint16_t len, uint8_t *p_param_list)
{
    //uint8_t adv_handle = 0;
    //uint16_t pos = 0;
    T_GAP_CAUSE cause;
    //LE_ARRAY_TO_UINT8(adv_handle, p_param_list + pos); pos += 1;
    cause = le_ext_adv_remove_set(adv_handle);
    hrp_profile_gap_le_cmd_result(cause);
}

#endif

#if F_BT_LE_5_0_AE_SCAN_SUPPORT
static void hrp_gap_le_ext_scan_set_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t param_type;
    uint8_t data_len;
    void *p_value;

    LE_ARRAY_TO_UINT16(param_type, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT8(data_len, p_param_list + pos); pos += 1;
    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, data_len);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, len);

    cause = le_ext_scan_set_param((T_LE_EXT_SCAN_PARAM_TYPE)param_type, data_len, p_value);

    hrp_profile_gap_le_cmd_result(cause);
    os_mem_free(p_value);
}
static void hrp_gap_le_ext_scan_get_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t param_type;
    void *p_value;

    LE_ARRAY_TO_UINT16(param_type, p_param_list + pos); pos += 2;

    cause = le_ext_scan_get_param((T_LE_EXT_SCAN_PARAM_TYPE)param_type,  p_value);

    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_ext_scan_start(uint16_t len, uint8_t *p_param_list)
{
    T_GAP_CAUSE cause;
    cause = le_ext_scan_start();
    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_ext_scan_stop(uint16_t len, uint8_t *p_param_list)
{
    T_GAP_CAUSE cause;
    cause = le_ext_scan_stop();
    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_ext_scan_set_phy_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t type;
    T_GAP_LE_EXT_SCAN_PARAM *p_param;

    LE_ARRAY_TO_UINT8(type, p_param_list + pos); pos += 1;
    p_param = os_mem_alloc(RAM_TYPE_DATA_ON, sizeof(T_GAP_LE_EXT_SCAN_PARAM));

    hrp_profile_fetch_buf(p_param, &pos, p_param_list, sizeof(T_GAP_LE_EXT_SCAN_PARAM));

    le_ext_scan_set_phy_param((T_LE_EXT_SCAN_PHY_TYPE)type, p_param);
    hrp_profile_gap_le_cmd_result(GAP_CAUSE_SUCCESS);
    os_mem_free(p_param);

}
#endif


#if F_BT_LE_4_1_CBC_SUPPORT
static void hrp_gap_le_cbc_set_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t param_type;
    uint8_t length;
    void *p_value;

    LE_ARRAY_TO_UINT16(param_type, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT8(length, p_param_list + pos); pos += 1;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    cause = le_cbc_set_param((T_LE_CBC_PARAM_TYPE)param_type, length, p_value);

    hrp_profile_gap_le_cmd_result(cause);
    os_mem_free(p_value);

}

static void hrp_gap_le_cbc_get_chann_param(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t param_type;
    uint16_t cid;
    uint16_t  p_value;

    LE_ARRAY_TO_UINT16(param_type, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(cid, p_param_list + pos); pos += 2;

    cause = le_cbc_get_chann_param((T_LE_CBC_CHANN_PARAM_TYPE)param_type, &p_value, cid);
    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_cbc_create(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint8_t conn_id;
    uint16_t le_psm;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(le_psm, p_param_list + pos); pos += 2;

    cause = le_cbc_create(conn_id, le_psm);
    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_cbc_disc(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t cid;
    LE_ARRAY_TO_UINT16(cid, p_param_list + pos); pos += 2;

    cause = le_cbc_disc(cid);
    hrp_profile_gap_le_cmd_result(cause);
}
static void hrp_gap_le_cbc_send_data(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t cid;
    uint8_t *p_data;
    uint16_t  data_len;

    LE_ARRAY_TO_UINT16(cid, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(data_len, p_param_list + pos); pos += 2;

    p_data = os_mem_alloc(RAM_TYPE_DATA_ON, data_len);

    hrp_profile_fetch_buf(p_data, &pos, p_param_list, data_len);

    APP_PRINT_INFO2("hrp_gap_le_cbc_send_data0: len:%d  pdata= %b", data_len,
                    TRACE_BINARY(data_len, p_data + (data_len - 100)));

    cause = le_cbc_send_data(cid, p_data, data_len);

    hrp_profile_gap_le_cmd_result(cause);
    os_mem_free(p_data);
}

static void hrp_gap_le_cbc_reg_psm(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t le_psm;
    uint8_t action;

    LE_ARRAY_TO_UINT16(le_psm, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT8(action, p_param_list + pos); pos += 1;

    cause = le_cbc_reg_psm(le_psm, action);
    hrp_profile_gap_le_cmd_result(cause);
}
static void hrp_gap_le_cbc_set_psm_security(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint16_t le_psm;
    bool active;
    uint8_t  mode;
    uint8_t key_size;

    LE_ARRAY_TO_UINT16(le_psm, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT8(active, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(mode, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(key_size, p_param_list + pos); pos += 1;

    cause = le_cbc_set_psm_security(le_psm, active, (T_LE_CBC_SECURITY_MODE) mode, key_size);
    hrp_profile_gap_le_cmd_result(cause);
}
#endif

#if F_BT_LE_PRIVACY_SUPPORT
static void hrp_gap_le_privacy_set_addr_resolution(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;
    T_GAP_CAUSE cause;
    bool enable;

    LE_ARRAY_TO_UINT8(enable, p_param_list + pos); pos += 1;

    cause = privacy_set_addr_resolution(enable);
    hrp_profile_gap_le_cmd_result(cause);
}

static void hrp_gap_le_privacy_read_peer_resolv_addr(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint8_t idx;
    T_LE_KEY_ENTRY *p_entry;

    LE_ARRAY_TO_UINT8(idx, p_param_list + pos); pos += 1;

    p_entry = le_find_key_entry_by_idx(idx);
    if (p_entry)
    {
        cause = privacy_read_peer_resolv_addr((T_GAP_REMOTE_ADDR_TYPE)p_entry->remote_bd.remote_bd_type,
                                              p_entry->remote_bd.addr);
    }
    hrp_profile_gap_le_cmd_result(cause);
}

static void hrp_gap_le_privacy_read_local_resolv_addr(uint16_t len, uint8_t *p_param_list)
{

    uint16_t pos = 0;
    T_GAP_CAUSE cause = GAP_CAUSE_INVALID_PARAM;
    uint8_t idx;
    T_LE_KEY_ENTRY *p_entry;

    LE_ARRAY_TO_UINT8(idx, p_param_list + pos); pos += 1;

    p_entry = le_find_key_entry_by_idx(idx);
    if (p_entry)
    {
        cause = privacy_read_local_resolv_addr((T_GAP_REMOTE_ADDR_TYPE)p_entry->remote_bd.remote_bd_type,
                                               p_entry->remote_bd.addr);
    }

    hrp_profile_gap_le_cmd_result(cause);
}
static void hrp_gap_le_privacy_set_resolv_priv_addr_timeout(uint16_t len, uint8_t *p_param_list)
{

    uint16_t interval;
    T_GAP_CAUSE cause;

    le_privacy_get_param(GAP_PARAM_PRIVACY_TIMEOUT, &interval);
    cause = privacy_set_gen_priv_addr_interval(interval);
    hrp_profile_gap_le_cmd_result(cause);
}

static void hrp_gap_le_privacy_modify_resolv_list(uint16_t len, uint8_t *p_param_list)
{
#if 0
    uint16_t pos = 0;
    T_GAP_CAUSE cause;
    uint8_t operation;
    uint8_t peer_identity_address_type;
    uint8_t peer_identity_address[6] = {0};

    LE_ARRAY_TO_UINT8(operation, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(peer_identity_address_type, p_param_list + pos); pos += 1;
    hrp_profile_fetch_buf(peer_identity_address, &pos, p_param_list, 6);

    cause = le_privacy_modify_resolv_list((T_GAP_RESOLV_LIST_OP) operation,
                                          (T_GAP_IDENT_ADDR_TYPE)peer_identity_address_type, peer_identity_address);
    hrp_profile_gap_le_cmd_result(cause);
#endif
}
/*
static void hrp_gap_le_privacy_set_mode(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint8_t privacy_mode;
    uint8_t idx;

    LE_ARRAY_TO_UINT8(privacy_mode, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(idx, p_param_list + pos); pos += 1;

    cause = le_privacy_set_mode((T_GAP_IDENT_ADDR_TYPE)privacy_table[idx].remote_bd_type,
                                privacy_table[idx].addr, (T_GAP_PRIVACY_MODE) privacy_mode);
    hrp_profile_gap_le_cmd_result(cause);

}
*/
static void hrp_gap_le_privacy_set_mode(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint8_t privacy_mode;
    uint8_t remote_bd_type;
    uint8_t addr[6] = {0};

    LE_ARRAY_TO_UINT8(privacy_mode, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(remote_bd_type, p_param_list + pos); pos += 1;
    hrp_profile_fetch_buf(addr, &pos, p_param_list, 6);

    cause = privacy_set_peer_mode((T_GAP_REMOTE_ADDR_TYPE)remote_bd_type,
                                  addr, privacy_mode);
    hrp_profile_gap_le_cmd_result(cause);
}
static void hrp_gap_le_privacy_modify_whitelist(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;

    uint8_t op;
    uint8_t idx;
    T_LE_KEY_ENTRY *p_entry;

    LE_ARRAY_TO_UINT8(op, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(idx, p_param_list + pos); pos += 1;

    if (op == (uint8_t)GAP_WHITE_LIST_OP_CLEAR)
    {
        cause = le_modify_white_list((T_GAP_WHITE_LIST_OP)op, NULL, GAP_REMOTE_ADDR_LE_PUBLIC);
    }
    else
    {
        p_entry = le_find_key_entry_by_idx(idx);
        if (p_entry)
        {
            cause = le_modify_white_list((T_GAP_WHITE_LIST_OP)op, p_entry->resolved_remote_bd.addr,
                                         (T_GAP_REMOTE_ADDR_TYPE)p_entry->resolved_remote_bd.remote_bd_type);
        }
        else
        {
            cause = GAP_CAUSE_ERROR_UNKNOWN;
        }
    }

    hrp_profile_gap_le_cmd_result(cause);

}
static void hrp_gap_le_privacy_adv_start(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    T_GAP_CAUSE cause;
    uint8_t idx;
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    T_LE_KEY_ENTRY *p_entry;

    LE_ARRAY_TO_UINT8(idx, p_param_list + pos); pos += 1;
    p_entry = le_find_key_entry_by_idx(idx);
    if (p_entry)
    {
        adv_direct_type = p_entry->resolved_remote_bd.remote_bd_type;
        memcpy(adv_direct_addr, p_entry->resolved_remote_bd.addr, 6);
    }

    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);

    cause = le_adv_start();
    hrp_profile_gap_le_cmd_result(cause);

}
#endif


//================================hrp gap le handle req=============================//

void (*(hrp_profile_handle_gap_le[]))(uint16_t len, uint8_t *p_param_list) =
{
    NULL,
    hrp_gap_le_set_gap_param,                    // 0x01
    hrp_gap_le_get_gap_param,                 //  0x02
    hrp_gap_le_get_max_link_num,         //0x03
    hrp_gap_le_modify_white_list,               //0x04
    hrp_gap_le_gen_rand_addr,                           //0x05
    hrp_gap_le_set_rand_addr,                           //06
    hrp_gap_le_cfg_local_identity_address,                  //07
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    hrp_gap_le_set_host_chann_classif,                  //08
#else
    NULL,                  //08
#endif
    hrp_gap_le_get_conn_param,                  //0x09
    hrp_gap_le_get_conn_info,                                                  //0x0a
    hrp_gap_le_get_conn_addr,                                                  //0x0b
#if F_BT_DLPS_EN
    hrp_gap_le_set_lps_bootup_active_time,                   //0c
    hrp_gap_le_check_lps_wakeup_time_enable,                                               //0d
    hrp_gap_le_check_lps_wakeup_time_disable,                                              //0e
#else
    NULL,                   //0c
    NULL,                                               //0d
    NULL,                                              //0e
#endif
    NULL,                                              //0f
    hrp_gap_le_get_active_link_num,                                      //0x10
    hrp_gap_le_get_idle_link_num,                 //0x11
    hrp_gap_le_disconnect,                  //12
    hrp_gap_le_read_rssi,                   //0x13
#if F_BT_LE_READ_CHANN_MAP
    hrp_gap_le_read_chann_map,          //0x14
#else
    NULL,          //0x14
#endif
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    hrp_gap_le_set_data_len,                //0x15
#else
    NULL,                //0x15
#endif
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
    hrp_gap_le_set_phy,                 //0x16
#else
    NULL,                 //0x16
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    hrp_gap_le_disable_slave_latency,       //0x17
    hrp_gap_le_update_passed_chann_map, //0x18
#else
    NULL,       //0x17
    NULL, //0x18
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    hrp_gap_le_set_conn_param,          //0x19
    hrp_gap_le_connect,                 //0x1A
#else
    NULL,          //0x19
    NULL,                 //0x1A
#endif
    hrp_gap_le_update_conn_param,       //0x1B
    NULL,                                        //0x1C
    NULL,                                       //0x1D
    NULL,                                       //0x1E
    NULL,                                   //0x1F
    hrp_gap_le_adv_start,                       //0x20
    hrp_gap_le_adv_stop,                        //0x21
    hrp_gap_le_adv_update_param,                //0x22
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
    hrp_gap_le_adv_read_tx_power,               //0x23
#else
    NULL,               //0x23
#endif
    NULL,                                       //0x24
    NULL,                                       //0x25
    NULL,                                       //0x26
    NULL,                                       //0x27
    NULL,                                       //0x28
    NULL,                                       //0x29
    NULL,                                       //0x2A
    NULL,                                       //0x2B
    NULL,                                       //0x2C
    NULL,                                       //0x2D
    NULL,                                       //0x2E
    NULL,                                       //0x2F
    hrp_gap_le_scan_start,                          //0x30
    hrp_gap_le_scan_stop,                           //0x31
    NULL,                                       //0x32
    NULL,                                       //0x33
    NULL,                                       //0x34
    NULL,                                       //0x35
    NULL,                                       //0x36
    NULL,                                       //0x37
    NULL,                                       //0x38
    NULL,                                       //0x39
    NULL,                                       //0x3A
    NULL,                                       //0x3B
    NULL,                                       //0x3C
    NULL,                                       //0x3D
    NULL,                                       //0x3E
    NULL,                                       //0x3F
    hrp_gap_le_bond_pair,                           //0x40
    hrp_gap_le_bond_get_display_key,                //0x41
    hrp_gap_le_bond_passkey_input_confirm,          //0x42
#if F_BT_LE_SMP_OOB_SUPPORT
    hrp_gap_le_bond_oob_input_confirm,              //0x43
#else
    NULL,
#endif
    hrp_gap_le_bond_just_work_confirm,              //0x44
    hrp_gap_le_bond_passkey_display_confirm,        //0x45
    hrp_gap_le_bond_user_confirm,                   //0x46
#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
    hrp_gap_le_bond_keypress_notify,                //0x47
#else
    NULL,
#endif
    hrp_gap_le_bond_cfg_local_key_distribute,       //0x48
#if F_BT_LE_SMP_SC_OOB_SUPPORT
    hrp_gap_le_bond_sc_local_oob_init,              //0x49
    hrp_gap_le_bond_sc_peer_oob_init,               //0x4A
#else
    NULL,
    NULL,
#endif
    hrp_gap_le_bond_clear_all_keys,                 //0x4B
    hrp_gap_le_bond_delete_by_idx,                  //0x4C
    hrp_gap_le_bond_delete_by_bd,                   //0x4D
    NULL,                                       //0x4E
    NULL,                                       //0x4F
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    hrp_gap_le_ext_adv_get_param,  //0x50
    hrp_gap_le_ext_adv_set_adv_param, //0x51
    hrp_gap_le_ext_adv_set_adv_data, //0x52
    hrp_gap_le_ext_adv_set_scan_response_data, //0x53
    hrp_gap_le_ext_adv_set_random,  //0x54
    hrp_gap_le_ext_adv_start_setting,  //0x55
    hrp_gap_le_ext_adv_set_adv_enable_param, //0x56
    hrp_gap_le_ext_adv_enable,    //0x57
    hrp_gap_le_ext_adv_disable,   //0x58
    hrp_gap_le_ext_adv_clear_set,  //0x59
    hrp_gap_le_ext_adv_remove_set,  //0x5a
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, //0x5f
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
    hrp_gap_le_ext_scan_set_param, //0x60
    hrp_gap_le_ext_scan_get_param, //0x61
    hrp_gap_le_ext_scan_start,   //0x62
    hrp_gap_le_ext_scan_stop,   //0x63
    hrp_gap_le_ext_scan_set_phy_param, //0x64
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif
    NULL,   //0x65
    NULL,   //0x66
    NULL,   //0x67
    NULL,   //0x68
    NULL,   //0x69
#if F_BT_LE_4_1_CBC_SUPPORT
    hrp_gap_le_cbc_set_param,   //0x6a
    hrp_gap_le_cbc_get_chann_param, //0x6b
    hrp_gap_le_cbc_create,        //0x6c
    hrp_gap_le_cbc_disc,      //0x6d
    hrp_gap_le_cbc_send_data,//0x6e
    hrp_gap_le_cbc_reg_psm,  //0x6f
    hrp_gap_le_cbc_set_psm_security, //0x70
#else
    NULL,   //0x6a
    NULL,//0x6b
    NULL,//0x6c
    NULL, //0x6d
    NULL,//0x6e
    NULL,//0x6f
    NULL, //0x70
#endif
    NULL,//0x71
    NULL, //0x72
    NULL,  //0x73
#if F_BT_LE_PRIVACY_SUPPORT
    hrp_gap_le_privacy_set_addr_resolution,  //0x74
    hrp_gap_le_privacy_read_peer_resolv_addr,    //0x75
    hrp_gap_le_privacy_read_local_resolv_addr,    //0x76
    hrp_gap_le_privacy_set_resolv_priv_addr_timeout, //0x77
    hrp_gap_le_privacy_modify_resolv_list,  //0x78
    hrp_gap_le_privacy_set_mode,  //0x79
    hrp_gap_le_privacy_modify_whitelist, //0x7a
    hrp_gap_le_privacy_adv_start, //0x7b
#else
    NULL,  //0x74
    NULL,    //0x75
    NULL,    //0x76
    NULL, //0x77
    NULL,  //0x78
    NULL,  //0x79
    NULL,  //0x7a
    NULL,  //0x7b
#endif


};







