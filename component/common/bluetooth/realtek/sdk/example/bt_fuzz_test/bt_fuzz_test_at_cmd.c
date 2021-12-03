#include <platform_opts_bt.h>
#if defined(CONFIG_BT_FUZZ_TEST) && CONFIG_BT_FUZZ_TEST
#include <string.h>
#include <trace_app.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <gap.h>
#include <gap_conn_le.h>
#include <gcs_client.h>
#include "bt_fuzz_test_app.h"
#include "bt_fuzz_test_app_flags.h"
#include "log_service.h"
#include "atcmd_bt.h"
#include "bt_fuzz_test_at_cmd.h"
#include <platform/platform_stdlib.h>
#include <gap_adv.h>
#if 0
#include <drv_types.h>
#include <hal_data.h>
#endif
#define BD_ADDR_LEN							6

static u8 ctoi(char c)
{
	if((c >= 'A') && (c <= 'F')) {
		return (c - 'A' + 0x0A);
	}

	if((c >= 'a') && (c <= 'f')) {
		return (c - 'a' + 0x0A);
	}

	if((c >= '0') && (c <= '9')) {
		return (c - '0' + 0x00);
	}

	return 0xFF;
}

static u8 hex_str_to_bd_addr(u32 str_len, s8 *str, u8 *num_arr)
{
	num_arr += str_len/2 -1;
	u32 n = 0;
	u8 num = 0;

	if (str_len < 2) {
		return FALSE;
	}
	while (n < str_len) {
		if ((num = ctoi(str[n++])) == 0xFF) {
			return FALSE;
		}
		*num_arr = num << 4;
		if ((num = ctoi(str[n++])) == 0xFF) {
			return FALSE;
		}
		*num_arr |= num;
		num_arr--;
	}
	return TRUE;
}

int hex_str_to_int(u32 str_len, s8*str)
{
	int result = 0;
	unsigned int n = 2;
	if(str[0]!='0' && ((str[1] != 'x') && (str[1] != 'X'))){
		return -1;
	}
	while(n < str_len){
		result = (result << 4) | (ctoi(str[n++]));
	}
	return result;
}

void bt_fuzz_test_app_set_pair_mode(uint8_t auth_flag,uint8_t io_capbility)
{
	u8	auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
	u16 auth_flags = auth_flag;
	u8	auth_io_cap = io_capbility;

	u8	auth_sec_req_enable = false;
	u16 auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
	int ret = 1;
	
	auth_sec_req_flags =  auth_flags;


	gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
	gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
	gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);

	le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
	le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
				  &auth_sec_req_flags);
	ret = gap_set_pairable_mode();
	if(ret != GAP_CAUSE_SUCCESS)
		BLE_PRINT("\n\rSet pairable mode fail!\r\n");
	else
		BLE_PRINT("\n\rSet pairable mode success!\r\n");

}

int bt_fuzz_test_rembd(int argc, char **argv)
{
	(void) argc;
	u8 DestAddr[6] = {0};
	hex_str_to_bd_addr(strlen(argv[1]), ( s8 *)argv[1], (u8*)DestAddr);

	g_cur_rembd[0] = DestAddr[0];
    g_cur_rembd[1] = DestAddr[1];
    g_cur_rembd[2] = DestAddr[2];
    g_cur_rembd[3] = DestAddr[3];
    g_cur_rembd[4] = DestAddr[4];
    g_cur_rembd[5] = DestAddr[5];

	BLE_PRINT("g_cur_rembd: 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    g_cur_rembd[5], g_cur_rembd[4],
                    g_cur_rembd[3], g_cur_rembd[2],
                    g_cur_rembd[1], g_cur_rembd[0]);

}

