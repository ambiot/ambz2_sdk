/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     configuration_client.c
* @brief    Source file for configuration client model.
* @details  Data types and external functions declaration.
* @author   bill
* @date     2016-3-24
* @version  v1.0
* *************************************************************************************
*/

/* Add Includes here */
#include <string.h>
#include "provisioner_app.h"
#include "mesh_api.h"
#include "configuration.h"
#include "platform_opts.h"

mesh_model_info_t cfg_client;
uint16_t cfg_client_key_index; //!< NetKey or AppKey depends on the mesh_node.features.cfg_model_use_app_key

#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
#include "bt_mesh_app_lib_intf.h"
extern struct BT_MESH_LIB_PRIV bt_mesh_lib_priv;
#endif

static mesh_msg_send_cause_t cfg_client_send(uint16_t dst, uint8_t *pmsg, uint16_t len)
{
    mesh_msg_t mesh_msg;
    mesh_msg.pmodel_info = &cfg_client;
    access_cfg(&mesh_msg);
    mesh_msg.pbuffer = pmsg;
    mesh_msg.msg_len = len;
#if MESH_CONFIGURATION_MODEL_USE_APP_KEY
    if (mesh_node.features.cfg_model_use_app_key)
    {
        mesh_msg.app_key_index = cfg_client_key_index;
    }
    else
#endif
    {
        mesh_msg.akf = 0;
        mesh_msg.net_key_index = cfg_client_key_index;
    }
    mesh_msg.dst = dst;
    return access_send(&mesh_msg);
}

mesh_msg_send_cause_t cfg_compo_data_get(uint16_t dst, uint8_t page)
{
    cfg_compo_data_get_t msg;
    msg.page = page;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_COMPO_DATA_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_beacon_get(uint16_t dst)
{
    cfg_beacon_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_BEACON_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_beacon_set(uint16_t dst, uint8_t state)
{
    cfg_beacon_set_t msg;
    msg.state = state;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_BEACON_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_default_ttl_get(uint16_t dst)
{
    cfg_default_ttl_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_DEFAULT_TTL_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_default_ttl_set(uint16_t dst, uint8_t ttl)
{
    cfg_default_ttl_set_t msg;
    msg.ttl = ttl;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_DEFAULT_TTL_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_proxy_get(uint16_t dst)
{
    cfg_proxy_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_PROXY_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_proxy_set(uint16_t dst, uint8_t state)
{
    cfg_proxy_set_t msg;
    msg.state = state;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_PROXY_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_relay_get(uint16_t dst)
{
    cfg_relay_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_RELAY_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_relay_set(uint16_t dst, uint8_t state, uint8_t count, uint8_t steps)
{
    cfg_relay_set_t msg;
    msg.state = state;
    msg.count = count;
    msg.steps = steps;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_RELAY_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_net_transmit_get(uint16_t dst)
{
    cfg_net_transmit_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NET_TRANS_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_net_transmit_set(uint16_t dst, uint8_t count, uint8_t steps)
{
    cfg_net_transmit_set_t msg;
    msg.count = count;
    msg.steps = steps;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NET_TRANS_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_model_pub_get(uint16_t dst, uint16_t element_addr, uint32_t model_id)
{
    cfg_model_pub_get_t msg;
    msg.element_addr = element_addr;
    msg.model_id = MESH_MODEL_CONVERT(model_id);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_MODEL_PUB_GET);
    return cfg_client_send(dst, (uint8_t *)&msg,
                           sizeof(msg) - (MESH_IS_VENDOR_MODEL(model_id) ? 0 : 2));
}

mesh_msg_send_cause_t cfg_model_pub_set(uint16_t dst, uint16_t element_addr, bool va_flag,
                                        uint8_t *pub_addr, pub_key_info_t pub_key_info, uint8_t pub_ttl, pub_period_t pub_period,
                                        pub_retrans_info_t pub_retrans_info, uint32_t model_id)
{
    cfg_model_pub_va_set_t pub_set;
    uint8_t *pbuffer = (uint8_t *)&pub_set;
    uint8_t index;
    if (va_flag)
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_PUB_VA_SET);
        index = ACCESS_OPCODE_SIZE(MESH_MSG_CFG_MODEL_PUB_VA_SET);
    }
    else
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_PUB_SET);
        index = ACCESS_OPCODE_SIZE(MESH_MSG_CFG_MODEL_PUB_SET);
    }
    LE_WORD2EXTRN(pbuffer + index, element_addr);
    index += 2;
    memcpy(pbuffer + index, pub_addr, va_flag ? 16 : 2);
    index += va_flag ? 16 : 2;
    pub_key_info.rfu = 0;
    /* to avoid gcc compile warning */
    pub_key_info_p temp = &pub_key_info;
    LE_WORD2EXTRN(pbuffer + index, *(uint16_t *)temp);
    pbuffer[index + 2] = pub_ttl;
    pbuffer[index + 3] = *(uint8_t *)&pub_period;
    pbuffer[index + 4] = *(uint8_t *)&pub_retrans_info;
    index += 5;
    if (MESH_IS_VENDOR_MODEL(model_id))
    {
        LE_DWORD2EXTRN(pbuffer + index, model_id);
        index += 4;
    }
    else
    {
        LE_WORD2EXTRN(pbuffer + index, model_id >> 16);
        index += 2;
    }
    return cfg_client_send(dst, pbuffer, index);
}

mesh_msg_send_cause_t cfg_model_sub_add(uint16_t dst, uint16_t element_addr, bool va_flag,
                                        uint8_t *addr, uint32_t model_id)
{
    cfg_model_sub_va_add_t sub_add;
    uint8_t *pbuffer = (uint8_t *)&sub_add;
    if (va_flag)
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_SUB_VA_ADD);
    }
    else
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_SUB_ADD);
    }
    LE_WORD2EXTRN(pbuffer + 2, element_addr);
    memcpy(pbuffer + 4, addr, va_flag ? 16 : 2);
    uint8_t index = 4 + (va_flag ? 16 : 2);
    if (MESH_IS_VENDOR_MODEL(model_id))
    {
        LE_DWORD2EXTRN(pbuffer + index, model_id);
        index += 4;
    }
    else
    {
        LE_WORD2EXTRN(pbuffer + index, model_id >> 16);
        index += 2;
    }
    return cfg_client_send(dst, pbuffer, index);
}

mesh_msg_send_cause_t cfg_model_sub_delete(uint16_t dst, uint16_t element_addr, bool va_flag,
                                           uint8_t *addr, uint32_t model_id)
{
    cfg_model_sub_delete_t sub_delete;
    uint8_t *pbuffer = (uint8_t *)&sub_delete;
    if (va_flag)
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_SUB_VA_DELETE);
    }
    else
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_SUB_DELETE);
    }
    LE_WORD2EXTRN(pbuffer + 2, element_addr);
    memcpy(pbuffer + 4, addr, va_flag ? 16 : 2);
    uint8_t index = 4 + (va_flag ? 16 : 2);
    if (MESH_IS_VENDOR_MODEL(model_id))
    {
        LE_DWORD2EXTRN(pbuffer + index, model_id);
        index += 4;
    }
    else
    {
        LE_WORD2EXTRN(pbuffer + index, model_id >> 16);
        index += 2;
    }
    return cfg_client_send(dst, pbuffer, index);
}

