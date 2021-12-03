
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
#include "profile_server.h"
#include "gap_adv.h"
#include "os_sched.h"
#include <ble_auto_test_case.h>
#include <tc_common.h>
#include <tc_1100.h>
#include <os_mem.h>

#if F_BT_LE_4_1_CBC_SUPPORT
#include "gap_credit_based_conn.h"
#endif

#if TC_1100_SUPPORT
typedef struct
{
    uint32_t total_test_count;
    uint32_t total_connect_count;
    uint32_t total_disc_count;
    uint32_t disc_error_cnt;
    uint32_t data_receive_cnt;
} TC_1100_MGR;

TC_1100_MGR *p_tc_1100_mgr = NULL;

void tc_1100_start(uint32_t count)
{
    if (NULL == p_tc_1100_mgr)
    {
        p_tc_1100_mgr = (TC_1100_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_1100_MGR));
    }
    else
    {
        memset(p_tc_1100_mgr, 0, sizeof(TC_1100_MGR));
    }

    p_tc_1100_mgr->total_test_count = count;
    le_cbc_reg_psm(0x25, 1);
    le_adv_start();
}

void tc_1100_dump_result(void)
{
    if (p_tc_1100_mgr != NULL)
    {
        APP_PRINT_INFO5("tc 1100: end: total_test_count %d total_connect_count %d total_disc_count %d"
                        "disc_error_cnt %d data_receive_cnt %d\r\n",
                        p_tc_1100_mgr->total_test_count, p_tc_1100_mgr->total_connect_count,
                        p_tc_1100_mgr->total_disc_count, p_tc_1100_mgr->disc_error_cnt,
                        p_tc_1100_mgr->data_receive_cnt);

        data_uart_print("tc 1100: end: total_test_count %d total_connect_count %d total_disc_count %d"
                        " disc_error_cnt %d data_receive_cnt %d\r\n",
                        p_tc_1100_mgr->total_test_count, p_tc_1100_mgr->total_connect_count,
                        p_tc_1100_mgr->total_disc_count, p_tc_1100_mgr->disc_error_cnt,
                        p_tc_1100_mgr->data_receive_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_1100_link_connected(uint8_t conn_id)
{

}

void tc_1100_receive_data(uint16_t cid)
{
    p_tc_1100_mgr->data_receive_cnt++;
}

void tc_1100_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1100_BT41_CONN_TEST,  reason);
    tc_1100_dump_result();
}

void tc_1100_chann_connected(uint16_t cid)
{
    p_tc_1100_mgr->total_connect_count++;
}

void tc_1100_chann_disconnected(uint16_t cid, uint16_t cause)
{
    p_tc_1100_mgr->total_disc_count++;
    if (cause == 0 || cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
    {
    }
    else
    {
        p_tc_1100_mgr->disc_error_cnt++;
        APP_PRINT_ERROR1("tc_1100: error disconnect cause 0x%x", cause);
    }
}
#endif

#if TC_1101_SUPPORT
typedef struct
{
    uint32_t total_test_count;
    uint16_t data_len;
    uint16_t mtu;
    uint16_t cid;
    uint32_t data_rx_cnt;
    uint32_t data_tx_cmpl_cnt;
    uint32_t data_tx_cnt;
} TC_1101_MGR;

TC_1101_MGR *p_tc_1101_mgr = NULL;

void tc_1101_start(uint32_t count, uint16_t data_len)
{
    if (NULL == p_tc_1101_mgr)
    {
        p_tc_1101_mgr = (TC_1101_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_1101_MGR));
    }
    else
    {
        memset(p_tc_1101_mgr, 0, sizeof(TC_1101_MGR));
    }

    p_tc_1101_mgr->total_test_count = count;
    p_tc_1101_mgr->data_len = data_len;
    le_cbc_reg_psm(0x25, 1);
    le_adv_start();
}

void tc_1101_dump_result(void)
{
    if (p_tc_1101_mgr != NULL)
    {
        APP_PRINT_INFO4("tc 1101: end: total_test_count %d data_rx_cnt %d data_tx_cmpl_cnt %d"
                        "data_tx_cnt %d\r\n",
                        p_tc_1101_mgr->total_test_count, p_tc_1101_mgr->data_rx_cnt,
                        p_tc_1101_mgr->data_tx_cmpl_cnt, p_tc_1101_mgr->data_tx_cnt);

        data_uart_print("tc 1101: end: total_test_count %d data_rx_cnt %d data_tx_cmpl_cnt %d"
                        " data_tx_cnt %d\r\n",
                        p_tc_1101_mgr->total_test_count, p_tc_1101_mgr->data_rx_cnt,
                        p_tc_1101_mgr->data_tx_cmpl_cnt, p_tc_1101_mgr->data_tx_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_1101_link_connected(uint8_t conn_id)
{
}

void tc_1101_receive_data(uint16_t cid, uint16_t length)
{
    p_tc_1101_mgr->data_rx_cnt++;
}

void tx_1101_send_data_cmpl(uint16_t cause, uint16_t credit)
{
    if (cause == 0)
    {
        p_tc_1101_mgr->data_tx_cmpl_cnt++;
        T_GAP_CAUSE cause;
        void *p_buffer;
        uint8_t i;
        uint16_t length = p_tc_1101_mgr->data_len;

        p_buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
        if (p_buffer != NULL)
        {
            memset(p_buffer, 2, length);
        }
        for (i = 0; i < credit; i++)
        {
            if (p_tc_1101_mgr->data_tx_cnt == p_tc_1101_mgr->total_test_count)
            {
                break;
            }
            cause = le_cbc_send_data(p_tc_1101_mgr->cid, p_buffer, length);
            p_tc_1101_mgr->data_tx_cnt++;
            if (cause != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR1("tc_1101: send data failed 0x%x", cause);
                break;
            }
        }
        os_mem_free(p_buffer);
    }
}

void tc_1101_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1101_BT41_TRX_STRESS_TEST,  reason);
    tc_1101_dump_result();
}

void tc_1101_chann_connected(uint16_t cid, uint16_t mtu, uint16_t credit)
{
    T_GAP_CAUSE cause;
    void *p_buffer;
    uint8_t i;
    uint16_t length;
    p_tc_1101_mgr->mtu = mtu;
    p_tc_1101_mgr->cid = cid;
    if (p_tc_1101_mgr->data_len == 0 || p_tc_1101_mgr->data_len >= p_tc_1101_mgr->mtu)
    {
        p_tc_1101_mgr->data_len = p_tc_1101_mgr->mtu;
    }
    length = p_tc_1101_mgr->data_len;

    p_buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
    if (p_buffer != NULL)
    {
        memset(p_buffer, 2, length);
    }
    for (i = 0; i < credit; i++)
    {
        cause = le_cbc_send_data(cid, p_buffer, length);
        p_tc_1101_mgr->data_tx_cnt++;
        if (cause != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("tc_1101: send data failed 0x%x", cause);
            break;
        }
    }
    os_mem_free(p_buffer);
}

void tc_1101_chann_disconnected(uint16_t cid, uint16_t cause)
{
    if (cause == 0 || cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
    {
    }
    else
    {
        APP_PRINT_ERROR1("tc_1101: error disconnect cause 0x%x", cause);
    }
}
#endif

#if TC_1102_SUPPORT
typedef struct
{
    uint16_t mtu;
    uint16_t cid;
} TC_1102_MGR;

TC_1102_MGR *p_tc_1102_mgr = NULL;

typedef struct
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;
    uint16_t length;
    uint32_t count;
    uint32_t count_remain;
    uint8_t initial_value;
    bool is_update;

    uint8_t conn_id;
    uint32_t begin_time;
    uint32_t end_time;
    uint32_t data_rate;
} T_1102_TEST_PARAM;

#define MAX_TP_TEST_COUNT 1000
#define TP_TEST_PARAM_COUNT         8
T_1102_TEST_PARAM g_1102_test_param[TP_TEST_PARAM_COUNT];
uint8_t g_1102_cur_test_index = 0;
uint8_t g_1102_used_test_index = 0;
uint8_t g_1102_test_start = false;

void tc_1102_tx_config_with_fixed_length(uint16_t length)
{
    uint8_t i = 0;
    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 6;
    g_1102_test_param[i].conn_slave_latency = 0;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;
    i++;

    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 6;
    g_1102_test_param[i].conn_slave_latency = 50;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;
    i++;

    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 10;
    g_1102_test_param[i].conn_slave_latency = 0;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;
    i++;

    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 10;
    g_1102_test_param[i].conn_slave_latency = 50;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;
    i++;

    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 15;
    g_1102_test_param[i].conn_slave_latency = 0;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;
    i++;

    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 15;
    g_1102_test_param[i].conn_slave_latency = 50;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;
    i++;

    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 20;
    g_1102_test_param[i].conn_slave_latency = 0;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;
    i++;

    g_1102_test_param[i].conn_id = 0;
    g_1102_test_param[i].con_interval = 20;
    g_1102_test_param[i].conn_slave_latency = 50;
    g_1102_test_param[i].conn_supervision_timeout = 1000;
    g_1102_test_param[i].length = length;
    g_1102_test_param[i].count = MAX_TP_TEST_COUNT;
    g_1102_test_param[i].count_remain = g_1102_test_param[i].count;
    g_1102_test_param[i].initial_value = 0;

    g_1102_used_test_index = i;
}

void tc_1102_start(uint16_t data_len)
{
    if (NULL == p_tc_1102_mgr)
    {
        p_tc_1102_mgr = (TC_1102_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_1102_MGR));
    }
    else
    {
        memset(p_tc_1102_mgr, 0, sizeof(TC_1102_MGR));
    }
    memset(g_1102_test_param, 0, sizeof(g_1102_test_param));
    tc_1102_tx_config_with_fixed_length(data_len);
    le_cbc_reg_psm(0x25, 1);
    le_adv_start();
}

bool tc_1102_send_data(uint16_t *p_credit)
{
    void *p_buffer;
    uint16_t length = g_1102_test_param[g_1102_cur_test_index].length;

    p_buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
    if (p_buffer != NULL)
    {
        memset(p_buffer, g_1102_test_param[g_1102_cur_test_index].initial_value, length);
    }
    else
    {
        return false;
    }

    if (le_cbc_send_data(p_tc_1102_mgr->cid, p_buffer, length) == GAP_CAUSE_SUCCESS)
    {
        (*p_credit)--;
        g_1102_test_param[g_1102_cur_test_index].initial_value++;
        g_1102_test_param[g_1102_cur_test_index].count_remain--;
        os_mem_free(p_buffer);
        return true;
    }
    else
    {
        APP_PRINT_ERROR0("tc_1102_send_data send: failed");
        os_mem_free(p_buffer);
        return false;
    }
}

void tc_1102_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO3("GAP_MSG_LE_CONN_PARAM_UPDATE update success, con_interval = 0x%x, conn_slave_latency = 0x%x, conn_supervision_timeout = 0x%x",
                    con_interval, conn_slave_latency, conn_supervision_timeout);

    if (g_1102_test_param[g_1102_cur_test_index].con_interval ==  con_interval &&
        g_1102_test_param[g_1102_cur_test_index].conn_slave_latency ==  conn_slave_latency &&
        g_1102_test_param[g_1102_cur_test_index].conn_supervision_timeout == conn_supervision_timeout
       )
    {
        g_1102_test_param[g_1102_cur_test_index].is_update = true;
        if (p_tc_1102_mgr->cid != 0)
        {
            uint16_t cur_credit;
            g_1102_test_param[g_1102_cur_test_index].begin_time = os_sys_time_get();
            APP_PRINT_INFO1("tc 1102 test: begin time = %dms",
                            g_1102_test_param[g_1102_cur_test_index].begin_time);
            le_cbc_get_chann_param(CBC_CHANN_PARAM_CUR_CREDITS, &cur_credit, p_tc_1102_mgr->cid);
            while (cur_credit)
            {
                if (g_1102_test_param[g_1102_cur_test_index].count_remain)
                {
                    if (tc_1102_send_data(&cur_credit) == false)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }

}

void tc_1102_mtu_size_info(uint8_t conn_id)
{
    g_1102_test_start = true;
    g_1102_cur_test_index = 0;

    le_update_conn_param(g_1102_test_param[g_1102_cur_test_index].conn_id,
                         g_1102_test_param[g_1102_cur_test_index].con_interval,
                         g_1102_test_param[g_1102_cur_test_index].con_interval,
                         g_1102_test_param[g_1102_cur_test_index].conn_slave_latency,
                         g_1102_test_param[g_1102_cur_test_index].conn_supervision_timeout,
                         2 * (g_1102_test_param[g_1102_cur_test_index].con_interval - 1),
                         2 * (g_1102_test_param[g_1102_cur_test_index].con_interval - 1));
}


static bool flag1102 = false;
void tc_1102_send_data_cmpl(uint16_t cause, uint16_t credit)
{
    uint16_t max_credit;
    le_cbc_get_chann_param(CBC_CHANN_PARAM_MAX_CREDITS, &max_credit, p_tc_1102_mgr->cid);
    while (credit)
    {
        if (g_1102_test_param[g_1102_cur_test_index].count_remain)
        {
            if (tc_1102_send_data(&credit) == false)
            {
                return;
            }
        }
        else
        {
            if (credit == max_credit)
            {
                g_1102_test_param[g_1102_cur_test_index].end_time = os_sys_time_get();
                uint32_t elapsed_time = os_time_get_elapsed(g_1102_test_param[g_1102_cur_test_index].begin_time,
                                                            g_1102_test_param[g_1102_cur_test_index].end_time);
                uint32_t data_rate =
                    g_1102_test_param[g_1102_cur_test_index].count * g_1102_test_param[g_1102_cur_test_index].length *
                    1000 /
                    (elapsed_time);

                APP_PRINT_INFO8("tc 1102 test: end:conn_interval = %d,conn_latency = %d, length = %d, count = %d,  begin time = %dms, end time = %dms, elapsed time = %dms, data rate = %dBytes/s",
                                g_1102_test_param[g_1102_cur_test_index].con_interval,
                                g_1102_test_param[g_1102_cur_test_index].conn_slave_latency,
                                g_1102_test_param[g_1102_cur_test_index].length,
                                g_1102_test_param[g_1102_cur_test_index].count,
                                g_1102_test_param[g_1102_cur_test_index].begin_time,
                                g_1102_test_param[g_1102_cur_test_index].end_time,
                                elapsed_time,
                                data_rate);
                if (false == flag1102)
                {
                    data_uart_print(" conn_interval,     latency,     length,     data rate(Bytes/s)\r\n");
                    flag1102 = true;
                }
                data_uart_print("     %d,              %d,              %d,                %d\r\n",
                                g_1102_test_param[g_1102_cur_test_index].con_interval,
                                g_1102_test_param[g_1102_cur_test_index].conn_slave_latency,
                                g_1102_test_param[g_1102_cur_test_index].length,
                                data_rate);
                if (g_1102_cur_test_index < g_1102_used_test_index)
                {
                    g_1102_cur_test_index++;
                    APP_PRINT_INFO2("g_1102_cur_test_index = %d, interval = %d", g_1102_cur_test_index,
                                    g_1102_test_param[g_1102_cur_test_index].con_interval);
                    le_update_conn_param(g_1102_test_param[g_1102_cur_test_index].conn_id,
                                         g_1102_test_param[g_1102_cur_test_index].con_interval,
                                         g_1102_test_param[g_1102_cur_test_index].con_interval,
                                         g_1102_test_param[g_1102_cur_test_index].conn_slave_latency,
                                         g_1102_test_param[g_1102_cur_test_index].conn_supervision_timeout,
                                         2 * (g_1102_test_param[g_1102_cur_test_index].con_interval - 1),
                                         2 * (g_1102_test_param[g_1102_cur_test_index].con_interval - 1)
                                        );

                }
                else
                {
                    le_cbc_disc(p_tc_1102_mgr->cid);
                }
            }
            break;
        }
    }
}

void tc_1102_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1102_BT41_TP_TEST,  reason);
}

void tc_1102_chann_connected(uint16_t cid, uint16_t mtu, uint16_t credit)
{
    p_tc_1102_mgr->cid = cid;
    p_tc_1102_mgr->mtu = mtu;
    if (g_1102_test_param[g_1102_cur_test_index].is_update == true)
    {
        uint16_t cur_credit;
        g_1102_test_param[g_1102_cur_test_index].begin_time = os_sys_time_get();
        APP_PRINT_INFO1("tc 1102 test: begin time = %dms",
                        g_1102_test_param[g_1102_cur_test_index].begin_time);
        le_cbc_get_chann_param(CBC_CHANN_PARAM_CUR_CREDITS, &cur_credit, p_tc_1102_mgr->cid);
        while (cur_credit)
        {
            if (g_1102_test_param[g_1102_cur_test_index].count_remain)
            {
                if (tc_1102_send_data(&cur_credit) == false)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
}

void tc_1102_chann_disconnected(uint16_t cid, uint16_t cause)
{
    if (cause == 0 || cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
    {
    }
    else
    {
        APP_PRINT_ERROR1("tc_1102: error disconnect cause 0x%x", cause);
    }
}
#endif

