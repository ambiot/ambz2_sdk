/**
********************************************************************************************************
Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file       Gattsvc_ndcs.c
* @brief     GATT built-in NDCS (Next DST (Daylight Saving Time) Change Service)
* @details
*
* @author       gordon
* @date         2015-07-09
* @version  v0.1
*/

#include <stddef.h>

#ifndef __GATTSVC_NDCS_H
#include <gattsvc_ndcs.h>
#endif

/**
 * @brief  service definition.
 *
 *   Next DST (Daylight Saving Time) Change Service
 */
const T_ATTRIB_APPL gatt_svc_ndcs[] =
{
    /** Primary Service */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /**< flags     */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_NEXT_DST_CHANGE),     /**< service UUID */
            HI_WORD(GATT_UUID_NEXT_DST_CHANGE)
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
            GATT_CHAR_PROP_READ                     /**< characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /**< permissions */
    },
    /** Time with DST value */
    {
        ATTRIB_FLAG_VALUE_APPL,                     /**< flags */
        {                                           /**< type_value */
            LO_WORD(GATT_UUID_CHAR_NDCS_TIME_WITH_DST),
            HI_WORD(GATT_UUID_CHAR_NDCS_TIME_WITH_DST)
        },
        8,                                          /**< bValueLen */
        NULL,
        GATT_PERM_READ                              /**< permissions */
    }
};

const int gatt_svc_ndcs_size        = sizeof(gatt_svc_ndcs);
const int gatt_svc_ndcs_nbr_of_attrib = sizeof(gatt_svc_ndcs) / sizeof(T_ATTRIB_APPL);

