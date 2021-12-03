/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    vendor_tp_client.c
  * @brief
  * @details
  * @author  jane
  * @date    2016-02-18
  * @version v0.1
  ******************************************************************************
  */

/** Add Includes here **/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST
#include "trace_app.h"
#include <string.h>
#include "ble_throughput_vendor_tp_client.h"
#include "ble_throughput_vendor_tp_config.h"

#if F_BT_LE_GATT_CLIENT_SUPPORT
typedef struct
{
    bool is_find_tp_char;
    T_GATT_CHARACT_ELEM16 tp_char;
    uint16_t cccd_handle;
} T_TP_LINK;

/** @brief  App link table */
T_TP_LINK tp_table[4];
static T_CLIENT_ID tp_client_id = CLIENT_PROFILE_GENERAL_ID;

static P_FUN_GENERAL_APP_CB tp_client_cb = NULL;

bool tp_client_start_discovery(uint8_t conn_id)
{
    return client_by_uuid_char_discovery(conn_id, tp_client_id, 1, 0xffff,
                                         GATT_UUID_VENDOR_TP_NOTIFY_INDICATE);
}

bool tp_client_write_cccd(uint8_t conn_id, uint16_t cccd_bits)
{
    uint16_t handle;
    uint16_t length;
    uint8_t *p_data;
    bool hdl_valid = false;

    APP_PRINT_INFO1("tp_client_write_cccd: cccd_bits = 0x%x", cccd_bits);

    if (tp_table[conn_id].cccd_handle)
    {
        handle = tp_table[conn_id].cccd_handle;
        length = sizeof(uint16_t);
        p_data = (uint8_t *)&cccd_bits;
        hdl_valid = true;
    }

    if (hdl_valid)
    {
        if (client_attr_write(conn_id, tp_client_id, GATT_WRITE_TYPE_REQ, handle,
                              length, p_data) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("tp_client_write_cccd: Request fail! Please check!");
    return false;
}

bool tp_client_write_value(uint8_t conn_id, uint16_t length, uint8_t *p_value)
{
    uint16_t handle;
    bool hdl_valid = false;

    if (tp_table[conn_id].tp_char.value_handle)
    {
        handle = tp_table[conn_id].tp_char.value_handle;
        hdl_valid = true;
    }

    if (hdl_valid)
    {
        if (client_attr_write(conn_id, tp_client_id, GATT_WRITE_TYPE_REQ, handle,
                              length, p_value) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("tp_client_write_value: Request fail! Please check!");
    return false;
}

bool tp_client_write_command(uint8_t conn_id, uint16_t length, uint8_t *p_value)
{
    uint16_t handle;
    bool hdl_valid = false;

    if (tp_table[conn_id].tp_char.value_handle)
    {
        handle = tp_table[conn_id].tp_char.value_handle;
        hdl_valid = true;
    }

    if (hdl_valid)
    {
        if (client_attr_write(conn_id, tp_client_id, GATT_WRITE_TYPE_CMD, handle,
                              length, p_value) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("tp_client_write_command: Request fail! Please check!");
    return false;
}

bool tp_client_read_prefer_param(uint8_t conn_id)
{
    if (client_attr_read_using_uuid(conn_id, tp_client_id, 0x01, 0xffff,
                                    GATT_UUID_VENDOR_TP_PREFER_PARAM, NULL) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }

    APP_PRINT_WARN0("tp_client_read_prefer_param: Request fail! Please check!");
    return false;
}

static void tp_client_discover_state_cb(uint8_t conn_id,  T_DISCOVERY_STATE discovery_state)
{
    T_TP_CB_DATA cb_data;
    cb_data.cb_type = TP_CLIENT_CB_TYPE_DISC_RESULT;
    cb_data.cb_content.disc_result.is_found = false;

    APP_PRINT_INFO1("tp_client_discover_state_cb: discovery_state = %d", discovery_state);
    if (discovery_state == DISC_STATE_CHAR_UUID16_DONE)
    {
        if (tp_table[conn_id].is_find_tp_char)
        {
            cb_data.cb_content.disc_result.is_found = true;
            memcpy(&cb_data.cb_content.disc_result.char_tp, &tp_table[conn_id].tp_char,
                   sizeof(T_GATT_CHARACT_ELEM16));
        }
    }
    else if (discovery_state == DISC_STATE_FAILED)
    {
    }

    /* Send discover state to application if needed. */
    if (tp_client_cb)
    {
        (*tp_client_cb)(tp_client_id, conn_id, &cb_data);
    }
    return;
}

static void tp_client_discover_result_cb(uint8_t conn_id,  T_DISCOVERY_RESULT_TYPE result_type,
                                         T_DISCOVERY_RESULT_DATA result_data)
{
	//data_uart_print("tp_client_discover_result_cb: result_type = %d", result_type);
    APP_PRINT_INFO1("tp_client_discover_result_cb: result_type = %d", result_type);
    if (result_type == DISC_RESULT_BY_UUID16_CHAR)
    {
        tp_table[conn_id].tp_char.decl_handle = result_data.p_char_uuid16_disc_data->decl_handle;
        tp_table[conn_id].tp_char.properties = result_data.p_char_uuid16_disc_data->properties;
        tp_table[conn_id].tp_char.value_handle = result_data.p_char_uuid16_disc_data->value_handle;
        tp_table[conn_id].tp_char.uuid16 = result_data.p_char_uuid16_disc_data->uuid16;
        tp_table[conn_id].is_find_tp_char = true;
        tp_table[conn_id].cccd_handle = tp_table[conn_id].tp_char.value_handle + 1;
    }

    return;
}

static void tp_client_write_cb(uint8_t conn_id, T_GATT_WRITE_TYPE type,
                               uint16_t handle, uint16_t cause,
                               uint8_t credits)
{
    T_TP_CB_DATA cb_data;
    cb_data.cb_type = TP_CLIENT_CB_TYPE_WRITE_RESULT;
    cb_data.cb_content.write_result.credits = credits;
    cb_data.cb_content.write_result.write_type = type;

    APP_PRINT_INFO1("tp_client_write_cb: result = 0x%x", cause);

    /* If write req success, branch to fetch value and send to application. */
    if (handle == tp_table[conn_id].tp_char.value_handle)
    {
        cb_data.cb_content.write_result.type = TP_WRITE_CHAR_VALUE;
        cb_data.cb_content.write_result.cause = cause;
    }
    else if (handle == tp_table[conn_id].cccd_handle)
    {
        cb_data.cb_content.write_result.type = TP_WRITE_CCCD;
        cb_data.cb_content.write_result.cause = cause;
    }

    /* Inform application the write result. */
    if (tp_client_cb)
    {
        (*tp_client_cb)(tp_client_id, conn_id, &cb_data);
    }

    return;
}

static T_APP_RESULT tp_client_notify_ind_cb(uint8_t conn_id, bool notify, uint16_t handle,
                                            uint16_t value_size, uint8_t *pValue)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_TP_CB_DATA cb_data;

    cb_data.cb_type = TP_CLIENT_CB_TYPE_NOTIF_IND_RESULT;

    if (handle == tp_table[conn_id].tp_char.value_handle)
    {
        cb_data.cb_content.notif_ind_data.is_notif = notify;
        cb_data.cb_content.notif_ind_data.value_size = value_size;
        cb_data.cb_content.notif_ind_data.pValue = pValue;
    }
    else
    {
        return app_result;
    }
    /* Inform application the notif/ind result. */
    if (tp_client_cb)
    {
        app_result = (*tp_client_cb)(tp_client_id, conn_id, &cb_data);
    }

    return app_result;
}

static void tp_client_read_result_cb(uint8_t conn_id,  uint16_t cause,
                                     uint16_t handle, uint16_t value_size, uint8_t *p_value)
{
    T_TP_CB_DATA cb_data;
    cb_data.cb_type = TP_CLIENT_CB_TYPE_READ_RESULT;

    PROFILE_PRINT_INFO2("tp_client_read_result_cb: handle 0x%x, cause 0x%x", handle, cause);
    cb_data.cb_content.read_result.cause = cause;
    cb_data.cb_content.read_result.type = TP_READ_PREFER_PARAM;
    if (cause == GAP_SUCCESS)
    {
        cb_data.cb_content.read_result.value_size = value_size;
        cb_data.cb_content.read_result.p_value = p_value;
    }

    if (tp_client_cb)
    {
        (*tp_client_cb)(tp_client_id, conn_id, &cb_data);
    }
    return;
}

static void tp_client_disc_cb(uint8_t conn_id)
{
    APP_PRINT_INFO0("tp_client_disc_cb.");
    memset(&tp_table[conn_id], 0, sizeof(T_TP_LINK));
    return;
}

const T_FUN_CLIENT_CBS TP_CLIENT_CBS =
{
    tp_client_discover_state_cb,   //!< Discovery State callback function pointer
    tp_client_discover_result_cb,  //!< Discovery result callback function pointer
    tp_client_read_result_cb,      //!< Read response callback function pointer
    tp_client_write_cb,     //!< Write result callback function pointer
    tp_client_notify_ind_cb,  //!< Notify Indicate callback function pointer
    tp_client_disc_cb       //!< Link disconnection callback function pointer
};


T_CLIENT_ID tp_client_add(P_FUN_GENERAL_APP_CB app_cb)
{
    T_CLIENT_ID client_id;
    if (false == client_register_spec_client_cb(&client_id, &TP_CLIENT_CBS))
    {
        tp_client_id = CLIENT_PROFILE_GENERAL_ID;
        APP_PRINT_ERROR0("tp_client_add_client Fail !!!");
        return tp_client_id;
    }
    tp_client_id = client_id;
    APP_PRINT_INFO1("tp_client_add_client: client ID = %d", tp_client_id);

    /* register callback for profile to inform application that some events happened. */
    tp_client_cb = app_cb;

    return client_id;
}
#endif
#endif
