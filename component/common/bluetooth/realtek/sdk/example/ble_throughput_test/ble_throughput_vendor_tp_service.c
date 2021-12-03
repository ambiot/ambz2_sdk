/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     vendor_tp_service.c
* @brief    simple BLE profile source file.
* @details  Demonstration of how to implement a self-definition profile.
* @author
* @date     2016-02-18
* @version  v0.1
*********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_THROUGHPUT_TEST) && CONFIG_BT_THROUGHPUT_TEST
#include "trace_app.h"
#include <string.h>
#include "ble_throughput_vendor_tp_service.h"
#include "gap.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
T_SERVER_ID vendor_tp_service_id;


/**<  Function pointer used to send event to application from simple profile. Initiated in vendor_tp_service_add. */
static P_FUN_SERVER_GENERAL_CB pfn_vendor_tp_service_cb = NULL;
TTP_PERFER_PARAM vendor_tp_param;


/**< @brief  profile/service definition.  */
const T_ATTRIB_APPL vendor_tp_service_tbl[] =
{
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_VENDOR_TP_SERVICE),      /* service UUID */
            HI_WORD(GATT_UUID_VENDOR_TP_SERVICE)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                         /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_NOTIFY | GATT_CHAR_PROP_INDICATE | GATT_CHAR_PROP_WRITE | GATT_CHAR_PROP_WRITE_NO_RSP)             /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },

    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                         /* type_value */
            LO_WORD(GATT_UUID_VENDOR_TP_NOTIFY_INDICATE),
            HI_WORD(GATT_UUID_VENDOR_TP_NOTIFY_INDICATE)
        },
        0,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ | GATT_PERM_WRITE                            /* permissions */
    },
    /* client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,                 /* flags */
        {                                         /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* permissions */
    },
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ              /* permissions */
    },
    {
        ATTRIB_FLAG_VOID,                         /* flags */
        {                                               /* type_value */
            LO_WORD(GATT_UUID_VENDOR_TP_PREFER_PARAM),
            HI_WORD(GATT_UUID_VENDOR_TP_PREFER_PARAM)
        },
        sizeof(vendor_tp_param),                                                 /* bValueLen */
        (void *) &vendor_tp_param,
        GATT_PERM_READ                           /* permissions */
    },
};

void vendor_tp_service_config_param(TTP_PERFER_PARAM param)
{
    memcpy(&vendor_tp_param, &param, sizeof(TTP_PERFER_PARAM));
}

/**
 * @brief read characteristic data from service.
 *
 * @param service_id          ServiceID of characteristic data.
 * @param iAttribIndex          Attribute index of getting characteristic data.
 * @param iOffset                Used for Blob Read.
 * @param piLength            length of getting characteristic data.
 * @param ppValue            data got from service.
 * @return Profile procedure result
*/
T_APP_RESULT  vendor_tp_service_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id,
                                             uint16_t iAttribIndex, uint16_t iOffset, uint16_t *piLength, uint8_t **ppValue)
{
    T_APP_RESULT  wCause  = APP_RESULT_SUCCESS;

    switch (iAttribIndex)
    {
    default:
        APP_PRINT_ERROR1("vendor_tp_service_attr_read_cb, Attr not found, index=%d", iAttribIndex);
        wCause  = APP_RESULT_ATTR_NOT_FOUND;
        break;


    }

    return (wCause);
}


/**
 * @brief write characteristic data from service.
 *
 * @param ServiceID          ServiceID to be written.
 * @param iAttribIndex          Attribute index of characteristic.
 * @param wLength            length of value to be written.
 * @param pValue            value to be written.
 * @return Profile procedure result
*/
T_APP_RESULT vendor_tp_service_attr_write_cb(uint8_t conn_id, T_SERVER_ID service_id,
                                             uint16_t iAttribIndex, T_WRITE_TYPE write_type, uint16_t wLength, uint8_t *pValue,
                                             P_FUN_WRITE_IND_POST_PROC *pWriteIndPostProc)
{
    TTP_CALLBACK_DATA callback_data;
    T_APP_RESULT  wCause = APP_RESULT_SUCCESS;
    if (VENDOR_TP_SERVICE_CHAR_TP_WRITE_INDEX == iAttribIndex)
    {
        callback_data.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;
        callback_data.conn_id  = conn_id;
        callback_data.msg_data.write.write_type = write_type;
        if (write_type == WRITE_REQUEST)
        {
            /* Notify Application. */
            uint8_t *p_param = pValue;
            callback_data.msg_data.write.opcode = (VENDOR_TP_OP) * p_param;
            p_param += 1;

            LE_ARRAY_TO_UINT32(callback_data.msg_data.write.u.notify_param.count, p_param);
            p_param += 4;

            LE_ARRAY_TO_UINT16(callback_data.msg_data.write.u.notify_param.length, p_param);

            if (pfn_vendor_tp_service_cb)
            {
                pfn_vendor_tp_service_cb(service_id, (void *)&callback_data);
            }
        }
        else
        {
            callback_data.msg_data.write.u.write_data.length = wLength;
            callback_data.msg_data.write.u.write_data.p_value = pValue;
            if (pfn_vendor_tp_service_cb)
            {
                pfn_vendor_tp_service_cb(service_id, (void *)&callback_data);
            }
        }
    }
    else
    {
        APP_PRINT_ERROR2("--> vendor_tp_service_attr_write_cb Error  iAttribIndex = 0x%x wLength=%d",
                         iAttribIndex,
                         wLength);
        wCause = APP_RESULT_ATTR_NOT_FOUND;
    }
    return wCause;
}

