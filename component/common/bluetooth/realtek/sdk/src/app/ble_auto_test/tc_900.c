
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


#include <ble_auto_test_case.h>
#include "privacy_mgnt.h"


#include <tc_common.h>
#include <tc_900.h>

#if TC_900_SUPPORT

T_PRIVACY_INIT_STATE g_privacy_init_state = PRIVACY_INIT_STATE_INIT;
T_PRIVACY_STATE PrivacyState = PRIVACY_STATE_INIT;


void privacy_state_callback(T_PRI_CB_TYPE type, uint8_t status)
{
    if (type == PRIVACY_STATE_MSGTYPE)
    {
        APP_PRINT_INFO2("privacy_state_callback: state (%d->%d)", PrivacyState, status);
        PrivacyState = (T_PRIVACY_STATE)status;
        if (PrivacyState == PRIVACY_STATE_IDLE)
        {
            if (app_get_cur_test_case() == TC_0900_PRIVACY_TEST_SLAVE)
            {
                if (g_privacy_init_state == PRIVACY_INIT_STATE_REL_LIST)
                {
                    le_privacy_set_addr_resolution(true);
                    g_privacy_init_state = PRIVACY_INIT_STATE_REL_ENABLE;
                }
            }
        }
    }
    else if (type == PRIVACY_RESOLUTION_STATUS_MSGTYPE)
    {
        if (status == LE_PRIVACY_RESOLUTION_ENABLED)
        {
            if (app_get_cur_test_case() == TC_0900_PRIVACY_TEST_SLAVE)
            {
                if (g_privacy_init_state == PRIVACY_INIT_STATE_REL_ENABLE)
                {
                    uint8_t local_bd_type = GAP_LOCAL_ADDR_LE_PUBLIC;
                    le_adv_get_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, &local_bd_type);
                    if (local_bd_type == GAP_LOCAL_ADDR_LE_RAP_OR_PUBLIC
                        || local_bd_type == GAP_LOCAL_ADDR_LE_RAP_OR_RAND)
                    {
                        uint8_t i = 0;
                        uint8_t  advDirectType = GAP_REMOTE_ADDR_LE_PUBLIC;
                        uint8_t  advDirectAddr[GAP_BD_ADDR_LEN] = {0};

                        for (i = 0; i < 4; i++)
                        {
                            if ((privacy_table[i].is_used == true) && (privacy_table[i].is_add_to_list == true))
                            {
                                advDirectType = privacy_table[i].remote_bd_type;
                                memcpy(advDirectAddr, privacy_table[i].addr, 6);
                                break;
                            }
                        }
                        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(advDirectType), &advDirectType);
                        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(advDirectAddr), advDirectAddr);
                    }
                    le_adv_start();
                    g_privacy_init_state = PRIVACY_INIT_STATE_REL_IDLE;
                }
            }
        }
    }
    else
    {
    }
}

void tc_900_privacy_slave(void)
{
    uint8_t secReqEnable = true;
    uint16_t secReqRequirement = GAP_AUTHEN_BIT_MITM_FLAG;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &secReqRequirement);
    //gaps_set_central_address_resolution(true);
    privacy_init_resolving_list(privacy_state_callback);
    if (privacy_handle_pending_resolving_list() == PRIVACY_STATE_IDLE)
    {
    }
    else
    {
        g_privacy_init_state = PRIVACY_INIT_STATE_REL_LIST;
    }
}

void tc_901_privacy_master(void)
{
    uint8_t secReqEnable = true;
    //uint16_t secReqRequirement = GAP_AUTHEN_BIT_MITM_FLAG;
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &secReqEnable);
    //le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(uint16_t), &secReqRequirement);
    //gaps_set_central_address_resolution(true);
    privacy_init_resolving_list(privacy_state_callback);
    privacy_handle_pending_resolving_list();
}

void tc_901_handle_bond_modify_msg(T_LE_BOND_MODIFY_TYPE type, T_LE_KEY_ENTRY *p_entry)
{
    APP_PRINT_INFO1("tc_901_handle_bond_modify_msg  GAP_MSG_LE_BOND_MODIFY_INFO:type=0x%x",
                    type);
    if (type == LE_BOND_CLEAR)
    {
        privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_CLEAR, GAP_IDENT_ADDR_PUBLIC,
                                           NULL);
        privacy_handle_pending_resolving_list();
    }
    else if (type == LE_BOND_ADD)
    {
        if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
        {
            privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_ADD,
                                               (T_GAP_IDENT_ADDR_TYPE)p_entry->resolved_remote_bd.remote_bd_type,
                                               p_entry->resolved_remote_bd.addr);
        }
        else if (p_entry->flags & LE_KEY_STORE_LOCAL_IRK_BIT)
        {
            privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_ADD,
                                               (T_GAP_IDENT_ADDR_TYPE)p_entry->remote_bd.remote_bd_type,
                                               p_entry->remote_bd.addr);
        }
        else
        {
        }
        privacy_handle_pending_resolving_list();
    }
    else if (type == LE_BOND_DELETE)
    {
        if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
        {
            privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_REMOVE,
                                               (T_GAP_IDENT_ADDR_TYPE)p_entry->resolved_remote_bd.remote_bd_type,
                                               p_entry->resolved_remote_bd.addr);
        }
        else if (p_entry->flags & LE_KEY_STORE_LOCAL_IRK_BIT)
        {
            privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_REMOVE,
                                               (T_GAP_IDENT_ADDR_TYPE)p_entry->remote_bd.remote_bd_type,
                                               p_entry->remote_bd.addr);
        }
        else
        {
        }
        privacy_handle_pending_resolving_list();
    }
    else
    {
    }
}

#endif

