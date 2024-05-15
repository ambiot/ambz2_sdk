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
 */

/**
  ******************************************************************************
  * @file    wifi_conf.h
  * @author
  * @version
  * @brief   This file provides user interface for Wi-Fi station and AP mode configuration
  *             base on the functionalities provided by Realtek Wi-Fi driver.
  ******************************************************************************
  */
#ifndef __WIFI_API_H
#define __WIFI_API_H

/** @addtogroup WIFI
 *  @brief      WIFI module
 *  @{
 */

#include "wifi_constants.h"
#include "wifi_structures.h"
#include "wifi_util.h"
#include "wifi_ind.h"
#include <platform/platform_stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup WIFI_Exported_Constants WIFI Exported Constants
 *  @{
 */
/******************************************************
 *                    Macros
 ******************************************************/

/** @defgroup API_INFO_Defs
 *  @{
 */

/**
 * @brief Create RTW_ENABLE_API_INFO
 */
#define RTW_ENABLE_API_INFO

/**
 * @brief Create RTW_API_INFO
 */
#ifdef RTW_ENABLE_API_INFO
#if defined(CONFIG_MBED_ENABLED)
extern __u32 GlobalDebugEnable;
#define RTW_API_INFO(...)     do {\
        if (GlobalDebugEnable) \
            printf(__VA_ARGS__);\
    }while(0)
#else
#define RTW_API_INFO printf
#endif
#else
#define RTW_API_INFO(args)
#endif
/** @} */


/** @defgroup MAC_Defs
  * @{
  */
/**
 * @brief mac address format
 */
#define MAC_ARG(x) ((u8*)(x))[0],((u8*)(x))[1],((u8*)(x))[2],((u8*)(x))[3],((u8*)(x))[4],((u8*)(x))[5]

/**
 * @brief compare mac address
 */
#define CMP_MAC( a, b )  (((a[0])==(b[0]))&& \
                          ((a[1])==(b[1]))&& \
                          ((a[2])==(b[2]))&& \
                          ((a[3])==(b[3]))&& \
                          ((a[4])==(b[4]))&& \
                          ((a[5])==(b[5])))

/**
 * @brief mac format
 */
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
/** @} */


/** @defgroup SCAN_Defs
  * @{
  */
/**
 * @brief scan longest wait time
 */
#define SCAN_LONGEST_WAIT_TIME  (4500)

/**
 * @brief enable for partial channel scan
 */
#define PSCAN_ENABLE 0x01

/**
 * @brief set to select scan time to FAST_SURVEY_TO, otherwise SURVEY_TO
 */
#define PSCAN_FAST_SURVEY 0x02

/**
 * @brief set to select scan time to FAST_SURVEY_TO and resend probe request
 */
#define PSCAN_SIMPLE_CONFIG   0x04
/** @} */

/**
  * @}
  */

/** @addtogroup WIFI_Exported_Types WIFI Exported Types
  * @{
  */

/** @addtogroup Enums
  * @{
  */
/******************************************************
 *                   Enumeration
 ******************************************************/
/**
 * @brief  The enumeration is wl band type.
 */
typedef enum _WL_BAND_TYPE {
	WL_BAND_2_4G = 0,       ///<2.4g band
	WL_BAND_5G,             ///<5g band
	WL_BAND_2_4G_5G_BOTH,   ///<2.4g&5g band
	WL_BANDMAX              ///<max band
} WL_BAND_TYPE, *PWL_BAND_TYPE;
/**
  * @}
  */

/** @addtogroup Structs
  * @{
  */
/******************************************************
 *                 Type Definitions
 ******************************************************/

/** Scan result callback function pointer type
 *
 * @param result_ptr  : A pointer to the pointer that indicates where to put the next scan result
 * @param user_data   : User provided data
 */
typedef void (*rtw_scan_result_callback_t)(rtw_scan_result_t **result_ptr, void *user_data);
typedef rtw_result_t (*rtw_scan_result_handler_t)(rtw_scan_handler_result_t *malloced_scan_result);

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct {
	char *buf;
	int buf_len;
} scan_buf_arg;

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct internal_scan_handler {
	rtw_scan_result_t **pap_details;
	rtw_scan_result_t *ap_details;
	int	scan_cnt;
	rtw_bool_t	scan_complete;
	unsigned char	max_ap_size;
	rtw_scan_result_handler_t gscan_result_handler;
#if defined(SCAN_USE_SEMAPHORE) && SCAN_USE_SEMAPHORE
	void *scan_semaphore;
#else
	int 	scan_running;
#endif
	void	*user_data;
	unsigned int	scan_start_time;
} internal_scan_handler_t;

