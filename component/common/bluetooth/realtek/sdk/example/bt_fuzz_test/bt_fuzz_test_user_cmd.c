/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      user_cmd.c
   * @brief     User defined test commands.
   * @details  User command interfaces.
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
#if defined(CONFIG_BT_FUZZ_TEST) && CONFIG_BT_FUZZ_TEST
#include <string.h>
#include <trace.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <bt_fuzz_test_user_cmd.h>
#include <gap.h>
#include <gap_conn_le.h>
#include <bt_fuzz_test_link_mgr.h>
#include <gap_adv.h>
#include <bt_fuzz_test_app.h>

/** @defgroup  FUZZ_TEST_CMD Fuzz Test User Command
    * @brief This file handles Fuzz Test User Command.
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @brief User command interface data, used to parse the commands from Data UART. */
T_USER_CMD_IF    bt_fuzz_test_user_cmd_if;

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief Show all devices connecting status
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "showcon",
        "showcon\n\r",
        "Show all devices connecting status\n\r",
        cmd_showcon
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_showcon(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id;
    T_GAP_CONN_INFO conn_info;
    for (conn_id = 0; conn_id < BT_FUZZ_TEST_APP_MAX_LINKS; conn_id++)
    {
        if (le_get_conn_info(conn_id, &conn_info))
        {
            data_uart_print("ShowCon conn_id %d state 0x%x role %d\r\n", conn_id,
                            conn_info.conn_state, conn_info.role);
            data_uart_print("RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
                            conn_info.remote_bd[5], conn_info.remote_bd[4],
                            conn_info.remote_bd[3], conn_info.remote_bd[2],
                            conn_info.remote_bd[1], conn_info.remote_bd[0],
                            conn_info.remote_bd_type);
        }
    }
    data_uart_print("active link num %d,  idle link num %d\r\n",
                    le_get_active_link_num(), le_get_idle_link_num());
    return (RESULT_SUCESS);
}

/**
 * @brief Disconnect to remote device
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "disc",
        "disc [conn_id]\n\r",
        "Disconnect to remote device\n\r",
        cmd_disc
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_disc(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;
    cause = le_disconnect(conn_id);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

/**
 * @brief Config authentication mode
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "ts",
        "ts [testsuite] [io_cap] [sec_enable] [start_pair] [read_handle] [write_handle]\n\r",
        "Set test configuration\r\n\
        [testsuite]: Index of testsuite: 0-(btle-smp), 1-(btle-smpc), 2-(btle-att), 3-(btle-attc)\r\n\
        [io_cap]: Set io Capabilities: 0-(display only), 1-(display yes/no), 2-(keyboard noly), 3-(no IO), 4-(keyboard display)\r\n\
        [sec_enable]: Enable secure connection flag: 0-(disable), 1-(enable)\r\n\
        [start_pair]: Start smp pairing procedure when connected: 0-(disable), 1-(enable)\r\n\
        [read_handle]: Handle to read\r\n\
        [write_handle]: Handle to write\r\n\
        sample: ts 3 3 0 0 x3 x5\n\r",
        cmd_testsuite
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_testsuite(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    testsuite = (T_FUZZ_TESTSUITE)p_parse_value->dw_param[0];
    uint8_t auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    T_GAP_IO_CAP auth_io_cap = (T_GAP_IO_CAP)p_parse_value->dw_param[1];
    bool sec_enable = (T_GAP_IO_CAP)p_parse_value->dw_param[2];
    m_start_pair = (T_GAP_IO_CAP)p_parse_value->dw_param[3];
    read_handle = p_parse_value->dw_param[4];
    write_handle = p_parse_value->dw_param[5];
    uint8_t  oob_enable = false;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    //uint8_t peer_addr[GAP_BD_ADDR_LEN] = {0};
    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    T_GAP_LOCAL_ADDR_TYPE local_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 80;
    conn_req_param.conn_interval_max = 80;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 1000;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    if (auth_io_cap != GAP_IO_CAP_NO_INPUT_NO_OUTPUT)
    {
        auth_flags |= GAP_AUTHEN_BIT_MITM_FLAG;
        auth_sec_req_flags |= GAP_AUTHEN_BIT_MITM_FLAG;
    }

    if (sec_enable)
    {
        auth_flags |= GAP_AUTHEN_BIT_SC_FLAG;
        auth_sec_req_flags |= GAP_AUTHEN_BIT_SC_FLAG;
    }

    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);
#endif
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);
    cause = gap_set_pairable_mode();

    switch (testsuite)
    {
    case FUZZ_TESTUITE_BTLE_SMP_510:
    case FUZZ_TESTUITE_BTLE_ATT_510:
    case FUZZ_TESTUITE_BTLE_PROFILES_510:
        le_adv_start();
        break;

    case FUZZ_TESTUITE_BTLE_SMPC_510:
    case FUZZ_TESTUITE_BTLE_ATTC_510:
    case FUZZ_TESTUITE_BTLE_HOGP_510:
    case FUZZ_TESTUITE_BTLE_HEALTH_510:
        //memcpy(peer_addr, g_cur_rembd, GAP_BD_ADDR_LEN);
        cause = le_connect(GAP_PHYS_CONN_INIT_1M_BIT,
                           g_cur_rembd,
                           remote_bd_type,
                           local_addr_type,
                           1000);
        break;

    case FUZZ_TESTUITE_BTLE_AD_510:
        le_scan_start();
        break;

    case FUZZ_TESTUITE_BTLE_RESERVER:
        le_adv_stop();
        le_disconnect(0);
        le_scan_stop();
        break;

    default:
        break;
    }

    return (T_USER_CMD_PARSE_RESULT)cause;
}

/**
 * @brief Clear all bonded devices information
 *
 * <b>Command table define</b>
 * \code{.c}
    {
        "bondclear",
        "bondclear\n\r",
        "Clear all bonded devices information\n\r",
        cmd_bondclear
    },
 * \endcode
 */
