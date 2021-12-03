
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


#include <os_mem.h>
#include <ble_auto_test_case.h>


#include <tc_common.h>
#include <tc_1100_sut.h>
#include <os_timer.h>


#if F_BT_LE_4_1_CBC_SUPPORT
#include "gap_credit_based_conn.h"
#endif

#if TC_1100_SUT_SUPPORT
typedef struct
{
    uint32_t total_test_count;
    uint32_t totol_connect_req;
    uint32_t total_connect_count;
    uint32_t total_disc_count;
    uint32_t disc_error_cnt;
    uint32_t data_send_cnt;
    uint8_t remote_bd[6];
    uint16_t cid;
} TC_1100_SUT_MGR;

TC_1100_SUT_MGR *p_tc_1100_sut_mgr = NULL;

void tc_1100_sut_start(uint32_t count, uint8_t remote_bd[6])
{
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 10000;

    if (NULL == p_tc_1100_sut_mgr)
    {
        p_tc_1100_sut_mgr = (TC_1100_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_1100_SUT_MGR));
    }
    else
    {
        memset(p_tc_1100_sut_mgr, 0, sizeof(TC_1100_SUT_MGR));
    }

    p_tc_1100_sut_mgr->total_test_count = count;
    memcpy(p_tc_1100_sut_mgr->remote_bd, remote_bd, 6);
    le_cbc_reg_psm(0x25, 1);
    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    le_connect(0, p_tc_1100_sut_mgr->remote_bd, GAP_REMOTE_ADDR_LE_PUBLIC,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout
              );
}