typedef struct {
	rtw_network_info_t	network_info;
	void *join_sema;
} internal_join_result_t;
/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup WIFI_Exported_Functions WIFI Exported Functions
  * @{
  */
/******************************************************
 *               Function Declarations
 ******************************************************/
/**
 * @brief      Initialize Realtek WiFi API System.
 *                - Initialize the required parts of the software platform.
 *                  i.e. worker, event registering, semaphore, etc.
 *                - Initialize the RTW API thread which handles the asynchronous event.
 * @return     RTW_SUCCESS: if initialization is successful
 * @note       API takes effect only when enable CONFIG_WIFI_IND_USE_THREAD.
 */
int wifi_manager_init(void);

/**
 * @brief      Join a Wi-Fi network.
 *             Scan for, associate and authenticate with a Wi-Fi network.
 *             On successful return, the system is ready to send data packets.
 *
 * @param[in]  ssid: A null terminated string containing the SSID name of the network to join.
 *             The data length of string pointed by ssid should not exceed 32.
 * @param[in]  security_type: Authentication type:
 *                         - RTW_SECURITY_OPEN            - Open Security
 *                         - RTW_SECURITY_WEP_PSK         - WEP Security with open authentication
 *                         - RTW_SECURITY_WEP_SHARED      - WEP Security with shared authentication
 *                         - RTW_SECURITY_WPA_AES_PSK     - WPA Security using AES cipher
 *                         - RTW_SECURITY_WPA_TKIP_PSK    - WPA Security using TKIP cipher
 *                         - RTW_SECURITY_WPA_MIXED_PSK   - WPA Security using AES and/or TKIP ciphers
 *                         - RTW_SECURITY_WPA2_AES_PSK    - WPA2 Security using AES cipher
 *                         - RTW_SECURITY_WPA2_TKIP_PSK   - WPA2 Security using TKIP cipher
 *                         - RTW_SECURITY_WPA2_MIXED_PSK  - WPA2 Security using AES and/or TKIP ciphers
 *                         - RTW_SECURITY_WPA3_AES_PSK    - WPA3-SAE Security using AES cipher
 *                         - RTW_SECURITY_WPA2_WPA3_MIXED - WPA3-SAE/WPA2 Security using AES cipher
 * @param[in]  password: A byte array containing the cleartext security key for the network.\n
 *             For WEP security, the data length of string pointed by password should be 5/10/13/26.\n
 *             For WPA/WPA2 security, the data length of string pointed by password should be between 8 and 64.\n
 *             For WPA3 security, the data length of string pointed by password should be between 8 and 128.
 * @param[in]  ssid_len: The length of the SSID in bytes.
 * @param[in]  password_len: The length of the security_key in bytes.
 * @param[in]  key_id: The index of the wep key (0, 1, 2, or 3). If not using it, leave it with value -1.
 * @param[in]  semaphore: A user provided semaphore that is flagged when the join is complete. If not using it, leave it with NULL value.
 * @return     RTW_SUCCESS: if the system is joined and ready to send data packets.
 *             RTW_ERROR: WiFi internal error.
 *             RTW_BADARG: Bad Argument input, such as wrong parameter length or format.
 *             RTW_BADSSIDLEN: Invalid SSID len, valid SSID length range is 1-32.
 *             RTW_BUSY: Wi-Fi connection is in process.
 *             RTW_INVALID_KEY: Wi-Fi password is not correct.
 *             RTW_NOMEM: No Memory, fail to malloc memory.
 *             RTW_NORESOURCE: Not Enough Resources, such as fail to create semaphore.
 *             RTW_TIMEOUT: Wi-Fi operation times out, such as Wi-Fi connect/disconnect/scan. (@ref rtw_result_t)
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_connect(
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore);

/**
 * @brief      Join a Wi-Fi network with specified BSSID.
 *             Scan for, associate and authenticate with a Wi-Fi network.
 *             On successful return, the system is ready to send data packets.
 * @param[in]  bssid: The specified BSSID to connect.
 *             The data length of string pointed by bssid should be ETH_ALEN (6).
 * @param[in]  ssid: A null terminated string containing the SSID name of the network to join.
 *             The data length of string pointed by ssid should not exceed 32.
 * @param[in]  security_type: Authentication type:
 *                         - RTW_SECURITY_OPEN            - Open Security
 *                         - RTW_SECURITY_WEP_PSK         - WEP Security with open authentication
 *                         - RTW_SECURITY_WEP_SHARED      - WEP Security with shared authentication
 *                         - RTW_SECURITY_WPA_AES_PSK     - WPA Security using AES cipher
 *                         - RTW_SECURITY_WPA_TKIP_PSK    - WPA Security using TKIP cipher
 *                         - RTW_SECURITY_WPA_MIXED_PSK   - WPA Security using AES and/or TKIP ciphers
 *                         - RTW_SECURITY_WPA2_AES_PSK    - WPA2 Security using AES cipher
 *                         - RTW_SECURITY_WPA2_TKIP_PSK   - WPA2 Security using TKIP cipher
 *                         - RTW_SECURITY_WPA2_MIXED_PSK  - WPA2 Security using AES and/or TKIP ciphers
 *                         - RTW_SECURITY_WPA3_AES_PSK    - WPA3-SAE Security using AES cipher
 *                         - RTW_SECURITY_WPA2_WPA3_MIXED - WPA3-SAE/WPA2 Security using AES cipher
 * @param[in]  password: A byte array containing the cleartext security key for the network.\n
 *             For WEP security, the data length of string pointed by password should be 5/10/13/26.\n
 *             For WPA/WPA2 security, the data length of string pointed by password should be between 8 and 64.\n
 *             For WPA3 security, the data length of string pointed by password should be between 8 and 128.
 * @param[in]  ssid_len: The length of the SSID in bytes.
 * @param[in]  password_len: The length of the security_key in bytes.
 * @param[in]  key_id: The index of the wep key.
 * @param[in]  semaphore: A user provided semaphore that is flagged when the join is complete.
 * @return     RTW_SUCCESS: if the system is joined and ready to send data packets.
 * @return     RTW_ERROR: WiFi internal errorl.
 * @return     RTW_BADSSIDLEN: Invalid SSID len, SSID should be 0-32 characters, BSSID should be 6.
 * @return     RTW_BUSY: Wi-Fi connection is in process.
 * @return     RTW_INVALID_KEY: Wi-Fi password is not correct.
 * @return     RTW_NOMEM: No Memory, fail to malloc memory.
 * @return     RTW_NORESOURCE: Not Enough Resources, such as fail to create semaphore.
 * @return     RTW_TIMEOUT: Join bss timeout. (@ref rtw_result_t)
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       The difference between @ref wifi_connect_bssid() and @ref wifi_connect() is that BSSID has higher priority as the basis of connection in @ref wifi_connect_bssid.
 */
int wifi_connect_bssid(
	unsigned char 		bssid[ETH_ALEN],
	char 				*ssid,
	rtw_security_t	security_type,
	char 				*password,
	int 				bssid_len,
	int 				ssid_len,
	int 				password_len,
	int 				key_id,
	void 				*semaphore);

/**
 * @brief      Disassociate from current Wi-Fi network.
 * @param      None
 * @return     RTW_SUCCESS: On successful disassociation from the AP.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_disconnect(void);

/**
* @brief      Check if Wi-Fi is connected to AP before dhcp.
* @param      None
* @return     RTW_SUCCESS: If Wi-Fi is connected to AP.
* @return     RTW_ERROR: WiFi internal error.
* @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
*/
int wifi_is_connected_to_ap(void);

/**
* @brief      Set partial scan retry times when PSCAN_FAST_SURVEY is set. Default is 7.
* @param[in]  times: partial scan retry times.
* @return     None.
*/
void wifi_set_partial_scan_retry_times(unsigned char times);

/**
 * @brief      Check if the specified interface is up.
 * @param[in]  interface: The interface can be set as RTW_STA_INTERFACE or RTW_AP_INTERFACE. (@ref rtw_interface_t)
 * @return     1: The interface is up.
 * @return     0: The interface is not up.
 */
int wifi_is_up(rtw_interface_t interface);

/**
 * @brief      Determines if a particular interface is ready to transceive ethernet packets.
 * @param[in]  interface: The interface can be set as RTW_STA_INTERFACE or RTW_AP_INTERFACE. (@ref rtw_interface_t)
 * @return     RTW_SUCCESS: if the interface is ready to transceive ethernet packets.
 * @return     RTW_ERROR: if the interface is not ready to transceive ethernet packets.
 */
int wifi_is_ready_to_transceive(rtw_interface_t interface);

/**
 * @brief      This function sets the current Media Access Control (MAC) address of the 802.11 device.
 * @param[in]  mac: Wi-Fi MAC address to set to. Format of MAC address do not need to include colons/separators (Eg: "a1b2c3d4e5f6")
 * @return     RTW_SUCCESS: if the MAC address is successfully set.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       This MAC address will be applied to WLAN0.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_mac_address(char *mac);

/**
 * @brief      Retrieves the current Media Access Control (MAC) address (or Ethernet hardware address) of the 802.11 device.
 * @param[out] mac: Pointer to the obtained mac address. Memory allocated for "mac" must be at least 18 bytes.
 *             The output format of "mac" will be the obtained MAC address in a string, with separators (Eg. "a1:b2:c3:d4:e5:f6")
 * @return     RTW_SUCCESS: if the MAC address is successfully obtained.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       This MAC address is read from WLAN0.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_mac_address(char *mac);

/**
 * @brief      Enable Wi-Fi powersave mode.
 * @param      None
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Both LPS and IPS will be enabled.
 * @note       SoftAP and concurrent mode will not enter powersave mode.
 */
int wifi_enable_powersave(void);

/**
 * @brief      Resume to the last Wi-Fi powersave mode which recorded in wifi driver.
 * @param      None
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: interface is not ready.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       SoftAP and concurrent mode will not enter powersave mode.
 */
int wifi_resume_powersave(void);

/**
 * @brief      Disable Wi-Fi powersave mode.
 * @param      None
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: interface is not ready.
 * @note       Both LPS and IPS will be disabled.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_disable_powersave(void);

/**
 * @brief      Get the associated clients with SoftAP.
 * @param[out] client_list_buffer: The location where the client list will be stored.
 * @param[in]  buffer_length: The buffer length is reserved for future use.
 *                Currently, buffer length is set to a fixed value: 25.
 * @return     RTW_SUCCESS: The result is successfully obtained.
 * @return     RTW_ERROR: client list is not ready.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the softAP is started before invoking this function. (@ref wifi_start_ap() or wifi_start_ap_with_hidden_ssid())
 */
int wifi_get_associated_client_list(void *client_list_buffer, unsigned short buffer_length);

/**
 * @brief      Get connected AP's BSSID.
 * @param[out] bssid: the location where the AP BSSID will be stored. The data length of string pointed by bssid should be ETH_ALEN (6).
 * @return     RTW_SUCCESS: The result is successfully obtained.
 * @return     RTW_ERROR: Wi-Fi connection is not ready.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the STA interface is associated with AP before invoking this function. (@ref wifi_connect() or wifi_connect_bssid())
 */
int wifi_get_ap_bssid(unsigned char *bssid);

/**
 * @brief      Get the SoftAP information.
 * @param[out] ap_info: The location where the AP info will be stored. (@ref rtw_bss_info_t)
 * @param[out] security: The location where the security type will be stored. (@ref rtw_security_t)
 * @return     RTW_SUCCESS: If the result is successfully obtained.
 * @return     RTW_ERROR: interface is not ready.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the softAP is started before invoking this function. (@ref wifi_start_ap() or wifi_start_ap_with_hidden_ssid())
 */
int wifi_get_ap_info(rtw_bss_info_t *ap_info, rtw_security_t *security);

/**
 * @brief      Set the country code to driver which is used to determine the channel set.
 * @param[in]  country_code: Specify the country code. (@ref rtw_country_code_t)
 * @return     RTW_SUCCESS: If the country code is successfully set.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_country(rtw_country_code_t country_code);

/**
 * @brief      Get the country code from driver.
 * @param[out] country_code: The location where the country code will be stored. (@ref rtw_country_code_t)
 * @return     RTW_SUCCESS: If the country code is successfully obtained.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_country(rtw_country_code_t *country_code);

/**
 * @brief      Get sta mode MAX data rate.
 * @param[out] inidata_rate: The location where the MAX data rate will be stored.
 * @return     RTW_SUCCESS: If the INIDATA_RATE is successfully obtained.
 * @return     RTW_ERROR: interface is not under WIFI_AP_STATE mode.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the STA interface is associated with AP before invoking this function. (@ref wifi_connect() or wifi_connect_bssid())
 * @note       inidata_rate = 2 * (data rate), you need to calculate the real rate by inidata_rate/2.0.
 */
int wifi_get_sta_max_data_rate(__u8 *inidata_rate);

/**
 * @brief      Retrieve the latest RSSI value.
 * @param[out] pRSSI: The location where the RSSI value will be stored.
 * @return     RTW_SUCCESS: if the RSSI is successfully obtained.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the WLAN0 interface is ready to transceive ethernet packets before invoking this function.
 *             (@ref wifi_connect() or wifi_connect_bssid() or wifi_start_ap() or wifi_start_ap_with_hidden_ssid())
 */
int wifi_get_rssi(int *pRSSI);

/**
 * @brief      Retrieve the latest SNR value.
 * @param[out] pRSSI: The location where the SNR value will be stored.
 * @return     RTW_SUCCESS: If the SNR is successfully obtained.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the WLAN0 interface is ready to transceive ethernet packets before invoking this function.
 *             (@ref wifi_connect() or wifi_connect_bssid() or wifi_start_ap() or wifi_start_ap_with_hidden_ssid())
 */
int wifi_get_snr(int *pSNR);

/**
 * @brief      Set the listening channel for promiscuous mode.
 *             Promiscuous mode will receive all the packets in this channel.
 * @param[in]  channel: The desired channel.
 * @return     RTW_SUCCESS: If the channel is successfully set.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Do NOT need to call this function for STA mode wifi driver, since it will determine the channel from received beacon.
 */
int wifi_set_channel(int channel);

/**
 * @brief      Get the current channel on STA interface(WLAN0_NAME).
 * @param[out] channel: The location where the channel value will be stored.
 * @return     RTW_SUCCESS: If the channel is successfully read.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the STA interface is associated with AP before invoking this function. (@ref wifi_connect() or wifi_connect_bssid())
 */
int wifi_get_channel(int *channel);

/**
 * @brief      Switch the current channelPlan on STA or AP interface.
 * @param[in]  channel_plan: the available channelPlan Map index
 *                           from 0x20 to 0x79
 * @return     RTW_SUCCESS: If the channel is successfully read.
 * @return     RTW_ERROR: If the channel is not successfully read.
 */
int wifi_change_channel_plan(uint8_t channel_plan);

/**
 * @brief      Register interest in a multicast address.\n
 *             Once a multicast address has been registered, all packets detected on the medium destined for that address are forwarded to the host.
 *             Otherwise they are ignored.
 * @param[in]  mac: Ethernet MAC address
 * @return     RTW_SUCCESS: If the address is registered successfully.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_register_multicast_address(rtw_mac_t *mac);

/**
 * @brief      Unregister interest in a multicast address.\n
 *             Once a multicast address has been unregistered, all packets detected on the medium destined for that address are ignored.
 * @param[in]  mac: Ethernet MAC address
 * @return     RTW_SUCCESS: If the address is unregistered successfully.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_unregister_multicast_address(rtw_mac_t *mac);

/**
 * @brief      Setup the adaptivity mode.
 *             The API is called in wifi_on() before WiFi initialization.
 *             You can replace this weak function by the same name function to setup adaptivity mode you want.
 * @param      None
 * @return     None
 */
_WEAK void wifi_set_mib(void);

/**
 * @brief      Setup country code.
 *             The API is called in wifi_on() after WiFi initialization.
 *             You can replace this weak function by the same name function to setup country code you want.
 * @param      None
 * @return     None
 */
_WEAK void wifi_set_country_code(void);

/**
 * @brief      Enable Wi-Fi RF.
 * @param      None
 * @return     RTW_SUCCESS: if success.
 * @return     RTW_ERROR: netif is DOWN.
 * @return     RTW_BADARG: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       The difference between @ref wifi_rf_on() and @ref wifi_on() is that @ref wifi_rf_on() simply enable RF HAL, it does not enable the driver or allocate memory.
 */
int wifi_rf_on(void);

/**
 * @brief      Disable Wi-Fi RF.
 * @param      None
 * @return     RTW_SUCCESS: if success.
 * @return     RTW_ERROR: other WiFi internal errors.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       The difference between @ref wifi_rf_off() and @ref wifi_off() is that @ref wifi_rf_off() simply disable RF HAL, the driver and used heap memory will NOT be released.
 */
int wifi_rf_off(void);

/**
 * @brief      Enable Wi-Fi.
 *                - Bring the Wireless interface "Up".
 *                - Initialize the driver thread which arbitrates access to the SDIO/SPI bus.
 *                - Please use wifi_off() or wifi_set_mode() if need to enable Wi-Fi in different mode.
 * @param[in]  mode: Decide to enable WiFi in which mode. The optional modes are
 *                - RTW_MODE_STA
 *                - RTW_MODE_AP
 *                - RTW_MODE_STA_AP
 *                - RTW_MODE_PROMISC
 *                - RTW_MODE_P2P
 * @return     RTW_SUCCESS: WiFi initialization successful.
 * @return     RTW_ERROR: WiFi internal error.
 * @return     1: WIFI is already running.
 */
int wifi_on(rtw_mode_t mode);

/**
 * @brief      Disable Wi-Fi.
 * @param      None
 * @return     RTW_SUCCESS: if deinit success, wifi mode is changed to RTW_MODE_NONE.
 * @return     RTW_ERROR: Deinit WIFI timeout.
 */
int wifi_off(void);

/**
 * @brief      Switch Wifi Mode
 * @param[in]  mode: Decide to switch WiFi to which mode. The optional modes are
 *                - RTW_MODE_STA
 *                - RTW_MODE_AP
 *                - RTW_MODE_STA_AP
 *                - RTW_MODE_PROMISC
 * @return     RTW_SUCCESS: if WiFi switch mode success.
 * @return     RTW_ERROR: WiFi internal error.
 */
int wifi_set_mode(rtw_mode_t mode);

/**
 * @brief      Disable Wi-Fi.
 * @param      None
 * @return     RTW_SUCCESS: if deinit success
 * @note       The difference between @ref wifi_off_fastly() and @ref wifi_off() is that @ref wifi_off_fastly() directly disable Wi-Fi,
 *             it does not wait for ongoing Tx/Rx or wait for Rx thread stopped.
 */
int wifi_off_fastly(void);

/**
 * @brief      Specify wpa mode for wifi connection.
 * @param[in]  wpa_mode: The desired wpa mode. The optional modes are
 *                - WPA_AUTO_MODE
 *                - WPA_ONLY_MODE
 *                - WPA2_ONLY_MODE
 *                - WPA3_ONLY_MODE
 *                - WPA_WPA2_MIXED_MODE
 *                - WPA2_WPA3_MIXED_MODE
 * @return     RTW_SUCCESS: if setting wpa mode successful.
 * @return     RTW_ERROR: WPA mode is not supported.
 */
int wifi_set_wpa_mode(rtw_wpa_mode wpa_mode);

/**
 * @brief      Set IPS/LPS mode.
 * @param[in]  ips_mode: The desired IPS mode. It becomes effective when wlan enter IPS.
 *             ips_mode is the abbreviation of Inactive Power Save mode.
 *             Wi-Fi automatically turns RF off if it is not associated to AP.
 *                - IPS_NONE          - disable ips
 *                - IPS_NORMAL        - enable ips
 *                - IPS_LEVEL_2       - enable ips and keep fw alive
 *                - IPS_RESUME        - resume to the last ips powermode which recorded in wifi driver
 * @param[in]  lps_mode: The desired LPS mode. It becomes effective when wlan enter LPS.
 *             lps_mode is the abbreviation of Leisure Power Save mode.
 *             Wi-Fi automatically turns RF off during the association to AP if traffic is not busy,
 *             while it also automatically turns RF on to listen to the beacon of the associated AP.
 *                - PS_MODE_ACTIVE    - disable lps
 *                - PS_MODE_MIN       - enable lps, awake every beacon
 *                - PS_MODE_RESUME    - resume to the last lps powermode which recorded in wifi driver
 * @return     RTW_SUCCESS: if setting the corresponding mode successful.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       SoftAP and concurrent mode will not enter powersave mode.
 */
int wifi_set_power_mode(unsigned char ips_mode, unsigned char lps_mode);

/**
 * @brief      Set TDMA parameters.
 * @param[in]  slot_period: We separate TBTT into 2 or 3 slots.
 *                - If we separate TBTT into 2 slots, then slot_period should be larger or equal to 50ms.
 *                  It means 2 slot periods are (slot_period) and (100-slot_period).
 *                - If we separate TBTT into 3 slots, then slot_period should be less or equal to 33ms.
 *                  It means 3 slot periods are (100 - 2 * slot_period), (slot_period) and (slot_period).
 * @param[in]  rfon_period_len_1: rf will turn on for (rfon_period_len_1)ms in slot 1.
 * @param[in]  rfon_period_len_2: rf will turn on for (rfon_period_len_2)ms in slot 2.
 * @param[in]  rfon_period_len_3: rf will turn on for (rfon_period_len_3)ms in slot 3.
 * @return     RTW_SUCCESS if setting TDMA parameters successful.
 * @return     RTW_ERROR: WiFi internal error.
 * @e.g.
 * slot_period = 60, rfon_period_len_1 = 10, rfon_period_len_2 = 20, rfon_period_len_3 = 0.
 * slot_period >= 50ms, so the TBTT is separated into 2 slots as follows:
 * |----TDMA period 1----|--TDMA period 2--|
 * |-10ms-|-----50ms-----|--20ms--|--20ms--|
 * |rf on-|----rf off----|--rf on-|-rf off-|
 */
int wifi_set_tdma_param(unsigned char slot_period, unsigned char rfon_period_len_1, unsigned char rfon_period_len_2, unsigned char rfon_period_len_3);

/**
 * @brief      Set LPS DTIM. It becomes effective when wlan enter IPS.
 * @param[in]  dtim: The desired LPS DTIM value.
 *             DTIM specifies the interval that station can remain asleep without listening to beacon
 *             to check if there is buffered package.
 * @return     RTW_SUCCESS: if setting LPS dtim successful.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_lps_dtim(unsigned char dtim);

/**
 * @brief      Get LPS DTIM.
 * @param[out] dtim: The location where the LPS DTIM value will be stored.
 *             DTIM specifies the interval that station can remain asleep without listening to beacon
 *             to check if there is buffered package.
 * @return     RTW_SUCCESS: if getting dtim successful.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_lps_dtim(unsigned char *dtim);

enum {
	LPS_THRESH_PKT_COUNT = 0,
	LPS_THRESH_DIRECT_ENTER,
	LPS_THRESH_TP,
};
typedef unsigned char rtw_lps_thresh_t;

/**
 * @brief      Set LPS threshold.
 * @param[in]  mode: LPS threshold mode can be
 *                - LPS_THRESH_PKT_COUNT: enter power save or not, according to packet num
 *                - LPS_THRESH_DIRECT_ENTER: enter power save directly
 *                - LPS_THRESH_TP: enter power save or not, according to throughput.
 * @return     RTW_SUCCESS: if set LPS threshold successful.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_lps_thresh(rtw_lps_thresh_t mode);

/**
 * @brief      Set LPS LEVEL
 * @param[in]  lps_level can be
 *                - LPS_NORMAL  - only turn off RF
 *                - LPS_LCLK    - turn off RF and stop the clock of MAC
 *                - LPS_PG      - turn off almost all the power of MAC circuit
 * @return     RTW_SUCCESS: if setting LPS level successful.
 * @return     RTW_ERROR: WiFi internal errore.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_lps_level(unsigned char lps_level);

/**
 * @brief      Set Management Frame Protection Support.
 * @param[in]  value:
 *                - NO_MGMT_FRAME_PROTECTION          - not support
 *                - MGMT_FRAME_PROTECTION_OPTIONAL    - capable
 *                - MGMT_FRAME_PROTECTION_REQUIRED    - required
 * @return     RTW_SUCCESS if setting Management Frame Protection Support successful.
 * @return     RTW_ERROR: WiFi internal error.
 */
int wifi_set_mfp_support(unsigned char value);

/**
 * @brief      Trigger Wi-Fi driver to start an infrastructure Wi-Fi network.
 * @param[in]  ssid: A null terminated string containing the SSID name of the network.
 *             The data length of string pointed by ssid should not exceed 32.
 * @param[in]  security_type:
 *                - RTW_SECURITY_OPEN             - Open Security
 *                - RTW_SECURITY_WPA2_AES_PSK     - WPA2 Security using AES cipher
 *                - RTW_SECURITY_WPA2_AES_CMAC    - WPA2 Security using AES cipher and Management Frame Protection
 *                - WEP security is NOT IMPLEMENTED. It is NOT SECURE!
 * @param[in]  password: A byte array containing the cleartext security key for the network.
 *             The data length of string pointed by password should be between 8 and 64.
 * @param[in]  ssid_len: The length of the SSID in bytes.
 * @param[in]  password_len: The length of the security_key in bytes.
 * @param[in]  channel: 802.11 channel number.
 * @warning    If a STA interface is active when this function is called,
 *             the softAP will start on the same channel as the STA.
 *             It will NOT use the channel provided!
 * @return     RTW_SUCCESS: If successfully creates an AP.
 * @return     RTW_ERROR: WiFi internal error
 * @return     RTW_BADARG: Bad Argument input, SSID should be 0-32 characters.
 * @return     RTW_INVALID_KEY: password is not correct.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_start_ap(
	char 				*ssid,
	rtw_security_t		security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int					channel);

/**
 * @brief      Start an infrastructure Wi-Fi network with hidden SSID.
 * @param[in]  ssid: A null terminated string containing the SSID name of the network to join.
 *             The data length of string pointed by ssid should not exceed 32.
 * @param[in]  security_type:
 *                - RTW_SECURITY_OPEN              - Open Security
 *                - RTW_SECURITY_WPA2_AES_PSK      - WPA2 Security using AES cipher
 *                - WEP security is NOT IMPLEMENTED. It is NOT SECURE!
 * @param[in]  password: A byte array containing the cleartext security key for the network.
 *             The data length of string pointed by password should be between 8 and 64.
 * @param[in]  ssid_len: The length of the SSID in bytes.
 * @param[in]  password_len: The length of the security_key in bytes.
 * @param[in]  channel: 802.11 channel number
 * @warning    If a STA interface is active when this function is called,
 *             the softAP will start on the same channel as the STA.
 *             It will NOT use the channel provided!
 * @return     RTW_SUCCESS: If successfully creates an AP.
 * @return     RTW_ERROR: WiFi internal error.
 * @return     RTW_BADARG: Bad Argument input, SSID should be 0-32 characters.
 * @return     RTW_INVALID_KEY: password is not correct.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       The difference between @ref wifi_start_ap_with_hidden_ssid() and @ref wifi_start_ap() is that the SSID is hidden in beacon.
 */
int wifi_start_ap_with_hidden_ssid(
	char 				*ssid,
	rtw_security_t		security_type,
	char 				*password,
	int 				ssid_len,
	int 				password_len,
	int					channel);

/**
 * @brief      Initiate a scan to search for 802.11 networks.
 *
 * @param[in]  scan_type:
 *                - RTW_SCAN_TYPE_ACTIVE          - Actively scan a network by sending 802.11 probe(s)
 *                - RTW_SCAN_TYPE_PASSIVE         - Passively scan a network by listening for beacons from APs
 * @param[in]  bss_type: Specifies the network type to search for.
 *                - RTW_BSS_TYPE_INFRASTRUCTURE   - Denotes infrastructure network
 *                - RTW_BSS_TYPE_ADHOC            - Denotes an 802.11 ad-hoc IBSS network
 *                - RTW_BSS_TYPE_ANY              - Denotes either infrastructure or ad-hoc network
 * @param[in]  result_ptr: Scan specific ssid.
 *             The first 4 bytes is ssid lenth, and ssid name append after it.
 *             If no specific ssid need to scan, PLEASE CLEAN result_ptr before pass it into parameter.
 * @param[out] result_ptr: a pointer to a result storage structure.
 * @return     RTW_SUCCESS: If success.
 * @return     RTW_ERROR: WiFi internal error.
 * @return     Other integer value: scan flag = scan_type | (bss_type << 8).
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       The scan progressively accumulates results over time, and may take between 1 and 3 seconds to complete.
 *             The results of the scan will be individually provided to the callback function.
 *             Note: The callback function will be executed in the context of the RTW thread.
 * @note       When scanning specific channels, devices with a strong signal strength on nearby channels may be detected.
 */
int wifi_scan(rtw_scan_type_t                    scan_type,
			  rtw_bss_type_t                     bss_type,
			  void                *result_ptr);

/**
 * @brief      Initiate a scan to search for 802.11 networks, a higher level API based on wifi_scan()
 *             to simplify the scan operation.
 * @param[in]  results_handler: The callback function which will receive and process the result data.
 * @param[in]  user_data: User specified data that will be passed directly to the callback function.
 * @return     RTW_SUCCESS: If success.
 * @return     RTW_ERROR: WiFi internal error.
 * @return     RTW_TIMEOUT: Wi-Fi operation times out.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Callback must not use blocking functions, since it is called from the context of the RTW thread.
 *             The callback, user_data variables will be referenced after the function returns.
 *             Those variables must remain valid until the scan is completed.
 *             The usage of this api can refer to @ref fATWS() in atcmd_wifi.c.
 */
int wifi_scan_networks(rtw_scan_result_handler_t results_handler, void *user_data);

/**
 * @brief      Initiate a scan to search for 802.11 networks with specified SSID.
 * @param[in]  results_handler: The callback function which will receive and process the result data.
 * @param[in]  user_data: User specified data that will be passed directly to the callback function.
 * @param[in]  scan_buflen: The length of the result storage structure.
 * @param[in]  ssid: The SSID of the target network.
 * @param[in]  ssid_len: The length of the target network SSID.
 * @return     RTW_SUCCESS: If success.
 * @return     RTW_ERROR: WiFi internal error.
 * @return     RTW_TIMEOUT: Wi-Fi operation times out.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Callback must not use blocking functions, since it is called from the context of the RTW thread.
 *             The callback, user_data variables will be referenced after the function returns.
 *             Those variables must remain valid until the scan is completed.
 */
int wifi_scan_networks_with_ssid(int (results_handler)(char *, int, char *, void *), void *user_data, int scan_buflen, char *ssid, int ssid_len);

/**
 * @brief      Initiate a scan to search for 802.11 networks with specified SSID, show the extended security info in the scan result.
 * @param[in]  results_handler: The callback function which will receive and process the result data.
 * @param[in]  user_data: User specified data that will be passed directly to the callback function.
 * @param[in]  scan_buflen: The length of the result storage structure.
 * @param[in]  ssid: The SSID of the target network.
 * @param[in]  ssid_len: The length of the target network SSID.
 * @return     RTW_SUCCESS: If success.
 * @return     RTW_ERROR: WiFi internal error.
 * @return     RTW_TIMEOUT: Wi-Fi operation times out.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Callback must not use blocking functions, since it is called from the context of the RTW thread.
 *             The callback, user_data variables will be referenced after the function returns.
 *             Those variables must remain valid until the scan is completed.
 * @note       The difference between @ref wifi_scan_networks_with_ssid_by_extended_security() and @ref wifi_scan_networks_with_ssid()
  *            is that the extended security info (security & cipher) is shown by @ref wifi_scan_networks_with_ssid_by_extended_security().
 */
int wifi_scan_networks_with_ssid_by_extended_security(int (results_handler)(char *, int, char *, void *), void *user_data, int scan_buflen, char *ssid,
		int ssid_len);
/**
 * @brief      Set the channel used to be partial scanned.
 * @param[in]  channel_list: the channel list to be scanned.
 * @param[in]  pscan_config: the pscan_config of the channel set.
 *                - PSCAN_ENABLE            -enable for partial channel scan
 *                - PSCAN_FAST_SURVEY        -set to select scan time to FAST_SURVEY_TO, otherwise SURVEY_TO
 *                - PSCAN_SIMPLE_CONFIG    -set to select scan time to FAST_SURVEY_TO and resend probe request
 * @param[in]  length: The length of the channel_list.
 * @return     RTW_SUCCESS: If success.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       This function should be used with wifi_scan() function.
 *             First, use @ref wifi_set_pscan_chan() to indicate which channel will be scanned,
 *             and then use @ref wifi_scan() to get scanned results.
 */
int wifi_set_pscan_chan(__u8 *channel_list, __u8 *pscan_config, __u8 length);

/**
 * @brief      get band type
 * @return     the support band type.
 *                - WL_BAND_2_4G: only support 2.4G
 *                - WL_BAND_5G: only support 5G
 *                - WL_BAND_2_4G_5G_BOTH: support both 2.4G and 5G
 */
WL_BAND_TYPE wifi_get_band_type(void);

/**
 * @brief      Get current Wi-Fi setting from driver.
 * @param[in]  ifname: the wlan interface name, can be WLAN0_NAME or WLAN1_NAME.
 * @param[out] pSetting: The location where the WIFI setting will be stored. (in structure @ref rtw_wifi_setting_t)
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_setting(const char *ifname, rtw_wifi_setting_t *pSetting);

/**
 * @brief      Show the network information stored in a rtw_wifi_setting_t structure.
 * @param[in]  ifname: the wlan interface name, can be WLAN0_NAME or WLAN1_NAME.
 * @param[in]  pSetting: Points to the rtw_wifi_setting_t structure which information is gotten by @ref wifi_get_setting().
 * @return     RTW_SUCCESS: if success
 * @note       Please invoke wifi_get_setting() before wifi_get_setting().
 */
int wifi_show_setting(const char *ifname, rtw_wifi_setting_t *pSetting);

/**
 * @brief      Set the network mode according to the data rate it supports.
 *             Driver works in BGN mode in default after driver initialization. This function is used to
 *             change wireless network mode for station mode before connecting to AP.
 * @param[in]  mode: Network mode to set. The value can be RTW_NETWORK_B/RTW_NETWORK_BG/RTW_NETWORK_BGN.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_network_mode(rtw_network_mode_t mode);

/**
 * @brief      Get the network mode.
 *             Driver works in BGN mode in default after driver initialization. This function is used to
 *             get the current wireless network mode for station mode.
 * @param[in]  pmode: Network mode to get.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_network_mode(rtw_network_mode_t *pmode);

/**
 * @brief      Get the current network mode the network interface is using against the wireless AP.
 *             Driver will match its supported network mode with AP's supported network mode when connecting
 *             and choose the best network mode. This function will get the network mode used to connect to AP.
 * @param[in]  pmode: Current network mode to get.
 * @return     the current network mode used.
 *                1) WIRELESS_11B: using 802.11b.
 *                2) WIRELESS_11G: using 802.11g.
 *                8) WIRELESS_11_24N: using 802.11n.
 *                40) WIRELESS_11AC: using 802.11ac.
 *                -1) RTW_ERROR: get fails.
 */
int wifi_get_cur_network_mode(void);

/**
 * @brief      Set the chip to start or stop the promiscuous mode.
 * @param[in]  enabled:
 *                - RTW_PROMISC_DISABLE     - disable the promisc.
 *                - RTW_PROMISC_ENABLE      - enable the promisc special for all ethernet frames.
 *                - RTW_PROMISC_ENABLE_1    - enable the promisc special for Broadcast/Multicast ethernet frames.
 *                - RTW_PROMISC_ENABLE_2    - enable the promisc special for all 802.11 frames.
 *                - RTW_PROMISC_ENABLE_3    - enable the promisc special for Broadcast/Multicast 802.11 frames.
 *                - RTW_PROMISC_ENABLE_3    - enable the promisc special for all 802.11 frames and MIMO PLCP headers.
 * @param[in]  callback: the callback function which will receive and process the network data.
 * @param[in]  len_used: specify if the promisc data length is used.
 *             If len_used set to 1, packet(frame data) length will be saved and transferred to callback function.
 *
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: interface is not ready.
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       This function can be used to implement vendor specified simple configure.
 * @note       To fetch Ethernet frames, the len_used should be set to 1
 */
int wifi_set_promisc(rtw_rcr_level_t enabled, void (*callback)(unsigned char *, unsigned int, void *), unsigned char len_used);

/**
 * @brief      Let Wi-Fi enter promiscuous mode.
 * @param[in]  None
 * @return     None
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       The difference between @ref wifi_enter_promisc_mode() and @ref wifi_on(RTW_MODE_PROMISC)
 *             is that @ref wifi_enter_promisc_mode() able to switch from other mode to promiscuous mode.
 */
void wifi_enter_promisc_mode(void);

/**
 * @brief      Set the wps phase.
 *             wps: Wi-Fi Protected Setup.
 * @param[in]  is_trigger_wps: to trigger wps function or not.
 *             is_trigger_wps value should only be 0 or 1.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_wps_phase(unsigned char is_trigger_wps);

/**
 * @brief      Trigger Wi-Fi driver to restart an infrastructure Wi-Fi network.
 * @warning    If a STA interface is active when this function is called, the softAP will
 *             start on the same channel as the STA. It will NOT use the channel provided!
 * @param[in]  ssid: A null terminated string containing the SSID name of the network.
 *             The data length of string pointed by ssid should not exceed 32.
 * @param[in]  security_type:
 *                - RTW_SECURITY_OPEN             - Open Security
 *                - RTW_SECURITY_WPA2_AES_PSK     - WPA2 Security using AES cipher
 *                - RTW_SECURITY_WPA2_AES_CMAC    - WPA2 Security using AES cipher and Management Frame Protection
 *                - WEP security is NOT IMPLEMENTED. It is NOT SECURE!
 * @param[in]  password: A byte array containing the cleartext security key for the network.
 *             The data length of string pointed by password should be between 8 and 64.
 * @param[in]  ssid_len: The length of the SSID in bytes.
 * @param[in]  password_len: The length of the security_key in bytes.
 * @param[in]  channel: 802.11 channel number.
 * @return     RTW_SUCCESS: If successfully creates an AP.
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       The difference between @ref wifi_restart_ap() and @ref wifi_start_ap()
 *             is that @ref wifi_restart_ap() call wifi_off() to disable current Wi-Fi first.
 */
int wifi_restart_ap(
	unsigned char 		*ssid,
	rtw_security_t		security_type,
	unsigned char 		*password,
	int 				ssid_len,
	int 				password_len,
	int					channel);

/**
 * @brief      Set autoreconnect mode with configuration.
 * @param[in]  mode: Refer to @ref rtw_autoreconnect_mode_t.
 *                - 0: Disable the autoreconnect mode
 *                - 1: Enable the autoreconnect mode
 *                - 2: Enable autoreconnect mode with infinite retry times.
 * @param[in]  retry_times: The number of retry limit.
 * @param[in]  timeout: The timeout value (in seconds).
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Defining CONFIG_AUTO_RECONNECT in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       The difference between @ref wifi_config_autoreconnect() and @ref wifi_set_autoreconnect() is that
 *             user can specify the retry times and timeout value in @ref wifi_config_autoreconnect().
 *             But in @ref wifi_set_autoreconnect(), 3 retry limit and 5 seconds timeout are set as default.
 */
int wifi_config_autoreconnect(__u8 mode, __u8 retry_times, __u16 timeout);

/**
 * @brief      Set reconnection mode with 3 retry limit and 5 seconds timeout as default.
 * @param[in]  mode: Set 1/0 to enable/disable the reconnection mode, set 2 to enable infinite reconnection mode.
 *             Refer to rtw_autoreconnect_mode_t enum in "wifi_constants.h".
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Defining CONFIG_AUTO_RECONNECT in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       The difference between @ref wifi_config_autoreconnect() and @ref wifi_set_autoreconnect() is that
 *             user can specify the retry times and timeout value in @ref wifi_config_autoreconnect().
 *             But in @ref wifi_set_autoreconnect() these values are set with 3 retry limit and 5 seconds timeout as default.
 */
int wifi_set_autoreconnect(__u8 mode);

/**
 * @brief      Get the result of setting reconnection mode.
 * @param[out] mode: The location where the reconnection mode will be stored.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Defining CONFIG_AUTO_RECONNECT in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 */
int wifi_get_autoreconnect(__u8 *mode);

/**
 * @brief      Present the device disconnect reason while connecting.
 * @param      None
 * @return     The error code while connecting. (@ref rtw_connect_error_flag_t)
 *               - RTW_NO_ERROR
 *               - RTW_NONE_NETWORK
 *               - RTW_CONNECT_FAIL
 *               - RTW_WRONG_PASSWORD
 *               - RTW_4WAY_HANDSHAKE_TIMEOUT
 *               - RTW_DHCP_FAIL
 *               - RTW_AUTH_FAIL
 *               - RTW_ASSOC_FAIL
 *               - RTW_DEAUTH_DEASSOC
 *               - RTW_UNKNOWN (initial status)
 */
int wifi_get_last_error(void);


#ifdef CONFIG_CUSTOM_IE
#ifndef BIT
#define BIT(x)	((__u32)1 << (x))
#endif

#ifndef _CUSTOM_IE_TYPE_
#define _CUSTOM_IE_TYPE_
/**
  * @brief  The enumeration is transmission type for wifi custom ie.
  */
enum CUSTOM_IE_TYPE {
	PROBE_REQ = BIT(0),
	PROBE_RSP = BIT(1),
	BEACON	  = BIT(2),
	ASSOC_REQ = BIT(3),
};
typedef __u32 rtw_custom_ie_type_t;
#endif /* _CUSTOM_IE_TYPE_ */

/* ie format
 * +-----------+--------+-----------------------+
 * |element ID | length | content in length byte|
 * +-----------+--------+-----------------------+
 *
 * type: refer to CUSTOM_IE_TYPE
 */
#ifndef _CUS_IE_
#define _CUS_IE_
/**
 * @brief      The structure is used to set WIFI custom ie list, and type match CUSTOM_IE_TYPE.\n
 *             The ie will be transmitted according to the type.
 */
typedef struct _cus_ie {
	__u8 *ie;
	__u8 type;
} rtw_custom_ie_t, *p_rtw_custom_ie_t;
#endif /* _CUS_IE_ */

/**
 * @brief      Setup custom ie list. (ie: Information Element)
 * @warning    This API can't be executed twice before deleting the previous custom ie list.
 * @param[in]  cus_ie: Pointer to WIFI CUSTOM IE list.
 * @param[in]  ie_num: The number of WIFI CUSTOM IE list.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Defining CONFIG_CUSTOM_IE in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_add_custom_ie(void *cus_ie, int ie_num);

/**
 * @brief      Update the item in WIFI CUSTOM IE list. (ie: Information Element)
 * @param[in]  cus_ie: Pointer to WIFI CUSTOM IE address.
 * @param[in]  ie_index: Index of WIFI CUSTOM IE list.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Defining CONFIG_CUSTOM_IE in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_update_custom_ie(void *cus_ie, int ie_index);

/**
 * @brief      Delete WIFI CUSTOM IE list. (ie: Information Element)
 * @param      None
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Defining CONFIG_CUSTOM_IE in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())

 */
int wifi_del_custom_ie(void);
#endif

#ifdef CONFIG_PROMISC

/**
 * @brief      Initialize packet filter related data.
 * @param      None
 * @return     None
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 */
void wifi_init_packet_filter(void);

/**
 * @brief      Add packet filter.
 * @param[in]  filter_id: The filter id.
 *             filter id can be num between 0 to 4.
 * @param[in]  patt: Point to the filter pattern.
 * @param[in]  rule: Point to the filter rule.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: invalid filter id
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       For now, the maximum number of filters is 5.
 */
int wifi_add_packet_filter(unsigned char filter_id, rtw_packet_filter_pattern_t *patt, rtw_packet_filter_rule_t rule);

/**
 * @brief      Enable the packet filter.
 * @param[in]  filter_id: The filter id.
 *             filter id can be num between 0 to 4.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: invalid filter id
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       The filter can be used only if it has been enabled.
 */
int wifi_enable_packet_filter(unsigned char filter_id);

/**
 * @brief      Disable the packet filter.
 * @param[in]  filter_id: The filter id.
 *             filter id can be num between 0 to 4.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: invalid filter id
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 */
int wifi_disable_packet_filter(unsigned char filter_id);

/**
 * @brief      Remove the packet filter.
 * @param[in]  filter_id: The filter id.
 *             filter id can be num between 0 to 4.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: invalid filter id
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 */
int wifi_remove_packet_filter(unsigned char filter_id);

/**
 * @brief      Filter out the retransmission MIMO packet in promisc mode.
 * @param[in]  enable: set 1 to enable filter retransmission pkt function, set 0 to disable this filter function.
 * @param[in]  filter_interval_ms:
 *             if 'enable' equals 0, it's useless;
 *             if 'enable' equals 1, this value:
 *                - indicate the time(ms) below which an adjacent pkt received will be claimed a retransmission pkt;
 *                - if it has the same length with the previous pkt, and driver will drop all retransmission pkts.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: only support retransmit plcp pkt, does not support normal pkt.
 * @note       Defining CONFIG_PROMISC and CONFIG_UNSUPPORT_PLCPHDR_RPT in "autoconf.h" needs to be done before compiling,
 *             or this API won't be effective.
 * @e.g.       For example, if the packet transmission time interval is 10ms,
 *             but driver receives two packets with the same length within 3ms,
 *             then the second packet will be dropped if configed as
 *             wifi_retransmit_packet_filter(1,3).
 */
int wifi_retransmit_packet_filter(u8 enable, u8 filter_interval_ms);

/**
 * @brief      Only receive the packets sent by the specified ap and phone in promisc mode.
 * @param[in]  enable: set 1 to enable filter, set 0 to disable this filter function.
 * @param[in]  ap_mac: if 'enable' equals 0, it's useless; if 'enable' equals 1, this value is the ap's mac address.
 * @param[in]  phone_mac: if 'enable' equals 0, it's useless; if 'enable' equals 1, this value is the phone's mac address.
 * @return     None.
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please invoke this function as "wifi_filter_by_ap_and_phone_mac(0,NULL,NULL)" before exiting promisc mode if you enabled it during the promisc mode.
 */
void wifi_filter_by_ap_and_phone_mac(u8 enable, void *ap_mac, void *phone_mac);

/**
 * @brief      config to report ctrl packet or not under promisc mode.
 * @param[in]  enable: set 1 to enable ctrl packet report, set 0 to disable ctrl packet report.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: promisc mode is not enabled.
 * @note       Defining CONFIG_PROMISC in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       This function can only be used under promisc mode, i.e. between wifi_set_promisc(enable,...,...) and wifi_set_promisc(disable,...,...)
 */
int wifi_promisc_ctrl_packet_rpt(u8 enable);
#endif

#ifdef CONFIG_ANTENNA_DIVERSITY
/**
 * @brief      Get antenna infomation.
 * @param[in]  antenna: Pointer to the obtained antenna value, 0: main, 1: aux.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Defining CONFIG_ANTENNA_DIVERSITY in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_antenna_info(unsigned char *antenna);
#endif // #ifdef CONFIG_ANTENNA_DIVERSITY

/**
 * @brief      Config mode of HW indicating packets (mgnt and data) and SW reporting packets to wifi_indication().
 * @param[in]  enable:
 *                - 0: disable mode(default), HW only indicate bssid-matched pkts and SW don't report.
 *                - 1: normal mode, HW only indicate bssid-matched pkts and SW report.
 *                - 2: wild mode, HW indicate all pkts and SW report.
 * @return     None
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
void wifi_set_indicate_mgnt(int enable);

/**
 * @brief      Set to use B mode in low rssi condition.
 * @param[in]  enable: determine if use the function or not.
 * @param[in]  rssi: the rssi threshold.
 *             If the network rssi is less than 'rssi', use B mode, else use BGN mode.
 *             Default is -60, you'd better set it less than -60, e.g. -65, -70.
 * @return     None
 */
void wifi_set_lowrssi_use_b(int enable, int rssi);


/**
 * @brief      Get the information of MP driver
 * @param[out] ability: The location where the MP driver information will be stored.
 *                0x1 stand for mp driver, and 0x0 stand for normal driver.
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_drv_ability(uint32_t *ability);

/**
 * @brief      Set channel plan into flash/efuse, must reboot after setting channel plan.
 * @param[in]  channel_plan: the value of channel plan, defined in wifi_constants.h
 *                - 0x20: WORLD1
 *                - 0x21: ETSI1
 *                - 0x22: FCC1
 *                - 0x23: MKK1
 *                - 0x24: ETSI2
 *                - 0x2A: FCC2
 *                - 0x47: WORLD2
 *                - 0x58: MKK2
 *                - 0x41: GLOBAL
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_channel_plan(uint8_t channel_plan);

/**
 * @brief      Get channel plan from calibration section
 * @param[out] channel_plan: The location where the channel plan will be stored.
 *             Channel plan is defined in wifi_constants.h
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_get_channel_plan(uint8_t *channel_plan);

#ifdef CONFIG_AP_MODE
/**
 * @brief      Enable packets forwarding in ap mode
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the softAP is started before invoking this function. (@ref wifi_start_ap() or wifi_start_ap_with_hidden_ssid())
 */
int wifi_enable_forwarding(void);

/**
 * @brief      Disable packets forwarding in ap mode
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 * @note       Please make sure the softAP is started before invoking this function. (@ref wifi_start_ap() or wifi_start_ap_with_hidden_ssid())
 */
int wifi_disable_forwarding(void);
#endif

#ifdef CONFIG_CONCURRENT_MODE
/**
 * @brief      Set flag for concurrent mode wlan1 issue_deauth when channel is switched by wlan0
 * @usage      wifi_set_ch_deauth(0) -> wlan0 wifi_connect -> wifi_set_ch_deauth(1)
 * @param[in]  enable:
 *                - 0: for disable (wlan1 will not issue deauth when channel is switched by wlan0)
 *                - 1: for enable (wlan1 will issue deauth when channel is switched by wlan0)
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Please make sure the Wi-Fi concurrent mode is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_ch_deauth(__u8 enable);
#endif

///@name Ameba1 Only
///@{
/**
 * @brief      enable AP sending QoS Null0 Data to poll Sta be alive.
 * @param[in]  enabled: enabled can be set to 0,1.
 *                - 0: means enable.
 *                - 1: means disable.
 * @return     None
 */
void wifi_set_ap_polling_sta(__u8 enabled);
///@}

#ifdef CONFIG_SW_MAILBOX_EN
/**
 * @brief      interface for bt to set mailbox info to wifi, mostly for coexistence usage
 * @param[in]  data: pointer of mailbox data
 * @param[in]  len: length of mailbox data
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Please make sure the Wi-Fi concurrent mode is enabled before invoking this function. (@ref wifi_on())
 */
int mailbox_to_wifi(u8 *data, u8 len);
#else
#define mailbox_to_wifi(data, len)
#endif

#ifdef CONFIG_WOWLAN_TCP_KEEP_ALIVE
/**
 * @brief      construct a tcp packet that offload to wlan. wlan would keep sending this packet to tcp server.
 * @param[in]  socket_fd: tcp socket
 * @param[in]  content: tcp payload
 * @param[in]  len: tcp payload size
 * @param[in]  interval_ms: the interval to send tcp packet. (in milliseconds)
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: interface is not running or keep alive is not enabled.
 * @note       Defining CONFIG_WOWLAN_TCP_KEEP_ALIVE in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_set_tcp_keep_alive_offload(int socket_fd, uint8_t *content, size_t len, uint32_t interval_ms);
#endif

// WoWlan related
//-------------------------------------------------------------//
#ifdef CONFIG_WOWLAN
typedef struct {
	unsigned int filter_id;
	unsigned int polarity;
	unsigned int type;
	unsigned int offset;
	unsigned char *bitmask;
	unsigned char *pattern;
} wowlan_pattern_param_t;

/**
 * @brief      control wowlan mode enable and disable
 * @param[in]  enable:
 *                - 0: means WOWLAN_DISABLE
 *                - 1: means WOWLAN_ENABLE
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Defining CONFIG_WOWLAN in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_wowlan_ctrl(int enable);

/**
 * @brief      set wowlan customer pattern
 * @param[in]  pattern: refer to struct wowlan_pattern_t
 * @return     RTW_SUCCESS: if success
 * @return     RTW_ERROR: WiFi internal error
 * @note       Defining CONFIG_WOWLAN in "autoconf.h" needs to be done before compiling, or this API won't be effective.
 * @note       Please make sure the Wi-Fi is enabled before invoking this function. (@ref wifi_on())
 */
int wifi_wowlan_set_pattern(wowlan_pattern_t pattern);
#endif
//-------------------------------------------------------------//
/**
  * @}
  */

#ifdef CONFIG_MCC_STA_AP_MODE
/**
 * @brief  Set the HW retry limit of management frame.
 * @param[in] retry_limit must be within the range 0 to 63.
 * @return  RTW_SUCCESS or RTW_ERROR.
 */
int wifi_set_mgnt_rt_lmt(unsigned char retry_limit);

/**
 * @brief  Set the HW retry limit of data frame.
 * @param[in] retry_limit must be within the range 0 to 63.
 * @return  RTW_SUCCESS or RTW_ERROR.
 */
int wifi_set_data_rt_lmt(unsigned char retry_limit);

/**
 * @brief  Get the HW retry limit of management frame.
 * @return  RTW_ERROR or retry_limit.
 */
int wifi_get_mgnt_rt_lmt(void);

/**
 * @brief  Get the HW retry limit of data frame.
 * @return  RTW_ERROR or retry_limit.
 */
int wifi_get_data_rt_lmt(void);

/**
 * @brief  Enable or disable TX and RX AMPDU.
 * @param[in] option: 0: disable, 1: enable, others: default (enable).
 * @param[in] path: 0: RTW_TX_AMPDU, 1: RTW_RX_AMPDU.
 * @return  RTW_SUCCESS or RTW_ERROR.
 */
int wifi_set_aggregation(unsigned char option, rtw_ampdu_mode_t path);

/**
 * @brief  Check if TX and RX AMPDU is enabled or disabled.
 * @param[in] path: 0: RTW_TX_AMPDU, 1: RTW_RX_AMPDU.
 * @return  RTW_ERROR, 0: disabled, 1: enabled.
 */
int wifi_get_aggregation(rtw_ampdu_mode_t path);

/**
 * @brief  Allow or reject broadcast and multicast packet.
 * @param[in] enable: 0: reject, 1: allow.
 * @param[in] types: 0: RTW_MULTICAST_PKT, 1: RTW_BROADCAST_PKT.
 * @return  RTW_SUCCESS or RTW_ERROR.
 */
int wifi_set_block_bc_mc_packet(unsigned char enable, unsigned char types);

/**
 * @brief  Check if broadcast and multicast packet is allowed or rejected.
 * @param[in] types: 0: RTW_MULTICAST_PKT, 1: RTW_BROADCAST_PKT.
 * @return  RTW_ERROR, 0: reject, 1: allow.
 */
int wifi_get_block_bc_mc_packet(unsigned char types);

#endif // CONFIG_MCC_STA_AP_MODE

#ifdef __cplusplus
}
#endif

/**
  * @}
  */

#endif // __WIFI_API_H

//----------------------------------------------------------------------------//

