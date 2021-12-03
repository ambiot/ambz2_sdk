/**
************************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file     user_cmd.c
* @brief    User defined profile test commands.
* @details  User command interfaces.
* @author
* @date     2016-02-18
* @version  v0.1
*************************************************************************************************************
*/
#include <string.h>
#include "trace_app.h"
#if F_BT_LE_GATT_CLIENT_SUPPORT
#include "profile_client.h"
#include "vendor_tp_client.h"
#include "vendor_pxpext_client.h"
#include <gaps_client.h>
#endif
#include "gap_bond_le.h"
#include "gap_scan.h"
#include "user_cmd.h"
#include "gap.h"
#include "link_mgr.h"

#include "ble_auto_test_application.h"
#include "gap_adv.h"
#include "gap_le.h"
#include "gap_conn_le.h"
#include <ble_auto_test_case.h>
#include "privacy_mgnt.h"
#include "gap_storage_le.h"


T_USER_CMD_IF user_cmd_if;
extern uint8_t gSimpleProfileServiceId;


/**
 * @brief  print project related string.
 *
 * @param none.
 * @return print project related string.
*/
char *User_CmdProjectInfo(void)
{
    static char *pStr;
    pStr = "ble_auto_test";
    return (pStr);
}

static T_USER_CMD_PARSE_RESULT cmd_reset(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if UPPER_STACK_USE_VIRTUAL_HCI
    WDG_SystemReset(RESET_ALL);
    return (RESULT_SUCESS);
#else
    //NVIC_SystemReset();
    return RESULT_CMD_NOT_SUPPORT;
#endif
}

static T_USER_CMD_PARSE_RESULT cmd_dlps_mode(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if F_BT_DLPS_EN
    app_set_dlps_mode(p_parse_value);
    return (RESULT_SUCESS);
#else
    return RESULT_ERR;
#endif
}

static T_USER_CMD_PARSE_RESULT UserCmdSelectTestCase(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_select_cur_test_case(p_parse_value);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT user_cmd_tc_dump(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_dump_cur_test_case(p_parse_value);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT user_cmd_tc_rembd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_USER_CMD_PARSE_RESULT result =  RESULT_SUCESS;
    bool ret;

    ret = app_set_rembd(p_parse_value);
    if (ret == false)
    {
        result = RESULT_ERR;
    }
    return result;
}

static T_USER_CMD_PARSE_RESULT user_cmd_tc_role(T_USER_CMD_PARSED_VALUE *p_parse_value)
{

    T_CUR_DEVICE_ROLE role;
    role = (T_CUR_DEVICE_ROLE)p_parse_value->dw_param[0];
    app_set_cur_tc_role(role);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT user_cmd_tc_add(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint32_t count = 10;
    if (p_parse_value->param_count >= 1)
    {
        count = p_parse_value->dw_param[0];
    }

    tc_add_case(count);
    return (RESULT_SUCESS);
}

extern void app_tc_add(T_USER_CMD_PARSED_VALUE *p_parse_value);
static T_USER_CMD_PARSE_RESULT user_cmd_tc_tcqueue(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_tc_add(p_parse_value);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT user_cmd_tc_auto(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    tc_start_next_case();
    return (RESULT_SUCESS);
}


static T_USER_CMD_PARSE_RESULT UserCmdLogOn(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 1);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 1);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_WARN, 1);
    data_uart_print("Log On Sucess\r\n");
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdLogOff(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 0);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 0);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_WARN, 0);
    data_uart_print("Log Off Sucess\r\n");
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_showcon(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = 0;
    T_GAP_CONN_INFO conn_info;
    for (conn_id = 0; conn_id < 4; conn_id++)
    {
        if (le_get_conn_info(conn_id, &conn_info))
        {
            data_uart_print("ShowCon conn_id = %d state = 0x%x role = %d\r\n", conn_id,
                            conn_info.conn_state, conn_info.role);
            data_uart_print("RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
                            conn_info.remote_bd[5], conn_info.remote_bd[4],
                            conn_info.remote_bd[3], conn_info.remote_bd[2],
                            conn_info.remote_bd[1], conn_info.remote_bd[0],
                            conn_info.remote_bd_type);
        }
    }
    return (RESULT_SUCESS);
}
#if F_BT_LE_GAP_CENTRAL_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_con(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t DestAddr[6] = {0};
    uint8_t addr_len;
    uint8_t DestAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 80;
    conn_req_param.conn_interval_max = 80;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 1000;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    data_uart_print("cmd_con\r\n");

    for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
    {
        DestAddr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1];
    }
    if (p_parse_value->param_count >= 7)
    {
        DestAddrType = p_parse_value->dw_param[6];
    }

    le_connect(0, DestAddr, (T_GAP_REMOTE_ADDR_TYPE)DestAddrType, GAP_LOCAL_ADDR_LE_PUBLIC,
               1000);

    return (RESULT_SUCESS);
}



