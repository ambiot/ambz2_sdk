#include <autoconf.h>
#include "osdep_service.h"
#include <platform/platform_stdlib.h>
#include <wifi/wifi_conf.h>
#include <lwip_netconf.h>
#include "netif.h"
#include "wlan_fast_connect/example_wlan_fast_connect.h"
#include "flash_api.h"
#include "device_lock.h"
#include "freertos_pmu.h"

#if defined(CONFIG_EXAMPLE_TICKLESS_WIFI_ROAMING) && CONFIG_EXAMPLE_TICKLESS_WIFI_ROAMING

#ifndef WLAN0_NAME
#define WLAN0_NAME		"wlan0"
#endif
#ifndef WLAN1_NAME
#define WLAN1_NAME		"wlan1"
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 			6
#endif

#define WIFI_ROAMING_TICKLESS_MONITOR 1
extern unsigned char roaming_type_flag;	// 1-tickless roaming; 2-normal roaming
extern xSemaphoreHandle roaming_sema;

#define RSSI_ROAMING_THRESHOLD -45//when current ap rssi < RSSI_ROAMING_THRESHOLD, start connect to an other better ap.
#define RSSI_SCAN_THRESHOLD -40	//when current ap rssi < RSSI_SCAN_THRESHOLD, start to scan a better ap.
#define MAX_CH_NUM 4
#define SCAN_BUFLEN 500 			//each scan list length= 14 + ssid_length(32MAX). so SCAN_BUFLEN should be AP_NUM*(14+32) at least
#define ROAMING_DBG_ENABLE 1 			//for debug log

extern u8 pmu_is_roaming_awake_fw();
extern void pmu_degrade_awake(u8);
extern void pmu_reset_awake(u8);
extern void pmu_set_roaming_awake(u8 enable, u8 threshhold);
extern int wifi_get_bcn_rssi(int *pRSSI);

#if	ROAMING_DBG_ENABLE
#define ROAMING_DBG	printf
#else
#define ROAMING_DBG
#endif

typedef struct wifi_roaming_ap
{
	u8 	ssid[33];
	u8 	bssid[ETH_ALEN];
	u8	channel;
	rtw_security_t		security_type;
	u8 	password[65];
	u8	key_idx;
	s32	rssi;		
#if CONFIG_LWIP_LAYER
	u8	ip[4];
#endif
}wifi_roaming_ap_t;

struct roaming_data {
	u8 num;
	struct wlan_fast_reconnect ap_info;
	u32 channel[MAX_CH_NUM];
};

typedef struct channel_plan_2g
{
	u8	channel[14];
	u8	len;
}channel_plan_2g_t;

channel_plan_2g_t channel_plan_2g = {{1,2,3,4,5,6,7,8,9,10,11,12,13}, 13};

enum {
	FAST_CONNECT_SPECIFIC_CH = 0,
	FAST_CONNECT_ALL_CH  = 1
};
typedef u8 fast_connect_ch_t;

#if CONFIG_LWIP_LAYER
extern struct netif xnetif[NET_IF_NUM]; 
#endif
static wifi_roaming_ap_t *ap_list;
static u8 pscan_enable = _TRUE; // if set _TRUE, please set pscan_channel_list
static u8 pscan_channel_list[]={1};// set by customer

#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
extern uint32_t offer_ip;
#endif


