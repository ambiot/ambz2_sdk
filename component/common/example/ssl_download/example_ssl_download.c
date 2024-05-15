#include <platform_opts.h>

#if defined(CONFIG_EXAMPLE_SSL_DOWNLOAD) && (CONFIG_EXAMPLE_SSL_DOWNLOAD == 1)

#include <FreeRTOS.h>
#include <task.h>
#include <platform/platform_stdlib.h>

#include "platform_opts.h"

#define STACKSIZE     2048
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
#include "device_lock.h"
#endif

#if CONFIG_USE_POLARSSL

#include <lwip/sockets.h>
#include <polarssl/config.h>
#include <polarssl/memory.h>
#include <polarssl/ssl.h>

#define SERVER_HOST    "176.34.62.248"
#define SERVER_PORT    443
#define RESOURCE       "/repository/IOT/Project_Cloud_A.bin"
#define BUFFER_SIZE    2048

static int my_random(void *p_rng, unsigned char *output, size_t output_len)
{
	rtw_get_random_bytes(output, output_len);
	return 0;
}

static void example_ssl_download_thread(void *param)
{
	int server_fd = -1, ret;
	struct sockaddr_in server_addr;
	ssl_context ssl;

	// Delay to wait for IP by DHCP
	vTaskDelay(10000);
	printf("\nExample: SSL download\n");

	memory_set_own(pvPortMalloc, vPortFree);
	memset(&ssl, 0, sizeof(ssl_context));

	if((ret = net_connect(&server_fd, SERVER_HOST, SERVER_PORT)) != 0) {
		printf("ERROR: net_connect ret(%d)\n", ret);
		goto exit;
	}

	if((ret = ssl_init(&ssl)) != 0) {
		printf("ERRPR: ssl_init ret(%d)\n", ret);
		goto exit;
	}

	ssl_set_endpoint(&ssl, SSL_IS_CLIENT);
	ssl_set_authmode(&ssl, SSL_VERIFY_NONE);
	ssl_set_rng(&ssl, my_random, NULL);
	ssl_set_bio(&ssl, net_recv, &server_fd, net_send, &server_fd);

	if((ret = ssl_handshake(&ssl)) != 0) {
		printf("ERROR: ssl_handshake ret(-0x%x)", -ret);
		goto exit;
	}
	else {
		unsigned char buf[BUFFER_SIZE + 1];
		int pos = 0, read_size = 0, resource_size = 0, content_len = 0, header_removed = 0;

		printf("SSL ciphersuite %s\n", ssl_get_ciphersuite(&ssl));
		sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", RESOURCE, SERVER_HOST);
		ssl_write(&ssl, buf, strlen(buf));

		while((read_size = ssl_read(&ssl, buf + pos, BUFFER_SIZE - pos)) > 0) {
			if(header_removed == 0) {
				char *header = NULL;

				pos += read_size;
				buf[pos] = 0;
				header = strstr(buf, "\r\n\r\n");

				if(header) {
					char *body, *content_len_pos;

					body = header + strlen("\r\n\r\n");
					*(body - 2) = 0;
					header_removed = 1;
					printf("\nHTTP Header: %s\n", buf);

					// Remove header size to get first read size of data from body head
					read_size = pos - ((unsigned char *) body - buf);
					pos = 0;

					content_len_pos = strstr(buf, "Content-Length: ");
					if(content_len_pos) {
						content_len_pos += strlen("Content-Length: ");
						*(char*)(strstr(content_len_pos, "\r\n")) = 0;
						content_len = atoi(content_len_pos);
					}
				}
				else {
					if(pos >= BUFFER_SIZE){
						printf("ERROR: HTTP header\n");
						goto exit;
					}

					continue;
				}
			}

			printf("read resource %d bytes\n", read_size);
			resource_size += read_size;
		}

		printf("exit read. ret = %d\n", read_size);
		printf("http content-length = %d bytes, download resource size = %d bytes\n", content_len, resource_size);
	}

exit:
	if(server_fd >= 0)
		net_close(server_fd);

	ssl_free(&ssl);
	vTaskDelete(NULL);
}

