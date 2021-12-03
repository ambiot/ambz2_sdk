/**
********************************************************************************************************
Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file       Gattsvc_rtus.c
* @brief     GATT built-in RTUS (Reference Time Update Service)
* @details
*
* @author       gordon
* @date         2015-07-09
* @version  v0.1
*/

#include <stddef.h>
#include <gattsvc_rtus.h>

/**
 * @brief  service definition.
 *
 *   Reference Time Update Service
 */
const T_ATTRIB_APPL gatt_svc_rtus[] =
{
    /** Primary Service*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /**< flags     */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_REFERENCE_TIME_UPDATE), /**< service UUID */
            HI_WORD(GATT_UUID_REFERENCE_TIME_UPDATE)
        },
        UUID_16BIT_SIZE,                            /**< bValueLen     */
        NULL,                                       /**< p_value_context */
        GATT_PERM_READ                              /**< permissions  */
    },

    /** Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE_NO_RSP             /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /**< permissions */
    },
    /** Time Update Control Point value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHAR_RTUS_CONTROL_POINT),
            HI_WORD(GATT_UUID_CHAR_RTUS_CONTROL_POINT)
        },
        1,                                          /**< bValueLen */
        NULL,
        GATT_PERM_WRITE                             /**< permissions */
    },

    /** Characteristic */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ                     /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /**< permissions */
    },
    /** Time Update State value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHAR_RTUS_STATE),
            HI_WORD(GATT_UUID_CHAR_RTUS_STATE)
        },
        2,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    }
};

const int gatt_svc_rtus_size        = sizeof(gatt_svc_rtus);
const int gatt_svc_rtus_nbr_of_attrib = sizeof(gatt_svc_rtus) / sizeof(T_ATTRIB_APPL);