int wifi_roaming_write_ap_to_flash(u8 * data, u32 len)
{
	flash_t flash;
	u8 i=0;
	struct roaming_data read_data = {0};
	u8 ap_change = 0;

	ROAMING_DBG("wifi_roaming_write_ap_to_flash\n");
	if(!data)
            return -1;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct  roaming_data), (u8 *) &read_data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	if(read_data.num == 0xff)
		read_data.num = 0;
	if(read_data.num < MAX_CH_NUM) {
		if(read_data.num){
			/*check if ap info has changed*/
			if(memcmp((u8 *)((struct wlan_fast_reconnect *)data)->psk_essid, (u8 *) (read_data.ap_info.psk_essid), 32)) {
				printf("ap ssid change\n");
				ap_change = 1;
			} else if(memcmp((u8 *)((struct wlan_fast_reconnect *)data)->psk_passphrase, (u8 *) (read_data.ap_info.psk_passphrase), 32)) {
				printf("ap password change\n");
				ap_change = 1;
			} else if(((struct wlan_fast_reconnect *)data)->security_type != read_data.ap_info.security_type) {
				printf("ap security type change\n");
				ap_change = 1;
			} else { /*ap info doesn't change*/
				for(i = 0;i < read_data.num; i++){
					if(read_data.channel[i] == ((struct wlan_fast_reconnect *)data)->channel) {
						ROAMING_DBG("Already stored this channel(%d)\n",((struct wlan_fast_reconnect *)data)->channel);
						 return -1;
					}		
				}
			}		
		}
		if(ap_change) {
			printf("erase flash and restore new ap info\n");
			memset((u8 *)&read_data,0,sizeof(struct  roaming_data));
			read_data.num = 1;
		} else {
			printf("Add a new ap into flash\n");
			read_data.num++;
		}
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_erase_sector(&flash, FAST_RECONNECT_DATA);	
		read_data.channel[read_data.num-1] = ((struct wlan_fast_reconnect *)data)->channel;//store channel
		if(read_data.num == 1)
			memcpy((u8 *)&read_data.ap_info,data,sizeof(struct wlan_fast_reconnect));//store fast connect info
		flash_stream_write(&flash, FAST_RECONNECT_DATA, sizeof(struct  roaming_data), (uint8_t *)  &read_data);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
	} else
		ROAMING_DBG("\r\n %s():flash has %d channels, no need to add more\n", __func__,read_data.num);
	
	return 0;
}

int wlan_fast_connect(struct wlan_fast_reconnect *data, u8 scan_type )
{
	uint32_t	channel;
	uint32_t    security_type;
	uint8_t     pscan_config;
	char key_id[2] = {0};
	int ret;
	uint32_t wifi_retry_connect = 3;//For fast wifi connect retry
	rtw_network_info_t wifi = {0};

#if CONFIG_LWIP_LAYER
	netif_set_up(&xnetif[0]);
#endif

#if CONFIG_AUTO_RECONNECT
	wifi_set_autoreconnect(0);
#endif
	//time_mers_set();
	ROAMING_DBG("\r\n %s()\n", __func__);
	memcpy(psk_essid, data->psk_essid, sizeof(data->psk_essid));
	memcpy(psk_passphrase, data->psk_passphrase, sizeof(data->psk_passphrase));
	memcpy(wpa_global_PSK, data->wpa_global_PSK, sizeof(data->wpa_global_PSK));

	channel = data->channel;
	sprintf(key_id,"%d",(char) (channel>>28));
	channel &= 0xff;
	security_type = data->security_type;
	pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
	//set partial scan for entering to listen beacon quickly
WIFI_RETRY_LOOP:
	if(scan_type == FAST_CONNECT_SPECIFIC_CH) {
		ret = wifi_set_pscan_chan((uint8_t *)&channel, &pscan_config, 1);
		if(ret < 0){
			free(data);
			return -1;
		}
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
                  printf("wifi retry\r\n");
                  goto WIFI_RETRY_LOOP;
              }
	}
	if(ret == RTW_SUCCESS){
		LwIP_DHCP(0, DHCP_START);
	}
#if CONFIG_AUTO_RECONNECT
	wifi_set_autoreconnect(1);
#endif
	return ret;
}

int wifi_roaming_init_done_callback(void)
{
	flash_t		flash;
	struct roaming_data read_data = {0};

#if WIFI_ROAMING_TICKLESS_MONITOR
	roaming_type_flag = 0;
	pmu_set_roaming_awake(1, 0-RSSI_SCAN_THRESHOLD);
#endif
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct  roaming_data), (u8 *) &read_data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	/* Check whether stored flash profile is empty */
	if(read_data.num == 0 || read_data.num > MAX_CH_NUM){
		printf("Fast connect profile is empty, abort fast connection\n");
	}
	/* Find the best ap in flash profile */
	else {
		ROAMING_DBG("[Fast connect] Connect to the best ap\n");
		wlan_fast_connect(&read_data.ap_info,FAST_CONNECT_ALL_CH);
	}
	return 0;
}

