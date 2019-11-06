#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <lwip/netdb.h>
#include <platform/platform_stdlib.h>
#include <lwip/ip.h>
#include <lwip_netconf.h>

#define PING_IP		"192.168.159.1"
#define PING_TO		1000
#define PING_ID		0xABCD
#define PING_ID_6   0x0100
#define BUF_SIZE	10000
#define STACKSIZE	1024


static unsigned short ping_seq = 0;
static int infinite_loop, ping_count, data_size, ping_interval, ping_call;


static int ping_total_time = 0, ping_received_count = 0;

static void generate_ping_echo(unsigned char *buf, int size)
{
	int i;
	struct icmp_echo_hdr *pecho;

	for(i = 0; i < size; i ++) {
		buf[sizeof(struct icmp_echo_hdr) + i] = (unsigned char) i;
	}

	pecho = (struct icmp_echo_hdr *) buf;
	ICMPH_TYPE_SET(pecho, ICMP_ECHO);
	ICMPH_CODE_SET(pecho, 0);
	pecho->chksum = 0;
	pecho->id = PING_ID;
	pecho->seqno = htons(++ ping_seq);

	//Checksum includes icmp header and data. Need to calculate after fill up icmp header
	pecho->chksum = inet_chksum(pecho, sizeof(struct icmp_echo_hdr) + size);
}
#if LWIP_IPV6
static void generate_ping_echo_6(unsigned char *buf, int size)
{
	int i;
	struct icmp_echo_hdr *pecho;

	for(i = 0; i < size; i ++) {
		buf[sizeof(struct icmp_echo_hdr) + i] = (unsigned char) i;
	}

	pecho = (struct icmp_echo_hdr *) buf;
	ICMPH_TYPE_SET(pecho, ICMP6_TYPE_EREQ);
	ICMPH_CODE_SET(pecho, 0);
	pecho->chksum = 0;
	pecho->id = PING_ID_6;
	pecho->seqno = htons(++ ping_seq);

	//Checksum includes icmp header and data. Need to calculate after fill up icmp header
	//pecho->chksum = inet_chksum(pecho, sizeof(struct icmp_echo_hdr) + size);
}
#endif
extern struct netif xnetif[];
void ping_test(void *param)
{
	int i, ping_socket;
	int pint_timeout = PING_TO;
	struct sockaddr_in to_addr, from_addr;
	int from_addr_len = sizeof(struct sockaddr);
	int ping_size, reply_size, ret_size;
	unsigned char *ping_buf, *reply_buf;
	unsigned int ping_time, reply_time;
	struct icmp_echo_hdr *pecho;
	unsigned int min_time = 1000, max_time = 0;
	struct hostent *server_host;
	char *host = param;
#if LWIP_IPV6
	int from_addr6_len = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 to_addr6, from_addr6;
	int ping_addr_is_ipv6;
	int ipv6_addr_equal = 0;
	LwIP_AUTOIP_IPv6(&xnetif[0]);
	//Wait for ipv6 addr process conflict done
	while(!ip6_addr_isvalid(netif_ip6_addr_state(&xnetif[0],0)));
#endif

	ping_total_time = 0;
	ping_received_count = 0;
	
	if(data_size > BUF_SIZE){
		printf("\n\r[ERROR] %s: data size error, can't exceed %d",__func__,BUF_SIZE);
		return;
	}

	//Ping size = icmp header(8 bytes) + data size
	ping_size = sizeof(struct icmp_echo_hdr) + data_size;

	ping_buf = pvPortMalloc(ping_size);
	if(NULL == ping_buf){
		printf("\n\r[ERROR] %s: Allocate ping_buf failed",__func__);
		return;
	}
	printf("\n\r[%s] PING %s %d(%d) bytes of data\n", __FUNCTION__, host, data_size, sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr) + data_size);			

#if LWIP_IPV6
	//check whether the host addr is ipv4 or ipv6 addr
	if(inet_pton(AF_INET, host, &to_addr.sin_addr)){
		ping_addr_is_ipv6 = 0;
#endif
		reply_size = ping_size + IP_HLEN;
#if LWIP_IPV6
	}else if(inet_pton(AF_INET6, host, &to_addr6.sin6_addr)){
		ping_addr_is_ipv6 = 1;
		reply_size = ping_size + IP6_HLEN;
	}	
