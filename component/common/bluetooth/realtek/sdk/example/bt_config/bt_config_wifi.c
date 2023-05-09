#include <platform_opts_bt.h>
#if (defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG)
#include "platform_stdlib.h"
#if defined(CONFIG_PLATFORM_8721D)
#include "ameba_soc.h"
#endif
#include "bt_config_wifi.h"
#include <osdep_service.h>
#include <gap_conn_le.h>
#include <gap_adv.h>
#include <lwip_netconf.h>
#include "wifi_conf.h"
#include "dhcp/dhcps.h"
#include "bt_config_app_task.h"
#include "os_mem.h"
#include "os_sync.h"
#include "os_task.h"
#include "os_sched.h"

extern void bt_config_app_deinit(void);
extern T_GAP_CONN_STATE bt_config_gap_conn_state;
extern T_GAP_DEV_STATE bt_config_gap_dev_state;

// a temp variable for wifi scan
static void *wifi_scan_sema = NULL;
static void *BC_status_monitor_task_hdl = NULL;

static uint8_t pscan_channel_2G[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};
static uint8_t pscan_channel_5G[] = {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165};

extern uint8_t airsync_specific;

static rtw_result_t scan_result_handler(rtw_scan_handler_result_t* scan_result)
{
	struct BC_wifi_scan_result* BC_scan_result = (struct BC_wifi_scan_result*) scan_result->user_data;
	
	if (scan_result->scan_complete != RTW_TRUE) {
		if (BC_scan_result->ap_num < BC_MAX_WIFI_SCAN_AP_NUM)
		{
			rtw_scan_result_t* record = &scan_result->ap_details;
			memcpy(&(BC_scan_result->ap_info[BC_scan_result->ap_num]), record, sizeof(rtw_scan_result_t));
			BC_scan_result->ap_num++;
		}
	} else{
		os_sem_give(wifi_scan_sema);
	}
	return RTW_SUCCESS;
}

BC_band_t BC_req_band_hdl(void)
{
	u8 Band;
	BC_printf("Band Request");
	Band = wifi_get_band_type();

	if(Band == WL_BAND_2_4G) {
		return BC_BAND_2G;
	} else if (Band == WL_BAND_5G) {
		return BC_BAND_5G;
	} else {
		return BC_BAND_2G_5G;
	}
}

int BC_req_scan_hdl(BC_band_t band, struct BC_wifi_scan_result* BC_scan_result)
{
	int ret = -1;
	uint8_t *pscan_config = NULL;
	uint8_t *pscan_channel;
	int pscan_config_size;
	
	BC_printf("Scan Request");
	memset(BC_scan_result, 0, sizeof(struct BC_wifi_scan_result));
	
	if (band == BC_BAND_2G) {		// 2.4G
		pscan_channel = pscan_channel_2G;
		pscan_config_size = sizeof(pscan_channel_2G);
	}
	else if (band == BC_BAND_5G) {	// 5G
		pscan_channel = pscan_channel_5G;
		pscan_config_size = sizeof(pscan_channel_5G);
	}
	
	pscan_config = (uint8_t*)os_mem_alloc(RAM_TYPE_DATA_ON, pscan_config_size);
	if(pscan_config == NULL) {
		BC_printf("[%s] malloc pscan_config fail!\r\n",__FUNCTION__);
		goto exit;
	}
	
	memset(pscan_config, PSCAN_ENABLE, pscan_config_size);
	ret = wifi_set_pscan_chan(pscan_channel, pscan_config, pscan_config_size);
	if(ret < 0) {
		BC_printf("[%s] wifi set partial scan channel fail\r\n",__FUNCTION__);
		goto exit;
	}
	
	BC_printf("Scan %s AP\r\n", (band == BC_BAND_2G)? "2.4G":"5G");
	os_sem_create(&wifi_scan_sema, 0, 1);
	BC_scan_result->ap_num = 0;
	ret = wifi_scan_networks(scan_result_handler,(void*) BC_scan_result);
	if(ret != RTW_SUCCESS) {
		BC_printf("wifi scan failed (%d)\r\n",ret);
		ret = -1;
	}
	os_sem_take(wifi_scan_sema, 0xFFFFFFFF);
	os_sem_delete(wifi_scan_sema);

exit:
	if(pscan_config != NULL)
		os_mem_free(pscan_config);
	return ret;
}

