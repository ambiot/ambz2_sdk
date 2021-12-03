/**
*********************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      hrp_profile_simple_ble_service.c
* @brief
* @details   none.
* @author
* @date      2018-1-26
* @version   v0.1
* *********************************************************************************************************
*/
#include "trace_app.h"
#include <hrp_profile_entry.h>
#include <os_mem.h>
#include <complete_ble_service.h>
#include <hrp_profile_le_simple_ble.h>
#if F_BT_LE_GATT_CLIENT_SUPPORT
#include <complete_ble_client.h>
#include "gaps_client.h"
#endif

extern T_SERVER_ID   simp_srv_id;           /**< Simple ble service id*/
extern T_APP_RESULT hrp_profile_callback(T_SERVER_ID service_id, void *p_data);
//==============================SimpBle Event======================//
static void hrp_profile_simp_ble_evet(uint16_t cmd_index, uint16_t param_list_len,
                                      uint8_t *p_param_list)
{
    uint8_t cmd_group = HRP_PROFILE_CMD_GROUP_EVENT_SIMP_BLE;
    hrp_profile_evet(cmd_group, cmd_index,  param_list_len, p_param_list);
}


static void hrp_profile_simp_ble_cmd_result(bool result)
{
    uint8_t Cause;
    if (result == true)
    {
        Cause = 0;
    }
    else
    {
        Cause = 1;
    }
    hrp_profile_simp_ble_evet(HRP_SIMP_BLE_CMD_RESULT, sizeof(uint8_t), (uint8_t *)&Cause);
}


//==============================Gaps Client  Event======================//
#if F_BT_LE_GATT_CLIENT_SUPPORT
static void hrp_profile_gaps_cleint_evet(uint16_t cmd_index, uint16_t param_list_len,
                                         uint8_t *p_param_list)
{
    uint8_t cmd_group = HRP_PROFILE_CMD_GROUP_EVENT_GAPS_CLIENT;
    hrp_profile_evet(cmd_group, cmd_index,  param_list_len, p_param_list);
}


static void hrp_profile_gaps_client_cmd_result(bool result)
{
    uint8_t Cause;
    if (result == true)
    {
        Cause = 0;
    }
    else
    {
        Cause = 1;
    }
    hrp_profile_gaps_cleint_evet(HRP_GAPS_CLIENT_CMD_RESULT, sizeof(uint8_t), (uint8_t *)&Cause);
}
#endif


/*********************************handle simp ble server event function**********************************/
void hrp_profile_handle_service_callback_type_indication_notification(uint8_t conn_id,
                                                                      T_SERVICE_CALLBACK_TYPE msg_type, uint8_t notification_indification_index)
{

    uint8_t Param[3];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, conn_id); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, msg_type); pos += 1;

    LE_UINT8_TO_ARRAY(Param + pos, notification_indification_index); pos += 1;

    hrp_profile_simp_ble_evet(HRP_SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION, pos, Param);
}
void hrp_profile_handle_service_callback_type_read_char_value(uint8_t conn_id,
                                                              T_SERVICE_CALLBACK_TYPE msg_type, uint8_t read_value_index)
{

    uint8_t Param[3];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, conn_id); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, msg_type); pos += 1;

    LE_UINT8_TO_ARRAY(Param + pos, read_value_index); pos += 1;

    hrp_profile_simp_ble_evet(HRP_SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE, pos, Param);
}
void  hrp_profile_handle_service_callback_type_write_char_value(uint8_t conn_id,
                                                                T_SERVICE_CALLBACK_TYPE msg_type,
                                                                uint8_t opcode,
                                                                T_WRITE_TYPE write_type,
                                                                uint16_t len)
{
    uint8_t Param[6];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, conn_id); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, msg_type); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, opcode); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, write_type); pos += 1;
    LE_UINT16_TO_ARRAY(Param + pos, len); pos += 2;

    hrp_profile_simp_ble_evet(HRP_SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE, pos, Param);
}