#endif

	reply_buf = pvPortMalloc(reply_size);
	if(NULL == reply_buf){
		vPortFree(ping_buf);
		printf("\n\r[ERROR] %s: Allocate reply_buf failed",__func__);
		return;
	}

	for(i = 0; (i < ping_count) || (infinite_loop == 1); i ++) {
#if LWIP_IPV6
		if(ping_addr_is_ipv6)
			ping_socket = socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
		else
#endif
		ping_socket = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
		if(ping_socket < 0){
			printf("create socket failed\r\n");
		}
#if defined(LWIP_SO_SNDRCVTIMEO_NONSTANDARD) && (LWIP_SO_SNDRCVTIMEO_NONSTANDARD == 0)	// lwip 1.5.0
		struct timeval timeout;
		timeout.tv_sec = pint_timeout / 1000;
		timeout.tv_usec = pint_timeout % 1000 * 1000;
		setsockopt(ping_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#else	// lwip 1.4.1
		setsockopt(ping_socket, SOL_SOCKET, SO_RCVTIMEO, &pint_timeout, sizeof(pint_timeout));
#endif

#if LWIP_IPV6
		if(ping_addr_is_ipv6){
			to_addr6.sin6_len = sizeof(to_addr6);
			to_addr6.sin6_family = AF_INET6;
		}
		else
#endif
		{
			to_addr.sin_len = sizeof(to_addr);
			to_addr.sin_family = AF_INET;
		}
		if ((inet_aton(host, &to_addr.sin_addr) == 0)
#if LWIP_IPV6
		 && (inet6_aton(host, &to_addr6.sin6_addr) == 0)
#endif
		)
		{
			server_host = gethostbyname(host);
			if(server_host == NULL){
				printf("\n\r[%s] Get host name failed in the %d ping test\n", __FUNCTION__, (i + 1));
				close(ping_socket);
				vTaskDelay(ping_interval * configTICK_RATE_HZ);
				continue;
			}
			memcpy((void *) &to_addr.sin_addr, (void *) server_host->h_addr, 4);
		}
#if LWIP_IPV6
		else if(ping_addr_is_ipv6){	
			inet6_aton(host, &to_addr6.sin6_addr);
		}
#endif
		else {
			to_addr.sin_addr.s_addr = inet_addr(host);
		}

#if LWIP_IPV6
		if(ping_addr_is_ipv6){
			generate_ping_echo_6(ping_buf, data_size);
			sendto(ping_socket, ping_buf, ping_size, 0, (struct sockaddr *) &to_addr6, sizeof(to_addr6));
		}
		else
#endif
		{
			generate_ping_echo(ping_buf, data_size);
			sendto(ping_socket, ping_buf, ping_size, 0, (struct sockaddr *) &to_addr, sizeof(to_addr));
		}
		
		ping_time = xTaskGetTickCount();
#if LWIP_IPV6
	if(ping_addr_is_ipv6){
		ret_size = recvfrom(ping_socket, reply_buf, reply_size, 0, (struct sockaddr *) &from_addr6, (socklen_t *) &from_addr6_len);
		if(!memcmp((void *)&from_addr6.sin6_addr, (void *)&to_addr6.sin6_addr, sizeof(from_addr6.sin6_addr)))
			ipv6_addr_equal = TRUE;
	}
	else
#endif
		ret_size = recvfrom(ping_socket, reply_buf, reply_size, 0, (struct sockaddr *) &from_addr, (socklen_t *) &from_addr_len);

		if(ret_size >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
			reply_time = xTaskGetTickCount();
#if LWIP_IPV6
			if(ping_addr_is_ipv6)
				pecho = (struct icmp_echo_hdr *)(reply_buf + IP6_HLEN);
			else
#endif
				pecho = (struct icmp_echo_hdr *)(reply_buf + IP_HLEN);
#if LWIP_IPV6
			if(ping_addr_is_ipv6 && ipv6_addr_equal){
				if((pecho->id == PING_ID_6) && (pecho->seqno == htons(ping_seq))) {
					printf("\n\r[%s] %d bytes from %s: icmp_seq=%d time=%d ms", __FUNCTION__, data_size, inet6_ntoa(from_addr6.sin6_addr), htons(pecho->seqno), (reply_time - ping_time) * portTICK_RATE_MS);
					ping_received_count++;
					ping_total_time += (reply_time - ping_time) * portTICK_RATE_MS;
					if((reply_time - ping_time) > max_time) max_time = (reply_time - ping_time);
					if((reply_time - ping_time) < min_time) min_time = (reply_time - ping_time);
				}
			}
			else
#endif 
				 if(from_addr.sin_addr.s_addr == to_addr.sin_addr.s_addr){
				if((pecho->id == PING_ID) && (pecho->seqno == htons(ping_seq))) {
					printf("\n\r[%s] %d bytes from %s: icmp_seq=%d time=%d ms", __FUNCTION__, data_size, inet_ntoa(from_addr.sin_addr), htons(pecho->seqno), (reply_time - ping_time) * portTICK_RATE_MS);
					ping_received_count++;
					ping_total_time += (reply_time - ping_time) * portTICK_RATE_MS;
					if((reply_time - ping_time) > max_time) max_time = (reply_time - ping_time);
					if((reply_time - ping_time) < min_time) min_time = (reply_time - ping_time);
				}	
			}
			else
				printf("\n\r[%s] Request timeout for icmp_seq %d\n", __FUNCTION__, ping_seq);
		}
		else
			printf("\n\r[%s] Request timeout for icmp_seq %d\n", __FUNCTION__, ping_seq);

		close(ping_socket);
		vTaskDelay(ping_interval * configTICK_RATE_HZ);
	}
	if(ping_count == 0){
		printf("\n\rNumber of echo requests to send cannot be zero\n\r");
	}
	else{
		printf("\n\r[%s] %d packets transmitted, %d received, %d%% packet loss, average %d ms", __FUNCTION__, ping_count, ping_received_count, (ping_count-ping_received_count)*100/ping_count, ping_received_count ? ping_total_time/ping_received_count : 0);
		printf("\n\r[%s] min: %d ms, max: %d ms\n\r", __FUNCTION__, min_time, max_time);
	}
	vPortFree(ping_buf);
	vPortFree(reply_buf);
	vPortFree(host);

	if(!ping_call)
		vTaskDelete(NULL);
}

