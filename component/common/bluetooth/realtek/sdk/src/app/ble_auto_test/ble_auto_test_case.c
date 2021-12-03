
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_auto_test_application.h"

#include "user_cmd.h"
#include "user_cmd_parse.h"
#if F_BT_LE_GATT_SERVER_SUPPORT
#include "profile_server.h"
#endif

#include <ble_auto_test_case.h>
#if F_BT_LE_PRIVACY_SUPPORT
#include "privacy_mgnt.h"
#endif

#if F_BT_DLPS_EN
#include "dlps.h"
#include <data_uart_dlps.h>
#endif

T_CUR_TEST_CASE g_cur_test_case;

T_CUR_DEVICE_ROLE g_cur_test_role;

T_OS_QUEUE tc_q;




TGATTDBdAddr tc_rembd_list[] = /* first entry is default BD: */
{
    { 0x66, 0x66, 0xee, 0x22, 0x11, 0x00 },    /* FPGA v6-5 */
    { 0x3d, 0x19, 0x48, 0x25, 0x80, 0x00 },    /* Stollmann Board 1 */
    { 0xe3, 0x18, 0x48, 0x25, 0x80, 0x00 },    /* Stollmann Board 2 */
    { 0x01, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB1*/
    { 0x02, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB2*/
    { 0x03, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB3*/
    { 0x04, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB4*/
    { 0x05, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB5*/
    { 0x06, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB6*/
    { 0x07, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB7*/
    { 0x08, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB8*/
    { 0x09, 0x44, 0x33, 0x22, 0x11, 0x00 },    /* Bee1 EVB9*/

};

TGATTDBdAddr g_cur_rembd;

uint32_t os_time_get_elapsed(uint32_t begin, uint32_t end)
{
#if 1
    if (end > begin)
    {
        return end - begin;
    }
    else
    {
        return ((uint32_t)(2147483647) - begin + end);
    }
#else
    uint32_t expire_count;

    if (begin <= end)
    {
        expire_count =  end - begin;
    }
    else
    {
        expire_count = 0xFFFFFFFF - (begin - end);
    }
    return expire_count;
#endif
}

uint32_t amebad_time_get_elapsed(uint32_t begin, uint32_t end)
{
    uint32_t expire_count;

    if (begin <= end)
    {
        expire_count =  end - begin;
    }
    else
    {
        expire_count = 0xFFFFFFFF - (begin - end);
    }
    expire_count = expire_count * 31;
    return expire_count;
}

T_CUR_DEVICE_ROLE app_get_cur_tc_role(void)
{
    return g_cur_test_role;
}

void app_set_cur_tc_role(T_CUR_DEVICE_ROLE role)
{
    switch (role)
    {
    case TC_ROLE_UNDEFINED:
        data_uart_print("app_set_cur_tc_role: TC_ROLE_UNDEFINED\r\n");
        break;
    case TC_ROLE_DUT:
        data_uart_print("app_set_cur_tc_role: role TC_ROLE_DUT\r\n");
        break;
    case TC_ROLE_SUT:
        data_uart_print("app_set_cur_tc_role: TC_ROLE_SUT\r\n");
        break;
    default:
        role = TC_ROLE_UNDEFINED;
        data_uart_print("app_set_cur_tc_role: TC_ROLE_UNDEFINED\r\n");
        break;
    }
    g_cur_test_role = role;
}


T_CUR_TEST_CASE app_get_cur_test_case(void)
{
    return g_cur_test_case;
}

void app_display_case_details(T_CUR_TEST_CASE test_case_id)
{
    switch (test_case_id)
    {
    case TC_IDLE:
        {
            data_uart_print("select one test case:\r\n");

            data_uart_print("testcase 1 - %s\r\n", "TC_0001_ADV_PERFORMANCE");
            data_uart_print("testcase 2 - %s\r\n", "TC_0200_TP_NOTIFICATION_TX_01");
            data_uart_print("testcase 3 - %s\r\n", "TC_0201_TP_INDICATION_TX_01");
            data_uart_print("testcase 4 - %s\r\n", "TC_0202_TP_WRITE_COMMAND_TX_01");
            data_uart_print("testcase 5 - %s\r\n", "TC_0203_TP_NOTIFICATION_RX_01");
            data_uart_print("testcase 6 - %s\r\n", "TC_0205_TP_WRITE_COMMAND_RX_01");
            data_uart_print("testcase 7 - %s\r\n", "TC_0204_TP_INDICATION_RX_01");
            data_uart_print("testcase 8 - %s\r\n", "TC_0300_ADV_ONLY");
            data_uart_print("testcase 9 - %s\r\n", "TC_0301_ADV_DISC");
            data_uart_print("testcase 10 - %s\r\n", "TC_0400_CONN_PARAM_UPDATE_SLAVE");
            data_uart_print("testcase 11 - %s\r\n", "TC_0401_CONN_PARAM_UPDATE_SLAVE_01");
            data_uart_print("testcase 12 - %s\r\n", "TC_0500_SLAVE_AUTO_ADV");
        }
        break;

    case TC_0001_ADV_PERFORMANCE:
        data_uart_print("TC_0001_ADV_PERFORMANCE is selected\r\n");
        APP_PRINT_INFO0("TC_0001_ADV_PERFORMANCE Start\r\n");
        break;
    case TC_0002_ADV_STRESS_START_STOP:
        data_uart_print("TC_0002_ADV_STRESS_START_STOP is selected\r\n");
        APP_PRINT_INFO0("TC_0002_ADV_STRESS_START_STOP Start\r\n");
        break;

    case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
        data_uart_print("TC_0101_SCAN_STRESS_ENABLE_DISABLE is selected, 7 params\r\n");
        data_uart_print("tc 101 [scan_mode] [interval] [window] [filte_policy] [filter_dup] [coount]\r\n");
        APP_PRINT_INFO0("TC_0101_SCAN_STRESS_ENABLE_DISABLE Start\r\n");
        break;

    case TC_0200_TP_NOTIFICATION_TX_01:
        data_uart_print("TC_0200_TP_NOTIFICATION_TX_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0200_TP_NOTIFICATION_TX_01 Start\r\n");

        break;
    case TC_0201_TP_INDICATION_TX_01:
        data_uart_print("TC_0201_TP_INDICATION_TX_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0201_TP_INDICATION_TX_01 Start\r\n");

        break;
    case TC_0202_TP_WRITE_COMMAND_TX_01:
        data_uart_print("TC_0202_TP_WRITE_COMMAND_TX_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0202_TP_WRITE_COMMAND_TX_01 Start\r\n");

        break;
    case TC_0203_TP_NOTIFICATION_RX_01:
        data_uart_print("TC_0203_TP_NOTIFICATION_RX_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0203_TP_NOTIFICATION_RX_01 Start\r\n");

        break;
    case TC_0204_TP_INDICATION_RX_01:
        data_uart_print("TC_0204_TP_INDICATION_RX_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0204_TP_INDICATION_RX_01 Start\r\n");

        break;
    case TC_0205_TP_WRITE_COMMAND_RX_01:
        data_uart_print("TC_0205_TP_WRITE_COMMAND_RX_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0205_TP_WRITE_COMMAND_RX_01 Start\r\n");

        break;
    case TC_0206_TP_NOTIFICATION_TX_02:
        data_uart_print("TC_0206_TP_NOTIFICATION_TX_02 is selected\r\n");
        APP_PRINT_INFO0("TC_0206_TP_NOTIFICATION_TX_02 Start\r\n");
        break;

    case TC_0207_TP_WRITE_COMMAND_RX_02:
        data_uart_print("TC_0207_TP_WRITE_COMMAND_RX_02 is selected\r\n");
        APP_PRINT_INFO0("TC_0207_TP_WRITE_COMMAND_RX_02 Start\r\n");
        break;

    case TC_0208_TP_NOTIF_WRITE_CMD_TRX_02:
        data_uart_print("TC_0208_TP_NOTIF_WRITE_CMD_TRX_02 is selected\r\n");
        APP_PRINT_INFO0("TC_0208_TP_NOTIF_WRITE_CMD_TRX_02 Start\r\n");
        break;

    case TC_0300_ADV_ONLY:
        data_uart_print("TC_0300_ADV_ONLY is selected\r\n");
        APP_PRINT_INFO0("TC_0300_ADV_ONLY Start\r\n");
        break;
    case TC_0301_ADV_DISC:
        data_uart_print("TC_0301_ADV_DISC is selected\r\n");
        APP_PRINT_INFO0("TC_0301_ADV_DISC Start\r\n");
        break;
    case TC_0400_CONN_PARAM_UPDATE_SLAVE:
        data_uart_print("TC_0400_CONN_PARAM_UPDATE_SLAVE is selected\r\n");
        APP_PRINT_INFO0("TC_0400_CONN_PARAM_UPDATE_SLAVE Start\r\n");

        break;
    case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
        data_uart_print("TC_0401_CONN_PARAM_UPDATE_SLAVE_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0401_CONN_PARAM_UPDATE_SLAVE_01 Start\r\n");

        break;
    case TC_0500_SLAVE_AUTO_ADV:
        data_uart_print("TC_0500_SLAVE_AUTO_ADV is selected\r\n");
        APP_PRINT_INFO0("TC_0500_SLAVE_AUTO_ADV Start\r\n");

        break;
    case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
        data_uart_print("TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ is selected\r\n");
        APP_PRINT_INFO0("TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ Start\r\n");

        break;

    case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
        data_uart_print("TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV is selected\r\n");
        APP_PRINT_INFO0("TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV Start\r\n");
        break;

    case TC_0600_IOP_PAIR_LEGACL:
        data_uart_print("TC_0600_IOP_PAIR_LEGACL is selected\r\n");
        APP_PRINT_INFO0("TC_0600_IOP_PAIR_LEGACL Start\r\n");

        break;
    case TC_0601_IOP_PAIR_SC:
        data_uart_print("TC_0601_IOP_PAIR_SC is selected\r\n");
        APP_PRINT_INFO0("TC_0601_IOP_PAIR_SC Start\r\n");

        break;
    case TC_0700_STABLE_NOTIFICATION_TX_01:
        data_uart_print("TC_0700_STABLE_NOTIFICATION_TX_01 is selected\r\n");
        APP_PRINT_INFO0("TC_0700_STABLE_NOTIFICATION_TX_01 Start\r\n");

        break;
    case TC_0800_IOP_PAIR_LEGACL:
        data_uart_print("TC_0800_IOP_PAIR_LEGACL is selected\r\n");
        APP_PRINT_INFO0("TC_0800_IOP_PAIR_LEGACL Start\r\n");

        break;
    case TC_0801_IOP_PAIR_SC:
        data_uart_print("TC_0801_IOP_PAIR_SC is selected\r\n");
        APP_PRINT_INFO0("TC_0801_IOP_PAIR_SC Start\r\n");

        break;
    case TC_0900_PRIVACY_TEST_SLAVE:
        data_uart_print("TC_0900_PRIVACY_TEST_SLAVE is selected\r\n");
        APP_PRINT_INFO0("TC_0900_PRIVACY_TEST_SLAVE Start\r\n");
        break;

    case TC_0901_PRIVACY_TEST_MASTER:
        data_uart_print("TC_0901_PRIVACY_TEST_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_0901_PRIVACY_TEST_MASTER Start\r\n");

        break;

    case TC_1000_CP_TEST:
        data_uart_print("TC_1000_CP_TEST is selected\r\n");
        APP_PRINT_INFO0("TC_1000_CP_TEST Start\r\n");

        break;

    case TC_1100_BT41_CONN_TEST:
        data_uart_print("TC_1100_BT41_CONN_TEST is selected\r\n");
        APP_PRINT_INFO0("TC_1100_BT41_CONN_TEST Start\r\n");
        break;

    case TC_1101_BT41_TRX_STRESS_TEST:
        data_uart_print("TC_1101_BT41_TRX_STRESS_TEST is selected\r\n");
        APP_PRINT_INFO0("TC_1101_BT41_TRX_STRESS_TEST Start\r\n");
        break;

    case TC_1102_BT41_TP_TEST:
        data_uart_print("TC_1102_BT41_TP_TEST is selected\r\n");
        APP_PRINT_INFO0("TC_1102_BT41_TP_TEST Start\r\n");
        break;
    case TC_1200_MULTI_LINK_4_MASTER:
        data_uart_print("TC_1200_MULTI_LINK_2_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_1200_MULTI_LINK_2_MASTER Start\r\n");
        break;
    case TC_1201_MULTI_LINK_4_MASTER:
        data_uart_print("TC_1200_MULTI_LINK_2_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_1200_MULTI_LINK_2_MASTER Start\r\n");
        break;
    case TC_1202_MULTI_LINK_4_MASTER:
        data_uart_print("TC_1200_MULTI_LINK_2_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_1200_MULTI_LINK_2_MASTER Start\r\n");
        break;
    case TC_1203_MULTI_LINK_4_MASTER:
        data_uart_print("TC_1200_MULTI_LINK_2_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_1200_MULTI_LINK_2_MASTER Start\r\n");
        break;
    case TC_1204_MULTI_LINK_4_MASTER:
        data_uart_print("TC_1200_MULTI_LINK_2_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_1200_MULTI_LINK_2_MASTER Start\r\n");
        break;
    case TC_1205_MULTI_LINK_4_MASTER:
        data_uart_print("TC_1200_MULTI_LINK_2_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_1200_MULTI_LINK_2_MASTER Start\r\n");
        break;
    case TC_1206_MULTI_LINK_4_MASTER:
        data_uart_print("TC_1200_MULTI_LINK_2_MASTER is selected\r\n");
        APP_PRINT_INFO0("TC_1200_MULTI_LINK_2_MASTER Start\r\n");
        break;

    default:
        data_uart_print("invlid test case id\r\n");
        return;
    }

}

void app_dump_tc_status(T_CUR_TEST_CASE test_case_id)
{
    T_CUR_DEVICE_ROLE role;
    role = app_get_cur_tc_role();
    switch (role)
    {
    case TC_ROLE_UNDEFINED:
    default:
        data_uart_print("app_handle_test_case: TC_ROLE_UNDEFINED\r\n");
        return;

    case TC_ROLE_DUT:
        {

            switch (test_case_id)
            {
            case TC_IDLE:
                data_uart_print("select one test case:\r\n");
                break;

            case TC_0001_ADV_PERFORMANCE:

                break;
            case TC_0002_ADV_STRESS_START_STOP:
                break;

            case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
                break;

            case TC_0200_TP_NOTIFICATION_TX_01:
                break;
            case TC_0201_TP_INDICATION_TX_01:
                break;
            case TC_0202_TP_WRITE_COMMAND_TX_01:
                break;
            case TC_0203_TP_NOTIFICATION_RX_01:
                break;
            case TC_0204_TP_INDICATION_RX_01:
                break;
            case TC_0205_TP_WRITE_COMMAND_RX_01:
                break;
#if TC_206_SUPPORT
            case TC_0206_TP_NOTIFICATION_TX_02:
                tc_206_dump_result();
                break;
#endif
#if TC_207_SUPPORT
            case TC_0207_TP_WRITE_COMMAND_RX_02:
                tc_207_dump_result();
                break;
#endif
#if TC_208_SUPPORT
            case TC_0208_TP_NOTIF_WRITE_CMD_TRX_02:
                tc_208_dump_tx_result();
                tc_208_dump_rx_result();
                break;
#endif
#if TC_300_SUPPORT
            case TC_0300_ADV_ONLY:
                tc_300_dump_result();
                break;
#endif

#if TC_301_SUPPORT
            case TC_0301_ADV_DISC:
                tc_301_dump_result();
                break;
#endif

#if TC_400_SUPPORT
            case TC_0400_CONN_PARAM_UPDATE_SLAVE:
                tc_400_dump_result();
                break;
#endif

#if TC_401_SUPPORT
            case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
                tc_401_dump_result();
                break;
#endif

#if TC_500_SUPPORT
            case TC_0500_SLAVE_AUTO_ADV:
                tc_500_dump_result();
                break;
#endif

#if TC_501_SUPPORT
            case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
                tc_501_dump_result();
                break;
#endif

#if TC_502_SUPPORT
            case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
                tc_502_dump_result();
                break;
#endif

#if TC_600_SUPPORT
            case TC_0600_IOP_PAIR_LEGACL:
                tc_600_dump_result();
                break;
#endif

#if TC_601_SUPPORT
            case TC_0601_IOP_PAIR_SC:
                tc_601_dump_result();
                break;
#endif

#if TC_700_SUPPORT
            case TC_0700_STABLE_NOTIFICATION_TX_01:
                break;

#endif


#if TC_800_SUPPORT
            case TC_0800_IOP_PAIR_LEGACL:
                tc_800_dump_result();
                break;
#endif

#if TC_801_SUPPORT
            case TC_0801_IOP_PAIR_SC:
                tc_801_dump_result();
                break;
#endif

#if TC_900_SUPPORT
            case TC_0900_PRIVACY_TEST_SLAVE:
                break;
#endif

#if TC_901_SUPPORT
            case TC_0901_PRIVACY_TEST_MASTER:
                break;
#endif

#if TC_1000_SUPPORT
            case TC_1000_CP_TEST:

                break;
#endif
#if TC_1100_SUPPORT
            case TC_1100_BT41_CONN_TEST:
                tc_1100_dump_result();
                break;
#endif
#if TC_1101_SUPPORT
            case TC_1101_BT41_TRX_STRESS_TEST:
                tc_1101_dump_result();
                break;
#endif
            default:
                return;
            }
        }
        break;

    case TC_ROLE_SUT:
        {
            switch (test_case_id)
            {
            case TC_IDLE:
                data_uart_print("select one test case:\r\n");
                break;

            case TC_0001_ADV_PERFORMANCE:
                break;

            case TC_0002_ADV_STRESS_START_STOP:
                break;

            case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
                break;


#if TC_200_SUT_SUPPORT
            case TC_0200_TP_NOTIFICATION_TX_01:
                tc_200_sut_dump_result();
                break;
#endif
#if TC_206_SUT_SUPPORT
            case TC_0206_TP_NOTIFICATION_TX_02:
                tc_206_sut_dump_result();
                break;
#endif
#if TC_207_SUT_SUPPORT
            case TC_0207_TP_WRITE_COMMAND_RX_02:
                tc_207_sut_dump_result();
                break;
#endif
#if TC_208_SUT_SUPPORT
            case TC_0208_TP_NOTIF_WRITE_CMD_TRX_02:
                tc_208_sut_dump_tx_result();
                tc_208_sut_dump_rx_result();
                break;
#endif
            case TC_0201_TP_INDICATION_TX_01:
                break;
            case TC_0202_TP_WRITE_COMMAND_TX_01:
                break;
            case TC_0203_TP_NOTIFICATION_RX_01:
                break;
            case TC_0204_TP_INDICATION_RX_01:
                break;
            case TC_0205_TP_WRITE_COMMAND_RX_01:
                break;

#if TC_300_SUT_SUPPORT
            case TC_0300_ADV_ONLY:
                tc_300_sut_dump_result();
                break;
#endif

#if TC_301_SUT_SUPPORT
            case TC_0301_ADV_DISC:
                tc_301_sut_dump_result();
                break;
#endif

#if TC_400_SUT_SUPPORT
            case TC_0400_CONN_PARAM_UPDATE_SLAVE:
                tc_400_sut_dump_result();
                break;
#endif

#if TC_401_SUT_SUPPORT
            case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
                tc_401_sut_dump_result();
                break;
#endif

#if TC_500_SUT_SUPPORT
            case TC_0500_SLAVE_AUTO_ADV:
                tc_500_sut_dump_result();
                break;
#endif

#if TC_501_SUT_SUPPORT
            case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
                tc_501_sut_dump_result();
                break;
#endif

#if TC_502_SUT_SUPPORT
            case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
                //tc_502_sut_dump_result();
                break;
#endif

#if TC_600_SUT_SUPPORT
            case TC_0600_IOP_PAIR_LEGACL:
                tc_600_sut_dump_result();
                break;
#endif

#if TC_601_SUT_SUPPORT
            case TC_0601_IOP_PAIR_SC:
                tc_601_sut_dump_result();
                break;
#endif

#if TC_700_SUT_SUPPORT
            case TC_0700_STABLE_NOTIFICATION_TX_01:
                //tc_701_sut_dump_result();
                break;
#endif

#if TC_800_SUT_SUPPORT
            case TC_0800_IOP_PAIR_LEGACL:
                tc_800_sut_dump_result();
                break;
#endif

#if TC_801_SUT_SUPPORT
            case TC_0801_IOP_PAIR_SC:
                tc_801_sut_dump_result();
                break;
#endif

#if TC_900_SUT_SUPPORT
            case TC_0900_PRIVACY_TEST_SLAVE:
                break;
#endif

#if TC_901_SUT_SUPPORT
            case TC_0901_PRIVACY_TEST_MASTER:
                break;
#endif

#if TC_1000_SUT_SUPPORT
            case TC_1000_CP_TEST:
                break;
#endif
#if TC_1100_SUT_SUPPORT
            case TC_1100_BT41_CONN_TEST:
                tc_1100_sut_dump_result();
                break;
#endif
#if TC_1101_SUT_SUPPORT
            case TC_1101_BT41_TRX_STRESS_TEST:
                tc_1101_sut_dump_result();
                break;
#endif
            default:
                return;
            }
        }
    }

    tc_dump_disc_reason();
}

void app_dump_cur_test_case(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_CUR_TEST_CASE test_case_id = TC_IDLE;
    if (p_parse_value->param_count == 0)
    {
        test_case_id = app_get_cur_test_case();
    }
    else
    {
        test_case_id = (T_CUR_TEST_CASE)p_parse_value->dw_param[0];
    }
    app_dump_tc_status(test_case_id);
}

bool app_set_rembd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = true;
    if (p_parse_value->param_count == 1)
    {
        memcpy(g_cur_rembd, &tc_rembd_list[p_parse_value->dw_param[0]], 6);
    }
    else if (p_parse_value->param_count == 6)
    {
        g_cur_rembd[0] = p_parse_value->dw_param[5];
        g_cur_rembd[1] = p_parse_value->dw_param[4];
        g_cur_rembd[2] = p_parse_value->dw_param[3];
        g_cur_rembd[3] = p_parse_value->dw_param[2];
        g_cur_rembd[4] = p_parse_value->dw_param[1];
        g_cur_rembd[5] = p_parse_value->dw_param[0];
    }
    else
    {
        ret = false;
    }

    data_uart_print("g_cur_rembd: 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    g_cur_rembd[5], g_cur_rembd[4],
                    g_cur_rembd[3], g_cur_rembd[2],
                    g_cur_rembd[1], g_cur_rembd[0]);
    return ret;
}

void app_handle_test_case(T_CUR_TEST_CASE test_case_id, T_CUR_DEVICE_ROLE role)
{
    switch (role)
    {
    case TC_ROLE_UNDEFINED:
        data_uart_print("app_handle_test_case: TC_ROLE_UNDEFINED\r\n");
        break;
    case TC_ROLE_DUT:
        data_uart_print("app_handle_test_case: role TC_ROLE_DUT\r\n");
        break;
    case TC_ROLE_SUT:
        data_uart_print("app_handle_test_case: TC_ROLE_UNDEFINED\r\n");
        break;

    }
}
#if F_BT_DLPS_EN

void app_set_dlps_mode(T_USER_CMD_PARSED_VALUE *p_parse_value)
{

    if (p_parse_value->dw_param[0] == 0)
    {
        lps_mode_pause();
        data_uart_print("app_set_dlps_mode: Active Mode\r\n");
    }
    else
    {
        uint16_t scan_interval = 400;
        uint16_t scan_window = 200;
        le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
        le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
        lps_mode_resume();
        data_uart_can_enter_dlps(true);
        data_uart_print("app_set_dlps_mode: LPS Mode\r\n");
    }
}
#endif

void app_set_cur_case(T_CUR_TEST_CASE test_case_id)
{
    g_cur_test_case = test_case_id;
}

void app_select_cur_test_case(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_CUR_TEST_CASE test_case_id;
    T_CUR_DEVICE_ROLE role;

    test_case_id = (T_CUR_TEST_CASE)p_parse_value->dw_param[0];
    role = app_get_cur_tc_role();

    switch (role)
    {
    case TC_ROLE_UNDEFINED:
        data_uart_print("app_handle_test_case: TC_ROLE_UNDEFINED\r\n");
        return;

    case TC_ROLE_DUT:
        {
            data_uart_print("app_handle_test_case: role TC_ROLE_DUT\r\n");
            app_display_case_details(test_case_id);
            switch (test_case_id)
            {
#if TC_0001_SUPPORT
            case TC_0001_ADV_PERFORMANCE:
                {
                    uint16_t advIntMin = 80;
                    uint16_t advIntMax = 80;
                    if (p_parse_value->param_count == 3)
                    {
                        advIntMin = p_parse_value->dw_param[1];
                        advIntMax = p_parse_value->dw_param[2];
                    }
                    tc_0001_adv_start(advIntMin, advIntMax);
                }
                break;
#endif

#if TC_0002_SUPPORT
            case TC_0002_ADV_STRESS_START_STOP:
                {
                    uint16_t advIntMin = 80;
                    uint16_t advIntMax = 80;
                    uint32_t max_count = 1000;
                    if (p_parse_value->param_count == 4)
                    {
                        advIntMin = p_parse_value->dw_param[1];
                        advIntMax = p_parse_value->dw_param[2];
                        max_count = p_parse_value->dw_param[3];
                    }
                    tc_0002_adv_start_stop_start(advIntMin, advIntMax, max_count);
                }
                break;
#endif

#if TC_100_SUPPORT
            case TC_0100_SCAN_PERFORMANCE:
                {

                }
                break;
#endif

#if TC_101_SUPPORT
            case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
                {
                    if (p_parse_value->param_count == 7)
                    {

                        T_GAP_SCAN_MODE     mode = (T_GAP_SCAN_MODE)p_parse_value->dw_param[1];
                        uint16_t                interval = p_parse_value->dw_param[2];
                        uint16_t                window = p_parse_value->dw_param[3];
                        T_GAP_SCAN_FILTER_POLICY filter_policy = (T_GAP_SCAN_FILTER_POLICY)
                                                                 p_parse_value->dw_param[4];
                        uint8_t                 filter_duplicates = p_parse_value->dw_param[5];
                        uint32_t max_count = p_parse_value->dw_param[6];
                        tc_0101_scan_stress_enable_disable_start(mode, interval, window,
                                                                 filter_policy, filter_duplicates, max_count);

                    }
                    else
                    {
                        data_uart_print("invlid param count, shall be 7 param count\r\n");
                    }
                }
                break;

#endif

#if TC_200_SUPPORT

            case TC_0200_TP_NOTIFICATION_TX_01:
                {
                    uint16_t interval = p_parse_value->dw_param[1];
                    uint16_t length = p_parse_value->dw_param[2];
                    uint8_t tx_octets = p_parse_value->dw_param[3];
                    bool test_drop_acl_data = false;
                    if (p_parse_value->param_count == 5)
                    {
                        test_drop_acl_data = p_parse_value->dw_param[4];
                    }
                    tc_200_tp_notification_tx_init_config(interval, length, tx_octets, test_drop_acl_data);
                    tc_200_tp_notification_tx_start();
                }

                break;
#endif
#if TC_206_SUPPORT

            case TC_0206_TP_NOTIFICATION_TX_02:
                {
                    uint16_t interval = p_parse_value->dw_param[1];
                    uint16_t latency = p_parse_value->dw_param[2];
                    uint16_t length = p_parse_value->dw_param[3];
                    uint8_t mode = p_parse_value->dw_param[4];
                    uint32_t count = p_parse_value->dw_param[5];
                    uint8_t data_check = 0;
                    if (p_parse_value->param_count == 7)
                    {
                        data_check = p_parse_value->dw_param[6];
                    }
                    tc_206_tp_notification_tx_init_config(interval, latency, length, mode, count, data_check);
                    tc_206_tp_notification_tx_start();
                }

                break;
#endif
#if TC_207_SUPPORT
            case TC_0207_TP_WRITE_COMMAND_RX_02:
                {
                    uint16_t interval = p_parse_value->dw_param[1];
                    uint16_t latency = p_parse_value->dw_param[2];
                    uint16_t length = p_parse_value->dw_param[3];
                    uint8_t mode = p_parse_value->dw_param[4];
                    uint32_t count = p_parse_value->dw_param[5];
                    uint8_t data_check = 0;
                    if (p_parse_value->param_count == 7)
                    {
                        data_check = p_parse_value->dw_param[6];
                    }
                    tc_207_tp_rx_init_config(interval, latency, length, mode, count, data_check);
                    tc_207_tp_rx_start();
                }

                break;
#endif
#if TC_208_SUPPORT
            case TC_0208_TP_NOTIF_WRITE_CMD_TRX_02:
                {
                    uint16_t interval = p_parse_value->dw_param[1];
                    uint16_t latency = p_parse_value->dw_param[2];
                    uint16_t length = p_parse_value->dw_param[3];
                    uint8_t mode = p_parse_value->dw_param[4];
                    uint32_t count = p_parse_value->dw_param[5];
                    uint8_t data_check = 0;
                    if (p_parse_value->param_count == 7)
                    {
                        data_check = p_parse_value->dw_param[6];
                    }
                    tc_208_tp_trx_init_config(interval, latency, length, mode, count, data_check);
                    tc_208_tp_trx_start();
                }

                break;
#endif
            case TC_0201_TP_INDICATION_TX_01:

                break;
            case TC_0202_TP_WRITE_COMMAND_TX_01:
                break;
            case TC_0203_TP_NOTIFICATION_RX_01:
                break;
            case TC_0204_TP_INDICATION_RX_01:
                break;
            case TC_0205_TP_WRITE_COMMAND_RX_01:
                break;

#if TC_300_SUPPORT
            case TC_0300_ADV_ONLY:
                {
                    uint32_t count = 10;
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_300_adv_only_start(count);
                }
                break;
#endif

#if TC_301_SUPPORT
            case TC_0301_ADV_DISC:
                {
                    uint32_t count = 10;
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_301_adv_disc_start(count);
                }
                break;
#endif

#if TC_400_SUPPORT
            case TC_0400_CONN_PARAM_UPDATE_SLAVE:
                {
                    uint32_t count = 10;
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_400_start(count);
                }
                break;
#endif

#if TC_401_SUPPORT
            case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
                {
                    uint32_t count = 10;
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_401_start(count);
                }
                break;
#endif

#if TC_500_SUPPORT
            case TC_0500_SLAVE_AUTO_ADV:
                {
                    uint32_t count = 10000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);

                    tc_500_salve_auto_adv_start(count);
                }
                break;
#endif

#if TC_501_SUPPORT
            case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
                {
                    uint32_t count = 10000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);

                    tc_501_salve_auto_adv_with_sec_req_start(count);
                }
                break;
#endif

#if TC_502_SUPPORT
            case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
                {
                    uint32_t count = 10000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);

                    tc_502_salve_auto_adv_start(count);
                }
                break;
#endif

#if TC_600_SUPPORT
            case TC_0600_IOP_PAIR_LEGACL:
                {
                    uint32_t count = 10000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);
                    tc_600_iop_android_legacl_pair_start(count);
                }
                break;
#endif

#if TC_601_SUPPORT
            case TC_0601_IOP_PAIR_SC:
                {
                    uint32_t count = 10000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);
                    tc_601_iop_android_sc_pair_start(count);
                }
                break;
#endif

#if TC_700_SUPPORT
            case TC_0700_STABLE_NOTIFICATION_TX_01:
                tc_700_stable_notification_tx_01_start();
                break;
#endif

#if TC_800_SUPPORT
            case TC_0800_IOP_PAIR_LEGACL:
                {
                    uint32_t count = 10000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);
                    tc_800_iop_android_legacl_pair_start(count);
                }
                break;
#endif

#if TC_801_SUPPORT
            case TC_0801_IOP_PAIR_SC:
                {
                    uint32_t count = 10000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);
                    tc_801_iop_android_sc_pair_start(count);
                }
                break;
#endif

#if TC_900_SUPPORT
            case TC_0900_PRIVACY_TEST_SLAVE:
                tc_900_privacy_slave();
                break;
#endif

#if TC_901_SUPPORT
            case TC_0901_PRIVACY_TEST_MASTER:
                tc_901_privacy_master();
                break;
#endif

#if TC_1000_SUPPORT
            case TC_1000_CP_TEST:
                tc_1000_hw_init();
                tc_1000_start();
                break;
            case TC_1001_CP_TEST:
                tc_1001_start();
                break;
            case TC_1002_CP_TEST:
                tc_1002_start();
                break;

#endif
#if TC_1100_SUPPORT
            case TC_1100_BT41_CONN_TEST:
                {
                    uint32_t count = 1000;

                    if (p_parse_value->param_count == 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    data_uart_print("count %d\r\n", count);

                    tc_1100_start(count);
                }
                break;
#endif
#if TC_1101_SUPPORT
            case TC_1101_BT41_TRX_STRESS_TEST:
                {
                    uint32_t count = 1000;
                    uint16_t data_length = 0;

                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    if (p_parse_value->param_count >= 3)
                    {
                        data_length = p_parse_value->dw_param[2];
                    }
                    data_uart_print("count %d, len %d\r\n", count, data_length);

                    tc_1101_start(count, data_length);
                }
                break;
#endif
#if TC_1102_SUPPORT
            case TC_1102_BT41_TP_TEST:
                {
                    uint16_t data_length = 20;

                    if (p_parse_value->param_count >= 2)
                    {
                        data_length = p_parse_value->dw_param[1];
                    }
                    data_uart_print("len %d\r\n", data_length);

                    tc_1102_start(data_length);
                }
                break;
#endif

#if TC_1200_SUPPORT
            case TC_1200_MULTI_LINK_4_MASTER:
                {
                    tc_1200_start(1);
                }
                break;
#endif

#if TC_1201_SUPPORT
            case TC_1201_MULTI_LINK_4_MASTER:
                {
                    tc_1201_start(1);
                }
                break;
#endif
#if TC_1202_SUPPORT
            case TC_1202_MULTI_LINK_4_MASTER:
                {
                    tc_1202_start(1);
                }
                break;
#endif
#if TC_1203_SUPPORT
            case TC_1203_MULTI_LINK_4_MASTER:
                {
                    tc_1203_start(1);
                }
                break;
#endif
#if TC_1204_SUPPORT
            case TC_1204_MULTI_LINK_4_MASTER:
                {
                    tc_1204_start(1);
                }
                break;
#endif
#if TC_1205_SUPPORT
            case TC_1205_MULTI_LINK_4_MASTER:
                {
                    tc_1205_start(1);
                }
                break;
#endif
#if TC_1206_SUPPORT
            case TC_1206_MULTI_LINK_4_MASTER:
                {
                    tc_1206_start(1);
                }
                break;
#endif
#if TC_1207_SUPPORT
            case TC_1207_MULTI_LINK_4_MASTER:
                {
                    tc_1207_start(1);
                }
                break;
#endif


            default:
                return;
            }
        }
        break;

    case TC_ROLE_SUT:
        {
            data_uart_print("app_handle_test_case: TC_ROLE_SUT\r\n");
            app_display_case_details(test_case_id);
            switch (test_case_id)
            {
            case TC_0001_ADV_PERFORMANCE:
                break;
            case TC_0002_ADV_STRESS_START_STOP:
                break;
#if TC_100_SUPPORT
            case TC_0100_SCAN_PERFORMANCE:
                break;
#endif

#if TC_101_SUPPORT
            case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
                {
                    if (p_parse_value->param_count == 7)
                    {
                        T_GAP_SCAN_MODE     mode = (T_GAP_SCAN_MODE)p_parse_value->dw_param[1];
                        uint16_t                interval = p_parse_value->dw_param[2];
                        uint16_t                window = p_parse_value->dw_param[3];
                        T_GAP_SCAN_FILTER_POLICY filter_policy = (T_GAP_SCAN_FILTER_POLICY)
                                                                 p_parse_value->dw_param[4];
                        uint8_t                 filter_duplicates = p_parse_value->dw_param[5];
                        uint32_t max_count = p_parse_value->dw_param[6];
                        tc_0101_scan_stress_enable_disable_start(mode, interval, window,
                                                                 filter_policy, filter_duplicates, max_count);

                    }
                    else
                    {
                        data_uart_print("invlid param count, shall be 7 param count\r\n");
                    }
                }
                break;
#endif

#if TC_200_SUT_SUPPORT
            case TC_0200_TP_NOTIFICATION_TX_01:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t mode = 0;
                    uint8_t mode_end = 0;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }
                    if (p_parse_value->param_count >= 3)
                    {
                        mode = p_parse_value->dw_param[2];
                    }
                    if (p_parse_value->param_count >= 4)
                    {
                        mode_end = p_parse_value->dw_param[3];
                    }

                    data_uart_print("count %d, mode %d\r\n", count, mode);
                    tc_200_sut_start(count, mode, mode_end, peer_addr);
                }
                break;
#endif
#if TC_206_SUT_SUPPORT
            case TC_0206_TP_NOTIFICATION_TX_02:
                {
                    uint8_t peer_addr[6] = {0};

                    memcpy(peer_addr, g_cur_rembd, 6);

                    tc_206_sut_start(peer_addr);
                }
                break;
#endif
#if TC_207_SUT_SUPPORT
            case TC_0207_TP_WRITE_COMMAND_RX_02:
                {
                    uint8_t peer_addr[6] = {0};

                    memcpy(peer_addr, g_cur_rembd, 6);

                    tc_207_sut_start(peer_addr);
                }
                break;
#endif
#if TC_208_SUT_SUPPORT
            case TC_0208_TP_NOTIF_WRITE_CMD_TRX_02:
                {
                    uint8_t peer_addr[6] = {0};

                    memcpy(peer_addr, g_cur_rembd, 6);

                    tc_208_sut_start(peer_addr);
                }
                break;
#endif
            case TC_0201_TP_INDICATION_TX_01:
                break;
            case TC_0202_TP_WRITE_COMMAND_TX_01:
                break;
            case TC_0203_TP_NOTIFICATION_RX_01:
                break;
            case TC_0204_TP_INDICATION_RX_01:
                break;
            case TC_0205_TP_WRITE_COMMAND_RX_01:
                break;

#if TC_300_SUT_SUPPORT
            case TC_0300_ADV_ONLY:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_300_sut_start(count, peer_addr);
                }
                break;
#endif

#if TC_301_SUT_SUPPORT
            case TC_0301_ADV_DISC:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count, peer_addr);
                    tc_301_sut_start(count, peer_addr);
                }
                break;
