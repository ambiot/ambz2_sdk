/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "MQTTClient.h"
#include "wifi_conf.h"

#if defined(MQTTV5)
#include "MQTTProperties.h"
#include "MQTTV5Packet.h"
#include "MQTTV5Connect.h"
#endif

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
#include "device_lock.h"
#define STACKSIZE 2048
#endif

#define MQTT_SELECT_TIMEOUT 1
static void messageArrived(MessageData* data)
{
	mqtt_printf(MQTT_INFO, "Message arrived on topic %s: %s\n", data->topicName->lenstring.data, (char *)data->message->payload);
}

//This example is original and cannot restart if failed. To use this example, define WAIT_FOR_ACK and not define MQTT_TASK in MQTTClient.h
void prvMQTTEchoTask(void *pvParameters)
{
	/* To avoid gcc warnings */
	( void ) pvParameters;
	
	/* connect to gpssensor.ddns.net, subscribe to a topic, send and receive messages regularly every 5 sec */
	MQTTClient client;
	Network network;
	unsigned char sendbuf[512], readbuf[80];
	int rc = 0, count = 0;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
	
#if defined(MQTTV5)
	
	//init connect properties and will properties
	MQTTProperties connectProperties = MQTTProperties_initializer,
	willProperties = MQTTProperties_initializer,
	subProperties = MQTTProperties_initializer,
	pubProperties = MQTTProperties_initializer;
	MQTTProperty connect_props[10], will_props[10], sub_props[10], pub_props[20];
	
	connectProperties.max_count = 6;
	connectProperties.array = connect_props;

	willProperties.max_count = 10;
	willProperties.array = will_props;
	
	subProperties.max_count = 10;
	subProperties.array = sub_props;
	
	pubProperties.max_count = 10;
	pubProperties.array = pub_props;
	
	MQTTProperty pub_format;
	pub_format.identifier = PAYLOAD_FORMAT_INDICATOR;
	pub_format.value.byte = 1;
	rc = MQTTProperties_add(&pubProperties, &pub_format);
	
	MQTTProperty connect;
	connect.identifier = SESSION_EXPIRY_INTERVAL;
	connect.value.integer4 = 10;
	rc = MQTTProperties_add(&connectProperties, &connect);
	
	MQTTProperty will;
	will.identifier = WILL_DELAY_INTERVAL;
	will.value.integer4 = 1;
	rc = MQTTProperties_add(&willProperties, &will);
	
	MQTTProperty will_format;
	will_format.identifier = PAYLOAD_FORMAT_INDICATOR;
	will_format.value.byte = 1;
	rc = MQTTProperties_add(&willProperties, &will_format);
	
	MQTTProperty will_topic;
	will_topic.identifier = RESPONSE_TOPIC;
	will_topic.value.value.data= "MQTTV5/test";
	will_topic.value.value.len=11;
	rc = MQTTProperties_add(&willProperties, &will_topic);
	
	MQTTProperty sub;
	sub.identifier = USER_PROPERTY;
	sub.value.value.data= "FFFF";
	sub.value.value.len=strlen(sub.value.value.data);
	sub.value.data.data= "AAAA";
	sub.value.data.len=strlen(sub.value.data.data);;
	rc = MQTTProperties_add(&subProperties, &sub);
	
#endif
	char* address = "broker.emqx.io";
	char* sub_topic = "MQTTV5/test";
	char* pub_topic = "MQTTV5/test";
	
	int port = 1883;

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
	rtw_create_secure_context(STACKSIZE*2);
	extern int NS_ENTRY secure_mbedtls_platform_set_calloc_free(void);
	secure_mbedtls_platform_set_calloc_free();
	extern void NS_ENTRY secure_set_ns_device_lock(void (*device_mutex_lock_func)(uint32_t), void (*device_mutex_unlock_func)(uint32_t));
	secure_set_ns_device_lock(device_mutex_lock, device_mutex_unlock);
#endif

	memset(readbuf, 0x00, sizeof(readbuf));
	
	NetworkInit(&network);
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

	mqtt_printf(MQTT_INFO, "Wait Wi-Fi to be connected.");
	while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) {
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
	mqtt_printf(MQTT_INFO, "Wi-Fi connected.");
	
	mqtt_printf(MQTT_INFO, "Connect Network \"%s\"", address);
	while ((rc = NetworkConnect(&network, address, port)) != 0){
		mqtt_printf(MQTT_INFO, "Return code from network connect is %d\n", rc);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	mqtt_printf(MQTT_INFO, "\"%s\" Connected", address);
	connectData.clientID.cstring = "AmebaMQTT";
#if defined(MQTTV5)
	connectData.MQTTVersion = 5;
	connectData.keepAliveInterval = 500;
	connectData.cleansession = 1      ;
	connectData.willFlag = 1;
	connectData.will.message.cstring = "will_message";
	connectData.will.qos = 1;
	connectData.will.retained = 1;
	connectData.will.topicName.cstring = "MQTTV5/test";
	
	struct subscribeOptions opts = {0, 0, 0, 0};
	opts.MaxQOS = 2;
	opts.noLocal = 0;
	opts.retainAsPublished = 1;
	opts.retainHandling = 2;
	
#else
	connectData.MQTTVersion = 3;
#endif
	mqtt_printf(MQTT_INFO, "Start MQTT connection");
#if defined(MQTTV5)
	while ((rc = MQTTV5Connect(&client, &connectData, &connectProperties, &willProperties)) != 0){
#else
	while ((rc = MQTTConnect(&client, &connectData)) != 0){
#endif
		mqtt_printf(MQTT_INFO, "Return code from MQTT connect is %d\n", rc);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	
	mqtt_printf(MQTT_INFO, "MQTT Connected");
	
	mqtt_printf(MQTT_INFO, "Subscribe to Topic: %s", sub_topic);
#if defined(MQTTV5)
	if ((rc = MQTTV5Subscribe(&client, sub_topic, &opts, messageArrived, &subProperties)) != 0) 
#else
	if ((rc = MQTTSubscribe(&client, sub_topic, QOS2, messageArrived)) != 0) 
#endif
	mqtt_printf(MQTT_INFO, "Return code from MQTT subscribe is %d\n", rc);

	mqtt_printf(MQTT_INFO, "Publish Topics: %s", pub_topic);
	
	while (1)
	{
		MQTTMessage message;
		char payload[300];
		
		if (++count == 0)
		count = 1;
		
		message.qos = QOS2;
		message.retained = 0;
		message.payload = payload;
		sprintf(payload, "hello from AMEBA %d", count);
		message.payloadlen = strlen(payload);
#if defined(MQTTV5)
		if ((rc = MQTTV5PublishHandle(&client, pub_topic, &message, &pubProperties)) != 0)
#else
		if ((rc = MQTTPublish(&client, pub_topic, &message)) != 0)
#endif
			mqtt_printf(MQTT_INFO,"Return code from MQTT publish is %d\n", rc);
		
		if ((rc = MQTTYield(&client, 1000)) != 0)
			mqtt_printf(MQTT_INFO,"Return code from yield is %d\n", rc);
		vTaskDelay(5000);
	}
	/* do not return */
	vTaskDelete( NULL );
}

#if defined(MQTT_TASK)
#if defined(MQTTV5)
void MQTTPublishMessage(MQTTClient* c, char *topic , MQTTProperties* properties)
#else
void MQTTPublishMessage(MQTTClient* c, char *topic)
#endif
{
	int rc = 0;
	static int count = 0;
	MQTTMessage message;
	char payload[300];
	message.qos = QOS1;
	message.retained = 0;		
	message.payload = payload;
	
	if(c->mqttstatus == MQTT_RUNNING)
	{
		count++;
		sprintf(payload, "hello from AMEBA %d", count);
		message.payloadlen = strlen(payload);			
		mqtt_printf(MQTT_INFO, "Publish Topic %s : %d", topic, count);
#if defined(MQTTV5)
		if ((rc = MQTTV5PublishHandle(c, topic, &message, properties)) != 0)
#else
		if ((rc = MQTTPublish(c, topic, &message)) != 0)
#endif
                {
			mqtt_printf(MQTT_INFO, "Return code from MQTT publish is %d\n", rc);
			MQTTSetStatus(c, MQTT_START);
			c->ipstack->disconnect(c->ipstack);
		}
	}

}

static void prvMQTTTask(void *pvParameters)
{
	/* To avoid gcc warnings */
	( void ) pvParameters;
	MQTTClient client;
	Network network;
	static unsigned char sendbuf[MQTT_SENDBUF_LEN], readbuf[MQTT_READBUF_LEN];
	int rc = 0, mqtt_pub_count = 0;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
#if defined(MQTTV5)
	MQTTProperty connect_props[10], will_props[10], pub_props[10];
	MQTTProperties connectProperties = MQTTProperties_initializer;
	connectProperties.max_count = 6;
	connectProperties.array = connect_props;
	MQTTProperty connect;
	connect.identifier = SESSION_EXPIRY_INTERVAL;
	connect.value.integer4 = 10;
	
	MQTTProperties willProperties = MQTTProperties_initializer;
	willProperties.max_count = 10;
	willProperties.array = will_props;
	MQTTProperty will;
	will.identifier = WILL_DELAY_INTERVAL;
	will.value.integer4 = 1;
	rc = MQTTProperties_add(&willProperties, &will);
	rc = MQTTProperties_add(&connectProperties, &connect);
	
	MQTTProperties pubProperties = MQTTProperties_initializer;
	pubProperties.max_count = 10;
	pubProperties.array = pub_props;
	
	MQTTProperty pub_format;
	pub_format.identifier = PAYLOAD_FORMAT_INDICATOR;
	pub_format.value.byte = 1;
	rc = MQTTProperties_add(&pubProperties, &pub_format);
	
	connectData.MQTTVersion = 5;
#else
	connectData.MQTTVersion = 3;
#endif
	connectData.clientID.cstring = "FT1_018";
	char* address = "broker.emqx.io";
	char* sub_topic = "LASS/Test/Pm25Ameba/#";
	char* pub_topic = "LASS/Test/Pm25Ameba/FT1_018";
	
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
	rtw_create_secure_context(STACKSIZE*2);
	extern int NS_ENTRY secure_mbedtls_platform_set_calloc_free(void);
	secure_mbedtls_platform_set_calloc_free();
	extern void NS_ENTRY secure_set_ns_device_lock(void (*device_mutex_lock_func)(uint32_t), void (*device_mutex_unlock_func)(uint32_t));
	secure_set_ns_device_lock(device_mutex_lock, device_mutex_unlock);
#endif

	NetworkInit(&network);
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));
	while (1)
	{
		while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) {
			mqtt_printf(MQTT_INFO, "Wait Wi-Fi to be connected.");
			vTaskDelay(5000 / portTICK_PERIOD_MS);
		}

		fd_set read_fds;
		fd_set except_fds;
		struct timeval timeout;

		FD_ZERO(&read_fds);
		FD_ZERO(&except_fds);
		timeout.tv_sec = MQTT_SELECT_TIMEOUT;
		timeout.tv_usec = 0;
		if(network.my_socket >= 0)
		{
			FD_SET(network.my_socket, &read_fds);
			FD_SET(network.my_socket, &except_fds);
			rc = FreeRTOS_Select(network.my_socket + 1, &read_fds, NULL, &except_fds, &timeout);
			if(FD_ISSET(network.my_socket, &except_fds))
			{
				mqtt_printf(MQTT_INFO, "except_fds is set");
				MQTTSetStatus(&client, MQTT_START); //my_socket will be close and reopen in MQTTDataHandle if STATUS set to MQTT_START
			}
			else if(rc == 0) //select timeout
			{
				if(++mqtt_pub_count == 5) //Send MQTT publish message every 5 seconds
				{
#if defined(MQTTV5)
					MQTTPublishMessage(&client, pub_topic, &pubProperties);
#else
					MQTTPublishMessage(&client, pub_topic);
#endif
					
					mqtt_pub_count = 0;
				}
			}
		}
#if defined(MQTTV5)
		MQTTDataHandle(&client, &read_fds, &connectData, messageArrived, address, sub_topic, &connectProperties, &willProperties);
#else
		MQTTDataHandle(&client, &read_fds, &connectData, messageArrived, address, sub_topic);
#endif
		
	}
}
#endif

void vStartMQTTTasks(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
	BaseType_t x = 0L;

#if defined(MQTT_TASK)
	xTaskCreate(prvMQTTTask,	/* The function that implements the task. */
	"MQTTTask",			/* Just a text name for the task to aid debugging. */
	usTaskStackSize,	/* The stack size is defined in FreeRTOSIPConfig.h. */
	(void *)x,		/* The task parameter, not used in this case. */
	uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
	NULL);				/* The task handle is not used. */
#else
	xTaskCreate(prvMQTTEchoTask,	/* The function that implements the task. */
	"MQTTEcho0",			/* Just a text name for the task to aid debugging. */
	usTaskStackSize + 128,	/* The stack size is defined in FreeRTOSIPConfig.h. */
	(void *)x,		/* The task parameter, not used in this case. */
	uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
	NULL);				/* The task handle is not used. */
#endif

}

void example_mqtt(void)
{
	vStartMQTTTasks(4096, tskIDLE_PRIORITY + 4);
}
/*-----------------------------------------------------------*/