mesh_msg_send_cause_t cfg_model_sub_delete_all(uint16_t dst, uint16_t element_addr,
                                               uint32_t model_id)
{
    cfg_model_sub_delete_all_t msg;
    msg.element_addr = element_addr;
    msg.model_id = MESH_MODEL_CONVERT(model_id);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_MODEL_SUB_DELETE_ALL);
    return cfg_client_send(dst, (uint8_t *)&msg,
                           sizeof(msg) - (MESH_IS_VENDOR_MODEL(model_id) ? 0 : 2));
}

mesh_msg_send_cause_t cfg_model_sub_overwrite(uint16_t dst, uint16_t element_addr, bool va_flag,
                                              uint8_t *addr, uint32_t model_id)
{
    cfg_model_sub_overwrite_t sub_overwrite;
    uint8_t *pbuffer = (uint8_t *)&sub_overwrite;
    if (va_flag)
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_SUB_VA_OVERWRITE);
    }
    else
    {
        ACCESS_OPCODE_BYTE(pbuffer, MESH_MSG_CFG_MODEL_SUB_OVERWRITE);
    }
    LE_WORD2EXTRN(pbuffer + 2, element_addr);
    memcpy(pbuffer + 4, addr, va_flag ? 16 : 2);
    uint8_t index = 4 + (va_flag ? 16 : 2);
    if (MESH_IS_VENDOR_MODEL(model_id))
    {
        LE_DWORD2EXTRN(pbuffer + index, model_id);
        index += 4;
    }
    else
    {
        LE_WORD2EXTRN(pbuffer + index, model_id >> 16);
        index += 2;
    }
    return cfg_client_send(dst, pbuffer, index);
}

mesh_msg_send_cause_t cfg_model_sub_get(uint16_t dst, uint16_t element_addr, uint32_t model_id)
{
    cfg_model_sub_get_t msg;
    if (MESH_IS_VENDOR_MODEL(model_id))
    {
        ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_VENDOR_MODEL_SUB_GET);
    }
    else
    {
        ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_SIG_MODEL_SUB_GET);
    }
    msg.element_addr = element_addr;
    msg.model_id = MESH_MODEL_CONVERT(model_id);
    return cfg_client_send(dst, (uint8_t *)&msg,
                           sizeof(msg) - (MESH_IS_VENDOR_MODEL(model_id) ? 0 : 2));
}

