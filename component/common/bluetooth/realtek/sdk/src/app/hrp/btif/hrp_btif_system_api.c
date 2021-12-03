#include <os_msg.h>
#include <trace_app.h>


#include <gap_le.h>

#include <hrp_application.h>
#include <hrp_btif_entry.h>
#include <hrp_btif_system_api.h>

#if F_BT_DLPS_EN
#include <hrp_dlps.h>
#endif

#if F_BT_LE_BTIF_SUPPORT

T_L2C_DATA_TRANS l2c_data_trans;

/**
 * @brief callback function, upper stack will call it to send message to ltp.
 *
 * @param pMsg --message pointer from upper stack.
 * @return none.
 * @retal void
*/
void hrp_btif_callback(T_BTIF_UP_MSG *pMsg)
{
    unsigned char Event = LTP_EVENT_BTIF_MESSAGE;

    if (os_msg_send(P_BtHrp->p_aci_tcb->QueueHandleMessage, &pMsg, 0) == false)
    {
        btif_buffer_put(pMsg);
    }
    else
    {
        hrp_send_event(&Event);    /* signal event to ltp task */
    }
}

void hrp_system_reset_rsp(uint16_t cause)
{

    HRP_MODULE_ID module_id = HRP_MODULE_UPPER_STACK;
    uint8_t cmd_group = HRP_BTIF_CMD_GROUP_SYSTEM;
    uint16_t cmd_index = HRP_SYSTEM_RESET_RSP;
    uint16_t param_list_len =  2;
    uint16_t *p_param_list = &cause;

    hrp_handle_upperstream_events(module_id,  cmd_group,
                                  cmd_index,  param_list_len, (uint8_t *)p_param_list);

}

void hrp_system_init_rsp(uint16_t cause)
{

    HRP_MODULE_ID module_id = HRP_MODULE_UPPER_STACK;
    uint8_t cmd_group = HRP_BTIF_CMD_GROUP_SYSTEM;
    uint16_t cmd_index = HRP_SYSTEM_INIT_RSP;
    uint16_t param_list_len =  3;
    uint8_t param_list[3];
    uint8_t *p_param_list = &param_list[0];
    uint8_t pos = 0;
    LE_UINT16_TO_ARRAY(p_param_list + pos, cause); pos += 2;
    LE_UINT8_TO_ARRAY(p_param_list + pos, HRP_SUB_TYPE);

    hrp_handle_upperstream_events(module_id,  cmd_group,
                                  cmd_index,  param_list_len, (uint8_t *)p_param_list);

}
void hrp_system_send_cmd_ack(uint8_t cmd_group, uint16_t cmd_index,
                             HRP_SYSTEM_CMD_STATUS status)
{
    HRP_MODULE_ID module_id = HRP_MODULE_UPPER_STACK;
    uint8_t event_group = HRP_BTIF_CMD_GROUP_SYSTEM;
    uint16_t event_index = HRP_SYSTEM_CMD_ACK;
    uint16_t param_list_len =  5;
    uint8_t param_list[5];
    uint8_t *p_param_list = &param_list[0];
    uint8_t pos = 0;
    uint32_t cmd_id = (((uint32_t)cmd_group) << 16) + cmd_index;

    LE_UINT32_TO_ARRAY(p_param_list + pos, cmd_id); pos += 4;
    LE_UINT8_TO_ARRAY(p_param_list + pos, status);

    hrp_handle_upperstream_events(module_id,  event_group,
                                  event_index,  param_list_len, p_param_list);
}
void hrp_system_enable_dlps_rsp(bool status)
{
    HRP_MODULE_ID module_id = HRP_MODULE_UPPER_STACK;
    uint8_t cmd_group = HRP_BTIF_CMD_GROUP_SYSTEM;
    uint16_t cmd_index = HRP_SYSTEM_ENABLE_DLPS_RSP;
    uint16_t param_list_len = 1;
    uint8_t param_list = status;

    hrp_handle_upperstream_events(module_id,  cmd_group,
                                  cmd_index,  param_list_len, &param_list);
}

