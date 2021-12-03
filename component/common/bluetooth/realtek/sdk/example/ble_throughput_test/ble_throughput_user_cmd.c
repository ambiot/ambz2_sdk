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
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST

#include <string.h>
#include "trace_app.h"
#if F_BT_LE_GATT_CLIENT_SUPPORT
#include "profile_client.h"
#include "ble_throughput_vendor_tp_client.h"
#include <gaps_client.h>
#endif
#include "gap_bond_le.h"
#include "gap_scan.h"
#include "ble_throughput_user_cmd.h"
#include "gap.h"
#include "ble_throughput_link_mgr.h"

#include "ble_throughput_app.h"
#include "gap_adv.h"
#include "gap_le.h"
#include "gap_conn_le.h"
#include "ble_throughput_test_case.h"
#include "gap_storage_le.h"
#include "user_cmd_parse.h"


T_USER_CMD_IF user_cmd_if;
extern uint8_t gSimpleProfileServiceId;


/**
 * @brief  print project related string.
 *
 * @param none.
 * @return print project related string.
*/

static T_USER_CMD_PARSE_RESULT UserCmdSelectTestCase(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    ble_throughput_app_select_cur_test_case(p_parse_value);
    return (RESULT_SUCESS);
}


static T_USER_CMD_PARSE_RESULT user_cmd_tc_rembd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_USER_CMD_PARSE_RESULT result =  RESULT_SUCESS;
    bool ret;

    ret = ble_throughput_app_set_rembd(p_parse_value);
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
    ble_throughput_app_set_cur_role(role);
    return (RESULT_SUCESS);
}


/*----------------------------------------------------
 * command table
 * --------------------------------------------------*/
const T_USER_CMD_TABLE_ENTRY user_cmd_table[] =
{
    /********************************Common*********************************/

    
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
        "rembd",
        "rembd [index]\n\r",
        "rembd\n\r",
        user_cmd_tc_rembd
    },
    
    /********************************Peripheral*********************************/
    /* MUST be at the end: */
    {
        0,
        0,
        0,
        0
    }
};
#endif