mesh_msg_send_cause_t cfg_net_key_add(uint16_t dst, uint16_t net_key_index, uint8_t net_key[16])
{
    cfg_net_key_add_t msg;
    msg.net_key_index = net_key_index;
    memcpy(msg.net_key, net_key, 16);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NET_KEY_ADD);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_net_key_update(uint16_t dst, uint16_t net_key_index, uint8_t net_key[16])
{
    cfg_net_key_update_t msg;
    msg.net_key_index = net_key_index;
    memcpy(msg.net_key, net_key, 16);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NET_KEY_UPDATE);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_net_key_delete(uint16_t dst, uint16_t net_key_index)
{
    cfg_net_key_delete_t msg;
    msg.net_key_index = net_key_index;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NET_KEY_DELETE);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_net_key_get(uint16_t dst)
{
    cfg_net_key_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NET_KEY_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_app_key_add(uint16_t dst, uint16_t net_key_index, uint16_t app_key_index,
                                      uint8_t app_key[16])
{
    cfg_app_key_add_t msg;
    msg.key_index[0] = net_key_index & 0xff;
    msg.key_index[1] = ((net_key_index >> 8) & 0x0f) + ((app_key_index & 0x0f) << 4);
    msg.key_index[2] = (app_key_index >> 4) & 0xff;
    memcpy(msg.app_key, app_key, 16);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_APP_KEY_ADD);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_app_key_update(uint16_t dst, uint16_t net_key_index,
                                         uint16_t app_key_index, uint8_t app_key[16])
{
    cfg_app_key_update_t msg;
    msg.key_index[0] = net_key_index & 0xff;
    msg.key_index[1] = ((net_key_index >> 8) & 0x0f) + ((app_key_index & 0x0f) << 4);
    msg.key_index[2] = (app_key_index >> 4) & 0xff;
    memcpy(msg.app_key, app_key, 16);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_APP_KEY_UPDATE);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_app_key_delete(uint16_t dst, uint16_t net_key_index,
                                         uint16_t app_key_index)
{
    cfg_app_key_delete_t msg;
    msg.key_index[0] = net_key_index & 0xff;
    msg.key_index[1] = ((net_key_index >> 8) & 0x0f) + ((app_key_index & 0x0f) << 4);
    msg.key_index[2] = (app_key_index >> 4) & 0xff;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_APP_KEY_DELETE);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_app_key_get(uint16_t dst, uint16_t net_key_index)
{
    cfg_app_key_get_t msg;
    msg.net_key_index = net_key_index;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_APP_KEY_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_node_identity_get(uint16_t dst, uint16_t net_key_index)
{
    cfg_node_identity_get_t msg;
    msg.net_key_index = net_key_index;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NODE_IDENTITY_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_node_identity_set(uint16_t dst, uint16_t net_key_index, uint8_t identity)
{
    cfg_node_identity_set_t msg;
    msg.net_key_index = net_key_index;
    msg.identity = identity;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NODE_IDENTITY_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_model_app_bind(uint16_t dst, uint16_t element_addr,
                                         uint16_t app_key_index,
                                         uint32_t model_id)
{
    cfg_model_app_bind_t msg;
    msg.element_addr = element_addr;
    msg.app_key_index = app_key_index;
    msg.model_id = MESH_MODEL_CONVERT(model_id);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_MODEL_APP_BIND);
    return cfg_client_send(dst, (uint8_t *)&msg,
                           sizeof(msg) - (MESH_IS_VENDOR_MODEL(model_id) ? 0 : 2));
}

mesh_msg_send_cause_t cfg_model_app_unbind(uint16_t dst, uint16_t element_addr,
                                           uint16_t app_key_index,
                                           uint32_t model_id)
{
    cfg_model_app_unbind_t msg;
    msg.element_addr = element_addr;
    msg.app_key_index = app_key_index;
    msg.model_id = MESH_MODEL_CONVERT(model_id);
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_MODEL_APP_UNBIND);
    return cfg_client_send(dst, (uint8_t *)&msg,
                           sizeof(msg) - (MESH_IS_VENDOR_MODEL(model_id) ? 0 : 2));
}

mesh_msg_send_cause_t cfg_model_app_get(uint16_t dst,  uint16_t element_addr, uint32_t model_id)
{
    cfg_model_app_get_t msg;
    if (MESH_IS_VENDOR_MODEL(model_id))
    {
        ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_VENDOR_MODEL_APP_GET);
    }
    else
    {
        ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_SIG_MODEL_APP_GET);
    }
    msg.element_addr = element_addr;
    msg.model_id = MESH_MODEL_CONVERT(model_id);
    return cfg_client_send(dst, (uint8_t *)&msg,
                           sizeof(msg) - (MESH_IS_VENDOR_MODEL(model_id) ? 0 : 2));
}

