/* find me service for ltp to register service */
#include "hrp.h"
#include "gatt.h"

uint8_t cha_val_v8_011[1] = {0x08};
uint8_t cha_val_v8_d11[1]  = {0x01};
uint8_t cha_val_v8_d21[1]  = {0x02};
uint8_t cha_val_v8_d31[1]  = {0x03};

const T_ATTRIB_APPL gatt_dfindme_profile[] =
{
    /*----------handle = 0x0008  {Service=0x1801 ("Attribute Profile")}---*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(0x1801),                         /* service UUID */
            HI_WORD(0x1801)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },

    /* +++++++++++ handle = 0x0009 Characteristic -- Service Changed +++++++++++++  */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_INDICATE                /* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },


    /* handle = 0x000a Characteristic value  -- Service Changed  */
    {
        ATTRIB_FLAG_VALUE_APPL,                         /* flags */
        {                                               /* type_value */
            LO_WORD(0x2A05),
            HI_WORD(0x2A05),
            0x01,
            0x00,
            0xff,
            0xff
        },
        4,                                              /* bValueLen */
        NULL,
        GATT_PERM_NOTIF_IND                             /* permissions */
    },


    /* handle = 0x000b Characteristic value  -- Client Characteristic Configuration  */

    {
        (ATTRIB_FLAG_VALUE_INCL |  ATTRIB_FLAG_CCCD_APPL),                           /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                           /* bValueLen */
        NULL,
        GATT_PERM_READ | GATT_PERM_WRITE           /* permissions */
    },

    /*----------------- handle = 0x000c {{Service=0xA00B ("Service B.5")} -------------------*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(0xA00B),                         /* service UUID */
            HI_WORD(0xA00B)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },

    /* +++++++++++ handle = 0x000d Characteristic -- Value V8 +++++++++++++  */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE /* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ              /* permissions */
    },


    /* handle = 0x000e Characteristic value  -- Value V8 */
    {
        ATTRIB_FLAG_VOID,                         /* flags */
        {                                           /* type_value */
            LO_WORD(0xB008),
            HI_WORD(0xB008),
        },
        1,                        /* bValueLen */
        (void *)cha_val_v8_011,
        GATT_PERM_READ | GATT_PERM_WRITE                       /* permissions */
    },


    /* +++++++++++ handle = 0x000f Descriptor --Descriptor V5D1  +++++++++++++  */
    {
        ATTRIB_FLAG_VOID,                     /* flags */
        {
            LO_WORD(0xB015),
            HI_WORD(0xB015),
        },
        1,                                           /* bValueLen */
        (void *)cha_val_v8_d11,
        GATT_PERM_READ | GATT_PERM_WRITE     /* permissions */
    },

    /* +++++++++++ handle = 0x0010 Descriptor --Descriptor V5D2  +++++++++++++  */
    {
        ATTRIB_FLAG_VOID,                    /* flags */
        {
            LO_WORD(0xB016),
            HI_WORD(0xB016),
        },
        1,                                           /* bValueLen */
        (void *)cha_val_v8_d21,
        GATT_PERM_READ    /* permissions */
    },

    /* +++++++++++ handle = 0x0011 Descriptor --Descriptor V5D3  +++++++++++++  */
    {
        ATTRIB_FLAG_VOID,                     /* flags */
        {
            LO_WORD(0xB017),
            HI_WORD(0xB017),
        },
        1,                                           /* bValueLen */
        (void *)cha_val_v8_d31,
        GATT_PERM_WRITE   /* permissions */
    },


    /*----------------- handle = 0x0012 {Service = 0x180f (Battery Service)}-------------------*/
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),  /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(0x180F),                         /* service UUID */
            HI_WORD(0x180F)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },

    /* +++++++++++ handle = 0x0013 Characteristic -- Value V8 +++++++++++++  */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_NOTIFY | GATT_CHAR_PROP_READ | GATT_CHAR_PROP_WRITE/* characteristic properties */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ              /* permissions */
    },

    /* handle = 0x0014 Characteristic value  -- Value V8 */
    {
        ATTRIB_FLAG_VOID | ATTRIB_FLAG_VALUE_APPL,      /* flags */
        {                                               /* type_value */
            LO_WORD(0x2A19),
            HI_WORD(0x2A19),
        },
        0,                                                 /* bValueLen */
        NULL,
        GATT_PERM_READ | GATT_PERM_WRITE       /* permissions */
        //lorna added GATT_PERM_WRITE for test BTIF_MSG_GATT_ATTR_WRITE_REQ_IND/CFM
    },

    /* handle = 0x0015  Characteristic value  client Characteristic configuration*/
    {
        (ATTRIB_FLAG_VALUE_INCL |  ATTRIB_FLAG_CCCD_APPL),                           /* flags */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                           /* bValueLen */
        NULL,
        GATT_PERM_READ | GATT_PERM_WRITE           /* permissions */
    },

};

const int gatt_dfindme_profile_size = sizeof(gatt_dfindme_profile);
const int gatt_svc_findme_nbr_of_attrib = sizeof(gatt_dfindme_profile) / sizeof(T_ATTRIB_APPL);

