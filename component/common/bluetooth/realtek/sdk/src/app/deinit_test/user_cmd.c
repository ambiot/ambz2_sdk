/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      user_cmd.c
   * @brief     User defined test commands.
   * @details  User command interfaces.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <string.h>
#include <os_mem.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_le.h>
#include <gap_conn_le.h>
#include <gap_bond_le.h>
#include <gap_adv.h>
#include <gap_scan.h>
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
#include <gap_vendor.h>
#endif
#if F_BT_LE_GATT_CLIENT_SUPPORT
#include "profile_client.h"
#include <gaps_client.h>
#include <complete_ble_client.h>
#endif
#include <gatt_builtin_services.h>
#include <complete_ble_service.h>
#include <user_cmd.h>
#include <link_mgr.h>
#include <gap_test_app.h>
#include <app_task.h>
#if F_BT_ANCS_CLIENT_SUPPORT
#include <ancs_client.h>
#include <ancs.h>
#endif
#if F_BT_LE_PRIVACY_SUPPORT
#include <gap_privacy.h>
#include <privacy_mgnt.h>
#endif
#if F_BT_DLPS_EN
#include <dlps.h>
#include <data_uart_dlps.h>
#endif
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
#include <gap_ext_scan.h>
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
#include <gap_ext_adv.h>
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
#include <gap_credit_based_conn.h>
#endif
#if  APP_HID_TEST
#include <hids_kb.h>
#endif
#if F_BT_LE_6_0_AOA_AOD_SUPPORT
#include <gap_aox.h>
#endif
/*common*/
#if  F_BT_UPPER_STACK_USE_VIRTUAL_HCI
//#include <rtl876x.h>
#endif

#ifdef  AMEBAD_BOARD
#include "ameba_soc.h"
#endif
#if  APP_HID_TEST
#include <hids_kb.h>
#endif
#include <test_task.h>
#include <uart_task.h>

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @brief User command interface data, used to parse the commands from Data UART. */
T_USER_CMD_IF    user_cmd_if;

#if F_BT_LE_5_0_AE_ADV_SUPPORT
// GAP - SCAN RSP data (max size = 31 bytes)
static const uint8_t ext_scan_response[] =
{
    0x03,
    GAP_ADTYPE_16BIT_COMPLETE,
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static const uint8_t ext_adv_data[] =
{
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    0x03,
    GAP_ADTYPE_16BIT_COMPLETE,
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),

    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_WORD(GAP_GATT_APPEARANCE_UNKNOWN),
    HI_WORD(GAP_GATT_APPEARANCE_UNKNOWN),

    0x0C,
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'B', '3', '_', 'G', 'a', 'p', 'T', 'e', 's', 't'
};

static const uint8_t ext_large_adv_data[] =
{
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4
};

/* Reduce length of ext_large_scan_data to 247 for testing two advertising sets simultaneously.

   Because another advertising set of connectable and scannable undirected advertisement with
   scan response data and advertising data will use three adv tasks. So only three adv tasks could
   be used by scannable undirected advertisement with extended advertising PDUs. */
static const uint8_t ext_large_scan_data[] =
{
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 7
};
#if 0
, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1,
5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1
};
#endif
#endif

/*============================================================================*
 *                              Functions
 *============================================================================*/

#if APP_HID_TEST
typedef enum
{
    /****Keyboard*******/
    NO_EVENT                   = 0x00,
    KB_ErrorRollOver           = 0x01,
    KB_POSTFail                = 0x02,
    KB_ErrorUndefined          = 0x03,
    KB_a_A                     = 0x04,
    KB_b_B                     = 0x05,
    KB_c_C                     = 0x06,
    KB_d_D                     = 0x07,
    KB_e_E                     = 0x08,
    KB_f_F                     = 0x09,
    KB_g_G                     = 0x0A,
    KB_h_H                     = 0x0B,
    KB_i_I                     = 0x0C,
    KB_j_J                     = 0x0D,
    KB_k_K                     = 0x0E,
    KB_l_L                     = 0x0F,
    KB_m_M                     = 0x10,
    KB_n_N                     = 0x11,
    KB_o_O                     = 0x12,
    KB_p_P                     = 0x13,
    KB_q_Q                     = 0x14,
    KB_r_R                     = 0x15,
    KB_s_S                     = 0x16,
    KB_t_T                     = 0x17,
    KB_u_U                     = 0x18,
    KB_v_V                     = 0x19,
    KB_w_W                     = 0x1A,
    KB_x_X                     = 0x1B,
    KB_y_Y                     = 0x1C,
    KB_z_Z                     = 0x1D,
    KB_1                       = 0x1E,
    KB_2                       = 0x1F,
    KB_3                       = 0x20,
    KB_4                       = 0x21,
    KB_5                       = 0x22,
    KB_6                       = 0x23,
    KB_7                       = 0x24,
    KB_8                       = 0x25,
    KB_9                       = 0x26,
    KB_0                       = 0x27,
    KB_Return_ENTER            = 0x28,
    KB_ESCAPE                  = 0x29,
    KB_DELETE_Backspace        = 0x2A,
    KB_Tab                     = 0x2B,
    KB_Spacebar                = 0x2C,
    KB_Minus_Underscore        = 0x2D,
    KB_Equal_Plus              = 0x2E,
    KB_LeftBracket_LeftBrace   = 0x2F,
    KB_RightBracket_RightBrace = 0x30,
    KB_BackSlash_VerticalBar   = 0x31,
    KB_NON_US_Pound            = 0x32,
    KB_Semicolon_Colon         = 0x33,
    KB_QuotationMark           = 0x34,
    KB_GraveAccent_Tilde       = 0x35,
    KB_Comma_LessThan          = 0x36,
    KB_Dot_LargerThan          = 0x37,
    KB_Slash_QuestionMark      = 0x38,
    KB_CapsLock                = 0x39,
    KB_F1                      = 0x3A,
    KB_F2                      = 0x3B,
    KB_F3                      = 0x3C,
    KB_F4                      = 0x3D,
    KB_F5                      = 0x3E,
    KB_F6                      = 0x3F,
    KB_F7                      = 0x40,
    KB_F8                      = 0x41,
    KB_F9                      = 0x42,
    KB_F10                     = 0x43,
    KB_F11                     = 0x44,
    KB_F12                     = 0x45,
    KB_PrintScreen             = 0x46,
    KB_ScrollLock              = 0x47,
    KB_Pause                   = 0x48,
    KB_Insert                  = 0x49,
    KB_Home                    = 0x4A,
    KB_PageUp                  = 0x4B,
    KB_DeleteForward           = 0x4C,
    KB_End                     = 0x4D,
    KB_PageDown                = 0x4E,
    KB_RightArrow              = 0x4F,
    KB_LeftArrow               = 0x50,
    KB_DownArrow               = 0x51,
    KB_UpArrow                 = 0x52,

    /****Keypad*******/
    KP_NumLock_Clear           = 0x53,
    KP_Divide                  = 0x54,
    KP_Multiply                = 0x55,
    KP_Minus                   = 0x56,
    KP_Plus                    = 0x57,
    KP_Enter                   = 0x58,
    KP_1_End                   = 0x59,
    KP_2_DownArrow             = 0x5A,
    KP_3_PageDn                = 0x5B,
    KP_4_LeftArrow             = 0x5C,
    KP_5                       = 0x5D,
    KP_6_RightArrow            = 0x5E,
    KP_7_Home                  = 0x5F,
    KP_8_UpArrow               = 0x60,
    KP_9_PageUp                = 0x61,
    KP_0_Insert                = 0x62,
    KP_Dot_Delete              = 0x63,

    /****Keyboard*******/
    KB_NON_US_BackSlash        = 0x64,
    KB_Application             = 0x65,
    KB_Power                   = 0x66,

    /****Keypad*******/
    KP_Equal                   = 0x67,

    /****Keyboard*******/
    KB_F13                     = 0x68,
    KB_F14                     = 0x69,
    KB_F15                     = 0x6A,
    KB_F16                     = 0x6B,
    KB_F17                     = 0x6C,
    KB_F18                     = 0x6D,
    KB_F19                     = 0x6E,
    KB_F20                     = 0x6F,
    KB_F21                     = 0x70,
    KB_F22                     = 0x71,
    KB_F23                     = 0x72,
    KB_F24                     = 0x73,
    KB_Execute                 = 0x74,
    KB_Help                    = 0x75,
    KB_Menu                    = 0x76,
    KB_Select                  = 0x77,
    KB_Stop                    = 0x78,
    KB_Again                   = 0x79,
    KB_Undo                    = 0x7A,
    KB_Cut                     = 0x7B,
    KB_Copy                    = 0x7C,
    KB_Paste                   = 0x7D,
    KB_Find                    = 0x7E,
    KB_Mute                    = 0x7F,
    KB_VolumeUp                = 0x80,
    KB_VolumeDown              = 0x81,
    KB_LockingCapsLock         = 0x82,
    KB_LockingNumLock          = 0x83,
    KB_LockingScrollLock       = 0x84,
    KB_Comma                   = 0x85,
    KB_EqualSign               = 0x86,
    KB_International1          = 0x87,
    KB_International2          = 0x88,
    KB_International3          = 0x89,
    KB_International4          = 0x8A,
    KB_International5          = 0x8B,
    KB_International6          = 0x8C,
    KB_International7          = 0x8D,
    KB_International8          = 0x8E,
    KB_International9          = 0x8F,
    KB_LANG1                   = 0x90,
    KB_LANG2                   = 0x91,
    KB_LANG3                   = 0x92,
    KB_LANG4                   = 0x93,
    KB_LANG5                   = 0x94,
    KB_LANG6                   = 0x95,
    KB_LANG7                   = 0x96,
    KB_LANG8                   = 0x97,
    KB_LANG9                   = 0x98,
    KB_AlternateErase          = 0x99,
    KB_SysReq_Attention        = 0x9A,
    KB_Cancel                  = 0x9B,
    KB_Clear                   = 0x9C,
    KB_Prior                   = 0x9D,
    KB_Return                  = 0x9E,
    KB_Separator               = 0x9F,
    KB_Out                     = 0xA0,
    KB_Oper                    = 0xA1,
    KB_Clear_Again             = 0xA2,
    KB_CrSel_Props             = 0xA3,
    KB_ExSel                   = 0xA4,

    KB_LeftControl             = 0xE0,
    KB_LeftShift               = 0xE1,
    KB_LeftAlt                 = 0xE2,
    KB_LeftGUI                 = 0xE3,
    KB_RightControl            = 0xE4,
    KB_RightShift              = 0xE5,
    KB_RightAlt                = 0xE6,
    KB_RightGUI                = 0xE7,

} KEY_USAGE_ID;

