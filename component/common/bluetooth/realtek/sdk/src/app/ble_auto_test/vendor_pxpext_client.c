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
#if F_BT_LE_GATT_CLIENT_SUPPORT
/** Add Includes here **/
#include "trace_app.h"
#include <string.h>
#include "profile_client.h"
#include "vendor_pxpext_client.h"


typedef struct
{
    bool is_find_pxp_char;
    T_GATT_CHARACT_ELEM128 pxp_char;
} T_PXP_LINK;

/** @brief  App link table */
T_PXP_LINK pxp_table[4];
static T_CLIENT_ID pxp_client_id = CLIENT_PROFILE_GENERAL_ID;

static P_FUN_GENERAL_APP_CB pxp_client_cb = NULL;

const uint8_t   GATT_UUID128_PXP_CHAR[16] =
{0xA6, 0xF6, 0xF6, 0x07, 0x4D, 0xC4, 0x9D, 0x98, 0x6D, 0x45, 0x29, 0xBB, 0xD1, 0xFF, 0x00, 0x00};

bool vendor_pxpext_start_discovery(uint8_t conn_id)
{
    return client_by_uuid128_char_discovery(conn_id, pxp_client_id, 1, 0xffff,
                                            (uint8_t *)GATT_UUID128_PXP_CHAR);
}

bool vendor_pxpext_write_value(uint8_t conn_id, uint16_t length, uint8_t *p_value)
{
    uint16_t handle;
    bool hdl_valid = false;

    if (pxp_table[conn_id].pxp_char.value_handle)
    {
        handle = pxp_table[conn_id].pxp_char.value_handle;
        hdl_valid = true;
    }

    if (hdl_valid)
    {
        if (client_attr_write(conn_id, pxp_client_id, GATT_WRITE_TYPE_REQ, handle,
                              length, p_value) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("vendor_pxpext_write_value: Request fail! Please check!");
    return false;
}

bool vendor_pxpext_read_value(uint8_t conn_id)
{
    uint16_t handle;
    bool hdl_valid = false;

    if (pxp_table[conn_id].pxp_char.value_handle)
    {
        handle = pxp_table[conn_id].pxp_char.value_handle;
        hdl_valid = true;
    }

    if (hdl_valid)
    {
        if (client_attr_read(conn_id, pxp_client_id, handle) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    APP_PRINT_WARN0("vendor_pxpext_read_value: Request fail! Please check!");
    return false;
}

static void vendor_pxpext_client_discover_state_cb(uint8_t conn_id,
                                                   T_DISCOVERY_STATE discovery_state)
{
    T_PXP_CB_DATA cb_data;
    cb_data.cb_type = PXP_CLIENT_CB_TYPE_DISC_RESULT;
    cb_data.cb_content.disc_result.is_found = false;

    APP_PRINT_INFO1("vendor_pxpext_client_discover_state_cb: discovery_state = %d", discovery_state);
    if (discovery_state == DISC_STATE_CHAR_UUID128_DONE)
    {
        if (pxp_table[conn_id].is_find_pxp_char)
        {
            cb_data.cb_content.disc_result.is_found = true;
            memcpy(&cb_data.cb_content.disc_result.pxp_char, &pxp_table[conn_id].pxp_char,
                   sizeof(T_GATT_CHARACT_ELEM128));
        }
    }
    else if (discovery_state == DISC_STATE_FAILED)
    {
    }

    /* Send discover state to application if needed. */
    if (pxp_client_cb)
    {
        (*pxp_client_cb)(pxp_client_id, conn_id, &cb_data);
    }
    return;
}

static void vendor_pxpext_client_discover_result_cb(uint8_t conn_id,
                                                    T_DISCOVERY_RESULT_TYPE result_type,
                                                    T_DISCOVERY_RESULT_DATA result_data)
{
    APP_PRINT_INFO1("vendor_pxpext_client_discover_result_cb: result_type = %d", result_type);
    if (result_type == DISC_RESULT_BY_UUID128_CHAR)
    {
        memcpy(&pxp_table[conn_id].pxp_char, result_data.p_char_uuid128_disc_data,
               sizeof(T_GATT_CHARACT_ELEM128));
        pxp_table[conn_id].is_find_pxp_char = true;
    }

    return;
}

static void vendor_pxpext_client_write_cb(uint8_t conn_id, T_GATT_WRITE_TYPE type,
                                          uint16_t handle, uint16_t cause,
                                          uint8_t credits)
{
    T_PXP_CB_DATA cb_data;
    cb_data.cb_type = PXP_CLIENT_CB_TYPE_WRITE_RESULT;

    APP_PRINT_INFO1("vendor_pxpext_client_write_cb: result = 0x%x", cause);

    /* If write req success, branch to fetch value and send to application. */
    if (handle == pxp_table[conn_id].pxp_char.value_handle)
    {
        cb_data.cb_content.write_result.cause = cause;
    }

    /* Inform application the write result. */
    if (pxp_client_cb)
    {
        (*pxp_client_cb)(pxp_client_id, conn_id, &cb_data);
    }

    return;
}

static void vendor_pxpext_client_read_cb(uint8_t conn_id,  uint16_t cause,
                                         uint16_t handle, uint16_t value_size, uint8_t *pValue)
{
    T_PXP_CB_DATA cb_data;
    cb_data.cb_type = PXP_CLIENT_CB_TYPE_READ_RESULT;

    APP_PRINT_INFO3("vendor_pxpext_client_read_cb: result= %d, handle = 0x%4.4x, size = %d", cause,
                    handle, value_size);

    if (handle == pxp_table[conn_id].pxp_char.value_handle)
    {
        cb_data.cb_content.read_result.cause = cause;
        cb_data.cb_content.read_result.value_size = value_size;
        cb_data.cb_content.read_result.pValue = pValue;
        /* Inform application the read result. */
        if (pxp_client_cb)
        {
            (*pxp_client_cb)(pxp_client_id, conn_id, &cb_data);
        }
    }


    return;
}

static void vendor_pxpext_client_disc_cb(uint8_t conn_id)
{
    APP_PRINT_INFO0("vendor_pxpext_client_disc_cb.");
    memset(&pxp_table[conn_id], 0, sizeof(T_PXP_LINK));
    return;
}

const T_FUN_CLIENT_CBS PXP_CLIENT_CBS =
{
    vendor_pxpext_client_discover_state_cb,   //!< Discovery State callback function pointer
    vendor_pxpext_client_discover_result_cb,  //!< Discovery result callback function pointer
    vendor_pxpext_client_read_cb,      //!< Read response callback function pointer
    vendor_pxpext_client_write_cb,     //!< Write result callback function pointer
    NULL,  //!< Notify Indicate callback function pointer
    vendor_pxpext_client_disc_cb       //!< Link disconnection callback function pointer
};


T_CLIENT_ID vendor_pxpext_client_add(P_FUN_GENERAL_APP_CB app_cb)
{
    T_CLIENT_ID client_id;
    if (false == client_register_spec_client_cb(&client_id, &PXP_CLIENT_CBS))
    {
        pxp_client_id = CLIENT_PROFILE_GENERAL_ID;
        APP_PRINT_ERROR0("vendor_pxpext_client_add Fail !!!");
        return pxp_client_id;
    }
    pxp_client_id = client_id;
    APP_PRINT_INFO1("vendor_pxpext_client_add: client ID = %d", pxp_client_id);

    /* register callback for profile to inform application that some events happened. */
    pxp_client_cb = app_cb;

    return client_id;
}
#endif
