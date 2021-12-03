/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     privacy_mgnt.c
* @brief    simple BLE profile source file.
* @details  Demonstration of how to implement a self-definition profile.
* @author
* @date     2016-02-18
* @version  v0.1
*********************************************************************************************************
*/
#include "trace_app.h"
#include <string.h>
#include "gap.h"
#include <privacy_mgnt.h>
#if F_BT_LE_PRIVACY_SUPPORT
T_LE_PRIVACY_ENTRY privacy_table[PRIVACY_ENTRY_SIZE];
bool clear_resolving_list_pending = false;
uint8_t modify_resolving_list_waiting_idx = 0xff;
bool wait_modify_resolving_list_rsp = false;
T_PRIVACY_STATE privacy_state = PRIVACY_STATE_INIT;
static P_FUN_PRIVACY_STATE_CB pfnPrivacyCB = NULL;

void privacy_init_resolving_list(P_FUN_PRIVACY_STATE_CB p_fun)
{
    APP_PRINT_INFO0("privacy_init_resolving_list");
    uint8_t i;
    T_LE_KEY_ENTRY *p_entry;
    memset(privacy_table, 0, PRIVACY_ENTRY_SIZE * sizeof(T_LE_PRIVACY_ENTRY));
    pfnPrivacyCB = p_fun;

    for (i = 0; i < bond_storage_num; i++)
    {
        p_entry = le_find_key_entry_by_idx(i);
        if (p_entry != NULL && p_entry->is_used)
        {
            if (p_entry->flags & LE_KEY_STORE_REMOTE_IRK_BIT)
            {
                APP_PRINT_INFO2("privacy_init_resolving_list: add BD=%s type=%d",
                                TRACE_BDADDR(p_entry->resolved_remote_bd.addr),
                                p_entry->resolved_remote_bd.remote_bd_type);
                privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_ADD,
                                                   (T_GAP_IDENT_ADDR_TYPE)p_entry->resolved_remote_bd.remote_bd_type,
                                                   p_entry->resolved_remote_bd.addr);
            }
        }
    }
}

T_PRIVACY_STATE privacy_handle_pending_resolving_list_int(void)
{
    APP_PRINT_INFO0("privacy_handle_pending_resolving_list");
    uint8_t i;
    T_PRIVACY_STATE state = PRIVACY_STATE_BUSY;
    if (wait_modify_resolving_list_rsp)
    {
        APP_PRINT_INFO0("wait rsp");
        return state;
    }
    if (clear_resolving_list_pending)
    {
        APP_PRINT_INFO0("clear");
        if (le_privacy_modify_resolv_list(GAP_RESOLV_LIST_OP_CLEAR, GAP_IDENT_ADDR_PUBLIC,
                                          NULL) == GAP_CAUSE_SUCCESS)
        {
            wait_modify_resolving_list_rsp = true;
            return state;
        }
        else
        {
            return state;
        }
    }
    for (i = 0; i < PRIVACY_ENTRY_SIZE; i++)
    {
        if (privacy_table[i].is_used && privacy_table[i].pending != PRIVACY_RESOLVING_LIST_IDLE)
        {
            T_GAP_CAUSE status;
            if (privacy_table[i].pending == PRIVACY_RESOLVING_LIST_ADD_PENDING)
            {

                APP_PRINT_INFO3("add i=%d BD=%s type=%d\n", i,
                                TRACE_BDADDR(privacy_table[i].addr),
                                privacy_table[i].remote_bd_type);
                status = le_privacy_modify_resolv_list(GAP_RESOLV_LIST_OP_ADD,
                                                       privacy_table[i].remote_bd_type,
                                                       privacy_table[i].addr);
                if (status == GAP_CAUSE_SUCCESS)
                {
                    wait_modify_resolving_list_rsp = true;
                    modify_resolving_list_waiting_idx = i;
                    return state;
                }
                else if (status != GAP_CAUSE_INVALID_STATE)
                {
                    APP_PRINT_INFO1("remove i=%d from pend add\n", i);
                    memset(&privacy_table[i], 0, sizeof(T_LE_PRIVACY_ENTRY));
                }
                else
                {
                    return state;
                }
            }
            else
            {
                APP_PRINT_INFO3("remove i=%d BD=%s type=%d", i,
                                TRACE_BDADDR(privacy_table[i].addr),
                                privacy_table[i].remote_bd_type);
                status = le_privacy_modify_resolv_list(GAP_RESOLV_LIST_OP_REMOVE,
                                                       privacy_table[i].remote_bd_type,
                                                       privacy_table[i].addr);
                if (status == GAP_CAUSE_SUCCESS)
                {
                    wait_modify_resolving_list_rsp = true;
                    modify_resolving_list_waiting_idx = i;
                    return state;
                }
                else if (status != GAP_CAUSE_INVALID_STATE)
                {
                    APP_PRINT_INFO1("remove i=%d from pend remove\n", i);
                    memset(&privacy_table[i], 0, sizeof(T_LE_PRIVACY_ENTRY));
                }
                else
                {
                    return state;
                }
            }
        }
    }
    APP_PRINT_INFO0("privacy_handle_pending_resolving_list: idle");
    state = PRIVACY_STATE_IDLE;
    return state;
}