void hrp_btif_system_reset(uint16_t len, uint8_t *p_param_list)
{
    APP_PRINT_INFO1("hrp_btif_system_reset system_status= %d", system_status);
    hrp_system_reset_rsp(BTIF_CAUSE_SUCCESS);
    hrp_system_reset();
}

void hrp_btif_system_init(uint16_t len, uint8_t *p_param_list)
{
    APP_PRINT_INFO1("system_status= %d", system_status);
    if (system_status != HRP_STATUS_RESET)
    {
        APP_PRINT_ERROR0("wrong status");
        return;
    }

    btif_register_req(hrp_btif_callback);

    system_status = HRP_STATUS_ACTIVE;
    active_module =  HRP_MODULE_UPPER_STACK;
    l2c_data_trans.is_testing = false;
    hrp_system_init_rsp(BTIF_CAUSE_SUCCESS);
}



void hrp_btif_system_event_ack(uint16_t len, uint8_t *p_param_list)
{
#if 0
    uint16_t pos = 0;
    uint32_t event_index;
    uint8_t status;
    LE_ARRAY_TO_UINT32(event_index, p_param_list); pos += 4;
    LE_ARRAY_TO_UINT8(status, p_param_list); pos++;

    //hrp_utils_handle_event_ack(event_index, status);
#endif
}

void hrp_btif_system_read_dlps_count_rsp(uint16_t cause, uint32_t count)
{
    HRP_MODULE_ID module_id = HRP_MODULE_UPPER_STACK;
    uint8_t cmd_group = HRP_BTIF_CMD_GROUP_SYSTEM;
    uint16_t cmd_index = HRP_SYSTEM_READ_DLPS_COUNT_RSP;
    uint16_t param_list_len =  6;
    uint8_t p_param_list[6] = {0};

    p_param_list[0] = cause & 0xff;
    p_param_list[1] = (cause >> 8) & 0xff;
    p_param_list[2] = count & 0xff;
    p_param_list[3] = (count >> 8) & 0xff;
    p_param_list[4] = (count >> 16) & 0xff;
    p_param_list[5] = (count >> 24) & 0xff;

    hrp_handle_upperstream_events(module_id,  cmd_group,
                                  cmd_index,  param_list_len, p_param_list);
}

void  hrp_btif_system_read_dlps_count_req(uint16_t len, uint8_t *p_param_list)
{
#if F_BT_DLPS_EN
    APP_PRINT_INFO1("hrp_btif_system_read_dlps_count_req count = %d", enter_dlps_count);
    hrp_btif_system_read_dlps_count_rsp(0, enter_dlps_count);
#endif
}


void hrp_btif_system_enable_dlps_req(uint16_t len, uint8_t *p_param_list)
{
    APP_PRINT_INFO0("hrp_btif_system_enable_dlps_req");
#if F_BT_DLPS_EN
    uint16_t pos = 0;
    uint8_t active;
    LE_ARRAY_TO_UINT8(active, p_param_list); pos++;

    hrp_dlps_active_dlps(active);
    hrp_dlps_allow_enter(false);
    hrp_system_enable_dlps_rsp(true);
#else
    hrp_system_enable_dlps_rsp(false);
#endif
}

void hrp_btif_system_le_cfg_passkey_value(uint16_t len, uint8_t *p_param_list)
{
    T_LE_CFG_PASSKEY_VALUE *p = (T_LE_CFG_PASSKEY_VALUE *)p_param_list;
    if (len != sizeof(T_LE_CFG_PASSKEY_VALUE))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_LE_CFG_PASSKEY_VALUE) = %d",
                         len, sizeof(T_LE_CFG_PASSKEY_VALUE));
        return;
    }
    btif_le_cfg_passkey_value(p->leFixedDisplayValue);

}

#endif
