/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//#define MQTT_MUTUAL_AUTHENTICATION

#ifdef MQTT_MUTUAL_AUTHENTICATION
static char g_root_crt[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIID9zCCAt+gAwIBAgIUOwLgBtkyxuJe5MDEvIIf7Fj8FdowDQYJKoZIhvcNAQEL\r\n"
	"BQAwgYoxCzAJBgNVBAYTAlNHMRIwEAYDVQQIDAlTaW5nYXBvcmUxFDASBgNVBAcM\r\n"
	"C0p1cm9uZyBFYXN0MRAwDgYDVQQKDAdyZWFsdGVrMRMwEQYDVQQDDApyZWFsdGVr\r\n"
	"LmlvMSowKAYJKoZIhvcNAQkBFhtkb21lbmlxdWVsZWVAcmVhbHRlay1zZy5jb20w\r\n"
	"HhcNMjEwMTEyMDgxNDMzWhcNMjIwMTEyMDgxNDMzWjCBijELMAkGA1UEBhMCU0cx\r\n"
	"EjAQBgNVBAgMCVNpbmdhcG9yZTEUMBIGA1UEBwwLSnVyb25nIEVhc3QxEDAOBgNV\r\n"
	"BAoMB3JlYWx0ZWsxEzARBgNVBAMMCnJlYWx0ZWsuaW8xKjAoBgkqhkiG9w0BCQEW\r\n"
	"G2RvbWVuaXF1ZWxlZUByZWFsdGVrLXNnLmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\n"
	"ggEPADCCAQoCggEBAKnGeZSQJ1OfK0c/HMZNbu6qux9LRjQorb3eYpoSzZfr8d3N\r\n"
	"DBIdPg2nAAXiU0rtlIn/H/QD3hqg3ByWPoByGNZYhKQ5Xxnx89BwnVGBreH9vCdU\r\n"
	"4OLFFzfgqbQynH60PYcHHqtie4FQgkCPGJZdEvRgUScch12HN3RK0nObwDffNi/3\r\n"
	"+Z8ANx+CHSGvZ/TOdFfB/q68W3KQvxYSqbhrXK1+NWY7sHxh3w6RmpIRGrUeonRk\r\n"
	"ltfgChTuOvSDqFXK+/fXOGJpdBBLXRuP9l8cfFGXoAkg3ovroWjQ75hqlcXFyknh\r\n"
	"1f672cbqAbjv2652M4CRQT3PRoSYTm5l7RE7LzUCAwEAAaNTMFEwHQYDVR0OBBYE\r\n"
	"FGx77ATFX86ML2KjtwDM/VN3zi/DMB8GA1UdIwQYMBaAFGx77ATFX86ML2KjtwDM\r\n"
	"/VN3zi/DMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAAftzfsp\r\n"
	"f10BDEA8XdwAJU8ndi5sI5TA9b2sQu6Y/5dvXuZKAhYQPSv4e2ojNP5KywiLtsog\r\n"
	"sQeJlbjwIIpqutBLAOP2sqvB282b03eTzB8TMHhqYKzmaketF0qWrQQBzLCjonFi\r\n"
	"JHtZo4YPjRMRiqkoAKkt5MUG7uYojH7nTFZh6m8hugIl890luW/OO28zh5GOZf5R\r\n"
	"dJ/mTCAHfgIyk7lsgiUOqLXlQ5FZ/swyvDSJSQN5Oc02SvbRHjMfgQh8oJukV79N\r\n"
	"WckMN7z3EyLYwAgf53ZE8jXihjwbXNUODuGhBv76cT3lLkPvRqGtgDm7UMiLw1e9\r\n"
	"SkdheZZBMR4YOKg=\r\n"
	"-----END CERTIFICATE-----\r\n";

static char g_client_crt[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDmTCCAoECFE8NtKZ4z65bB88Ij6JsUz+MpEJoMA0GCSqGSIb3DQEBCwUAMIGK\r\n"
	"MQswCQYDVQQGEwJTRzESMBAGA1UECAwJU2luZ2Fwb3JlMRQwEgYDVQQHDAtKdXJv\r\n"
	"bmcgRWFzdDEQMA4GA1UECgwHcmVhbHRlazETMBEGA1UEAwwKcmVhbHRlay5pbzEq\r\n"
	"MCgGCSqGSIb3DQEJARYbZG9tZW5pcXVlbGVlQHJlYWx0ZWstc2cuY29tMB4XDTIx\r\n"
	"MDExMjA4MTczNFoXDTIxMDQyMjA4MTczNFowgYYxCzAJBgNVBAYTAlNHMRIwEAYD\r\n"
	"VQQIDAlTaW5nYXBvcmUxETAPBgNVBAcMCFdvb2RsYW5kMRAwDgYDVQQKDAdyZWFs\r\n"
	"dGVrMRIwEAYDVQQDDAlpYW1jbGllbnQxKjAoBgkqhkiG9w0BCQEWG2RvbWVuaXF1\r\n"
	"ZWxlZUByZWFsdGVrLXNnLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC\r\n"
	"ggEBAK4zyBb+q4Zy/PnSTBYXW7RCjmCqg9q9hh7fPCpuPuwy3x6ac5SLNiJoMrI/\r\n"
	"mN2BaKGFqJhj6Ot5nZHgBoeYuxPLfAn390aJfyxhyZlRfzpSHezrMf2YDFDgiwl2\r\n"
	"+bqZkezCqFvwMBAmGjnd9FXv2Mab4hLdqB0ibPtqqEugx55PxqME1ZdDV19U5oYZ\r\n"
	"fnWS7mIveIOsTgVzfM57x4S2XWtpp7Dh7GyuXed2JmDb7jG53/yRf7WeTq3q9OX7\r\n"
	"DvlfncQE0nhf/5xQ825vkK219Q3/aAI2Sa3iL9NFhjA5G+pwLZ4fkBOQ9Q4Hx1BF\r\n"
	"c2PcpTsPHYrzZOZUkHE/IE/cRXMCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAJJS6\r\n"
	"FdS6XTHR8xbrZDNdrXyVe7BBOPXM5v5xczUsDW1zOyvlnSVjRz4O95y1VModiHKZ\r\n"
	"OpjzH0H40AaPhBavZrbyDDMc3bL7WcqkAcaX8DXM+kl2ooZ2RyD95+rJ55rULmJw\r\n"
	"o0tH8E9Rid+qAXznoaSafs8hi2ZRg/lIHeY8V2y97+WXFI3dXZALMKB3hPAP8kDx\r\n"
	"+hWjD6zWFtZulByhLslzROAsMdNHlzUy5Pxyc0fcyGNUCZlC3YJuPeUWK6h3IxpF\r\n"
	"DPaxeeukiPnpbwQCpP4tcpo3dUX/0tQfidLL3HLpDCOTBkaTFi+xAaJq6BXEdQoA\r\n"
	"5diZEXvxjLklXbagHA==\r\n"
	"-----END CERTIFICATE-----\r\n";

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
//client key is moved to secure zone
#else
static char g_client_pkey[] =
	"-----BEGIN RSA PRIVATE KEY-----\r\n"
	"MIIEowIBAAKCAQEArjPIFv6rhnL8+dJMFhdbtEKOYKqD2r2GHt88Km4+7DLfHppz\r\n" \
	"lIs2Imgysj+Y3YFooYWomGPo63mdkeAGh5i7E8t8Cff3Rol/LGHJmVF/OlId7Osx\r\n" \
	"/ZgMUOCLCXb5upmR7MKoW/AwECYaOd30Ve/YxpviEt2oHSJs+2qoS6DHnk/GowTV\r\n" \
	"l0NXX1Tmhhl+dZLuYi94g6xOBXN8znvHhLZda2mnsOHsbK5d53YmYNvuMbnf/JF/\r\n" \
	"tZ5Orer05fsO+V+dxATSeF//nFDzbm+QrbX1Df9oAjZJreIv00WGMDkb6nAtnh+Q\r\n" \
	"E5D1DgfHUEVzY9ylOw8divNk5lSQcT8gT9xFcwIDAQABAoIBAGMgqjKTlq16T4Ql\r\n" \
	"bBCCGDf6S6SsQz287U4x/72xYHLt+9MhVrXnhdWMb5TI5cbeNdj8AtU8vTUJzmVN\r\n" \
	"EsokWghImXves8JayZhKyPpKytg4FDlWGOtSk2eObu/i2PabHtOYbU9r3R3DYNce\r\n" \
	"+0PPqQ5TdBECOwDhLxzeM4hdpFnr4LUO3YGR0kDDIL+9eD4S8rFehD/MuiyFAexZ\r\n" \
	"EZCiY/KVUaxwnOLCCIGKrsX4UZJQs56Y/frKYIJGV/MLsid1dA43Ea5xh2DjGfWL\r\n" \
	"Kyv4gddqC5NmxzHOd4CAkz+N9V3uoLUl/DrFnNTZk2y0qGibFyYZ1m+MdQTIqgUz\r\n" \
	"rxy/ijkCgYEA4Ioyjj4oPe7tf2didNnJ7UaAcZ0rfIlrDmP28/RU5ce49xoBiyHx\r\n" \
	"dyr0WZz4esUe/8DE9Tmh1DA1Ud1iNMTX9vqfi1H6dbtcqN43NxLkUobDXzbo3yTf\r\n" \
	"x3A//0pkqaLfvIgPYJm6zHo81f0oH+ifVbUm2j7qqf7ev8lNk3BsCWUCgYEAxpwT\r\n" \
	"H9No0jVaNKOQNHCNBf5X18Jhze1NxGYsc+NHByZqwj8lPHfIn8mFXEOqLResvquz\r\n" \
	"aacSmBKk3X1PjR15esJ7bjn1jSg5xn+gqnKdiL7aGQaYlfoa94+lxmFc8Rwh16oy\r\n" \
	"FkAWCvXe6qoDL2D+LA84GdNXl9acImZuI87JkfcCgYBTEg2+HjTZJOnstpzwShqw\r\n" \
	"k+K1JcaO3nAi9MzKWFyIXimKEa78gGRLCDM6bB5pnT/osrKrXtUGIYe4b8UDbMfR\r\n" \
	"cctQydV1dQnE1+FpmK2r8nuZYEErPQMlT9v9YNJ5B+7qWxQFUxqOx7J2IKCVnYRF\r\n" \
	"Oy3SlHBQ0CAsqBbxMyhlGQKBgD01wNcgKsAOXVy3xCvFPFFKj/wZFleG+V68NpsI\r\n" \
	"ws07U8/F8k3uhNBqJUXIPNxk3YJuHH9FVH+1z0XL5waQvO3OnofcKQd2DEhW8UdW\r\n" \
	"x6JrX4Ay/jfKuyDtDqRTQXe1ueBjvcgJvEFogWlFOHITrIIbtqiO8AIFzFz1wKAc\r\n" \
	"jRcFAoGBAKvChQ4oNdCup+BzaspVB5EOq/DOD/tOsWxxfVGXJzbb3lqcyOZH7Y2J\r\n" \
	"qmwbvXxMWxar6Q14+5NjSHGLgXm88fietkqn2koHZCehGeos57xAPXtdrWSOlqVP\r\n" \
	"NgpHqwoFduLqUqdrRblzguxCxk+p1ObZn8E+t8JAeERvqj0m4Hh5\r\n"
	"-----END RSA PRIVATE KEY-----\r\n";
#endif
#endif

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
static void messageArrived(MessageData *data)
{
	mqtt_printf(MQTT_INFO, "Message arrived on topic %s: %s\n", data->topicName->lenstring.data, (char *)data->message->payload);
}

//This example is original and cannot restart if failed. To use this example, define WAIT_FOR_ACK and not define MQTT_TASK in MQTTClient.h
void prvMQTTEchoTask(void *pvParameters)
{
	/* To avoid gcc warnings */
	(void) pvParameters;

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
	will_topic.value.value.data = "MQTTV5/test";
	will_topic.value.value.len = 11;
	rc = MQTTProperties_add(&willProperties, &will_topic);

	MQTTProperty sub;
	sub.identifier = USER_PROPERTY;
	sub.value.value.data = "FFFF";
	sub.value.value.len = strlen(sub.value.value.data);
	sub.value.data.data = "AAAA";
	sub.value.data.len = strlen(sub.value.data.data);;
	rc = MQTTProperties_add(&subProperties, &sub);

#endif
	char *address = "broker.emqx.io";
	char *sub_topic = "MQTTV5/test";
	char *pub_topic = "MQTTV5/test";

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
	rtw_create_secure_context(STACKSIZE * 2);
	extern int NS_ENTRY secure_mbedtls_platform_set_calloc_free(void);
	secure_mbedtls_platform_set_calloc_free();
	extern void NS_ENTRY secure_set_ns_device_lock(void (*device_mutex_lock_func)(uint32_t), void (*device_mutex_unlock_func)(uint32_t));
	secure_set_ns_device_lock(device_mutex_lock, device_mutex_unlock);
#endif

	memset(readbuf, 0x00, sizeof(readbuf));

	NetworkInit(&network);
#ifdef MQTT_MUTUAL_AUTHENTICATION
	network.rootCA = g_root_crt;
	network.clientCA = g_client_crt;
	network.private_key = g_client_pkey;
	network.use_ssl = 1;
	network.my_port = 8883;
#else
	network.my_port = 1883;
#endif

	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

	mqtt_printf(MQTT_INFO, "Wait Wi-Fi to be connected.");
	while (wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) {
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
	mqtt_printf(MQTT_INFO, "Wi-Fi connected.");

	mqtt_printf(MQTT_INFO, "Connect Network \"%s\"", address);
	while ((rc = NetworkConnect(&network, address, network.my_port)) != 0) {
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
	while ((rc = MQTTV5Connect(&client, &connectData, &connectProperties, &willProperties)) != 0)
#else
	while ((rc = MQTTConnect(&client, &connectData)) != 0)
#endif
	{
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

	while (1) {
		MQTTMessage message;
		char payload[300];

		if (++count == 0) {
			count = 1;
		}

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
			mqtt_printf(MQTT_INFO, "Return code from MQTT publish is %d\n", rc);

		if ((rc = MQTTYield(&client, 1000)) != 0) {
			mqtt_printf(MQTT_INFO, "Return code from yield is %d\n", rc);
		}
		vTaskDelay(5000);
	}
	/* do not return */
	//vTaskDelete(NULL);
}

#if defined(MQTT_TASK)
#if defined(MQTTV5)
void MQTTPublishMessage(MQTTClient *c, char *topic, MQTTProperties *properties)
#else
void MQTTPublishMessage(MQTTClient *c, char *topic)
#endif
{
	int rc = 0;
	static int count = 0;
	MQTTMessage message;
	char payload[300];
	message.qos = QOS1;
	message.retained = 0;
	message.payload = payload;

	if (c->mqttstatus == MQTT_RUNNING) {
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
	(void) pvParameters;
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
	char *address = "broker.emqx.io";
	char *sub_topic = "LASS/Test/Pm25Ameba/#";
	char *pub_topic = "LASS/Test/Pm25Ameba/FT1_018";

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1) && defined(CONFIG_SSL_CLIENT_PRIVATE_IN_TZ) && (CONFIG_SSL_CLIENT_PRIVATE_IN_TZ == 1)
	rtw_create_secure_context(STACKSIZE * 2);
	extern int NS_ENTRY secure_mbedtls_platform_set_calloc_free(void);
	secure_mbedtls_platform_set_calloc_free();
	extern void NS_ENTRY secure_set_ns_device_lock(void (*device_mutex_lock_func)(uint32_t), void (*device_mutex_unlock_func)(uint32_t));
	secure_set_ns_device_lock(device_mutex_lock, device_mutex_unlock);
#endif

	NetworkInit(&network);
#ifdef MQTT_MUTUAL_AUTHENTICATION
	network.rootCA = g_root_crt;
	network.clientCA = g_client_crt;
	network.private_key = g_client_pkey;
	network.use_ssl = 1;
	network.my_port = 8883;
#else
	network.my_port = 1883;
#endif
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));
	while (1) {
		while (wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) {
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
		if (network.my_socket >= 0) {
			FD_SET(network.my_socket, &read_fds);
			FD_SET(network.my_socket, &except_fds);
			rc = FreeRTOS_Select(network.my_socket + 1, &read_fds, NULL, &except_fds, &timeout);
			if (FD_ISSET(network.my_socket, &except_fds)) {
				mqtt_printf(MQTT_INFO, "except_fds is set");
				MQTTSetStatus(&client, MQTT_START); //my_socket will be close and reopen in MQTTDataHandle if STATUS set to MQTT_START
			} else if (rc == 0) { //select timeout
				if (++mqtt_pub_count == 5) { //Send MQTT publish message every 5 seconds
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
