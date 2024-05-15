/******************************************************************************
  *
  * This module is a confidential and proprietary property of RealTek and
  * possession or use of this module requires written permission of RealTek.
  *
  * Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
  *
******************************************************************************/

#ifndef _WIRELESS_H
#define _WIRELESS_H

/***************************** INCLUDES *****************************/


//#include <sockets.h>
#define IFNAMSIZ	16
#define	ARPHRD_ETHER	1	/* ethernet hardware format */


#define WIRELESS_EXT	22



/**************************** CONSTANTS ****************************/
typedef unsigned char __u8;
typedef char __s8;
typedef unsigned short __u16;
typedef short __s16;
typedef unsigned int __u32;
typedef int __s32;
typedef	unsigned long long __u64;
typedef	long long __i64;

#define	E2BIG		 7	/* Argument list too long */

#define ETH_ALEN	6		/* Octets in one ethernet addr	 */


#define SIOCDEVPRIVATE	0x89F0	/* to 89FF */

#define SIOCSIWFREQ	0x8B04		/* set channel/frequency (Hz) */
#define SIOCGIWFREQ	0x8B05		/* get channel/frequency (Hz) */
#define SIOCSIWMODE	0x8B06		/* set operation mode */
#define SIOCGIWMODE	0x8B07		/* get operation mode */
#define SIOCGIWSENS	0x8B09		/* get sensitivity (dBm) */

#define SIOCSIWAP	0x8B14		/* set access point MAC addresses */
#define SIOCGIWAP	0x8B15		/* get access point MAC addresses */
#define SIOCSIWSCAN	0x8B18		/* trigger scanning (list cells) */
#define SIOCGIWSCAN	0x8B19		/* get scanning results */

#define SIOCSIWESSID	0x8B1A		/* set ESSID (network name) */
#define SIOCGIWESSID	0x8B1B		/* get ESSID */

#define SIOCSIWGENIE	0x8B30		/* set generic IE */

#define SIOCSIWAUTH	0x8B32		/* set authentication mode params */
#define SIOCGIWAUTH	0x8B33		/* get authentication mode params */

#define SIOCSIWENCODEEXT 0x8B34		/* set encoding token & mode */
#define SIOCGIWENCODEEXT 0x8B35		/* get encoding token & mode */

#define SIOCSIWPMKSA	0x8B36		/* PMKSA cache operation */

#define SIOCSIWMGNTSEND	0x8B37		/* Send Mgnt Frame or Action Frame */

#define SIOCSIWEAPOLSEND	0x8B38		/* Send WPS EAPOL Frame */

#define SIOCSIMAILBOX	0x8B39		/* Set MailBox Info */

#define SIOCSIWMFP	0x8B3A		/* Set Management Frame Protection Support */

/* Get SNR */
#define SIOCGIWSNR	0x8B3C

#define SIOCSIWGRPID	0x8B3B		/* Set Finite cyclic groups id for SAE  */

/* Get rssiBCN */
#define SIOCGIWBCNSENS	0x8B40 		/* Get beacon average rssi */

#define SIOCIWFIRSTPRIV	0x8BE0
#define SIOCIWLASTPRIV	0x8BFF

#define SIOCSIWPRIVADAPTIVITY	0x8BFB
#define SIOCGIWPRIVPASSPHRASE	0x8BFC
#define SIOCSIWPRIVCOUNTRY		0x8BFD
#define SIOCSIWPRIVAPESSID		0x8BFE
#define SIOCSIWPRIVPASSPHRASE	0x8BFF

/* The first and the last (range) */
#define SIOCIWFIRST	0x8B00
#define SIOCIWLAST	SIOCIWLASTPRIV		/* 0x8BFF */
#define IW_IOCTL_IDX(cmd)	((cmd) - SIOCIWFIRST)

/* Odd : get (world access), even : set (root access) */
#define IW_IS_SET(cmd)	(!((cmd) & 0x1))
#define IW_IS_GET(cmd)	((cmd) & 0x1)

#define IWEVCUSTOM	0x8C02		/* Driver specific ascii string */
#define IWEVREGISTERED	0x8C03		/* Discovered a new node (AP mode) */
#define IWEVEXPIRED	0x8C04		/* Expired a node (AP mode) */
#define IWEVMICHAELMICFAILURE 0x8C06	/* Michael MIC failure
					 * (struct iw_michaelmicfailure)
					 */