#endif

#if TC_400_SUT_SUPPORT
            case TC_0400_CONN_PARAM_UPDATE_SLAVE:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count, peer_addr);
                    tc_400_sut_start(count, peer_addr);
                }
                break;
#endif

#if TC_401_SUT_SUPPORT
            case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count, peer_addr);
                    tc_401_sut_start(count, peer_addr);
                }

                break;
#endif

#if TC_500_SUT_SUPPORT
            case TC_0500_SLAVE_AUTO_ADV:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count, peer_addr);
                    tc_500_sut_start(count, peer_addr);
                }
                break;
#endif

#if TC_501_SUT_SUPPORT
            case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count, peer_addr);
                    tc_501_sut_start(count, peer_addr);
                }
                break;
#endif

#if TC_502_SUT_SUPPORT
            case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
                break;
#endif

#if TC_600_SUT_SUPPORT
            case TC_0600_IOP_PAIR_LEGACL:
                {
                    uint32_t count = 10000;

                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_600_sut_iop_android_legacl_pair_start(count, peer_addr);
                }
                break;
#endif

#if TC_601_SUT_SUPPORT
            case TC_0601_IOP_PAIR_SC:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }


                    data_uart_print("count %d\r\n", count);

                    tc_601_sut_iop_android_sc_pair_start(count, peer_addr);
                }
                break;
