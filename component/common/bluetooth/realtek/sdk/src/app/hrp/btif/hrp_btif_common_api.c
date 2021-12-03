#include <trace_app.h>
#include <hrp_btif_common_api.h>

#if F_BT_LE_BTIF_SUPPORT

void hrp_register_req(uint16_t len, uint8_t *p_param_list)
{
//    btif_register_req()
}

void hrp_release_req(uint16_t len, uint8_t *p_param_list)
{
    //btif_release_req()
}
void hrp_dev_cfg_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_DEV_CFG_OPCODE cfg_op = (T_BTIF_DEV_CFG_OPCODE)p_param_list[0];
    //uint8_t *reqParam = &p_param_list[1];
    APP_PRINT_INFO1("hrp_dev_cfg_req   op= %d", cfg_op);
    switch (cfg_op)
    {
    /*
    case  BTIF_DEV_CFG_OP_NAME :
    {
        T_BTIF_DEV_CFG_DEV *p_name = (T_BTIF_DEV_CFG_DEV *)reqParam;
        btif_dev_cfg_name_req(p_name->cod, p_name->name);
        break;
    }

    case  BTIF_DEV_CFG_OP_DID_EIR:
    {
        T_BTIF_DEV_CFG_DID_EIR *p_did_eir = (T_BTIF_DEV_CFG_DID_EIR *)reqParam;
        btif_dev_did_eir_cfg_req(p_did_eir->vendor_id, p_did_eir->vendor_id_src,
                                 p_did_eir->product_id, p_did_eir->product_version);
        break;
    }

    case  BTIF_DEV_CFG_OP_EXT_EIR:
    {
        T_BTIF_DEV_CFG_EXT_EIR *p_ext_eir = (T_BTIF_DEV_CFG_EXT_EIR *)reqParam;
        btif_dev_ext_eir_cfg_req(p_ext_eir->p_data);
        break;
    }

    case  BTIF_DEV_CFG_OP_PAGE_SCAN:
    {
        T_BTIF_DEV_CFG_PAGE_SCAN *p_page_scan = (T_BTIF_DEV_CFG_PAGE_SCAN *)reqParam;
        btif_dev_page_scan_cfg_req(p_page_scan->scan_type, p_page_scan->interval,
                                   p_page_scan->window, p_page_scan->page_tout);
        break;
    }
    case  BTIF_DEV_CFG_OP_INQUIRY_SCAN:
    {
        T_BTIF_DEV_CFG_INQUIRY_SCAN *p_inquiry_scan = (T_BTIF_DEV_CFG_INQUIRY_SCAN *)reqParam;
        btif_dev_inquiry_scan_cfg_req(p_inquiry_scan->scan_type, p_inquiry_scan->interval,
                                      p_inquiry_scan->window);
        break;
    }
    case  BTIF_DEV_CFG_OP_INQUIRY_MODE:
    {
        T_BTIF_DEV_CFG_INQUIRY_MODE *p_inquiry_mode = (T_BTIF_DEV_CFG_INQUIRY_MODE *)reqParam;
        btif_dev_inquiry_mode_cfg_req(p_inquiry_mode->mode);
        break;
    }
    case  BTIF_DEV_CFG_OP_LINK_POLICY:
    {
        T_BTIF_DEV_CFG_LINK_POLICY *p_link_policy = (T_BTIF_DEV_CFG_LINK_POLICY *)reqParam;
        btif_dev_link_policy_cfg_req(p_link_policy->policy, p_link_policy->supv_tout);
        break;
    }
    case  BTIF_DEV_CFG_OP_BT_MODE:
    {
        T_BTIF_DEV_CFG_BT_MODE *p_bt_mode = (T_BTIF_DEV_CFG_BT_MODE *)reqParam;
        btif_dev_bt_mode_cfg_req(p_bt_mode->mode);
        break;
    }

    case  BTIF_DEV_CFG_OP_ACCEPT_CONN_ROLE:
    {
        T_BTIF_DEV_CFG_ACCEPT_ROLE *p_accept_role = (T_BTIF_DEV_CFG_ACCEPT_ROLE *)reqParam;
        btif_dev_accept_role_cfg_req(p_accept_role->role);
        break;
    }
    */
    default:
        break;
    }


}