#define IWEVMGNTRECV	0x8C10		/* Indicate Mgnt Frame to uplayer */


#define IW_PRIV_TYPE_MASK	0x7000	/* Type of arguments */
#define IW_PRIV_TYPE_NONE	0x0000
#define IW_PRIV_TYPE_BYTE	0x1000	/* Char as number */
#define IW_PRIV_TYPE_CHAR	0x2000	/* Char as character */
#define IW_PRIV_TYPE_INT	0x4000	/* 32 bits int */
#define IW_PRIV_TYPE_FLOAT	0x5000	/* struct iw_freq */
#define IW_PRIV_TYPE_ADDR	0x6000	/* struct sockaddr */

#define IW_PRIV_SIZE_FIXED	0x0800	/* Variable or fixed number of args */

#define IW_PRIV_SIZE_MASK	0x07FF	/* Max number of those args */

#define IW_MAX_FREQUENCIES	32

#define IW_MAX_BITRATES		32

#define IW_ESSID_MAX_SIZE	32

#define IW_MODE_AUTO	0	/* Let the driver decides */
#define IW_MODE_ADHOC	1	/* Single cell network */
#define IW_MODE_INFRA	2	/* Multi cell network, roaming, ... */
#define IW_MODE_MASTER	3	/* Synchronisation master or Access Point */

#define IW_ENCODE_DISABLED	0x8000	/* Encoding disabled */
#define IW_ENCODE_TEMP		0x0400  /* Temporary key */

#define IW_CUSTOM_MAX		256	/* In bytes */

#define IW_AUTH_INDEX		0x0FFF
#define IW_AUTH_FLAGS		0xF000

#define IW_AUTH_WPA_VERSION		0
#define IW_AUTH_CIPHER_PAIRWISE		1
#define IW_AUTH_CIPHER_GROUP		2
#define IW_AUTH_KEY_MGMT		3
#define IW_AUTH_TKIP_COUNTERMEASURES	4
#define IW_AUTH_DROP_UNENCRYPTED	5
#define IW_AUTH_80211_AUTH_ALG		6
#define IW_AUTH_WPA_ENABLED		7
#define IW_AUTH_RX_UNENCRYPTED_EAPOL	8
#define IW_AUTH_ROAMING_CONTROL		9
#define IW_AUTH_PRIVACY_INVOKED		10


/* IW_AUTH_80211_AUTH_ALG values (bit field) */
#define IW_AUTH_ALG_OPEN_SYSTEM	0x00000001
#define IW_AUTH_ALG_SHARED_KEY	0x00000002

/* SIOCSIWENCODEEXT definitions */
#define IW_ENCODE_SEQ_MAX_SIZE	8
/* struct iw_encode_ext ->alg */
#define IW_ENCODE_ALG_NONE	0
#define IW_ENCODE_ALG_WEP	1
#define IW_ENCODE_ALG_TKIP	2
#define IW_ENCODE_ALG_CCMP	3
#define IW_ENCODE_ALG_PMK   4
#define IW_ENCODE_ALG_AES_CMAC  5 //IGTK

/* struct iw_encode_ext ->ext_flags */
#define IW_ENCODE_EXT_TX_SEQ_VALID	0x00000001
#define IW_ENCODE_EXT_RX_SEQ_VALID	0x00000002
#define IW_ENCODE_EXT_GROUP_KEY		0x00000004
#define IW_ENCODE_EXT_SET_TX_KEY	0x00000008

#define IW_MICFAILURE_GROUP	0x00000004
#define IW_MICFAILURE_PAIRWISE	0x00000008
struct sockaddr_t {
	__u8 sa_len;
	__u8 sa_family;
	char sa_data[14];
};

struct	iw_param {
	__s32		value;		/* The value of the parameter itself */
	__u8		fixed;		/* Hardware should not use auto select */
	__u8		disabled;	/* Disable the feature */
	__u16		flags;		/* Various specifc flags (if any) */
};

struct	iw_point {
	void		*pointer;	/* Pointer to the data  (in user space) */
	__u16		length;		/* number of fields or size in bytes */
	__u16		flags;		/* Optional params */
};

