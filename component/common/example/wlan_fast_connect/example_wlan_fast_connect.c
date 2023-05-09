/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/

 /** @file

	This example demonstrate how to implement wifi fast reconnection
**/
#include <platform_opts.h>
#include <wlan_fast_connect/example_wlan_fast_connect.h>


#include "task.h"
#include <platform/platform_stdlib.h>
#include <wifi/wifi_conf.h>
#include "flash_api.h"
#include "device_lock.h"
#include "netif.h"
#include <lwip_netconf.h>

extern struct netif xnetif[NET_IF_NUM];
extern u32 g_reconnect_delay;

write_reconnect_ptr p_write_reconnect_ptr;

#define FAST_VERSION_LEN    16
char fast_reconn_ver[FAST_VERSION_LEN] = "VERSION_V01";

#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
uint32_t offer_ip = 0;
uint32_t server_ip = 0;
#endif
#define WIFI_RETRYCOUNT 5
/*
* Usage:
*       wifi connection indication trigger this function to save current
*       wifi profile in flash
*
* Condition: 
*       CONFIG_EXAMPLE_WLAN_FAST_CONNECT flag is set
*/

int wlan_write_reconnect_data_to_flash(u8 *data, uint32_t len)
{
	flash_t flash;
	struct wlan_fast_reconnect read_data = {0};
	if(!data)
		return -1;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct wlan_fast_reconnect), (u8 *) &read_data);

#if ATCMD_VER == ATVER_2
	struct wlan_fast_reconnect *copy_data = (struct wlan_fast_reconnect *) data;
	copy_data->enable = read_data.enable;
#endif

	//wirte it to flash if different content: SSID, Passphrase, Channel, Security type
	if(memcmp(data, (u8 *) &read_data, sizeof(struct wlan_fast_reconnect)) != 0) {
#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
		printf("\r\n %s():not the same ssid/passphrase/channel/offer_ip, write new profile to flash", __func__);
#else
		printf("\r\n %s():not the same ssid/passphrase/channel, write new profile to flash", __func__);
#endif
		flash_erase_sector(&flash, FAST_RECONNECT_DATA);
		flash_stream_write(&flash, FAST_RECONNECT_DATA, len, (uint8_t *) data);
		flash_stream_write(&flash, FAST_RECONN_VERSION,sizeof(fast_reconn_ver),(uint8_t *)fast_reconn_ver);
		
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return 0;
}

int wifi_init_done_prior(void)
{	
	flash_t		flash;
	uint32_t	channel;
	uint32_t    security_type;
	uint8_t     pscan_config;
	char key_id[2] = {0};
	int ret;
	uint32_t wifi_retry_connect = WIFI_RETRYCOUNT;//For fast wifi connect retry
	rtw_network_info_t wifi = {0};

	struct wlan_fast_reconnect_prior *data;	
	data = (struct wlan_fast_reconnect_prior *)malloc(sizeof(struct wlan_fast_reconnect_prior));
	if(data){
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct wlan_fast_reconnect_prior), (uint8_t *)data);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		/* Check whether stored flash profile is empty */
		struct wlan_fast_reconnect_prior *empty_data;
		empty_data = (struct wlan_fast_reconnect_prior *)malloc(sizeof(struct wlan_fast_reconnect_prior));
		if(empty_data){
			memset(empty_data, 0xff, sizeof(struct wlan_fast_reconnect_prior));
			if(memcmp(empty_data, data, sizeof(struct wlan_fast_reconnect_prior)) == 0){
				printf("[FAST_CONNECT] Fast connect profile is empty, abort fast connection\n");
				free(data);
				free(empty_data);
				return 0;
			}
			free(empty_data);
		}

		memcpy(psk_essid, data->psk_essid, sizeof(data->psk_essid));
		memcpy(psk_passphrase, data->psk_passphrase, sizeof(data->psk_passphrase));
		memcpy(wpa_global_PSK, data->wpa_global_PSK, sizeof(data->wpa_global_PSK));
		channel = data->channel;
		snprintf(key_id, sizeof(key_id), "%d",(char) (channel>>28));
		channel &= 0xff;
		security_type = data->security_type;
		pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
		//set partial scan for entering to listen beacon quickly