#endif


            case TC_0700_STABLE_NOTIFICATION_TX_01:
                //tc_700_stable_notification_tx_01_start();
                break;


#if TC_800_SUT_SUPPORT
            case TC_0800_IOP_PAIR_LEGACL:
                {
                    uint32_t count = 10000;

                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_800_sut_iop_android_legacl_pair_start(count, peer_addr);
                }
                break;
#endif

#if TC_801_SUT_SUPPORT
            case TC_0801_IOP_PAIR_SC:
                {
                    uint32_t count = 10000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }


                    data_uart_print("count %d\r\n", count);

                    tc_801_sut_iop_android_sc_pair_start(count, peer_addr);
                }
                break;
#endif

#if TC_900_SUPPORT
            case TC_0900_PRIVACY_TEST_SLAVE:
                tc_900_privacy_slave();
                break;
            case TC_0901_PRIVACY_TEST_MASTER:
                tc_901_privacy_master();
                break;
#endif

#if TC_1000_SUPPORT
            case TC_1000_CP_TEST:
                tc_1000_hw_init();
                tc_1000_start();
                break;
            case TC_1001_CP_TEST:
                tc_1001_start();
                break;
            case TC_1002_CP_TEST:
                tc_1002_start();
                break;
#endif
#if TC_1100_SUT_SUPPORT
            case TC_1100_BT41_CONN_TEST:
                {
                    uint32_t count = 1000;
                    uint8_t peer_addr[6] = {0};
                    uint8_t addr_len;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count == 8)
                    {
                        for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
                        {
                            peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
                        }
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_1100_sut_start(count, peer_addr);
                }
                break;
