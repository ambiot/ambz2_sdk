#ifndef __EXAMPLE_FAST_RECONNECTION_H__
#define __EXAMPLE_FAST_RECONNECTION_H__


/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/
#include "FreeRTOS.h"
#include <autoconf.h>
#include "main.h"

#define IW_WPA2_PASSPHRASE_MAX_SIZE 64
#define IW_WPA3_PASSPHRASE_MAX_SIZE 128
#define IW_PASSPHRASE_MAX_SIZE IW_WPA3_PASSPHRASE_MAX_SIZE
//#define FAST_RECONNECT_DATA (0x80000 - 0x1000)
#define NDIS_802_11_LENGTH_SSID         32
#define A_SHA_DIGEST_LEN		20

#define FAST_RECONN_VERSION		        (FAST_RECONNECT_DATA + 0xFF0)

struct wlan_fast_reconnect {
	unsigned char psk_essid[NDIS_802_11_LENGTH_SSID + 4];
	unsigned char psk_passphrase[IW_PASSPHRASE_MAX_SIZE + 1];
	unsigned char wpa_global_PSK[A_SHA_DIGEST_LEN * 2];
	uint32_t	channel;
	uint32_t    security_type;
#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
	uint32_t offer_ip;
	uint32_t server_ip;
#endif
#if ATCMD_VER == ATVER_2
	uint32_t    enable;
#endif
};	

struct wlan_fast_reconnect_prior {
	unsigned char psk_essid[32 + 4];
	unsigned char psk_passphrase[64 + 1];
	unsigned char wpa_global_PSK[20 * 2];
	uint32_t	channel;
	uint32_t    security_type;
#if defined(CONFIG_FAST_DHCP) && CONFIG_FAST_DHCP
	uint32_t offer_ip;
#endif
#if ATCMD_VER == ATVER_2
	uint32_t    enable;
#endif
};



typedef int (*wlan_init_done_ptr)(void);
typedef int (*write_reconnect_ptr)(uint8_t *data, uint32_t len);


//Variable
extern unsigned char psk_essid[NET_IF_NUM][NDIS_802_11_LENGTH_SSID+4];
extern unsigned char psk_passphrase[NET_IF_NUM][IW_PASSPHRASE_MAX_SIZE + 1];
extern unsigned char wpa_global_PSK[NET_IF_NUM][A_SHA_DIGEST_LEN * 2];
extern unsigned char psk_passphrase64[IW_WPA2_PASSPHRASE_MAX_SIZE + 1];

//Function
extern wlan_init_done_ptr p_wlan_init_done_callback;
extern write_reconnect_ptr p_write_reconnect_ptr;
 
void example_wlan_fast_connect(void);

#endif //#ifndef __EXAMPLE_FAST_RECONNECTION_H__
