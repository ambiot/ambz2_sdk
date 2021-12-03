/**
*********************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hrp_profile_entry.c
* @brief
* @details   none.
* @author    Thomas
* @date      2017-11-8
* @version   v0.1
* *********************************************************************************************************
*/
#include <trace_app.h>
#include <hrp_profile_entry.h>

#if F_BT_DLPS_EN
#include <hrp_dlps.h>
#endif

extern void hrp_profile_system_send_cmd_ack(uint8_t cmd_group, uint16_t cmd_index,
                                            HRP_PROFILE_SYSTEM_CMD_STATUS status);

void hrp_profile_handle_req(uint8_t cmd_group, uint16_t cmd_index, uint16_t param_list_len,
                            uint8_t *p_param_list)
{
    if ((system_status == HRP_STATUS_ACTIVE) &&
        (active_module !=  HRP_MODULE_PROFILE))
    {
        APP_PRINT_ERROR2("wrong module status: system_status= %d  active_module= %d",
                         system_status, active_module);
        return ;
    }

    APP_PRINT_TRACE3("down cmd group: cmdGroup= %x  cmd_index= %d , posLength = %d",
                     cmd_group, cmd_index, param_list_len);
#if F_BT_DLPS_EN
    if (hrp_dlps_status == HRP_DLPS_STATUS_ACTIVED)
    {
        if (!((cmd_group == HRP_PROFILE_CMD_GROUP_EVENT_SYS) && (cmd_index == PROFILE_SYSTEM_EVENT_ACK)))
        {
            hrp_profile_system_send_cmd_ack(cmd_group, cmd_index, HRP_PROFILE_SYSTEM_COMMAND_COMPLETE);
        }
        hrp_dlps_allow_enter(true);
        APP_PRINT_INFO0("hrp_profile_handle_req  allow enter dlps");
    }

#endif

    switch (cmd_group)
    {
    case HRP_PROFILE_CMD_GROUP_SYSTEM:
        if (hrp_profile_handle_system[cmd_index] != NULL)
        {
            hrp_profile_handle_system[cmd_index](param_list_len, p_param_list);
        }
        break;



    case HRP_PROFILE_CMD_GROUP_LE:
        if (hrp_profile_handle_gap_le[cmd_index] != NULL)
        {
            hrp_profile_handle_gap_le[cmd_index](param_list_len, p_param_list);
        }
        break;
    case HRP_PROFILE_CMD_GROUP_SIMP_BLE:
        if (hrp_profile_handle_simp_ble[cmd_index] != NULL)
        {
            hrp_profile_handle_simp_ble[cmd_index](param_list_len, p_param_list);
        }
        break;
    case HRP_PROFILE_CMD_GROUP_GAPS_CLIENT:
        if (hrp_profile_handle_gaps_client[cmd_index] != NULL)
        {
            hrp_profile_handle_gaps_client[cmd_index](param_list_len, p_param_list);
        }
        break;
    default:
        APP_PRINT_INFO2("unknown cmd group: cmd_group= %d  cmd_index= %d",
                        cmd_group, cmd_index);
        break;
    }
}

//========================utils=============================//
void hrp_profile_commit_buf(uint8_t *destbuf, int *pos, void *srcbuf, int length)
{
    int position = *pos;
    memcpy(destbuf + position, srcbuf, length);
    *pos = position + length;
}

void hrp_profile_fetch_buf(void *destbuf, uint16_t *pos, void *srcbuf, int length)
{
    int position = *pos;
    memcpy(destbuf, (uint8_t *)srcbuf + position, length);
    *pos = position + length;
}

void hrp_profile_evet(uint8_t cmd_group, uint16_t cmd_index, uint16_t param_list_len,
                      uint8_t *p_param_list)
{
    HRP_MODULE_ID module_id = HRP_MODULE_PROFILE;

    APP_PRINT_INFO4("hrp_profile_evet : upperstream events: cmdGroup= %x  cmdIndex= %d , posLength = %d , pData= %b",
                    cmd_group, cmd_index, param_list_len, TRACE_BINARY(param_list_len, p_param_list));
    hrp_handle_upperstream_events(module_id,  cmd_group,
                                  cmd_index,  param_list_len, p_param_list);
}






