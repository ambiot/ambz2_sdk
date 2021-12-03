/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    ota_client.c
  * @brief   ota service client source file.
  * @details
  * @author
  * @date
  * @version
  ******************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2015 Realtek Semiconductor Corporation</center></h2>
  ******************************************************************************
  */

/** Add Includes here **/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT) && CONFIG_BT_OTA_CENTRAL_CLIENT
#include <string.h>
#include <stdbool.h>
#include "trace_app.h"
#include "gap_conn_le.h"
#include "bt_types.h"
#include "mem_types.h"
#include "os_mem.h"
#include "ota_client.h"
/**
 * @brief  OTA client Link control block definition.
 */
typedef struct
{
    T_OTA_DISC_STATE disc_state;
    uint16_t         hdl_cache[HDL_OTA_CACHE_LEN];
} T_OTA_LINK, *P_OTA_LINK;


static P_OTA_LINK ota_table;
static uint8_t ota_link_num;

/**<  OTA client ID. */
static T_CLIENT_ID ota_client = CLIENT_PROFILE_GENERAL_ID;
/**<  Callback used to send data to app from OTA client layer. */
static P_FUN_GENERAL_APP_CB ota_client_cb = NULL;

const uint8_t ota_service_uuid[16] = {GATT_UUID_OTA_SERVICE};

/**
  * @brief  Used by application, to start the discovery procedure of OTA server.
  * @param[in]  conn_id connection ID.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool ota_client_start_discovery(uint8_t conn_id)
{
    APP_PRINT_INFO0("ota_client_start_discovery");
    if (conn_id >= ota_link_num)
    {
        PROFILE_PRINT_ERROR1("ota_client_start_discovery: failed invalid conn_id %d", conn_id);
        return false;
    }
    /* First clear handle cache. */
    memset(&ota_table[conn_id], 0, sizeof(T_OTA_LINK));
    ota_table[conn_id].disc_state = DISC_OTA_START;
    if (client_by_uuid128_srv_discovery(conn_id, ota_client,
                                        (uint8_t *)ota_service_uuid) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    return false;
}