static T_USER_CMD_PARSE_RESULT cmd_key(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t keycount = 0;
    uint16_t usage_id = p_parse_value->dw_param[1];
    uint8_t keyboard_data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t keyboard_release[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    /********Normal Keys**********/
    if (usage_id <= 0xA4)
    {
        if (keycount < 6)
        {
            keyboard_data[2 + keycount] = usage_id;
            keycount++;
        }
    }
    /*********Modifier Keys********/
    else if ((usage_id >= 0xE0) && (usage_id <= 0xE7))
    {
        keyboard_data[0] |= 1 << (usage_id & 0x07);
    }

    server_send_data(conn_id, hid_srv_id, GATT_SVC_HID_REPORT_INPUT_INDEX,
                     keyboard_data, sizeof(keyboard_data), GATT_PDU_TYPE_NOTIFICATION);
    server_send_data(conn_id, hid_srv_id, GATT_SVC_HID_REPORT_INPUT_INDEX,
                     keyboard_release, sizeof(keyboard_release), GATT_PDU_TYPE_NOTIFICATION);
    return (RESULT_SUCESS);
}

#endif

static T_USER_CMD_PARSE_RESULT cmd_reset(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#ifdef AMEBAD_BOARD
    u32 Temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_BOOT_CFG);
    Temp |= BIT_SOC_BOOT_PATCH_KM4_RUN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_BOOT_CFG, Temp);
    NVIC_SystemReset();
#endif
    return (RESULT_SUCESS);
}
static T_USER_CMD_PARSE_RESULT cmd_vendor(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t param[2];
    param[0] = 3;
    param[1] = 1;

    gap_vendor_cmd_req(0xFD83, 2, param);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_reg(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    gap_test_case = (T_GAP_TEST_CASE)p_parse_value->dw_param[0];
    switch (gap_test_case)
    {
    case GAP_TC_00_NORMAL:   //normal mode
        break;

#if F_BT_ANCS_CLIENT_SUPPORT
    case GAP_TC_01_ANCS:   //test ancs
        {
            uint8_t link_num;
            uint8_t auth_sec_req_enable = true;
            le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &auth_sec_req_enable);
            link_num = le_get_max_link_num();
            ancs_init(link_num);
        }
        break;
#endif
#if F_BT_GAPS_CHAR_WRITEABLE
    case GAP_TC_02_GAPS_WRITEABLE:   //test gap service write
        {
            uint8_t appearance_prop = GAPS_PROPERTY_WRITE_ENABLE;
            uint8_t device_name_prop = GAPS_PROPERTY_WRITE_ENABLE;
            T_LOCAL_APPEARANCE appearance_local;
            T_LOCAL_NAME local_device_name;
            if (flash_load_local_appearance(&appearance_local) == 0)
            {
                gaps_set_parameter(GAPS_PARAM_APPEARANCE, sizeof(uint16_t), &appearance_local.local_appearance);
            }

            if (flash_load_local_name(&local_device_name) == 0)
            {
                gaps_set_parameter(GAPS_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, local_device_name.local_name);
            }
            gaps_set_parameter(GAPS_PARAM_APPEARANCE_PROPERTY, sizeof(appearance_prop), &appearance_prop);
            gaps_set_parameter(GAPS_PARAM_DEVICE_NAME_PROPERTY, sizeof(device_name_prop), &device_name_prop);
            gatt_register_callback((void *)gap_service_callback);
        }
        break;
#endif

#if F_BT_LE_PRIVACY_SUPPORT
    case GAP_TC_03_PRIVACY://privacy
        {
            uint8_t auth_sec_req_enable = true;
            uint8_t irk_auto = true;
            uint16_t privacy_timeout = 0x384;
            uint8_t central_address_resolution = 1;
            if (p_parse_value->param_count == 2)
            {
                privacy_timeout = p_parse_value->dw_param[1];
            }
            le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &auth_sec_req_enable);
            le_bond_set_param(GAP_PARAM_BOND_GEN_LOCAL_IRK_AUTO, sizeof(uint8_t), &irk_auto);
            le_privacy_set_param(GAP_PARAM_PRIVACY_TIMEOUT, sizeof(uint16_t), &privacy_timeout);
            le_privacy_register_cb(privacy_msg_callback);
            gaps_set_parameter(GAPS_PARAM_CENTRAL_ADDRESS_RESOLUTION, sizeof(central_address_resolution),
                               &central_address_resolution);
        }
        break;
#endif
    case GAP_TC_04_PIN_CODE:
        {
            uint32_t auth_fix_passkey = 123456; /* passkey "000000"*/
            uint8_t auth_use_fix_passkey = true;
            le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(uint32_t), &auth_fix_passkey);
            le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(uint8_t), &auth_use_fix_passkey);
        }
        break;
#if F_BT_LE_5_0_SUPPORT
    case GAP_TC_05_BT5:
        {
            T_GAP_LE_CONN_REQ_PARAM conn_req_param;
            uint8_t  use_extended = true;
            uint8_t  phys_prefer = GAP_PHYS_PREFER_ALL;
            uint8_t  tx_prefer = GAP_PHYS_PREFER_1M_BIT | GAP_PHYS_PREFER_2M_BIT |
                                 GAP_PHYS_PREFER_CODED_BIT;
            uint8_t  rx_prefer = GAP_PHYS_PREFER_1M_BIT | GAP_PHYS_PREFER_CODED_BIT |
                                 GAP_PHYS_PREFER_2M_BIT;
#if F_BT_LE_5_0_AE_ADV_SUPPORT
            le_set_gap_param(GAP_PARAM_USE_EXTENDED_ADV, sizeof(use_extended), &use_extended);
#endif
            le_set_gap_param(GAP_PARAM_DEFAULT_PHYS_PREFER, sizeof(phys_prefer), &phys_prefer);
            le_set_gap_param(GAP_PARAM_DEFAULT_TX_PHYS_PREFER, sizeof(tx_prefer), &tx_prefer);
            le_set_gap_param(GAP_PARAM_DEFAULT_RX_PHYS_PREFER, sizeof(rx_prefer), &rx_prefer);
            conn_req_param.scan_interval = 0x20;
            conn_req_param.scan_window = 0x20;
            conn_req_param.conn_interval_min = 80;
            conn_req_param.conn_interval_max = 80;
            conn_req_param.conn_latency = 0;
            conn_req_param.supv_tout = 1000;
            conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
            conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
            le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
            conn_req_param.conn_latency = 1;
            le_set_conn_param(GAP_CONN_PARAM_2M, &conn_req_param);
            conn_req_param.conn_latency = 2;
            le_set_conn_param(GAP_CONN_PARAM_CODED, &conn_req_param);

            {
                uint8_t ext_scan_filter_policy = GAP_SCAN_FILTER_ANY;
                uint8_t ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
                uint8_t scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT | GAP_EXT_SCAN_PHYS_CODED_BIT;

                T_GAP_LE_EXT_SCAN_PARAM extended_scan_param[GAP_EXT_SCAN_MAX_PHYS_NUM];
                extended_scan_param[0].scan_type = GAP_SCAN_MODE_ACTIVE;
                extended_scan_param[0].scan_interval = 400;
                extended_scan_param[0].scan_window = 200;

                extended_scan_param[1].scan_type = GAP_SCAN_MODE_ACTIVE;
                extended_scan_param[1].scan_interval = 440;
                extended_scan_param[1].scan_window = 220;
                le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(ext_scan_filter_policy),
                                      &ext_scan_filter_policy);
                le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(ext_scan_filter_duplicate),
                                      &ext_scan_filter_duplicate);
                le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                                      &scan_phys);
                le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_1M, &extended_scan_param[0]);
                le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_CODED, &extended_scan_param[1]);
            }
#endif
        }
        break;
#endif
    case GAP_TC_07_CCCD:
        {
            uint8_t auth_sec_req_enable = true;
            le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &auth_sec_req_enable);
            simp_ble_service_add_service((void *)app_profile_callback);
            simp_ble_service_add_service((void *)app_profile_callback);
            simp_ble_service_add_service((void *)app_profile_callback);
            simp_ble_service_add_service((void *)app_profile_callback);
            simp_ble_service_add_service((void *)app_profile_callback);
            simp_ble_service_add_service((void *)app_profile_callback);
        }
        break;
#if F_BT_LE_LOCAL_IRK_SETTING_SUPPORT
    case GAP_TC_08_LOCAL_IRK:
        {
            T_LOCAL_IRK le_local_irk = {0, 1, 0, 5, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 9};
            le_bond_set_param(GAP_PARAM_BOND_SET_LOCAL_IRK, GAP_KEY_LEN, le_local_irk.local_irk);
        }
        break;
#endif
    case GAP_TC_09_SLAVE_LATENCY://test slave latency
        break;

#if F_BT_LE_4_1_CBC_SUPPORT
    case GAP_TC_10_BT41:
        {
            uint8_t chann_num = 1;
            if (p_parse_value->param_count > 1)
            {
                chann_num = p_parse_value->dw_param[1];
            }
            le_cbc_init(chann_num);
            le_cbc_register_app_cb(app_credit_based_conn_callback);
        }
        break;
#endif
#if F_BT_LE_GAP_MSG_INFO_WAY
    case GAP_TC_12_GAP_MSG_CALLBACK:
        {
            le_gap_msg_info_way(false);
        }
        break;
#endif
    case GAP_TC_13_V3_V8_TX:
        break;

    case GAP_TC_14_SRV_CHANGE:   //test gap service write
        {
            uint8_t auth_sec_req_enable = true;
            le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(uint8_t), &auth_sec_req_enable);
            //gatt_register_callback((void *)gap_service_callback);
        }
        break;

#if APP_HID_TEST
    case GAP_TC_15_HID_PROFILE:   //test gap service write
        {
            hid_srv_id = hids_add_service((void *)app_profile_callback);
        }
        break;
