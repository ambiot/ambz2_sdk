/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     mesh_cmd.c
  * @brief    Source file for mesh common cmd.
  * @details  User command interfaces.
  * @author   bill
  * @date     2017-3-31
  * @version  v1.0
  * *************************************************************************************
  */

/* Add Includes here */
#include <string.h>
#if defined(CONFIG_PLATFORM_8721D)
#include "ameba_soc.h"
#endif
#include "trace.h"
#include "app_msg.h"
#include "gap_wrapper.h"
#include "mesh_cmd.h"
#include "mesh_api.h"
#include "proxy_client.h"
#include "ping.h"
#include "tp.h"

bool dev_info_show_flag;
uint8_t proxy_client_conn_id;

user_cmd_parse_result_t user_cmd_reset(user_cmd_parse_value_t *pparse_value)
{
    //WDG_SystemReset(RESET_ALL, SW_RESET_APP_START);
    /* avoid gcc compile warning */
    (void)pparse_value;
    
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_list(user_cmd_parse_value_t *pparse_value)
{
    /* avoid gcc compile warning */
    (void)pparse_value;

    data_uart_debug("MeshState:\t%d\r\n", mesh_node.node_state);
    data_uart_debug("DevUUID:\t");
    data_uart_dump(mesh_node.dev_uuid, 16);
    uint8_t bt_addr[6];
    gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
    data_uart_debug("BTAddr:\t\t0x%02x%02x%02x%02x%02x%02x\r\n",
                    bt_addr[5], bt_addr[4], bt_addr[3], bt_addr[2], bt_addr[1], bt_addr[0]);
    for (uint16_t index = 0; index < mesh_node.dev_key_num; index++)
    {
        if (mesh_node.dev_key_list[index].used && mesh_node.dev_key_list[index].element_num != 0)
        {
            data_uart_debug("DevKey:\t\t%d-0x%04x-%d-", index, mesh_node.dev_key_list[index].unicast_addr,
                            mesh_node.dev_key_list[index].element_num);
            data_uart_dump(mesh_node.dev_key_list[index].dev_key, 16);
        }
    }
    for (uint16_t index = 0; index < mesh_node.app_key_num; index++)
    {
        if (mesh_node.app_key_list[index].key_state != MESH_KEY_STATE_INVALID)
        {
            data_uart_debug("AppKey:\t\t%d-0x%04x-%d-%d-%d\r\n", index,
                            mesh_node.app_key_list[index].app_key_index_g, mesh_node.app_key_list[index].key_state,
                            key_state_to_tx_loop(mesh_node.app_key_list[index].key_state),
                            mesh_node.app_key_list[index].net_key_binding);
            for (uint8_t loop = 0; loop < 2; loop++)
            {
                if (mesh_node.app_key_list[index].papp_key[loop] != NULL)
                {
                    data_uart_debug("\t\t");
                    data_uart_dump(mesh_node.app_key_list[index].papp_key[loop]->app_key, 16);
                }
            }
        }
    }
    for (uint16_t index = 0; index < mesh_node.net_key_num; index++)
    {
        if (mesh_node.net_key_list[index].key_state != MESH_KEY_STATE_INVALID)
        {
            data_uart_debug("NetKey:\t\t%d-0x%04x-%d-%d-%d\r\n", index,
                            mesh_node.net_key_list[index].net_key_index_g, mesh_node.net_key_list[index].key_state,
                            key_state_to_tx_loop(mesh_node.net_key_list[index].key_state),
                            key_state_to_key_refresh_phase(mesh_node.net_key_list[index].key_state));
            if (mesh_node.net_key_list[index].net_key_index_g & 0x8000)
            {
                break;
            }
            for (uint8_t loop = 0; loop < 2; loop++)
            {
                if (mesh_node.net_key_list[index].pnet_key[loop] != NULL)
                {
                    data_uart_debug("\t\t");
                    data_uart_dump(mesh_node.net_key_list[index].pnet_key[loop]->net_key, 16);
                }
            }
        }
    }
    data_uart_debug("IVindex:\t%d-0x%x\r\n", mesh_node.iv_update_flag, mesh_node.iv_index);
    data_uart_debug("Seq:\t\t0x%06x\r\n", mesh_node.seq);
    data_uart_debug("NodeAddr:\t0x%04x-%d-%d\r\n", mesh_node.unicast_addr,
                    mesh_node.element_queue.count, mesh_node.model_num);
    mesh_element_p pelement = (mesh_element_p)mesh_node.element_queue.pfirst;
    while (pelement != NULL)
    {
        data_uart_debug("Element:\t%d-%d\r\n", pelement->element_index, pelement->model_queue.count);
        mesh_model_p pmodel = (mesh_model_p)pelement->model_queue.pfirst;
        while (pmodel != NULL)
        {
            data_uart_debug("Model:\t\t%d-%d-0x%08x", pmodel->pmodel_info->model_index,
                            pmodel->model_index, pmodel->pmodel_info->model_id);
            uint8_t key_flag = true;
            for (uint16_t index = 0; index < mesh_node.app_key_num; index++)
            {
                if (plt_bit_pool_get(pmodel->app_key_binding, index) &&
                    mesh_node.app_key_list[index].key_state != MESH_KEY_STATE_INVALID)
                {
                    if (key_flag)
                    {
                        key_flag = false;
                        data_uart_debug("-(key:%d", index);
                    }
                    else
                    {
                        data_uart_debug("-%d", index);
                    }
                }
            }
            if (!key_flag)
            {
                data_uart_debug(")");
            }
            if (MESH_NOT_UNASSIGNED_ADDR(pmodel->pub_params.pub_addr))
            {
                data_uart_debug("-(pub:0x%04x-%d-%d)", pmodel->pub_params.pub_addr, pmodel->pub_params.pub_ttl,
                                pmodel->pub_params.pub_key_info.app_key_index);
            }
            mesh_model_p pmodelb = pmodel;
            while (pmodelb->pmodel_info->pmodel_bound != NULL)
            {
                pmodelb = (mesh_model_p)pmodelb->pmodel_info->pmodel_bound->pmodel;
            }
            mesh_addr_member_p paddr_element = (mesh_addr_member_p)pmodelb->sub_queue.pfirst;
            while (paddr_element != NULL)
            {
                if (paddr_element == (mesh_addr_member_p)pmodelb->sub_queue.pfirst)
                {
                    if (pmodelb != pmodel)
                    {
                        data_uart_debug("-(sub:-%d-%d-0x%04x",
                                        ((mesh_model_p)pmodel->pmodel_info->pmodel_bound->pmodel)->model_index,
                                        pmodelb->model_index, paddr_element->mesh_addr);
                    }
                    else
                    {
                        data_uart_debug("-(sub:0x%04x", paddr_element->mesh_addr);
                    }
                }
                else
                {
                    data_uart_debug("-0x%04x", paddr_element->mesh_addr);
                }
                paddr_element = paddr_element->pnext;
                if (paddr_element == NULL)
                {
                    data_uart_debug(")");
                }
            }
            pmodel = pmodel->pnext;
            data_uart_debug("\r\n");
        }
        pelement = pelement->pnext;
    }
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_dev_info_show(user_cmd_parse_value_t *pparse_value)
{
    dev_info_show_flag = pparse_value->dw_parameter[0] ? true : false;
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_dev_uuid_set(user_cmd_parse_value_t *pparse_value)
{
    plt_hex_to_bin(mesh_node.dev_uuid, (uint8_t *)pparse_value->pparameter[0], 16);
    data_uart_debug("DevUUID:\t");
    data_uart_dump(mesh_node.dev_uuid, 16);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_bearer_adv_set(user_cmd_parse_value_t *pparse_value)
{
    bearer_adv_set(pparse_value->dw_parameter[0]);
    data_uart_debug("Adv bearer: %s\r\n", pparse_value->dw_parameter[0] ? "on" : "off");
    return USER_CMD_RESULT_OK;
}

extern prov_auth_value_type_t prov_auth_value_type;
user_cmd_parse_result_t user_cmd_prov_auth_value(user_cmd_parse_value_t *pparse_value)
{
    uint8_t auth_data[16];
    uint8_t auth_value_len;
    uint8_t *auth_value;
    if (prov_auth_value_type != PROV_AUTH_VALUE_TYPE_ALPHANUMERIC)
    {
        auth_value_len = plt_hex_to_bin(auth_data, (uint8_t *)pparse_value->pparameter[0],
                                        sizeof(auth_data));
        auth_value = auth_data + sizeof(auth_data) - auth_value_len;
    }
    else
    {
        auth_value_len = strlen(pparse_value->pparameter[0]);
        auth_value = (uint8_t *)pparse_value->pparameter[0];
    }
    data_uart_debug("AuthValue: len=%d, value=", auth_value_len);
    data_uart_dump(auth_value, auth_value_len);
    return prov_auth_value_set(auth_value, auth_value_len) ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
}

user_cmd_parse_result_t user_cmd_vir_addr_set(user_cmd_parse_value_t *pparse_value)
{
    uint8_t label_uuid[16];
    uint16_t va_index;
    uint16_t va_addr;
    plt_hex_to_bin(label_uuid, (uint8_t *)pparse_value->pparameter[0], sizeof(label_uuid));
    va_index = vir_addr_add(label_uuid);
    va_addr = vir_addr_get(va_index);
    if (MESH_NOT_UNASSIGNED_ADDR(va_addr))
    {
        data_uart_debug("index=%d va=0x%04x\r\n", va_index, va_addr);
        return USER_CMD_RESULT_OK;
    }
    else
    {
        return USER_CMD_RESULT_ERROR;
    }
}

static uint32_t ping_time_us;
static uint32_t ping_time_ms;
static uint16_t pong_count;
static uint32_t rtime_sum;
static uint32_t ping_count;
static uint32_t pong_count_sum;
static uint32_t rtime_sum_sum;
static plt_timer_t ping_timer;
static ping_pong_type_t ping_type;
static uint16_t ping_dst;
static uint8_t ping_ttl;
static uint16_t ping_key_index;
static uint16_t pong_max_delay;
static uint32_t rtime_min;
static uint32_t rtime_max;
static mesh_msg_send_cause_t (*const ping_pf[3])(uint16_t dst, uint8_t ttl, uint16_t key_index,
                                                 uint16_t pong_max_delay) = {trans_ping, ping, big_ping};

int32_t tp_reveive(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    (void)type;
    mesh_msg_t *pmesh_msg = (mesh_msg_t *)pargs;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    tp_msg_t *pmsg = (tp_msg_t *)(pbuffer);
    
    data_uart_debug("From:0x%04x To:0x%04x Tid:%d Time:%dms\r\n", pmesh_msg->src, pmesh_msg->dst,
                    pmsg->tid, plt_time_read_ms());
    if (pmesh_msg->msg_len < sizeof(tp_msg_t))
    {
        data_uart_send_string(pmsg->padding, pmesh_msg->msg_len - MEMBER_OFFSET(tp_msg_t, padding));
        data_uart_debug("\r\n>");
    }
    return 0;
}

void pong_receive(uint16_t src, uint16_t dst, uint8_t hops_forward, ping_pong_type_t type,
                  uint8_t hops_reverse, uint16_t pong_delay)
{
    /* avoid gcc compile warning */
    (void)dst;
    (void)type;
   
    pong_count++;
    uint32_t pong_time_us = plt_time_read_us();
    uint32_t pong_time_ms = plt_time_read_ms();
    uint32_t diff_time = plt_time_diff(ping_time_ms, ping_time_us, pong_time_ms, pong_time_us);
    if (diff_time & 0x80000000)
    {
        diff_time &= 0x7fffffff;
        diff_time /= 1000;
    }
    if (ping_count == 1)
    {
        data_uart_debug("\b");
    }
    uint32_t rtime = diff_time - pong_delay * 10 + (diff_time > pong_delay * 10 ? 0 : 30);
    rtime_sum += rtime;
    if (rtime > rtime_max)
    {
        rtime_max = rtime;
    }
    if (rtime < rtime_min)
    {
        rtime_min = rtime;
    }
    data_uart_debug("%d\t0x%04x\t%d\t%d\t%d\t%d\t%d\t%d\r\n", pong_count, src, hops_forward,
                    hops_reverse, diff_time, pong_delay * 10, rtime, rtime_sum / pong_count);
}

#if defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER
extern void *bt_mesh_provisioner_evt_queue_handle; //!< Event queue handle
extern void *bt_mesh_provisioner_io_queue_handle; //!< IO queue handle
#elif defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE 
extern void *bt_mesh_device_evt_queue_handle; //!< Event queue handle
extern void *bt_mesh_device_io_queue_handle; //!< IO queue handle
#elif defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE 
extern void *bt_mesh_device_multiple_profile_evt_queue_handle;  //!< Event queue handle
extern void *bt_mesh_device_multiple_profile_io_queue_handle;   //!< IO queue handle
#elif defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE 
extern void *bt_mesh_provisioner_multiple_profile_evt_queue_handle;  //!< Event queue handle
extern void *bt_mesh_provisioner_multiple_profile_io_queue_handle;   //!< IO queue handle
#endif
static void ping_timeout_cb(void *ptimer)
{
    /* avoid gcc compile warning */
    (void)ptimer;
    uint8_t event = EVENT_IO_TO_APP;
    T_IO_MSG msg;
    msg.type = PING_TIMEOUT_MSG;
#if defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER
    if (os_msg_send(bt_mesh_provisioner_io_queue_handle, &msg, 0) == false)
    {
    }
    else if (os_msg_send(bt_mesh_provisioner_evt_queue_handle, &event, 0) == false)
    {
    }
#elif defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE 
    if (os_msg_send(bt_mesh_device_io_queue_handle, &msg, 0) == false)
    {
    }
    else if (os_msg_send(bt_mesh_device_evt_queue_handle, &event, 0) == false)
    {
    }
#elif defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE 
    if (os_msg_send(bt_mesh_device_multiple_profile_io_queue_handle, &msg, 0) == false)
    {
    }
    else if (os_msg_send(bt_mesh_device_multiple_profile_evt_queue_handle, &event, 0) == false)
    {
    }
#elif defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE 
    if (os_msg_send(bt_mesh_provisioner_multiple_profile_io_queue_handle, &msg, 0) == false)
    {
    }
    else if (os_msg_send(bt_mesh_provisioner_multiple_profile_evt_queue_handle, &event, 0) == false)
    {
    }
#endif
}

mesh_msg_send_cause_t ping_handle_timeout(void)
{
    ping_count++;
    pong_count_sum += pong_count;
    rtime_sum_sum += rtime_sum;
    data_uart_debug("count\tdst\tf-hops\tr-hops\ttime\tdelay\trtime\tatime\t%d\r\n", ping_count);
    ping_time_us = plt_time_read_us();
    ping_time_ms = plt_time_read_ms();
    pong_count = 0;
    rtime_sum = 0;
    return ping_pf[ping_type](ping_dst, ping_ttl, ping_key_index, pong_max_delay);
}

static user_cmd_parse_result_t common_ping(user_cmd_parse_value_t *pparse_value)
{
    ping_dst = pparse_value->dw_parameter[0];
    ping_ttl = pparse_value->dw_parameter[1];
    ping_key_index = pparse_value->dw_parameter[2];
    pong_max_delay = pparse_value->dw_parameter[3];
    uint16_t period = pparse_value->dw_parameter[4];
    if (0 == period || 0 == pparse_value->para_count)
    {
        if (ping_timer != NULL)
        {
            plt_timer_delete(ping_timer, 0);
            ping_timer = NULL;
        }
        if (0 == pparse_value->para_count)
        {
            pong_count_sum += pong_count;
            rtime_sum_sum += rtime_sum;
            data_uart_debug("ping statistic: ping=%d pong=%d time(ms) min=%d max=%d avg=%d\r\n", ping_count,
                            pong_count_sum,
                            rtime_min, rtime_max, rtime_sum_sum / pong_count_sum);
            return USER_CMD_RESULT_OK;
        }
    }
    else
    {
        if (period < pong_max_delay)
        {
            period = pong_max_delay;
        }
        if (NULL == ping_timer)
        {
            ping_timer = plt_timer_create("ping", period * 10, true, 0,
                                          ping_timeout_cb);
            if (ping_timer != NULL)
            {
                plt_timer_start(ping_timer, 0);
            }
        }
        else
        {
            plt_timer_change_period(ping_timer, period * 10, 0);
        }
    }
    ping_count = 0;
    pong_count = 0;
    pong_count_sum = 0;
    rtime_sum = 0;
    rtime_sum_sum = 0;
    rtime_min = 0xffffffff;
    rtime_max = 0x0;
    if (MESH_MSG_SEND_CAUSE_SUCCESS == ping_handle_timeout())
    {
        return USER_CMD_RESULT_OK;
    }
    else
    {
        return USER_CMD_RESULT_ERROR;
    }
}

user_cmd_parse_result_t user_cmd_trans_ping(user_cmd_parse_value_t *pparse_value)
{
    ping_type = PING_PONG_TYPE_TRANSPORT;
    return common_ping(pparse_value);
}

user_cmd_parse_result_t user_cmd_ping(user_cmd_parse_value_t *pparse_value)
{
    ping_type = PING_PONG_TYPE_ACCESS;
    return common_ping(pparse_value);
}

user_cmd_parse_result_t user_cmd_big_ping(user_cmd_parse_value_t *pparse_value)
{
    ping_type = PING_PONG_TYPE_ACCESS_BIG;
    return common_ping(pparse_value);
}

user_cmd_parse_result_t user_cmd_tp_msg(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint8_t ttl = pparse_value->dw_parameter[1];
    uint16_t app_key_index = pparse_value->dw_parameter[2];
    if (MESH_MSG_SEND_CAUSE_SUCCESS == tp_msg(dst, ttl, app_key_index,
                                              (uint8_t *)pparse_value->pparameter[3], strlen(pparse_value->pparameter[3])))
    {
        return USER_CMD_RESULT_OK;
    }
    else
    {
        return USER_CMD_RESULT_ERROR;
    }
}

user_cmd_parse_result_t user_cmd_tp_start(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint8_t ttl = pparse_value->dw_parameter[1];
    uint16_t app_key_index = pparse_value->dw_parameter[2];
    uint32_t count = pparse_value->dw_parameter[3];
    if (MESH_MSG_SEND_CAUSE_SUCCESS == tp_start(dst, ttl, app_key_index, count))
    {
        return USER_CMD_RESULT_OK;
    }
    else
    {
        return USER_CMD_RESULT_ERROR;
    }
}

user_cmd_parse_result_t user_cmd_connect(user_cmd_parse_value_t *pparse_value)
{
    uint8_t addr[6];
    plt_hex_to_bin(addr, (uint8_t *)pparse_value->pparameter[0], 6);
    plt_swap1(addr, 6);
    uint8_t addr_type = pparse_value->dw_parameter[1] ? GAP_LOCAL_ADDR_LE_RANDOM :
                        GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    conn_req_param.scan_interval = 0xA0;	//100ms
    conn_req_param.scan_window = 0x80;		//80ms
    conn_req_param.conn_interval_min = 0x60;	//120ms
    conn_req_param.conn_interval_max = 0x60;	//120ms
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 1000;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
    le_connect(GAP_PHYS_CONN_INIT_1M_BIT, addr, (T_GAP_REMOTE_ADDR_TYPE)addr_type,
               GAP_LOCAL_ADDR_LE_PUBLIC, 1000);
    data_uart_debug("Connecting ");
    plt_swap1(addr, 6);
    data_uart_dump(addr, 6);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_disconnect(user_cmd_parse_value_t *pparse_value)
{
    data_uart_debug("Disconnecting\r\n");
    le_disconnect(pparse_value->dw_parameter[0]);
    return USER_CMD_RESULT_OK;
}


user_cmd_parse_result_t user_cmd_proxy_discover(user_cmd_parse_value_t *pparse_value)
{
    data_uart_debug("Proxy Start Discover\r\n");
#if defined(MESH_DEVICE) && MESH_DEVICE
    /* for pts test */
    proxy_client_add(NULL);
    /* replace proxy server */
    //proxy_ctx_set_cb(proxy_ctx_id, proxy_client_data_in_write, proxy_service_receive);
    //proxy_ctx_set_filter_type(proxy_ctx_id, PROXY_CFG_FILTER_TYPE_BLACK_LIST);
#endif
    proxy_client_conn_id = pparse_value->dw_parameter[0];
    proxy_client_start_discovery(proxy_client_conn_id);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_proxy_cccd_operate(user_cmd_parse_value_t *pparse_value)
{
    /* Indicate which char CCCD command. */
    uint8_t cmd_type = (uint8_t)pparse_value->dw_parameter[0];
    /* Enable or disable, 1--enable, 0--disable. */
    bool cmd_data = (bool)pparse_value->dw_parameter[1];
    data_uart_debug("Proxy Cccd Operate\r\n");
    switch (cmd_type)
    {
    case 0:/* V3 Notify char notif enable/disable. */
        {
            proxy_client_data_out_cccd_set(proxy_client_conn_id, cmd_data);
            //proxy_ctx_set_link(proxy_ctx_id, proxy_client_conn_id);
            data_uart_debug("GATT bearer: on\r\n");
            if (bearer_adv_get())
            {
                data_uart_debug("\r\nThe adv bearer exits too, you can use \"bas\" cmd to close it\r\n");
            }
        }
        break;
    default:
        return USER_CMD_RESULT_WRONG_PARAMETER;
    }
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_proxy_list(user_cmd_parse_value_t *pparse_value)
{
    /* avoid gcc compile warning */
    (void)pparse_value;
    
    data_uart_debug("Proxy Server Handle List:\r\nidx\thandle\r\n");
    for (proxy_handle_type_t hdl_idx = HDL_PROXY_SRV_START; hdl_idx < HDL_PROXY_CACHE_LEN; hdl_idx++)
    {
        data_uart_debug("%d\t0x%x\r\n", hdl_idx, proxy_client_handle_get(proxy_client_conn_id, hdl_idx));
    }
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_proxy_cfg_set_filter_type(user_cmd_parse_value_t
                                                           *pparse_value)
{
    proxy_cfg_filter_type_t filter_type = (proxy_cfg_filter_type_t)pparse_value->dw_parameter[0];
    proxy_cfg_set_filter_type(proxy_ctx_id_get(proxy_client_conn_id, PROXY_CTX_TYPE_PROXY),
                              filter_type);
    data_uart_debug("Proxy cfg set filter type %d\r\n", filter_type);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_proxy_cfg_add_addr(user_cmd_parse_value_t *pparse_value)
{
    uint16_t addr = pparse_value->dw_parameter[0];
    proxy_cfg_add_remove_addr(proxy_ctx_id_get(proxy_client_conn_id, PROXY_CTX_TYPE_PROXY), TRUE, &addr,
                              1);
    data_uart_debug("Proxy cfg add addr 0x%04x\r\n", addr);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_proxy_cfg_remove_addr(user_cmd_parse_value_t *pparse_value)
{
    uint16_t addr = pparse_value->dw_parameter[0];
    proxy_cfg_add_remove_addr(proxy_ctx_id_get(proxy_client_conn_id, PROXY_CTX_TYPE_PROXY), FALSE,
                              &addr, 1);
    data_uart_debug("Proxy cfg remove addr 0x%04x\r\n", addr);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_log_set(user_cmd_parse_value_t *pparse_value)
{
    log_module_trace_set(TRACE_MODULE_LOWERSTACK, TRACE_LEVEL_ERROR,
                         pparse_value->dw_parameter[0] & 0x1);
    log_module_trace_set(TRACE_MODULE_LOWERSTACK, TRACE_LEVEL_WARN,
                         pparse_value->dw_parameter[0] & 0x1);
    log_module_trace_set(TRACE_MODULE_LOWERSTACK, TRACE_LEVEL_INFO,
                         pparse_value->dw_parameter[0] & 0x1);
    log_module_trace_set(TRACE_MODULE_LOWERSTACK, TRACE_LEVEL_TRACE,
                         pparse_value->dw_parameter[0] & 0x1);

    log_module_bitmap_trace_set(0x7FFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE,
                                pparse_value->dw_parameter[0] & 0x2);
    log_module_bitmap_trace_set(0x7FFFFFFFFFFFFFFF, TRACE_LEVEL_INFO,
                                pparse_value->dw_parameter[0] & 0x2);

    uint32_t log_value[MESH_LOG_LEVEL_SIZE];
    memset(log_value, pparse_value->dw_parameter[0] & 0x4 ? 0xff : 0, sizeof(log_value));
    diag_level_set(TRACE_LEVEL_TRACE, log_value);

    data_uart_debug("Log setting %d!\r\n", pparse_value->dw_parameter[0]);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_time(user_cmd_parse_value_t *pparse_value)
{
    /* avoid gcc compile warning */
    (void)pparse_value;
    uint32_t time_ms = plt_time_read_ms();
    uint32_t day, hour, minute, second;
    second = time_ms / 1000;
    minute = second / 60;
    second %= 60;
    hour = minute / 60;
    minute %= 60;
    day = hour / 24;
    hour %= 24;
    data_uart_debug("Time: %dd%dh%dm%ds\r\n", day, hour, minute, second);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_adv_power_set(user_cmd_parse_value_t *pparse_value)
{
    //uint8_t tx_gain[4] = {0x30, 0x80, 0xA0, 0xF0};
    int8_t tx_gain_info[4] = {-20, 0, 4, 8};
    //le_adv_set_tx_power(GAP_ADV_TX_POW_SET_1M, tx_gain[pparse_value->dw_parameter[0]]);
    data_uart_debug("Adv power %d dBm\r\n", tx_gain_info[pparse_value->dw_parameter[0]]);
    return USER_CMD_RESULT_OK;
}

void fn_cb(uint8_t frnd_index, fn_cb_type_t type, uint16_t lpn_addr);
user_cmd_parse_result_t user_cmd_fn_init(user_cmd_parse_value_t *pparse_value)
{
#if MESH_FN
    fn_params_t fn_params;
    uint8_t fn_num = pparse_value->dw_parameter[0];
    fn_params.queue_size = pparse_value->dw_parameter[1];
    if (pparse_value->dw_parameter[2] > 0 && pparse_value->dw_parameter[2] < 0xff)
    {
        mesh_node.frnd_rx_window = pparse_value->dw_parameter[2];
    }
    return fn_init(fn_num, &fn_params, fn_cb) ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
#else
    return USER_CMD_RESULT_ERROR;
#endif
}

user_cmd_parse_result_t user_cmd_fn_deinit(user_cmd_parse_value_t *pparse_value)
{
	/* avoid gcc compile warning */
    (void)pparse_value;
	fn_deinit();
	return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_hb_pub(user_cmd_parse_value_t *pparse_value)
{
    hb_pub_t pub;
    pub.dst = pparse_value->dw_parameter[0];
    pub.count = pparse_value->dw_parameter[1];
    pub.period = pparse_value->dw_parameter[2];
    pub.ttl = 0x7f;
    pub.features.relay = 0;
    pub.features.proxy = 0;
    pub.features.frnd = 0;
    pub.features.lpn = 0;
    pub.features.rfu = 0;
    pub.net_key_index = 0;

    hb_publication_set(pub);
    hb_timer_start(HB_TIMER_PUB);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_hb_sub(user_cmd_parse_value_t *pparse_value)
{
    hb_sub_t sub;
    sub.src = pparse_value->dw_parameter[0];
    sub.dst = pparse_value->dw_parameter[1];
    sub.period = pparse_value->dw_parameter[2];
    sub.count = 0;
    sub.min_hops = 0x7f;
    sub.max_hops = 0;

    hb_subscription_set(sub);
    hb_timer_start(HB_TIMER_SUB);
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_mesh_deinit(user_cmd_parse_value_t *pparse_value)
{
    /* avoid gcc compile warning */
    (void)pparse_value;
    
    mesh_deinit();
    return USER_CMD_RESULT_OK;
}

