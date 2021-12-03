/**
********************************************************************************************************
Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file       gattsvc_dis.c
* @brief     device information service
* @details
*
* @author   gordon
* @date         2015-06-30
* @version  v0.1
*/

#include <stddef.h>

#ifndef __GATTSVC_DIS_H
#include <gattsvc_dis.h>
#endif


/**
* @brief        device information service
*
*
*/
const T_ATTRIB_APPL gatt_svc_dis[] =
{
    /** Primary Service */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),   /**< flags     */
        {                                                       /**<type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_DEVICE_INFORMATION_SERVICE),      /**< service UUID */
            HI_WORD(GATT_UUID_DEVICE_INFORMATION_SERVICE)
        },
        UUID_16BIT_SIZE,                            /**< bValueLen     */
        NULL,                                       /**< p_value_context */
        GATT_PERM_READ                              /**<permissions  */
    },

    /** Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                       /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /**< permissions */
    },
    /** System ID String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHAR_SYSTEM_ID),
            HI_WORD(GATT_UUID_CHAR_SYSTEM_ID)
        },
        0,                                          /**< variable size */
        (void *)NULL,
        GATT_PERM_READ                              /**< permissions */
    },

    /** Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                       /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /**< permissions */
    },
    /** Manufacturer Name String characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHAR_MANUFACTURER_NAME),
            HI_WORD(GATT_UUID_CHAR_MANUFACTURER_NAME)
        },
        0,                                          /**< variable size */
        (void *)NULL,
        GATT_PERM_READ                              /**< permissions */
    },

    /** Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                       /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /**< permissions */
    },
    /** Model Number characteristic value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHAR_MODEL_NUMBER),
            HI_WORD(GATT_UUID_CHAR_MODEL_NUMBER)
        },
        0,                                          /**< variable size */
        (void *)NULL,
        GATT_PERM_READ                              /**< permissions */
    }
};

const int gatt_svc_dis_size        = sizeof(gatt_svc_dis);
const int gatt_svc_dis_nbr_of_attrib = sizeof(gatt_svc_dis) / sizeof(T_ATTRIB_APPL);

