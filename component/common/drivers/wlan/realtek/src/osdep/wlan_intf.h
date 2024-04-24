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
#ifndef __WLAN_INTF_H__
#define __WLAN_INTF_H__

#ifdef	__cplusplus
extern "C" {
#endif
#include <autoconf.h>

#include <wireless.h>
#include "wifi_constants.h"
#include "wifi_structures.h"
#include "freertos/wrapper.h"

#ifndef WLAN0_IDX
#define WLAN0_IDX	0
#endif
#ifndef WLAN1_IDX
#define WLAN1_IDX	1
#endif
#ifndef WLAN_UNDEF
#define WLAN_UNDEF	-1
#endif

/***********************************************************/
/*
struct sk_buff {
	// These two members must be first.
	struct sk_buff		*next;		// Next buffer in list
	struct sk_buff		*prev;		// Previous buffer in list

	struct sk_buff_head	*list;			// List we are on
	unsigned char		*head;		// Head of buffer
	unsigned char		*data;		// Data head pointer
	unsigned char		*tail;		// Tail pointer
	unsigned char		*end;		//End pointer
	struct net_device 	*dev;		//Device we arrived on/are leaving by
	unsigned int 		len;			// Length of actual data
};
*/
/************************************************************/

//----- ------------------------------------------------------------------
// Wlan Interface opened for upper layer
//----- ------------------------------------------------------------------
int rltk_wlan_init(int idx_wlan, rtw_mode_t mode);				//return 0: success. -1:fail
void rltk_wlan_deinit(void);
void rltk_wlan_deinit_fastly(void);
void rltk_wlan_deinit_if2(void);
int rltk_wlan_start(int idx_wlan);
unsigned char rltk_wlan_check_isup(int idx);
void rltk_wlan_tx_inc(int idx);
void rltk_wlan_tx_dec(int idx);
struct sk_buff *rltk_wlan_get_recv_skb(int idx);
struct sk_buff *rltk_wlan_alloc_skb(unsigned int total_len);
void rltk_wlan_send_skb(int idx, struct sk_buff *skb);
void rltk_netif_rx(struct sk_buff *skb);
int rltk_wlan_get_sta_max_data_rate(unsigned char *inidata_rate);
int rltk_set_sta_num(unsigned char ap_sta_num);
int rltk_del_station(const char *ifname, unsigned char *hwaddr);
int rltk_get_auto_chl(const char *ifname, unsigned char *channel_set, unsigned char channel_num);
int rltk_set_tx_power_percentage(rtw_tx_pwr_percentage_t power_percentage_idx);
int rltk_set_tx_power_db_offset(signed char power_db_offset);
int rltk_wlan_control(unsigned long cmd, void *data);
int rltk_change_mac_address_from_ram(int idx, unsigned char *mac);
unsigned char rltk_wlan_running(unsigned char idx);
#ifdef CONFIG_MCC_STA_AP_MODE
int rltk_wlan_get_macid(unsigned char *macid0, unsigned char *macid1);
int rltk_wlan_txrpt_statistic(const char *ifname, rtw_fw_txrpt_stats_t *txrpt_stats);
void rltk_wlan_enable_ccx_txrpt(const char *ifname, int enable);
int rltk_wlan_ccx_txrpt_retry(rtw_fw_txrpt_retry_t *txrpt_retry);
int rltk_wlan_set_mgnt_retry_lmt(unsigned char retry_limit);
int rltk_wlan_get_mgnt_retry_lmt(void);
int rltk_wlan_set_data_retry_lmt(unsigned char retry_limit);
int rltk_wlan_get_data_retry_lmt(void);
int rltk_wlan_set_aggregation(unsigned char option, rtw_ampdu_mode_t path);
int rltk_wlan_get_aggregation(rtw_ampdu_mode_t path);
int rltk_wlan_set_block_bc_mc_packet(unsigned char enable, unsigned char types);
int rltk_wlan_get_block_bc_mc_packet(unsigned char types);
#endif

void rltk_wlan_statistic(unsigned char idx);
int rltk_wlan_handshake_done(void);
int rltk_wlan_rf_on(void);
int rltk_wlan_rf_off(void);
int rltk_wlan_check_bus(void);
int rltk_wlan_wireless_mode(unsigned char mode);
int rltk_wlan_get_wireless_mode(unsigned char *pmode);
int rltk_wlan_get_cur_wireless_mode(void);
int rltk_wlan_set_wpa_mode(const char *ifname, unsigned int wpa_mode);
int rltk_wlan_set_wps_phase(unsigned char is_trigger_wps);
void rltk_wlan_PRE_SLEEP_PROCESSING(void);
int rltk_wlan_is_connected_to_ap(void);
int rltk_wlan_set_rate_bitmap(unsigned int rate_bitmap);
int rltk_wlan_set_auto_rate_fallback(unsigned int rate_fallback_1_4, unsigned int rate_fallback_5_8);
int rltk_wlan_set_retry_limit(unsigned char retry_limit);
void rltk_wlan_set_no_beacon_timeout(unsigned char timeout_sec);
void rltk_wlan_set_scan_chan_interval(unsigned short interval_ms);
void rltk_wlan_map_in_efuse(unsigned char map_in_efuse);
int rltk_wlan_map_read(unsigned char *data, unsigned short cnts);

#ifdef CONFIG_IEEE80211W
void rltk_wlan_tx_sa_query(unsigned char key_type);
void rltk_wlan_tx_deauth(unsigned char b_broadcast, unsigned char key_type);
void rltk_wlan_tx_auth(void);
#endif

void rltk_wlan_pwrlmt_ext_enable(unsigned char enable);
unsigned char rltk_wlan_pwrlmt_ext_is_enable(void);

#if CONFIG_MAC_ADDRESS
unsigned char *rltk_get_mac(void);
int  rltk_check_mac(void);
void rltk_set_mac(unsigned char *mac);
#endif

int rtw_ps_enable(int enable);
void rltk_wlan_btcoex_set_bt_state(unsigned char state);
unsigned char rltk_wlan_is_mp(void);

#if defined (CONFIG_AP_MODE)
int rltk_suspend_softap(const char *ifname);
int rltk_suspend_softap_beacon(const char *ifname);
int rltk_remove_softap_in_concurrent_mode(const char *ifname);
int rltk_resume_softap(const char *ifname);
int rltk_reattach_softap_in_concurrent_mode(const char *ifname);
#endif

#ifdef CONFIG_BT_COEXIST_SOC
int rltk_coex_set_wifi_slot(unsigned char wifi_slot);
int rltk_coex_ble_scan_duty_update(unsigned char duty);
int rltk_coex_set_ble_scan_duty(unsigned char duty);  //Z2 old function
int rltk_coex_set_wifi_scan_slot_time(unsigned char slot_time);
int rltk_coex_set_wlan_slot_preempting(unsigned char bitmask);
void rltk_coex_enable_null_frame(void);
void rltk_coex_disable_null_frame(void);
void rltk_coex_keep_bt_scan_for_wpa3(void);
void rltk_coex_stop_bt_scan_for_wpa3(void);
#endif

u8 rltk_wlan_ap_compatibility_is_enable(u32 bitmap);
void rltk_wlan_enable_preassociate(unsigned char enable);
void rltk_wlan_enable_check_bcn_info(unsigned char enable);
void rltk_wlan_enable_issue_deauth(unsigned char enable);
void rltk_wlan_enable_wep_auth_algo_switch(unsigned char enable);
void rltk_wlan_enable_delayed_reordering(unsigned char enable);
void rltk_wlan_optimize_rx_compulsorily(unsigned char enable);

#ifdef CONFIG_WLAN_SWITCH_MODE
int rltk_wlan_reinit_drv_sw(const char *ifname, rtw_mode_t mode);
int rltk_set_mode_prehandle(rtw_mode_t curr_mode, rtw_mode_t next_mode, const char *ifname);
int rltk_set_mode_posthandle(rtw_mode_t curr_mode, rtw_mode_t next_mode, const char *ifname);
#endif

int rltk_get_thermal_meter(u32 *thermal_value);

#if (RTL8710C_SUPPORT == 1)
int rltk_phydm_set_nbi(const char *ifname, BOOL enable, u32 f_intf);
#endif

void rltk_wlan_set_partial_scan_retry_times(unsigned char times);

#ifdef CONFIG_IEEE80211W
int rltk_set_pmf(unsigned char mode);
#endif
unsigned char rltk_get_band_type(void);
int rltk_wlan_get_statistic(const char *ifname, struct net_device_stats *stats);
int rltk_wlan_get_rx_statistics(struct net_rx_stats *rx_stats);
void rltk_wlan_set_rts_threshold(__u16 rts_threshold);
void rltk_wlan_set_rts_rate(__u8 rts_rate);
void rltk_wlan_set_vcs_mode(__u8 vrtl_carrier_sense_mode);
int rltk_wlan_resume_powersave(void);
int rltk_wlan_disable_powersave(void);

#if CONFIG_AUTO_RECONNECT
unsigned char *rltk_wlan_get_saved_bssid(void);
#endif

unsigned char rltk_wlan_scan_with_ssid_by_extended_security_is_enable(void);
void rltk_wlan_enable_scan_with_ssid_by_extended_security(unsigned char enable);
int rltk_wlan_set_autoreconnect(const char *ifname, __u8 mode, __u8 retry_times, __u16 timeout);
int rltk_wlan_get_autoreconnect(const char *ifname, __u8 *mode);
void rltk_set_normal_rx_under_promisc_ap(int enable);
void rltk_wlan_set_link_err(u32 errbit);
u32 rltk_wlan_get_link_err(void);
int rltk_wlan_beacon_change_count_set(unsigned char count);
int rltk_get_all_rssi(unsigned char *rssi_stats);
unsigned char rltk_wlan_beacon_change_count_get(void);
void rltk_set_connect_retry_limit(unsigned char reauth_limit, unsigned char reassoc_limit, unsigned char eap_limit);
void rltk_set_connect_retry_interval(unsigned short reauth_to, unsigned short reassoc_to, unsigned short eap1_to, unsigned short eap2_to);
void rltk_set_loop_scan_limit(unsigned short lp_scan_limit);
void rltk_set_fast_scan_interval(unsigned short fast_scan_to);
void rltk_set_connect_user_config_enable(unsigned char user_config);
void rltk_wlan_enable_resist_microwave_disturb(unsigned char resist_microwave_disturb);
void rtw_get_wlan_version(char *version);
void rltk_wlan_set_partial_scan_chan_interval(u32 interval_ms);
int rltk_wlan_get_reason_code(const char *ifname, unsigned short *disassoc_reason);
int rltk_wlan_get_country_code(const char *ifname, rtw_country_code_t *country_code);
void rltk_wlan_set_no_tx_time_limit(int limit_time);
int rltk_wlan_set_igi(unsigned char igi, int enable);
int rltk_show_xtal_track_table(void);
int rltk_set_xtal_track_table(int is_positive, int offset, int value);
int rltk_wlan_change_channel_plan(unsigned char channel_plan);

#ifdef	__cplusplus
}
#endif



#endif //#ifndef __WLAN_INTF_H__