static T_USER_CMD_PARSE_RESULT cmd_bondclear(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    le_bond_clear_all_keys();
    return (RESULT_SUCESS);
}

/************************** Central only *************************************/
static T_USER_CMD_PARSE_RESULT cmd_rembd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    g_cur_rembd[0] = p_parse_value->dw_param[5];
    g_cur_rembd[1] = p_parse_value->dw_param[4];
    g_cur_rembd[2] = p_parse_value->dw_param[3];
    g_cur_rembd[3] = p_parse_value->dw_param[2];
    g_cur_rembd[4] = p_parse_value->dw_param[1];
    g_cur_rembd[5] = p_parse_value->dw_param[0];

    data_uart_print("g_cur_rembd: 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    g_cur_rembd[5], g_cur_rembd[4],
                    g_cur_rembd[3], g_cur_rembd[2],
                    g_cur_rembd[1], g_cur_rembd[0]);
    return (T_USER_CMD_PARSE_RESULT)(RESULT_SUCESS);
}

/*----------------------------------------------------
 * command table
 * --------------------------------------------------*/
const T_USER_CMD_TABLE_ENTRY bt_fuzz_test_user_cmd_table[] =
{
    /************************** Common cmd *************************************/
    {
        "showcon",
        "showcon\n\r",
        "Show all devices connecting status\n\r",
        cmd_showcon
    },
    {
        "disc",
        "disc [conn_id]\n\r",
        "Disconnect to remote device\n\r",
        cmd_disc
    },
    {
        "ts",
        "ts [testsuite] [io_cap] [sec_enable] [start_pair] [read_handle] [write_handle]\n\r",
        "Set test configuration\r\n\
        [testsuite]: Index of testsuite: 0-(btle-smp), 1-(btle-smpc), 2-(btle-att), 3-(btle-attc)\r\n\
        [io_cap]: Set io Capabilities: 0-(display only), 1-(display yes/no), 2-(keyboard noly), 3-(no IO), 4-(keyboard display)\r\n\
        [sec_enable]: Enable secure connection flag: 0-(disable), 1-(enable)\r\n\
        [start_pair]: Start smp pairing procedure when connected: 0-(disable), 1-(enable)\r\n\
        [read_handle]: Handle to read\r\n\
        [write_handle]: Handle to write\r\n\
        sample: ts 3 3 0 0 x3 x5\n\r",
        cmd_testsuite
    },
    {
        "bondclear",
        "bondclear\n\r",
        "Clear all bonded devices information\n\r",
        cmd_bondclear
    },
    /************************** Central only *************************************/
    {
        "rembd",
        "rembd [index]\n\r",
        "rembd\n\r",
        cmd_rembd
    },
    /* MUST be at the end: */
    {
        0,
        0,
        0,
        0
    }
};
#endif
/** @} */ /* End of group FUZZ_TEST_CMD */
