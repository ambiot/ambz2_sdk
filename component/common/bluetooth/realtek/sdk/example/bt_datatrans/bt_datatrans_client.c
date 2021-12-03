/**
*****************************************************************************************
*     Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    simple_ble_client.c
  * @brief   Simple BLE client source file.
  * @details
  * @author  jane
  * @date    2016-02-18
  * @version v0.1
  ******************************************************************************
  */
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
/** Add Includes here **/
#include "trace_app.h"
#include <string.h>
#include "bt_datatrans_client.h"
#include "bt_datatrans_multilink_manager.h"
#include "bt_datatrans_module_param_config.h"
#include <os_mem.h>

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
typedef struct
{
    T_DTS_DISC_STATE disc_state;
    uint16_t          hdl_cache[HDL_DTS_CACHE_LEN];
} T_DTS_LINK, *P_DTS_LINK;
/**
  * @{ Used for CCCD handle discovering in discovery procedure
  */
typedef enum
{
    TRANS_CCCD_DISC_START,
    TRANS_CCCD_DISC_DT_WRITE_NOTIFY,
    TRANS_CCCD_DISC_DT_FLOW_NOTIFY,
    TRANS_CCCD_DISC_END
} TSimpCCCD_DiscState, *PSimpCCCD_DiscState;
/**
  * @}
  */

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
/**<  Simple BLE client ID. */
static P_DTS_LINK dts_table;
static uint8_t dts_link_num;
static T_CLIENT_ID dts_client = CLIENT_PROFILE_GENERAL_ID;
/**<  Callback used to send data to app from Simple BLE client layer. */
static P_FUN_GENERAL_APP_CB dts_client_cb = NULL;


/**
  * @brief  Used by application, to start the discovery procedure of ias server.
  * @param[in]  conn_id connection ID.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
extern uint8_t GATT_UUID128_DATATRANS_PROFILE[];
bool bt_datatrans_client_start_discovery(uint8_t conn_id)
{
    PROFILE_PRINT_INFO0("dts_client_start_discovery");
    if (conn_id >= dts_link_num)
    {
        PROFILE_PRINT_ERROR1("kns_client_start_discovery: failed invalid conn_id %d", conn_id);
        return false;
    }
    /* First clear handle cache. */
    memset(&dts_table[conn_id], 0, sizeof(T_DTS_LINK));
    dts_table[conn_id].disc_state = DISC_DTS_START;
    if (client_by_uuid128_srv_discovery(conn_id, dts_client,
                                        (uint8_t *)GATT_UUID128_DATATRANS_PROFILE) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    return false;
}