WIFI_RETRY_LOOP:
		ret = wifi_set_pscan_chan((uint8_t *)&channel, &pscan_config, 1);
		if(ret < 0){
			free(data);
			return -1;
		}

		wifi.security_type = security_type;
		//SSID
		strcpy((char *)wifi.ssid.val, (char*)psk_essid);
		wifi.ssid.len = strlen((char*)psk_essid);

		switch(security_type){
			case RTW_SECURITY_WEP_PSK:
				wifi.password = (unsigned char*) psk_passphrase;
				wifi.password_len = strlen((char*)psk_passphrase);
				wifi.key_id = atoi((const char *)key_id);
				break;
			case RTW_SECURITY_WPA_TKIP_PSK:
			case RTW_SECURITY_WPA2_AES_PSK:
#ifdef CONFIG_SAE_SUPPORT
			case RTW_SECURITY_WPA3_AES_PSK:
			case RTW_SECURITY_WPA2_WPA3_MIXED:
#endif
				wifi.password = (unsigned char*) psk_passphrase;
				wifi.password_len = strlen((char*)psk_passphrase);
				break;
			default:
				break;
		}

#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
		offer_ip = data->offer_ip;
#endif

		ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
			wifi.password_len, wifi.key_id, NULL);
		if(ret != RTW_SUCCESS){
			wifi_retry_connect--;
			if(wifi_retry_connect > 0){
				if (g_reconnect_delay > 0) {
					vTaskDelay(g_reconnect_delay);
					g_reconnect_delay = 0;
				}
				printf("wifi retry\r\n");
				goto WIFI_RETRY_LOOP;
			}
		}
		if(ret == RTW_SUCCESS){
			LwIP_DHCP(0, DHCP_START);
#if LWIP_VERSION_MAJOR >= 2 && LWIP_VERSION_MINOR >= 1
#if LWIP_IPV6
			LwIP_DHCP6(0, DHCP6_START);
#endif
#endif
		}

		free(data);
		return 0;
	}
	printf("malloc fail\r\n");
	return -1;
}
/*
* Usage:
*       After wifi init done, waln driver call this function to check whether
*       auto-connect is required.
*
*       This function read previous saved wlan profile in flash and execute connection.
*
* Condition: 
*       CONFIG_EXAMPLE_WLAN_FAST_CONNECT flag is set
*/
int wlan_init_done_callback(void)
{
	flash_t		flash;
	uint32_t	channel;
	uint32_t    security_type;
	uint8_t     pscan_config;
	char key_id[2] = {0};
	int ret;
	uint32_t wifi_retry_connect = WIFI_RETRYCOUNT;//For fast wifi connect retry
	rtw_network_info_t wifi = {0};

	char read_reconn_ver[FAST_VERSION_LEN] = {0};
	char empty_reconn_ver[FAST_VERSION_LEN] = {0}; 

	uint32_t preadfrom = FAST_RECONN_VERSION - 1;
	uint8_t read_data = 0 ;
	int read_offset = 0;
	
	memset(empty_reconn_ver, 0xff, FAST_VERSION_LEN);
	
#if CONFIG_LWIP_LAYER
	netif_set_up(&xnetif[0]);
#endif
#if LWIP_VERSION_MAJOR >= 2 && LWIP_VERSION_MINOR >= 1
#if LWIP_IPV6
	netif_create_ip6_linklocal_address(&xnetif[0], 1);
#endif
#endif

#if CONFIG_AUTO_RECONNECT
	//setup reconnection flag
	if(wifi_set_autoreconnect(1) < 0){
		return -1;
	}
#endif

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FAST_RECONN_VERSION, sizeof(read_reconn_ver), (uint8_t *) read_reconn_ver);
	
	for(read_offset = (FAST_RECONN_VERSION - FAST_RECONNECT_DATA); read_offset > 0; read_offset--)
	{		
		flash_stream_read(&flash,preadfrom, 1, (uint8_t *)&read_data);
		if(read_data == 0xff) {
			preadfrom--;
		}
		else {
			break ;
		}
	}
	
	device_mutex_unlock(RT_DEV_LOCK_FLASH);	
	
	if((memcmp(read_reconn_ver,fast_reconn_ver, sizeof(fast_reconn_ver)) == 0) || 
		((read_offset > sizeof(struct wlan_fast_reconnect_prior) ) && (memcmp(read_reconn_ver,empty_reconn_ver, FAST_VERSION_LEN) == 0)) )
	{	
		struct wlan_fast_reconnect *data;
		data = (struct wlan_fast_reconnect *)malloc(sizeof(struct wlan_fast_reconnect));
		if(data){
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct wlan_fast_reconnect), (uint8_t *)data);
			device_mutex_unlock(RT_DEV_LOCK_FLASH);

			/* Check whether stored flash profile is empty */
			struct wlan_fast_reconnect *empty_data;
			empty_data = (struct wlan_fast_reconnect *)malloc(sizeof(struct wlan_fast_reconnect));
			if(empty_data){
				memset(empty_data, 0xff, sizeof(struct wlan_fast_reconnect));
				if(memcmp(empty_data, data, sizeof(struct wlan_fast_reconnect)) == 0){
					printf("[FAST_CONNECT] Fast connect profile is empty, abort fast connection\n");
					free(data);
					free(empty_data);
					return 0;
				}
				free(empty_data);
			}

			memcpy(psk_essid, data->psk_essid, sizeof(data->psk_essid));
			memcpy(psk_passphrase, data->psk_passphrase, sizeof(data->psk_passphrase));
			memcpy(wpa_global_PSK, data->wpa_global_PSK, sizeof(data->wpa_global_PSK));
			channel = data->channel;
			snprintf(key_id, sizeof(key_id), "%d",(char) (channel>>28));
			channel &= 0xff;
			security_type = data->security_type;
			pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
			//set partial scan for entering to listen beacon quickly
	WIFI_RETRY_LOOP:
			ret = wifi_set_pscan_chan((uint8_t *)&channel, &pscan_config, 1);
			if(ret < 0){
				free(data);
				return -1;
			}

			wifi.security_type = security_type;
			//SSID
			strcpy((char *)wifi.ssid.val, (char*)psk_essid);
			wifi.ssid.len = strlen((char*)psk_essid);

			switch(security_type){
				case RTW_SECURITY_WEP_PSK:
					wifi.password = (unsigned char*) psk_passphrase;
					wifi.password_len = strlen((char*)psk_passphrase);
					wifi.key_id = atoi((const char *)key_id);
					break;
				case RTW_SECURITY_WPA_TKIP_PSK:
				case RTW_SECURITY_WPA2_AES_PSK:
#ifdef CONFIG_SAE_SUPPORT
				case RTW_SECURITY_WPA3_AES_PSK:
				case RTW_SECURITY_WPA2_WPA3_MIXED:
#endif
					wifi.password = (unsigned char*) psk_passphrase;
					wifi.password_len = strlen((char*)psk_passphrase);
					break;
				default:
					break;
			}