mesh_msg_send_cause_t cfg_node_reset(uint16_t dst)
{
    cfg_node_reset_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_NODE_RESET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_frnd_get(uint16_t dst)
{
    cfg_frnd_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_FRND_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_frnd_set(uint16_t dst, uint8_t state)
{
    cfg_frnd_set_t msg;
    msg.state = state;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_FRND_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_lpn_poll_timeout_get(uint16_t dst, uint16_t lpn_addr)
{
    cfg_lpn_poll_timeout_get_t msg;
    msg.lpn_addr = lpn_addr;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_LPN_POLL_TO_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_key_refresh_phase_get(uint16_t dst, uint16_t net_key_index)
{
    cfg_key_refresh_phase_get_t msg;
    msg.net_key_index = net_key_index;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_KEY_REFRESH_PHASE_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_key_refresh_phase_set(uint16_t dst, uint16_t net_key_index, uint8_t state)
{
    cfg_key_refresh_phase_set_t msg;
    msg.net_key_index = net_key_index;
    msg.state = state;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_KEY_REFRESH_PHASE_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_hb_pub_get(uint16_t dst)
{
    cfg_hb_pub_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_HB_PUB_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_hb_pub_set(uint16_t dst, uint16_t dst_pub, uint8_t count_log,
                                     uint8_t period_log,
                                     uint8_t ttl, hb_pub_features_t features, uint16_t net_key_index)
{
    cfg_hb_pub_set_t msg;
    msg.dst = dst_pub;
    msg.count_log = count_log;
    msg.period_log = period_log;
    msg.ttl = ttl;
    msg.features = features;
    msg.net_key_index = net_key_index;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_HB_PUB_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_hb_sub_get(uint16_t dst)
{
    cfg_hb_sub_get_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_HB_SUB_GET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

mesh_msg_send_cause_t cfg_hb_sub_set(uint16_t dst, uint16_t src, uint16_t dst_set,
                                     uint8_t period_log)
{
    cfg_hb_sub_set_t msg;
    msg.src = src;
    msg.dst = dst_set;
    msg.period_log = period_log;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_CFG_HB_SUB_SET);
    return cfg_client_send(dst, (uint8_t *)&msg, sizeof(msg));
}

#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO
#include "bt_mesh_app_lib_intf.h"
extern struct BT_MESH_LIB_PRIV bt_mesh_lib_priv;
extern void update_node_group(uint16_t mesh_addr, uint16_t group_addr);
#endif

bool cfg_client_receive(mesh_msg_p pmesh_msg)
{
    bool ret = TRUE;
    bool parse = FALSE;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    switch (pmesh_msg->access_opcode)
    {
    case MESH_MSG_CFG_BEACON_STAT:
        {
            parse = TRUE;
            cfg_beacon_stat_t *pmsg = (cfg_beacon_stat_t *)pbuffer;
            data_uart_debug("beacon state: %d\r\n", pmsg->state);
        }
        break;
    case MESH_MSG_CFG_COMPO_DATA_STAT:
        {
            cfg_compo_data_stat_t *pmsg = (cfg_compo_data_stat_t *)pbuffer;
            if (pmsg->page == 0)
            {
                parse = TRUE;
                uint8_t element_index = 0;
                uint8_t *pdata = pmsg->data;
                uint8_t *pend = pbuffer + pmesh_msg->msg_len;
                if (pmesh_msg->msg_len < (MEMBER_OFFSET(cfg_compo_data_stat_t, data) + 10))
                {
                    goto compo_data_end;
                }
                data_uart_debug("cdp0 parsed: src=0x%04x cid=0x%04x pid=0x%04x vid=0x%04x rpl=%d features=0x%04x\r\n",
                                pmesh_msg->src, LE_EXTRN2WORD(pdata), LE_EXTRN2WORD(pdata + 2),
                                LE_EXTRN2WORD(pdata + 4), LE_EXTRN2WORD(pdata + 6), LE_EXTRN2WORD(pdata + 8));
                pdata += 10;
                while (pdata < pend)
                {
                    if (pend - pdata < 4)
                    {
                        goto compo_data_end;
                    }
                    uint8_t sig_model_num = pdata[2];
                    uint8_t vendor_model_num = pdata[3];
                    uint8_t model_num;
                    data_uart_debug("element %d: loc=%d sig=%d vendor=%d\r\n", element_index, LE_EXTRN2WORD(pdata),
                                    sig_model_num, vendor_model_num);
                    pdata += 4;
                    if (sig_model_num)
                    {
                        if (pend - pdata < (sig_model_num << 1))
                        {
                            goto compo_data_end;
                        }
                        data_uart_debug("\ts:");
                        for (model_num = 0; model_num < sig_model_num; model_num++, pdata += 2)
                        {
                            data_uart_debug(" 0x%04xffff", LE_EXTRN2WORD(pdata));
                        }
                        data_uart_debug("\r\n");
                    }
                    if (vendor_model_num)
                    {
                        if (pend - pdata < (vendor_model_num << 2))
                        {
                            goto compo_data_end;
                        }
                        data_uart_debug("\tv:");
                        for (model_num = 0; model_num < vendor_model_num; model_num++, pdata += 4)
                        {
                            data_uart_debug(" 0x%08x", LE_EXTRN2DWORD(pdata));
                        }
                        data_uart_debug("\r\n");
                    }
                    element_index++;
                }
                break;
compo_data_end:
                parse = FALSE;
                data_uart_debug("cdp0 of 0x%04x invalid!\r\n", pmesh_msg->src);
            }
        }
        break;
    case MESH_MSG_CFG_DEFAULT_TTL_STAT:
        {
            parse = TRUE;
            cfg_default_ttl_stat_t *pmsg = (cfg_default_ttl_stat_t *)pbuffer;
            data_uart_debug("default ttl: %d\r\n", pmsg->ttl);
        }
        break;
    case MESH_MSG_CFG_PROXY_STAT:
        {
            parse = TRUE;
            cfg_proxy_stat_t *pmsg = (cfg_proxy_stat_t *)pbuffer;
            data_uart_debug("proxy state: %d\r\n", pmsg->state);
        }
        break;
    case MESH_MSG_CFG_RELAY_STAT:
        {
            parse = TRUE;
            cfg_relay_stat_t *pmsg = (cfg_relay_stat_t *)pbuffer;
            data_uart_debug("relay state: %d, count = %d step = %d\r\n", pmsg->state, pmsg->count, pmsg->steps);
        }
        break;
    case MESH_MSG_CFG_NET_TRANS_STAT:
        {
            parse = TRUE;
            cfg_net_transmit_stat_t *pmsg = (cfg_net_transmit_stat_t *)pbuffer;
            data_uart_debug("net transmit state: count = %d step = %d\r\n", pmsg->count, pmsg->steps);
        }
        break;
    case MESH_MSG_CFG_MODEL_PUB_STAT:
        {
            cfg_model_pub_stat_t *pmsg = (cfg_model_pub_stat_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                data_uart_debug("Pub stat: pub addr = 0x%04x ttl = %d!\r\n", pmsg->pub_addr, pmsg->pub_ttl);
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                if (bt_mesh_lib_priv.connect_device_sema && bt_mesh_lib_priv.connect_device_mesh_addr == pmesh_msg->src) {
                    bt_mesh_lib_priv.connect_device_flag = 1;
                    rtw_up_sema(&bt_mesh_lib_priv.connect_device_sema);
                }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                if (bt_mesh_indication(GEN_MESH_CODE(_model_pub_set), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_pub_set));  
                }
#endif
            }
            else
            {
                data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                if (bt_mesh_lib_priv.connect_device_sema && bt_mesh_lib_priv.connect_device_mesh_addr == pmesh_msg->src) {
                    bt_mesh_lib_priv.connect_device_flag = 0;
                    rtw_up_sema(&bt_mesh_lib_priv.connect_device_sema);
                }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                if (bt_mesh_indication(GEN_MESH_CODE(_model_pub_set), BT_MESH_USER_CMD_FAIL, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_pub_set));  
                }
#endif
            }
        }
        break;
    case MESH_MSG_CFG_MODEL_SUB_STAT:
        {
            cfg_model_sub_stat_t *pmsg = (cfg_model_sub_stat_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                parse = TRUE;
                data_uart_debug("Success!\r\n");
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                if (bt_mesh_lib_priv.set_node_group_sema && bt_mesh_lib_priv.set_node_group_mesh_addr == pmesh_msg->src) {
                    bt_mesh_lib_priv.set_node_group_flag = 1;
                    rtw_up_sema(&bt_mesh_lib_priv.set_node_group_sema);
                }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                {
                    uint8_t ret = USER_API_RESULT_ERROR;
                    ret = bt_mesh_indication(GEN_MESH_CODE(_model_sub_add), BT_MESH_USER_CMD_SUCCESS, (void *)&pmesh_msg->src);
                    if (ret != USER_API_RESULT_OK) {
                        if (ret != USER_API_RESULT_INCORRECT_CODE) {
                            data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_sub_add));
                            break;
                        }  
                    } else {
                        break;
                    }
                    ret = bt_mesh_indication(GEN_MESH_CODE(_model_sub_delete), BT_MESH_USER_CMD_SUCCESS, (void *)&pmesh_msg->src);
                    if (ret != USER_API_RESULT_OK) {
                        if (ret != USER_API_RESULT_INCORRECT_CODE) {
                            data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_sub_delete));
                            break;
                        }  
                    } else {
                        break;
                    }
                }
#endif
            }
            else
            {
                data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                if (bt_mesh_lib_priv.set_node_group_sema && bt_mesh_lib_priv.set_node_group_mesh_addr == pmesh_msg->src) {
                    bt_mesh_lib_priv.set_node_group_flag = 0;
                    rtw_up_sema(&bt_mesh_lib_priv.set_node_group_sema);
                }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                {
                    uint8_t ret = USER_API_RESULT_ERROR;
                    ret = bt_mesh_indication(GEN_MESH_CODE(_model_sub_add), BT_MESH_USER_CMD_FAIL, (void *)&pmesh_msg->src);
                    if (ret != USER_API_RESULT_OK) {
                        if (ret != USER_API_RESULT_INCORRECT_CODE) {
                            data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_sub_add));
                            break;
                        }  
                    } else {
                        break;
                    }
                    ret = bt_mesh_indication(GEN_MESH_CODE(_model_sub_delete), BT_MESH_USER_CMD_FAIL, (void *)&pmesh_msg->src);
                    if (ret != USER_API_RESULT_OK) {
                        if (ret != USER_API_RESULT_INCORRECT_CODE) {
                            data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_sub_delete));
                            break;
                        }  
                    } else {
                        break;
                    }
                }
#endif
            }
        }
        break;
    case MESH_MSG_CFG_SIG_MODEL_SUB_LIST:
    case MESH_MSG_CFG_VENDOR_MODEL_SUB_LIST:
        {
            cfg_model_sub_list_t *pmsg = (cfg_model_sub_list_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                parse = TRUE;
                uint32_t model_id = pmesh_msg->access_opcode == MESH_MSG_CFG_SIG_MODEL_SUB_LIST ?
                                    MESH_MODEL_TRANSFORM(pmsg->model_id) : pmsg->model_id;
                data_uart_debug("Model sub list: sr 0x%04x element idx %d model id 0x%08x\r\n\t", pmesh_msg->src,
                                pmsg->element_addr - pmesh_msg->src, model_id);
                uint16_t len = MEMBER_OFFSET(cfg_model_sub_list_t,
                                             model_id) + (pmesh_msg->access_opcode == MESH_MSG_CFG_SIG_MODEL_SUB_LIST ? 2 : 4);
                pbuffer += len;
                for (uint8_t loop = 0; loop < pmesh_msg->msg_len - len; loop += 2)
                {
                    data_uart_debug(" 0x%04x", LE_EXTRN2WORD(pbuffer + loop));
                }
#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO
                {
                    uint16_t mesh_addr = pmesh_msg->src;
                    uint16_t group_addr = 0;
                    if (pmesh_msg->msg_len - len != 0)
                        group_addr = LE_EXTRN2WORD(pbuffer + 0);
                    update_node_group(mesh_addr, group_addr);
                }
#endif
                data_uart_debug("\r\n");
            }
            else
            {
                data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
            }
        }
        break;
    case MESH_MSG_CFG_NET_KEY_STAT:
        {
            if (pmesh_msg->msg_len == sizeof(cfg_net_key_stat_t))
            {
                cfg_net_key_stat_t *pmsg = (cfg_net_key_stat_t *)pbuffer;
                if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
                {
                    parse = TRUE;
                    data_uart_debug("Success!\r\n");
                }
                else
                {
                    data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
                }
            }
        }
        break;
    case MESH_MSG_CFG_NET_KEY_LIST:
        {
            pbuffer += MEMBER_OFFSET(cfg_net_key_list_t, net_key_indexes);
            uint16_t len = pmesh_msg->msg_len - MEMBER_OFFSET(cfg_net_key_list_t, net_key_indexes);
            uint8_t remainder = len % 3;
            if (remainder == 0 || remainder == 2)
            {
                parse = TRUE;
                uint16_t key_count = len / 3 * 2 + (remainder != 0);
                data_uart_debug("NetKey List: num = %d!\r\n\t", key_count);
                for (uint16_t loop = 0; loop < key_count; loop++)
                {
                    uint16_t offset = (loop >> 1) * 3;
                    uint16_t key_index;
                    if (loop % 2 == 0)
                    {
                        key_index = 0xfff & LE_EXTRN2WORD(pbuffer + offset);
                    }
                    else
                    {
                        key_index = LE_EXTRN2WORD(pbuffer + offset + 1) >> 4;
                    }
                    data_uart_debug(" 0x%03x", key_index);
                }
                data_uart_debug("\r\n");
            }
            else
            {
                data_uart_debug("Fail, len = %d!\r\n", len);
            }
        }
        break;
    case MESH_MSG_CFG_APP_KEY_STAT:
        {
            if (pmesh_msg->msg_len == sizeof(cfg_app_key_stat_t))
            {
                cfg_app_key_stat_t *pmsg = (cfg_app_key_stat_t *)pbuffer;
                if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
                {
                    parse = TRUE;
                    data_uart_debug("Success!\r\n");
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                    if (bt_mesh_lib_priv.connect_device_sema && bt_mesh_lib_priv.connect_device_mesh_addr == pmesh_msg->src) {
                        bt_mesh_lib_priv.connect_device_flag = 1;
                        rtw_up_sema(&bt_mesh_lib_priv.connect_device_sema);
                    }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                    if (bt_mesh_indication(GEN_MESH_CODE(_app_key_add), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                        data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_app_key_add));  
                    }
#endif
                }
                else
                {
                    data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                    if (bt_mesh_lib_priv.connect_device_sema && bt_mesh_lib_priv.connect_device_mesh_addr == pmesh_msg->src) {
                        bt_mesh_lib_priv.connect_device_flag = 0;
                        rtw_up_sema(&bt_mesh_lib_priv.connect_device_sema);
                    }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                    if (bt_mesh_indication(GEN_MESH_CODE(_app_key_add), BT_MESH_USER_CMD_FAIL, NULL) != USER_API_RESULT_OK) {
                        data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_app_key_add));  
                    }