int bt_fuzz_test_set_testsuite(int argc, char **argv)
{
	T_GAP_CAUSE cause;
	int ret = 1;
	u8 auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
	
    u16 auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    u8 auth_io_cap;
    u8 sec_enable;

    switch(argc){
        case 2:
            testsuite = (T_FUZZ_TESTSUITE)(hex_str_to_int(strlen(argv[1]), ( s8 *)argv[1]));
            break;

        case 3:
            testsuite = (T_FUZZ_TESTSUITE)(hex_str_to_int(strlen(argv[1]), ( s8 *)argv[1]));
            auth_io_cap = atoi(argv[2]);
            break;

        case 4:
            testsuite = (T_FUZZ_TESTSUITE)(hex_str_to_int(strlen(argv[1]), ( s8 *)argv[1]));
            auth_io_cap = atoi(argv[2]);
            sec_enable = atoi(argv[3]);
            break;

        case 5:
            testsuite = (T_FUZZ_TESTSUITE)(hex_str_to_int(strlen(argv[1]), ( s8 *)argv[1]));
            auth_io_cap = atoi(argv[2]);
            sec_enable = atoi(argv[3]);
            m_start_pair = atoi(argv[4]);
            break;

      case 6:
            testsuite = (T_FUZZ_TESTSUITE)(hex_str_to_int(strlen(argv[1]), ( s8 *)argv[1]));
            auth_io_cap = atoi(argv[2]);
            sec_enable = atoi(argv[3]);
            m_start_pair = atoi(argv[4]);
            read_handle = hex_str_to_int(strlen(argv[5]), ( s8 *)argv[5]);
            break;

      case 7:
            testsuite = (T_FUZZ_TESTSUITE)(hex_str_to_int(strlen(argv[1]), ( s8 *)argv[1]));
            auth_io_cap = atoi(argv[2]);
            sec_enable = atoi(argv[3]);
            m_start_pair = atoi(argv[4]);
            read_handle = hex_str_to_int(strlen(argv[5]), ( s8 *)argv[5]);
            write_handle = hex_str_to_int(strlen(argv[6]), ( s8 *)argv[6]);
            break;
    }

    u8  oob_enable = false;
    u8  auth_sec_req_enable = false;
    u16 auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    T_GAP_REMOTE_ADDR_TYPE remote_bd_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    T_GAP_LOCAL_ADDR_TYPE local_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
#if 0
    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 80;
    conn_req_param.conn_interval_max = 80;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 1000;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
#endif
	conn_req_param.scan_interval = 0x520;	//820ms
    conn_req_param.scan_window = 0x520;		//820ms
    conn_req_param.conn_interval_min = 0x60;	//120ms
    conn_req_param.conn_interval_max = 0x60;	//120ms
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
	if(cause == GAP_CAUSE_SUCCESS)
			BLE_PRINT("\n\rSet pairable mode success!\r\n");
		else
			BLE_PRINT("\n\rSet pairable mode fail!\r\n");
	BLE_PRINT("testsuite = %d\r\n",testsuite);
    switch (testsuite)
    {
    case FUZZ_TESTUITE_BTLE_SMP_510:
		bt_fuzz_test_app_set_pair_mode(0xd,4);  //dispaly and keyboard   SC    
    case FUZZ_TESTUITE_BTLE_ATT_510:
    case FUZZ_TESTUITE_BTLE_PROFILES_510:
        le_adv_start();
        break;

    case FUZZ_TESTUITE_BTLE_SMPC_510:
		bt_fuzz_test_app_set_pair_mode(0xd,1);  //dispaly yes or no   SC
    case FUZZ_TESTUITE_BTLE_ATTC_510:
    case FUZZ_TESTUITE_BTLE_HOGP_510:
    case FUZZ_TESTUITE_BTLE_HEALTH_510:
        //memcpy(peer_addr, g_cur_rembd, GAP_BD_ADDR_LEN);
	    BLE_PRINT("g_cur_rembd: 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n",
	                g_cur_rembd[5], g_cur_rembd[4],
	                g_cur_rembd[3], g_cur_rembd[2],
	                g_cur_rembd[1], g_cur_rembd[0]);
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

    return cause;

}

int bt_fuzz_test_app_handle_at_cmd(uint16_t subtype, void *arg)
{
	//int common_cmd_flag = 0;
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if (arg) {
		argc = parse_param(arg, argv);
	}

	switch (subtype) {
		case BT_ATCMD_SET_TEST_SUITE:
			bt_fuzz_test_set_testsuite(argc, argv);
			break;
		default:
			break;
	}

	return 0;
}
#endif