/**
  * @brief  Used by application, to read data from server by using handles.
  * @param[in]  conn_id connection ID.
  * @param[in]  read_type one of characteristic that has the readable property.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool dts_client_read_by_handle(uint8_t conn_id,
                               T_DTS_READ_TYPE read_type)
{
    bool hdl_valid = false;
    uint16_t  handle;
    if (conn_id >= dts_link_num)
    {
        PROFILE_PRINT_ERROR1("dts_client_read_by_handle: failed invalid conn_id %d", conn_id);
        return false;
    }

    switch (read_type)
    {
    case DTS_READ_FLOW:
        if (dts_table[conn_id].hdl_cache[HDL_DTS_FLOW])
        {
            handle = dts_table[conn_id].hdl_cache[HDL_DTS_FLOW];
            hdl_valid = true;
        }
        break;
    default:
        return false;
    }

    if (hdl_valid)
    {
        if (client_attr_read(conn_id, dts_client, handle) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("dts_client_read_by_handle: Request fail! Please check!");
    return false;
}
/**
  * @brief  Used by application, to enable or disable the notification of peer server's V3 Notify Characteristic.
  * @param[in]  conn_id connection ID.
  * @param[in]  notify 0--disable the notification, 1--enable the notification.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool dts_client_set_data_notify(uint8_t conn_id, bool notify)
{
    if (conn_id >= dts_link_num)
    {
        PROFILE_PRINT_ERROR1("dts_client_set_v3_notify: failed invalid conn_id %d", conn_id);
        return false;
    }

    if (dts_table[conn_id].hdl_cache[HDL_DTS_VALUE_NOTIFY_CCCD])
    {
        uint16_t handle = dts_table[conn_id].hdl_cache[HDL_DTS_VALUE_NOTIFY_CCCD];
        uint16_t length = sizeof(uint16_t);
        uint16_t cccd_bits = notify ? 1 : 0;
        if (client_attr_write(conn_id, dts_client, GATT_WRITE_TYPE_REQ, handle,
                              length, (uint8_t *)&cccd_bits) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("dts_client_set_data_notify: Request fail! Please check!");
    return false;
}

/**
  * @brief  Used by application, to enable or disable the notification of peer server's V3 Notify Characteristic.
  * @param[in]  conn_id connection ID.
  * @param[in]  notify 0--disable the notification, 1--enable the notification.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool dts_client_set_flow_notify(uint8_t conn_id, bool notify)
{
    if (conn_id >= dts_link_num)
    {
        PROFILE_PRINT_ERROR1("dts_client_set_v3_notify: failed invalid conn_id %d", conn_id);
        return false;
    }

    if (dts_table[conn_id].hdl_cache[HDL_DTS_FLOW_CCCD])
    {
        uint16_t handle = dts_table[conn_id].hdl_cache[HDL_DTS_FLOW_CCCD];
        uint16_t length = sizeof(uint16_t);
        uint16_t cccd_bits = notify ? 1 : 0;
        if (client_attr_write(conn_id, dts_client, GATT_WRITE_TYPE_REQ, handle,
                              length, (uint8_t *)&cccd_bits) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("dts_client_set_data_notify: Request fail! Please check!");
    return false;
}


/**
  * @brief  Used by application, to write data of V2 write Characteristic.
  * @param[in]  conn_id connection ID.
  * @param[in]  length  write data length
  * @param[in]  p_value point the value to write
  * @param[in]  type    write type.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool kns_client_write_data_char(uint8_t conn_id, uint16_t length, uint8_t *p_value,
                                T_GATT_WRITE_TYPE type)
{
    if (conn_id >= dts_link_num)
    {
        PROFILE_PRINT_ERROR1("dts_client_write_v2_char: failed invalid conn_id %d", conn_id);
        return false;
    }

    if (dts_table[conn_id].hdl_cache[HDL_DTS_VALUE_WRITE])
    {
        uint16_t handle = dts_table[conn_id].hdl_cache[HDL_DTS_VALUE_WRITE];
        if (client_attr_write(conn_id, dts_client, type, handle,
                              length, p_value) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("dts_ble_client_write_v2_char: Request fail! Please check!");
    return false;
}

static bool dts_client_start_char_discovery(uint8_t conn_id)
{
    uint16_t start_handle;
    uint16_t end_handle;

    APP_PRINT_INFO0("dts_client_start_ias_char_discovery");
    start_handle = dts_table[conn_id].hdl_cache[HDL_DTS_SRV_START];
    end_handle = dts_table[conn_id].hdl_cache[HDL_DTS_SRV_END];
    if (client_all_char_discovery(conn_id, dts_client, start_handle,
                                  end_handle) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    return false;
}
static bool dts_client_start_char_descriptor_discovery(uint8_t conn_id)
{
    uint16_t start_handle;
    uint16_t end_handle;

    PROFILE_PRINT_INFO0("dts_client_start_char_descriptor_discovery");
    start_handle = dts_table[conn_id].hdl_cache[HDL_DTS_VALUE_NOTIFY];
    end_handle = dts_table[conn_id].hdl_cache[HDL_DTS_SRV_END];
    if (client_all_char_descriptor_discovery(conn_id, dts_client, start_handle,
                                             end_handle) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    return false;
}
/**
  * @brief  Called by profile client layer, when discover state of discovery procedure changed.
  * @param  conn_id: connection ID.
  * @param  discovery_state: current service discovery state.
  * @retval None
  */