struct	iw_freq {
	__s32		m;		/* Mantissa */
	__s16		e;		/* Exponent */
	__u8		i;		/* List index (when in range struct) */
	__u8		flags;		/* Flags (fixed/auto) */
};

struct	iw_quality {
	__u8		qual;		/* link quality (%retries, SNR,
					   %missed beacons or better...) */
	__u8		level;		/* signal level (dBm) */
	__u8		noise;		/* noise level (dBm) */
	__u8		updated;	/* Flags to know if updated */
};

struct	iw_encode_ext {
	__u32		ext_flags; /* IW_ENCODE_EXT_* */
	__u8		tx_seq[IW_ENCODE_SEQ_MAX_SIZE]; /* LSB first */
	__u8		rx_seq[IW_ENCODE_SEQ_MAX_SIZE]; /* LSB first */
	struct sockaddr_t	addr; /* ff:ff:ff:ff:ff:ff for broadcast/multicast
			       * (group) keys or unicast address for
			       * individual keys */
	__u16		alg; /* IW_ENCODE_ALG_* */
	__u16		key_len;
#ifdef __CC_ARM	//Fix Keil compile error, must modify sizeof iw_encode_ext - Alex Fang
	__u8		key[1];
#else
	__u8		key[0];
#endif
};

struct	iw_michaelmicfailure {
	__u32		flags;
	struct sockaddr_t	src_addr;
	__u8		tsc[IW_ENCODE_SEQ_MAX_SIZE]; /* LSB first */
};

union	iwreq_data {
	/* Config - generic */
	char		name[IFNAMSIZ];
	/* Name : used to verify the presence of  wireless extensions.
	 * Name of the protocol/provider... */

	struct iw_point	essid;		/* Extended network name */
	struct iw_param	nwid;		/* network id (or domain - the cell) */
	struct iw_freq	freq;		/* frequency or channel :
					 * 0-1000 = channel
					 * > 1000 = frequency in Hz */

	struct iw_param	sens;		/* signal level threshold */
	struct iw_param bcnsens;        /* signal level threshold */
	struct iw_param	snr;			/* signal noise ratio */
	struct iw_param	bitrate;	/* default bit rate */
	struct iw_param	txpower;	/* default transmit power */
	struct iw_param	rts;		/* RTS threshold threshold */
	struct iw_param	frag;		/* Fragmentation threshold */
	__u32		mode;		/* Operation mode */
	struct iw_param	retry;		/* Retry limits & lifetime */

	struct iw_point	encoding;	/* Encoding stuff : tokens */
	struct iw_param	power;		/* PM duration/timeout */
	struct iw_quality qual;		/* Quality part of statistics */

	struct sockaddr_t	ap_addr;	/* Access point address */
	struct sockaddr_t	addr;		/* Destination address (hw/mac) */

	struct iw_param	param;		/* Other small parameters */
	struct iw_point	data;		/* Other large parameters */
	struct iw_point	passphrase;		/* Extended network name */
};

struct	iwreq {
#if 0
	union {
		char	ifrn_name[IFNAMSIZ];	/* if name, e.g. "eth0" */
	} ifr_ifrn;
#endif
	char	ifr_name[IFNAMSIZ];	/* if name, e.g. "eth0" */

	/* Data part (defined just above) */
	union	iwreq_data	u;
};

struct	iw_priv_args {
	__u32		cmd;		/* Number of the ioctl to issue */
	__u16		set_args;	/* Type and number of args */
	__u16		get_args;	/* Type and number of args */
	char		name[IFNAMSIZ];	/* Name of the extension */
};

#define IW_EXT_STR_FOURWAY_DONE  "WPA/WPA2 handshake done"
#define IW_EXT_STR_RECONNECTION_FAIL  "RECONNECTION FAILURE"
#define IW_EVT_STR_STA_ASSOC	"STA Assoc"
#define IW_EVT_STR_STA_DISASSOC	"STA Disassoc"
#define IW_EVT_STR_SEND_ACTION_DONE	"Send Action Done"
#define IW_EVT_STR_NO_NETWORK "No Assoc Network After Scan Done"
#define IW_EVT_STR_ICV_ERROR "ICV Eror"
#define IW_EVT_STR_CHALLENGE_FAIL "Auth Challenge Fail"
#define IW_EVT_STR_NO_BEACON "No Beacon"
#endif	/* _WIRELESS_H */