#endif
                }
            }
        }

        break;
    case MESH_MSG_CFG_APP_KEY_LIST:
        {
            cfg_app_key_list_t *pmsg = (cfg_app_key_list_t *)pbuffer;
            pbuffer += MEMBER_OFFSET(cfg_app_key_list_t, app_key_indexes);
            uint16_t len = pmesh_msg->msg_len - MEMBER_OFFSET(cfg_app_key_list_t, app_key_indexes);
            uint8_t remainder = len % 3;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS && (remainder == 0 || remainder == 2))
            {
                parse = TRUE;
                uint16_t key_count = len / 3 * 2 + (remainder != 0);
                data_uart_debug("AppKey List: NetKeyIndex = 0x%03x num = %d!\r\n\t", pmsg->net_key_index,
                                key_count);
                for (uint16_t loop = 0; loop < key_count; loop++)
                {
                    uint16_t offset = (loop >> 1) * 3;
                    uint16_t key_index;
                    if (loop % 2 == 0)
                    {
                        key_index = 0xfff & LE_EXTRN2WORD(pbuffer + offset);
                    }
                    else
                    {
                        key_index = LE_EXTRN2WORD(pbuffer + offset + 1) >> 4;
                    }
                    data_uart_debug(" 0x%03x", key_index);
                }
                data_uart_debug("\r\n");
            }
            else
            {
                data_uart_debug("Fail, stat = %d len = %d!\r\n", pmsg->stat, len);
            }
        }
        break;
    case MESH_MSG_CFG_NODE_IDENTITY_STAT:
        {
            cfg_node_identity_stat_t *pmsg = (cfg_node_identity_stat_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                parse = TRUE;
                data_uart_debug("node identity state: %d on NetKeyIndex 0x%03x\r\n", pmsg->identity,
                                pmsg->net_key_index);
            }
            else
            {
                data_uart_debug("Fail, stat = %d!\r\n", pmsg->stat);
            }
        }
        break;
    case MESH_MSG_CFG_MODEL_APP_STAT:
        {
            cfg_model_app_stat_t *pmsg = (cfg_model_app_stat_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                parse = TRUE;
                data_uart_debug("Success!\r\n");
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                if (bt_mesh_lib_priv.connect_device_sema && bt_mesh_lib_priv.connect_device_mesh_addr == pmesh_msg->src) {
                    bt_mesh_lib_priv.connect_device_flag = 1;
                    rtw_up_sema(&bt_mesh_lib_priv.connect_device_sema);
                }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                if (bt_mesh_indication(GEN_MESH_CODE(_model_app_bind), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_app_bind));  
                }
#endif
            }
            else
            {
                data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
                if (bt_mesh_lib_priv.connect_device_sema && bt_mesh_lib_priv.connect_device_mesh_addr == pmesh_msg->src) {
                    bt_mesh_lib_priv.connect_device_flag = 0;
                    rtw_up_sema(&bt_mesh_lib_priv.connect_device_sema);
                }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
                if (bt_mesh_indication(GEN_MESH_CODE(_model_app_bind), BT_MESH_USER_CMD_FAIL, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_model_app_bind));  
                }