static void datatrans_client_discover_state_cb(uint8_t conn_id,  T_DISCOVERY_STATE discovery_state)
{
    bool cb_flag = false;
    T_DTS_CLIENT_CB_DATA cb_data;
    cb_data.cb_type = DTS_CLIENT_CB_TYPE_DISC_STATE;

    APP_PRINT_INFO1("datatrans_client_discover_state_cb: discovery_state %d", discovery_state);

    if (dts_table[conn_id].disc_state == DISC_DTS_START)
    {
        uint16_t *hdl_cache;
        hdl_cache = dts_table[conn_id].hdl_cache;

        switch (discovery_state)
        {
        case DISC_STATE_SRV_DONE:
            /* Indicate that service handle found. Start discover characteristic. */
            if ((hdl_cache[HDL_DTS_SRV_START] != 0)
                || (hdl_cache[HDL_DTS_SRV_END] != 0))
            {
                if (dts_client_start_char_discovery(conn_id) == false)
                {
                    dts_table[conn_id].disc_state = DISC_DTS_FAILED;
                    cb_flag = true;
                }
            }
            /* No Ias BLE service handle found. Discover procedure complete. */
            else
            {
                dts_table[conn_id].disc_state = DISC_DTS_FAILED;
                cb_flag = true;
            }
            break;
        case DISC_STATE_CHAR_DONE:
            if (hdl_cache[HDL_DTS_FLOW] != 0)
            {
                if (dts_client_start_char_descriptor_discovery(conn_id) == false)
                {
                    dts_table[conn_id].disc_state = DISC_DTS_FAILED;
                    cb_flag = true;
                }
            }
            else
            {
                dts_table[conn_id].disc_state = DISC_DTS_FAILED;
                cb_flag = true;
            }
            break;
        case DISC_STATE_CHAR_DESCRIPTOR_DONE:
            dts_table[conn_id].disc_state = DISC_DTS_DONE;
            cb_flag = true;
            break;
        case DISC_STATE_FAILED:
            dts_table[conn_id].disc_state = DISC_DTS_DONE;
            cb_flag = true;
            break;
        default:
            APP_PRINT_ERROR0("kns_handle_discover_state: Invalid Discovery State!");
            break;
        }
    }

    /* Send discover state to application if needed. */
    if (cb_flag && dts_client_cb)
    {
        cb_data.cb_content.disc_state = dts_table[conn_id].disc_state;
        (*dts_client_cb)(dts_client, conn_id, &cb_data);
    }
    return;
}
/**
  * @brief  Called by profile client layer, when discover result fetched.
  * @param  conn_id: connection ID.
  * @param  result_type: indicate which type of value discovered in service discovery procedure.
  * @param  result_data: value discovered.
  * @retval None
  */