void example_tickless_wifi_roaming_init(void)
{	
	// Call back from wlan driver after wlan init done
	p_wlan_init_done_callback = wifi_roaming_init_done_callback;

	// Call back from application layer after wifi_connection success
	p_write_reconnect_ptr = wifi_roaming_write_ap_to_flash;
}
    
u32 wifi_roaming_find_ap(char*buf, int buflen, char *target_ssid, void *user_data)
{
	u32 target_security = *(u32 *)user_data;
	
	u32 plen = 0;	
	while(plen < buflen)
	{
		u32 len, ssid_len, security_mode, security_type, channel;
		s32 rssi;
		u8 *mac, *ssid;				
		// len
		len = (int)*(buf + plen);
		// check end
		if(len == 0|| len == strlen(target_ssid)) break;// if len == ssid_len, it means driver dont do scan,maybe it is busy now, buf detail is the same as it initialized
		// mac
		mac =(u8*)(buf + plen + 1);
		// rssi
		rssi = *(s32*)(buf + plen + 1 + 6);
		// security_mode offset = 11
		security_mode = (u8)*(buf + plen + 1 + 6 + 4);
        printf("wifi_roaming_find_ap %x \r\n",security_mode);
		switch(security_mode){
			case IW_ENCODE_ALG_NONE:
				security_type = RTW_SECURITY_OPEN;
				break;
			case IW_ENCODE_ALG_WEP:
				security_type = RTW_SECURITY_WEP_PSK;
				break;
			case IW_ENCODE_ALG_TKIP:
				security_type = RTW_SECURITY_WPA_TKIP_PSK;
				break;
			case IW_ENCODE_ALG_CCMP:
				security_type = RTW_SECURITY_WPA2_AES_PSK;
				break;
			default:
				break;
		}
		// channel
		channel = *(buf + plen + 1 + 6 + 4 + 1 + 1);
		// ssid
		ssid_len = len - 1 - 6 - 4 - 1 - 1 - 1;
		ssid = (u8*)(buf + plen + 1 + 6 + 4 + 1 + 1 + 1);
		ROAMING_DBG("Scan ap:"MAC_FMT" channel %d %s %d\n", MAC_ARG(mac),channel, ssid, ssid_len);
		if(target_security == security_type ||
		((target_security & (WPA2_SECURITY|WPA_SECURITY))&&(security_type & (WPA2_SECURITY|WPA_SECURITY)))){
			if(ap_list->rssi < rssi){
				ROAMING_DBG("rssi(%d) is better than last(%d)\n",rssi,ap_list->rssi);
				memset(ap_list, 0 , sizeof(wifi_roaming_ap_t));
				memcpy(ap_list->bssid, mac, ETH_ALEN);
				ap_list->channel = channel;
				ap_list->rssi = rssi;
			}
		}
		plen += len;			
	}
	return 0;
}