#endif
            }
        }
        break;
    case MESH_MSG_CFG_SIG_MODEL_APP_LIST:
    case MESH_MSG_CFG_VENDOR_MODEL_APP_LIST:
        {
            cfg_model_app_list_t *pmsg = (cfg_model_app_list_t *)pbuffer;
            uint32_t model_id = pmesh_msg->access_opcode == MESH_MSG_CFG_SIG_MODEL_APP_LIST ?
                                MESH_MODEL_TRANSFORM(pmsg->model_id) : pmsg->model_id;
            uint8_t header_len = MEMBER_OFFSET(cfg_model_app_list_t,
                                               model_id) + (pmesh_msg->access_opcode == MESH_MSG_CFG_SIG_MODEL_APP_LIST ? 2 : 4);
            pbuffer += header_len;
            uint16_t len = pmesh_msg->msg_len - header_len;
            uint8_t remainder = len % 3;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS && (remainder == 0 || remainder == 2))
            {
                parse = TRUE;
                uint16_t key_count = len / 3 * 2 + (remainder != 0);
                data_uart_debug("Model AppKey List: Element Index = %d model id = 0x%08x num = %d!\r\n\t",
                                pmsg->element_addr - pmesh_msg->src, model_id, key_count);
                for (uint16_t loop = 0; loop < key_count; loop++)
                {
                    uint16_t offset = (loop >> 1) * 3;
                    uint16_t key_index;
                    if (loop % 2 == 0)
                    {
                        key_index = 0xfff & LE_EXTRN2WORD(pbuffer + offset);
                    }
                    else
                    {
                        key_index = LE_EXTRN2WORD(pbuffer + offset + 1) >> 4;
                    }
                    data_uart_debug(" 0x%03x", key_index);
                }
                data_uart_debug("\r\n");
            }
            else
            {
                data_uart_debug("Fail, stat = %d len = %d!\r\n", pmsg->stat, len);
            }
        }
        break;
    case MESH_MSG_CFG_NODE_RESET_STAT:
        {
            parse = TRUE;
            data_uart_debug("Node 0x%04x reseted!\r\n", pmesh_msg->src);
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
            if (bt_mesh_lib_priv.connect_device_nr_sema && bt_mesh_lib_priv.connect_device_nr_mesh_addr == pmesh_msg->src) {
                rtw_up_sema(&bt_mesh_lib_priv.connect_device_nr_sema);
            }
            if (bt_mesh_lib_priv.delete_node_sema && bt_mesh_lib_priv.delete_node_mesh_addr == pmesh_msg->src) {
                rtw_up_sema(&bt_mesh_lib_priv.delete_node_sema);
            }
#endif
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO
            if (bt_mesh_lib_priv.connect_device_nr_mesh_addr == pmesh_msg->src) {
                if (bt_mesh_indication(GEN_MESH_CODE(_node_reset), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_node_reset));  
                }
                break;
            }
            if (bt_mesh_lib_priv.delete_node_mesh_addr == pmesh_msg->src) {
                if (bt_mesh_indication(GEN_MESH_CODE(_node_reset), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_node_reset));  
                }
                break;
            }
            if (bt_mesh_indication(GEN_MESH_CODE(_node_reset), BT_MESH_USER_CMD_FAIL, NULL) != USER_API_RESULT_OK) {
                data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_node_reset));  
            }
