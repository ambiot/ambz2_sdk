/**
*********************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hrp_profile_system_api.c
* @brief
* @details   none.
* @author    Thomas
* @date      2017-11-8
* @version   v0.1
* *********************************************************************************************************
*/

#include <trace_app.h>

#include "hrp_profile_system_api.h"


#include <hrp_gap_le.h>

#if F_BT_DLPS_EN
#include <hrp_dlps.h>
#endif


//#include "cod.h"


#define REMOVE_OLD_PROFILE_INIT 1

extern void *hIoQueueHandle;

uint8_t hrp_gap_support_mode = 0;

uint8_t hrp_gap_initiated_mode = 0;

//=============================event============================//
static void hrp_profile_sys_event(uint16_t cmd_index, uint16_t param_list_len,
                                  uint8_t *p_param_list)
{
    uint8_t cmd_group = HRP_PROFILE_CMD_GROUP_EVENT_SYS;
    hrp_profile_evet(cmd_group, cmd_index,  param_list_len, p_param_list);
}
static void hrp_profile_system_cmd_result(uint8_t sysCause)
{
    hrp_profile_sys_event(PROFILE_SYSTEM_CMD_RESULT, sizeof(uint8_t), &sysCause);
}

static void hrp_profile_system_reset_cmp(uint8_t sysCause)
{
    hrp_profile_sys_event(PROFILE_SYSTEM_RESET_COMPLETE, sizeof(uint8_t), &sysCause);
}

static void hrp_profile_system_init_cmp(uint8_t sysCause)
{
    uint8_t Param[8];
    int pos = 0;

    LE_UINT8_TO_ARRAY(Param + pos, sysCause); pos += 1;
    if (sysCause == PROFILE_SYS_CMD_SUCESS)
    {
        gap_get_param(GAP_PARAM_BD_ADDR, P_BtHrp->ownBDAddress);
        memcpy(Param + pos, P_BtHrp->ownBDAddress, 6);
    }

    pos += 6;
    LE_UINT8_TO_ARRAY(Param + pos, sysCause); pos += 1;
    hrp_profile_sys_event(PROFILE_SYSTEM_INIT_COMPLETE, pos, Param);
}

static void hrp_profile_system_set_gap_param_rsp(PROFILE_SYS_CAUSE sysCause)
{

    hrp_profile_sys_event(PROFILE_SYSTEM_SET_GAP_PARAM_RSP, sizeof(uint8_t), &(sysCause));
}

static void hrp_profile_system_set_pairable_mode_rsp(PROFILE_SYS_CAUSE sysCause)
{
    hrp_profile_sys_event(PROFILE_SYSTEM_SET_PAIRABLE_MODE_RSP, sizeof(uint8_t), &(sysCause));
}

static void hrp_profile_system_enable_dlps_rsp(PROFILE_SYS_CAUSE sysCause)
{
    hrp_profile_sys_event(PROFILE_SYSTEM_ENABLE_DLPS_RSP, sizeof(uint8_t), &(sysCause));
}