static T_USER_CMD_PARSE_RESULT cmd_condev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    data_uart_print("UserCmdConnect\r\n");
    if (p_parse_value->dw_param[0] < dev_list_count)
    {
        T_GAP_LE_CONN_REQ_PARAM conn_req_param;

        conn_req_param.scan_interval = 0x10;
        conn_req_param.scan_window = 0x10;
        conn_req_param.conn_interval_min = 80;
        conn_req_param.conn_interval_max = 80;
        conn_req_param.conn_latency = 0;
        conn_req_param.supv_tout = 1000;
        conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
        conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

        le_connect(0, dev_list[p_parse_value->dw_param[0]].bd_addr,
                   (T_GAP_REMOTE_ADDR_TYPE)dev_list[p_parse_value->dw_param[0]].bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC,
                   1000);
    }
    else
    {
        return RESULT_ERR;
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_chanclassset(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t chanMap[5] = {0};
    uint8_t i;
    for (i = 0; i < 5; i++)
    {
        chanMap[i] = (uint8_t)p_parse_value->dw_param[i];
    }
    chanMap[4] = chanMap[4] & 0x1F;

    if (le_set_host_chann_classif(chanMap) == GAP_CAUSE_ERROR_UNKNOWN)
    {
        data_uart_print("cmd_chanclassset Fail!!!\r\n");
        return (RESULT_ERR);
    }

    return (RESULT_SUCESS);
}
#endif
static T_USER_CMD_PARSE_RESULT cmd_disc(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_disconnect(p_parse_value->dw_param[0]);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_iocapset(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t ioCap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    if (p_parse_value->param_count >= 1)
    {
        switch (p_parse_value->dw_param[0])
        {
        case 0:
            ioCap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
            data_uart_print("GAP_IO_CAP_NO_INPUT_NO_OUTPUT\r\n");
            break;
        case 1:
            ioCap = GAP_IO_CAP_DISPLAY_ONLY;
            data_uart_print("GAP_IO_CAP_DISPLAY_ONLY\r\n");
            break;
        case 2:
            ioCap = GAP_IO_CAP_KEYBOARD_ONLY;
            data_uart_print("GAP_IO_CAP_KEYBOARD_ONLY\r\n");
            break;
        case 3:
            ioCap = GAP_IO_CAP_DISPLAY_YES_NO;
            data_uart_print("GAP_IO_CAP_DISPLAY_YES_NO\r\n");
            break;
        case 4:
            ioCap = GAP_IO_CAP_KEYBOARD_DISPLAY;
            data_uart_print("GAP_IO_CAP_KEYBOARD_DISPLAY\r\n");
            break;
        default:
            data_uart_print("GAP_IO_CAP parameter error!!!\r\n");
            break;
        }
    }
    else
    {
        data_uart_print("GAP_IO_CAP_NO_INPUT_NO_OUTPUT\r\n");
    }
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
    gap_set_pairable_mode();

    return (RESULT_SUCESS);
}

#if F_BT_LE_GAP_SCAN_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_scan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t scan_mode = GAP_SCAN_MODE_PASSIVE;
    uint8_t scan_filter_policy = GAP_SCAN_FILTER_ANY;
    if (p_parse_value->param_count > 0)
    {
        scan_mode = p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        scan_filter_policy = p_parse_value->dw_param[1];
    }
    link_mgr_clear_device_list();
    le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                      &scan_filter_policy);
    le_scan_start();
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_stopscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_scan_stop();
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_showdev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t i;
    data_uart_print("Advertising and Scan response: filter uuid = 0xA00A dev list\r\n");
    for (i = 0; i < dev_list_count; i++)
    {
        data_uart_print("RemoteBd[%d] = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n", i,
                        dev_list[i].bd_addr[5], dev_list[i].bd_addr[4],
                        dev_list[i].bd_addr[3], dev_list[i].bd_addr[2],
                        dev_list[i].bd_addr[1], dev_list[i].bd_addr[0],
                        dev_list[i].bd_type);
    }

    return (RESULT_SUCESS);
}
#endif
static T_USER_CMD_PARSE_RESULT cmd_sauth(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_bond_pair(p_parse_value->dw_param[0]);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdUpdateConnParam(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint16_t desired_min_interval = p_parse_value->dw_param[1];
    uint16_t desired_max_interval = p_parse_value->dw_param[2];
    uint16_t desired_slave_latency = p_parse_value->dw_param[3];
    uint16_t desired_conn_timeout = p_parse_value->dw_param[4];

    le_update_conn_param(p_parse_value->dw_param[0],
                         desired_min_interval,
                         desired_max_interval,
                         desired_slave_latency,
                         desired_conn_timeout,
                         2 * (desired_min_interval - 1),
                         2 * (desired_min_interval - 1)
                        );
    return (RESULT_SUCESS);
}
static T_USER_CMD_PARSE_RESULT UserCmdAddToWhitelist(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CONN_INFO conn_info;
    if (conn_id >= APP_MAX_LINKS)
    {
        return RESULT_ERR;
    }
    le_get_conn_info(conn_id, &conn_info);
    le_modify_white_list(GAP_WHITE_LIST_OP_ADD, conn_info.remote_bd,
                         (T_GAP_REMOTE_ADDR_TYPE)conn_info.remote_bd_type);
    return (RESULT_SUCESS);

}
#if F_BT_LE_GATT_CLIENT_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_gapdis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    gaps_start_discovery(p_parse_value->dw_param[0]);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_gapread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    /* Indicate which char to be read. */
    T_GAPS_READ_TYPE read_char_type = (T_GAPS_READ_TYPE)p_parse_value->dw_param[1];


    gaps_read(p_parse_value->dw_param[0], read_char_type);

    return (RESULT_SUCESS);
}
#endif
static T_USER_CMD_PARSE_RESULT UserCmdTxPowerSet(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if 0
    uint8_t type = p_parse_value->dw_param[0];
    uint8_t tx_gain = p_parse_value->dw_param[1];


    le_adv_set_tx_power((T_LE_ADV_TX_POW_TYPE)type, tx_gain);
#endif
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_rssiread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{

    le_read_rssi(p_parse_value->dw_param[0]);

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdSetLocalType(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t local_bd_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    if (p_parse_value->dw_param[0] > 3)
    {
        return RESULT_ERR;
    }
    local_bd_type = p_parse_value->dw_param[0];
    if (local_bd_type == GAP_LOCAL_ADDR_LE_RANDOM)
    {
        uint8_t random_addr[6] = {0};
        if (p_parse_value->dw_param[1] == GAP_RAND_ADDR_STATIC)
        {
            random_addr[0] = 0x4E;
            random_addr[1] = 0xEF;
            random_addr[2] = 0x3B;
            random_addr[3] = 0x58;
            random_addr[4] = 0xFA;
            random_addr[5] = 0xFF;
        }
        else
        {
            le_gen_rand_addr((T_GAP_RAND_ADDR_TYPE)p_parse_value->dw_param[1], random_addr);
        }
        data_uart_print("set local random addr:0x%02x%02x%02x%02x%02x%02x\r\n",
                        random_addr[5],
                        random_addr[4],
                        random_addr[3],
                        random_addr[2],
                        random_addr[1],
                        random_addr[0]);
        le_set_rand_addr(random_addr);
    }

    le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
    le_scan_set_param(GAP_PARAM_SCAN_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdSetRandom(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t random_addr[6] = {0};
    if (p_parse_value->dw_param[0] > 2)
    {
        return RESULT_ERR;
    }
    le_gen_rand_addr((T_GAP_RAND_ADDR_TYPE)p_parse_value->dw_param[0], random_addr);
    le_set_rand_addr(random_addr);
    return (RESULT_SUCESS);
}
#if F_BT_LE_READ_CHANN_MAP
static T_USER_CMD_PARSE_RESULT UserCmdReadChanMap(T_USER_CMD_PARSED_VALUE *p_parse_value)
{

    le_read_chann_map(p_parse_value->dw_param[0]);

    return (RESULT_SUCESS);
}
#endif
static T_USER_CMD_PARSE_RESULT UserCmdPatchVerGet(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //uint16_t patch_version = get_image_version(SIGNATURE_PATCH_FLASH);

    //p_user_cmd_if->cPrompt);

    return (RESULT_SUCESS);
}
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
static T_USER_CMD_PARSE_RESULT UserCmdSetDataLength(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint16_t txOctets = p_parse_value->dw_param[1];
    uint16_t txTime = p_parse_value->dw_param[2];

    le_set_data_len(p_parse_value->dw_param[0],
                    txOctets,
                    txTime);
    return (RESULT_SUCESS);
}
#endif
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
static T_USER_CMD_PARSE_RESULT UserCmdReadAdvPower(T_USER_CMD_PARSED_VALUE *p_parse_value)
{

    le_adv_read_tx_power();

    return (RESULT_SUCESS);
}
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
static T_USER_CMD_PARSE_RESULT cmd_latency(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_disable_slave_latency(p_parse_value->dw_param[0], p_parse_value->dw_param[1]);
    return (RESULT_SUCESS);
}
#endif
static T_USER_CMD_PARSE_RESULT UserCmdLpsCmd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if 0
    bool lpscmd = (bool)p_parse_value->dw_param[0];
    uint8_t *yesStr = "yes";
    uint8_t *noStr = "no";

    dataUARTCmdPrint(p_user_cmd_if, "UserCmdLpsCmd: enable DLPS? %s\r\n",
                     (lpscmd == 0) ? noStr : yesStr);

    if (lpscmd)
    {
        LPS_MODE_Resume();
    }
    else
    {
        LPS_MODE_Pause();
    }
#endif
    return (RESULT_SUCESS);
}



static T_USER_CMD_PARSE_RESULT cmd_authmode(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //configure parameter first
    uint8_t pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_MITM_FLAG;
    uint8_t io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t oob_enable = false;
    uint8_t secReqEnable = false;
    uint16_t secReqRequirement = GAP_AUTHEN_BIT_MITM_FLAG;

    if (p_parse_value->param_count > 0)
    {
        auth_flags = p_parse_value->dw_param[0];
        secReqRequirement = p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        io_cap = p_parse_value->dw_param[1];
    }
    if (p_parse_value->param_count > 2)
    {
        secReqEnable = p_parse_value->dw_param[2];
    }
    if (p_parse_value->param_count > 3)
    {
        oob_enable = p_parse_value->dw_param[3];
    }
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(uint8_t), &pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(uint16_t), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(uint8_t), &io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);
#endif
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &secReqRequirement);
    gap_set_pairable_mode();
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdGetBondDevInfo(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LE_KEY_ENTRY *p_entry = le_get_high_priority_bond();
    if (p_entry != NULL)
    {
        data_uart_print("UserCmdGetBondDevInfo: 0x%02x%02x%02x%02x%02x%02x, addr_type:%d\r\n",
                        p_entry->remote_bd.addr[5],
                        p_entry->remote_bd.addr[4],
                        p_entry->remote_bd.addr[3],
                        p_entry->remote_bd.addr[2],
                        p_entry->remote_bd.addr[1],
                        p_entry->remote_bd.addr[0],
                        p_entry->remote_bd.remote_bd_type);
    }
    else
    {
        data_uart_print("UserCmdGetBondDevInfo no exist\r\n");
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdEraseBondDev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_bond_clear_all_keys();

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_userconf(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CFM_CAUSE cause = GAP_CFM_CAUSE_ACCEPT;
    if (p_parse_value->dw_param[1] == 0)
    {
        cause = GAP_CFM_CAUSE_REJECT;
    }
    le_bond_user_confirm(p_parse_value->dw_param[0], cause);

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_authkey(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_bond_passkey_input_confirm(p_parse_value->dw_param[0], p_parse_value->dw_param[1],
                                  GAP_CFM_CAUSE_ACCEPT);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdInitStartAdv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_adv_start();
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_adv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //advertising parameters
    T_GAP_CAUSE cause;
    uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;
    uint16_t advIntMin = (uint16_t)p_parse_value->dw_param[0];
    uint16_t advIntMax = (uint16_t)p_parse_value->dw_param[1];

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    cause = le_adv_start();
    if (cause == GAP_CAUSE_SUCCESS)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        data_uart_print("cause %d\r\n", cause);
        return RESULT_ERR;
    }
}

static T_USER_CMD_PARSE_RESULT cmd_stopadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_adv_stop();

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_advld(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t  advEventType = GAP_ADTYPE_ADV_LDC_DIRECT_IND;
    uint8_t  advDirectAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  advDirectAddr[GAP_BD_ADDR_LEN] = {0};

    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint16_t advIntMin = 0x80;
    uint16_t advIntMax = 0x80;


    //get connected device address and address type
    le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR_TYPE, &advDirectAddrType);
    le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR, &advDirectAddr);

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(advDirectAddrType),
                     &advDirectAddrType);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(advDirectAddr), advDirectAddr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_start();

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_advhd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t  advEventType = GAP_ADTYPE_ADV_HDC_DIRECT_IND;
    uint8_t  advDirectAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  advDirectAddr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t addr_len;


    for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
    {
        advDirectAddr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1];
    }

    //get connected device address and address type
    //le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR_TYPE, &whiteListAddrType);
    //le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR, &whiteListAddr);

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(advDirectAddrType),
                     &advDirectAddrType);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(advDirectAddr), advDirectAddr);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);

    le_adv_start();

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_advscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //advertising parameters
    uint8_t  advEventType = GAP_ADTYPE_ADV_SCAN_IND;
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;
    uint16_t advIntMin = 0x80;
    uint16_t advIntMax = 0x80;
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_start();

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_advnonconn(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //advertising parameters
    uint8_t  advEventType = GAP_ADTYPE_ADV_NONCONN_IND;
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;
    uint16_t advIntMin = 0x80;
    uint16_t advIntMax = 0x80;
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_start();

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdStartAdvWithWL(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //advertising parameters
    uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_WHITE_LIST_ALL;
    uint16_t advIntMin = 0x80;
    uint16_t advIntMax = 0x80;

    T_GAP_REMOTE_ADDR_TYPE  whiteListAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  whiteListAddr[GAP_BD_ADDR_LEN] = {0};

    //get connected device address and address type
    le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR_TYPE, &whiteListAddrType);
    le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR, &whiteListAddr);

    le_modify_white_list(GAP_WHITE_LIST_OP_ADD, whiteListAddr, whiteListAddrType);
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_start();

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdStartAdvSet(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //advertising parameters
    uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;
    uint16_t advIntMin = 0x80;
    uint16_t advIntMax = 0x80;


    //get connected device address and address type
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_update_param();

    return (RESULT_SUCESS);
}
#if F_BT_LE_PRIVACY_SUPPORT
static T_USER_CMD_PARSE_RESULT UserCmdStartAdvRel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t  advDirectType = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  advDirectAddr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;

    if (p_parse_value->param_count > 0)
    {
        advEventType = p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        advFilterPolicy = p_parse_value->dw_param[1];
    }
    if (p_parse_value->param_count > 2)
    {
        uint8_t idx = p_parse_value->dw_param[2];
        if (privacy_table[idx].is_used == true)
        {
            advDirectType = privacy_table[idx].remote_bd_type;
            memcpy(advDirectAddr, privacy_table[idx].addr, 6);
        }
        else
        {
            return RESULT_CMD_ERR_PARAM;
        }

        advFilterPolicy = p_parse_value->dw_param[2];
    }

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(advDirectType), &advDirectType);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(advDirectAddr), advDirectAddr);

    le_adv_start();
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdReadlra(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];

    if (privacy_table[idx].is_used)
    {
        le_privacy_read_local_resolv_addr((T_GAP_IDENT_ADDR_TYPE)privacy_table[idx].remote_bd_type,
                                          privacy_table[idx].addr);
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);

}