void example_ssl_download(void)
{
	if(xTaskCreate(example_ssl_download_thread, ((const char*)"example_ssl_download_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
}

#elif CONFIG_USE_MBEDTLS /* CONFIG_USE_POLARSSL */

#include <mbedtls/config.h>
#include <mbedtls/platform.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>

//#define SSL_CLIENT_EXT

#ifdef SSL_CLIENT_EXT
extern int ssl_client_ext_init(void);
extern int ssl_client_ext_setup(mbedtls_ssl_config *conf);
extern void ssl_client_ext_free(void);
#endif

#define SERVER_HOST    "176.34.62.248"
#define SERVER_PORT    "443"
#define RESOURCE       "/repository/IOT/Project_Cloud_A.bin"
#define BUFFER_SIZE    2048

static int my_random(void *p_rng, unsigned char *output, size_t output_len)
{
	rtw_get_random_bytes(output, output_len);
	return 0;
}

static void* my_calloc(size_t nelements, size_t elementSize)
{
	size_t size;
	void *ptr = NULL;

	size = nelements * elementSize;
	ptr = pvPortMalloc(size);

	if(ptr)
		memset(ptr, 0, size);

	return ptr;
}

static void example_ssl_download_thread(void *param)
{
	int ret;
	mbedtls_net_context server_fd;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
	extern void rtw_create_secure_context(u32 secure_stack_size);
	rtw_create_secure_context(STACKSIZE*2);
	extern int NS_ENTRY secure_mbedtls_platform_set_calloc_free(void);
	secure_mbedtls_platform_set_calloc_free();
	extern void NS_ENTRY secure_set_ns_device_lock(void (*device_mutex_lock_func)(uint32_t), void (*device_mutex_unlock_func)(uint32_t));
	secure_set_ns_device_lock(device_mutex_lock, device_mutex_unlock);
#endif

	// Delay to wait for IP by DHCP
	vTaskDelay(10000);
	printf("\nExample: SSL download\n");

	mbedtls_platform_set_calloc_free(my_calloc, vPortFree);

	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);

#ifdef SSL_CLIENT_EXT
	if((ret = ssl_client_ext_init()) != 0) {
		printf(" failed\n\r  ! ssl_client_ext_init returned %d\n", ret);
		goto exit;
	}
#endif

	if((ret = mbedtls_net_connect(&server_fd, SERVER_HOST, SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0) {
		printf("ERROR: mbedtls_net_connect ret(%d)\n", ret);
		goto exit;
	}

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	if((ret = mbedtls_ssl_config_defaults(&conf,
			MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {

		printf("ERROR: mbedtls_ssl_config_defaults ret(%d)\n", ret);
		goto exit;
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
	mbedtls_ssl_conf_rng(&conf, my_random, NULL);

#ifdef SSL_CLIENT_EXT
	if((ret = ssl_client_ext_setup(&conf)) != 0) {
		printf(" failed\n\r  ! ssl_client_ext_setup returned %d\n", ret);
		goto exit;
	}
#endif

#if (defined(MBEDTLS_SSL_IN_CONTENT_LEN) && (MBEDTLS_SSL_IN_CONTENT_LEN  == 4096)) ||  \
	(defined(MBEDTLS_SSL_MAX_CONTENT_LEN) && (MBEDTLS_SSL_MAX_CONTENT_LEN  == 4096))
	if(ret = mbedtls_ssl_conf_max_frag_len(&conf, MBEDTLS_SSL_MAX_FRAG_LEN_4096) < 0) {
			printf("ERROR: mbedtls_ssl_conf_max_frag_len ret(%d)\n", ret);
			goto exit;
	}
#endif

	if((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
		printf("ERRPR: mbedtls_ssl_setup ret(%d)\n", ret);
		goto exit;
	}

	if((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
		printf("ERROR: mbedtls_ssl_handshake ret(-0x%x)", -ret);
		goto exit;
	}
	else {
		unsigned char buf[BUFFER_SIZE + 1];
		int pos = 0, read_size = 0, resource_size = 0, content_len = 0, header_removed = 0;

		printf("SSL ciphersuite %s\n", mbedtls_ssl_get_ciphersuite(&ssl));
		sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", RESOURCE, SERVER_HOST);
		mbedtls_ssl_write(&ssl, buf, strlen(buf));

		while((read_size = mbedtls_ssl_read(&ssl, buf + pos, BUFFER_SIZE - pos)) > 0) {
			if(header_removed == 0) {
				char *header = NULL;

				pos += read_size;
				buf[pos] = 0;
				header = strstr(buf, "\r\n\r\n");

				if(header) {
					char *body, *content_len_pos;

					body = header + strlen("\r\n\r\n");
					*(body - 2) = 0;
					header_removed = 1;
					printf("\nHTTP Header: %s\n", buf);

					// Remove header size to get first read size of data from body head
					read_size = pos - ((unsigned char *) body - buf);
					pos = 0;

					content_len_pos = strstr(buf, "Content-Length: ");
					if(content_len_pos) {
						content_len_pos += strlen("Content-Length: ");
						*(strstr(content_len_pos, "\r\n")) = 0;
						content_len = atoi(content_len_pos);
					}
				}
				else {
					if(pos >= BUFFER_SIZE){
						printf("ERROR: HTTP header\n");
						goto exit;
					}

					continue;
				}
			}

			printf("read resource %d bytes\n", read_size);
			resource_size += read_size;
		}

		printf("exit read. ret = %d\n", read_size);
		printf("http content-length = %d bytes, download resource size = %d bytes\n", content_len, resource_size);
	}

exit:
	mbedtls_net_free(&server_fd);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);

#ifdef SSL_CLIENT_EXT
	ssl_client_ext_free();
#endif

	vTaskDelete(NULL);
}

void example_ssl_download(void)
{
	if(xTaskCreate(example_ssl_download_thread, ((const char*)"example_ssl_download_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
}

#endif /* CONFIG_USE_POLARSSL */

#endif /*CONFIG_EXAMPLE_SSL_DOWNLOAD*/