// ***************************************************************************************************
void    hrp_profile_server_simp_ble_callback(TSIMP_CALLBACK_DATA *p_simp_cb_data)
{
    switch (p_simp_cb_data->msg_type)
    {
    case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
        {
            switch (p_simp_cb_data->msg_data.notification_indification_index)
            {
            case SIMP_NOTIFY_INDICATE_V3_ENABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_ENABLE");
                }
                break;

            case SIMP_NOTIFY_INDICATE_V3_DISABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_DISABLE");
                }
                break;
            case SIMP_NOTIFY_INDICATE_V4_ENABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_ENABLE");
                }
                break;
            case SIMP_NOTIFY_INDICATE_V4_DISABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_DISABLE");
                }
                break;
            case SIMP_NOTIFY_INDICATE_V8_NOTIFY_ENABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_NOTIFY_ENABLE");
                }
                break;
            case SIMP_NOTIFY_INDICATE_V8_INDICATE_ENABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_INDICATE_ENABLE");
                }
                break;
            case SIMP_NOTIFY_INDICATE_V8_NOTIFY_INDICATE_ENABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_NOTIFY_INDICATE_ENABLE");
                }
                break;
            case SIMP_NOTIFY_INDICATE_V8_DISABLE:
                {
                    APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V8_DISABLE");
                }
                break;
            }
            hrp_profile_handle_service_callback_type_indication_notification(p_simp_cb_data->conn_id,
                                                                             p_simp_cb_data->msg_type, p_simp_cb_data->msg_data.notification_indification_index);

        }
        break;

    case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
        {
            if (p_simp_cb_data->msg_data.read_value_index == SIMP_READ_V1)
            {
                uint8_t value = 0x88;
                APP_PRINT_INFO1("SIMP_READ_V1: 0x%x", value);
                //simp_ble_service_set_parameter(SIMPLE_BLE_SERVICE_PARAM_V1_READ_CHAR_VAL, 1, &value);
            }
            hrp_profile_handle_service_callback_type_read_char_value(p_simp_cb_data->conn_id,
                                                                     p_simp_cb_data->msg_type, p_simp_cb_data->msg_data.read_value_index);

        }
        break;
    case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
        {
            switch (p_simp_cb_data->msg_data.write.opcode)
            {
            case SIMP_WRITE_V2:
                {
                    APP_PRINT_INFO2("SIMP_WRITE_V2: write type %d, len %d", p_simp_cb_data->msg_data.write.write_type,
                                    p_simp_cb_data->msg_data.write.len);
                }
                break;

            case SIMP_WRITE_V6:
                {
                    APP_PRINT_INFO1("SIMP_WRITE_V6: len = 0x%x", p_simp_cb_data->msg_data.write.len);
                }
                break;

            default:
                break;
            }
            hrp_profile_handle_service_callback_type_write_char_value(p_simp_cb_data->conn_id,
                                                                      p_simp_cb_data->msg_type,
                                                                      p_simp_cb_data->msg_data.write.opcode,
                                                                      p_simp_cb_data->msg_data.write.write_type,
                                                                      p_simp_cb_data->msg_data.write.len);
        }
        break;

    default:
        break;
    }

}
/*********************************handle simp ble client event function**********************************/


void hrp_profile_handle_simp_ble_common_event(uint16_t cmd_index, uint16_t cause)
{
    uint8_t Param[2];
    int pos = 0;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;

    hrp_profile_simp_ble_evet(cmd_index, pos, Param);

}

void hrp_profile_handle_simp_ble_notif_ind_common_event(uint16_t cmd_index, uint16_t value_size,
                                                        uint8_t *p_value)
{
    uint8_t Param[500] = {0};
    int pos = 0;
    LE_UINT16_TO_ARRAY(Param + pos, value_size); pos += 2;
    hrp_profile_commit_buf(Param, &pos, p_value, value_size);

    hrp_profile_simp_ble_evet(cmd_index, pos, Param);

}
#if F_BT_LE_GATT_CLIENT_SUPPORT
void hrp_profile_handle_simp_client_cb_type_disc_state(T_SIMP_DISC_STATE  disc_state)
{

    uint8_t Param[1];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, disc_state); pos += 1;

    hrp_profile_simp_ble_evet(HRP_SIMP_CLIENT_CB_TYPE_DISC_STATE, pos, Param);
}

void hrp_profile_handle_simp_read_v1_read(uint16_t cause, uint16_t value_size, uint8_t *p_value)
{

    uint8_t Param[100] = {0};
    int pos = 0;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;
    LE_UINT16_TO_ARRAY(Param + pos, value_size); pos += 2;
    hrp_profile_commit_buf(Param, &pos, p_value, value_size);

    hrp_profile_simp_ble_evet(HRP_SIMP_READ_V1_READ, pos, Param);
}
void    hrp_profile_handle_simp_read_v3_notify_cccd(bool v3_notify_cccd, uint16_t cause)
{
    uint8_t Param[3];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, v3_notify_cccd); pos += 1;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;

    hrp_profile_simp_ble_evet(HRP_SIMP_READ_V3_NOTIFY_CCCD, pos, Param);
}