#endif
#if TC_1101_SUT_SUPPORT
            case TC_1101_BT41_TRX_STRESS_TEST:
                {
                    uint32_t count = 1000;
                    uint8_t peer_addr[6] = {0};
                    uint16_t data_len = 0;

                    memcpy(peer_addr, g_cur_rembd, 6);
                    if (p_parse_value->param_count >= 2)
                    {
                        count = p_parse_value->dw_param[1];
                    }

                    if (p_parse_value->param_count >= 3)
                    {
                        data_len = p_parse_value->dw_param[2];
                    }

                    data_uart_print("count %d\r\n", count);
                    tc_1101_sut_start(count, peer_addr, data_len);
                }
                break;
#endif
#if TC_1102_SUT_SUPPORT
            case TC_1102_BT41_TP_TEST:
                {
                    uint8_t peer_addr[6] = {0};
                    memcpy(peer_addr, g_cur_rembd, 6);

                    tc_1102_sut_start(peer_addr);
                }
                break;
#endif

#if TC_1204_SUT_SUPPORT
            case TC_1204_MULTI_LINK_4_MASTER:
                {
                    tc_1204_sut_start(1);
                }
                break;
#endif

            default:
                return;
            }
        }
        break;
    }


    g_cur_test_case = test_case_id;
}




