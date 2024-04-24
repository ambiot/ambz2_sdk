/******************************************************************************
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

/**
  ******************************************************************************
  * @file    wifi_constants.h
  * @author
  * @version
  * @brief   This file provides the data types used for wlan API.
  ******************************************************************************
  */

#ifndef _WIFI_CONSTANTS_H
#define _WIFI_CONSTANTS_H

/** @addtogroup WIFI
 *  @brief      WIFI module
 *  @{
 */

#ifdef	__cplusplus
extern "C" {
#endif

/** @addtogroup WIFI_Exported_Constants WIFI Exported Constants
  * @{
  */


/** @defgroup WLAN_Defs
  * @{
  */
#ifndef WLAN0_NAME
#define WLAN0_NAME		"wlan0" /**< WLAN0 NAME */
#endif
#ifndef WLAN1_NAME
#define WLAN1_NAME		"wlan1" /**< WLAN1 NAME */
#endif
/** @} */

/** @defgroup Security_Defs
  * @{
  */
#define WEP_ENABLED             0x0001                /**< wep enable */
#define TKIP_ENABLED            0x0002                /**< tkip enable */
#define AES_ENABLED             0x0004                /**< aes enable */
#define WSEC_SWFLAG             0x0008                /**< WSEC SWFLAG */
#define AES_CMAC_ENABLED        0x0010                /**< aes cmac enable */
#define ENTERPRISE_ENABLED      0x0020                /**< enterprise enable */

#define SHARED_ENABLED          0x00008000            /**< shared enable */
#define WPA_SECURITY            0x00200000            /**< wpa */
#define WPA2_SECURITY           0x00400000            /**< wpa2 */
#define WPA3_SECURITY           0x00800000            /**< wpa3 */
#define WPS_ENABLED             0x10000000            /**< wps enable*/

#define RTW_WPA2_MAX_PSK_LEN    (64)                  /**< wpa2 maximum psk length */
#define RTW_WPA3_MAX_PSK_LEN    (128)                 /**< wpa3 maximum psk length */
#define RTW_MAX_PSK_LEN         RTW_WPA3_MAX_PSK_LEN  /**< maximum psk length */
#define RTW_MIN_PSK_LEN         (8)                   /**< minimum psk length */
#define RTW_MAX_SSID_LEN        (32)                  /**< maximum ssid length */
#define RTW_MIN_SSID_LEN        (0)                   /**< minimum ssid length */

#define MCSSET_LEN              16                    /**<mcsset length */
/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup WIFI_Exported_Types WIFI Exported Types
  * @{
  */

/** @addtogroup Enums
  * @{
  */

/**
  * @brief  The enumeration lists the results of the function.
  */
typedef enum {
	RTW_SUCCESS                      = 0,    /**< Success */
	RTW_PENDING                      = 1,    /**< Reserved, Pending. */
	RTW_TIMEOUT                      = 2,    /**< Wi-Fi operation times out, such as Wi-Fi connect/disconnect/scan. */
	RTW_PARTIAL_RESULTS              = 3,    /**< Reserved, Partial results. */
	RTW_INVALID_KEY                  = 4,    /**< Wi-Fi password in station or softap mode is not correct. */
	RTW_DOES_NOT_EXIST               = 5,    /**< Reserved, Does not exist. */
	RTW_NOT_AUTHENTICATED            = 6,    /**< Reserved, Not authenticated. */
	RTW_NOT_KEYED                    = 7,    /**< Reserved, Not keyed. */
	RTW_IOCTL_FAIL                   = 8,    /**< Reserved, IOCTL fail. */
	RTW_BUFFER_UNAVAILABLE_TEMPORARY = 9,    /**< Buffer malloc failed. */
	RTW_BUFFER_UNAVAILABLE_PERMANENT = 10,   /**< Reserved, Buffer unavailable permanently. */
	RTW_WPS_PBC_OVERLAP              = 11,   /**< Reserved, WPS PBC overlap. */
	RTW_CONNECTION_LOST              = 12,   /**< Reserved, Connection lost. */

	RTW_ERROR                        = -1,   /**< Generic Error */
	RTW_BADARG                       = -2,   /**< Bad Argument input, such as wrong parameter length or format. */
	RTW_BADOPTION                    = -3,   /**< Reserved, Bad option. */
	RTW_NOTUP                        = -4,   /**< Reserved, Not up. */
	RTW_NOTDOWN                      = -5,   /**< Reserved, Not down. */
	RTW_NOTAP                        = -6,   /**< Reserved, Not AP. */
	RTW_NOTSTA                       = -7,   /**< Reserved, Not STA. */
	RTW_BADKEYIDX                    = -8,   /**< Reserved, BAD Key Index */
	RTW_RADIOOFF                     = -9,   /**< Reserved, Radio Off. */
	RTW_NOTBANDLOCKED                = -10,  /**< Reserved, Not band locked. */
	RTW_NOCLK                        = -11,  /**< Reserved, No Clock. */
	RTW_BADRATESET                   = -12,  /**< Reserved, BAD Rate valueset. */
	RTW_BADBAND                      = -13,  /**< Reserved, BAD Band. */
	RTW_BUFTOOSHORT                  = -14,  /**< Reserved, Buffer too short. */
	RTW_BUFTOOLONG                   = -15,  /**< Reserved, Buffer too long. */
	RTW_BUSY                         = -16,  /**< Wi-Fi connection is in process. */
	RTW_NOTASSOCIATED                = -17,  /**< Reserved, Not Associated */
	RTW_BADSSIDLEN                   = -18,  /**< Invalid SSID len, SSID should be 0-32 characters, BSSID should be 6. */
	RTW_OUTOFRANGECHAN               = -19,  /**< Reserved, Out of Range Channel */
	RTW_BADCHAN                      = -20,  /**< Reserved, Bad Channel */
	RTW_BADADDR                      = -21,  /**< Reserved, Bad Address */
	RTW_NORESOURCE                   = -22,  /**< Not Enough Resources, such as fail to create semaphore. */
	RTW_UNSUPPORTED                  = -23,  /**< Reserved, Unsupported */
	RTW_BADLEN                       = -24,  /**< Reserved, Bad length */
	RTW_NOTREADY                     = -25,  /**< Reserved, Not Ready */
	RTW_EPERM                        = -26,  /**< Reserved, Not Permitted */
	RTW_NOMEM                        = -27,  /**< No Memory, fail to malloc memory. */
	RTW_ASSOCIATED                   = -28,  /**< Reserved, Associated */
	RTW_RANGE                        = -29,  /**< Reserved, Not In Range */
	RTW_NOTFOUND                     = -30,  /**< Fail to find target AP in scan result list. */
	RTW_WME_NOT_ENABLED              = -31,  /**< Reserved, WME Not Enabled */
	RTW_TSPEC_NOTFOUND               = -32,  /**< Reserved, TSPEC Not Found */
	RTW_ACM_NOTSUPPORTED             = -33,  /**< Reserved, ACM Not Supported */
	RTW_NOT_WME_ASSOCIATION          = -34,  /**< Reserved, Not WME Association */
	RTW_SDIO_ERROR                   = -35,  /**< Reserved, SDIO Bus Error */
	RTW_WLAN_DOWN                    = -36,  /**< Reserved, WLAN Not Accessible */
	RTW_BAD_VERSION                  = -37,  /**< Reserved, Incorrect version */
	RTW_TXFAIL                       = -38,  /**< Reserved, TX failure */
	RTW_RXFAIL                       = -39,  /**< Reserved, RX failure */
	RTW_NODEVICE                     = -40,  /**< Reserved, Device not present */
	RTW_UNFINISHED                   = -41,  /**< Reserved, To be finished */
	RTW_NONRESIDENT                  = -42,  /**< Reserved, access to nonresident overlay */
	RTW_DISABLED                     = -43   /**< Reserved, Disabled in this build */
} rtw_result_t;

/**
  * @brief  The enumeration lists the possible security types to set when connection.\n
  *			Station mode supports OPEN, WEP, and WPA2.\n
  *			AP mode support OPEN and WPA2.
  */
typedef enum {
	RTW_SECURITY_OPEN                      = (0),                                                                                /**< Open security                                               */
	RTW_SECURITY_WEP_PSK                   = (WEP_ENABLED),                                                                      /**< WEP Security with open authentication                       */
	RTW_SECURITY_WEP_SHARED                = (WEP_ENABLED | SHARED_ENABLED),                                                     /**< WEP Security with shared authentication                     */
	RTW_SECURITY_WPA_TKIP_PSK              = (WPA_SECURITY | TKIP_ENABLED),                                                      /**< WPA Security with TKIP                                      */
	RTW_SECURITY_WPA_AES_PSK               = (WPA_SECURITY | AES_ENABLED),                                                       /**< WPA Security with AES                                       */
	RTW_SECURITY_WPA_MIXED_PSK             = (WPA_SECURITY | TKIP_ENABLED  | AES_ENABLED),                                       /**< WPA Security with AES & TKIP                                */
	RTW_SECURITY_WPA2_TKIP_PSK             = (WPA2_SECURITY | TKIP_ENABLED),                                                     /**< WPA2 Security with TKIP                                     */
	RTW_SECURITY_WPA2_AES_PSK              = (WPA2_SECURITY | AES_ENABLED),                                                      /**< WPA2 Security with AES                                      */
	RTW_SECURITY_WPA2_MIXED_PSK            = (WPA2_SECURITY | TKIP_ENABLED  | AES_ENABLED),                                      /**< WPA2 Security with AES & TKIP                               */
	RTW_SECURITY_WPA2_AES_CMAC             = (WPA2_SECURITY | AES_CMAC_ENABLED),                                                 /**< WPA2 Security with AES and Management Frame Protection      */
	RTW_SECURITY_WPA_WPA2_TKIP_PSK         = (WPA_SECURITY | WPA2_SECURITY | TKIP_ENABLED),                                      /**< WPA/WPA2 Security with TKIP                                 */
	RTW_SECURITY_WPA_WPA2_AES_PSK          = (WPA_SECURITY | WPA2_SECURITY | AES_ENABLED),                                       /**< WPA/WPA2 Security with AES                                  */
	RTW_SECURITY_WPA_WPA2_MIXED_PSK        = (WPA_SECURITY | WPA2_SECURITY | TKIP_ENABLED | AES_ENABLED),                        /**< WPA/WPA2 Security with AES & TKIP                           */
	RTW_SECURITY_WPA_TKIP_ENTERPRISE       = (WPA_SECURITY | TKIP_ENABLED | ENTERPRISE_ENABLED),                                 /**< WPA Security with TKIP via 802.1X authentication            */
	RTW_SECURITY_WPA_AES_ENTERPRISE        = (WPA_SECURITY | AES_ENABLED | ENTERPRISE_ENABLED),                                  /**< WPA Security with AES via 802.1X authentication             */
	RTW_SECURITY_WPA_MIXED_ENTERPRISE      = (WPA_SECURITY | AES_ENABLED | TKIP_ENABLED | ENTERPRISE_ENABLED),                   /**< WPA Security with AES & TKIP via 802.1X authentication      */
	RTW_SECURITY_WPA2_TKIP_ENTERPRISE      = (WPA2_SECURITY | TKIP_ENABLED | ENTERPRISE_ENABLED),                                /**< WPA2 Security with TKI via 802.1X authentication            */
	RTW_SECURITY_WPA2_AES_ENTERPRISE       = (WPA2_SECURITY | AES_ENABLED | ENTERPRISE_ENABLED),                                 /**< WPA2 Security with AES via 802.1X authentication            */
	RTW_SECURITY_WPA2_MIXED_ENTERPRISE     = (WPA2_SECURITY | AES_ENABLED | TKIP_ENABLED | ENTERPRISE_ENABLED),                  /**< WPA2 Security with AES & TKIP via 802.1X authentication     */
	RTW_SECURITY_WPA_WPA2_TKIP_ENTERPRISE  = (WPA_SECURITY | WPA2_SECURITY | TKIP_ENABLED | ENTERPRISE_ENABLED),                 /**< WPA/WPA2 Security with TKIP via 802.1X authentication       */
	RTW_SECURITY_WPA_WPA2_AES_ENTERPRISE   = (WPA_SECURITY | WPA2_SECURITY | AES_ENABLED | ENTERPRISE_ENABLED),                  /**< WPA/WPA2 Security with AES via 802.1X authentication        */
	RTW_SECURITY_WPA_WPA2_MIXED_ENTERPRISE = (WPA_SECURITY | WPA2_SECURITY | AES_ENABLED | TKIP_ENABLED | ENTERPRISE_ENABLED),   /**< WPA/WPA2 Security with AES & TKIP via 802.1X authentication */

	RTW_SECURITY_WPS_OPEN                  = (WPS_ENABLED),                                                                      /**< WPS with open security                                      */
	RTW_SECURITY_WPS_SECURE                = (WPS_ENABLED | AES_ENABLED),                                                        /**< WPS with AES security                                       */

	RTW_SECURITY_WPA3_AES_PSK              = (WPA3_SECURITY | AES_ENABLED),                                                      /**< WPA3-SAE with AES security                                  */
	RTW_SECURITY_WPA2_WPA3_MIXED           = (WPA2_SECURITY | WPA3_SECURITY | AES_ENABLED),                                      /**< WPA3-SAE/WPA2 with AES security                             */

	RTW_SECURITY_UNKNOWN                   = (-1),                                                                               /**< May be returned by scan function if security is unknown. Do not pass this to the join function! */

	RTW_SECURITY_FORCE_32_BIT              = (0x7fffffff)                                                                        /**< Exists only to force rtw_security_t type to 32 bits        */
} rtw_security_t;

/**
 * @brief The enumeration lists wpa mode
 */
typedef enum {
	WPA_AUTO_MODE,        /**< WPA auto mode*/
	WPA_ONLY_MODE,        /**< WPA only mode*/
	WPA2_ONLY_MODE,       /**< WPA2 only mode*/
	WPA3_ONLY_MODE,       /**< WPA3 only mode*/
	WPA_WPA2_MIXED_MODE,  /**< WPA/WPA2 mode*/
	WPA2_WPA3_MIXED_MODE  /**< WPA2/WPA3 mode*/
} rtw_wpa_mode;

/**
 * @brief The enumeration lists encryption types
 */
typedef enum {
	RTW_ENCRYPTION_UNKNOWN = 0,     /**< unknown encryption type*/
	RTW_ENCRYPTION_OPEN = 1,        /**< open encryption type*/
	RTW_ENCRYPTION_WEP40 = 2,       /**< WEP40 encryption type*/
	RTW_ENCRYPTION_WPA_TKIP = 3,    /**< WPA+TKIP encryption type*/
	RTW_ENCRYPTION_WPA_AES = 4,     /**< WPA+AES encryption type*/
	RTW_ENCRYPTION_WPA2_TKIP = 5,   /**< WPA2+TKIP encryption type*/
	RTW_ENCRYPTION_WPA2_AES = 6,    /**< WPA2+AES encryption type*/
	RTW_ENCRYPTION_WPA2_MIXED = 7,  /**< WPA2+MIXED encryption type*/
	RTW_ENCRYPTION_WEP104 = 9,      /**< WEP104 encryption type*/
	RTW_ENCRYPTION_UNDEF = 0xFF,    /**< undefined encryption type*/
} rtw_encryption_t;

/**
 * @brief The enumeration lists false and true
 */
typedef enum {
	RTW_FALSE = 0, /**< false*/
	RTW_TRUE  = 1  /**< true*/
} rtw_bool_t;

/**
  * @brief  The enumeration lists the band types.
  */
typedef enum {
	RTW_802_11_BAND_5GHZ   = 0, /**< Denotes 5GHz radio band   */
	RTW_802_11_BAND_2_4GHZ = 1  /**< Denotes 2.4GHz radio band */
} rtw_802_11_band_t;

/**
  * @brief  The enumeration lists all the country codes able to set to Wi-Fi driver.
  */
typedef enum {
	/* CHANNEL PLAN */
	RTW_COUNTRY_WORLD1, ///< 0x20
	RTW_COUNTRY_ETSI1,  ///< 0x21
	RTW_COUNTRY_FCC1,   ///< 0x22
	RTW_COUNTRY_MKK1,   ///< 0x23
	RTW_COUNTRY_ETSI2,  ///< 0x24
	RTW_COUNTRY_FCC2,   ///< 0x2A
	RTW_COUNTRY_WORLD2, ///< 0x47
	RTW_COUNTRY_MKK2,   ///< 0x58
	RTW_COUNTRY_GLOBAL, ///< 0x41

	/* SPECIAL */
	RTW_COUNTRY_WORLD,  ///< WORLD1
	RTW_COUNTRY_EU,     ///< ETSI1

	/* JAPANESE */
	RTW_COUNTRY_JP,     ///< MKK1

	/* FCC , 19 countries*/
	RTW_COUNTRY_AS,     ///< FCC2
	RTW_COUNTRY_BM,
	RTW_COUNTRY_CA,
	RTW_COUNTRY_DM,
	RTW_COUNTRY_DO,
	RTW_COUNTRY_FM,
	RTW_COUNTRY_GD,
	RTW_COUNTRY_GT,
	RTW_COUNTRY_GU,
	RTW_COUNTRY_HT,
	RTW_COUNTRY_MH,
	RTW_COUNTRY_MP,
	RTW_COUNTRY_NI,
	RTW_COUNTRY_PA,
	RTW_COUNTRY_PR,
	RTW_COUNTRY_PW,
	RTW_COUNTRY_TW,
	RTW_COUNTRY_US,
	RTW_COUNTRY_VI,

	/* others,  ETSI */
	RTW_COUNTRY_AD,     ///< ETSI1
	RTW_COUNTRY_AE,
	RTW_COUNTRY_AF,
	RTW_COUNTRY_AI,
	RTW_COUNTRY_AL,
	RTW_COUNTRY_AM,
	RTW_COUNTRY_AN,
	RTW_COUNTRY_AR,
	RTW_COUNTRY_AT,
	RTW_COUNTRY_AU,
	RTW_COUNTRY_AW,
	RTW_COUNTRY_AZ,
	RTW_COUNTRY_BA,
	RTW_COUNTRY_BB,
	RTW_COUNTRY_BD,
	RTW_COUNTRY_BE,
	RTW_COUNTRY_BF,
	RTW_COUNTRY_BG,
	RTW_COUNTRY_BH,
	RTW_COUNTRY_BL,
	RTW_COUNTRY_BN,
	RTW_COUNTRY_BO,
	RTW_COUNTRY_BR,
	RTW_COUNTRY_BS,
	RTW_COUNTRY_BT,
	RTW_COUNTRY_BY,
	RTW_COUNTRY_BZ,
	RTW_COUNTRY_CF,
	RTW_COUNTRY_CH,
	RTW_COUNTRY_CI,
	RTW_COUNTRY_CL,
	RTW_COUNTRY_CN,
	RTW_COUNTRY_CO,
	RTW_COUNTRY_CR,
	RTW_COUNTRY_CX,
	RTW_COUNTRY_CY,
	RTW_COUNTRY_CZ,
	RTW_COUNTRY_DE,
	RTW_COUNTRY_DK,
	RTW_COUNTRY_DZ,
	RTW_COUNTRY_EC,
	RTW_COUNTRY_EE,
	RTW_COUNTRY_EG,
	RTW_COUNTRY_ES,
	RTW_COUNTRY_ET,
	RTW_COUNTRY_FI,
	RTW_COUNTRY_FR,
	RTW_COUNTRY_GB,
	RTW_COUNTRY_GE,
	RTW_COUNTRY_GF,
	RTW_COUNTRY_GH,
	RTW_COUNTRY_GL,
	RTW_COUNTRY_GP,
	RTW_COUNTRY_GR,
	RTW_COUNTRY_GY,
	RTW_COUNTRY_HK,
	RTW_COUNTRY_HN,
	RTW_COUNTRY_HR,
	RTW_COUNTRY_HU,
	RTW_COUNTRY_ID,
	RTW_COUNTRY_IE,
	RTW_COUNTRY_IL,
	RTW_COUNTRY_IN,
	RTW_COUNTRY_IQ,
	RTW_COUNTRY_IR,
	RTW_COUNTRY_IS,
	RTW_COUNTRY_IT,
	RTW_COUNTRY_JM,
	RTW_COUNTRY_JO,
	RTW_COUNTRY_KE,
	RTW_COUNTRY_KH,
	RTW_COUNTRY_KN,
	RTW_COUNTRY_KP,
	RTW_COUNTRY_KR,
	RTW_COUNTRY_KW,
	RTW_COUNTRY_KY,
	RTW_COUNTRY_KZ,
	RTW_COUNTRY_LA,
	RTW_COUNTRY_LB,
	RTW_COUNTRY_LC,
	RTW_COUNTRY_LI,
	RTW_COUNTRY_LK,
	RTW_COUNTRY_LR,
	RTW_COUNTRY_LS,
	RTW_COUNTRY_LT,
	RTW_COUNTRY_LU,
	RTW_COUNTRY_LV,
	RTW_COUNTRY_MA,
	RTW_COUNTRY_MC,
	RTW_COUNTRY_MD,
	RTW_COUNTRY_ME,
	RTW_COUNTRY_MF,
	RTW_COUNTRY_MK,
	RTW_COUNTRY_MN,
	RTW_COUNTRY_MO,
	RTW_COUNTRY_MQ,
	RTW_COUNTRY_MR,
	RTW_COUNTRY_MT,
	RTW_COUNTRY_MU,
	RTW_COUNTRY_MV,
	RTW_COUNTRY_MW,
	RTW_COUNTRY_MX,
	RTW_COUNTRY_MY,
	RTW_COUNTRY_NG,
	RTW_COUNTRY_NL,
	RTW_COUNTRY_NO,
	RTW_COUNTRY_NP,
	RTW_COUNTRY_NZ,
	RTW_COUNTRY_OM,
	RTW_COUNTRY_PE,
	RTW_COUNTRY_PF,
	RTW_COUNTRY_PG,
	RTW_COUNTRY_PH,
	RTW_COUNTRY_PK,
	RTW_COUNTRY_PL,
	RTW_COUNTRY_PM,
	RTW_COUNTRY_PT,
	RTW_COUNTRY_PY,
	RTW_COUNTRY_QA,
	RTW_COUNTRY_RS,
	RTW_COUNTRY_RU,
	RTW_COUNTRY_RW,
	RTW_COUNTRY_SA,
	RTW_COUNTRY_SE,
	RTW_COUNTRY_SG,
	RTW_COUNTRY_SI,
	RTW_COUNTRY_SK,
	RTW_COUNTRY_SN,
	RTW_COUNTRY_SR,
	RTW_COUNTRY_SV,
	RTW_COUNTRY_SY,
	RTW_COUNTRY_TC,
	RTW_COUNTRY_TD,
	RTW_COUNTRY_TG,
	RTW_COUNTRY_TH,
	RTW_COUNTRY_TN,
	RTW_COUNTRY_TR,
	RTW_COUNTRY_TT,
	RTW_COUNTRY_TZ,
	RTW_COUNTRY_UA,
	RTW_COUNTRY_UG,
	RTW_COUNTRY_UY,
	RTW_COUNTRY_UZ,
	RTW_COUNTRY_VC,
	RTW_COUNTRY_VE,
	RTW_COUNTRY_VN,
	RTW_COUNTRY_VU,
	RTW_COUNTRY_WF,
	RTW_COUNTRY_WS,
	RTW_COUNTRY_YE,
	RTW_COUNTRY_YT,
	RTW_COUNTRY_ZA,
	RTW_COUNTRY_ZW,

	RTW_COUNTRY_MAX

} rtw_country_code_t;

/**
  * @brief  The enumeration lists the adaptivity types.
  */
typedef enum {
	RTW_ADAPTIVITY_DISABLE = 0,   /**< Disable adaptivity */
	RTW_ADAPTIVITY_NORMAL,        /**< Enable adaptivity */
	RTW_ADAPTIVITY_CARRIER_SENSE  /**< Enable adaptivity carrier sense*/
} rtw_adaptivity_mode_t;

/**
  * @brief  The enumeration lists the trp_tis types.
  */
typedef enum {
	RTW_TRP_TIS_DISABLE = 0,   /**< Disable */
	RTW_TRP_TIS_NORMAL,        /**< Enable */
	RTW_TRP_TIS_DYNAMIC,       /**< Enable dynamic mechanism */
	RTW_TRP_TIS_FIX_ACK_RATE,  /**< Fix ack rate to 6M */
} rtw_trp_tis_mode_t;

/**
  * @brief  The enumeration lists the supported operation mode by WIFI driver,
  *			including station and AP mode.
  */
typedef enum {
	RTW_MODE_NONE = 0,  /**< None */
	RTW_MODE_STA,       /**< STA mode */
	RTW_MODE_AP,        /**< AP mode */
	RTW_MODE_STA_AP,    /**< AP and STA mode */
	RTW_MODE_PROMISC,   /**< Promisc mode */
	RTW_MODE_P2P        /**< P2P mode */
} rtw_mode_t;

typedef enum {
	RTW_SCAN_FULL = 0,
	RTW_SCAN_SOCIAL,
	RTW_SCAN_ONE
} rtw_scan_mode_t;

/**
  * @brief  The enumeration lists the supported autoreconnect mode by WIFI driver.
  */
typedef enum {
	RTW_AUTORECONNECT_DISABLE = 0,  /**< Disable the autoreconnect mode */
	RTW_AUTORECONNECT_FINITE,       /**< Enable the autoreconnect mode */
	RTW_AUTORECONNECT_INFINITE      /**< Enable the autoreconnect mode with infinite retry times */
} rtw_autoreconnect_mode_t;

/**
  * @brief  The enumeration lists the status to describe the connection link.
  */
typedef enum {
	RTW_LINK_DISCONNECTED = 0,  /**< Disconnect status */
	RTW_LINK_CONNECTED          /**< Connect status */
} rtw_link_status_t;

/**
  * @brief  The enumeration lists the scan types.
  */
typedef enum {
	RTW_SCAN_TYPE_ACTIVE              = 0x00,  /**< Actively scan a network by sending 802.11 probe(s)         */
	RTW_SCAN_TYPE_PASSIVE             = 0x01,  /**< Passively scan a network by listening for beacons from APs */
} rtw_scan_type_t;

/**
  * @brief  The enumeration lists the bss types.
  */
typedef enum {
	RTW_BSS_TYPE_INFRASTRUCTURE = 0, /**< Denotes infrastructure network                  */
	RTW_BSS_TYPE_ADHOC          = 1, /**< Denotes an 802.11 ad-hoc IBSS network           */
	RTW_BSS_TYPE_ANY            = 2, /**< Denotes either infrastructure or ad-hoc network */

	RTW_BSS_TYPE_UNKNOWN        = -1 /**< May be returned by scan function if BSS type is unknown. Do not pass this to the Join function */
} rtw_bss_type_t;

/**
  * @brief  The enumeration lists the scan command type.
  */
typedef enum {
	RTW_SCAN_COMMAMD = 0x01  /**< Scan command */
} rtw_scan_command_t;

/**
  * @brief  The enumeration lists the command type.
  */
typedef enum {
	COMMAND1 = 0x01  /**< COMMAND1 */
} rtw_command_type;

/**
  * @brief  The enumeration lists the WPS types.
  */
typedef enum {
	RTW_WPS_TYPE_DEFAULT                = 0x0000,  /**< default type */
	RTW_WPS_TYPE_USER_SPECIFIED         = 0x0001,  /**< user specified type */
	RTW_WPS_TYPE_MACHINE_SPECIFIED      = 0x0002,  /**< machine specified type */
	RTW_WPS_TYPE_REKEY                  = 0x0003,  /**< retry key type */
	RTW_WPS_TYPE_PUSHBUTTON             = 0x0004,  /**< push button type */
	RTW_WPS_TYPE_REGISTRAR_SPECIFIED    = 0x0005,  /**< register specified type */
	RTW_WPS_TYPE_NONE                   = 0x0006,  /**< none */
	RTW_WPS_TYPE_WSC                    = 0x0007   /**< wsc type */
} rtw_wps_type_t;

/**
  * @brief  The enumeration lists all the network bgn mode.
  */
typedef enum {
	RTW_NETWORK_B   = 1,  /**< b mode */
	RTW_NETWORK_BG  = 3,  /**< bg mode */
	RTW_NETWORK_BGN = 11  /**< bgn mode */
} rtw_network_mode_t;

/**
  * @brief  The enumeration lists the interfaces.
  */
typedef enum {
	RTW_STA_INTERFACE     = 0, /**< STA or Client Interface  */
	RTW_AP_INTERFACE      = 1, /**< SoftAP Interface         */
} rtw_interface_t;

/**
  * @brief  The enumeration lists the packet filter rules.
  */
typedef enum {
	RTW_POSITIVE_MATCHING  = 0, /**< Receive the data matching with this pattern and discard the other data   */
	RTW_NEGATIVE_MATCHING  = 1  /**< Discard the data matching with this pattern and receive the other data */
} rtw_packet_filter_rule_t;

/**
  * @brief  The enumeration lists the promisc levels.
  */
typedef enum {
	RTW_PROMISC_DISABLE = 0,  /**< Disable the promisc */
	RTW_PROMISC_ENABLE = 1,   /**< Fetch all ethernet packets */
	RTW_PROMISC_ENABLE_1 = 2, /**< Fetch only B/M packets */
	RTW_PROMISC_ENABLE_2 = 3, /**< Fetch all 802.11 packets*/
	RTW_PROMISC_ENABLE_3 = 4, /**< Fetch only B/M 802.11 packets*/
	RTW_PROMISC_ENABLE_4 = 5, /**< Fetch all 802.11 packets & MIMO PLCP headers. Please note that the PLCP header would be struct rtw_rx_info_t defined in wifi_structures.h*/
} rtw_rcr_level_t;

/**
  * @brief  The enumeration lists the promisc rx type.
  */
#if defined(CONFIG_UNSUPPORT_PLCPHDR_RPT) && CONFIG_UNSUPPORT_PLCPHDR_RPT
typedef enum {
	RTW_RX_NORMAL = 0,     /**< The supported 802.11 packet*/
	RTW_RX_UNSUPPORT = 1,  /**< Unsupported 802.11 packet info */
} rtw_rx_type_t;
#endif

/**
  * @brief  The enumeration lists the disconnect reasons.
  */
typedef enum {
	RTW_NO_ERROR = 0,               /**< no error */
	RTW_NONE_NETWORK = 1,           /**< none network */
	RTW_CONNECT_FAIL = 2,           /**< connect fail */
	RTW_WRONG_PASSWORD = 3,         /**< wrong password */
	RTW_4WAY_HANDSHAKE_TIMEOUT = 4, /**< 4 way handshake timeout*/
	RTW_DHCP_FAIL = 5,              /**< dhcp fail*/
	RTW_AUTH_FAIL = 6,              /**< auth fail */
	RTW_ASSOC_FAIL = 7,             /**< association fail */
	RTW_DEAUTH_DEASSOC = 8,         /**< deauth or deassoc */
	RTW_UNKNOWN,
} rtw_connect_error_flag_t;

/**
  * @brief The enumeration lists the power status.
  */
typedef enum {
	RTW_TX_PWR_PERCENTAGE_100 = 0,  /**< 100%, default target output power. */
	RTW_TX_PWR_PERCENTAGE_75 = 1,   /**< 75% */
	RTW_TX_PWR_PERCENTAGE_50 = 2,   /**< 50% */
	RTW_TX_PWR_PERCENTAGE_25 = 3,   /**< 25% */
	RTW_TX_PWR_PERCENTAGE_12_5 = 4, /**< 12.5% */
} rtw_tx_pwr_percentage_t;

/**
  * @brief  The enumeration is event type indicated from wlan driver.
  */
typedef enum {
	WIFI_EVENT_CONNECT = 0,                /**< Indicate wifi connect */
	WIFI_EVENT_DISCONNECT = 1,             /**< Indicate wifi disconnect */
	WIFI_EVENT_FOURWAY_HANDSHAKE_DONE = 2, /**< Indicate 4-Way Handshake success */
	WIFI_EVENT_SCAN_RESULT_REPORT = 3,     /**< Indicate to get the scan result */
	WIFI_EVENT_SCAN_DONE = 4,              /**< Indicate scan complete */
	WIFI_EVENT_RECONNECTION_FAIL = 5,      /**< Indicate wifi reconnection fail */
	WIFI_EVENT_SEND_ACTION_DONE = 6,       /**< Indicate send action done, only p2p has implementation */
	WIFI_EVENT_RX_MGNT = 7,                /**< Indicate mgnt and data frame to uplayer, need to set indicate mode by wifi_set_indicate_mgnt() */
	WIFI_EVENT_STA_ASSOC = 8,              /**< Indicate station associate to softAP */
	WIFI_EVENT_STA_DISASSOC = 9,           /**< Indicate station disassociate to softAP */
	WIFI_EVENT_STA_WPS_START = 10,         /**< Indicate station WPS start */
	WIFI_EVENT_WPS_FINISH = 11,            /**< Indicate WPS finish */
	WIFI_EVENT_EAPOL_START = 12,           /**< Indicate EAPOL start */
	WIFI_EVENT_EAPOL_RECVD = 13,           /**< Indicate EAPOL received  */
	WIFI_EVENT_NO_NETWORK = 14,            /**< Indicate no assoc network after scan done */
	WIFI_EVENT_BEACON_AFTER_DHCP = 15,     /**< Indicate received beacon after DHCP */
	WIFI_EVENT_IP_CHANGED = 16,            /**< Indicate IP address changed */
	WIFI_EVENT_ICV_ERROR = 17,             /**< Indicate ICV error */
	WIFI_EVENT_CHALLENGE_FAIL = 18,        /**< Indicate auth challenge fail */
	WIFI_EVENT_STA_START = 19,             /**< Reserved, Indicate station start */
	WIFI_EVENT_STA_STOP = 20,              /**< Reserved, Indicate station stop */
	WIFI_EVENT_AP_START = 21,              /**< Reserved, Indicate softAP start */
	WIFI_EVENT_AP_STOP = 22,               /**< Reserved, Indicate softAP stop */
	WIFI_EVENT_STA_GOT_IP = 23,            /**< Reserved, Indicate station got IP address */
	WIFI_EVENT_STA_LOST_IP = 24,           /**< Reserved, Indicate station lost IP address */
	WIFI_EVENT_NO_BEACON = 25,             /**< Indicate no beacon received in a period of time (around 2s) */
	WIFI_EVENT_TARGET_SSID_RSSI = 26,      /**< Indicate to get the RSSI value of the target SSID */
	WIFI_EVENT_DHCP_RENEW = 27,            /**< Reserved, Indicate DHCP renew */
	WIFI_EVENT_SWITCH_CHANNE = 28,         /**< Indicate CSA switch channel */
	WIFI_EVENT_MAX,
} rtw_event_indicate_t;

/**
  * @brief  The enumeration lists the AMPDU types
  */
typedef enum {
	RTW_TX_AMPDU = 0,
	RTW_RX_AMPDU,
} rtw_ampdu_mode_t;

/**
  * @brief  The enumeration lists the packet types
  */
typedef enum {
	RTW_MULTICAST_PKT = 0,
	RTW_BROADCAST_PKT
} rtw_bc_mc_t;
/**
  * @}
  */

/**
  * @}
  */

#ifdef	__cplusplus
}
#endif

/**
  * @}
  */

#endif /* _WIFI_CONSTANTS_H */
