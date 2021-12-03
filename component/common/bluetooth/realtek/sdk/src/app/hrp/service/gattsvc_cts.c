/**
********************************************************************************************************
Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file       Gattsvc_cts.c
* @brief     GATT built-in GLS (Current Time Service)
* @details
*
* @author       gordon
* @date         2015-07-09
* @version  v0.1
*/

#include <stddef.h>

#ifndef __GATTSVC_CTS_H
#include <gattsvc_cts.h>
#endif

/** CTS Current Time Service */
#define GATT_UUID_CURRENT_TIME                 0x1805

#define GATT_UUID_CHAR_CTS_CURRENT_TIME        0x2A2B
#define GATT_UUID_CHAR_CTS_LOCAL_TIME_INFO     0x2A0F
#define GATT_UUID_CHAR_CTS_REF_TIME_INFO       0x2A14
/**
 * @brief  service definition.
 *
 *  Current Time Service
 */
const T_ATTRIB_APPL gatt_svc_cts[] =
{
    /** Primary Service */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),      /**< flags     */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_CURRENT_TIME),            /**< service UUID */
            HI_WORD(GATT_UUID_CURRENT_TIME)
        },
        UUID_16BIT_SIZE,                                /**< bValueLen     */
        NULL,                                           /**< p_value_context */
        GATT_PERM_READ                                  /**< permissions  */
    },

    /**Characteristic>>*/
    {
        ATTRIB_FLAG_VALUE_INCL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            (GATT_CHAR_PROP_READ |                      /**< characteristic properties */
             GATT_CHAR_PROP_NOTIFY)
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                              /**< bValueLen */
        NULL,
        GATT_PERM_READ                                  /**< permissions */
    },
    /** Current Time value */
    {
        ATTRIB_FLAG_VALUE_APPL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHAR_CTS_CURRENT_TIME),
            HI_WORD(GATT_UUID_CHAR_CTS_CURRENT_TIME)
        },
        10,                                             /**< bValueLen */
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
    },

    /** Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                         /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                              /**< bValueLen */
        NULL,
        GATT_PERM_READ                                  /**< permissions */
    },
    /** Local Time Information value */
    {
        ATTRIB_FLAG_VALUE_APPL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHAR_CTS_LOCAL_TIME_INFO),
            HI_WORD(GATT_UUID_CHAR_CTS_LOCAL_TIME_INFO)
        },
        2,                                              /**< bValueLen */
        NULL,
        GATT_PERM_READ                                  /**< permissions */
    },

    /** Characteristic*/
    {
        ATTRIB_FLAG_VALUE_INCL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                         /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                              /**< bValueLen */
        NULL,
        GATT_PERM_READ                                  /**< permissions */
    },
    /** Reference Time Information value */
    {
        ATTRIB_FLAG_VALUE_APPL,                         /**< flags */
        {                                            /**< type_value */
            LO_WORD(GATT_UUID_CHAR_CTS_REF_TIME_INFO),
            HI_WORD(GATT_UUID_CHAR_CTS_REF_TIME_INFO)
        },
        4,                                              /**< bValueLen */
        NULL,
        GATT_PERM_READ                                  /**< permissions */
    }
};

const int gatt_svc_cts_size        = sizeof(gatt_svc_cts);
const int gatt_svc_cts_nbr_of_attrib = sizeof(gatt_svc_cts) / sizeof(T_ATTRIB_APPL);