static T_USER_CMD_PARSE_RESULT UserCmdReadpra(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];

    if (privacy_table[idx].is_used)
    {
        le_privacy_read_peer_resolv_addr(privacy_table[idx].remote_bd_type,
                                         privacy_table[idx].addr);
        return (RESULT_SUCESS);
    }
    return (RESULT_ERR);
}

static T_USER_CMD_PARSE_RESULT UserCmdSetrea(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool enable = true;
    if (p_parse_value->dw_param[0] == 0)
    {
        enable = false;
    }
    le_privacy_set_addr_resolution(enable);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdShowRel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t i;
    data_uart_print("show resolved list\r\n");

    for (i = 0; i < PRIVACY_ENTRY_SIZE; i++)
    {
        if (privacy_table[i].is_used)
        {
            data_uart_print("rel[%d] RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d is_add_to_list=%d pending=%d\r\n",
                            i,
                            privacy_table[i].addr[5], privacy_table[i].addr[4],
                            privacy_table[i].addr[3], privacy_table[i].addr[2],
                            privacy_table[i].addr[1], privacy_table[i].addr[0],
                            privacy_table[i].remote_bd_type,
                            privacy_table[i].is_add_to_list,
                            privacy_table[i].pending);
        }
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdSetPrivacy(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];
    uint8_t mode = p_parse_value->dw_param[1];
    data_uart_print("show resolved list\r\n");
    if (privacy_table[idx].is_used)
    {
        le_privacy_set_mode((T_GAP_IDENT_ADDR_TYPE)privacy_table[idx].remote_bd_type,
                            privacy_table[idx].addr,
                            (T_GAP_PRIVACY_MODE)mode);
    }
    else
    {
        return RESULT_CMD_ERR_PARAM;
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdAddRelToWhitelist(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_WHITE_LIST_OP op = (T_GAP_WHITE_LIST_OP)p_parse_value->dw_param[0];
    uint8_t idx = p_parse_value->dw_param[1];
    if (op == GAP_WHITE_LIST_OP_CLEAR)
    {
        le_modify_white_list(op, NULL, GAP_REMOTE_ADDR_LE_PUBLIC);
    }
    else
    {
        if (privacy_table[idx].is_used)
        {
            le_modify_white_list(op, privacy_table[idx].addr,
                                 (T_GAP_REMOTE_ADDR_TYPE)privacy_table[idx].remote_bd_type);
        }
        else
        {
            return RESULT_CMD_ERR_PARAM;
        }
    }
    return (RESULT_SUCESS);

}

static T_USER_CMD_PARSE_RESULT UserCmdConnectRelDev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];

    if (privacy_table[idx].is_used)
    {
        T_GAP_LE_CONN_REQ_PARAM conn_req_param;

        conn_req_param.scan_interval = 0x10;
        conn_req_param.scan_window = 0x10;
        conn_req_param.conn_interval_min = 80;
        conn_req_param.conn_interval_max = 80;
        conn_req_param.conn_latency = 0;
        conn_req_param.supv_tout = 1000;
        conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
        conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
        le_connect(0, privacy_table[idx].addr,
                   (T_GAP_REMOTE_ADDR_TYPE)privacy_table[idx].remote_bd_type,
                   GAP_LOCAL_ADDR_LE_PUBLIC,
                   1000);
    }
    else
    {
        return RESULT_ERR;
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdSetLocalIrk(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_LOCAL_IRK irk;
    uint8_t type = p_parse_value->dw_param[0];

    if (type == 0)

    {
        memset(irk.local_irk, 0, 16);
    }
    else if (type == 1)
    {
        memset(irk.local_irk, 0x02, 16);
    }
    else
    {
        uint8_t i = 0;
        for (i = 0; i < 16; i++)
        {
            irk.local_irk[i] = i;
        }
    }
    flash_save_local_irk(&irk);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdSetRemoteIrk(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if 0
    T_LE_KEY_ENTRY *p_entry = NULL;
    //uint8_t                 remote_BD[6] = {0xEF, 0x24, 0x45, 0x67, 0x89, 0xAB};
    uint8_t                 remote_BD[6] = {0xAB, 0x89, 0x67, 0x45, 0x24, 0xEF};
    T_GAP_REMOTE_ADDR_TYPE   remote_BD_Type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_LE_REMOTE_IRK p_irk;
    p_entry = le_allocate_key_entry(remote_BD, remote_BD_Type);
#if 0
    p_irk.addr[0] = 0xEF;
    p_irk.addr[1] = 0x24;
    p_irk.addr[2] = 0x45;
    p_irk.addr[3] = 0x67;
    p_irk.addr[4] = 0x89;
    p_irk.addr[5] = 0xAB;
#endif
    p_irk.addr[0] = 0xAB;
    p_irk.addr[1] = 0x89;
    p_irk.addr[2] = 0x67;
    p_irk.addr[3] = 0x45;
    p_irk.addr[4] = 0x24;
    p_irk.addr[5] = 0xEF;
    p_irk.addr_type = 0;
    //memset(p_irk.key, 0x2,16);
    p_irk.key[0] = 0x00;
    p_irk.key[1] = 0x11;
    p_irk.key[2] = 0x22;
    p_irk.key[3] = 0x33;
    p_irk.key[4] = 0x44;
    p_irk.key[5] = 0x55;
    p_irk.key[6] = 0x66;
    p_irk.key[7] = 0x77;
    p_irk.key[8] = 0x88;
    p_irk.key[9] = 0x99;
    p_irk.key[10] = 0xAA;
    p_irk.key[11] = 0xBB;
    p_irk.key[12] = 0xCC;
    p_irk.key[13] = 0xDD;
    p_irk.key[14] = 0xEE;
    p_irk.key[15] = 0xFF;

    if (p_entry != NULL)
    {
        le_save_key(p_entry, BTIF_LINK_KEY_LE_REMOTE_IRK, 24,
                    (uint8_t *)&p_irk);
    }
    else
    {
        return RESULT_ERR;
    }
#endif
    return (RESULT_SUCESS);
}
#endif
#if F_BT_LE_GATT_CLIENT_SUPPORT
static T_USER_CMD_PARSE_RESULT UserCmdTpDis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    tp_client_start_discovery(conn_id);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdTpWrite(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t data[270];
    uint16_t length = 10;
    uint16_t i;
    if (p_parse_value->param_count > 1)
    {
        length = p_parse_value->dw_param[1];
    }
    for (i = 0; i < length; i++)
    {
        data[i] = i;
    }
    tp_client_write_value(p_parse_value->dw_param[0], length, data);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT UserCmdPxpDis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    vendor_pxpext_start_discovery(conn_id);
    return (RESULT_SUCESS);
}
static T_USER_CMD_PARSE_RESULT UserCmdPxpWrite(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t data[270];
    uint16_t length = 10;
    uint16_t i;
    if (p_parse_value->param_count > 1)
    {
        length = p_parse_value->dw_param[1];
    }
    for (i = 0; i < length; i++)
    {
        data[i] = i;
    }
    vendor_pxpext_write_value(p_parse_value->dw_param[0], length, data);
    return (RESULT_SUCESS);
}
static T_USER_CMD_PARSE_RESULT UserCmdPxpRead(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    vendor_pxpext_read_value(conn_id);
    return (RESULT_SUCESS);
}
#endif
/*----------------------------------------------------
 * command table
 * --------------------------------------------------*/
const T_USER_CMD_TABLE_ENTRY user_cmd_table[] =
{
    /********************************Common*********************************/

    {
        "reset",
        "reset\n\r",
        "reset\n\r",
        cmd_reset
    },

    {
        "dlps",
        "dlps [mode]\n\r",
        "dlps\n\r",
        cmd_dlps_mode
    },

    {
        "tc",
        "testcase [index]\n\r",
        "testcase\n\r",
        UserCmdSelectTestCase
    },
    {
        "tcrole",
        "tcrole [role]\n\r",
        "tcrole\n\r",
        user_cmd_tc_role
    },
    {
        "tcdump",
        "tcdump [index]\n\r",
        "tcdump\n\r",
        user_cmd_tc_dump
    },

    {
        "rembd",
        "rembd [index]\n\r",
        "rembd\n\r",
        user_cmd_tc_rembd
    },



    {
        "tcadd",
        "tcadd [index]\n\r",
        "tcadd\n\r",
        user_cmd_tc_add
    },
    {
        "tcqueue",
        "tcqueue [index]\n\r",
        "tcqueue\n\r",
        user_cmd_tc_tcqueue
    },
    {
        "tcauto",
        "tcauto [index]\n\r",
        "tcauto\n\r",
        user_cmd_tc_auto
    },

    {
        "logon",
        "logon\n\r",
        "logon\n\r",
        UserCmdLogOn
    },
    {
        "logoff",
        "logoff\n\r",
        "logoff\n\r",
        UserCmdLogOff
    },
    {
        "patchver",
        "patchver\n\r",
        "Get the patch Version\n\r",
        UserCmdPatchVerGet
    },
    {
        "txpwrset",
        "txpwrset [txpwridx]\n\r",
        "Set the TX Power of BLE RF\n\r",
        UserCmdTxPowerSet
    },
    {
        "rssiread",
        "rssiread [conn_id]\n\r",
        "Read the RSSI value of this local MDL ID\n\r",
        cmd_rssiread
    },
    {
        "setrandom",
        "setrandom [type]\n\r",
        "set random address type: 0-static, 1-non-resolved, 2-resolved\n\r",
        UserCmdSetRandom
    },
    {
        "setlocaltype",
        "setlocaltype [type]\n\r",
        "set local address type: 0-public, 1-random, 2-resolved or public 3- resolved or random\n\r",
        UserCmdSetLocalType
    },
#if F_BT_LE_READ_CHANN_MAP
    {
        "readchanmap",
        "readchanmap [conn_id]\n\r",
        "read channel map\n\r",
        UserCmdReadChanMap
    },
#endif
    {
        "addwl",
        "addwl [conn_id]\n\r",
        "add remote BD to whitelist\n\r",
        UserCmdAddToWhitelist
    },
    {
        "dlps",
        "dlps [cmd]\n\r",
        "Set dlps mode\n\r",
        UserCmdLpsCmd
    },
    {
        "showcon",
        "showcon\n\r",
        "Show all device connecting status\n\r",
        cmd_showcon
    },
    {
        "disc",
        "disc [conn_id]\n\r",
        "disconnect to remote device\n\r",
        cmd_disc
    },
    {
        "authmode",
        "authmode [auth_flags] [io_cap] [sec enable]\n\r",
        "set auth mode\n\r",
        cmd_authmode
    },
    {
        "iocapset",
        "iocapset [iocap]\n\r",
        "choose io capability.\n\r",
        cmd_iocapset
    },
    {
        "sauth",
        "sauth [conn_id]\n\r",
        "bond manager authentication request.\n\r",
        cmd_sauth
    },
    {
        "bonddevget",
        "bonddevget\n\r",
        "Get Bonded device information\n\r",
        UserCmdGetBondDevInfo
    },
    {
        "bonddeverase",
        "bonddeverase\n\r",
        "Erase Bonded device information\n\r",
        UserCmdEraseBondDev
    },
    {
        "userconf",
        "userconf [conn_id] [0/1]\n\r",
        "Send user confirmation 0-reject 1-accept\n\r",
        cmd_userconf
    },
    {
        "authkey",
        "authkey [conn_id] [key]\n\r",
        "input key\n\r",
        cmd_authkey
    },
    {
        "conupdreq",
        "conupdreq [conn_id] [Interval_min] [Interval_max] [latency] [suptimeout]\n\r",
        "Update connection parameter\n\r",
        UserCmdUpdateConnParam
    },
    {
        "addwl",
        "addwl [conn_id]\n\r",
        "add remote BD to whitelist\n\r",
        UserCmdAddToWhitelist
    },
    /********************************Central*********************************/
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    {
        "setdatalength",
        "setdatalength [conn_id] [tx octests] [tx time]\n\r",
        "set data length\n\r",
        UserCmdSetDataLength
    },
#endif
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
    {
        "readadvpwr",
        "readadvpwr\n\r",
        "Read adv channel tx power\n\r",
        UserCmdReadAdvPower
    },
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    {
        "latency",
        "latency [0/1]\n\r",
        "on off latency\n\r",
        cmd_latency
    },
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    {
        "con",
        "con [BD0] [BD1] [BD2] [BD3] [BD4] [BD5] [bd_addr type]\n\r",
        "connect to remote device\n\r",
        cmd_con
    },
    {
        "condev",
        "condev [idx]\n\r",
        "connect to remote device\n\r",
        cmd_condev
    },
    {
        "chanclassset",
        "chanclassset [idx0] [idx1] [idx2] [idx3] [idx4]\n\r",
        "Set Host Channel Classification\n\r",
        cmd_chanclassset
    },
#endif
#if F_BT_LE_GAP_SCAN_SUPPORT
    {
        "scan",
        "scan [1/2] [filter]\n\r",
        "scan remote devices 1-passive 2-active\n\r",
        cmd_scan
    },

    {
        "stopscan",
        "stopscan\n\r",
        "stopscan remote device\n\r",
        cmd_stopscan
    },
    {
        "showdev",
        "showdev\n\r",
        "Show simple dev list\n\r",
        cmd_showdev
    },
#endif
#if F_BT_LE_GATT_CLIENT_SUPPORT
    {
        "gapdis",
        "gapdis [conn_id]\n\r",
        "Start discovery gap service\n\r",
        cmd_gapdis
    },
    {
        "gapread",
        "gapread [conn_id] [char]\n\r",
        "Read all related chars by user input\n\r",
        cmd_gapread
    },
#endif
    {
        "initadv",
        "initadv\n\r",  // minInterval: 0x0020 - 0x4000 (20ms - 10240ms, 0.625ms/step)
        "Init advertising\n\r",     // maxInterval: 0x0020 - 0x4000 (20ms - 10240ms, 0.625ms/step)
        UserCmdInitStartAdv
    },
    {
        "adv",
        "adv [min_interval] [max_interval]\n\r",  // min_interval: 0x0020 - 0x4000 (20ms - 10240ms, 0.625ms/step)
        "Start Undirected advertising\n\r",     // max_interval: 0x0020 - 0x4000 (20ms - 10240ms, 0.625ms/step)
        cmd_adv
    },
    {
        "stopadv",
        "stopadv\n\r",
        "Stop advertising\n\r",
        cmd_stopadv
    },
    {
        "advld",
        "advld\n\r",
        "Start lower duty directed advertising\n\r",
        cmd_advld
    },
    {
        "advhd",
        "advhd [index of BD]\n\r",
        "Start high duty directed advertising\n\r",
        cmd_advhd
    },
    {
        "advscan",
        "advscan\n\r",
        "Start scannable undirected advertising\n\r",
        cmd_advscan
    },
    {
        "advnonconn",
        "advnonconn\n\r",
        "Start non_connectable undirected advertising\n\r",
        cmd_advnonconn
    },
    {
        "advwl",
        "advwl\n\r",
        "Start undirected advertising with white list\n\r",
        UserCmdStartAdvWithWL
    },
    {
        "advset",
        "advset [flag]\n\r",
        "set adv param\n\r",
        UserCmdStartAdvSet
    },
#if F_BT_LE_PRIVACY_SUPPORT
    {
        "advrel",
        "advrel\n\r",
        "Start undirected advertising with local type = resolved\n\r",
        UserCmdStartAdvRel
    },
    {
        "setrae",
        "setrae [enable]\n\r",
        "set addr resolution enable.\n\r",
        UserCmdSetrea
    },
    {
        "readlra",
        "readlra [conn_id]\n\r",
        "read local resolvable address.\n\r",
        UserCmdReadlra
    },
    {
        "readpra",
        "readlra [conn_id]\n\r",
        "read peer resolvable address..\n\r",
        UserCmdReadpra
    },
    {
        "showrel",
        "showrel\n\r",
        "show resolved list.\n\r",
        UserCmdShowRel
    },
    {
        "setprivacy",
        "setprivacy [rel_idx] [0/1]\n\r",
        "set privacy mode.\n\r",
        UserCmdSetPrivacy
    },
    {
        "wlrel",
        "wlrel [op] [rel_idx]\n\r",
        "whitelist\n\r",
        UserCmdAddRelToWhitelist
    },
    {
        "conrel",
        "conrel [idx]\n\r",
        "connect to remote device\n\r",
        UserCmdConnectRelDev
    },
    {
        "setlocalirk",
        "setlocalirk [0/1/2]\n\r",
        "set local irk\n\r",
        UserCmdSetLocalIrk
    },
    {
        "setremoteirk",
        "setremoteirk\n\r",
        "set remote irk\n\r",
        UserCmdSetRemoteIrk
    },
#endif
#if F_BT_LE_GATT_CLIENT_SUPPORT
    {
        "tpdis",
        "tpdis [conn_id]\n\r",
        "vendor tp service discovery.\n\r",
        UserCmdTpDis
    },
    {
        "tpwrite",
        "tpwrite [conn_id]\n\r",
        "vendor tp service write char.\n\r",
        UserCmdTpWrite
    },
    {
        "pxpdis",
        "pxpdis [conn_id]\n\r",
        "vendor pxpext service discovery.\n\r",
        UserCmdPxpDis
    },
    {
        "pxpwrite",
        "pxpwrite [conn_id]\n\r",
        "vendor pxpext service write char.\n\r",
        UserCmdPxpWrite
    },
    {
        "pxpread",
        "pxpread [conn_id]\n\r",
        "vendor pxpext service discovery.\n\r",
        UserCmdPxpRead
    },
#endif
    /********************************Peripheral*********************************/
    /* MUST be at the end: */
    {
        0,
        0,
        0,
        0
    }
};
