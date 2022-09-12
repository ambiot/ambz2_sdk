COAP CLIENT EXAMPLE

Description:
This example demonstrates how to use libcoap C library to build a CoAP client.

Configuration:
Modify SERVER_HOST, SERVER_PORT in example_coap_client.c based on your CoAP server.
Exclude component\common\network\coap from build in IAR.

[platform_opts.h]
#define CONFIG_EXAMPLE_COAP                 0
#define CONFIG_EXAMPLE_COAP_CLIENT	        1

[lwipopts.h]
#define LWIP_TIMEVAL_PRIVATE            1
#define SO_REUSE                        1
#define MEMP_USE_CUSTOM_POOLS           1
#define LWIP_IPV6                       1
#ifndef xchar
#define xchar(i)             ((i) < 10 ? '0' + (i) : 'A' + (i) - 10)
#endif

Execution:
Can make automatical Wi-Fi connection when booting by using wlan fast connect example.
A CoAP client example thread will be started automatically when booting.

In default, the client will attempt to contact the server at coap://coap.me/hello with port 5683.
The server will return a string with the word "world" to the client.

Note:
Company Firewall may block CoAP message. You can use copper (https://addons.mozilla.org/en-US/firefox/addon/copper-270430/) to test the server's reachability.