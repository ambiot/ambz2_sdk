#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_throughput_app.h"
#include "os_sched.h"
#include "ble_throughput_user_cmd.h"
#include "user_cmd_parse.h"
#if F_BT_LE_GATT_SERVER_SUPPORT
#include "profile_server.h"
#endif

#include <ble_throughput_test_case.h>
#include "atcmd_bt.h"
#include "log_service.h"

T_CUR_TEST_CASE g_cur_test_case;

T_CUR_DEVICE_ROLE g_cur_test_role;

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

uint32_t str_to_uint32(char *p)
{
    uint32_t result = 0;
    bool     hex = false;

    /* check if value is dec */
    if (p[0] == 'x')
    {
        hex = true;
        p = &p[1];
    }
    else if ((p[0] == '0') && (p[1] == 'x'))
    {
        hex = true;
        p = &p[2];
    }

    for (;;)
    {
        char     ch;
        ch = *(p++) | 0x20;                 /* convert to lower case */

        if (hex)                            /* dec value */
        {
            /* hex value */
            if ((ch >= 'a') && (ch <= 'f'))
            {
                ch -= ('a' - 10);
            }
            else if ((ch >= '0') && (ch <= '9'))
            {
                ch -= '0';
            }
            else
            {
                break;
            }
            result = (result << 4);
            result += (ch & 0x0f);
        }
        else
        {
            if (ch < '0' || ch > '9')
            {
                break;    /* end of string reached */
            }
            result = 10 * result + ch - '0';
        }
    }
    return (result);
}


uint32_t ble_throughput_os_time_get_elapsed(uint32_t begin, uint32_t end)
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

T_CUR_DEVICE_ROLE ble_throughput_app_get_cur_role(void)
{
    return g_cur_test_role;
}

void ble_throughput_app_set_cur_role(T_CUR_DEVICE_ROLE role)
{
    switch (role)
    {
    case TC_ROLE_UNDEFINED:
        data_uart_print("ble_throughput_app_set_cur_role: TC_ROLE_UNDEFINED\r\n");
        break;
    case TC_ROLE_DUT:
        data_uart_print("ble_throughput_app_set_cur_role: role TC_ROLE_DUT\r\n");
        break;
    case TC_ROLE_SUT:
        data_uart_print("ble_throughput_app_set_cur_role: TC_ROLE_SUT\r\n");
        break;
    default:
        role = TC_ROLE_UNDEFINED;
        data_uart_print("ble_throughput_app_set_cur_role: TC_ROLE_UNDEFINED\r\n");
        break;
    }
    g_cur_test_role = role;
}


T_CUR_TEST_CASE ble_throughput_app_get_cur_test_case(void)
{
    return g_cur_test_case;
}

void ble_throughput_app_display_case_details(T_CUR_TEST_CASE test_case_id)
{
    switch (test_case_id)
    {
    case TC_IDLE:
        {
            data_uart_print("select one test case:\r\n");

            data_uart_print("testcase 1 - %s\r\n", "TC_0206_TP_NOTIFICATION_TX_02");
            data_uart_print("testcase 2 - %s\r\n", "TC_0207_TP_WRITE_COMMAND_RX_02");
        }
        break;

    case TC_0206_TP_NOTIFICATION_TX_02:
        data_uart_print("TC_0206_TP_NOTIFICATION_TX_02 is selected\r\n");
        APP_PRINT_INFO0("TC_0206_TP_NOTIFICATION_TX_02 Start\r\n");
        break;

    case TC_0207_TP_WRITE_COMMAND_RX_02:
        data_uart_print("TC_0207_TP_WRITE_COMMAND_RX_02 is selected\r\n");
        APP_PRINT_INFO0("TC_0207_TP_WRITE_COMMAND_RX_02 Start\r\n");
        break;

    default:
        data_uart_print("invlid test case id\r\n");
        return;
    }

}

bool ble_throughput_app_set_rembd(T_USER_CMD_PARSED_VALUE *p_parse_value)
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

