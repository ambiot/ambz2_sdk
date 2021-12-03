/**
********************************************************************************************************
Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file       Gattsvc_bas.c
* @brief     GATT built-in BAS (Battery Service)
* @details
*
* @author       gordon
* @date         2015-07-09
* @version  v0.1
*/

#include <stddef.h>

#ifndef __GATTSVC_BAS_H
#include <gattsvc_bas.h>
#endif


/**
 * @brief  service definition.
 *
 *  Battery Service
 */
const T_ATTRIB_APPL gatt_svc_bas[] =
{
    /** primary Service */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),   /**< flags  */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_BATTERY),                 /**< service UUID */
            HI_WORD(GATT_UUID_BATTERY)
        },
        UUID_16BIT_SIZE,                                /**< bValueLen     */
        NULL,                                           /**< ValueContext */
        GATT_PERM_READ                                  /**< permissions  */
    },

    /**  Characteristic  */
    {
        ATTRIB_FLAG_VALUE_INCL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ |                      /**< characteristic properties */
             GATT_CHAR_PROP_NOTIFY)
        },
        1,                                              /**< bValueLen */
        NULL,
        GATT_PERM_READ                                  /**< permissions */
    },
    /** Battery Level value */
    {
        ATTRIB_FLAG_VALUE_APPL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHAR_BAS_LEVEL),
            HI_WORD(GATT_UUID_CHAR_BAS_LEVEL)
        },
        1,                                              /**< bValueLen */
        NULL,
        GATT_PERM_READ                                  /**< permissions */
    },
    /** client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT),   /**< client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                              /**< bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)              /**< permissions */
    }
};

const int gatt_svc_bas_size        = sizeof(gatt_svc_bas);
const int gatt_svc_bas_nbr_of_attrib = sizeof(gatt_svc_bas) / sizeof(T_ATTRIB_APPL);