static void datatrans_client_discover_result_cb(uint8_t conn_id,
                                                T_DISCOVERY_RESULT_TYPE result_type,
                                                T_DISCOVERY_RESULT_DATA result_data)
{
    APP_PRINT_INFO1("dts_client_discover_result_cb: result_type %d", result_type);
    if (dts_table[conn_id].disc_state == DISC_DTS_START)
    {
        uint16_t handle;
        uint16_t *hdl_cache;
        hdl_cache = dts_table[conn_id].hdl_cache;

        switch (result_type)
        {
        case DISC_RESULT_SRV_DATA:
            hdl_cache[HDL_DTS_SRV_START] = result_data.p_srv_disc_data->att_handle;
            hdl_cache[HDL_DTS_SRV_END] = result_data.p_srv_disc_data->end_group_handle;
            break;
        case DISC_RESULT_CHAR_UUID128:
//            handle = result_data.p_char_uuid128_disc_data->value_handle;
//            if (!memcmp(result_data.p_char_uuid128_disc_data->uuid128, GATT_UUID128_PARAM, 16))
//            {
//                hdl_cache[HDL_KNS_PARA] = handle;
//            }
//            else if (!memcmp(result_data.p_char_uuid128_disc_data->uuid128, GATT_UUID128_KEY, 16))
//            {
//                hdl_cache[HDL_KNS_NOTIFY_KEY] = handle;
//            }
            break;

        case DISC_RESULT_CHAR_UUID16:
            handle = result_data.p_char_uuid16_disc_data->value_handle;
            if (result_data.p_char_uuid16_disc_data->uuid16 == GATT_UUID_CHAR_DATA)
            {
                dts_table[conn_id].hdl_cache[HDL_DTS_VALUE_WRITE] = handle;
                //dts_table[conn_id].properties = result_data.p_char_uuid16_disc_data->properties;
            }
            else if (result_data.p_char_uuid16_disc_data->uuid16 == GATT_UUID_CHAR_DATA_NOTIFY)
            {
                dts_table[conn_id].hdl_cache[HDL_DTS_VALUE_NOTIFY] = handle;
                //bas_table[conn_id].properties = result_data.p_char_uuid16_disc_data->properties;
            }
            else if (result_data.p_char_uuid16_disc_data->uuid16 == GATT_UUID_FLOW_CTRL_NOTIFY)
            {
                dts_table[conn_id].hdl_cache[HDL_DTS_FLOW] = handle;
                //bas_table[conn_id].properties = result_data.p_char_uuid16_disc_data->properties;
            }
            break;

        case DISC_RESULT_CHAR_DESC_UUID16:
            /* When use client_all_char_descriptor_discovery. */
            if (result_data.p_char_desc_uuid16_disc_data->uuid16 == GATT_UUID_CHAR_CLIENT_CONFIG)
            {
                handle = result_data.p_char_desc_uuid16_disc_data->handle;

                if ((handle > hdl_cache[HDL_DTS_VALUE_NOTIFY])
                    && (hdl_cache[HDL_DTS_VALUE_NOTIFY_CCCD] == 0))
                {
                    hdl_cache[HDL_DTS_VALUE_NOTIFY_CCCD] = handle;
                }
                else if ((handle > hdl_cache[HDL_DTS_FLOW])
                         && (hdl_cache[HDL_DTS_FLOW_CCCD] == 0))
                {
                    hdl_cache[HDL_DTS_FLOW_CCCD] = handle;
                }
            }
            break;

        default:
            APP_PRINT_ERROR0("kns_handle_discover_result: Invalid Discovery Result Type!");
            break;
        }
    }

    return;
}

/**
  * @brief  Called by profile client layer, when read request responsed.
  * @param  conn_id: connection ID.
  * @param  result: read request from peer device success or not.
  * @param  handle: handle of the value in read response.
  * @param  value_size: size of the value in read response.
  * @param  pValue: pointer to the value in read response.
  * @retval None
  */