int wifi_roaming_scan(struct roaming_data  read_data, s32 cur_rssi)
{
	wifi_roaming_ap_t	roaming_ap;
	rtw_wifi_setting_t	setting;
	u8	pscan_config;
	channel_plan_2g_t channel_plan_temp_2g =  channel_plan_2g;
	u8 ch = 0;

	//static u8 pmu_flag = 0;
	
	memset(&setting, 0, sizeof(rtw_wifi_setting_t));
	memset(&roaming_ap, 0, sizeof(wifi_roaming_ap_t));
	roaming_ap.rssi = -100;

	wifi_get_setting(WLAN0_NAME,&setting);
	strcpy((char*)roaming_ap.ssid, (char const*)setting.ssid);
	roaming_ap.security_type =  setting.security_type;
	strcpy((char*)roaming_ap.password, (char const*)setting.password);
	roaming_ap.key_idx = setting.key_idx;

	if(RTW_ERROR == wifi_get_ap_bssid(roaming_ap.bssid))
		printf("\r\n get AP BSSID FAIL!");


	pmu_acquire_wakelock(PMU_ROAMING_TICKLESS);

	if(pscan_enable == _TRUE){
		/*scan specific channels*/
		if(0 < read_data.num && read_data.num < MAX_CH_NUM){
			ROAMING_DBG("\r\n %s():try to find a better ap in flash\n", __func__);
			while(read_data.num) {
				//time_mers_set();
				pscan_channel_list[0]=read_data.channel[read_data.num - 1];
				read_data.num--;
				pscan_config = PSCAN_ENABLE;
				wifi_set_pscan_chan(pscan_channel_list, &pscan_config, 1);
				wifi_scan_networks_with_ssid((int (*)(char *, int, char *, void *))wifi_roaming_find_ap, 
				(void *)&roaming_ap.security_type, SCAN_BUFLEN, (char*)roaming_ap.ssid, strlen((char const*)roaming_ap.ssid));
				channel_plan_temp_2g.channel[pscan_channel_list[0] -1] = 0;
				//time_mers_set();
			//	vTaskDelay(500);
			}
		}
		if(ap_list->rssi - cur_rssi > 10) {
			ROAMING_DBG("\r\n %s():Find a best ap in flash saved channels suc.its rssi(%d)\n", __func__,ap_list->rssi);
		} else {
			/*scan other channels*/
			ROAMING_DBG("\r\n %s():Find a best ap in flash saved channels fail,try other channels.its rssi(%d),cur_rssi(%d) \n", __func__,ap_list->rssi, cur_rssi);
			for(ch = 0;ch < channel_plan_temp_2g.len;ch++) {
				if(channel_plan_temp_2g.channel[ch]) {
					pscan_channel_list[0]=channel_plan_temp_2g.channel[ch];
					pscan_config = PSCAN_ENABLE;	
 					wifi_set_pscan_chan(pscan_channel_list, &pscan_config, 1);
					wifi_scan_networks_with_ssid((int (*)(char *, int, char *, void *))wifi_roaming_find_ap, 
						(void *)&roaming_ap, SCAN_BUFLEN, (char*)roaming_ap.ssid, strlen((char const*)roaming_ap.ssid));
					//vTaskDelay(500);
				}
			}
		}
	}else {
		wifi_scan_networks_with_ssid((int (*)(char *, int, char *, void *))wifi_roaming_find_ap, 
			(void *)&roaming_ap, SCAN_BUFLEN, (char*)roaming_ap.ssid, strlen((char const*)roaming_ap.ssid));
	}

	pmu_release_wakelock(PMU_ROAMING_TICKLESS);
	
	if(memcmp(roaming_ap.bssid, ap_list->bssid, ETH_ALEN)){
		ROAMING_DBG("\r\n %s():Find a ap\n", __func__);
		return 1;
	} else{
		ROAMING_DBG("\r\n %s():NO FIND anyother AP\n", __func__);
		return 0;
	}
					
}
void wifi_tickless_roaming_thread(void *param)
{
	int	ap_rssi;
	//unsigned char wake_delay = 0;
	//unsigned char wake_range = 0;
	struct roaming_data read_data = {0};
	flash_t flash;

	while(1){//wait wifi connect
		if(wifi_is_up(RTW_STA_INTERFACE)&&(RTW_SUCCESS == wifi_is_ready_to_transceive(RTW_STA_INTERFACE)))
			break;
		else
			vTaskDelay(1000);
	}
//	vTaskDelay(2000);//wait rssi stable

	while(1)	{
		ROAMING_DBG("\r\n==> wait sema\n");
		roaming_type_flag =0;
		while(rtw_down_timeout_sema((void*)&roaming_sema, portMAX_DELAY) != pdTRUE);
		if(roaming_type_flag == 1)
			ROAMING_DBG("\r\n==> tickless roaming\n");
		else if(roaming_type_flag == 2)
			ROAMING_DBG("\r\n==> normal roaming\n");
		else if(roaming_type_flag == 3)
			ROAMING_DBG("\r\n==> DecisionDisconnect  roaming\n");
		else 
			ROAMING_DBG("\r\n==> other reason roaming\n");
		if(wifi_is_up(RTW_STA_INTERFACE)) {		
			//wifi_get_rssi(&ap_rssi);
			wifi_get_bcn_rssi(&ap_rssi);
		//	ROAMING_DBG("\r\n %s():Check if need to scan an other ap(rssi:%d)\n", __func__,ap_rssi);
			if((ap_rssi < RSSI_SCAN_THRESHOLD)) {
			//	if( polling_count >= 1) {
					ROAMING_DBG("\r\n %s():Start scan, current rssi(%d) < scan threshold rssi(%d) \n", __func__,ap_rssi,RSSI_SCAN_THRESHOLD);
					ap_list = (wifi_roaming_ap_t *)malloc(sizeof(wifi_roaming_ap_t));
					memset(ap_list, 0, sizeof(wifi_roaming_ap_t));
					ap_list->rssi = -100;
					device_mutex_lock(RT_DEV_LOCK_FLASH);

					flash_stream_read(&flash, FAST_RECONNECT_DATA, sizeof(struct  roaming_data), (u8 *) &read_data);
					device_mutex_unlock(RT_DEV_LOCK_FLASH);
	
					/*find a better ap*/
					if(wifi_roaming_scan(read_data,ap_rssi)) {
						//wifi_get_rssi(&ap_rssi);
						wifi_get_bcn_rssi(&ap_rssi);
						if(ap_list->rssi - ap_rssi > 10) {
							ROAMING_DBG("\r\n %s():The found ap IS Better, rssi(%d)\n", __func__,ap_list->rssi);				
							if(ap_rssi < RSSI_ROAMING_THRESHOLD) {
								pmu_reset_awake(1);
								pmu_reset_awake(2);
								/*connect a better ap*/
								ROAMING_DBG("\r\n %s():Start roaming, current rssi(%d) < threshold(%d),target ap(%d)\n", __func__,ap_rssi,RSSI_ROAMING_THRESHOLD,ap_list->rssi);
								read_data.ap_info.channel = ap_list->channel;

								wlan_fast_connect(&read_data.ap_info,FAST_CONNECT_SPECIFIC_CH);
							}else if(ap_rssi > RSSI_SCAN_THRESHOLD) {
								/*no need to roaming*/
								ROAMING_DBG("\r\n %s():Current rssi=%d,no need to roaming\n",__func__,ap_rssi);
							}
						}else {
							ROAMING_DBG("\r\n %s():The found ap ISNOT Better, rssi(%d)\n", __func__,ap_list->rssi);
							pmu_degrade_awake(roaming_type_flag);
						}
						//	vTaskDelay(1000);
					} else {
                        ROAMING_DBG("\r\n %s():can't find any AP!!!\n", __func__);
						pmu_degrade_awake(roaming_type_flag);
					}
					free(ap_list);

			//		polling_count = 0;	
			//	}
			//	else				
			//		polling_count++;
			}else {
                ROAMING_DBG("\r\n %s():ap_rssi %d > RSSI_SCAN_THRESHOLD\n", __func__,ap_list->rssi);
                pmu_degrade_awake(roaming_type_flag);
			}
		//	else
		//		polling_count = 0;
		}
	//	vTaskDelay(1000);
	}
	//vTaskDelete(NULL);
}

void example_tickless_wifi_roaming(void)
{
	if(xTaskCreate(wifi_tickless_roaming_thread, ((const char*)"wifi_tickless_roaming_thread"), 1024, NULL, tskIDLE_PRIORITY + 5 , NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(wifi_tickless_roaming_thread) failed", __FUNCTION__);

	return;
}
#endif