void    hrp_profile_handle_simp_read_v4_indicate_cccd(bool v4_indicate_cccd, uint16_t cause)
{
    uint8_t Param[3];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, v4_indicate_cccd); pos += 1;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;

    hrp_profile_simp_ble_evet(HRP_SIMP_READ_V4_INDICATE_CCCD, pos, Param);
}

void    hrp_profile_handle_simp_read_v8_cccd(uint16_t v8_notify_ind_cccd, uint16_t cause)
{
    uint8_t Param[4];
    int pos = 0;
    LE_UINT16_TO_ARRAY(Param + pos, v8_notify_ind_cccd); pos += 2;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;

    hrp_profile_simp_ble_evet(HRP_SIMP_READ_V8_CCCD, pos, Param);
}

void hrp_profile_handle_simp_read_v7_read_long(uint16_t value_size, uint8_t *p_value,
                                               uint16_t cause)
{

    uint8_t Param[500] = {0};
    int pos = 0;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;
    LE_UINT16_TO_ARRAY(Param + pos, value_size); pos += 2;
    hrp_profile_commit_buf(Param, &pos, p_value, value_size);

    hrp_profile_simp_ble_evet(HRP_SIMP_READ_V7_READ_LONG, pos, Param);
}


void hrp_profile_simp_ble_get_hdl_cache_rsp(uint8_t srv_start, uint8_t cache_len,
                                            uint16_t *hdl_cache)
{
    uint8_t Param[2 + sizeof(uint16_t) * HDL_SIMBLE_CACHE_LEN];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, srv_start); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, cache_len); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, sizeof(uint16_t) * HDL_SIMBLE_CACHE_LEN); pos += 1;

    hrp_profile_commit_buf(Param, &pos, hdl_cache, sizeof(uint16_t) * HDL_SIMBLE_CACHE_LEN);

    hrp_profile_simp_ble_evet(HRP_SIMP_GET_HDL_CACHE_RSP, pos, Param);

}