T_PRIVACY_STATE privacy_handle_pending_resolving_list(void)
{
    T_PRIVACY_STATE state = privacy_handle_pending_resolving_list_int();
    if (privacy_state != state)
    {
        privacy_state = state;
        if (pfnPrivacyCB)
        {
            pfnPrivacyCB(PRIVACY_STATE_MSGTYPE, privacy_state);
        }
    }
    return state;
}
void privacy_add_pending_resolving_list(T_GAP_RESOLV_LIST_OP op,
                                        T_GAP_IDENT_ADDR_TYPE addr_type, uint8_t *addr)
{
    APP_PRINT_INFO1("privacy_add_pending_resolving_list op = %d", op);
    switch (op)
    {
    case GAP_RESOLV_LIST_OP_CLEAR:
        clear_resolving_list_pending = true;
        break;

    case GAP_RESOLV_LIST_OP_ADD:
        //if((addr_type == BTIF_RemoteBDTypeLEPublic)
        //||(addr_type == BTIF_RemoteBDTypeLEPublicIdentityAddress)
        //||(addr_type == BTIF_RemoteBDTypeLERandomIdentityAddress)
        //||((addr_type == BTIF_RemoteBDTypeLERandom) && (addr[5]&RANDOM_ADDR_MASK == RANDOM_ADDR_MASK_STATIC)))
        {
            uint8_t i;
            for (i = 0; i < PRIVACY_ENTRY_SIZE; i++)
            {
                if (privacy_table[i].is_used)
                {
                    if ((privacy_table[i].remote_bd_type == addr_type)
                        && (memcmp(privacy_table[i].addr, addr, 6) == 0))
                    {
                        if (!privacy_table[i].is_add_to_list)
                        {
                            privacy_table[i].pending = PRIVACY_RESOLVING_LIST_ADD_PENDING;
                        }
                        break;
                    }
                }
            }
            if (i != PRIVACY_ENTRY_SIZE)
            {
                break;
            }
            for (i = 0; i < PRIVACY_ENTRY_SIZE; i++)
            {
                if (!privacy_table[i].is_used)
                {
                    privacy_table[i].is_used = true;
                    privacy_table[i].remote_bd_type = addr_type;
                    memcpy(privacy_table[i].addr, addr, 6);
                    privacy_table[i].pending = PRIVACY_RESOLVING_LIST_ADD_PENDING;
                    break;
                }
            }
        }
        break;

    case GAP_RESOLV_LIST_OP_REMOVE:
        //if((addr_type == BTIF_RemoteBDTypeLEPublicIdentityAddress)
        //||(addr_type == BTIF_RemoteBDTypeLERandomIdentityAddress))
        {
            uint8_t i;
            for (i = 0; i < PRIVACY_ENTRY_SIZE; i++)
            {
                if (privacy_table[i].is_used)
                {
                    if ((privacy_table[i].remote_bd_type == addr_type)
                        && (memcmp(privacy_table[i].addr, addr, 6) == 0))
                    {
                        if (privacy_table[i].is_add_to_list)
                        {
                            privacy_table[i].pending = PRIVACY_RESOLVING_LIST_REMOVE_PENDING;
                        }
                        else
                        {
                            memset(&privacy_table[i], 0, sizeof(T_LE_PRIVACY_ENTRY));
                        }
                        break;
                    }
                }
            }
        }
        break;
    default:
        break;
    }
}