void hrp_profile_system_send_cmd_ack(uint8_t cmd_group, uint16_t cmd_index,
                                     HRP_PROFILE_SYSTEM_CMD_STATUS status)
{
    HRP_MODULE_ID module_id = HRP_MODULE_PROFILE;
    uint8_t event_group = HRP_PROFILE_CMD_GROUP_EVENT_SYS;
    uint16_t event_index = PROFILE_SYSTEM_CMD_ACK;
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

//======================handle legacy msg=============================//
void hrp_profile_handle_initCmplete(uint8_t initiated_mode)
{
    hrp_gap_initiated_mode |= initiated_mode;

    /* both supported mode have completed initiate */
    if ((hrp_gap_initiated_mode & hrp_gap_support_mode) == hrp_gap_support_mode)
    {
        APP_PRINT_TRACE0("gap init complete\n");
        hrp_profile_system_init_cmp(PROFILE_SYS_CMD_SUCESS);
    }
}

//===============================cmd=============================//
void hrp_profile_system_reset(uint16_t len, uint8_t *p_param_list)
{
    APP_PRINT_INFO1("hrp_profile_system_reset:system_status= %d", system_status);
    hrp_profile_system_cmd_result(PROFILE_SYS_CMD_SUCESS);
    hrp_profile_system_reset_cmp(PROFILE_SYS_CMD_SUCESS);
    hrp_system_reset();
}
T_GAP_CAUSE hrp_gap_common_set_param(uint16_t type, uint8_t len, void *p_value)
{
    APP_PRINT_INFO2("hrp_gap_common_set_param: type:%x  data= %b", type, TRACE_BINARY(len, p_value));
    return gap_set_param((T_GAP_PARAM_TYPE)type, len, (void *)p_value);
}

void hrp_profile_system_set_gap_param(uint16_t len, uint8_t *p_param_list)
{
    uint8_t pos = 0;
    uint8_t param_type;
    uint8_t data_len;
    uint16_t data_type;
    uint8_t *p_value;
    T_GAP_CAUSE return_cause = GAP_CAUSE_INVALID_PARAM;

    if (len < GAP_DATA_TYPE_LEN + 2)
    {
        APP_PRINT_ERROR0("hrp_profile_system_set_gap_param: wrong length");
        hrp_profile_system_set_gap_param_rsp(PROFILE_SYS_CMD_INVALID_PARAM);
        return;
    }

    while ((pos + 1 + GAP_DATA_TYPE_LEN + 1) < len)
    {
        /* parse data field: data_len(1 byte) + type(GAP_DATA_TYPE_LEN bytes) + value (data_len - GAP_DATA_TYPE_LEN)*/
        param_type = p_param_list[pos++];
        data_len = p_param_list[pos++];
        if (data_len < 3)
        {
            APP_PRINT_ERROR1("hrp_profile_system_set_gap_param: invalid data_len =%d ", data_len);
            hrp_profile_system_set_gap_param_rsp(PROFILE_SYS_CMD_INVALID_PARAM);
            return;
        }
        LE_ARRAY_TO_UINT16(data_type, p_param_list + pos); pos += 2;
        p_value = p_param_list + pos;
        pos += data_len - GAP_DATA_TYPE_LEN;

        APP_PRINT_INFO4("param_type %x data_len %x data_type %x data %b", param_type, data_len, data_type,
                        TRACE_BINARY(data_len - GAP_DATA_TYPE_LEN, p_value));
        switch (param_type)
        {
        case GAP_PARAM_TYPE_COMMON:
            return_cause = hrp_gap_common_set_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
        case GAP_PARAM_TYPE_LEGACY:
            //hrp_gap_legacy_set_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
        case GAP_PARAM_TYPE_LEGACY_BOND:
            // hrp_gap_legacy_set_bond_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
        case GAP_PARAM_TYPE_LE:
            return_cause = hrp_le_set_gap_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
        case GAP_PARAM_TYPE_LE_BOND:
            return_cause = hrp_le_set_bond_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
#if F_BT_LE_GAP_SCAN_SUPPORT
        case GAP_PARAM_TYPE_LE_SCAN:
            return_cause = hrp_le_set_scan_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
#endif
#if F_BT_LE_PRIVACY_SUPPORT
        case GAP_PARAM_TYPE_LE_PRIVACY:
            return_cause = hrp_le_set_privacy_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
#endif
        case GAP_PARAM_TYPE_LE_ADV:
            return_cause = hrp_le_set_adv_param(data_type, data_len - GAP_DATA_TYPE_LEN, (void *)p_value);
            break;
        default:
            {
                APP_PRINT_ERROR1("hrp_profile_system_set_gap_param:invalid param type = %d ", param_type);
                hrp_profile_system_set_gap_param_rsp(PROFILE_SYS_CMD_INVALID_PARAM);
                return;
            }
        }

        hrp_profile_system_set_gap_param_rsp((PROFILE_SYS_CAUSE)return_cause);

    }
}
void hrp_profile_system_set_pairable_mode(uint16_t len, uint8_t *p_param_list)
{
    PROFILE_SYS_CAUSE cause = PROFILE_SYS_CMD_SUCESS;

    hrp_profile_system_cmd_result(PROFILE_SYS_CMD_SUCESS);
    if (gap_set_pairable_mode() != GAP_CAUSE_SUCCESS)
    {
        cause = PROFILE_SYS_CMD_FAIL;
    }
    hrp_profile_system_set_pairable_mode_rsp(cause);
}

void hrp_profile_system_enable_dlps(uint16_t len, uint8_t *p_param_list)
{
    PROFILE_SYS_CAUSE cause = PROFILE_SYS_CMD_FAIL;
    APP_PRINT_INFO0("hrp_profile_system_enable_dlps");
#if F_BT_DLPS_EN
    uint16_t pos = 0;
    uint8_t active;
    LE_ARRAY_TO_UINT8(active, p_param_list); pos++;

    if (hrp_dlps_active_dlps(active))
    {
        cause = PROFILE_SYS_CMD_SUCESS;
    }
    hrp_dlps_allow_enter(false);
    hrp_profile_system_enable_dlps_rsp(cause);
#else
    hrp_profile_system_enable_dlps_rsp(cause);
#endif
}

void hrp_profile_system_init_gap(uint16_t len, uint8_t *p_param_list)
{
    hrp_profile_system_cmd_result(PROFILE_SYS_CMD_SUCESS);

    APP_PRINT_INFO1("hrp_profile_system_init_gap:system_status= %d", system_status);
    /*
     if (system_status != HRP_STATUS_RESET)
     {
         APP_PRINT_ERROR0("hrp_profile_system_init_gap: wrong status");
         hrp_profile_system_init_cmp(PROFILE_SYS_CMD_INVALID_STATE);
         return;
     }
    */
    uint16_t pos = 0;
    uint8_t  legacy_profile_mask;
    uint16_t le_profile_sever_mask;
    uint16_t le_profile_client_mask;

    hrp_gap_support_mode = p_param_list[pos]; pos++;
    legacy_profile_mask = p_param_list[pos]; pos++;
    LE_ARRAY_TO_UINT16(le_profile_sever_mask, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(le_profile_client_mask, p_param_list + pos); pos += 2;

    if (hrp_gap_support_mode == 0)
    {
        APP_PRINT_ERROR0("hrp_profile_system_init_gap: at least support one of bredr/ble");
        hrp_profile_system_init_cmp(PROFILE_SYS_CMD_INVALID_PARAM);
        return;
    }

    /* gap common init */
    //gap_init_timer(P_BtHrp->p_aci_tcb->QueueHandleEvent, MAX_NUMBER_OF_GAP_TIMER);

    /* gap legacy init */
    if (hrp_gap_support_mode & HRP_GAP_LEGACY)
    {
        //hrp_gap_common_set_default_param();
        //hrp_gap_legacy_set_default_param();
        //hrp_gap_legacy_init(legacy_profile_mask);
        APP_PRINT_INFO1("legacy_profile_mask :%d", legacy_profile_mask);
    }

    /* gap le init */
    if (hrp_gap_support_mode & HRP_GAP_LE)
    {
        hrp_gap_le_init(le_profile_sever_mask, le_profile_client_mask);
    }

    /* start gap */
    system_status = HRP_STATUS_ACTIVE;
    active_module =  HRP_MODULE_PROFILE;

    gap_start_bt_stack(P_BtHrp->p_aci_tcb->QueueHandleEvent, hIoQueueHandle, MAX_NUMBER_OF_MESSAGE);

    //wait le/legacy gap init complete msg, then send init_gap_rsp
}



void (*(hrp_profile_handle_system[]))(uint16_t len, uint8_t *p_param_list) =
{

    NULL,
    hrp_profile_system_reset,           /* 0x01 */
    hrp_profile_system_init_gap,
    hrp_profile_system_set_gap_param,   /* 0x03, PROFILE_SYSTEM_SET_GAP_PARAM_REQ 0x03*/
    hrp_profile_system_set_pairable_mode,/* 0x04 */
    hrp_profile_system_enable_dlps,  //0x05
};