static void datatrans_client_read_result_cb(uint8_t conn_id,  uint16_t cause,
                                            uint16_t handle, uint16_t value_size, uint8_t *p_value)
{
    T_DTS_CLIENT_CB_DATA cb_data;
    uint16_t *hdl_cache;
    hdl_cache = dts_table[conn_id].hdl_cache;

    cb_data.cb_type = DTS_CLIENT_CB_TYPE_READ_RESULT;

    APP_PRINT_INFO2("dts_client_read_result_cb: handle 0x%x, cause 0x%x", handle, cause);
    cb_data.cb_content.read_result.cause = cause;

    if (handle == hdl_cache[HDL_DTS_FLOW])
    {
        cb_data.cb_content.read_result.type = DTS_READ_FLOW;
        if (cause == GAP_SUCCESS)
        {
            cb_data.cb_content.read_result.data.flow_read.p_value = p_value;
            cb_data.cb_content.read_result.data.flow_read.value_size = value_size;
        }
        else
        {
            cb_data.cb_content.read_result.data.flow_read.value_size = 0;
        }
    }
    else if (handle == hdl_cache[HDL_DTS_VALUE_NOTIFY_CCCD])
    {
        cb_data.cb_content.read_result.type = DTS_READ_DATA_NOTIFY_CCCD;
        if (cause == GAP_SUCCESS)
        {
            uint16_t ccc_bit;
            if (value_size != 2)
            {
                PROFILE_PRINT_ERROR1("dts_client_read_result_cb: invalid cccd len %d", value_size);
                return;
            }
            LE_ARRAY_TO_UINT16(ccc_bit, p_value);

            if (ccc_bit & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {
                cb_data.cb_content.read_result.data.data_notify_cccd = true;
            }
            else
            {
                cb_data.cb_content.read_result.data.data_notify_cccd = false;
            }
        }
    }
    else if (handle == hdl_cache[HDL_DTS_FLOW_CCCD])
    {
        cb_data.cb_content.read_result.type = DTS_READ_FLOW_NOTIFY_CCCD;
        if (cause == GAP_SUCCESS)
        {
            uint16_t ccc_bit;
            if (value_size != 2)
            {
                PROFILE_PRINT_ERROR1("dts_client_read_result_cb: invalid cccd len %d", value_size);
                return;
            }
            LE_ARRAY_TO_UINT16(ccc_bit, p_value);

            if (ccc_bit & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {
                cb_data.cb_content.read_result.data.flow_notify_cccd = true;
            }
            else
            {
                cb_data.cb_content.read_result.data.flow_notify_cccd = false;
            }
        }
    }
    else
    {
        return;
    }
    /* Inform application the read result. */
    if (dts_client_cb)
    {
        (*dts_client_cb)(dts_client, conn_id, &cb_data);
    }

    return;
}

/**
  * @brief  Called by profile client layer, when write request complete.
  * @param  conn_id: connection ID.
  * @param  result: write request send success or not.
  * @retval None
  */
static void datatrans_client_write_result_cb(uint8_t conn_id, T_GATT_WRITE_TYPE type,
                                             uint16_t handle, uint16_t cause,
                                             uint8_t credits)
{
    T_DTS_CLIENT_CB_DATA cb_data;
    uint16_t *hdl_cache;
    hdl_cache = dts_table[conn_id].hdl_cache;
    cb_data.cb_type = DTS_CLIENT_CB_TYPE_WRITE_RESULT;

    PROFILE_PRINT_INFO2("dts_client_write_result_cb: handle 0x%x, cause 0x%x", handle, cause);
	//APP_PRINT_INFO2("dts_client_write_result_cb: handle 0x%x, cause 0x%x", handle, cause); //debug
	
    cb_data.cb_content.write_result.cause = cause;

    if (handle == hdl_cache[HDL_DTS_VALUE_WRITE])
    {
        cb_data.cb_content.write_result.type = DTS_WRITE_DATA;
    }
    else if (handle == hdl_cache[HDL_DTS_FLOW])
    {
        cb_data.cb_content.write_result.type = DTS_WRITE_CTRL;
    }
    else if (handle == hdl_cache[HDL_DTS_VALUE_NOTIFY_CCCD])
    {
        cb_data.cb_content.write_result.type = DTS_WRITE_DATA_NOTIFY_CCCD;
    }
    else if (handle == hdl_cache[HDL_DTS_FLOW_CCCD])
    {
        cb_data.cb_content.write_result.type = DTS_WRITE_FLOW_NOTIFY_CCCD;
    }
    else
    {
        return;
    }
    /* Inform application the write result. */
    if (dts_client_cb)
    {
        (*dts_client_cb)(dts_client, conn_id, &cb_data);
    }

    return;
}

/**
  * @brief  Called by profile client layer, when notification or indication arrived.
  * @param  conn_id: connection ID.
  * @param  handle: handle of the value in received data.
  * @param  value_size: size of the value in received data.
  * @param  pValue: pointer to the value in received data.
  * @retval APP_RESULT_SUCCESS--procedure OK.
  *         other--procedure exception.
  */
static T_APP_RESULT datatrans_client_notif_ind_result_cb(uint8_t conn_id, bool notify,
                                                         uint16_t handle,
                                                         uint16_t value_size, uint8_t *p_value)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    T_DTS_CLIENT_CB_DATA cb_data;
    uint16_t *hdl_cache;
    hdl_cache = dts_table[conn_id].hdl_cache;
	APP_PRINT_INFO1("datatrans_client_notif_ind_result_cb: handle 0x%x", handle);
    cb_data.cb_type = DTS_CLIENT_CB_TYPE_NOTIF_IND_RESULT;

    if (handle == hdl_cache[HDL_DTS_VALUE_NOTIFY])
    {
        cb_data.cb_content.notif_ind_data.type = DTS_DATA_NOTIFY;
        cb_data.cb_content.notif_ind_data.data.value_size = value_size;
        cb_data.cb_content.notif_ind_data.data.p_value = p_value;
    }
    else if (handle == hdl_cache[HDL_DTS_FLOW])
    {
        cb_data.cb_content.notif_ind_data.type = DTS_FLOW_NOTIFY;
        cb_data.cb_content.notif_ind_data.data.value_size = value_size;
        cb_data.cb_content.notif_ind_data.data.p_value = p_value;
    }
    else
    {
        return app_result;
    }
    /* Inform application the notif/ind result. */
    if (dts_client_cb)
    {
        app_result = (*dts_client_cb)(dts_client, conn_id, &cb_data);
    }

    return app_result;
}

/**
  * @brief  Called by profile client layer, when link disconnected.
  *         NOTE--we should reset some state when disconnected.
  * @param  conn_id: connection ID.
  * @retval None
  */
static void datatrans_client_disconnect_cb(uint8_t conn_id)
{
    APP_PRINT_INFO0("dts_client_disconnect_cb.");
    if (conn_id >= dts_link_num)
    {
        PROFILE_PRINT_ERROR1("dts_client_disconnect_cb: failed invalid conn_id %d", conn_id);
        return;
    }
    memset(&dts_table[conn_id], 0, sizeof(T_DTS_LINK));
    return;
}

/**
 * @brief Simple BLE Client Callbacks.
*/
const T_FUN_CLIENT_CBS dts_client_cbs =
{
    datatrans_client_discover_state_cb,   //!< Discovery State callback function pointer
    datatrans_client_discover_result_cb,  //!< Discovery result callback function pointer
    datatrans_client_read_result_cb,      //!< Read response callback function pointer
    datatrans_client_write_result_cb,     //!< Write result callback function pointer
    datatrans_client_notif_ind_result_cb,  //!< Notify Indicate callback function pointer
    datatrans_client_disconnect_cb       //!< Link disconnection callback function pointer
};

/**
  * @brief  add Simple BLE client to application.
  * @param  app_cb: pointer of app callback function to handle specific client module data.
  * @retval Client ID of the specific client module.
  */
T_CLIENT_ID datatrans_add_client(P_FUN_GENERAL_APP_CB app_cb, uint8_t link_num)
{

    uint16_t size;
    if (link_num > DTS_MAX_LINKS)
    {
        PROFILE_PRINT_ERROR1("datatrans_add_client: invalid link_num %d", link_num);
        return 0xff;
    }
    if (false == client_register_spec_client_cb(&dts_client, &dts_client_cbs))
    {
        dts_client = CLIENT_PROFILE_GENERAL_ID;
        APP_PRINT_ERROR0("datatrans_add_client failed");
        return dts_client;
    }
    APP_PRINT_INFO1("datatrans_add_client: dts_client %d", dts_client);

    /* register callback for profile to inform application that some events happened. */
    dts_client_cb = app_cb;
    dts_link_num = link_num;
    size = dts_link_num * sizeof(T_DTS_LINK);
    dts_table = os_mem_zalloc(RAM_TYPE_DATA_ON, size);

    return dts_client;
}

void bt_datatrans_delete_client(void)
{
    if (dts_table != NULL) {
        os_mem_free(dts_table);
        dts_table = NULL;
    }
}

#endif