void privacy_handle_LE_ADDRESS_RESOLUTION_STATUS_INFO_MSGTYPE(T_LE_PRIVACY_RESOLUTION_STATUS_INFO
                                                              res_status)
{
    APP_PRINT_INFO1("GAP_MSG_LE_PRIVACY_RESOLUTION_STATUS_INFO: status = 0x%x\n", res_status.status);
    if (pfnPrivacyCB)
    {
        pfnPrivacyCB(PRIVACY_RESOLUTION_STATUS_MSGTYPE, res_status.status);
    }
}

void privacy_handle_LE_MODIFY_RESOLVING_LIST_MSGTYPE(T_LE_PRIVACY_MODIFY_RESOLV_LIST_RSP
                                                     *pListRsp)
{
    APP_PRINT_INFO2("GAP_MSG_LE_PRIVACY_MODIFY_RESOLV_LIST: operation = 0x%x casue = 0x%x",
                    pListRsp->operation, pListRsp->cause);
    wait_modify_resolving_list_rsp = false;
    if (pListRsp->cause != GAP_SUCCESS)
    {
        if (pListRsp->cause == (HCI_ERR | HCI_ERR_UNKNOWN_CONN_ID)
            || pListRsp->cause == (HCI_ERR | HCI_ERR_MEMORY_FULL)
            || pListRsp->cause == (HCI_ERR | HCI_ERR_INVALID_PARAM))
        {
            memset(&privacy_table[modify_resolving_list_waiting_idx], 0, sizeof(T_LE_PRIVACY_ENTRY));
        }
        modify_resolving_list_waiting_idx = 0xff;
    }
    else
    {
        if (pListRsp->operation == GAP_RESOLV_LIST_OP_CLEAR)
        {
            clear_resolving_list_pending = false;
            memset(privacy_table, 0, PRIVACY_ENTRY_SIZE * sizeof(T_LE_PRIVACY_ENTRY));
        }
        else if (pListRsp->operation == GAP_RESOLV_LIST_OP_ADD)
        {
            if (modify_resolving_list_waiting_idx < PRIVACY_ENTRY_SIZE)
            {
                privacy_table[modify_resolving_list_waiting_idx].is_add_to_list = true;
                privacy_table[modify_resolving_list_waiting_idx].pending = PRIVACY_RESOLVING_LIST_IDLE;
                modify_resolving_list_waiting_idx = 0xff;
            }
        }
        else
        {
            if (modify_resolving_list_waiting_idx < PRIVACY_ENTRY_SIZE)
            {
                memset(&privacy_table[modify_resolving_list_waiting_idx], 0, sizeof(T_LE_PRIVACY_ENTRY));
                modify_resolving_list_waiting_idx = 0xff;
            }
        }
        privacy_handle_pending_resolving_list();
    }
}

void privacy_handle_LE_READ_PEER_RESOLVABLE_ADDRESS_MSGTYPE(
    T_LE_PRIVACY_READ_PEER_RESOLV_ADDR_RSP *pLEReadPeerResolvableAddressRsp)
{
    APP_PRINT_INFO2("GAP_MSG_LE_PRIVACY_READ_PEER_RESOLV_ADDR: cause = 0x%x BD=%s",
                    pLEReadPeerResolvableAddressRsp->cause,
                    TRACE_BDADDR(pLEReadPeerResolvableAddressRsp->peer_resolv_addr));
}

extern bool le_check_local_resolved_address(uint8_t *unresolved_addr, uint8_t *local_irk);
//extern T_LOCAL_IRK gap_local_irk;
void privacy_handle_LE_READ_LOCAL_RESOLVABLE_ADDRESS_MSGTYPE(
    T_LE_PRIVACY_READ_LOCAL_RESOLV_ADDR_RSP *
    pLEReadLocalResolvableAddressRsp)
{
    APP_PRINT_INFO2("GAP_MSG_LE_PRIVACY_READ_LOCAL_RESOLV_ADDR: cause = 0x%x BD=%s",
                    pLEReadLocalResolvableAddressRsp->cause,
                    TRACE_BDADDR(pLEReadLocalResolvableAddressRsp->local_resolv_addr));
    if (pLEReadLocalResolvableAddressRsp->cause == 0)
    {
        //bool is_local = le_check_local_resolved_address(pLEReadLocalResolvableAddressRsp->local_resolv_addr,
        //gap_local_irk.local_irk);
        //APP_PRINT_INFO1("GAP_MSG_LE_PRIVACY_READ_LOCAL_RESOLV_ADDR: is_local=%d",
        //is_local);
    }
}

