##################################################################################
#                                                                                #
#                             Example MQTT                                       #
#                                                                                #
##################################################################################

Date: 2018-05-29

Table of Contents
~~~~~~~~~~~~~~~~~
 - Description
 - Setup Guide
 - Result description
 - Other Reference
 - Supported List

 
Description
~~~~~~~~~~~
		This is an example to use the MQTT APIs in example_mqtt.c. 
		In this example only IPv4 is used for mqtt connection.

Setup Guide
~~~~~~~~~~~
	To execute this example automatically when booting, configuration should be set as below.
	1) If test mqtt mutual authentication, please enable //#define MQTT_MUTUAL_AUTHENTICATION in example_mqtt.c
	2) The CONFIG_EXAMPLE_MQTT in platform_opts.h must be enabled as follows.
		/* platform_opts.h *./
		#define CONFIG_EXAMPLE_MQTT 1
		For MQTT Version 5.0 Example uncomment #define MQTTV5

		To manage connection exception, LWIP_TCP_KEEPALIVE and LWIP_UART_ADAPTER in 
		lwipopts.h must be enabled as follows.
		/* lwipopts.h */
		#define LWIP_TCP_KEEPALIVE 1
		#define LWIP_UART_ADAPTER 1

Parameter Setting and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		Modify the address in example_mqtt.c based on your MQTT broker's ip address or domain name.
		Likewise modify the subscribe and publish topic to your desired topic.
			char* address = "broker.emqx.io";
			char* sub_topic = "LASS/Test/Pm25Ameba/#";
			char* pub_topic = "LASS/Test/Pm25Ameba/FT1_018";

		Change the port in example_mqtt.c based on your MQTT broker's setting
			network.my_port = 1883;

Result description
~~~~~~~~~~~~~~~~~~
	In the example, MQTT task is created to present MQTT connection and message processing, including:
		1) Check WIFI status and wait WIFI to be connected. MQTT will start after device connected 
		   with AP and got IP.
		2) Establish TCP/IP connection with MQTT server.
		3) Send a CONNECT message to server and wait for a CONNACK message from server.
		4) Subscribe to a topic, sending SUBSCRIBE to server and wait for SUBACK from server.
		5) Publish message to server every 5 seconds.
		6) Read and response message. Keep alive with server.
		7) If mqtt status is set to MQTT_START, the client will close the TCP/IP socket connection, and
		   restart the session by opening a new socket to the server and issuing a CONNECT message.
		   The client will subscribe to the topic again.
		Some strategies are used to manage connection exception
		1) Lwip_select is used to check data arrival and connection exception. Message is read only if
		   tcp data has arrived. If exception fd is set, MQTT will restart.
		2) SO_KEEPALIVE and TCP_KEEPIDLE are set to clear TCP buffer when network is bad. For if
		   TCP buffer is full and can¡¦t allocate more memory, the situation will last for about 20
		   minutes until MAX data retries reached, and then MQTT will not restart successfully during
		   this time for allocating memory failed.

Other Reference
~~~~~~~~~~~~~~~
		For more details, please refer to UM0060 Realtek Ameba-all MQTT User Guide.pdf

Supported List
~~~~~~~~~~~~~~
[Supported List]
		Supported :
			Ameba-1, Ameba-z, Ameba-pro, Ameba-z2