void    hrp_profile_client_simp_ble_callback(T_SIMP_CLIENT_CB_DATA *p_simp_client_cb_data)
{
    uint16_t value_size;
    uint8_t *p_value = NULL;
    switch (p_simp_client_cb_data->cb_type)
    {
    case SIMP_CLIENT_CB_TYPE_DISC_STATE:
        {

            switch (p_simp_client_cb_data->cb_content.disc_state)
            {
            case DISC_SIMP_DONE:
                APP_PRINT_INFO0("app_client_callback: discover simp procedure done.");
                break;
            case DISC_SIMP_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("app_client_callback: discover simp request failed.");
                break;
            default:
                break;
            }
            hrp_profile_handle_simp_client_cb_type_disc_state(p_simp_client_cb_data->cb_content.disc_state);

        }
        break;
    case SIMP_CLIENT_CB_TYPE_READ_RESULT:
        switch (p_simp_client_cb_data->cb_content.read_result.type)
        {
        case SIMP_READ_V1_READ:
            {

                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    value_size = p_simp_client_cb_data->cb_content.read_result.data.v1_read.value_size;

                    p_value = p_simp_client_cb_data->cb_content.read_result.data.v1_read.p_value;
                    APP_PRINT_INFO2("SIMP_READ_V1_READ: value_size %d, value %b",
                                    value_size, TRACE_BINARY(value_size, p_value));
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V1_READ: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }

                hrp_profile_handle_simp_read_v1_read(p_simp_client_cb_data->cb_content.read_result.cause,
                                                     value_size, p_value);
            }

            break;
        case SIMP_READ_V3_NOTIFY_CCCD:
            {

                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("SIMP_READ_V3_NOTIFY_CCCD: notify %d",
                                    p_simp_client_cb_data->cb_content.read_result.data.v3_notify_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V3_NOTIFY_CCCD: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }

                hrp_profile_handle_simp_read_v3_notify_cccd(
                    p_simp_client_cb_data->cb_content.read_result.data.v3_notify_cccd,
                    p_simp_client_cb_data->cb_content.read_result.cause);
            }

            break;
        case SIMP_READ_V4_INDICATE_CCCD:
            {
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("SIMP_READ_V4_INDICATE_CCCD: indicate %d",
                                    p_simp_client_cb_data->cb_content.read_result.data.v4_indicate_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V4_INDICATE_CCCD: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }
                hrp_profile_handle_simp_read_v4_indicate_cccd(
                    p_simp_client_cb_data->cb_content.read_result.data.v4_indicate_cccd,
                    p_simp_client_cb_data->cb_content.read_result.cause);
            }

            break;

        case SIMP_READ_V8_CCCD:
            {
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("SIMP_READ_V8_CCCD: cccd 0x%x",
                                    p_simp_client_cb_data->cb_content.read_result.data.v8_notify_ind_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V8_CCCD: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }
                hrp_profile_handle_simp_read_v8_cccd(
                    p_simp_client_cb_data->cb_content.read_result.data.v8_notify_ind_cccd,
                    p_simp_client_cb_data->cb_content.read_result.cause);
            }

            break;

        case SIMP_READ_V7_READ_LONG:
            {
                if (p_simp_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    value_size = p_simp_client_cb_data->cb_content.read_result.data.v7_read.value_size;
                    p_value = p_simp_client_cb_data->cb_content.read_result.data.v7_read.p_value;
                    APP_PRINT_INFO2("SIMP_READ_V7_READ_LONG: value_size %d, value %b",
                                    value_size, TRACE_BINARY(value_size, p_value));
                }
                else
                {
                    APP_PRINT_ERROR1("SIMP_READ_V7_READ_LONG: failed cause 0x%x",
                                     p_simp_client_cb_data->cb_content.read_result.cause);
                }

                hrp_profile_handle_simp_read_v7_read_long(value_size, p_value,
                                                          p_simp_client_cb_data->cb_content.read_result.cause);
            }

            break;

        default:
            break;
        }
        break;

    case SIMP_CLIENT_CB_TYPE_WRITE_RESULT:
        switch (p_simp_client_cb_data->cb_content.write_result.type)
        {
        case SIMP_WRITE_V2_WRITE:
            {
                APP_PRINT_INFO1("SIMP_WRITE_V2_WRITE: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                hrp_profile_handle_simp_ble_common_event(HRP_SIMP_WRITE_V2_WRITE,
                                                         p_simp_client_cb_data->cb_content.write_result.cause);

            }

            break;
        case SIMP_WRITE_V3_NOTIFY_CCCD:
            {
                APP_PRINT_INFO1("SIMP_WRITE_V3_NOTIFY_CCCD: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                hrp_profile_handle_simp_ble_common_event(HRP_SIMP_WRITE_V3_NOTIFY_CCCD,
                                                         p_simp_client_cb_data->cb_content.write_result.cause);
            }

            break;
        case SIMP_WRITE_V4_INDICATE_CCCD:
            {
                APP_PRINT_INFO1("SIMP_WRITE_V4_INDICATE_CCCD: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                hrp_profile_handle_simp_ble_common_event(HRP_SIMP_WRITE_V4_INDICATE_CCCD,
                                                         p_simp_client_cb_data->cb_content.write_result.cause);
            }

            break;
        case SIMP_WRITE_V6_WRITE_LONG:
            {
                APP_PRINT_INFO1("SIMP_WRITE_V6_WRITE_LONG: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);
                hrp_profile_handle_simp_ble_common_event(HRP_SIMP_WRITE_V6_WRITE_LONG,
                                                         p_simp_client_cb_data->cb_content.write_result.cause);

            }

            break;
        case SIMP_WRITE_V8_CCCD:
            {
                APP_PRINT_INFO1("SIMP_WRITE_V8_CCCD: write result 0x%x",
                                p_simp_client_cb_data->cb_content.write_result.cause);

                hrp_profile_handle_simp_ble_common_event(HRP_SIMP_WRITE_V8_CCCD,
                                                         p_simp_client_cb_data->cb_content.write_result.cause);

            }

            break;
        default:
            break;
        }
        break;

    case SIMP_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
        value_size = p_simp_client_cb_data->cb_content.notif_ind_data.data.value_size;
        p_value = p_simp_client_cb_data->cb_content.notif_ind_data.data.p_value;

        switch (p_simp_client_cb_data->cb_content.notif_ind_data.type)
        {
        case SIMP_V3_NOTIFY:
            {
                APP_PRINT_INFO2("SIMP_V3_NOTIFY: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                /* if (gap_v3_notif_test.v3_tx_conn_id == conn_id)
                 {
                     gap_v3_notif_test.v3_rx_num++;
                 }
                 */
                hrp_profile_handle_simp_ble_notif_ind_common_event(HRP_SIMP_V3_NOTIFY, value_size, p_value);
            }

            break;
        case SIMP_V4_INDICATE:
            {
                APP_PRINT_INFO2("SIMP_V4_INDICATE: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                hrp_profile_handle_simp_ble_notif_ind_common_event(HRP_SIMP_V4_INDICATE, value_size, p_value);
            }

            break;
        case SIMP_V8_INDICATE:
            {
                APP_PRINT_INFO2("SIMP_V8_INDICATE: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                hrp_profile_handle_simp_ble_notif_ind_common_event(HRP_SIMP_V8_INDICATE, value_size, p_value);
            }

            break;
        case SIMP_V8_NOTIFY:
            {
                /*
                     if (gap_v3_notif_test.v3_tx_conn_id == conn_id)
                     {
                         gap_v3_notif_test.v8_rx_num++;
                     }
                     */
                APP_PRINT_INFO2("SIMP_V8_NOTIFY: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                hrp_profile_handle_simp_ble_notif_ind_common_event(HRP_SIMP_V8_NOTIFY, value_size, p_value);
            }

            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

}

///=============================Simp Ble Client  cmd======================

static void  hrp_simp_ble_client_start_all_discovery(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    bool result;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;

    if ((client_all_primary_srv_discovery(conn_id, CLIENT_PROFILE_GENERAL_ID)) == GAP_CAUSE_SUCCESS)
    {
        result = true;
    }
    else
    {
        result = false;
    }

    APP_PRINT_INFO1("hrp_simp_ble_client_start_all_discovery:  result 0x%x", result);
    hrp_profile_simp_ble_cmd_result(result);
}
static void  hrp_simp_ble_client_start_discovery(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t conn_id;
    bool result;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;

    result = simp_ble_client_start_discovery(conn_id);

    APP_PRINT_INFO1("hrp_simp_ble_client_start_discovery:  result 0x%x", result);
    hrp_profile_simp_ble_cmd_result(result);
}
static void hrp_simp_ble_client_read_by_handle(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint8_t read_type;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(read_type, p_param_list + pos); pos += 1;

    result = simp_ble_client_read_by_handle(conn_id, (T_SIMP_READ_TYPE)read_type);

    hrp_profile_simp_ble_cmd_result(result);
}

static void hrp_simp_ble_client_read_by_uuid(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint8_t read_type;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(read_type, p_param_list + pos); pos += 1;

    result = simp_ble_client_read_by_uuid(conn_id, (T_SIMP_READ_TYPE) read_type);

    hrp_profile_simp_ble_cmd_result(result);


}
static void hrp_simp_ble_client_write_v2_char(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint8_t type;
    uint16_t length;
    uint8_t *p_value;


    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(type, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(length, p_param_list + pos); pos += 2;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    result = simp_ble_client_write_v2_char(conn_id, length, p_value, (T_GATT_WRITE_TYPE)type);

    hrp_profile_simp_ble_cmd_result(result);
    os_mem_free(p_value);
}

static void hrp_simp_ble_client_write_v6_char(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint16_t length;
    uint8_t *p_value;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(length, p_param_list + pos); pos += 2;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    result = simp_ble_client_write_v6_char(conn_id, length, p_value);

    hrp_profile_simp_ble_cmd_result(result);
    os_mem_free(p_value);

}

static void hrp_simp_ble_client_set_v3_notify(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    bool notify;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(notify, p_param_list + pos); pos += 1;

    result = simp_ble_client_set_v3_notify(conn_id, notify);

    hrp_profile_simp_ble_cmd_result(result);
}

static void hrp_simp_ble_client_set_v4_ind(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    bool ind;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(ind, p_param_list + pos); pos += 1;

    result = simp_ble_client_set_v4_ind(conn_id, ind);

    hrp_profile_simp_ble_cmd_result(result);

}
static void  hrp_simp_ble_client_set_v8_cccd(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint16_t cccd_bits;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(cccd_bits, p_param_list + pos); pos += 2;

    result = simp_ble_client_set_v8_cccd(conn_id, cccd_bits);

    hrp_profile_simp_ble_cmd_result(result);
}
static void  hrp_simp_ble_client_get_hdl_cache(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint8_t hdl_idx;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;

    uint16_t hdl_cache[HDL_SIMBLE_CACHE_LEN] = {0};

    result = simp_ble_client_get_hdl_cache(conn_id, hdl_cache, sizeof(uint16_t) * HDL_SIMBLE_CACHE_LEN);
    hrp_profile_simp_ble_cmd_result(result);
    if (result)
    {
        for (hdl_idx = HDL_SIMBLE_SRV_START; hdl_idx < HDL_SIMBLE_CACHE_LEN; hdl_idx++)
        {
            APP_PRINT_INFO2("-->Index %d -- Handle 0x%x\r\n", hdl_idx, hdl_cache[hdl_idx]);
        }
        hrp_profile_simp_ble_get_hdl_cache_rsp(HDL_SIMBLE_SRV_START, HDL_SIMBLE_CACHE_LEN, hdl_cache);
    }
}
static void  hrp_simp_ble_client_set_hdl_cache(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint8_t length;
    uint16_t *p_hdl_cache;
    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(length, p_param_list + pos); pos += 1;

    p_hdl_cache = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_hdl_cache, &pos, p_param_list, length);

    result = simp_ble_client_set_hdl_cache(conn_id, p_hdl_cache, length);
    hrp_profile_simp_ble_cmd_result(result);
    os_mem_free(p_hdl_cache);
}
#endif
///=============================Simp Ble Server  cmd======================
static void  hrp_simp_ble_service_set_parameter(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t param_type;
    uint16_t length;
    uint8_t *p_value;
    LE_ARRAY_TO_UINT8(param_type, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(length, p_param_list + pos); pos += 2;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    result = simp_ble_service_set_parameter((T_SIMP_PARAM_TYPE)param_type, length, p_value);
    hrp_profile_simp_ble_cmd_result(result);
    os_mem_free(p_value);
}


static void  hrp_simp_ble_service_send_v3_notify(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    //T_SERVER_ID service_id;
    uint8_t *p_value;
    uint16_t length;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    //LE_ARRAY_TO_UINT8(service_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(length, p_param_list + pos); pos += 2;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    result = simp_ble_service_send_v3_notify(conn_id,  simp_srv_id, p_value, length);
    hrp_profile_simp_ble_cmd_result(result);
    APP_PRINT_INFO5("hrp_simp_ble_service_send_v3_notify: result=%d ,conn_id= %d,  simp_srv_id= %d, length =%d, pValue= %b",
                    result, conn_id, simp_srv_id, length, TRACE_BINARY(length, p_value));
    os_mem_free(p_value);
}

static void  hrp_simp_ble_service_send_v4_indicate(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    //T_SERVER_ID service_id;
    uint8_t *p_value;
    uint16_t length;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    // LE_ARRAY_TO_UINT8(service_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(length, p_param_list + pos); pos += 2;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    result = simp_ble_service_send_v4_indicate(conn_id,  simp_srv_id, p_value, length);
    hrp_profile_simp_ble_cmd_result(result);
    os_mem_free(p_value);
}


static void  hrp_simp_ble_service_simple_v8_notify(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    // T_SERVER_ID service_id;
    uint8_t *p_value;
    uint16_t length;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    //LE_ARRAY_TO_UINT8(service_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(length, p_param_list + pos); pos += 2;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    result = simp_ble_service_simple_v8_notify(conn_id,  simp_srv_id, p_value, length);
    hrp_profile_simp_ble_cmd_result(result);
    os_mem_free(p_value);
}

static void  hrp_simp_ble_service_simple_v8_indicate(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    // T_SERVER_ID service_id;
    uint8_t *p_value;
    uint16_t length;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    // LE_ARRAY_TO_UINT8(service_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT16(length, p_param_list + pos); pos += 2;

    p_value = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_value, &pos, p_param_list, length);

    result = simp_ble_service_simple_v8_indicate(conn_id,  simp_srv_id, p_value, length);
    hrp_profile_simp_ble_cmd_result(result);
    os_mem_free(p_value);
}

static void  hrp_simp_ble_service_add_service(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    uint8_t count;

    LE_ARRAY_TO_UINT8(count, p_param_list + pos); pos += 1;
    for (uint8_t i = 1; i <= count; i++)
    {
        simp_ble_service_add_service((void *)hrp_profile_callback);
        APP_PRINT_INFO1("hrp_simp_ble_service_add_service :add service  %d  times.",  i);
    }
}


void (*(hrp_profile_handle_simp_ble[]))(uint16_t len, uint8_t *p_param_list) =
{
#if F_BT_LE_GATT_CLIENT_SUPPORT
    hrp_simp_ble_client_start_all_discovery,
    hrp_simp_ble_client_start_discovery,   //0x01
    hrp_simp_ble_client_read_by_handle,
    hrp_simp_ble_client_read_by_uuid,
    hrp_simp_ble_client_write_v2_char,
    hrp_simp_ble_client_write_v6_char,   //0x05
    hrp_simp_ble_client_set_v3_notify,
    hrp_simp_ble_client_set_v4_ind,
    hrp_simp_ble_client_set_v8_cccd,
    hrp_simp_ble_client_get_hdl_cache,
    hrp_simp_ble_client_set_hdl_cache,   //0x0a
#else
    NULL,
    NULL,   //0x01
    NULL,
    NULL,
    NULL,
    NULL,   //0x05
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,   //0x0a
#endif
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    hrp_simp_ble_service_set_parameter, //0x10
    hrp_simp_ble_service_send_v3_notify,
    hrp_simp_ble_service_send_v4_indicate,
    hrp_simp_ble_service_simple_v8_notify,
    hrp_simp_ble_service_simple_v8_indicate,
    hrp_simp_ble_service_add_service,   //0x15


};


#if F_BT_LE_GATT_CLIENT_SUPPORT
/*********************************handle gaps client event function**********************************/
void hrp_profile_handle_gaps_client_cb_type_disc_state(T_GAPS_CLIENT_CB_TYPE cb_type,
                                                       T_GAPS_DISC_STATE disc_state)
{

    uint8_t Param[2];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, cb_type); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, disc_state); pos += 1;

    hrp_profile_gaps_cleint_evet(HRP_GAPS_CLIENT_CB_TYPE_DISC_STATE, pos, Param);
}

void  hrp_profile_handle_gaps_client_cb_type_read_device_name(T_GAPS_CLIENT_CB_TYPE  cb_type,
                                                              T_GAPS_READ_TYPE type,
                                                              uint16_t cause, uint16_t value_size, uint8_t *p_value)
{
    uint8_t Param[100] = {0};
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, cb_type); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, type); pos += 1;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;
    LE_UINT16_TO_ARRAY(Param + pos, value_size); pos += 2;

    hrp_profile_commit_buf(Param, &pos, p_value, value_size);

    hrp_profile_gaps_cleint_evet(HRP_GAPS_READ_DEVICE_NAME, pos, Param);

}
void  hrp_profile_handle_gaps_client_cb_type_read_apperance(T_GAPS_CLIENT_CB_TYPE  cb_type,
                                                            T_GAPS_READ_TYPE type,
                                                            uint16_t cause, uint16_t appearance)
{
    uint8_t Param[6];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, cb_type); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, type); pos += 1;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;
    LE_UINT16_TO_ARRAY(Param + pos, appearance); pos += 2;

    hrp_profile_gaps_cleint_evet(HRP_GAPS_READ_APPEARANCE, pos, Param);

}
void  hrp_profile_handle_gaps_client_cb_type_read_central_addr_resolution(
    T_GAPS_CLIENT_CB_TYPE cb_type, T_GAPS_READ_TYPE type, uint16_t cause, uint8_t central_addr_res)
{
    uint8_t Param[5];
    int pos = 0;
    LE_UINT8_TO_ARRAY(Param + pos, cb_type); pos += 1;
    LE_UINT8_TO_ARRAY(Param + pos, type); pos += 1;
    LE_UINT16_TO_ARRAY(Param + pos, cause); pos += 2;
    LE_UINT8_TO_ARRAY(Param + pos, central_addr_res); pos += 1;

    hrp_profile_gaps_cleint_evet(HRP_GAPS_READ_CENTRAL_ADDR_RESOLUTION, pos, Param);

}