#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
			offer_ip = data->offer_ip;
			server_ip = data->server_ip;
#endif

			ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
				wifi.password_len, wifi.key_id, NULL);
			if(ret != RTW_SUCCESS){
				wifi_retry_connect--;
				if(wifi_retry_connect > 0){
					printf("wifi retry\r\n");
					if (g_reconnect_delay > 0) {
						vTaskDelay(g_reconnect_delay);
						g_reconnect_delay = 0;
					}
					goto WIFI_RETRY_LOOP;
				}
			}
			if(ret == RTW_SUCCESS){
				LwIP_DHCP(0, DHCP_START);
#if LWIP_VERSION_MAJOR >= 2 && LWIP_VERSION_MINOR >= 1
#if LWIP_IPV6
				LwIP_DHCP6(0, DHCP6_START);
#endif
#endif
			}

			free(data);
		}
	}
	else
	{
		wifi_init_done_prior();
	}
	
	return 0;
}

int Erase_Fastconnect_data(void){
	flash_t flash;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash, FAST_RECONNECT_DATA);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return 0;
}

void example_wlan_fast_connect()
{
	// Call back from wlan driver after wlan init done
	p_wlan_init_done_callback = wlan_init_done_callback;

	// Call back from application layer after wifi_connection success
	p_write_reconnect_ptr = wlan_write_reconnect_data_to_flash;

}