/**
  * @brief send notification of simple notify characteristic value.
  *
  * @param[in] service_id         service ID of service.
  * @param[in] value             characteristic value to notify
  * @return notification action result
  * @retval 1 true
  * @retval 0 false
  */
//uint8_t counter = 0;

bool vendor_tp_service_v1_notification(uint8_t conn_id, T_SERVER_ID service_id, void *p_value,
                                       uint16_t length)
{
    uint8_t *p_data = (uint8_t *)p_value;
    uint16_t dataLen = length;

    APP_PRINT_INFO0("<-- vendor_tp_service_v1_notification");
    // send notification to client
    return server_send_data(conn_id, service_id, VENDOR_TP_SERVICE_CHAR_TP_WRITE_INDEX, p_data,
                            dataLen, GATT_PDU_TYPE_NOTIFICATION);
}

bool vendor_tp_service_v1_indication(uint8_t conn_id, T_SERVER_ID service_id, void *p_value,
                                     uint16_t length)
{
    uint8_t *p_data = (uint8_t *)p_value;
    uint16_t dataLen = length;

    APP_PRINT_INFO0("<-- vendor_tp_service_v1_indication");
	//data_uart_print("<-- vendor_tp_service_v1_indication");
    // send notification to client
    return server_send_data(conn_id, service_id, VENDOR_TP_SERVICE_CHAR_TP_WRITE_INDEX, p_data,
                            dataLen, GATT_PDU_TYPE_INDICATION);
}

/**
 * @brief update CCCD bits from stack.
 *
 * @param service_id          Service ID.
 * @param Index          Attribute index of characteristic data.
 * @param wCCCBits         CCCD bits from stack.
 * @return None
*/
void vendor_tp_service_cccd_update_cb(uint8_t conn_id, T_SERVER_ID serviceId, uint16_t Index,
                                      uint16_t wCCCBits)
{
    TTP_CALLBACK_DATA callback_data;

    callback_data.conn_id = conn_id;
    APP_PRINT_INFO2("vendor_tp_service_cccd_update_cb  Index = %d wCCCDBits %x", Index,
                    wCCCBits);
    switch (Index)
    {
    case VENDOR_TP_SERVICE_CHAR_TP_NOTIFY_INDICATE_CCCD_INDEX:
        {
            bool bHandle = true;
            if (wCCCBits & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
            {
                // Enable Notification
                callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;
                callback_data.msg_data.notification_indification_index = VENDOR_TP_SERVICE_V1_NOTIFICATION_ENABLE;

                /* Notify Application. */
                if (pfn_vendor_tp_service_cb && (bHandle == true))
                {
                    pfn_vendor_tp_service_cb(serviceId, (void *)&callback_data);
                }
            }
            else
            {
                // Disable Notification
                callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;
                callback_data.msg_data.notification_indification_index = VENDOR_TP_SERVICE_V1_NOTIFICATION_DISABLE;

                /* Notify Application. */
                if (pfn_vendor_tp_service_cb && (bHandle == true))
                {
                    pfn_vendor_tp_service_cb(serviceId, (void *)&callback_data);
                }
            }

            if (wCCCBits & GATT_CLIENT_CHAR_CONFIG_INDICATE)
            {
                // Enable Indication
                callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;
                callback_data.msg_data.notification_indification_index = VENDOR_TP_SERVICE_V1_INDICATION_ENABLE;

                /* Notify Application. */
                if (pfn_vendor_tp_service_cb && (bHandle == true))
                {
                    pfn_vendor_tp_service_cb(serviceId, (void *)&callback_data);
                }
            }
            else
            {
                // Disable Indication
                callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;
                callback_data.msg_data.notification_indification_index = VENDOR_TP_SERVICE_V1_INDICATION_DISABLE;

                /* Notify Application. */
                if (pfn_vendor_tp_service_cb && (bHandle == true))
                {
                    pfn_vendor_tp_service_cb(serviceId, (void *)&callback_data);
                }
            }

        }
        break;

    default:
        break;
    }
}


/**
 * @brief Simple ble Service Callbacks.
*/
const T_FUN_GATT_SERVICE_CBS vendor_tp_service_cbs =
{
    vendor_tp_service_attr_read_cb,  // Read callback function pointer
    vendor_tp_service_attr_write_cb, // Write callback function pointer
    vendor_tp_service_cccd_update_cb  // CCCD update callback function pointer
};



/**
  * @brief add Simple BLE service to application.
  *
  * @param[in] pFunc          pointer of app callback function called by profile.
  * @return service ID auto generated by profile layer.
  * @retval service_id
  */
T_SERVER_ID vendor_tp_service_add(void *pFunc)
{
    if (false == server_add_service(&vendor_tp_service_id,
                                    (uint8_t *)vendor_tp_service_tbl,
                                    sizeof(vendor_tp_service_tbl),
                                    vendor_tp_service_cbs))
    {
        APP_PRINT_ERROR1("vendor_tp_service_add: service_id %d", vendor_tp_service_id);
        vendor_tp_service_id = 0xff;
        return vendor_tp_service_id;
    }

    pfn_vendor_tp_service_cb = (P_FUN_SERVER_GENERAL_CB)pFunc;
    return vendor_tp_service_id;
}
#endif