void app_tc_dut_add(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_CUR_TEST_CASE test_case_id = (T_CUR_TEST_CASE)p_parse_value->dw_param[0];

    data_uart_print("app_tc_add: role TC_ROLE_DUT\r\n");

    switch (test_case_id)
    {
#if TC_0001_SUPPORT
    case TC_0001_ADV_PERFORMANCE:
        {
            uint16_t advIntMin = 80;
            uint16_t advIntMax = 80;
            if (p_parse_value->param_count == 3)
            {
                advIntMin = p_parse_value->dw_param[1];
                advIntMax = p_parse_value->dw_param[2];
            }
            tc_0001_add_case(advIntMin, advIntMax);
        }
        break;
#endif

#if TC_0002_SUPPORT
    case TC_0002_ADV_STRESS_START_STOP:
        {
            uint16_t advIntMin = 80;
            uint16_t advIntMax = 80;
            uint32_t max_count = 1000;
            if (p_parse_value->param_count == 4)
            {
                advIntMin = p_parse_value->dw_param[1];
                advIntMax = p_parse_value->dw_param[2];
                max_count = p_parse_value->dw_param[3];
            }
            tc_0002_add_case(advIntMin, advIntMax, max_count);
        }
        break;
#endif

#if TC_100_SUPPORT
    case TC_0100_SCAN_PERFORMANCE:
        {

        }
        break;
#endif

#if TC_101_SUPPORT
    case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
        {
            if (p_parse_value->param_count == 7)
            {

                T_GAP_SCAN_MODE     mode = (T_GAP_SCAN_MODE)p_parse_value->dw_param[1];
                uint16_t                interval = p_parse_value->dw_param[2];
                uint16_t                window = p_parse_value->dw_param[3];
                T_GAP_SCAN_FILTER_POLICY filter_policy = (T_GAP_SCAN_FILTER_POLICY)
                                                         p_parse_value->dw_param[4];
                uint8_t                 filter_duplicates = p_parse_value->dw_param[5];
                uint32_t max_count = p_parse_value->dw_param[6];
                tc_0101_scan_stress_enable_disable_start(mode, interval, window,
                                                         filter_policy, filter_duplicates, max_count);

            }
            else
            {
                data_uart_print("invlid param count, shall be 7 param count\r\n");
            }
        }
        break;
#endif

#if F_BT_LE_GAP_PERIPHERAL_SUPPORT

#if TC_200_SUPPORT
    case TC_0200_TP_NOTIFICATION_TX_01:
        {
            uint16_t interval = p_parse_value->dw_param[1];
            uint16_t length = p_parse_value->dw_param[2];
            uint8_t tx_octets = p_parse_value->dw_param[3];
            tc_200_tp_notification_tx_init_config(interval, length, tx_octets, false);
            tc_200_tp_notification_tx_start();
        }
        break;
#endif
#if TC_200_SUPPORT
    case TC_0201_TP_INDICATION_TX_01:
        break;
#endif

    case TC_0202_TP_WRITE_COMMAND_TX_01:
        break;
    case TC_0203_TP_NOTIFICATION_RX_01:
        break;
    case TC_0204_TP_INDICATION_RX_01:
        break;
    case TC_0205_TP_WRITE_COMMAND_RX_01:
        break;

#if TC_300_SUPPORT
    case TC_0300_ADV_ONLY:
        {
            uint32_t count = 10000;
            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_300_add_case(count);
        }
        break;
#endif

#if TC_301_SUPPORT
    case TC_0301_ADV_DISC:
        {
            uint32_t count = 10000;
            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_301_add_case(count);
        }
        break;
#endif

#if TC_400_SUPPORT
    case TC_0400_CONN_PARAM_UPDATE_SLAVE:
        {
            uint32_t count = 10000;
            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_400_add_case(count);
        }
        break;
#endif

#if TC_401_SUPPORT
    case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
        {
            uint32_t count = 10000;
            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_401_add_case(count);
        }
        break;
#endif

#if TC_500_SUPPORT
    case TC_0500_SLAVE_AUTO_ADV:
        {
            uint32_t count = 10000;

            if (p_parse_value->param_count == 2)
            {
                count = p_parse_value->dw_param[1];
            }
            data_uart_print("count %d\r\n", count);

            tc_500_add_case(count);
        }
        break;
#endif

#if TC_501_SUPPORT
    case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
        {
            uint32_t count = 10000;

            if (p_parse_value->param_count == 2)
            {
                count = p_parse_value->dw_param[1];
            }
            data_uart_print("count %d\r\n", count);

            tc_501_add_case(count);
        }
        break;
#endif

#if TC_502_SUPPORT
    case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
        {

        }
        break;
#endif

#if TC_600_SUPPORT
    case TC_0600_IOP_PAIR_LEGACL:
        {
            uint32_t count = 10000;
            if (p_parse_value->param_count == 2)
            {
                count = p_parse_value->dw_param[1];
            }
            data_uart_print("count %d\r\n", count);
            tc_600_add_case(count);
        }
        break;
#endif

#if TC_601_SUPPORT
    case TC_0601_IOP_PAIR_SC:
        {
            uint32_t count = 10000;
            if (p_parse_value->param_count == 2)
            {
                count = p_parse_value->dw_param[1];
            }
            data_uart_print("count %d\r\n", count);
            tc_601_add_case(count);
        }
        break;
#endif

#if TC_700_SUPPORT
    case TC_0700_STABLE_NOTIFICATION_TX_01:
        tc_700_stable_notification_tx_01_start();
        break;

#endif

#if TC_800_SUPPORT
    case TC_0800_IOP_PAIR_LEGACL:
        {
            uint32_t count = 10000;

            data_uart_print("count %d\r\n", count);
            tc_800_add_case(count);
        }
        break;
#endif

#if TC_801_SUPPORT
    case TC_0801_IOP_PAIR_SC:
        {
            uint32_t count = 10000;

            data_uart_print("count %d\r\n", count);
            tc_801_add_case(count);
        }
        break;
#endif

#if TC_900_SUPPORT
    case TC_0900_PRIVACY_TEST_SLAVE:
        tc_900_privacy_slave();
        break;
#endif

#if TC_901_SUPPORT
    case TC_0901_PRIVACY_TEST_MASTER:
        tc_901_privacy_master();
        break;
#endif
#endif


#if TC_1000_SUPPORT

#if F_CP_TEST_SUPPORT
    case TC_1000_CP_TEST:
        tc_1000_hw_init();
        tc_1000_start();
        break;
    case TC_1001_CP_TEST:
        tc_1001_start();
        break;
    case TC_1002_CP_TEST:
        tc_1002_start();
        break;
#endif
#endif

    default:
        return;
    }
}