void ble_throughput_app_select_cur_test_case(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_CUR_TEST_CASE test_case_id;
    T_CUR_DEVICE_ROLE role;

    test_case_id = (T_CUR_TEST_CASE)p_parse_value->dw_param[0];
    role = ble_throughput_app_get_cur_role();

    switch (role)
    {
    case TC_ROLE_UNDEFINED:
        data_uart_print("app_handle_test_case: TC_ROLE_UNDEFINED\r\n");
        return;

    case TC_ROLE_DUT:
        {
            data_uart_print("app_handle_test_case: role TC_ROLE_DUT\r\n");
            ble_throughput_app_display_case_details(test_case_id);
            switch (test_case_id)
            {
            
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
                    ble_throughput_206_tp_notification_tx_init_config(interval, latency, length, mode, count, data_check);
                    ble_throughput_206_tp_notification_tx_start();
                }

                break;
				
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
                    ble_throughput_207_tp_rx_init_config(interval, latency, length, mode, count, data_check);
                    ble_throughput_207_tp_rx_start();
                }

                break;
				
            default:
                return;
            }
        }
        break;

    case TC_ROLE_SUT:
        {
            data_uart_print("app_handle_test_case: TC_ROLE_SUT\r\n");
            ble_throughput_app_display_case_details(test_case_id);
            switch (test_case_id)
            {
            
            case TC_0206_TP_NOTIFICATION_TX_02:
                {
                    uint8_t peer_addr[6] = {0};

                    memcpy(peer_addr, g_cur_rembd, 6);

                    ble_throughput_206_sut_start(peer_addr);
                }
                break;
				
            case TC_0207_TP_WRITE_COMMAND_RX_02:
                {
                    uint8_t peer_addr[6] = {0};

                    memcpy(peer_addr, g_cur_rembd, 6);

                    ble_throughput_207_sut_start(peer_addr);
                }
                break;
				
            default:
                return;
            }
        }
        break;
    }


    g_cur_test_case = test_case_id;
}

extern T_TP_TEST_PARAM g_206_tp_test_param;
extern T_TP_TEST_PARAM g_207_tp_test_param;

void ble_throughput_app_get_result(void)
{
	if(ble_throughput_app_get_cur_role() == TC_ROLE_DUT)
	{
		if(ble_throughput_app_get_cur_test_case() == TC_0206_TP_NOTIFICATION_TX_02)
		{
			int tx_count = g_206_tp_test_param.count - g_206_tp_test_param.count_remain;
			g_206_tp_test_param.end_time = os_sys_time_get();
			g_206_tp_test_param.elapsed_time = ble_throughput_os_time_get_elapsed(g_206_tp_test_param.begin_time,
																	g_206_tp_test_param.end_time);
			g_206_tp_test_param.data_rate = tx_count * g_206_tp_test_param.length * 1000 /
																	(g_206_tp_test_param.elapsed_time);
		} else if(ble_throughput_app_get_cur_test_case() == TC_0207_TP_WRITE_COMMAND_RX_02){
			g_207_tp_test_param.end_time = os_sys_time_get();
			g_207_tp_test_param.elapsed_time = ble_throughput_os_time_get_elapsed(g_207_tp_test_param.begin_time,
																			   g_207_tp_test_param.end_time);
			g_207_tp_test_param.data_rate = g_207_tp_test_param.count * g_207_tp_test_param.length * 1000 /
																	(g_207_tp_test_param.elapsed_time);
		}
		le_disconnect(0);		
	}
	else {
		printf("ATBT=RESULT only used by DUT!!\n\r");
	}		
}

int ble_throughput_at_cmd(int argc, char **argv)
{
	int ret = 0;
	int i = 0;
	int role = 0;
	T_USER_CMD_PARSED_VALUE parsed_value;
	T_USER_CMD_PARSED_VALUE *p_parsed_value = &parsed_value;
	p_parsed_value->param_count = argc-2;

	for (unsigned int j = 0 ; j < USER_CMD_MAX_PARAMETERS; j++)
    {
        p_parsed_value->p_param[j]  = NULL;
        p_parsed_value->dw_param[j] = 0;
    }
	
	if(strcmp(argv[1],"ROLE") == 0) {
		
		if(argc != 3){
			printf("ERROR:input parameter error!\n\r");
			return -1;
		}
		if(strcmp(argv[2],"1")&&strcmp(argv[2],"2")){
			printf("ERROR:input parameter error!\n\r");
			return -1;
		}
		
		role = str_to_uint32(argv[2]);
		ble_throughput_app_set_cur_role(role);
	}else if(strcmp(argv[1], "REMBD") == 0){
		if(argc !=8){
			printf("ERROR:input parameter error!\n\r");
			return -1;
		}
		do
		{
			p_parsed_value->dw_param[i] = str_to_uint32(argv[2+i]);
			i++;
		}while(i<6);
		ret = ble_throughput_app_set_rembd(p_parsed_value);
		if(ret == false)
			return -1;
	}else if(strcmp(argv[1], "TEST") == 0){	
		if((argc != 9)&&(argc != 3)){
			printf("ERROR:input parameter error!\n\r");
			return -1;
		}
		do
		{
			p_parsed_value->dw_param[i] = str_to_uint32(argv[2+i]);
			i++;
		}while(i<argc-2);
		ble_throughput_app_select_cur_test_case(p_parsed_value);
	}else if(strcmp(argv[1], "RESULT") == 0){
		ble_throughput_app_get_result();
	}else{
		printf("ERROR:input parameter error!\n\r");
		return -1;
	}
	
	return 0;
}


int ble_throughput_app_handle_at_cmd(uint16_t subtype, void *arg)
{
	int common_cmd_flag = 0;
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if (arg) {
		argc = parse_param(arg, argv);
	}

	if(subtype == BT_ATCMD_TP_TEST)
		ble_throughput_at_cmd(argc, argv);
		
	return common_cmd_flag;
}
#endif
