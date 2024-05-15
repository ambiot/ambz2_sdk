Websocket Client Example

Description:
A simple websocket client example which send "hello" and "world" to server.
The server will reply the message it received.
Once the client received "world", it will disconnect with server.
In this example only IPv4 is used to test the websocket connection


To build your own echo server, you can refer to https://github.com/jmalloc/echo-server

Configuration:

[platform_opts.h]
	#define CONFIG_EXAMPLE_WEBSOCKET_CLIENT 	1

	If using the Websocket server with TLS connection:

	[wsclient_api.h]
		#define USING_SSL

	[config_rsa.h]
		If connecting to websocket-echo.com, ensure the following configurations are enabled:
			#define MBEDTLS_GCM_C
			#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
			#define MBEDTLS_ECDH_C
			#define MBEDTLS_ECP_C

	For Matter SDK
	[component/common/application/matter/common/mbedtls/mbedtls_config.h]
		If connecting to websocket-echo.com, ensure the following configurations are enabled:
			#define MBEDTLS_GCM_C
			#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
			#define MBEDTLS_SSL_MAX_CONTENT_LEN 16384

	[example_wsclient.c]
		wsclient_context *wsclient = create_wsclient("wss://websocket-echo.com", 0, NULL, NULL, 1500);

		Note: Please take note the link above may expired anytime. Alternatively, please test with websocket_server example.
		wsclient_context *wsclient = create_wsclient("wss://server ip address", 0, NULL, NULL, 1500);

	If using the Websocket server without TLS connection:

	[example_wsclient.c]
		wsclient_context *wsclient = create_wsclient("ws://websocket-echo.com", 0, NULL, NULL, 1500);

		Note: Please take note the link above may expired anytime. Alternatively, please test with websocket_server example.
		wsclient_context *wsclient = create_wsclient("ws://server ip address", 0, NULL, NULL, 1500);

Execution:
Can make automatical Wi-Fi connection when booting by using wlan fast connect example.
A websocket client example thread will be started automatically when booting.
If using other websocekt server, modify the create_wsclient() API and the handle_message() function depending on the condition of the server.

Note :
AmebaPro and AmebaZ2 don't support polarssl, only support mbedtls.

[Supported List]
	Supported :
		Ameba-1, Ameba-z, Ameba-pro, Ameba-z2