void tc_1100_sut_dump_result(void)
{
    if (p_tc_1100_sut_mgr != NULL)
    {
        APP_PRINT_INFO5("tc 1100 sut: end: total_test_count %d total_connect_count %d total_disc_count %d"
                        "disc_error_cnt %d data_send_cnt %d\r\n",
                        p_tc_1100_sut_mgr->total_test_count, p_tc_1100_sut_mgr->total_connect_count,
                        p_tc_1100_sut_mgr->total_disc_count, p_tc_1100_sut_mgr->disc_error_cnt,
                        p_tc_1100_sut_mgr->data_send_cnt);

        data_uart_print("tc 1100 sut: end: total_test_count %d total_connect_count %d total_disc_count %d"
                        "disc_error_cnt %d data_send_cnt %d\r\n",
                        p_tc_1100_sut_mgr->total_test_count, p_tc_1100_sut_mgr->total_connect_count,
                        p_tc_1100_sut_mgr->total_disc_count, p_tc_1100_sut_mgr->disc_error_cnt,
                        p_tc_1100_sut_mgr->data_send_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_1100_sut_link_connected(uint8_t conn_id)
{
    p_tc_1100_sut_mgr->totol_connect_req++;
    le_cbc_create(conn_id, 0x25);
}

void tc_1100_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_local_disc_reason(TC_1100_BT41_CONN_TEST,  reason);
    tc_1100_sut_dump_result();
}

void tc_1100_sut_chann_connected(uint16_t cid)
{
    uint8_t data[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    p_tc_1100_sut_mgr->total_connect_count++;
    p_tc_1100_sut_mgr->cid = cid;
    le_cbc_send_data(cid, data, 20);
}

void tc_1100_sut_chann_data_send_cmpl(uint16_t cause)
{
    if (cause == 0)
    {
        p_tc_1100_sut_mgr->data_send_cnt++;
    }
    le_cbc_disc(p_tc_1100_sut_mgr->cid);
}

void tc_1100_sut_chann_disconnected(uint16_t cid, uint16_t cause)
{
    p_tc_1100_sut_mgr->total_disc_count++;
    if (cause == 0 || cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
    {
    }
    else
    {
        p_tc_1100_sut_mgr->disc_error_cnt++;
        APP_PRINT_ERROR1("tc_1100_sut: error disconnect cause 0x%x", cause);
    }
    if (p_tc_1100_sut_mgr->totol_connect_req == p_tc_1100_sut_mgr->total_test_count)
    {
        le_disconnect(0);
    }
    else
    {
        p_tc_1100_sut_mgr->totol_connect_req++;
        le_cbc_create(0, 0x25);
    }
}
#endif

#if TC_1101_SUT_SUPPORT
typedef struct
{
    uint32_t total_test_count;
    uint16_t data_len;
    uint16_t mtu;
    uint16_t cid;
    uint32_t data_rx_cnt;
    uint32_t data_tx_cmpl_cnt;
    uint32_t data_tx_cnt;
    uint8_t remote_bd[6];
} TC_1101_SUT_MGR;

TC_1101_SUT_MGR *p_tc_1101_sut_mgr = NULL;

void tc_1101_sut_timeout_handler(void *pxTimer)
{
    APP_PRINT_INFO0("tc_1101_sut_timeout_handler");
    le_cbc_disc(p_tc_1101_sut_mgr->cid);
}

void tc_1101_sut_start(uint32_t count, uint8_t remote_bd[6], uint16_t data_len)
{
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 10000;

    if (NULL == p_tc_1101_sut_mgr)
    {
        p_tc_1101_sut_mgr = (TC_1101_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_1101_SUT_MGR));
    }
    else
    {
        memset(p_tc_1101_sut_mgr, 0, sizeof(TC_1101_SUT_MGR));
    }
    if (NULL == g_test_timer_handle)
    {
        os_timer_create(&g_test_timer_handle, "testTimer", 1, 5000, false, tc_1101_sut_timeout_handler);
    }

    p_tc_1101_sut_mgr->data_len = data_len;
    p_tc_1101_sut_mgr->total_test_count = count;
    memcpy(p_tc_1101_sut_mgr->remote_bd, remote_bd, 6);
    le_cbc_reg_psm(0x25, 1);
    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    le_connect(0, p_tc_1101_sut_mgr->remote_bd, GAP_REMOTE_ADDR_LE_PUBLIC,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout
              );
}

void tc_1101_sut_dump_result(void)
{
    if (p_tc_1101_sut_mgr != NULL)
    {
        APP_PRINT_INFO4("tc 1101 sut: end: total_test_count %d data_rx_cnt %d data_tx_cmpl_cnt %d"
                        "data_tx_cnt %d\r\n",
                        p_tc_1101_sut_mgr->total_test_count,
                        p_tc_1101_sut_mgr->data_rx_cnt,
                        p_tc_1101_sut_mgr->data_tx_cmpl_cnt,
                        p_tc_1101_sut_mgr->data_tx_cnt);

        data_uart_print("tc 1101 sut: end: total_test_count %d data_rx_cnt %d data_tx_cmpl_cnt %d"
                        "data_tx_cnt %d\r\n",
                        p_tc_1101_sut_mgr->total_test_count,
                        p_tc_1101_sut_mgr->data_rx_cnt,
                        p_tc_1101_sut_mgr->data_tx_cmpl_cnt,
                        p_tc_1101_sut_mgr->data_tx_cnt);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_1101_sut_link_connected(uint8_t conn_id)
{
    le_cbc_create(conn_id, 0x25);
}

void tc_1101_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_local_disc_reason(TC_1101_BT41_TRX_STRESS_TEST,  reason);
    tc_1101_sut_dump_result();
}

void tc_1101_sut_chann_connected(uint16_t cid, uint16_t mtu, uint8_t credit)
{
    T_GAP_CAUSE cause;
    void *p_buffer;
    uint8_t i;
    uint16_t length;
    p_tc_1101_sut_mgr->mtu = mtu;
    p_tc_1101_sut_mgr->cid = cid;
    if (p_tc_1101_sut_mgr->data_len == 0 || p_tc_1101_sut_mgr->data_len >= p_tc_1101_sut_mgr->mtu)
    {
        p_tc_1101_sut_mgr->data_len = p_tc_1101_sut_mgr->mtu;
    }
    length = p_tc_1101_sut_mgr->data_len;

    p_buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
    if (p_buffer != NULL)
    {
        memset(p_buffer, 2, length);
    }
    for (i = 0; i < credit; i++)
    {
        cause = le_cbc_send_data(cid, p_buffer, length);
        p_tc_1101_sut_mgr->data_tx_cnt++;
        if (cause != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("tc_1101: send data failed 0x%x", cause);
            break;
        }
    }
    os_mem_free(p_buffer);
}

void tc_1101_sut_receive_data(uint16_t cid, uint16_t length)
{
    p_tc_1101_sut_mgr->data_rx_cnt++;
}

void tc_1101_sut_chann_data_send_cmpl(uint16_t cause, uint8_t credit)
{
    if (cause == 0)
    {
        p_tc_1101_sut_mgr->data_tx_cmpl_cnt++;
        T_GAP_CAUSE cause;
        void *p_buffer;
        uint8_t i;
        uint16_t length = p_tc_1101_sut_mgr->data_len;

        p_buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
        if (p_buffer != NULL)
        {
            memset(p_buffer, 2, length);
        }
        for (i = 0; i < credit; i++)
        {
            if (p_tc_1101_sut_mgr->data_tx_cnt == p_tc_1101_sut_mgr->total_test_count)
            {
                break;
            }
            cause = le_cbc_send_data(p_tc_1101_sut_mgr->cid, p_buffer, length);
            p_tc_1101_sut_mgr->data_tx_cnt++;
            if (cause != GAP_CAUSE_SUCCESS)
            {
                APP_PRINT_ERROR1("tc_1101: send data failed 0x%x", cause);
                break;
            }
        }
        os_mem_free(p_buffer);
    }
    if (p_tc_1101_sut_mgr->data_tx_cnt == p_tc_1101_sut_mgr->total_test_count)
    {
        uint16_t cur_credit;
        uint16_t max_credit;
        le_cbc_get_chann_param(CBC_CHANN_PARAM_CUR_CREDITS, &cur_credit, p_tc_1101_sut_mgr->cid);
        le_cbc_get_chann_param(CBC_CHANN_PARAM_MAX_CREDITS, &max_credit, p_tc_1101_sut_mgr->cid);
        if (cur_credit == max_credit)
        {
            //start total timer
            os_timer_start(&g_test_timer_handle);
        }
    }
}

void tc_1101_sut_chann_disconnected(uint16_t cid, uint16_t cause)
{
    if (cause == 0 || cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
    {
    }
    else
    {
        APP_PRINT_ERROR1("tc_1101_sut: error disconnect cause 0x%x", cause);
    }
    le_disconnect(0);
}
#endif

#if TC_1102_SUT_SUPPORT
typedef struct
{
    uint32_t total_rx_count;
    uint32_t rx_count;
    uint16_t mtu;
    uint16_t cid;
    uint8_t remote_bd[6];
} TC_1102_SUT_MGR;

TC_1102_SUT_MGR *p_tc_1102_sut_mgr = NULL;

void tc_1102_sut_start(uint8_t remote_bd[6])
{
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    uint16_t scan_timeout = 10000;

    if (NULL == p_tc_1102_sut_mgr)
    {
        p_tc_1102_sut_mgr = (TC_1102_SUT_MGR *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(TC_1102_SUT_MGR));
    }
    else
    {
        memset(p_tc_1102_sut_mgr, 0, sizeof(TC_1102_SUT_MGR));
    }

    memcpy(p_tc_1102_sut_mgr->remote_bd, remote_bd, 6);
    le_cbc_reg_psm(0x25, 1);
    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 6;
    conn_req_param.conn_interval_max = 6;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 300;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    le_connect(0, p_tc_1102_sut_mgr->remote_bd, GAP_REMOTE_ADDR_LE_PUBLIC,
               GAP_LOCAL_ADDR_LE_PUBLIC, scan_timeout
              );
}

void tc_1102_sut_conn_param_update_event(uint8_t conn_id)
{
    uint16_t con_interval;
    uint16_t conn_slave_latency;
    uint16_t conn_supervision_timeout;

    le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &con_interval, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
    le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);

    APP_PRINT_INFO4("tc_1102_sut_conn_param_update_event: interval = 0x%x, latency = 0x%x, timeout = 0x%x, total_rx_count %d",
                    con_interval, conn_slave_latency, conn_supervision_timeout,
                    p_tc_1102_sut_mgr->rx_count);

    data_uart_print("interval = 0x%x, latency = 0x%x, timeout = 0x%x, total_rx_count  %d\r\n",
                    con_interval, conn_slave_latency, conn_supervision_timeout,
                    p_tc_1102_sut_mgr->rx_count);
    p_tc_1102_sut_mgr->rx_count = 0;
}

void tc_1102_sut_dump_result(void)
{
    if (p_tc_1102_sut_mgr != NULL)
    {
        APP_PRINT_INFO2("tc 1102 sut: end: rx_count %d total_rx_count %d\r\n",
                        p_tc_1102_sut_mgr->rx_count,
                        p_tc_1102_sut_mgr->total_rx_count);

        data_uart_print("tc 1102 sut: end: rx_count %d total_rx_count %d\r\n",
                        p_tc_1102_sut_mgr->rx_count,
                        p_tc_1102_sut_mgr->total_rx_count);
    }
    else
    {
        data_uart_print("Not running\r\n");
    }
}

void tc_1102_sut_link_connected(uint8_t conn_id)
{
    le_cbc_create(conn_id, 0x25);
}

void tc_1102_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_local_disc_reason(TC_1102_BT41_TP_TEST,  reason);
    tc_1102_sut_dump_result();
}

void tc_1102_sut_chann_connected(uint16_t cid, uint16_t mtu, uint8_t credit)
{
    p_tc_1102_sut_mgr->mtu = mtu;
    p_tc_1102_sut_mgr->cid = cid;
}

void tc_1102_sut_receive_data(uint16_t cid, uint16_t length)
{
    p_tc_1102_sut_mgr->rx_count++;
    p_tc_1102_sut_mgr->total_rx_count++;
}

void tc_1102_sut_chann_disconnected(uint16_t cid, uint16_t cause)
{
    if (cause == 0 || cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
    {
    }
    else
    {
        APP_PRINT_ERROR1("tc_1102_sut: error disconnect cause 0x%x", cause);
    }
    le_disconnect(0);
}
#endif