void do_ping_call(char *ip, int loop, int count)
{
	ping_call = 1;
	ping_seq = 0;
	data_size = 120;
	ping_interval = 1;
	infinite_loop = loop;
	ping_count = count;
	char * host;
	host = pvPortMalloc(strlen(ip) + 1);
	memset(host, 0, (strlen(ip) + 1));
	memcpy(host, ip, strlen(ip));
	ping_test(host);
}

int get_ping_report(int *ping_lost){
	*ping_lost = ping_count - ping_received_count;
	return 0;
}

void cmd_ping(int argc, char **argv)
{
    int argv_count = 2;
	char * host;

    if(argc < 2)
        goto Exit;

    //ping cmd default value
    infinite_loop = 0;
    ping_count    = 4;
    data_size     = 32;
    ping_interval = 1;
    ping_call     = 1;
    ping_seq      = 0;

    while(argv_count<=argc){
        //first operation
        if(argv_count == 2){
            host = pvPortMalloc(strlen(argv[argv_count-1]) + 1);
            memset(host, 0, (strlen(argv[argv_count-1]) + 1));
            strncpy(host, argv[argv_count-1], strlen(argv[argv_count-1]));
            argv_count++;
        }
        else{
            if(strcmp(argv[argv_count-1], "-t") == 0){
                infinite_loop = 1;
                argv_count++;
            }
            else if(strcmp(argv[argv_count-1], "-n") == 0){
                if(argc < (argv_count+1))
                    goto Exit;
                ping_count = (int) atoi(argv[argv_count]);
                argv_count+=2;
            }
            else if(strcmp(argv[argv_count-1], "-l") == 0){
                if(argc < (argv_count+1))
                    goto Exit;
                data_size = (int) atoi(argv[argv_count]);
                argv_count+=2;
            }
            else{
                goto Exit;
            }
        }
    }

    ping_test(host);

    return;

Exit:
    printf("\n\r[ATWI] Usage: ATWI=[host],[options]\n");
    printf("\n\r     -t        Ping the specified host until stopped\n");
    printf("  \r     -n    #   Number of echo requests to send (default 4 times)\n");
    printf("  \r     -l    #   Send buffer size (default 32 bytes)\n");
    printf("\n\r   Example:\n");
    printf("  \r     ATWI=192.168.1.2,-n,100,-l,5000\n");
    return;
}

void do_ping_test(char *ip, int size, int count, int interval)
{
	char *host;
	if((sizeof(struct icmp_echo_hdr) + size) > BUF_SIZE) {
		printf("\n\r%s BUF_SIZE(%d) is too small", __FUNCTION__, BUF_SIZE);
		return;
	}

	if(ip == NULL){
		host = pvPortMalloc(strlen(PING_IP) + 1);
		memset(host, 0, (strlen(PING_IP) + 1));
		strcpy(host, PING_IP);
	}
	else{
		host = pvPortMalloc(strlen(ip) + 1);
		memset(host, 0, (strlen(ip) + 1));
		strcpy(host, ip);
	}

	ping_call = 0;
	ping_seq = 0;
	data_size = size;
	ping_interval = interval;

	if(count == 0) {
		infinite_loop = 1;
		ping_count = 0;
	}
	else {
		infinite_loop = 0;
		ping_count = count;
	}

	if(xTaskCreate(ping_test, (char const*)((const signed char*)"ping_test"), STACKSIZE, host, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}