void    hrp_profile_client_gaps_callback(T_GAPS_CLIENT_CB_DATA *p_gaps_cb_data)
{
    switch (p_gaps_cb_data->cb_type)
    {
    case GAPS_CLIENT_CB_TYPE_DISC_STATE:
        {
            switch (p_gaps_cb_data->cb_content.disc_state)
            {
            case DISC_GAPS_DONE:
                /* Discovery Simple BLE service procedure successfully done. */
                APP_PRINT_INFO0("app_client_callback: discover gaps procedure done.");
                break;
            case DISC_GAPS_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("app_client_callback: discover gaps request failed.");
                break;
            default:
                break;
            }

            hrp_profile_handle_gaps_client_cb_type_disc_state(p_gaps_cb_data->cb_type,
                                                              p_gaps_cb_data->cb_content.disc_state);

        }

        break;
    case GAPS_CLIENT_CB_TYPE_READ_RESULT:
        {
            switch (p_gaps_cb_data->cb_content.read_result.type)
            {
            case GAPS_READ_DEVICE_NAME:
                {
                    if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                    {
                        APP_PRINT_INFO1("GAPS_READ_DEVICE_NAME: device name %s.",
                                        TRACE_STRING(p_gaps_cb_data->cb_content.read_result.data.device_name.p_value));
                    }
                    else
                    {
                        APP_PRINT_INFO1("GAPS_READ_DEVICE_NAME: failded cause 0x%x",
                                        p_gaps_cb_data->cb_content.read_result.cause);
                    }
                    hrp_profile_handle_gaps_client_cb_type_read_device_name(p_gaps_cb_data->cb_type,
                                                                            p_gaps_cb_data->cb_content.read_result.type,
                                                                            p_gaps_cb_data->cb_content.read_result.cause,
                                                                            p_gaps_cb_data->cb_content.read_result.data.device_name.value_size,
                                                                            p_gaps_cb_data->cb_content.read_result.data.device_name.p_value);
                }

                break;
            case GAPS_READ_APPEARANCE:
                {

                    if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                    {
                        APP_PRINT_INFO1("GAPS_READ_APPEARANCE: appearance %d",
                                        p_gaps_cb_data->cb_content.read_result.data.appearance);
                    }
                    else
                    {
                        APP_PRINT_INFO1("GAPS_READ_APPEARANCE: failded cause 0x%x",
                                        p_gaps_cb_data->cb_content.read_result.cause);
                    }

                    hrp_profile_handle_gaps_client_cb_type_read_apperance(p_gaps_cb_data->cb_type,
                                                                          p_gaps_cb_data->cb_content.read_result.type,
                                                                          p_gaps_cb_data->cb_content.read_result.cause,
                                                                          p_gaps_cb_data->cb_content.read_result.data.appearance);
                }

                break;
            case GAPS_READ_CENTRAL_ADDR_RESOLUTION:
                {
                    if (p_gaps_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                    {
                        APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: central_addr_res %d",
                                        p_gaps_cb_data->cb_content.read_result.data.central_addr_res);
                    }
                    else
                    {
                        APP_PRINT_INFO1("GAPS_READ_CENTRAL_ADDR_RESOLUTION: failded cause 0x%x",
                                        p_gaps_cb_data->cb_content.read_result.cause);
                    }
                    hrp_profile_handle_gaps_client_cb_type_read_central_addr_resolution(p_gaps_cb_data->cb_type,
                                                                                        p_gaps_cb_data->cb_content.read_result.type,
                                                                                        p_gaps_cb_data->cb_content.read_result.cause,
                                                                                        p_gaps_cb_data->cb_content.read_result.data.central_addr_res);
                }

                break;
            default:
                break;
            }
        }

        break;

    default:
        break;
    }

}
///=============================Gaps client   cmd function======================
static void hrp_gaps_start_discovery(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;

    result = gaps_start_discovery(conn_id);

    hrp_profile_gaps_client_cmd_result(result);
}