int BC_req_connect_hdl(uint8_t *ssid, uint8_t *password, uint8_t *bssid, rtw_security_t security, BC_band_t band)
{
	rtw_network_info_t wifi = {0};
	int mode, ret;
	uint32_t tick1, tick2, tick3;
	uint8_t assoc_by_bssid = 0;
	uint8_t DCHP_state;
	(void) band;
	
	BC_printf("Connect Request");
	
	BC_DBG("[Connect] band: [%s], BSSID: ["MAC_FMT"]",
			   (band==BC_BAND_2G)?"2.4G": (band==BC_BAND_5G)?"5G":"UNKNOWN", MAC_ARG(bssid));
	BC_DBG("[Connect] SSID: [%s], password: [%s]\r\n",
			   ssid, password);
	
#if CONFIG_AUTO_RECONNECT
	/* disable auto reconnect */
	wifi_set_autoreconnect(0);
#endif
	
	wifi.ssid.len = strlen((char const*)ssid);
	memcpy(wifi.ssid.val, ssid, wifi.ssid.len);
	wifi.password_len = strlen((char const*)password);
	wifi.password = password;
	memcpy(wifi.bssid.octet,bssid,6);
	wifi.security_type = security;
	wifi.key_id = 0; // WEP key ID missed in BT Config, default WEP key ID 0
	
	tick1 = rtw_get_current_time();

	if (wifi.bssid.octet[0] != 0) {
		assoc_by_bssid = 1;
	}
	else if (wifi.ssid.len == 0) {
		BC_printf("Error: SSID can't be empty\r\n");
		return -1;
	}
	
	//Check if in AP mode
	wext_get_mode(WLAN0_NAME, &mode);
	if(mode != IW_MODE_INFRA) {
        wifi_set_mode(RTW_MODE_STA);
	}

	
	if(assoc_by_bssid){
		ret = wifi_connect_bssid(wifi.bssid.octet, (char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, 
						6, wifi.ssid.len, wifi.password_len, wifi.key_id, NULL);		
	} else {
		ret = wifi_connect((char*)wifi.ssid.val, wifi.security_type, (char*)wifi.password, wifi.ssid.len,
					wifi.password_len, wifi.key_id, NULL);
	}
	
	if(ret != RTW_SUCCESS){
		BC_printf("ERROR: Can't connect to AP\r\n");
		return ret;
	}
	tick2 = rtw_get_current_time();
	BC_printf("Connected after %dms.\r\n", (tick2-tick1));
	
#if CONFIG_LWIP_LAYER
	/* Start DHCPClient */
	DCHP_state = LwIP_DHCP(0, DHCP_START);

	if ( DCHP_state != DHCP_ADDRESS_ASSIGNED) {
		return -1;
	}
	tick3 = rtw_get_current_time();
	BC_printf("Got IP after %dms.\r\n", (tick3-tick1));
#endif
	
	return 0;
}

void BC_req_status_hdl(BC_status_t *status, uint8_t *SSID, uint8_t *BSSID, rtw_security_t *security, uint8_t *channel, int *rssi)
{
	rtw_wifi_setting_t setting;
	int last_conn_error;
	last_conn_error = wifi_get_last_error();
	
	//BC_printf("Status Request");
	if (!rltk_wlan_running(WLAN0_IDX)) {
		*status = BC_STATE_DISABLED;
		BC_printf("%s is disabled\r\n", WLAN0_NAME);
	}
	else if (last_conn_error == RTW_WRONG_PASSWORD) {
		*status = BC_STATE_WRONG_PASSWORD;
		BC_printf("Wrong Password\r\n");
	}
	else if (wifi_is_ready_to_transceive(RTW_STA_INTERFACE)== RTW_SUCCESS) {
		if (wifi_get_setting(WLAN0_NAME,&setting) != -1) {
			*status = BC_STATE_CONNECTED;
			memcpy(SSID, setting.ssid, BC_MAX_SSID_LEN);
			wext_get_bssid(WLAN0_NAME, BSSID);
			*security = setting.security_type;
			*channel = setting.channel;
			wifi_get_rssi(rssi);
			
			BC_DBG("Connected, SSID: [%s], BSSID: ["MAC_FMT"], channel: [%d], rssi: [%d]",
				    SSID, MAC_ARG(BSSID), *channel, *rssi);	
		}
		else {
			*status= BC_STATE_IDLE;
			BC_DBG("Wifi not Connected\r\n");
		}
	}
	else {
		*status = BC_STATE_IDLE;
		BC_DBG("Wifi not Connected\r\n");
	}
}

void BC_status_monitor(void *p_param)
{
	(void)p_param;
	T_GAP_CONN_STATE gap_conn_state;

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	osif_create_secure_context(configMINIMAL_SECURE_STACK_SIZE);
#endif

	while (1)
	{
		os_delay(500);
		if ((wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS)) {	// wifi connected
#if defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG
			if (airsync_specific) {
				gap_conn_state = bt_airsync_config_gap_conn_state;
			} else
#endif
			{
				gap_conn_state = bt_config_gap_conn_state;
			}

			if (gap_conn_state != GAP_CONN_STATE_CONNECTED) {					// BT not connected
				if (airsync_specific) {
					BC_printf("[%s] wifi connected, deinit BT Airsync Config\r\n", __FUNCTION__);
				} else {
					BC_printf("[%s] wifi connected, deinit BT Config\r\n", __FUNCTION__);
				}
				break;
			}
		}
	}
	
	BC_status_monitor_task_hdl = NULL;
#if defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG
	if (airsync_specific) {
		bt_airsync_config_app_deinit();
	} else
#endif
	{
		bt_config_app_deinit();
	}

	os_task_delete(NULL);
}

void bt_config_wifi_init(void)
{
	BC_cmd_task_init();

	if (BC_status_monitor_task_hdl == NULL) {
		if(os_task_create(&BC_status_monitor_task_hdl, (char const *)"BC_status_monitor", BC_status_monitor, NULL, 512, 1) != true){
			BC_printf("[%s] Create BC_status_monitor failed\r\n", __FUNCTION__);
		}
	}
	else {
		BC_printf("BC_status_monitor already on\r\n");
	}
}

void bt_config_wifi_deinit(void)
{
	T_GAP_DEV_STATE gap_dev_state;

	// Stop ADV
#if defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG
	if (airsync_specific) {
		bt_airsync_config_send_msg(0);
	} else
#endif
	{
		bt_config_send_msg(0);
	}

	do {
		os_delay(1);
#if defined(CONFIG_BT_AIRSYNC_CONFIG) && CONFIG_BT_AIRSYNC_CONFIG
		if (airsync_specific) {
			gap_dev_state = bt_airsync_config_gap_dev_state;
		} else
#endif
		{
			gap_dev_state = bt_config_gap_dev_state;
		}
	} while (gap_dev_state.gap_adv_state != GAP_ADV_STATE_IDLE);

	BC_cmd_task_deinit();
	if (BC_status_monitor_task_hdl != NULL) {
		os_task_delete(BC_status_monitor_task_hdl);
		BC_status_monitor_task_hdl = NULL;
	}
	airsync_specific = 0;
}
#endif
