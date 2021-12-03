
/*
* Broadcast thread for AmebaCam app
*/
#include "platform_os.h"
#include "rtsp/rtsp_api.h"
#include "bt_mesh_broadcast_demo.h"
#include "sockets.h"
#include "lwip_netconf.h" //for LwIP_GetIP, LwIP_GetMAC
#include "netif.h"
#include "platform_os.h"
#include "os_task.h"
#if defined(CONFIG_PLATFORM_8195A)
#include "mmf_sink.h"
#endif

extern struct netif xnetif[NET_IF_NUM];
//extern u8 AmebaCam_device_name[256];
char AmebaCam_device_name_demo[256] = {0};


#if defined(CONFIG_PLATFORM_8195A)
	#if ENABLE_PROXY_SEVER
		msink_context	*rtsp_sink;
	#endif
#endif


void media_amebacam_broadcast_all_demo(void);
void *media_amebacam_broadcast_task = NULL;

static void amebacam_broadcast_thread_all_demo(void *param)
{
    /* avoid gcc compile warning */
    (void)param;
	int socket = -1;
	int broadcast = 1;
	struct sockaddr_in bindAddr;
	uint16_t port = 49154;
	unsigned char packet[32];
	int broadcast_retry_count = 5;
	int i = 0;
	
	printf("amebacam_broadcast_thread_all_demo\r\n");
	// Create socket
	if((socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: socket failed\n");
		goto exit;
	}
	
	// Set broadcast socket option
	if(setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0){
		printf("ERROR: setsockopt failed\n");
		goto exit;
	}
	
	// Set the bind address
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.s_addr = INADDR_ANY;
	if(bind(socket, (struct sockaddr *) &bindAddr, sizeof(bindAddr)) < 0){
		printf("ERROR: bind failed\n");
		goto exit;
	}
	
	
	while(1) {
		int sendLen;
		struct sockaddr to;
		struct sockaddr_in *to_sin = (struct sockaddr_in*) &to;
		to_sin->sin_family = AF_INET;
		to_sin->sin_port = htons(49153);
		to_sin->sin_addr.s_addr = INADDR_BROADCAST;
		sprintf((char*)packet, "wake_up");
		for(i=0;i<broadcast_retry_count;i++){
			if((sendLen = sendto(socket, packet, strlen((char *)(&packet[0])), 0, &to, sizeof(struct sockaddr))) < 0)
				printf("ERROR: sendto broadcast\n");

			os_delay(2);
		}
		goto exit;
	}

exit:
	printf("broadcast example finish\n");
	close(socket);
	os_task_delete(NULL);
	return;
}

void amebacam_broadcast_demo_thread(void *param)
{
    /* avoid gcc compile warning */
    (void)param;
#if defined(CONFIG_PLATFORM_8195A)
	#if ENABLE_PROXY_SEVER
		printf("AmebaCam wait sink\n\r");
		while (rtsp_sink==NULL) {
			os_delay(1000);
		}
	#endif
#endif
#if defined(CONFIG_PLATFORM_8721D) || defined(CONFIG_PLATFORM_8710C)
	AmebaCam_device_name_demo[0] = 'A';
#endif

	while (AmebaCam_device_name_demo[0]=='\0') {
		os_delay(2);
	}
	
	media_amebacam_broadcast_all_demo();
	printf("\n\rAmebaCam Broadcast\r\n");
	
	int socket = -1;
	int broadcast = 1;
	struct sockaddr_in bindAddr;
	uint16_t port = 49152;//server
	uint16_t port2 = 49151;//client
	unsigned char packet[32];
	uint8_t *mac;
	static unsigned char broadcast_to_app[300];
#if defined(CONFIG_PLATFORM_8195A)
	#if ENABLE_PROXY_SEVER	
		struct rtsp_context * rtsp_ctx = rtsp_sink->drv_priv;
		u8 proxy_suffix[256];
		memset(proxy_suffix, 0, 256);
		sprintf(proxy_suffix, "%s_%d",AmebaCam_device_name_demo,rtsp_ctx->id);
	#endif
#endif
	mac = (uint8_t*) LwIP_GetMAC(&xnetif[0]);
	
	// Create socket
	if((socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("ERROR: Broadcast thread of AmebaCam failed socket failed\n\r");
		goto err;
	}
	
	// Set broadcast socket option
	if(setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0){
		printf("ERROR: setsockopt failed\n");
		goto err;
	}
	
	// Set the bind address
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.s_addr = INADDR_ANY;
	if(bind(socket, (struct sockaddr *) &bindAddr, sizeof(bindAddr)) < 0){
		printf("ERROR: bind failed\n");
		goto err;
	}
	
	
	while(1) {
		// Receive broadcast
		int packetLen = 0;
		struct sockaddr from;
		struct sockaddr_in *from_sin = (struct sockaddr_in*) &from;
		u32_t fromLen = sizeof(from);
		
		if((packetLen = recvfrom(socket, packet, sizeof(packet), 0, &from, &fromLen)) >= 0) {
			uint8_t *ip = (uint8_t *) &from_sin->sin_addr.s_addr;
			//uint16_t from_port = ntohs(from_sin->sin_port);
			printf("Broadcast from AmebaCam App (%d.%d.%d.%d)\n\r",ip[0], ip[1], ip[2], ip[3]);
		}
		
		// Send broadcast
		if(packetLen > 0) {
			int sendLen;
			struct sockaddr to;
			struct sockaddr_in *to_sin = (struct sockaddr_in*) &to;
			to_sin->sin_family = AF_INET;
			to_sin->sin_port = htons(port2);
			to_sin->sin_addr.s_addr = from_sin->sin_addr.s_addr;//INADDR_ANY;
#if defined(CONFIG_PLATFORM_8195A)
	#if ENABLE_PROXY_SEVER
			sprintf((char*)broadcast_to_app, "%02x%02x%02x%02x%02x%02x;%d;%s;", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
			rtsp_ctx->proxy_port,proxy_suffix);
	#else
			sprintf((char*)broadcast_to_app, "%02x%02x%02x%02x%02x%02x;0;%s;", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
			AmebaCam_device_name_demo);
	#endif
#else
			sprintf((char*)broadcast_to_app, "%02x%02x%02x%02x%02x%02x;0;%s;", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
			AmebaCam_device_name_demo);	
#endif
			sendLen = sendto(socket, broadcast_to_app, strlen((const char*)broadcast_to_app), 0, &to, sizeof(struct sockaddr));
			if( sendLen < 0)
				printf("ERROR: Send Broadcast to AmebaCam App Fail\n\r");
			else
				printf("Broadcast to AmebaCam App: %s\n\r",broadcast_to_app);
				//printf("sendto - %d bytes to broadcast:%d, data: %s\n", sendLen, port2,broadcast_to_app);
		}
	}
	
err:
	printf("ERROR: Broadcast thread of AmebaCam failed\n");
	close(socket);
	
	os_task_delete(NULL);
}

void media_amebacam_broadcast_all_demo(void)
{
	/*user can start their own task here*/
	if (os_task_create(&media_amebacam_broadcast_task, "amebacam_broadcast_all", amebacam_broadcast_thread_all_demo,
                    NULL, 2048, 1) != true) {
		printf("\r\n media_amebacam_broadcast_all: Create Task Error\n");
	}
}