void hrp_read_rssi_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_READ_RSSI_REQ *p_req = (T_BTIF_READ_RSSI_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_READ_RSSI_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_READ_RSSI_REQ) = %d",
                         len, sizeof(T_BTIF_READ_RSSI_REQ));
        return;
    }
    btif_read_rssi_req(p_req->bd_addr, p_req->remote_addr_type);
}
void hrp_vendor_cmd_req(uint16_t len, uint8_t *p_param_list)
{

}

void hrp_pairable_mode_set_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_PAIRABLE_MODE_SET_REQ *p_req = (T_BTIF_PAIRABLE_MODE_SET_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_PAIRABLE_MODE_SET_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_PAIRABLE_MODE_SET_REQ) = %d",
                         len, sizeof(T_BTIF_PAIRABLE_MODE_SET_REQ));
        return;
    }
    btif_pairable_mode_set_req(p_req->enable, p_req->enable, p_req->requirements,
                               p_req->io_capabilities,
                               p_req->oob_present);
}

void hrp_user_passkey_req_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_USER_PASSKEY_REQ_CFM *p_cfm = (T_BTIF_USER_PASSKEY_REQ_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_USER_PASSKEY_REQ_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_USER_PASSKEY_REQ_CFM) = %d",
                         len, sizeof(T_BTIF_USER_PASSKEY_REQ_CFM));
        return;
    }
    APP_PRINT_TRACE0("hrp_user_passkey_req_cfm");
    btif_user_passkey_req_cfm(p_cfm->bd_addr, p_cfm->remote_addr_type, p_cfm->cause);
}

void hrp_user_passkey_req_reply_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_USER_PASSKEY_REQ_REPLY_REQ *p_req = (T_BTIF_USER_PASSKEY_REQ_REPLY_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_USER_PASSKEY_REQ_REPLY_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_USER_PASSKEY_REQ_REPLY_REQ) = %d",
                         len, sizeof(T_BTIF_USER_PASSKEY_REQ_REPLY_REQ));
        return;
    }
    btif_user_passkey_req_reply_req(p_req->bd_addr, p_req->remote_addr_type, p_req->passkey,
                                    p_req->cause);
}

void hrp_authen_result_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_AUTHEN_RESULT_CFM *p_cfm = (T_BTIF_AUTHEN_RESULT_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_AUTHEN_RESULT_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_AUTHEN_RESULT_CFM) = %d",
                         len, sizeof(T_BTIF_AUTHEN_RESULT_CFM));
        return;
    }
    btif_authen_result_cfm(p_cfm->bd_addr, p_cfm->remote_addr_type,
                           p_cfm->key_type,
                           p_cfm->cause);
}

void hrp_authen_key_req_cfm(uint16_t len, uint8_t *p_param_list)
{
    uint8_t bd_addr[6];
    uint8_t remote_addr_type;
    uint8_t key_len;
    uint8_t link_key[28];
    uint8_t key_type;
    uint8_t cause;
    uint8_t pos = 0;

    memcpy(bd_addr, p_param_list + pos, 6); pos += 6;
    LE_ARRAY_TO_UINT8(remote_addr_type, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(key_len, p_param_list + pos); pos++;
    memcpy(link_key, p_param_list + pos, 28); pos += 28;
    LE_ARRAY_TO_UINT8(key_type, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos++;

    btif_authen_key_req_cfm(bd_addr, (T_BTIF_REMOTE_ADDR_TYPE)remote_addr_type, key_len, link_key,
                            (T_BTIF_KEY_TYPE)key_type, (T_BTIF_CAUSE)cause);
}

void hrp_user_cfm_req_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_USER_CFM_REQ_CFM *p_cfm = (T_BTIF_USER_CFM_REQ_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_USER_CFM_REQ_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_USER_CFM_REQ_CFM) = %d",
                         len, sizeof(T_BTIF_USER_CFM_REQ_CFM));
        return;
    }
    btif_user_cfm_req_cfm(p_cfm->bd_addr, p_cfm->remote_addr_type, p_cfm->cause);
}
#if F_BT_KEY_PRESS_SUPPORT
void hrp_keypress_notif_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_KEYPRESS_NOTIF_REQ *p_req = (T_BTIF_KEYPRESS_NOTIF_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_KEYPRESS_NOTIF_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_KEYPRESS_NOTIF_REQ) = %d",
                         len, sizeof(T_BTIF_KEYPRESS_NOTIF_REQ));
        return;
    }
    btif_keypress_notif_req(p_req->bd_addr, p_req->remote_addr_type, p_req->event_type);
}


#endif
#endif