#endif

    default:
        break;
    }

    /* Register BT Stack. */
    if (gap_start_bt_stack(evt_queue_handle, io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE))
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_dlps(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if F_BT_DLPS_EN
    if (p_parse_value->dw_param[0] == 0)
    {
        lps_mode_pause();
        data_uart_print("Active Mode\r\n");
    }
    else
    {
        lps_mode_resume();
        data_uart_can_enter_dlps(true);
        data_uart_print("LPS Mode\r\n");
    }
#endif
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_tracelevel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    if (p_parse_value->dw_param[0] == 1)
    {
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 1);
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 1);
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_WARN, 1);
        data_uart_print("Log On Sucess\r\n");
    }
    else if (p_parse_value->dw_param[0] == 0)
    {
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 0);
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 0);
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_WARN, 0);
        data_uart_print("Log Off Sucess\r\n");
    }
    else
    {
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 0);
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 0);
        log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_WARN, 0);
        log_module_trace_set(TRACE_MODULE_GAP, TRACE_LEVEL_INFO, true);
        log_module_trace_set(TRACE_MODULE_APP, TRACE_LEVEL_INFO, true);
        data_uart_print("Log Test Mode\r\n");
    }
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_setlocaltype(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t local_bd_type = p_parse_value->dw_param[0];
    T_GAP_RAND_ADDR_TYPE rand_addr = (T_GAP_RAND_ADDR_TYPE)p_parse_value->dw_param[1];
    uint8_t random_addr[6] = {0};
    random_addr[0] = 0x4E;
    random_addr[1] = 0xEF;
    random_addr[2] = 0x3B;
    random_addr[3] = 0x58;
    random_addr[4] = 0xFA;
    random_addr[5] = 0xFF;
#if F_BT_LE_PRIVACY_SUPPORT
    if (local_bd_type > GAP_LOCAL_ADDR_LE_RAP_OR_RAND)
#else
    if (local_bd_type > GAP_LOCAL_ADDR_LE_RANDOM)
#endif
    {
        return RESULT_CMD_ERR_PARAM;
    }

    if (local_bd_type == GAP_LOCAL_ADDR_LE_RANDOM)
    {
        if (rand_addr != GAP_RAND_ADDR_STATIC)
        {
            le_gen_rand_addr((T_GAP_RAND_ADDR_TYPE)p_parse_value->dw_param[1], random_addr);
        }
        le_set_rand_addr(random_addr);
        if (rand_addr == GAP_RAND_ADDR_STATIC)
        {
            le_cfg_local_identity_address(random_addr, GAP_IDENT_ADDR_RAND);
        }
        data_uart_print("set local random addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                        random_addr[5],
                        random_addr[4],
                        random_addr[3],
                        random_addr[2],
                        random_addr[1],
                        random_addr[0]);
    }
#if F_BT_LE_PRIVACY_SUPPORT
    else if (local_bd_type == GAP_LOCAL_ADDR_LE_RAP_OR_RAND)
    {
        le_set_rand_addr(random_addr);
        le_cfg_local_identity_address(random_addr, GAP_IDENT_ADDR_RAND);
    }
#endif
    le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
    le_scan_set_param(GAP_PARAM_SCAN_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_setstatic(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t local_bd_type = GAP_LOCAL_ADDR_LE_RANDOM;
    uint8_t type = p_parse_value->dw_param[0];
    uint8_t random_addr[6] = {0};
    random_addr[0] = 0x4E;
    random_addr[1] = 0xEF;
    random_addr[2] = 0x3B;
    random_addr[3] = 0x58;
    random_addr[4] = 0xFA;
    random_addr[5] = 0xFF;

    if (type == 0)
    {
        random_addr[0] = 0x00;
    }
    else
    {
        random_addr[0] = 0x01;
    }
    data_uart_print("set local random addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    random_addr[5],
                    random_addr[4],
                    random_addr[3],
                    random_addr[2],
                    random_addr[1],
                    random_addr[0]);
    le_set_rand_addr(random_addr);
    le_cfg_local_identity_address(random_addr, GAP_IDENT_ADDR_RAND);

    le_adv_set_param(GAP_PARAM_ADV_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
    le_scan_set_param(GAP_PARAM_SCAN_LOCAL_ADDR_TYPE, sizeof(local_bd_type), &local_bd_type);
    return (RESULT_SUCESS);
}

#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_writedefaultdatalen(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint16_t tx_octets = p_parse_value->dw_param[0];
    uint16_t tx_time = p_parse_value->dw_param[1];

    cause = le_write_default_data_len(tx_octets, tx_time);
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
static T_USER_CMD_PARSE_RESULT cmd_rssiread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;
    cause = le_read_rssi(conn_id);

    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_txpwrset(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    return RESULT_SUCESS;
}

#if F_BT_LE_READ_CHANN_MAP
static T_USER_CMD_PARSE_RESULT cmd_readchanmap(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;

    cause = le_read_chann_map(conn_id);

    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
static T_USER_CMD_PARSE_RESULT cmd_showcon(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t link_num;
    uint8_t conn_id;
    T_GAP_CONN_INFO conn_info;
    link_num = le_get_max_link_num();
    for (conn_id = 0; conn_id < link_num; conn_id++)
    {
        if (le_get_conn_info(conn_id, &conn_info))
        {
            data_uart_print("ShowCon conn_id %d state 0x%x role %d\r\n", conn_id,
                            conn_info.conn_state, conn_info.role);
            data_uart_print("RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
                            conn_info.remote_bd[5], conn_info.remote_bd[4],
                            conn_info.remote_bd[3], conn_info.remote_bd[2],
                            conn_info.remote_bd[1], conn_info.remote_bd[0],
                            conn_info.remote_bd_type);
        }
    }
    data_uart_print("active link num %d,  idle link num %d\r\n",
                    le_get_active_link_num(), le_get_idle_link_num());
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_conupdreq(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  conn_id = p_parse_value->dw_param[0];
    uint16_t conn_interval_min = p_parse_value->dw_param[1];
    uint16_t conn_interval_max = p_parse_value->dw_param[2];
    uint16_t conn_latency = p_parse_value->dw_param[3];
    uint16_t supervision_timeout = p_parse_value->dw_param[4];


    cause = le_update_conn_param(conn_id,
                                 conn_interval_min,
                                 conn_interval_max,
                                 conn_latency,
                                 supervision_timeout,
                                 2 * (conn_interval_min - 1),
                                 2 * (conn_interval_max - 1)
                                );
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_disc(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;
    cause = le_disconnect(conn_id);
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_setdatalength(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint16_t tx_octets = p_parse_value->dw_param[1];
    uint16_t tx_time = p_parse_value->dw_param[2];

    cause = le_set_data_len(conn_id,
                            tx_octets,
                            tx_time);
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif

static T_USER_CMD_PARSE_RESULT cmd_wl(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    T_GAP_WHITE_LIST_OP op = (T_GAP_WHITE_LIST_OP)p_parse_value->dw_param[0];
    uint8_t conn_id = p_parse_value->dw_param[1];
    T_GAP_CONN_INFO conn_info;

    if (op == GAP_WHITE_LIST_OP_CLEAR)
    {
        cause = le_modify_white_list(GAP_WHITE_LIST_OP_CLEAR, NULL, GAP_REMOTE_ADDR_LE_PUBLIC);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }

    if (le_get_conn_info(conn_id, &conn_info))
    {
        cause = le_modify_white_list(op, conn_info.remote_bd,
                                     (T_GAP_REMOTE_ADDR_TYPE)conn_info.remote_bd_type);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_CMD_ERR_PARAM;
    }
}

static T_USER_CMD_PARSE_RESULT cmd_wldev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    T_GAP_WHITE_LIST_OP op = (T_GAP_WHITE_LIST_OP)p_parse_value->dw_param[0];
    uint8_t dev_idx = p_parse_value->dw_param[1];

    if (op == GAP_WHITE_LIST_OP_CLEAR)
    {
        cause = le_modify_white_list(GAP_WHITE_LIST_OP_CLEAR, NULL, GAP_REMOTE_ADDR_LE_PUBLIC);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }

    if (dev_idx < dev_list_count)
    {
        cause = le_modify_white_list(op, dev_list[dev_idx].bd_addr,
                                     (T_GAP_REMOTE_ADDR_TYPE)dev_list[dev_idx].bd_type);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_ERR;
    }
}

static T_USER_CMD_PARSE_RESULT cmd_authmode(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  oob_enable = false;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    if (p_parse_value->param_count > 0)
    {
        auth_flags = p_parse_value->dw_param[0];
        auth_sec_req_flags = p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        auth_io_cap = p_parse_value->dw_param[1];
    }
    if (p_parse_value->param_count > 2)
    {
        auth_sec_req_enable = p_parse_value->dw_param[2];
    }
    if (p_parse_value->param_count > 3)
    {
        oob_enable = p_parse_value->dw_param[3];
    }
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);
#endif
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);
    cause = gap_set_pairable_mode();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_setkeydis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t init_dist = p_parse_value->dw_param[0];
    uint8_t rsp_dist = p_parse_value->dw_param[1];
    le_bond_cfg_local_key_distribute(init_dist, rsp_dist);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_sauth(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;
    cause = le_bond_pair(conn_id);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_userconf(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CFM_CAUSE confirm = GAP_CFM_CAUSE_ACCEPT;
    T_GAP_CAUSE cause;
    if (p_parse_value->dw_param[1] == 0)
    {
        confirm = GAP_CFM_CAUSE_REJECT;
    }
    cause = le_bond_user_confirm(conn_id, confirm);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_authkey(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint32_t passcode = p_parse_value->dw_param[1];
    T_GAP_CAUSE cause;
    T_GAP_CFM_CAUSE confirm = GAP_CFM_CAUSE_ACCEPT;
    if (passcode > GAP_PASSCODE_MAX)
    {
        confirm = GAP_CFM_CAUSE_REJECT;
    }
    cause = le_bond_passkey_input_confirm(conn_id, passcode,
                                          confirm);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

#if F_BT_LE_SMP_OOB_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_oob(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t oob_data[GAP_OOB_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5};
    oob_data[0] = p_parse_value->dw_param[1];
    oob_data[3] = p_parse_value->dw_param[2];
    oob_data[7] = p_parse_value->dw_param[3];
    oob_data[15] = p_parse_value->dw_param[4];
    le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);

    cause = le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_keypress(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_KEYPRESS_NOTIF_TYPE notify_type = (T_GAP_KEYPRESS_NOTIF_TYPE)p_parse_value->dw_param[1];
    T_GAP_CAUSE cause;
    cause = le_bond_keypress_notify(conn_id, notify_type);
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
static T_USER_CMD_PARSE_RESULT cmd_bondclear(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if F_BT_GAP_KEY_MANAGER_SUPPORT
    le_bond_clear_all_keys();
#endif
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_bondinfo(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
#if F_BT_GAP_KEY_MANAGER_SUPPORT
    uint8_t i;
    T_LE_KEY_ENTRY *p_entry;
    for (i = 0; i < bond_storage_num; i++)
    {
        p_entry = le_find_key_entry_by_idx(i);
        if (p_entry != NULL)
        {
            data_uart_print("bond_dev[%d]: bd 0x%02x%02x%02x%02x%02x%02x, addr_type %d, flags 0x%x\r\n",
                            p_entry->idx,
                            p_entry->remote_bd.addr[5],
                            p_entry->remote_bd.addr[4],
                            p_entry->remote_bd.addr[3],
                            p_entry->remote_bd.addr[2],
                            p_entry->remote_bd.addr[1],
                            p_entry->remote_bd.addr[0],
                            p_entry->remote_bd.remote_bd_type,
                            p_entry->flags);
        }
    }
#endif
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_scan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

    if (p_parse_value->param_count > 0)
    {
        scan_filter_policy = p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        scan_filter_duplicate = p_parse_value->dw_param[1];
    }

    link_mgr_clear_device_list();
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                      &scan_filter_policy);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                      &scan_filter_duplicate);
    cause = le_scan_start();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_stopscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_scan_stop();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_showdev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t i;
    data_uart_print("Advertising and Scan response: filter uuid = 0xA00A dev list\r\n");
    for (i = 0; i < dev_list_count; i++)
    {
        data_uart_print("RemoteBd[%d] = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
                        i,
                        dev_list[i].bd_addr[5], dev_list[i].bd_addr[4],
                        dev_list[i].bd_addr[3], dev_list[i].bd_addr[2],
                        dev_list[i].bd_addr[1], dev_list[i].bd_addr[0],
                        dev_list[i].bd_type);
        if (dev_list[i].bd_type == GAP_REMOTE_ADDR_LE_RANDOM)
        {
            uint8_t addr = dev_list[i].bd_addr[5] & RANDOM_ADDR_MASK;
            if (addr == RANDOM_ADDR_MASK_STATIC)
            {
                data_uart_print("Static Random Addr\r\n");
            }
            else if (addr == RANDOM_ADDR_MASK_RESOLVABLE)
            {
                data_uart_print("Resolv Random Addr\r\n");
            }
            else if (addr == RANDOM_ADDR_MASK_NON_RESOLVABLE)
            {
                data_uart_print("Non-resolv Random Addr\r\n");
            }
            else
            {
                data_uart_print("Unknown Random Addr\r\n");
            }
        }
    }

    return (RESULT_SUCESS);
}

#if F_BT_LE_GAP_CENTRAL_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_con(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t addr[6] = {0};
    uint8_t addr_len;
    uint8_t addr_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    T_GAP_LE_CONN_REQ_PARAM conn_req_param;

    conn_req_param.scan_interval = 0x10;
    conn_req_param.scan_window = 0x10;
    conn_req_param.conn_interval_min = 80;
    conn_req_param.conn_interval_max = 80;
    conn_req_param.conn_latency = 0;
    conn_req_param.supv_tout = 1000;
    conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
    conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
    le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

    for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
    {
        addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1];
    }
    if (p_parse_value->param_count >= 7)
    {
        addr_type = p_parse_value->dw_param[6];
    }

    cause = le_connect(GAP_PHYS_CONN_INIT_1M_BIT, addr, (T_GAP_REMOTE_ADDR_TYPE)addr_type,
                       GAP_LOCAL_ADDR_LE_PUBLIC,
                       1000);

    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_condev(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t dev_idx = p_parse_value->dw_param[0];
    if (dev_idx < dev_list_count)
    {
        T_GAP_CAUSE cause;
        T_GAP_LE_CONN_REQ_PARAM conn_req_param;
        T_GAP_LOCAL_ADDR_TYPE local_addr_type = (T_GAP_LOCAL_ADDR_TYPE)p_parse_value->dw_param[1];
        uint8_t  init_phys = 0;
#if F_BT_LE_5_0_SUPPORT
        uint32_t mode = p_parse_value->dw_param[2];

        switch (mode)
        {
        case 0x000:
            init_phys = 0;
            break;
        case 0x001:
            init_phys = GAP_PHYS_CONN_INIT_1M_BIT;
            break;
        case 0x010:
            init_phys = GAP_PHYS_CONN_INIT_2M_BIT;
            break;
        case 0x011:
            init_phys = GAP_PHYS_CONN_INIT_2M_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
            break;
        case 0x100:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT;
            break;
        case 0x101:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT | GAP_PHYS_CONN_INIT_1M_BIT;
            break;
        case 0x110:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT | GAP_PHYS_CONN_INIT_2M_BIT;
            break;
        case 0x111:
            init_phys = GAP_PHYS_CONN_INIT_CODED_BIT |
                        GAP_PHYS_CONN_INIT_2M_BIT |
                        GAP_PHYS_CONN_INIT_1M_BIT;
            break;

        default:
            break;
        }
#endif

        conn_req_param.scan_interval = 0x10;
        conn_req_param.scan_window = 0x10;
        conn_req_param.conn_interval_min = 80;
        conn_req_param.conn_interval_max = 80;
        conn_req_param.conn_latency = 0;
        conn_req_param.supv_tout = 1000;
        conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
        conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
        cause = le_connect(init_phys, dev_list[dev_idx].bd_addr,
                           (T_GAP_REMOTE_ADDR_TYPE)dev_list[dev_idx].bd_type,
                           local_addr_type,
                           1000);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_ERR;
    }
}

static T_USER_CMD_PARSE_RESULT cmd_chanclassset(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t chan_map[5] = {0};
    uint8_t i;
    for (i = 0; i < 5; i++)
    {
        chan_map[i] = (uint8_t)p_parse_value->dw_param[i];
    }

    chan_map[4] = chan_map[4] & 0x1F;

    cause = le_set_host_chann_classif(chan_map);

    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
#if F_BT_LE_GATT_CLIENT_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_srvdis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;

    cause = client_all_primary_srv_discovery(conn_id, CLIENT_PROFILE_GENERAL_ID);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_gapdis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    bool ret = gaps_start_discovery(conn_id);
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_gapread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    T_GAPS_READ_TYPE read_type = (T_GAPS_READ_TYPE)p_parse_value->dw_param[1];
    bool ret = gaps_read(conn_id, read_type);
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_simpdis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    bool ret = simp_ble_client_start_discovery(conn_id);
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_simpread(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    /* Indicate which char to be read. */
    uint8_t conn_id = p_parse_value->dw_param[0];
    bool ret;
    T_SIMP_READ_TYPE read_type = (T_SIMP_READ_TYPE)p_parse_value->dw_param[1];
    /* Read by handle or UUID, 1--by UUID, 0--by handle. */
    uint8_t read_pattern = (uint8_t)p_parse_value->dw_param[2];

    if (read_pattern)
    {
        ret = simp_ble_client_read_by_uuid(conn_id, read_type);
    }
    else
    {
        ret = simp_ble_client_read_by_handle(conn_id, read_type);
    }

    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_simpcccd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    /* Indicate which char CCCD command. */
    bool ret = false;
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t type = p_parse_value->dw_param[1];
    /* Enable or disable, 1--enable, 0--disable. */
    uint16_t cccd_data = p_parse_value->dw_param[2];

    switch (type)
    {
    case 0:/* V3 Notify char notif enable/disable. */
        ret = simp_ble_client_set_v3_notify(conn_id, cccd_data);
        break;
    case 1:/* V4 Indicate char indicate enable/disable. */
        ret = simp_ble_client_set_v4_ind(conn_id, cccd_data);
        break;
    case 2:
        ret = simp_ble_client_set_v8_cccd(conn_id, cccd_data);
        break;
    default:
        break;
    }
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_simpwrite(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret = false;
    uint8_t conn_id = p_parse_value->dw_param[0];
    /* Indicate which char to be written. */
    uint8_t write_type = (uint8_t)p_parse_value->dw_param[1];
    uint8_t data[270];
    uint16_t length = 270;
    uint16_t i;
    for (i = 0; i < 270; i++)
    {
        data[i] = i;
    }
    if (p_parse_value->param_count > 2)
    {
        length = p_parse_value->dw_param[2];
        if (length > 270)
        {
            length = 270;
        }
    }

    switch (write_type)
    {
    case 2:/* Write the V2 Writable char value on peer server. */
        {
            T_GATT_WRITE_TYPE type = (T_GATT_WRITE_TYPE)p_parse_value->dw_param[3];
            if (type > GATT_WRITE_TYPE_SIGNED_CMD)
            {
                return (RESULT_CMD_ERR_PARAM);
            }
            ret = simp_ble_client_write_v2_char(conn_id, length, data, type);
        }
        break;
    case 3:
        {
            ret = simp_ble_client_write_v6_char(conn_id, length, data);
        }
        break;
    default:
        break;
    }

    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_simphdl(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint16_t hdl_cache[HDL_SIMBLE_CACHE_LEN];
    uint8_t hdl_idx;
    bool ret = simp_ble_client_get_hdl_cache(conn_id, hdl_cache,
                                             sizeof(uint16_t) * HDL_SIMBLE_CACHE_LEN);

    if (ret)
    {
        for (hdl_idx = HDL_SIMBLE_SRV_START; hdl_idx < HDL_SIMBLE_CACHE_LEN; hdl_idx++)
        {
            data_uart_print("-->Index %d -- Handle 0x%x\r\n", hdl_idx, hdl_cache[hdl_idx]);
        }
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif
#if F_BT_ANCS_CLIENT_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_ancsdis(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool ret;
    uint8_t conn_id = p_parse_value->dw_param[0];
    ret = ancs_start_discovery(conn_id);
    if (ret)
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}
#endif
static T_USER_CMD_PARSE_RESULT cmd_startadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_adv_start();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_advdata(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t len = p_parse_value->dw_param[0];
    uint8_t value = p_parse_value->dw_param[1];
    uint8_t adv_data[31];
    memset(adv_data, value, len);
    le_adv_set_param(GAP_PARAM_ADV_DATA, len, adv_data);
    cause = le_adv_update_param();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_stopadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_adv_stop();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_adv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = 80;
    uint16_t adv_int_max = 80;

    if (p_parse_value->param_count > 0)
    {
        adv_int_min = p_parse_value->dw_param[0];
        adv_int_max = p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        adv_filter_policy = p_parse_value->dw_param[1];
    }

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);

    cause = le_adv_start();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_advld(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_LDC_DIRECT_IND;
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint16_t adv_int_min = 300;
    uint16_t adv_int_max = 320;
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  addr_len;

    for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
    {
        adv_direct_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1];
    }

    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);

    cause = le_adv_start();

    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_advhd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_HDC_DIRECT_IND;
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  addr_len;

    for (addr_len = 0; addr_len < GAP_BD_ADDR_LEN; addr_len++)
    {
        adv_direct_addr[addr_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - addr_len - 1];
    }

    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);
    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);

    cause = le_adv_start();

    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_advscan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_SCAN_IND;
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = 300;
    uint16_t adv_int_max = 320;

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);

    cause = le_adv_start();

    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_advnonconn(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_NONCONN_IND;
    uint8_t  adv_chann_map = GAP_ADVCHAN_ALL;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    uint16_t adv_int_min = 300;
    uint16_t adv_int_max = 320;

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(adv_chann_map), &adv_chann_map);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(adv_int_min), &adv_int_min);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(adv_int_max), &adv_int_max);

    cause = le_adv_start();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_advwl(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    //advertising parameters
    T_GAP_CAUSE cause;
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_WHITE_LIST_ALL;
    uint8_t  wl_addr_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  wl_addr[GAP_BD_ADDR_LEN] = {0};

    //get connected device address and address type
    le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR_TYPE, &wl_addr_type);
    le_get_gap_param(GAP_PARAM_LATEST_CONN_BD_ADDR, &wl_addr);

    le_modify_white_list(GAP_WHITE_LIST_OP_ADD, wl_addr, (T_GAP_REMOTE_ADDR_TYPE)wl_addr_type);

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);

    cause = le_adv_start();

    return (T_USER_CMD_PARSE_RESULT)cause;
}

#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_readadvpwr(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_adv_read_tx_power();
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
static T_USER_CMD_PARSE_RESULT cmd_latency(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    bool disable = p_parse_value->dw_param[1];
    T_GAP_CAUSE cause;

    cause = le_disable_slave_latency(conn_id, disable);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_epassed(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    /* Indicate which char to be written. */
    bool enable = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;

    cause = le_update_passed_chann_map(enable);
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
static T_USER_CMD_PARSE_RESULT cmd_srvchange(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t  conn_id = p_parse_value->dw_param[0];
    uint16_t start_handle   = p_parse_value->dw_param[1];
    uint16_t end_handle   = p_parse_value->dw_param[2];
    gatts_service_changed_indicate(conn_id, start_handle, end_handle);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_simpv3notify(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t notif_val[244];
    uint16_t length = p_parse_value->dw_param[1];
    uint16_t tx_num = 0;

    if (length > 244)
    {
        return RESULT_ERR;
    }
    memset(notif_val, 1, length);

    if (p_parse_value->param_count > 2)
    {
        tx_num = p_parse_value->dw_param[2];
    }

#if  F_BT_UPPER_STACK_USE_VIRTUAL_HCI
    GPIO->DATAOUT |= BIT(18);
    GPIO->DATAOUT &= ~BIT(18);
#endif
    if (!simp_ble_service_send_v3_notify(conn_id, simp_srv_id,
                                         &notif_val,
                                         length))
    {
        return (RESULT_ERR);
    }
    if (tx_num > 1)
    {
        gap_v3_notif_test.v3_tx_idx = 2;
        gap_v3_notif_test.v3_tx_num = tx_num - 1;
        gap_v3_notif_test.v3_tx_conn_id = conn_id;
        gap_v3_notif_test.v3_tx_len = length;
        gap_v3_notif_test.v3_tx_cmp_num = 0;
        gap_v3_notif_test.v3_rx_num = 0;
        gap_v3_notif_test.v8_rx_num = 0;
        gap_v3_notif_test.v8_tx_cnt = 0;
        gap_v3_notif_test.v3_tx_cnt = 1;

    }
#if  F_BT_UPPER_STACK_USE_VIRTUAL_HCI
    GPIO->DATAOUT |= BIT(18);
    GPIO->DATAOUT &= ~BIT(18);
#endif
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_simpv3dump(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    data_uart_print("simpv3dump: v3_tx_idx 0x%x, v3_tx_conn_id %d, v3_tx_num %d, v3_tx_cmp_num %d, v3_tx_cnt %d, v8_tx_cnt %d\r\n",
                    gap_v3_notif_test.v3_tx_idx,
                    gap_v3_notif_test.v3_tx_conn_id,
                    gap_v3_notif_test.v3_tx_num,
                    gap_v3_notif_test.v3_tx_cmp_num,
                    gap_v3_notif_test.v3_tx_cnt,
                    gap_v3_notif_test.v8_tx_cnt);
    data_uart_print("simpv3dump: v3_rx_num %d, v8_rx_num %d\r\n",
                    gap_v3_notif_test.v3_rx_num,
                    gap_v3_notif_test.v8_rx_num);
    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_simpv4ind(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t ind_val[244];
    uint16_t length = p_parse_value->dw_param[1];
    uint16_t i;

    if (length > 244)
    {
        return RESULT_ERR;
    }
    for (i = 0; i < length; i++)
    {
        ind_val[i] = i;
    }

    if (simp_ble_service_send_v4_indicate(conn_id, simp_srv_id,
                                          &ind_val,
                                          length))
    {
        return (RESULT_SUCESS);
    }
    else
    {
        return (RESULT_ERR);
    }
}

static T_USER_CMD_PARSE_RESULT cmd_simpv8notify(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t notify_val = (uint8_t)p_parse_value->dw_param[1];


    if (!simp_ble_service_simple_v8_notify(conn_id, simp_srv_id,
                                           &notify_val,
                                           1))
    {
        return (RESULT_ERR);
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_simpv8ind(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t ind_val = (uint8_t)p_parse_value->dw_param[1];


    if (!simp_ble_service_simple_v8_indicate(conn_id, simp_srv_id,
                                             &ind_val, 1))
    {
        return (RESULT_ERR);
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_simpv7len(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint16_t len = p_parse_value->dw_param[0];
    uint8_t data[SIMP_READ_V7_MAX_LEN];
    uint16_t i;
    if (len > SIMP_READ_V7_MAX_LEN)
    {
        return (RESULT_ERR);
    }
    for (i = 0; i < len; i++)
    {
        data[i] = i;
    }
    simp_ble_service_set_parameter(SIMPLE_BLE_SERVICE_PARAM_V7_LEN, len, data);

    return (RESULT_SUCESS);
}


static T_USER_CMD_PARSE_RESULT cmd_simpv2writeconf(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];


    if (!server_attr_write_confirm(conn_id,
                                   simp_srv_id,
                                   SIMPLE_BLE_SERVICE_CHAR_V2_WRITE_INDEX,
                                   APP_RESULT_SUCCESS))
    {
        return (RESULT_ERR);
    }

    return (RESULT_SUCESS);
}


#if F_BT_LE_5_0_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_setphy(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t conn_id = p_parse_value->dw_param[0];
    uint8_t all_phys;
    uint8_t tx_phys;
    uint8_t rx_phys;
    T_GAP_PHYS_OPTIONS phy_options = GAP_PHYS_OPTIONS_CODED_PREFER_S8;
    T_GAP_CAUSE cause;

    if (p_parse_value->dw_param[1] == 0)
    {
        all_phys = GAP_PHYS_PREFER_ALL;
        tx_phys = GAP_PHYS_PREFER_1M_BIT;
        rx_phys = GAP_PHYS_PREFER_1M_BIT;
    }
    else if (p_parse_value->dw_param[1] == 1)
    {
        all_phys = GAP_PHYS_PREFER_ALL;
        tx_phys = GAP_PHYS_PREFER_2M_BIT;
        rx_phys = GAP_PHYS_PREFER_2M_BIT;
    }
    else if (p_parse_value->dw_param[1] == 2)
    {
        all_phys = GAP_PHYS_PREFER_ALL;
        tx_phys = GAP_PHYS_PREFER_CODED_BIT;
        rx_phys = GAP_PHYS_PREFER_CODED_BIT;
        phy_options = GAP_PHYS_OPTIONS_CODED_PREFER_S8;
    }
    else if (p_parse_value->dw_param[1] == 3)
    {
        all_phys = GAP_PHYS_PREFER_ALL;
        tx_phys = GAP_PHYS_PREFER_CODED_BIT;
        rx_phys = GAP_PHYS_PREFER_CODED_BIT;
        phy_options = GAP_PHYS_OPTIONS_CODED_PREFER_S2;
    }
    else
    {
        all_phys = GAP_PHYS_NO_PREFER_TX_BIT;
        tx_phys = GAP_PHYS_PREFER_2M_BIT;
        rx_phys = GAP_PHYS_PREFER_1M_BIT;
    }
    cause = le_set_phy(conn_id, all_phys, tx_phys, rx_phys, phy_options);

    return (T_USER_CMD_PARSE_RESULT)cause;
}

#if F_BT_LE_5_0_AE_SCAN_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_escan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    uint8_t  ext_scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT | GAP_EXT_SCAN_PHYS_CODED_BIT;
    uint8_t  mode = p_parse_value->dw_param[0];

    link_mgr_clear_device_list();
    if (mode == 1)
    {
        ext_scan_duration = 500;
    }
    else if (mode == 2)
    {
        ext_scan_duration = 500;
        ext_scan_period = 8;
        ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLED_RESET_FOR_EACH_PERIOD;
    }
    else if (mode == 3)
    {
        ext_scan_filter_policy = GAP_SCAN_FILTER_WHITE_LIST;
    }
    else if (mode == 4)
    {
        scan_phys = p_parse_value->dw_param[1];
    }
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(ext_scan_filter_policy),
                          &ext_scan_filter_policy);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(ext_scan_filter_duplicate),
                          &ext_scan_filter_duplicate);
    cause = le_ext_scan_start();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_stopescan(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_ext_scan_stop();
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif

#if F_BT_LE_5_0_AE_ADV_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_seadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t  adv_handle = p_parse_value->dw_param[0];
    uint16_t duration = 0;
    uint8_t  max_num  = 0;
    T_GAP_CAUSE cause;
    if (p_parse_value->param_count > 1)
    {
        duration = p_parse_value->dw_param[1];
    }
    if (p_parse_value->param_count > 2)
    {
        max_num = p_parse_value->dw_param[2];
    }

    cause = le_ext_adv_set_adv_enable_param(adv_handle, duration, max_num);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_ueadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];
    uint8_t update_flags = EXT_ADV_SET_ADV_PARAS | EXT_ADV_SET_ADV_DATA | EXT_ADV_SET_SCAN_RSP_DATA;
    T_GAP_CAUSE cause;
    uint8_t adv_handle;
    uint16_t adv_event_prop = (uint16_t) LE_EXT_ADV_LEGACY_ADV_CONN_SCAN_UNDIRECTED;
    uint32_t primary_adv_interval_min = 320;
    uint32_t primary_adv_interval_max = 320;
    uint8_t  primary_adv_channel_map = GAP_ADVCHAN_ALL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t *p_peer_address = NULL;
    uint8_t filter_policy = 0;
    uint8_t tx_power = 127;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    uint8_t secondary_adv_max_skip = 0x00;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;
    uint8_t adv_sid = 0;
    bool scan_req_notification_enable = false;

    if (p_parse_value->param_count > 1)
    {
        update_flags = p_parse_value->dw_param[1];
    }
    if (p_parse_value->param_count > 2)
    {
        own_address_type = (T_GAP_LOCAL_ADDR_TYPE)p_parse_value->dw_param[2];
    }
    if (idx == 0)
    {
        adv_handle = le_ext_adv_create_adv_handle();
        le_ext_adv_set_adv_param(adv_handle,
                                 adv_event_prop,
                                 primary_adv_interval_min,
                                 primary_adv_interval_max,
                                 primary_adv_channel_map,
                                 own_address_type,
                                 peer_address_type,
                                 p_peer_address,
                                 (T_GAP_ADV_FILTER_POLICY)filter_policy,
                                 tx_power,
                                 primary_adv_phy,
                                 secondary_adv_max_skip,
                                 secondary_adv_phy,
                                 adv_sid,
                                 scan_req_notification_enable);
        le_ext_adv_set_adv_data(adv_handle, sizeof(ext_adv_data), (uint8_t *)ext_adv_data);
        le_ext_adv_set_scan_response_data(adv_handle, sizeof(ext_scan_response),
                                          (uint8_t *)ext_scan_response);
    }
    else
    {
        adv_event_prop = (uint16_t) LE_EXT_ADV_EXTENDED_ADV_SCAN_UNDIRECTED;
        switch (idx)
        {
        case 0x11:
            {
                primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
                secondary_adv_phy = GAP_PHYS_1M;
            }
            break;
        case 0x12:
            {
                primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
                secondary_adv_phy = GAP_PHYS_2M;
            }
            break;

        case 0x13:
            {
                primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
                secondary_adv_phy = GAP_PHYS_CODED;
            }
            break;

        case 0x31:
            {
                primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
                secondary_adv_phy = GAP_PHYS_1M;
            }
            break;

        case 0x32:
            {
                primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
                secondary_adv_phy = GAP_PHYS_2M;
            }
            break;

        case 0x33:
            {
                primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
                secondary_adv_phy = GAP_PHYS_CODED;
            }
            break;

        default:
            adv_event_prop = (uint16_t) LE_EXT_ADV_EXTENDED_ADV_SCAN_UNDIRECTED;
            primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
            break;
        }
        if (p_parse_value->param_count > 3)
        {
            adv_event_prop = p_parse_value->dw_param[3];
        }
#if 0
        if (adv_event_prop & GAP_EXT_ADV_EVT_PROP_SCANNABLE_ADV == 0)
        {
            update_flags &= ~EXT_ADV_SET_SCAN_RSP_DATA;

        }

        if (adv_event_prop & GAP_EXT_ADV_EVT_PROP_CONNECTABLE_ADV == 0)
        {
            update_flags &= ~EXT_ADV_SET_ADV_DATA;
        }
#endif
        adv_handle = le_ext_adv_create_adv_handle();
        le_ext_adv_set_adv_param(adv_handle,
                                 adv_event_prop,
                                 primary_adv_interval_min,
                                 primary_adv_interval_max,
                                 primary_adv_channel_map,
                                 own_address_type,
                                 peer_address_type,
                                 p_peer_address,
                                 (T_GAP_ADV_FILTER_POLICY)filter_policy,
                                 tx_power,
                                 primary_adv_phy,
                                 secondary_adv_max_skip,
                                 secondary_adv_phy,
                                 adv_sid,
                                 scan_req_notification_enable);
        le_ext_adv_set_adv_data(adv_handle, sizeof(ext_large_adv_data), (uint8_t *)ext_large_adv_data);
        le_ext_adv_set_scan_response_data(adv_handle, sizeof(ext_large_scan_data),
                                          (uint8_t *)ext_large_scan_data);
    }
    if (own_address_type == GAP_LOCAL_ADDR_LE_RANDOM)
    {
        uint8_t rand_addr[6];
        le_gen_rand_addr(GAP_RAND_ADDR_STATIC, rand_addr);
        cause = le_ext_adv_set_random(adv_handle, rand_addr);
    }
    else if (own_address_type == GAP_LOCAL_ADDR_LE_RAP_OR_RAND)
    {
        uint8_t rand_addr[6];
        rand_addr[0] = 0x4E;
        rand_addr[1] = 0x11;
        rand_addr[2] = 0x22;
        rand_addr[3] = 0x33;
        rand_addr[4] = 0x44;
        rand_addr[5] = adv_handle;
        cause = le_ext_adv_set_random(adv_handle, rand_addr);
        le_cfg_local_identity_address(rand_addr, GAP_IDENT_ADDR_RAND);
    }
    cause = le_ext_adv_start_setting(adv_handle, update_flags);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_uedadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[6];
    T_GAP_CAUSE cause;
    uint8_t adv_handle;
    uint16_t adv_event_prop = (uint16_t) LE_EXT_ADV_EXTENDED_ADV_NON_SCAN_NON_CONN_DIRECTED;
    uint32_t primary_adv_interval_min = 320;
    uint32_t primary_adv_interval_max = 320;
    uint8_t primary_adv_channel_map = GAP_ADVCHAN_ALL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  peer_address[GAP_BD_ADDR_LEN] = {0};
    uint8_t filter_policy = 0;
    uint8_t tx_power = 127;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    uint8_t secondary_adv_max_skip = 0x00;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_1M;
    uint8_t adv_sid = 0;
    bool scan_req_notification_enable = false;
    uint8_t peer_address_len;

    for (peer_address_len = 0; peer_address_len < GAP_BD_ADDR_LEN; peer_address_len++)
    {
        peer_address[peer_address_len] = p_parse_value->dw_param[GAP_BD_ADDR_LEN - peer_address_len - 1];
    }

    if (p_parse_value->param_count > 7)
    {
        adv_event_prop = p_parse_value->dw_param[7];

    }
    switch (idx)
    {
    case 0x11:
        {
            primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
            secondary_adv_phy = GAP_PHYS_1M;
        }
        break;
    case 0x12:
        {
            primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
            secondary_adv_phy = GAP_PHYS_2M;
        }
        break;

    case 0x13:
        {
            primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
            secondary_adv_phy = GAP_PHYS_CODED;
        }
        break;

    case 0x31:
        {
            primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
            secondary_adv_phy = GAP_PHYS_1M;
        }
        break;

    case 0x32:
        {
            primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
            secondary_adv_phy = GAP_PHYS_2M;
        }
        break;

    case 0x33:
        {
            primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
            secondary_adv_phy = GAP_PHYS_CODED;
        }
        break;

    default:
        adv_event_prop = (uint16_t) LE_EXT_ADV_EXTENDED_ADV_SCAN_UNDIRECTED;
        primary_adv_phy = GAP_PHYS_PRIM_ADV_CODED;
        break;
    }

    adv_handle = le_ext_adv_create_adv_handle();
    le_ext_adv_set_adv_param(adv_handle,
                             adv_event_prop,
                             primary_adv_interval_min,
                             primary_adv_interval_max,
                             primary_adv_channel_map,
                             own_address_type,
                             peer_address_type,
                             peer_address,
                             (T_GAP_ADV_FILTER_POLICY)filter_policy,
                             tx_power,
                             primary_adv_phy,
                             secondary_adv_max_skip,
                             secondary_adv_phy,
                             adv_sid,
                             scan_req_notification_enable);
    le_ext_adv_set_adv_data(adv_handle, sizeof(ext_large_adv_data), (uint8_t *)ext_large_adv_data);
    le_ext_adv_set_scan_response_data(adv_handle, sizeof(ext_large_scan_data),
                                      (uint8_t *)ext_large_scan_data);
    cause = le_ext_adv_start_setting(adv_handle, EXT_ADV_SET_AUTO);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_eadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t adv_handle[4] = {0};
    uint8_t num_of_sets = 1;
    T_GAP_CAUSE cause;
    if (p_parse_value->param_count > 0)
    {
        adv_handle[0] = p_parse_value->dw_param[0];
        num_of_sets = 1;
    }
    if (p_parse_value->param_count > 1)
    {
        adv_handle[1] = p_parse_value->dw_param[1];
        num_of_sets = 2;
    }
    if (p_parse_value->param_count > 2)
    {
        adv_handle[2] = p_parse_value->dw_param[2];
        num_of_sets = 3;
    }
    if (p_parse_value->param_count > 3)
    {
        adv_handle[3] = p_parse_value->dw_param[3];
        num_of_sets = 4;
    }
    cause = le_ext_adv_enable(num_of_sets, adv_handle);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_stopeadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t adv_handle[4] = {0};
    uint8_t num_of_sets = 1;
    T_GAP_CAUSE cause;
    if (p_parse_value->param_count > 0)
    {
        adv_handle[0] = p_parse_value->dw_param[0];
        num_of_sets = 1;
    }
    if (p_parse_value->param_count > 1)
    {
        adv_handle[1] = p_parse_value->dw_param[1];
        num_of_sets = 2;
    }
    if (p_parse_value->param_count > 2)
    {
        adv_handle[2] = p_parse_value->dw_param[2];
        num_of_sets = 3;
    }
    if (p_parse_value->param_count > 3)
    {
        adv_handle[3] = p_parse_value->dw_param[3];
        num_of_sets = 4;
    }
    cause = le_ext_adv_disable(num_of_sets, adv_handle);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_ceadv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_ext_adv_clear_set();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_readv(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t adv_handle = 0;
    T_GAP_CAUSE cause;
    if (p_parse_value->param_count > 0)
    {
        adv_handle = p_parse_value->dw_param[0];
    }
    cause = le_ext_adv_remove_set(adv_handle);
    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif
#endif

#if F_BT_LE_PRIVACY_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_advrel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t  adv_direct_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t  adv_direct_addr[GAP_BD_ADDR_LEN] = {0};
    uint8_t  adv_evt_type = GAP_ADTYPE_ADV_IND;
    uint8_t  adv_filter_policy = GAP_ADV_FILTER_ANY;
    T_GAP_CAUSE cause;

    if (p_parse_value->param_count > 0)
    {
        adv_evt_type = p_parse_value->dw_param[0];
    }
    if (p_parse_value->param_count > 1)
    {
        adv_filter_policy = p_parse_value->dw_param[1];
    }
    if (p_parse_value->param_count > 2)
    {
        uint8_t idx = p_parse_value->dw_param[2];
        if (privacy_table[idx].is_used == true)
        {
            adv_direct_type = privacy_table[idx].remote_bd_type;
            memcpy(adv_direct_addr, privacy_table[idx].addr, 6);
        }
        else
        {
            return RESULT_CMD_ERR_PARAM;
        }
    }

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(adv_evt_type), &adv_evt_type);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(adv_filter_policy), &adv_filter_policy);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(adv_direct_type), &adv_direct_type);
    le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(adv_direct_addr), adv_direct_addr);

    cause = le_adv_start();
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_readlra(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;

    if (privacy_table[idx].is_used)
    {
        cause = le_privacy_read_local_resolv_addr((T_GAP_IDENT_ADDR_TYPE)privacy_table[idx].remote_bd_type,
                                                  privacy_table[idx].addr);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_ERR;
    }
}

static T_USER_CMD_PARSE_RESULT cmd_readpra(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;
    if (privacy_table[idx].is_used)
    {
        cause = le_privacy_read_peer_resolv_addr(privacy_table[idx].remote_bd_type,
                                                 privacy_table[idx].addr);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_ERR;
    }
}

static T_USER_CMD_PARSE_RESULT cmd_setrae(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    bool enable = true;
    T_GAP_CAUSE cause;
    if (p_parse_value->dw_param[0] == 0)
    {
        enable = false;
    }
    cause = le_privacy_set_addr_resolution(enable);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_showrel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t i;
    data_uart_print("show resolved list\r\n");

    for (i = 0; i < PRIVACY_ENTRY_SIZE; i++)
    {
        if (privacy_table[i].is_used)
        {
            data_uart_print("rel[%d] RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d is_add_to_list=%d pending=%d\r\n",
                            i,
                            privacy_table[i].addr[5], privacy_table[i].addr[4],
                            privacy_table[i].addr[3], privacy_table[i].addr[2],
                            privacy_table[i].addr[1], privacy_table[i].addr[0],
                            privacy_table[i].remote_bd_type,
                            privacy_table[i].is_add_to_list,
                            privacy_table[i].pending);
        }
    }

    return (RESULT_SUCESS);
}

static T_USER_CMD_PARSE_RESULT cmd_setprivacy(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];
    uint8_t mode = p_parse_value->dw_param[1];
    T_GAP_CAUSE cause;
    if (privacy_table[idx].is_used)
    {
        cause = le_privacy_set_mode((T_GAP_IDENT_ADDR_TYPE)privacy_table[idx].remote_bd_type,
                                    privacy_table[idx].addr,
                                    (T_GAP_PRIVACY_MODE)mode);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_CMD_ERR_PARAM;
    }
}

static T_USER_CMD_PARSE_RESULT cmd_wlrel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_WHITE_LIST_OP op = (T_GAP_WHITE_LIST_OP)p_parse_value->dw_param[0];
    uint8_t idx = p_parse_value->dw_param[1];
    T_GAP_CAUSE cause;
    if (op == GAP_WHITE_LIST_OP_CLEAR)
    {
        cause = le_modify_white_list(op, NULL, GAP_REMOTE_ADDR_LE_PUBLIC);
    }
    else
    {
        if (privacy_table[idx].is_used)
        {
            cause = le_modify_white_list(op, privacy_table[idx].addr,
                                         (T_GAP_REMOTE_ADDR_TYPE)privacy_table[idx].remote_bd_type);
        }
        else
        {
            return RESULT_CMD_ERR_PARAM;
        }
    }
    return (T_USER_CMD_PARSE_RESULT)cause;

}

static T_USER_CMD_PARSE_RESULT cmd_conrel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint8_t idx = p_parse_value->dw_param[0];
    T_GAP_CAUSE cause;

    if (privacy_table[idx].is_used)
    {
        T_GAP_LE_CONN_REQ_PARAM conn_req_param;

        conn_req_param.scan_interval = 0x10;
        conn_req_param.scan_window = 0x10;
        conn_req_param.conn_interval_min = 80;
        conn_req_param.conn_interval_max = 80;
        conn_req_param.conn_latency = 0;
        conn_req_param.supv_tout = 1000;
        conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
        conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
        cause = le_connect(0, privacy_table[idx].addr,
                           (T_GAP_REMOTE_ADDR_TYPE)privacy_table[idx].remote_bd_type,
                           (T_GAP_LOCAL_ADDR_TYPE)p_parse_value->dw_param[1],
                           1000);
        return (T_USER_CMD_PARSE_RESULT)cause;
    }
    else
    {
        return RESULT_ERR;
    }
}
#endif

#if F_BT_LE_4_1_CBC_SUPPORT
static T_USER_CMD_PARSE_RESULT cmd_lepsm(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_cbc_reg_psm(p_parse_value->dw_param[0], p_parse_value->dw_param[1]);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_lesec(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_cbc_set_psm_security(p_parse_value->dw_param[0], p_parse_value->dw_param[1],
                                    (T_LE_CBC_SECURITY_MODE)p_parse_value->dw_param[2], p_parse_value->dw_param[3]);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_conle(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_cbc_create(p_parse_value->dw_param[0], p_parse_value->dw_param[1]);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_discle(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    cause = le_cbc_disc(p_parse_value->dw_param[0]);
    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_ledata(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause;
    void *p_buffer;
    uint16_t cid = p_parse_value->dw_param[0];
    uint16_t length = p_parse_value->dw_param[1];
    p_buffer = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
    if (p_buffer != NULL)
    {
        memset(p_buffer, 2, length);

        cause = le_cbc_send_data(cid, p_buffer, length);
        os_mem_free(p_buffer);
    }
    else
    {
        return RESULT_GAP_CAUSE_NO_RESOURCE;
    }

    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif

#if F_BT_CONTROLLER_POWER_CONTROL
static T_USER_CMD_PARSE_RESULT cmd_poweron(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause = gap_bt_power_on();

    return (T_USER_CMD_PARSE_RESULT)cause;
}

static T_USER_CMD_PARSE_RESULT cmd_poweroff(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    T_GAP_CAUSE cause = gap_bt_power_off();

    return (T_USER_CMD_PARSE_RESULT)cause;
}
#endif

static T_USER_CMD_PARSE_RESULT cmd_taskadd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    test_task_init();

    return RESULT_SUCESS;
}

static T_USER_CMD_PARSE_RESULT cmd_taskdel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    test_task_deinit();

    return RESULT_SUCESS;
}

static T_USER_CMD_PARSE_RESULT cmd_stackadd(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    upper_task_init();
    app_init();
    return RESULT_SUCESS;
}

static T_USER_CMD_PARSE_RESULT cmd_stackdel(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    app_deinit();
    upper_task_deinit();
    return RESULT_SUCESS;
}

static T_USER_CMD_PARSE_RESULT cmd_tc(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    test_case_id = (T_CUR_TEST_CASE)p_parse_value->dw_param[0];
    test_max_count = p_parse_value->dw_param[1];
    test_cur_count = 0;
    app_send_msg_to_uart_app(TC_START, 0);
    return RESULT_SUCESS;
}

static T_USER_CMD_PARSE_RESULT cmd_dump(T_USER_CMD_PARSED_VALUE *p_parse_value)
{
    uint16_t type = p_parse_value->dw_param[0];
    if (type == 0)
    {
        APP_PRINT_INFO1("os_mem_peek: %d", os_mem_peek(RAM_TYPE_DATA_ON));
    }
    else if (type == 1)
    {
        //vHeapUsageDump();

        /* Dump task stack usage */
        //extern void vTaskStackDump(void);
        //vTaskStackDump();

        extern void bte_pool_dump(uint8_t handle);
        bte_pool_dump(0xFF);

        extern void hci_dump(void);
        hci_dump();
    }
    else
    {
    }

    return RESULT_SUCESS;
}

/*----------------------------------------------------
 * command table
 * --------------------------------------------------*/
const T_USER_CMD_TABLE_ENTRY user_cmd_table[] =
{
    /************************** Common cmd *************************************/
    {
        "reset",
        "reset\n\r",
        "reset system\n\r",
        cmd_reset
    },
    {
        "vendor",
        "vendor\n\r",
        "vendor cmd\n\r",
        cmd_vendor
    },
    {
        "reg",
        "reg [test_case]\n\r",
        "Init stack, reg test case\n\r",
        cmd_reg
    },
    {
        "dlps",
        "dlps [mode]\n\r",
        "dlps\n\r",
        cmd_dlps
    },
    {
        "tracelevel",
        "tracelevel [0/1: off/on]\n\r",
        "Set trace level\n\r",
        cmd_tracelevel
    },
    {
        "setlocaltype",
        "setlocaltype [type]\n\r",
        "set local address type: 0-public, 1-random, 2-resolved or public 3- resolved or random\n\r",
        cmd_setlocaltype
    },
    {
        "setstatic",
        "setstatic [type]\n\r",
        "set static address type: 0-, 1-\n\r",
        cmd_setstatic
    },
    {
        "txpwrset",
        "txpwrset [type] [tx gain]\n\r",
        "Set the TX Power of BLE RF\n\r",
        cmd_txpwrset
    },
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    {
        "wrdefaultdatalen",
        "wrdefaultdatalen [tx octests] [tx time]\n\r",
        "Specify suggested data length\n\r",
        cmd_writedefaultdatalen
    },
#endif
    {
        "rssiread",
        "rssiread [conn_id]\n\r",
        "Read the RSSI value\n\r",
        cmd_rssiread
    },
#if F_BT_LE_READ_CHANN_MAP
    {
        "readchanmap",
        "readchanmap [conn_id]\n\r",
        "read channel map\n\r",
        cmd_readchanmap
    },
#endif
    {
        "conupdreq",
        "conupdreq [conn_id] [interval_min] [interval_max] [latency] [supervision_timeout]\n\r",
        "LE connection param update request\r\n\
        sample: conupdreq 0 0x30 0x40 0 500\n\r",
        cmd_conupdreq
    },
    {
        "showcon",
        "showcon\n\r",
        "Show all devices connecting status\n\r",
        cmd_showcon
    },
    {
        "disc",
        "disc [conn_id]\n\r",
        "Disconnect to remote device\n\r",
        cmd_disc
    },
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
    {
        "setdatalength",
        "setdatalength [conn_id] [tx octests] [tx time]\n\r",
        "set data length\n\r",
        cmd_setdatalength
    },
#endif
    {
        "wl",
        "wl [op] [conn_id]\n\r",
        "modify remote BD to whitelist\n\r",
        cmd_wl
    },
    {
        "wldev",
        "wldev [op] [dev idx]\n\r",
        "modify remote BD to whitelist\n\r",
        cmd_wldev
    },
    /**************************Auth Common cmd *************************************/
    {
        "authmode",
        "authmode [auth_flags] [io_cap] [sec_enable] [oob_enable]\n\r",
        "Config authentication mode\r\n\
        [auth_flags]:authentication req bit field: bit0-(bonding), bit2-(MITM), bit3-(SC)\r\n\
        [io_cap]:set io Capabilities: 0-(display only), 1-(display yes/no), 2-(keyboard noly), 3-(no IO), 4-(keyboard display)\r\n\
        [sec_enable]:Start smp pairing procedure when connected: 0-(disable), 1-(enable)\r\n\
        [oob_enable]:Enable oob flag: 0-(disable), 1-(enable)\r\n\
        sample: authmode 0x5 2 1 0\n\r",
        cmd_authmode
    },
    {
        "setkeydis",
        "setkeydis [init] [resp]\n\r",
        "set key distribute\n\r",
        cmd_setkeydis
    },
    {
        "sauth",
        "sauth [conn_id]\n\r",
        "Send authentication request\n\r",
        cmd_sauth
    },
    {
        "userconf",
        "userconf [conn_id] [conf]\n\r",
        "Send user confirmation when show GAP_MSG_LE_BOND_USER_CONFIRMATION\r\n\
        [conf]: 0-(Reject), 1-(Accept)\r\n\
        sample: userconf 0 1\n\r",
        cmd_userconf
    },
    {
        "authkey",
        "authkey [conn_id] [passkey]\n\r",
        "Input passkey when show GAP_MSG_LE_BOND_PASSKEY_INPUT\r\n\
        [passkey]: 0 - 999999\r\n\
        sample: authkey 0 123456\n\r",
        cmd_authkey
    },
#if F_BT_LE_SMP_OOB_SUPPORT
    {
        "oob",
        "oob [conn_id] [oob0] [oob3] [oob7] [oob15]\n\r",
        "input oob data\n\r",
        cmd_oob
    },
#endif
#if F_BT_LE_4_2_KEY_PRESS_SUPPORT
    {
        "keypress",
        "keypress [conn_id] [type]\n\r",
        "keypress notification\n\r",
        cmd_keypress
    },
#endif
    {
        "bondinfo",
        "bondinfo\n\r",
        "Get all Bonded devices information\n\r",
        cmd_bondinfo
    },
    {
        "bondclear",
        "bondclear\n\r",
        "Clear all bonded devices information\n\r",
        cmd_bondclear
    },
    /************************** Central only *************************************/
    {
        "scan",
        "scan [filter_policy] [filter_duplicate]\n\r",
        "Start scan\r\n\
        [filter_policy]: 0-(any), 1-(whitelist), 2-(any RPA), 3-(whitelist RPA) \r\n\
        [filter_duplicate]: 0-(disable), 1-(enable) \n\r",
        cmd_scan
    },
    {
        "stopscan",
        "stopscan\n\r",
        "Stop scan\n\r",
        cmd_stopscan
    },
    {
        "showdev",
        "showdev\n\r",
        "Show scan dev list: filter simple ble service\n\r",
        cmd_showdev
    },
#if F_BT_LE_GAP_CENTRAL_SUPPORT
    {
        "con",
        "con [BD0] [BD1] [BD2] [BD3] [BD4] [BD5] [addr_type]\n\r",
        "Connect to remote device: use address\r\n\
        [BD0] [BD1] [BD2] [BD3] [BD4] [BD5]: remote device address\r\n\
        [addr_type]: 0-(public), 1-(random)\r\n\
        sample: con x11 x22 x33 x44 x55 x66 0 \n\r",
        cmd_con
    },
    {
        "condev",
        "condev [idx]\n\r",
        "Connect to remote device: use showdev to show idx\r\n\
        [idx]: use cmd showdev to show idx before use this cmd\r\n\
        sample: condev 0\n\r",
        cmd_condev
    },
    {
        "chanclassset",
        "chanclassset [idx0] [idx1] [idx2] [idx3] [idx4]\n\r",
        "Set Host Channel Classification\r\n\
        [idx0] [idx1] [idx2] [idx3] [idx4]: channel bit map\r\n\
        sample: chanclassset xff xff x3f xff x00\n\r",
        cmd_chanclassset
    },
#endif
    /************************** GATT client *************************************/
#if F_BT_LE_GATT_CLIENT_SUPPORT
    {
        "srvdis",
        "srvdis [conn_id]\n\r",
        "Service discovery, discover all primary services\n\r",
        cmd_srvdis
    },
    /*GAPS client*/
    {
        "gapdis",
        "gapdis [conn_id]\n\r",
        "Start discovery gap service\n\r",
        cmd_gapdis
    },
    {
        "gapread",
        "gapread [conn_id] [type]\n\r",
        "Read GAP service characteristic value\r\n\
        [type]: 0-(read device name), 1-(read appearance)\r\n\
        simple: gapread 0 0\n\r",
        cmd_gapread
    },
    {
        "simpdis",
        "simpdis [conn_id]\n\r",
        "Start discovery simple ble service\n\r",
        cmd_simpdis
    },
    {
        "simpread",
        "simpread [conn_id] [type] [pattern]\n\r",
        "Read simple ble service characteristic and descriptor value\r\n\
        [type]: 0-(read v1), 1-(v3 cccd), 2-(v4 cccd)\r\n\
        [pattern]: 0-(read by handle), 1-(read by uuid)\r\n\
        sample: simpread 0 1 0 \n\r",
        cmd_simpread
    },
    {
        "simpcccd",
        "simpcccd [conn_id] [type] [enable]\n\r",
        "Config simple ble service client characteristic configuration descriptor value\r\n\
        [type]: 0-(v3 notify), 1-(v4 indication) 2-(v8 notify and indication)\r\n\
        [enable](type!=2): 0-(disable), 1-(enable) \r\n\
        [enable](type==2): 0-(disable), 0x01-(notify), 0x10-(indicate), 0x11-(notify and indicate) \r\n\
        sample: simpcccd 0 1 1\n\r",
        cmd_simpcccd
    },
    {
        "simpwrite",
        "simpwrite [conn_id] [char] [value]\n\r",
        "Write all related chars by user input\n\r",
        cmd_simpwrite
    },
    {
        "simphdl",
        "simphdl [conn_id]\n\r",
        "List simple ble service handle cache\n\r",
        cmd_simphdl
    },
#endif
#if F_BT_ANCS_CLIENT_SUPPORT
    {
        "ancsdis",
        "ancsdis [conn_id]\n\r",
        "Start discovery ancs service\n\r",
        cmd_ancsdis
    },
#endif
    /***************peripheral*******************/
    {
        "startadv",
        "startadv\n\r",
        "start advertising without setting advertising parameters\n\r",
        cmd_startadv
    },
    {
        "advdata",
        "advdata [len] [value]\n\r",        // len: 1-31
        "Modify advertising data\n\r",     // value: 0x00, ff
        cmd_advdata
    },
    {
        "stopadv",
        "stopadv\n\r",
        "Stop advertising\n\r",
        cmd_stopadv
    },
    {
        "adv",
        "adv [adv_interval] [filter_policy]\n\r",
        "Start Undirected advertising\r\n\
        [adv_interval]: 0x0020 - 0x4000 (20ms - 10240ms, 0.625ms/step)\r\n\
        [filter_policy]: 0-(all), 1-(whitelist scan), 2-(whitelist conn), 3-(whitelist all)\r\n\
        sample: adv x40 0 \n\r",
        cmd_adv
    },
    {
        "advld",
        "advld [BD0] [BD1] [BD2] [BD3] [BD4] [BD5]\n\r",
        "Start lower duty directed advertising\r\n\
        [BD0] [BD1] [BD2] [BD3] [BD4] [BD5]: peer address\r\n\
        sample: advld x11 x22 x33 x44 x55 x66\n\r",
        cmd_advld
    },
    {
        "advhd",
        "advhd [BD0] [BD1] [BD2] [BD3] [BD4] [BD5]\n\r",
        "Start high duty directed advertising\r\n\
        [BD0] [BD1] [BD2] [BD3] [BD4] [BD5]: peer address\r\n\
        sample: advhd x11 x22 x33 x44 x55 x66\n\r",
        cmd_advhd
    },
    {
        "advscan",
        "advscan\n\r",
        "Start scannable undirected advertising\n\r",
        cmd_advscan
    },
    {
        "advnonconn",
        "advnonconn\n\r",
        "Start non_connectable undirected advertising\n\r",
        cmd_advnonconn
    },
    {
        "advwl",
        "advwl\n\r",
        "Start undirected advertising with white list\n\r",
        cmd_advwl
    },
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
    {
        "readadvpwr",
        "readadvpwr\n\r",
        "Read adv channel tx power\n\r",
        cmd_readadvpwr
    },
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    {
        "latency",
        "latency [conn_id] [on_off]\n\r",
        "On off slave latency\r\n\
        [on_off]: 0-(turn on the latency), 1-(turn off the latency)\r\n\
        sample: latency 0 1\n\r",
        cmd_latency
    },
    {
        "epassed",
        "epassed [enable]\n\r",
        "Update instant passed channel map\r\n\
        [enable]: 0 - (disable), 1-(enable)\r\n\
        sample: epassed 1\n\r",
        cmd_epassed
    },
#endif
#if F_BT_GAP_HANDLE_VENDOR_FEATURE
    {
        "vadv",
        "vadv [enable] \n\r",
        "Each adv channel with diff data\r\n\
        [enable]: 0-(disable), 1-(enable)\r\n\
        sample: vadv 1 \n\r",
        cmd_vadv
    },
    {
        "vadvdata",
        "vadvdata [type] [len] [value]\n\r",
        "Set adv channel adv data and scan response data\r\n\
        [type]: 0-(adv 38 data), 1-(adv 39 data), 2-(scan response 38 data), 3-(scan response 39 data)\r\n\
        [len]: 0-31\r\n\
        [value]: adv data value\r\n\
        sample: vadvdata 1 5 5 \n\r",
        cmd_vadvdata
    },
#endif
    /********************************Profile Server*********************************/
    {
        "srvchange",
        "srvchange [conn_id] [start_handle] [end_handle] \n\r",
        "Send service change indication\r\n\
        [start_handle]: 1 - 0xFFFF\r\n\
        [end_handle]: 1 - 0xFFFF\n\r",
        cmd_srvchange
    },
    {
        "simpv3notify",
        "simpv3notify [conn_id] [len] \n\r",
        "Send V3 notification\r\n\
        [len]: 0 - (mtu-3)\n\r",
        cmd_simpv3notify
    },
    {
        "simpv3dump",
        "simpv3dump\n\r",
        "Dump v3 result\n\r",
        cmd_simpv3dump
    },
    {
        "simpv4ind",
        "simpv4ind [conn_id] [len]\n\r",
        "Send V4 indication\r\n\
        [len]: 0 - (mtu-3)\n\r",
        cmd_simpv4ind
    },
    {
        "simpv8notify",
        "simpv8notify [conn_id] [val]\n\r",
        "Notify Characteristic V8 Notify value.\n\r",
        cmd_simpv8notify
    },
    {
        "simpv8ind",
        "simpv8ind [conn_id] [val]\n\r",
        "Indicate Characteristic V8 Indicate value.\n\r",
        cmd_simpv8ind
    },
    {
        "simpv7len",
        "simpv7len [len]\n\r",
        "Set V7 Characteristic value length.\n\r",
        cmd_simpv7len
    },
    {
        "simpv2writeconf",
        "simpv2writeconf [conn_id]\n\r",
        "Write Response Characteristic V2 Write Request.\n\r",
        cmd_simpv2writeconf
    },
#if F_BT_LE_5_0_SUPPORT
    {
        "setphy",
        "setphy [conn_id] [phy]\n\r",
        "set phy.\n\r",
        cmd_setphy
    },
#if F_BT_LE_5_0_AE_SCAN_SUPPORT
    {
        "escan",
        "escan [0/1/2/3]\n\r",
        "escan remote devices\n\r",
        cmd_escan
    },
    {
        "stopescan",
        "stopescan\n\r",
        "stopescan remote device\n\r",
        cmd_stopescan
    },
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
    {
        "ueadv",
        "ueadv [adv_handle] [flags]\n\r",
        "Update extend advertising param\n\r",
        cmd_ueadv
    },
    {
        "uedadv",
        "uedadv [adv_handle] [flags]\n\r",
        "Update extend directed advertising param\n\r",
        cmd_uedadv
    },
    {
        "seadv",
        "seadv [adv_handle] [duration] [max_num]\n\r",
        "Set extended adv enable param\n\r",
        cmd_seadv
    },
    {
        "eadv",
        "eadv [adv_handle1] [adv_handle2] [adv_handle3] [adv_handle4]\n\r",
        "Start extended adv\n\r",
        cmd_eadv
    },
    {
        "stopeadv",
        "stopeadv [adv_handle1] [adv_handle2] [adv_handle3] [adv_handle4]\n\r",
        "Stop extended adv\n\r",
        cmd_stopeadv
    },
    {
        "ceadv",
        "ceadv\n\r",
        "clear extended adv set\n\r",
        cmd_ceadv
    },
    {
        "readv",
        "readv [adv_handle]\n\r",
        "remove extended adv set\n\r",
        cmd_readv
    },
#endif
#endif
#if F_BT_LE_PRIVACY_SUPPORT
    {
        "advrel",
        "advrel\n\r",
        "Start undirected advertising with local type = resolved\n\r",
        cmd_advrel
    },
    {
        "setrae",
        "setrae [enable]\n\r",
        "set addr resolution enable.\n\r",
        cmd_setrae
    },
    {
        "readlra",
        "readlra [conn_id]\n\r",
        "read local resolvable address.\n\r",
        cmd_readlra
    },
    {
        "readpra",
        "readlra [conn_id]\n\r",
        "read peer resolvable address..\n\r",
        cmd_readpra
    },
    {
        "showrel",
        "showrel\n\r",
        "show resolved list.\n\r",
        cmd_showrel
    },
    {
        "setprivacy",
        "setprivacy [rel_idx] [0/1]\n\r",
        "set privacy mode.\n\r",
        cmd_setprivacy
    },
    {
        "wlrel",
        "wlrel [op] [rel_idx]\n\r",
        "whitelist\n\r",
        cmd_wlrel
    },
    {
        "conrel",
        "conrel [idx]\n\r",
        "connect to remote device\n\r",
        cmd_conrel
    },
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
    {
        "lepsm",
        "lepsm [psm] [action]\n\r",
        "set le psm\n\r",
        cmd_lepsm
    },
    {
        "lesec",
        "lesec [psm] [active] [mode] [key_size]\n\r",
        "set le psm security\n\r",
        cmd_lesec
    },
    {
        "conle",
        "conle [conn_id] [psm]\n\r",
        "create credit based connection\n\r",
        cmd_conle
    },
    {
        "discle",
        "discle [cid]\n\r",
        "disconnect credit based connection\n\r",
        cmd_discle
    },
    {
        "ledata",
        "ledata [cid] [num] [length]\n\r",
        "send data through credit based connection\n\r",
        cmd_ledata
    },
#endif
#if APP_HID_TEST
    {
        "key",
        "key [conn_id][key]\n\r",
        "Set key\n\r",
        cmd_key
    },
#endif
#if F_BT_CONTROLLER_POWER_CONTROL
    {
        "poweron",
        "poweron \n\r",
        "Turn power on\n\r",
        cmd_poweron
    },
    {
        "poweroff",
        "poweroff \n\r",
        "Turn power off\n\r",
        cmd_poweroff
    },
#endif
    {
        "taskadd",
        "taskadd \n\r",
        "Add task\n\r",
        cmd_taskadd
    },
    {
        "taskdel",
        "taskadd \n\r",
        "Delete task\n\r",
        cmd_taskdel
    },
    {
        "stackadd",
        "stackadd \n\r",
        "Add stack task\n\r",
        cmd_stackadd
    },
    {
        "stackdel",
        "stackdel \n\r",
        "Delete stack task\n\r",
        cmd_stackdel
    },
    {
        "dump",
        "dump [type] \n\r",
        "Dump information\n\r",
        cmd_dump
    },
    {
        "tc",
        "tc [case] [count]\n\r",
        "auto test case\n\r",
        cmd_tc
    },
    /* MUST be at the end: */
    {
        0,
        0,
        0,
        0
    }
};