void privacy_handle_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT_MSGTYPE(
    T_LE_PRIVACY_SET_RESOLV_PRIV_ADDR_TIMEOUT_RSP   *pTimeoutRsp)
{
    APP_PRINT_INFO1("GAP_MSG_LE_PRIVACY_SET_RESOLV_PRIV_ADDR_TIMEOUT: cause = 0x%x",
                    pTimeoutRsp->cause);
}

#if 0
void privacy_handle_LE_BOND_MODIFY_MSGTYPE(P_LE_BOND_MODIFY_INFO  pDeleteInfo)
{
    APP_PRINT_INFO2("LE_BOND_MODIFY_MSGTYPE: type = 0x%x bd_addr = %s",
                    pDeleteInfo->remote_bd_type, TRACE_BDADDR(pDeleteInfo->remote_bd));
    privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_REMOVE, pDeleteInfo->remote_bd_type,
                                       pDeleteInfo->remote_bd);
    privacy_handle_pending_resolving_list();
}
#endif

#if 0
void privacy_handle_LE_REMOTE_IRK_RECEIVE_MSGTYPE(P_LE_REMOTE_IRK_RECEIVE p_irk)
{
    APP_PRINT_INFO2("LE_REMOTE_IRK_RECEIVE_MSGTYPE: type = 0x%x bd_addr = %s",
                    p_irk->remote_bd_type, TRACE_BDADDR(p_irk->remote_bd));
    privacy_add_pending_resolving_list(GAP_RESOLV_LIST_OP_ADD, p_irk->remote_bd_type,
                                       p_irk->remote_bd);
    privacy_handle_pending_resolving_list();
}
#endif
void privacy_handle_LE_SET_PRIVACY_MODE_MSGTYPE(T_LE_PRIVACY_SET_MODE_RSP *p_privacy_mode)
{
    APP_PRINT_INFO1("GAP_MSG_LE_PRIVACY_SET_MODE: cause = %d",
                    p_privacy_mode->cause);
}

T_APP_RESULT privacy_msg_callback(uint8_t msg_type, T_LE_PRIVACY_CB_DATA msg_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO1("privacy_msg_callback: msg_type = %d", msg_type);
    switch (msg_type)
    {
    case GAP_MSG_LE_PRIVACY_RESOLUTION_STATUS_INFO:
        privacy_handle_LE_ADDRESS_RESOLUTION_STATUS_INFO_MSGTYPE(
            msg_data.le_privacy_resolution_status_info);
        break;
    case GAP_MSG_LE_PRIVACY_SET_RESOLV_PRIV_ADDR_TIMEOUT:
        privacy_handle_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT_MSGTYPE(
            msg_data.p_le_privacy_set_resolv_priv_addr_timeout_rsp);
        break;

    case GAP_MSG_LE_PRIVACY_MODIFY_RESOLV_LIST:
        privacy_handle_LE_MODIFY_RESOLVING_LIST_MSGTYPE(msg_data.p_le_privacy_modify_resolv_list_rsp);
        break;
    case GAP_MSG_LE_PRIVACY_READ_PEER_RESOLV_ADDR:
        privacy_handle_LE_READ_PEER_RESOLVABLE_ADDRESS_MSGTYPE(
            msg_data.p_le_privacy_read_peer_resolv_addr_rsp);
        break;
    case GAP_MSG_LE_PRIVACY_READ_LOCAL_RESOLV_ADDR:
        privacy_handle_LE_READ_LOCAL_RESOLVABLE_ADDRESS_MSGTYPE(
            msg_data.p_le_privacy_read_local_resolv_addr_rsp);
        break;
    //case LE_BOND_DELETE_MSGTYPE:
    //privacy_handle_LE_BOND_DELETE_MSGTYPE(msg_data.p_le_bond_delete_info);
    //break;

    //case LE_REMOTE_IRK_RECEIVE_MSGTYPE:
    //privacy_handle_LE_REMOTE_IRK_RECEIVE_MSGTYPE(msg_data.p_remote_irk_rec);
    //break;
    case GAP_MSG_LE_PRIVACY_SET_MODE:
        privacy_handle_LE_SET_PRIVACY_MODE_MSGTYPE(msg_data.p_le_privacy_set_mode_rsp);
        break;
    default:
        break;
    }
    return result;
}
#endif