void app_tc_sut_add(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_CUR_TEST_CASE test_case_id = (T_CUR_TEST_CASE)p_parse_value->dw_param[0];

    data_uart_print("app_handle_test_case: TC_ROLE_SUT\r\n");
    app_display_case_details(test_case_id);
    switch (test_case_id)
    {
#if F_BT_LE_GAP_PERIPHERAL_SUPPORT

#if TC_0001_SUPPORT
    case TC_0001_ADV_PERFORMANCE:
        {
            uint16_t advIntMin = 80;
            uint16_t advIntMax = 80;
            if (p_parse_value->param_count == 3)
            {
                advIntMin = p_parse_value->dw_param[1];
                advIntMax = p_parse_value->dw_param[2];
            }
            tc_0001_add_case(advIntMin, advIntMax);
        }
        break;
#endif

#if TC_0002_SUPPORT
    case TC_0002_ADV_STRESS_START_STOP:
        {
            uint16_t advIntMin = 80;
            uint16_t advIntMax = 80;
            uint32_t max_count = 1000;
            if (p_parse_value->param_count == 4)
            {
                advIntMin = p_parse_value->dw_param[1];
                advIntMax = p_parse_value->dw_param[2];
                max_count = p_parse_value->dw_param[3];
            }
            tc_0002_add_case(advIntMin, advIntMax, max_count);
        }
        break;
#endif
#endif

#if TC_100_SUPPORT
    case TC_0100_SCAN_PERFORMANCE:
        {

        }
        break;
#endif

#if TC_101_SUPPORT
    case TC_0101_SCAN_STRESS_ENABLE_DISABLE:
        {
            if (p_parse_value->param_count == 7)
            {

                T_GAP_SCAN_MODE     mode = (T_GAP_SCAN_MODE)p_parse_value->dw_param[1];
                uint16_t                interval = p_parse_value->dw_param[2];
                uint16_t                window = p_parse_value->dw_param[3];
                T_GAP_SCAN_FILTER_POLICY filter_policy = (T_GAP_SCAN_FILTER_POLICY)
                                                         p_parse_value->dw_param[4];
                uint8_t                 filter_duplicates = p_parse_value->dw_param[5];
                uint32_t max_count = p_parse_value->dw_param[6];
                tc_0101_scan_stress_enable_disable_start(mode, interval, window,
                                                         filter_policy, filter_duplicates, max_count);

            }
            else
            {
                data_uart_print("invlid param count, shall be 7 param count\r\n");
            }
        }
        break;
#endif



#if TC_200_SUT_SUPPORT
    case TC_0200_TP_NOTIFICATION_TX_01:
        {
            uint32_t count = 1;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_200_sut_add_case(count, g_cur_rembd);
        }
        break;
#endif

    case TC_0201_TP_INDICATION_TX_01:
        break;
    case TC_0202_TP_WRITE_COMMAND_TX_01:
        break;
    case TC_0203_TP_NOTIFICATION_RX_01:
        break;
    case TC_0204_TP_INDICATION_RX_01:
        break;
    case TC_0205_TP_WRITE_COMMAND_RX_01:
        break;

#if TC_300_SUT_SUPPORT
    case TC_0300_ADV_ONLY:
        {
            uint32_t count = 10000;
            uint8_t peer_addr[6] = {0};
            uint8_t addr_len;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
            {
                peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
            }
            data_uart_print("count %d\r\n", count);
            tc_300_sut_add_case(count, peer_addr);
        }
        break;
#endif

#if TC_301_SUT_SUPPORT
    case TC_0301_ADV_DISC:
        {
            uint32_t count = 10000;
            uint8_t peer_addr[6] = {0};
            uint8_t addr_len;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
            {
                peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
            }

            data_uart_print("count %d\r\n", count, peer_addr);
            tc_301_sut_add_case(count, peer_addr);
        }
        break;
#endif

#if TC_400_SUT_SUPPORT
    case TC_0400_CONN_PARAM_UPDATE_SLAVE:
        {
            uint32_t count = 10000;
            uint8_t peer_addr[6] = {0};
            uint8_t addr_len;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
            {
                peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
            }

            data_uart_print("count %d\r\n", count, peer_addr);
            tc_400_sut_add_case(count, peer_addr);
        }
        break;
#endif

#if TC_401_SUT_SUPPORT
    case TC_0401_CONN_PARAM_UPDATE_SLAVE_01:
        {
            uint32_t count = 10000;
            uint8_t peer_addr[6] = {0};
            uint8_t addr_len;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
            {
                peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
            }

            data_uart_print("count %d\r\n", count, peer_addr);
            tc_401_sut_add_case(count, peer_addr);
        }
        break;
#endif

#if TC_500_SUT_SUPPORT
    case TC_0500_SLAVE_AUTO_ADV:
        {
            uint32_t count = 10000;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_500_sut_add_case(count, g_cur_rembd);
        }

        break;
#endif

#if TC_501_SUT_SUPPORT
    case TC_0501_SLAVE_AUTO_ADV_WITH_SEC_REQ:
        {
            uint32_t count = 10000;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_501_sut_add_case(count, g_cur_rembd);
        }
        break;
#endif

#if TC_502_SUT_SUPPORT
    case TC_0502_SLAVE_MULTIPLE_LINK_AUTO_ADV:
        break;
#endif

#if TC_600_SUT_SUPPORT
    case TC_0600_IOP_PAIR_LEGACL:
        {
            uint32_t count = 10000;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_600_sut_add_case(count, g_cur_rembd);
        }
        break;
#endif

#if TC_601_SUT_SUPPORT
    case TC_0601_IOP_PAIR_SC:
        {
            uint32_t count = 10000;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            data_uart_print("count %d\r\n", count);
            tc_601_sut_add_case(count, g_cur_rembd);
        }
        break;
#endif

#if TC_700_SUPPORT
    case TC_0700_STABLE_NOTIFICATION_TX_01:
        tc_700_stable_notification_tx_01_start();
        break;
#endif

#if TC_800_SUT_SUPPORT
    case TC_0800_IOP_PAIR_LEGACL:
        {
            uint32_t count = 10000;
            uint8_t peer_addr[6] = {0};
            uint8_t addr_len;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
            {
                peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
            }

            data_uart_print("count %d\r\n", count, peer_addr);
            tc_800_sut_add_case(count, peer_addr);
        }
        break;
#endif

#if TC_801_SUT_SUPPORT
    case TC_0801_IOP_PAIR_SC:
        {
            uint32_t count = 10000;
            uint8_t peer_addr[6] = {0};
            uint8_t addr_len;

            if (p_parse_value->param_count >= 2)
            {
                count = p_parse_value->dw_param[1];
            }

            for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
            {
                peer_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1 + 2];
            }

            data_uart_print("count %d\r\n", count, peer_addr);
            tc_801_sut_add_case(count, peer_addr);
        }
        break;
#endif

#if TC_900_SUPPORT
    case TC_0900_PRIVACY_TEST_SLAVE:
        tc_900_privacy_slave();
        break;
#endif

#if TC_901_SUPPORT
    case TC_0901_PRIVACY_TEST_MASTER:
        tc_901_privacy_master();
        break;
#endif

#if TC_1000_SUPPORT
#if F_CP_TEST_SUPPORT
    case TC_1000_CP_TEST:
        tc_1000_hw_init();
        tc_1000_start();
        break;
    case TC_1001_CP_TEST:
        tc_1001_start();
        break;
    case TC_1002_CP_TEST:
        tc_1002_start();
        break;
#endif
#endif



    default:
        return;
    }
}


void app_tc_add(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_CUR_TEST_CASE test_case_id;
    T_CUR_DEVICE_ROLE role;

    test_case_id = (T_CUR_TEST_CASE)p_parse_value->dw_param[0];
    role = app_get_cur_tc_role();

    memset(&g_ble_disconn_reason, 0, sizeof(g_ble_disconn_reason));

    switch (role)
    {
    case TC_ROLE_UNDEFINED:
        data_uart_print("app_tc_add: TC_ROLE_UNDEFINED\r\n");
        return;

    case TC_ROLE_DUT:
        app_tc_dut_add(p_parse_value);
        break;

    case TC_ROLE_SUT:
        app_tc_sut_add(p_parse_value);
        break;

    default:
        break;
    }

    g_cur_test_case = test_case_id;
}