static void hrp_gaps_read(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint8_t read_type;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(read_type, p_param_list + pos); pos += 1;

    result = gaps_read(conn_id, (T_GAPS_READ_TYPE)read_type);

    hrp_profile_gaps_client_cmd_result(result);
}

static void hrp_gaps_get_hdl_cache(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint16_t *p_hdl_cache;
    uint8_t  length;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(length, p_param_list + pos); pos += 1;
    p_hdl_cache = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    //hrp_profile_fetch_buf(p_hdl_cache, &pos, p_param_list, length);

    result = gaps_get_hdl_cache(conn_id, p_hdl_cache, length);

    hrp_profile_gaps_client_cmd_result(result);

    os_mem_free(p_hdl_cache);
}

static void hrp_gaps_set_hdl_cache(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    uint16_t *p_hdl_cache;
    uint8_t  length;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;
    LE_ARRAY_TO_UINT8(length, p_param_list + pos); pos += 1;
    p_hdl_cache = os_mem_alloc(RAM_TYPE_DATA_ON, length);

    hrp_profile_fetch_buf(p_hdl_cache, &pos, p_param_list, length);

    result = gaps_set_hdl_cache(conn_id, p_hdl_cache, length);

    hrp_profile_gaps_client_cmd_result(result);
    os_mem_free(p_hdl_cache);
}
#endif
#if F_BT_LE_PRIVACY_SUPPORT
static void hrp_gaps_check_resolvable_private_addr_only_char(uint16_t len, uint8_t *p_param_list)
{
    uint16_t pos = 0;
    bool result;
    uint8_t conn_id;
    bool *p_is_exist;

    LE_ARRAY_TO_UINT8(conn_id, p_param_list + pos); pos += 1;

    result = gaps_check_resolvable_private_addr_only_char(conn_id, p_is_exist);

    hrp_profile_gaps_client_cmd_result(result);
}
#endif

void (*(hrp_profile_handle_gaps_client[]))(uint16_t len, uint8_t *p_param_list) =
{
    NULL,
#if F_BT_LE_GATT_CLIENT_SUPPORT
    hrp_gaps_start_discovery,   //0x01
    hrp_gaps_read,
    hrp_gaps_get_hdl_cache,
    hrp_gaps_set_hdl_cache,
#else
    NULL,   //0x01
    NULL,
    NULL,
    NULL,
#endif
#if F_BT_LE_PRIVACY_SUPPORT
    hrp_gaps_check_resolvable_private_addr_only_char,
#else
    NULL,
#endif
    NULL,
    NULL,
    NULL,


};