#else
            if (bt_mesh_indication(GEN_MESH_CODE(_node_reset), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_node_reset));  
            }
#endif
#endif
        }
        break;
    case MESH_MSG_CFG_FRND_STAT:
        {
            parse = TRUE;
            cfg_frnd_stat_t *pmsg = (cfg_frnd_stat_t *)pbuffer;
            data_uart_debug("frnd state: %d\r\n", pmsg->state);
        }
        break;
    case MESH_MSG_CFG_LPN_POLL_TO_STAT:
        {
            parse = TRUE;
            cfg_lpn_poll_timeout_stat_t *pmsg = (cfg_lpn_poll_timeout_stat_t *)pbuffer;
            data_uart_debug("fn = 0x%04x lpn = 0x%04x poll_timeout = %d00ms\r\n", pmsg->lpn_addr,
                            pmsg->poll_to[0] + (pmsg->poll_to[1] << 8) + (pmsg->poll_to[2] << 16));
        }
        break;
    case MESH_MSG_CFG_KEY_REFRESH_PHASE_STAT:
        {
            cfg_key_refresh_phase_stat_t *pmsg = (cfg_key_refresh_phase_stat_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                parse = TRUE;

                data_uart_debug("Node 0x%04x: NetKeyIndex = 0x%03x state = %d\r\n", pmesh_msg->src,
                                pmsg->net_key_index, pmsg->state);
            }
            else
            {
                data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
            }
        }
        break;
    case MESH_MSG_CFG_HB_PUB_STAT:
        {
            cfg_hb_pub_stat_t *pmsg = (cfg_hb_pub_stat_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                parse = TRUE;

                data_uart_debug("Hb: pub dst = 0x%04x ttl = %d NetKeyIndex = 0x%03x\r\n", pmsg->dst, pmsg->ttl,
                                pmsg->net_key_index);
            }
            else
            {
                data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
            }
        }
        break;
    case MESH_MSG_CFG_HB_SUB_STAT:
        {
            cfg_hb_sub_stat_t *pmsg = (cfg_hb_sub_stat_t *)pbuffer;
            if (pmsg->stat == MESH_MSG_STAT_SUCCESS)
            {
                parse = TRUE;

                data_uart_debug("Hb: sub src = 0x%04x dst = 0x%04x hops range = [%d, %d]r\n", pmsg->src, pmsg->dst,
                                pmsg->min_hops, pmsg->max_hops);
            }
            else
            {
                data_uart_debug("Fail, status = %d!\r\n", pmsg->stat);
            }
        }
        break;
    default:
        ret = FALSE;
        break;
    }

    if (ret == TRUE)
    {
        if (parse == FALSE)
        {
            data_uart_debug("cfg_client_receive: opcode = 0x%x, len = %d, value = ", pmesh_msg->access_opcode,
                            pmesh_msg->msg_len);
            data_uart_dump(pbuffer, pmesh_msg->msg_len);
        }
        printi("cfg_client_receive: opcode = 0x%x, len = %d, value = ", pmesh_msg->access_opcode,
               pmesh_msg->msg_len);
        dprintt(pbuffer, pmesh_msg->msg_len);
    }
    return ret;
}

bool cfg_client_reg(void)
{
    if (NULL != mesh_model_info_get_by_model_id(0, MESH_MODEL_CFG_CLIENT))
    {
        return FALSE;
    }

    cfg_client.model_id = MESH_MODEL_CFG_CLIENT;
    cfg_client.model_receive = cfg_client_receive;
    return mesh_model_reg(0, &cfg_client);
}

bool cfg_client_key_set(uint16_t key_index)
{
    if (mesh_node.features.cfg_model_use_app_key && key_index >= mesh_node.app_key_num)
    {
        return false;
    }
    else if (mesh_node.features.cfg_model_use_app_key == 0 && key_index >= mesh_node.net_key_num)
    {
        return false;
    }
    else
    {
        cfg_client_key_index = key_index;
        return true;
    }
}