static bool ota_client_start_char_discovery(uint8_t conn_id)
{
    uint16_t start_handle;
    uint16_t end_handle;

    APP_PRINT_INFO0("ota_client_start_char_discovery");
    start_handle = ota_table[conn_id].hdl_cache[HDL_OTA_SRV_START];
    end_handle = ota_table[conn_id].hdl_cache[HDL_OTA_SRV_END];
    if (client_all_char_discovery(conn_id, ota_client, start_handle,
                                  end_handle) == GAP_CAUSE_SUCCESS)
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
bool ota_client_read_by_handle(uint8_t conn_id, T_OTA_READ_TYPE read_type)
{
    bool hdl_valid = false;
    uint16_t  handle;
    if (conn_id >= ota_link_num)
    {
        PROFILE_PRINT_ERROR1("ota_client_read_by_handle: failed invalid conn_id %d", conn_id);
        return false;
    }

    switch (read_type)
    {
    case OTA_READ_DEVICE_MAC:
        if (ota_table[conn_id].hdl_cache[HDL_OTA_DEVICE_MAC])
        {
            handle = ota_table[conn_id].hdl_cache[HDL_OTA_DEVICE_MAC];
            hdl_valid = true;
        }
        break;
    case OTA_READ_PATCH_VER:
        if (ota_table[conn_id].hdl_cache[HDL_OTA_PATCH_VER])
        {
            handle = ota_table[conn_id].hdl_cache[HDL_OTA_PATCH_VER];
            hdl_valid = true;
        }
        break;
    case OTA_READ_APP_VER:
        if (ota_table[conn_id].hdl_cache[HDL_OTA_APP_VER])
        {
            handle = ota_table[conn_id].hdl_cache[HDL_OTA_APP_VER];
            hdl_valid = true;
        }
        break;
    case OTA_READ_PATCH_EXT_VER:
        if (ota_table[conn_id].hdl_cache[HDL_OTA_PATCH_EXT_VER])
        {
            handle = ota_table[conn_id].hdl_cache[HDL_OTA_PATCH_EXT_VER];
            hdl_valid = true;
        }
        break;
    case OTA_READ_DEVICE_INFO:
        if (ota_table[conn_id].hdl_cache[HDL_OTA_DEVICE_INFO])
        {
            handle = ota_table[conn_id].hdl_cache[HDL_OTA_DEVICE_INFO];
            hdl_valid = true;
        }
        break;
    case OTA_READ_IMG_VER:
        if (ota_table[conn_id].hdl_cache[HDL_OTA_IMG_VER])
        {
            handle = ota_table[conn_id].hdl_cache[HDL_OTA_IMG_VER];
            hdl_valid = true;
        }
        break;

    default:
        return false;
    }

    if (hdl_valid)
    {
        if (client_attr_read(conn_id, ota_client, handle) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("ota_client_read_by_handle: Request fail! Please check!");
    return false;
}


/**
  * @brief  Used by application, to write ota cmd write Characteristic.
  * @param[in]  conn_id connection ID.
  * @param[in]  length  write data length
  * @param[in]  p_value point the value to write
  * @param[in]  type    write type.
  * @retval true send request to upper stack success.
  * @retval false send request to upper stack failed.
  */
bool ota_client_write_char(uint8_t conn_id, T_OTA_WRTIE_TYPE write_type)
{
    if (conn_id >= ota_link_num)
    {
        PROFILE_PRINT_ERROR1("ota_client_write_char: failed invalid conn_id %d", conn_id);
        return false;
    }

    switch (write_type)
    {
    case OTA_WRITE_CMD:
        {
            PROFILE_PRINT_INFO1("write handle 0x%x", ota_table[conn_id].hdl_cache[HDL_OTA_CMD]);
            if (ota_table[conn_id].hdl_cache[HDL_OTA_CMD])
            {
                uint16_t handle = ota_table[conn_id].hdl_cache[HDL_OTA_CMD];
                uint8_t ota_cmd = OTA_WRITE_OTA_CMD_CHAR_VAL;
                if (client_attr_write(conn_id, ota_client, GATT_WRITE_TYPE_CMD, handle,
                                      sizeof(ota_cmd), &ota_cmd) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            break;
        }
    case OTA_WRITE_TEST_MODE:
        {
            if (ota_table[conn_id].hdl_cache[HDL_OTA_TEST_MODE])
            {
                uint16_t handle = ota_table[conn_id].hdl_cache[HDL_OTA_TEST_MODE];
                uint8_t test_mode = OTA_WRITE_TEST_MODE_CHAR_VAL;
                if (client_attr_write(conn_id, ota_client, GATT_WRITE_TYPE_CMD, handle,
                                      sizeof(test_mode), &test_mode) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            break;
        }
    case OTA_WRITE_IMG_COUNTER:
        {
            if (ota_table[conn_id].hdl_cache[HDL_OTA_IMG_COUNTER])
            {
                uint16_t handle = ota_table[conn_id].hdl_cache[HDL_OTA_IMG_COUNTER];
                uint8_t img_counter = OTA_WRITE_OTA_IMG_COUNTER_CHAR_VAL;
                if (client_attr_write(conn_id, ota_client, GATT_WRITE_TYPE_CMD, handle,
                                      sizeof(img_counter), &img_counter) == GAP_CAUSE_SUCCESS)
                {
                    return true;
                }
            }
            break;
        }
    default:
        break;

    }

    PROFILE_PRINT_WARN1("ota_client_write_char fail! write_type: %d!", write_type);
    return false;
}


/**
  * @brief  Used by application, to get handle cache.
  * @param[in]  conn_id connection ID.
  * @param[in]  p_hdl_cache pointer of the handle cache table
  * @param[in]  len the length of handle cache table
  * @retval true success.
  * @retval false failed.
  */
bool ota_client_get_hdl_cache(uint8_t conn_id, uint16_t *p_hdl_cache, uint8_t len)
{
    if (conn_id >= ota_link_num)
    {
        PROFILE_PRINT_ERROR1("ota_client_get_hdl_cache: failed invalid conn_id %d", conn_id);
        return false;
    }
    if (ota_table[conn_id].disc_state != DISC_OTA_DONE)
    {
        PROFILE_PRINT_ERROR1("ota_client_get_hdl_cache: failed invalid state %d",
                             ota_table[conn_id].disc_state);
        return false;
    }
    if (len != sizeof(uint16_t) * HDL_OTA_CACHE_LEN)
    {
        PROFILE_PRINT_ERROR1("ota_client_get_hdl_cache: failed invalid len %d", len);
        return false;
    }
    memcpy(p_hdl_cache, ota_table[conn_id].hdl_cache, len);
    return true;
}
/**
  * @brief  Used by application, to set handle cache.
  * @param[in]  conn_id connection ID.
  * @param[in]  p_hdl_cache pointer of the handle cache table
  * @param[in]  len the length of handle cache table
  * @retval true success.
  * @retval false failed.
  */
bool ota_client_set_hdl_cache(uint8_t conn_id, uint16_t *p_hdl_cache, uint8_t len)
{
    if (conn_id >= ota_link_num)
    {
        PROFILE_PRINT_ERROR1("ota_client_set_hdl_cache: failed invalid conn_id %d", conn_id);
        return false;
    }
    if (ota_table[conn_id].disc_state != DISC_OTA_IDLE)
    {
        PROFILE_PRINT_ERROR1("ota_client_set_hdl_cache: failed invalid state %d",
                             ota_table[conn_id].disc_state);
        return false;
    }
    if (len != sizeof(uint16_t) * HDL_OTA_CACHE_LEN)
    {
        PROFILE_PRINT_ERROR1("ota_client_set_hdl_cache: failed invalid len %d", len);
        return false;
    }
    memcpy(ota_table[conn_id].hdl_cache, p_hdl_cache, len);
    ota_table[conn_id].disc_state = DISC_OTA_DONE;
    return true;
}


static void ota_client_discover_state_cb(uint8_t conn_id,  T_DISCOVERY_STATE discovery_state)
{
    bool cb_flag = false;
    T_OTA_CLIENT_CB_DATA cb_data;
    cb_data.cb_type = OTA_CLIENT_CB_TYPE_DISC_STATE;

    PROFILE_PRINT_INFO1("ota_client_discover_state_cb: discovery_state = %d", discovery_state);
    if (ota_table[conn_id].disc_state == DISC_OTA_START)
    {
        switch (discovery_state)
        {
        case DISC_STATE_SRV_DONE:
            /* Indicate that service handle found. Start discover characteristic. */
            if ((ota_table[conn_id].hdl_cache[HDL_OTA_SRV_START] != 0)
                || (ota_table[conn_id].hdl_cache[HDL_OTA_SRV_END] != 0))
            {
                if (ota_client_start_char_discovery(conn_id) == false)
                {
                    ota_table[conn_id].disc_state = DISC_OTA_FAILED;
                    cb_flag = true;
                }
            }
            /* No ota handle found. Discover procedure complete. */
            else
            {
                ota_table[conn_id].disc_state = DISC_OTA_FAILED;
                cb_flag = true;
            }
            break;
        case DISC_STATE_CHAR_DONE:
            ota_table[conn_id].disc_state = DISC_OTA_DONE;
            cb_flag = true;
            break;
        case DISC_STATE_CHAR_DESCRIPTOR_DONE:
            break;
        default:
            PROFILE_PRINT_ERROR0("Invalid Discovery State!");
            break;
        }
    }

    /* Send discover state to application if needed. */
    if (cb_flag && ota_client_cb)
    {
        cb_data.cb_content.disc_state = ota_table[conn_id].disc_state;
        (*ota_client_cb)(ota_client, conn_id, &cb_data);
    }
    return;
}


static void ota_client_discover_result_cb(uint8_t conn_id,  T_DISCOVERY_RESULT_TYPE result_type,
                                          T_DISCOVERY_RESULT_DATA result_data)
{
    uint16_t handle;
    uint16_t *hdl_cache;
    hdl_cache = ota_table[conn_id].hdl_cache;
    PROFILE_PRINT_INFO1("ota_client_discover_result_cb: result_type = %d", result_type);
    if (ota_table[conn_id].disc_state == DISC_OTA_START)
    {
        switch (result_type)
        {
        case DISC_RESULT_SRV_DATA:
            PROFILE_PRINT_INFO0("DISC_RESULT_SRV_DATA");
            ota_table[conn_id].hdl_cache[HDL_OTA_SRV_START] =
                result_data.p_srv_disc_data->att_handle;
            ota_table[conn_id].hdl_cache[HDL_OTA_SRV_END] =
                result_data.p_srv_disc_data->end_group_handle;
            break;

        case DISC_RESULT_CHAR_UUID16:
            {
                PROFILE_PRINT_INFO2("DISC_RESULT_CHAR_UUID16: handle 0x%x, uuid 0x%x",
                                    result_data.p_char_uuid16_disc_data->value_handle,
                                    result_data.p_char_uuid16_disc_data->uuid16);
                handle = result_data.p_char_uuid16_disc_data->value_handle;
                switch (result_data.p_char_uuid16_disc_data->uuid16)
                {
                case GATT_UUID_CHAR_OTA:
                    hdl_cache[HDL_OTA_CMD] = handle;
                    break;
                case GATT_UUID_CHAR_MAC:
                    hdl_cache[HDL_OTA_DEVICE_MAC] = handle;
                    break;
                case GATT_UUID_CHAR_PATCH:
                    hdl_cache[HDL_OTA_PATCH_VER] = handle;
                    break;
                case GATT_UUID_CHAR_APP_VERSION:
                    hdl_cache[HDL_OTA_APP_VER] = handle;
                    break;
                case GATT_UUID_CHAR_PATCH_EXTENSION:
                    hdl_cache[HDL_OTA_PATCH_EXT_VER] = handle;
                    break;
                case GATT_UUID_CHAR_TEST_MODE:
                    hdl_cache[HDL_OTA_TEST_MODE] = handle;
                    break;
                case GATT_UUID_CHAR_DEVICE_INFO:
                    hdl_cache[HDL_OTA_DEVICE_INFO] = handle;
                    break;
                case GATT_UUID_CHAR_IMAGE_COUNT_TO_UPDATE:
                    hdl_cache[HDL_OTA_IMG_COUNTER] = handle;
                    break;
                case GATT_UUID_CHAR_IMAGE_VERSION:
                    hdl_cache[HDL_OTA_IMG_VER] = handle;
                    break;
                default:
                    /* have no intrest on this handle. */
                    break;
                }
            }
            break;

        default:
            PROFILE_PRINT_ERROR0("Invalid Discovery Result Type!");
            break;
        }
    }

    return;
}

static void ota_client_read_result_cb(uint8_t conn_id,  uint16_t cause,
                                      uint16_t handle, uint16_t value_size, uint8_t *p_value)
{
    T_OTA_CLIENT_CB_DATA cb_data;
    uint16_t *hdl_cache;
    hdl_cache = ota_table[conn_id].hdl_cache;
    cb_data.cb_type = OTA_CLIENT_CB_TYPE_READ_RESULT;

    PROFILE_PRINT_INFO2("ota_client_read_result_cb: handle 0x%x, cause 0x%x", handle, cause);
    cb_data.cb_content.read_result.cause = cause;

    if (handle == hdl_cache[HDL_OTA_DEVICE_MAC])
    {
        cb_data.cb_content.read_result.type = OTA_READ_DEVICE_MAC;
        if (cause == GAP_SUCCESS)
        {
            cb_data.cb_content.read_result.data.p_value = p_value;
            cb_data.cb_content.read_result.data.value_size = value_size;
        }
        else
        {
            cb_data.cb_content.read_result.data.value_size = 0;
        }
    }
    else if (handle == hdl_cache[HDL_OTA_PATCH_VER])
    {
        cb_data.cb_content.read_result.type = OTA_READ_PATCH_VER;
        if (cause == GAP_SUCCESS)
        {
            cb_data.cb_content.read_result.data.p_value = p_value;
            cb_data.cb_content.read_result.data.value_size = value_size;
        }
        else
        {
            cb_data.cb_content.read_result.data.value_size = 0;
        }
    }
    else if (handle == hdl_cache[HDL_OTA_APP_VER])
    {
        cb_data.cb_content.read_result.type = OTA_READ_APP_VER;
        if (cause == GAP_SUCCESS)
        {
            cb_data.cb_content.read_result.data.p_value = p_value;
            cb_data.cb_content.read_result.data.value_size = value_size;
        }
        else
        {
            cb_data.cb_content.read_result.data.value_size = 0;
        }
    }
    else if (handle == hdl_cache[HDL_OTA_PATCH_EXT_VER])
    {
        cb_data.cb_content.read_result.type = OTA_READ_PATCH_EXT_VER;
        if (cause == GAP_SUCCESS)
        {
            cb_data.cb_content.read_result.data.p_value = p_value;
            cb_data.cb_content.read_result.data.value_size = value_size;
        }
        else
        {
            cb_data.cb_content.read_result.data.value_size = 0;
        }
    }
    else if (handle == hdl_cache[HDL_OTA_DEVICE_INFO])
    {
        cb_data.cb_content.read_result.type = OTA_READ_DEVICE_INFO;
        if (cause == GAP_SUCCESS)
        {
            cb_data.cb_content.read_result.data.p_value = p_value;
            cb_data.cb_content.read_result.data.value_size = value_size;
        }
        else
        {
            cb_data.cb_content.read_result.data.value_size = 0;
        }
    }
    else if (handle == hdl_cache[HDL_OTA_IMG_VER])
    {
        cb_data.cb_content.read_result.type = OTA_READ_IMG_VER;
        if (cause == GAP_SUCCESS)
        {
            //ota header, secure boot, patch, app version
            cb_data.cb_content.read_result.data.p_value = p_value;
            cb_data.cb_content.read_result.data.value_size = value_size;
        }
        else
        {
            cb_data.cb_content.read_result.data.value_size = 0;
        }
    }
    else
    {
        return;
    }

    if (ota_client_cb)
    {
        (*ota_client_cb)(ota_client, conn_id, &cb_data);
    }
    return;
}

static void ota_client_write_result_cb(uint8_t conn_id, T_GATT_WRITE_TYPE type,
                                       uint16_t handle,
                                       uint16_t cause,
                                       uint8_t credits)
{
    T_OTA_CLIENT_CB_DATA cb_data;
    uint16_t *hdl_cache;
    hdl_cache = ota_table[conn_id].hdl_cache;
    cb_data.cb_type = OTA_CLIENT_CB_TYPE_WRITE_RESULT;

    PROFILE_PRINT_INFO2("ota_client_write_result_cb: handle 0x%x, cause 0x%x", handle, cause);
    cb_data.cb_content.write_result.cause = cause;

    if (handle == hdl_cache[HDL_OTA_CMD])
    {
        cb_data.cb_content.write_result.type = OTA_WRITE_CMD;
    }
    else if (handle == hdl_cache[HDL_OTA_TEST_MODE])
    {
        cb_data.cb_content.write_result.type = OTA_WRITE_TEST_MODE;
    }
    else if (handle == hdl_cache[HDL_OTA_IMG_COUNTER])
    {
        cb_data.cb_content.write_result.type = OTA_WRITE_IMG_COUNTER;
    }
    else
    {
        return;
    }

    /* Inform application the write result. */
    if (ota_client_cb)
    {
        (*ota_client_cb)(ota_client, conn_id, &cb_data);
    }
    return;
}


/**
 * @brief OTA Client Callbacks.
*/
const T_FUN_CLIENT_CBS ota_client_cbs =
{
    ota_client_discover_state_cb,   //!< Discovery State callback function pointer
    ota_client_discover_result_cb,  //!< Discovery result callback function pointer
    ota_client_read_result_cb,      //!< Read response callback function pointer
    ota_client_write_result_cb,     //!< Write result callback function pointer
    NULL, //ota_client_notify_ind_cb,       //!< Notify Indicate callback function pointer
    NULL//ota_client_disc_cb              //!< Link disconnection callback function pointer
};

/**
  * @brief  add Dfu client to application.
  * @param  appCB: pointer of app callback function to handle specific client module data.
  * @retval Client ID of the specific client module.
  */
T_CLIENT_ID ota_add_client(P_FUN_GENERAL_APP_CB app_cb, uint8_t link_num)
{
    uint16_t size;
    if (link_num > OTA_MAX_LINKS)
    {
        PROFILE_PRINT_ERROR1("ota_add_client: invalid link_num %d", link_num);
        return 0xff;
    }
    if (false == client_register_spec_client_cb(&ota_client, &ota_client_cbs))
    {
        ota_client = CLIENT_PROFILE_GENERAL_ID;
        DFU_PRINT_ERROR0("ota_client_add: fail!");
        return ota_client;
    }
    DFU_PRINT_INFO1("ota_client_add: client ID = %d", ota_client);
    ota_link_num = link_num;
    size = ota_link_num * sizeof(T_OTA_LINK);
    ota_table = os_mem_zalloc(RAM_TYPE_DATA_ON, size);

    /* register callback for profile to inform application that some events happened. */
    ota_client_cb = app_cb;
    return ota_client;

}

void ota_delete_client(void)
{
    if (ota_table != NULL) {
        os_mem_free(ota_table);
        ota_table = NULL;
    }
}


/**
  * @brief  ota client connect target device.
  * @param  p_le_scan_info: filtered scan info
  */
void ota_client_connect_device(T_LE_SCAN_INFO *p_le_scan_info)
{
    PROFILE_PRINT_INFO1("ota_client_connect_device: p_le_scan_info->adv_type = %d",
                        p_le_scan_info->adv_type);
    if (p_le_scan_info->adv_type != GAP_ADV_EVT_TYPE_UNDIRECTED)
    {
        return;
    }

    T_GAP_LE_CONN_REQ_PARAM conn_req_param;
    conn_req_param.scan_interval = 0xA0; 
    conn_req_param.scan_window = 0x80; 
    conn_req_param.conn_interval_min = 0x60;
    conn_req_param.conn_interval_max = 0x60;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 1000;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
    T_GAP_CAUSE cause;
    if (GAP_CAUSE_SUCCESS == (cause = le_connect(GAP_PHYS_CONN_INIT_1M_BIT, p_le_scan_info->bd_addr,
                                                 p_le_scan_info->remote_addr_type,
                                                 GAP_LOCAL_ADDR_LE_PUBLIC, 1000)))
    {
        PROFILE_PRINT_INFO2("ota_client_connect_device: try connecting %s, remote_addr_type=%d",
                            TRACE_BDADDR(p_le_scan_info->bd_addr), p_le_scan_info->remote_addr_type);
    }
    else
    {
        PROFILE_PRINT_WARN1("ota_client_connect_device: FAIL cause=%d!", cause);
    }


    return;
}

#endif
