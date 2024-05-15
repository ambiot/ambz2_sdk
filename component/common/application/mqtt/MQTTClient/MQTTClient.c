/*******************************************************************************
* Copyright (c) 2014, 2017 IBM Corp.
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* and Eclipse Distribution License v1.0 which accompany this distribution.
*
* The Eclipse Public License is available at
*    http://www.eclipse.org/legal/epl-v10.html
* and the Eclipse Distribution License is available at
*   http://www.eclipse.org/org/documents/edl-v10.php.
*
* Contributors:
*   Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
*   Ian Craggs - fix for #96 - check rem_len in readPacket
*   Ian Craggs - add ability to set message handler separately #6
*******************************************************************************/
#include "MQTTClient.h"

#include <stdio.h>
#include <string.h>
#if defined(MQTTV5)
#include "MQTTProperties.h"
#include "MQTTV5Subscribe.h"
#include "MQTTV5Connect.h"
#include "MQTTV5Publish.h"
#include "MQTTV5Unsubscribe.h"
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

const char * const mqtt_status_str[]=
{
	"MQTT_START",
	"MQTT_CONNECT",
	"MQTT_SUBTOPIC",
	"MQTT_RUNNING"
};

struct GetReason
{
	int code;
	const char* reason;
} GetReason[] =
{
	{0, "Success | Normal disconnection | Granted QoS 0"},
	{1, "Granted QoS 1"},
	{2, "Granted QoS 2"},
	{4, "Disconnect with Will Message"},
	{16, "No matching subscribers"},
	{17, "No subscription existed"},
	{24, "Continue authentication"},
	{25, "Re-authenticate "},
	{128, "Unspecified error"},
	{129, "Malformed Packet"},
	{130, "Protocol Error"},
	{131, "Implementation specific error"},
	{132, "Unsupported Protocol Version"},
	{133, "Client Identifier not valid"},
	{134, "Bad User Name or Password"},
	{135, "Not authorized"},
	{136, "Server unavailable"},
	{137, "Server busy"},
	{138, "Banned"},
	{139, "Server shutting down"},
	{140, "Bad authentication method"},
	{141, "Keep Alive timeout"},
	{142, "Session taken over"},
	{143, "Topic Filter invalid "},
	{144, "Topic Name invalid"},
	{145, "Packet Identifier in use"},
	{146, "Packet Identifier not found"},
	{147, "Receive Maximum exceeded"},
	{148, "Topic Alias invalid"},
	{149, "Packet too large"},
	{150, "Message rate too high"},
	{151, "Quota exceeded"},
	{152, "Administrative action"},
	{153, "Payload format invalid "},
	{154, "Retain not supported"},
	{155, "QoS not supported "},
	{156, "Use another server"},
	{157, "Server moved"},
	{158, "Shared Subscriptions not supported"},
	{159, "Connection rate exceeded"},
	{160, "Maximum connect time"},
	{161, "Subscription Identifiers not supported"},
	{162, "Wildcard Subscriptions not supported"}
};

static void NewMessageData(MessageData* md, MQTTString* aTopicName, MQTTMessage* aMessage) {
	md->topicName = aTopicName;
	md->message = aMessage;
}


static int getNextPacketId(MQTTClient *c) {
	return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}


static int sendPacket(MQTTClient* c, int length, Timer* timer)
{
	int rc = FAILURE,
	sent = 0;
	//goto exit and return failure if packet > max packet size
#if defined(MQTTV5)
	//The Client MUST NOT send packets exceeding Maximum Packet Size to the Server
	if(length > gMaxPacketSize)
	goto exit;
#endif
	while (sent < length && !TimerIsExpired(timer))
	{
		rc = c->ipstack->mqttwrite(c->ipstack, &c->buf[sent], length, TimerLeftMS(timer));
		if (rc < 0)  // there was an error writing the data
		break;
		sent += rc;
	}
	if (sent == length)
	{
		TimerCountdown(&c->last_sent, c->keepAliveInterval); // record the fact that we have successfully sent the packet
		rc = SUCCESS;
	}
	else
	rc = FAILURE;
#if defined(MQTTV5)
exit:
#endif
	return rc;
}


void MQTTClientInit(MQTTClient* c, Network* network, unsigned int command_timeout_ms,
unsigned char* sendbuf, size_t sendbuf_size, unsigned char* readbuf, size_t readbuf_size)
{
	int i;
	c->ipstack = network;

	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
	c->messageHandlers[i].topicFilter = 0;
	c->command_timeout_ms = command_timeout_ms;
	c->buf = sendbuf;
	c->buf_size = sendbuf_size;
	c->readbuf = readbuf;
	c->readbuf_size = readbuf_size;
	c->isconnected = 0;
	c->cleansession = 0;
	c->ping_outstanding = 0;
	c->defaultMessageHandler = NULL;
	c->next_packetid = 1;
	TimerInit(&c->last_sent);
	TimerInit(&c->last_received);
	TimerInit(&c->pingresp_timer);
	c->ipstack->m2m_rxevent = 0;
	c->mqttstatus = MQTT_START;
	TimerInit(&c->cmd_timer);
	TimerInit(&c->ping_timer);
}


static int decodePacket(MQTTClient* c, int* value, int timeout)
{
	unsigned char i;
	int multiplier = 1;
	int len = 0;
	const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

	*value = 0;
	do
	{
		int rc = MQTTPACKET_READ_ERROR;

		if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
		{
			rc = MQTTPACKET_READ_ERROR; /* bad data */
			goto exit;
		}
		rc = c->ipstack->mqttread(c->ipstack, &i, 1, timeout);
		if (rc != 1)
		goto exit;
		*value += (i & 127) * multiplier;
		multiplier *= 128;
	} while ((i & 128) != 0);
exit:
	return len;
}


static int readPacket(MQTTClient* c, Timer* timer)
{
	MQTTHeader header = {0};
	int len = 0;
	int rem_len = 0;

	/* 1. read the header byte.  This has the packet type in it */
	int rc = c->ipstack->mqttread(c->ipstack, c->readbuf, 1, TimerLeftMS(timer));
	mqtt_printf(MQTT_MSGDUMP, "read packet header failed");
	if (rc != 1)
	goto exit;

	len = 1;
	/* 2. read the remaining length.  This is variable in itself */
	decodePacket(c, &rem_len, TimerLeftMS(timer));
	len += MQTTPacket_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

	if (rem_len > (c->readbuf_size - len))
	{
		mqtt_printf(MQTT_WARNING, "rem_len = %d, read buffer will overflow", rem_len);
		rc = BUFFER_OVERFLOW;
		goto exit;
	}

	/* 3. read the rest of the buffer using a callback to supply the rest of the data */
	if (rem_len > 0 && (rc = c->ipstack->mqttread(c->ipstack, c->readbuf + len, rem_len, TimerLeftMS(timer)) != rem_len)) {
		rc = 0;
		mqtt_printf(MQTT_MSGDUMP, "read the rest of the data failed");
		goto exit;
	}

	header.byte = c->readbuf[0];
	rc = header.bits.type;
	if (c->keepAliveInterval > 0)
	TimerCountdown(&c->last_received, c->keepAliveInterval); // record the fact that we have successfully received a packet
exit:
	return rc;
}


// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char* topicFilter, MQTTString* topicName)
{
	char* curf = topicFilter;
	char* curn = topicName->lenstring.data;
	char* curn_end = curn + topicName->lenstring.len;

	while (*curf && curn < curn_end)
	{
		if (*curn == '/' && *curf != '/')
		break;
		if (*curf != '+' && *curf != '#' && *curf != *curn)
		break;
		if (*curf == '+')
		{   // skip until we meet the next separator, or end of string
			char* nextpos = curn + 1;
			while (nextpos < curn_end && *nextpos != '/')
			nextpos = ++curn + 1;
		}
		else if (*curf == '#')
		curn = curn_end - 1;    // skip until end of string
		curf++;
		curn++;
	};

	return (curn == curn_end) && (*curf == '\0');
}


int deliverMessage(MQTTClient* c, MQTTString* topicName, MQTTMessage* message)
{
	int i;
	int rc = FAILURE;

	// we have to find the right message handler - indexed by topic
	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
	{
		if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter) ||
					isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
		{
			if (c->messageHandlers[i].fp != NULL)
			{
				MessageData md;
				NewMessageData(&md, topicName, message);
				c->messageHandlers[i].fp(&md);
				rc = SUCCESS;
			}
		}
	}

	if (rc == FAILURE && c->defaultMessageHandler != NULL)
	{
		MessageData md;
		NewMessageData(&md, topicName, message);
		c->defaultMessageHandler(&md);
		rc = SUCCESS;
	}

	return rc;
}


int keepalive(MQTTClient* c)
{
	int rc = SUCCESS;

	if (c->keepAliveInterval == 0)
	goto exit;

	// If we are waiting for a ping response, check if it has been too long
	if (c->ping_outstanding)
	{
		if (TimerIsExpired(&c->pingresp_timer))
		{
			rc = FAILURE; /* PINGRESP not received in keepalive interval */
			goto exit;
		}
	} else
	{
		// If we have not sent or received anything in the timeout period,
		// send out a ping request
		if (TimerIsExpired(&c->last_sent) || TimerIsExpired(&c->last_received))
		{
			Timer timer;

			TimerInit(&timer);
			TimerCountdownMS(&timer, 1000);
			int len = MQTTSerialize_pingreq(c->buf, c->buf_size);
			if (len > 0 && (rc = sendPacket(c, len, &timer)) == SUCCESS)
			{
				// send the ping packet
				// Expect the PINGRESP within 2 seconds of the PINGREQ
				// being sent
				TimerCountdownMS(&c->pingresp_timer, 2000 );
				c->ping_outstanding = 1;
			}
		}
	}

exit:
	return rc;
}


void MQTTCleanSession(MQTTClient* c)
{
	int i = 0;

	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
	c->messageHandlers[i].topicFilter = NULL;
}


void MQTTCloseSession(MQTTClient* c)
{
	c->ping_outstanding = 0;
	c->isconnected = 0;
	if (c->cleansession)
	MQTTCleanSession(c);
}


int cycle(MQTTClient* c, Timer* timer)
{
	int len = 0,
	rc = SUCCESS;
#if defined(MQTTV5)
	MQTTProperties ackProperties = MQTTProperties_initializer;
	//publish, pubrec, pubrel has total of 9 properties
	MQTTProperty ack_props[9];
	ackProperties.max_count = 9;
	ackProperties.array = ack_props;
	int ReasonCode = -1;
#endif
	int packet_type = readPacket(c, timer);     /* read the socket, see what work is due */
	mqtt_printf(MQTT_DEBUG, "Read packet type: %s", MQTTPacketIDPrint(packet_type));
	switch (packet_type)
	{
	default:
		/* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
		rc = packet_type;
		goto exit;
	case 0: /* timed out reading packet */
		break;
	case CONNACK:
	case PUBACK:
	case SUBACK:
	case UNSUBACK:
		{
			break;
		}
	case PUBLISH:
		{
			MQTTString topicName;
			MQTTMessage msg;
			int intQoS;
			msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
			msg.id = 0;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName, &ackProperties,
						(unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
#else
			if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
						(unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
#endif
			goto exit;
			//If a Server or Client receives a PUBLISH packet which has both QoS bits set to 1 it is a Malformed Packet. Use DISCONNECT with
			//Reason Code 0x81(129)
#if defined(MQTTV5)
			if (intQoS > 2)
			{
				if ((rc = MQTTV5Disconnect(c, 129, NULL)) != 0)
				{
					mqtt_printf(MQTT_INFO, "Return code from MQTT disconnect is %d\n", rc);
					goto exit;
				}
				mqtt_printf(MQTT_INFO, "Incorrect QOS = %d, Disconnected\n", intQoS);
				break;
			}
#endif
			msg.qos = (enum QoS)intQoS;
			deliverMessage(c, &topicName, &msg);
			if (msg.qos != QOS0)
			{
				if (msg.qos == QOS1)
#if defined(MQTTV5)
				len = MQTTV5Serialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id, 0, &ackProperties);
#else
				len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
#endif
				else if (msg.qos == QOS2)
#if defined(MQTTV5)
				len = MQTTV5Serialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id, 0, &ackProperties);
#else
				len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
#endif
				if (len <= 0)
				rc = FAILURE;
				else
				{
					rc = sendPacket(c, len, timer);
				}
				if (rc == FAILURE)
				goto exit; // there was a problem
			}
			break;
		}
		break;
	case PUBREC:
	case PUBREL:
		{
			unsigned short mypacketid = 0;
			unsigned char dup, type;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_ack(&type, &dup, &mypacketid, &ReasonCode, &ackProperties ,c->readbuf, c->readbuf_size) != 1)
#else
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
#endif
			{
				rc = FAILURE; // there was a problem
				goto exit;
			}
#if defined(MQTTV5)
			if ((len = MQTTV5Serialize_ack(c->buf, c->buf_size,
							(packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid, 0, &ackProperties)) <= 0)
#else
			if ((len = MQTTSerialize_ack(c->buf, c->buf_size,
							(packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
#endif
			{
				rc = FAILURE;
				goto exit;
			}
			if ((rc = sendPacket(c, len, timer)) != SUCCESS) // send the PUBREL packet
			{
				rc = FAILURE; // there was a problem
				goto exit;
			} 
			goto exit; // there was a problem
			break;
		}

	case PUBCOMP:
		{
			break;
		}
	case PINGRESP:
		{
			c->ping_outstanding = 0;
			break;
		}
#if defined(MQTTV5)
	case DISCONNECT:
		if(MQTTV5Deserialize_disconnect(&ackProperties, &ReasonCode, c->readbuf, c->readbuf_size) != 1){
			mqtt_printf(MQTT_DEBUG, "Deserialize DISCONNECT failed");
		}
		break;
#endif
	}

exit:
	if (keepalive(c) != SUCCESS) {
		//check only keepalive FAILURE status so that previous FAILURE status can be considered as FAULT
		rc = FAILURE;
	}
	if (rc == SUCCESS)
	rc = packet_type;
	return rc;
}

#if defined(MQTT_TASK)
void MQTTSetStatus(MQTTClient* c, int mqttstatus)
{	
	c->mqttstatus = mqttstatus;
	mqtt_printf(MQTT_INFO, "Set mqtt status to %s", mqtt_status_str[mqttstatus]);
}
#if defined(MQTTV5)
int MQTTDataHandle(MQTTClient* c, fd_set *readfd, MQTTPacket_connectData *connectData, messageHandler messageHandler, char* address, char* topic, MQTTProperties* properties, MQTTProperties* willproperties)
#else
int MQTTDataHandle(MQTTClient* c, fd_set *readfd, MQTTPacket_connectData *connectData, messageHandler messageHandler, char* address, char* topic)
#endif
{	
	short packet_type = 0;
	int rc = 0;
	int mqttstatus = c->mqttstatus;
	int mqtt_rxevent = 0;
	int mqtt_fd = c->ipstack->my_socket;
#if defined(MQTTV5)
	struct subscribeOptions opts = {0, 0, 0, 0};
	opts.MaxQOS = 2;
	opts.noLocal = 0;
	opts.retainAsPublished = 1;
	opts.retainHandling = 2;
	
	MQTTProperties ackproperties;
	ackproperties.count = 0;
	ackproperties.length = 0;
	MQTTProperty ack_props[20];
	ackproperties.array = ack_props;
	ackproperties.max_count = 20;
	
	int reasoncode = -1;
#endif
	mqtt_rxevent = (mqtt_fd >= 0) ? FD_ISSET( mqtt_fd, readfd) : 0;

	if(mqttstatus == MQTT_START) {
		mqtt_printf(MQTT_INFO, "MQTT start");
		if(c->isconnected){
			c->isconnected = 0;
		}
		mqtt_printf(MQTT_INFO, "Connect Network \"%s\"", address);
		if((rc = NetworkConnect(c->ipstack, address, c->ipstack->my_port)) != 0){
			mqtt_printf(MQTT_INFO, "Return code from network connect is %d\n", rc);
			goto exit;
		}
		mqtt_printf(MQTT_INFO, "\"%s\" Connected", address);
		mqtt_printf(MQTT_INFO, "Start MQTT connection");
		TimerInit(&c->cmd_timer);
		TimerCountdownMS(&c->cmd_timer, c->command_timeout_ms);
#if defined(MQTTV5)
		if ((rc = MQTTV5Connect(c, connectData, properties, willproperties)) != 0)
#else
		if ((rc = MQTTConnect(c, connectData)) != 0)
#endif
		{
			mqtt_printf(MQTT_INFO, "Return code from MQTT connect is %d\n", rc);
			goto exit;
		}
		MQTTSetStatus(c, MQTT_CONNECT);
		goto exit;
	}
	
	if(mqtt_rxevent){
		c->ipstack->m2m_rxevent = 0;
		Timer timer;
		TimerInit(&timer);
		TimerCountdownMS(&timer, 1000);
		packet_type = readPacket(c, &timer);
		if(packet_type > 0 && packet_type < 15)
		mqtt_printf(MQTT_DEBUG, "Read packet type is %s", MQTTPacketIDPrint(packet_type));
		else{
			mqtt_printf(MQTT_DEBUG, "Read packet type is %d", packet_type);
			MQTTSetStatus(c, MQTT_START);
			c->ipstack->disconnect(c->ipstack);
			rc = FAILURE;
			goto exit;
			}
	}
	switch(mqttstatus){
	case MQTT_CONNECT:
		if (packet_type == CONNACK)
		{
			unsigned char connack_rc = 255;
			unsigned char sessionPresent = 0;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_connack(&ackproperties,&sessionPresent, &connack_rc, c->readbuf, c->readbuf_size) == 1)
#else
			if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, c->readbuf, c->readbuf_size) == 1)
#endif
			{
				rc = connack_rc;
				mqtt_printf(MQTT_INFO, "MQTT Connected");
				TimerInit(&c->cmd_timer);
				TimerCountdownMS(&c->cmd_timer, c->command_timeout_ms);
#if defined(MQTTV5)
				if (gServerKeepAlive > 0)
				c->keepAliveInterval = gServerKeepAlive;
				if ((rc = MQTTV5Subscribe(c, topic, &opts, messageHandler, properties)) != 0)
#else
				if ((rc = MQTTSubscribe(c, topic, QOS2, messageHandler)) != 0)
#endif
				{
					mqtt_printf(MQTT_INFO, "Return code from MQTT subscribe is %d\n", rc);
				}else{
					mqtt_printf(MQTT_INFO, "Subscribe to Topic: %s", topic);
					MQTTSetStatus(c, MQTT_SUBTOPIC);
				}
			}else{
				mqtt_printf(MQTT_DEBUG, "Deserialize CONNACK failed");
				rc = FAILURE;
			}
		}else if(TimerIsExpired(&c->cmd_timer)){
			mqtt_printf(MQTT_DEBUG, "Not received CONNACK");
			rc = FAILURE;
		}
		if(rc == FAILURE){
			MQTTSetStatus(c, MQTT_START);
		}
		break;
	case MQTT_SUBTOPIC:
		if(packet_type == SUBACK){
			int count = 0, grantedQoS = -1;
			unsigned short mypacketid;
			int isSubscribed = 0;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_suback(&mypacketid, &ackproperties, 1, &count, &grantedQoS, c->readbuf, c->readbuf_size) == 1)
#else
			if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, c->readbuf, c->readbuf_size) == 1)
#endif
			{
					rc = grantedQoS; // 0, 1, 2 or 0x80 
#if !defined(MQTTV5)
					mqtt_printf(MQTT_DEBUG, "grantedQoS: %d", grantedQoS);
#endif
			}
			if (rc != 0x80)
			{
				int i;
				for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
				{
					if (c->messageHandlers[i].topicFilter == topic)
					{
						isSubscribed = 1;
						break;
					}
				}
				if(!isSubscribed)
				for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
				{
					if (c->messageHandlers[i].topicFilter == 0)
					{
						c->messageHandlers[i].topicFilter = topic;
						c->messageHandlers[i].fp = messageHandler;
						break;
					}
				}
				rc = 0;
				MQTTSetStatus(c, MQTT_RUNNING);
			}
		}else if(TimerIsExpired(&c->cmd_timer)){
			mqtt_printf(MQTT_DEBUG, "Not received SUBACK");
			rc = FAILURE;
		}
		if(rc == FAILURE){
			MQTTSetStatus(c, MQTT_START);
		}
		break;
	case MQTT_RUNNING:
		if(packet_type>0){
			int len = 0;
			Timer timer;
			TimerInit(&timer);
			TimerCountdownMS(&timer, 10000);
			switch(packet_type){
	case CONNACK:
		break;
	case PUBACK:
		{
			unsigned short mypacketid;
			unsigned char dup, type;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_ack(&type, &dup, &mypacketid, &reasoncode, &ackproperties, c->readbuf, c->readbuf_size) != 1)
#else
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
#endif
			{
				rc = FAILURE;
				break;
			}
		}
	case SUBACK:
		break;
	case UNSUBACK:
		break;
	case PUBLISH:
		{
			MQTTString topicName;
			MQTTMessage msg;
			int intQoS;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName, &ackproperties,
						(unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
#else
			if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
						(unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
#endif
			{
				rc = FAILURE;
				mqtt_printf(MQTT_DEBUG, "Deserialize PUBLISH failed");
				goto exit;
			}

			msg.qos = (enum QoS)intQoS;
			deliverMessage(c, &topicName, &msg);
			if (msg.qos != QOS0)
			{
				if (msg.qos == QOS1){
#if defined(MQTTV5)
					len = MQTTV5Serialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id, 0, &ackproperties);
#else
					len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
#endif
					mqtt_printf(MQTT_DEBUG, "send PUBACK");
				}else if (msg.qos == QOS2){
#if defined(MQTTV5)
					len = MQTTV5Serialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id, 0, &ackproperties);
#else
					len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
#endif
					mqtt_printf(MQTT_DEBUG, "send PUBREC");
				}else{
					mqtt_printf(MQTT_DEBUG, "invalid QoS: %d", msg.qos);
				}
				if (len <= 0){
					rc = FAILURE;
					mqtt_printf(MQTT_DEBUG, "Serialize_ack failed");
					goto exit;
				}else{
					if((rc = sendPacket(c, len, &timer)) == FAILURE){
						MQTTSetStatus(c, MQTT_START);
						goto exit; // there was a problem
					}
				}
			}
			break;
		}
	case PUBREC:
		{
			unsigned short mypacketid;
			unsigned char dup, type;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_ack(&type, &dup, &mypacketid, &reasoncode, &ackproperties, c->readbuf, c->readbuf_size) != 1)
#else
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
#endif
			{
				mqtt_printf(MQTT_DEBUG, "Deserialize PUBREC failed");
				rc = FAILURE;
			}
#if defined(MQTTV5)
			else if ((len = MQTTV5Serialize_ack(c->buf, c->buf_size, PUBREL, 0, mypacketid, 0, &ackproperties)) <= 0)
#else
			else if ((len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREL, 0, mypacketid)) <= 0)
#endif
			{
				mqtt_printf(MQTT_DEBUG, "Serialize PUBREL failed");
				rc = FAILURE;
			}else if ((rc = sendPacket(c, len, &timer)) != SUCCESS){ // send the PUBREL packet
				rc = FAILURE; // there was a problem
				MQTTSetStatus(c, MQTT_START);
			}
			break;
		}
	case PUBREL:
		{
			unsigned short mypacketid;
			unsigned char dup, type;
#if defined(MQTTV5)
			if (MQTTV5Deserialize_ack(&type, &dup, &mypacketid, &reasoncode, &ackproperties, c->readbuf, c->readbuf_size) != 1)
#else
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
#endif
			{
				mqtt_printf(MQTT_DEBUG, "Deserialize PUBREL failed");
				rc = FAILURE;
			}else if ((len = MQTTSerialize_ack(c->buf, c->buf_size, PUBCOMP, 0, mypacketid)) <= 0){
				mqtt_printf(MQTT_DEBUG, "Serialize PUBCOMP failed");
				rc = FAILURE;
			}else if ((rc = sendPacket(c, len, &timer)) != SUCCESS){ // send the PUBCOMP packet
				rc = FAILURE; // there was a problem
				MQTTSetStatus(c, MQTT_START);
			}
			break;
		}
	case PUBCOMP:
		break;
	case PINGRESP:
		c->ping_outstanding = 0;
		break;
#if defined(MQTTV5)
	case DISCONNECT:
		if(MQTTV5Deserialize_disconnect(&ackproperties, &reasoncode, c->readbuf, c->readbuf_size) != 1){
			mqtt_printf(MQTT_DEBUG, "Deserialize DISCONNECT failed");
		}
		break;
#endif
			}
		}
		keepalive(c);
		break;
	default:
		break;
	}
exit:
	return rc;
}

#endif

int MQTTYield(MQTTClient* c, int timeout_ms)
{
	int rc = SUCCESS;
	Timer timer;

	TimerInit(&timer);
	TimerCountdownMS(&timer, timeout_ms);

	do
	{
		if (cycle(c, &timer) == FAILURE)
		{
			rc = FAILURE;
			break;
		}
	} while (!TimerIsExpired(&timer));
	
	return rc;
}

int MQTTIsConnected(MQTTClient* client)
{
	return client->isconnected;
}

void MQTTRun(void* parm)
{
	Timer timer;
	MQTTClient* c = (MQTTClient*)parm;

	TimerInit(&timer);

	while (1)
	{
		TimerCountdownMS(&timer, 500); /* Don't wait too long if no traffic is incoming */
		cycle(c, &timer);
	}
}


int waitfor(MQTTClient* c, int packet_type, Timer* timer)
{
	int rc = FAILURE;

	do
	{
		if (TimerIsExpired(timer))
		break; // we timed out
		rc = cycle(c, timer);
	}
	while (rc != packet_type && rc >= 0);

	return rc;
}

int MQTTConnectWithResults(MQTTClient* c, MQTTPacket_connectData* options, MQTTConnackData* data)
{
	Timer connect_timer;
	int rc = FAILURE;
	MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
	int len = 0;

	if (c->isconnected) /* don't send connect packet again if we are already connected */
	goto exit;

	TimerInit(&connect_timer);
	TimerCountdownMS(&connect_timer, c->command_timeout_ms);

	if (options == 0)
	options = &default_options; /* set default options if none were supplied */

	c->keepAliveInterval = options->keepAliveInterval;
	c->cleansession = options->cleansession;
	TimerCountdown(&c->last_received, c->keepAliveInterval);
	if ((len = MQTTSerialize_connect(c->buf, c->buf_size, options)) <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &connect_timer)) != SUCCESS)  // send the connect packet
	goto exit; // there was a problem
#if defined(WAIT_FOR_ACK)
	// this will be a blocking call, wait for the connack
	if (waitfor(c, CONNACK, &connect_timer) == CONNACK)
	{
		data->rc = 0;
		data->sessionPresent = 0;
		if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, c->readbuf, c->readbuf_size) == 1)
		rc = data->rc;
		else
		rc = FAILURE;
	}
	else
	rc = FAILURE;
#endif
exit:
	if (rc == SUCCESS)
	{
		c->isconnected = 1;
		c->ping_outstanding = 0;
	}

	return rc;
}

int MQTTConnect(MQTTClient* c, MQTTPacket_connectData* options)
{
	MQTTConnackData data;
	return MQTTConnectWithResults(c, options, &data);
}

int MQTTSetMessageHandler(MQTTClient* c, const char* topicFilter, messageHandler messageHandler)
{
	int rc = FAILURE;
	int i = -1;

	/* first check for an existing matching slot */
	for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
	{
		if (c->messageHandlers[i].topicFilter != NULL && strcmp(c->messageHandlers[i].topicFilter, topicFilter) == 0)
		{
			if (messageHandler == NULL) /* remove existing */
			{
				c->messageHandlers[i].topicFilter = NULL;
				c->messageHandlers[i].fp = NULL;
			}
			rc = SUCCESS; /* return i when adding new subscription */
			break;
		}
	}
	/* if no existing, look for empty slot (unless we are removing) */
	if (messageHandler != NULL) {
		if (rc == FAILURE)
		{
			for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
			{
				if (c->messageHandlers[i].topicFilter == NULL)
				{
					rc = SUCCESS;
					break;
				}
			}
		}
		if (i < MAX_MESSAGE_HANDLERS)
		{
			c->messageHandlers[i].topicFilter = topicFilter;
			c->messageHandlers[i].fp = messageHandler;
		}
	}
	return rc;
}


int MQTTSubscribeWithResults(MQTTClient* c, const char* topicFilter, enum QoS qos,
messageHandler messageHandler, MQTTSubackData* data)
{
	int rc = FAILURE;
	Timer timer;
	int len = 0;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *)topicFilter;

	if (!c->isconnected)
	goto exit;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic, (int*)&qos);
	if (len <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
	goto exit;             // there was a problem
#if defined(WAIT_FOR_ACK)
	if (waitfor(c, SUBACK, &timer) == SUBACK)      // wait for suback
	{
		int count = 0;
		unsigned short mypacketid;
		data->grantedQoS = QOS0;
		if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&data->grantedQoS, c->readbuf, c->readbuf_size) == 1)
		{
			if (data->grantedQoS != 0x80)
			rc = MQTTSetMessageHandler(c, topicFilter, messageHandler);
		}
	}
	else
	rc = FAILURE;
#endif
exit:
	if (rc == FAILURE)
	MQTTCloseSession(c);
	return rc;
}

int MQTTSubscribe(MQTTClient* c, const char* topicFilter, enum QoS qos,
messageHandler messageHandler)
{
	MQTTSubackData data;
	return MQTTSubscribeWithResults(c, topicFilter, qos, messageHandler, &data);
}

int MQTTUnsubscribe(MQTTClient* c, const char* topicFilter)
{
	int rc = FAILURE;
	Timer timer;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *)topicFilter;
	int len = 0;

	if (!c->isconnected)
	goto exit;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	if ((len = MQTTSerialize_unsubscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic)) <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
	goto exit; // there was a problem
#if defined(WAIT_FOR_ACK)
	if (waitfor(c, UNSUBACK, &timer) == UNSUBACK)
	{
		unsigned short mypacketid;  // should be the same as the packetid above
		if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size) == 1)
		{
			/* remove the subscription message handler associated with this topic, if there is one */
			MQTTSetMessageHandler(c, topicFilter, NULL);
		}
	}
	else
	rc = FAILURE;
#endif
exit:
	if (rc == FAILURE)
	MQTTCloseSession(c);
	return rc;
}

int MQTTPublish(MQTTClient* c, const char* topicName, MQTTMessage* message)
{
	int rc = FAILURE;
	Timer timer;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *)topicName;
	int len = 0;

	if (!c->isconnected)
	goto exit;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	if (message->qos == QOS1 || message->qos == QOS2)
	message->id = getNextPacketId(c);

	len = MQTTSerialize_publish(c->buf, c->buf_size, 0, message->qos, message->retained, message->id,
	topic, (unsigned char*)message->payload, message->payloadlen);
	if (len <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
	goto exit; // there was a problem
#if defined(WAIT_FOR_ACK)
	if (message->qos == QOS1)
	{
		if (waitfor(c, PUBACK, &timer) == PUBACK)
		{
			unsigned short mypacketid;
			unsigned char dup, type;
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
			rc = FAILURE;
		}
		else
		rc = FAILURE;
	}
	else if (message->qos == QOS2)
	{
		if (waitfor(c, PUBCOMP, &timer) == PUBCOMP)
		{
			unsigned short mypacketid;
			unsigned char dup, type;
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
			rc = FAILURE;
		}
		else
		rc = FAILURE;
	}
#endif
exit:
	if (rc == FAILURE)
	MQTTCloseSession(c);
	return rc;
}

int MQTTDisconnect(MQTTClient* c)
{
	int rc = FAILURE;
	Timer timer;     // we might wait for incomplete incoming publishes to complete
	int len = 0;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	len = MQTTSerialize_disconnect(c->buf, c->buf_size);
	if (len > 0)
	rc = sendPacket(c, len, &timer);            // send the disconnect packet
	MQTTCloseSession(c);

	return rc;
}

/*******************************************************API for MQTT5.0*******************************************************************/
/*****************************************************************************************************************************************/
#if defined(MQTTV5)
int MQTTV5ConnectWithResults(MQTTClient* c, MQTTPacket_connectData* options, MQTTConnackData* data, MQTTProperties* connectproperties ,MQTTProperties* willproperties, MQTTProperties* connackproperties)
{
	Timer connect_timer;
	int rc = FAILURE;
	MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
	int len = 0;

	if (c->isconnected) /* don't send connect packet again if we are already connected */
	goto exit;

	TimerInit(&connect_timer);
	TimerCountdownMS(&connect_timer, c->command_timeout_ms);

	if (options == 0)
	options = &default_options; /* set default options if none were supplied */

	c->keepAliveInterval = options->keepAliveInterval;
	c->cleansession = options->cleansession;
	TimerCountdown(&c->last_received, c->keepAliveInterval);
	if ((len = MQTTV5Serialize_connect(c->buf, c->buf_size, options, connectproperties, willproperties)) <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &connect_timer)) != SUCCESS)  // send the connect packet
	goto exit; // there was a problem
#if defined(WAIT_FOR_ACK)
	// this will be a blocking call, wait for the connack
	if (waitfor(c, CONNACK, &connect_timer) == CONNACK)
	{
		data->rc = 0;
		data->sessionPresent = 0;
		if (MQTTV5Deserialize_connack(connackproperties,&data->sessionPresent, &data->rc, c->readbuf, c->readbuf_size) == 1)
		rc = data->rc;
		else
		rc = FAILURE;
	}
	else{
		rc = FAILURE;
		mqtt_printf(MQTT_DEBUG, "Not received CONNACK");
	}
#endif 
exit:
	if (rc == SUCCESS)
	{
		c->isconnected = 1;
		c->ping_outstanding = 0;
	}

	return rc;
}

int MQTTV5Connect(MQTTClient* c, MQTTPacket_connectData* options, MQTTProperties* connectproperties, MQTTProperties* willproperties)
{
	int rc = -1;
	MQTTConnackData data;
#if defined(WAIT_FOR_ACK)
	MQTTProperties connackproperties;
	connackproperties.count = 0;
	connackproperties.length = 0;
	//max 17 properties can be returned for CONNACK
	MQTTProperty connack_props[17];
	connackproperties.array = connack_props;
	connackproperties.max_count = 17;
	rc =  MQTTV5ConnectWithResults(c, options, &data, connectproperties, willproperties, &connackproperties);
#else
	rc =  MQTTV5ConnectWithResults(c, options, &data, connectproperties, willproperties, NULL);
#endif
#if defined(WAIT_FOR_ACK)
	if (gServerKeepAlive > 0)
	c->keepAliveInterval = gServerKeepAlive;
#endif
	return rc;
}

int MQTTV5SubscribeWithResults(MQTTClient* c, const char* topicFilter, struct subscribeOptions* options,
messageHandler messageHandler, MQTTSubackData* data,  MQTTProperties* subproperties, MQTTProperties* subackproperties)
{
	int rc = FAILURE;
	Timer timer;
	int len = 0;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *)topicFilter;

	if (!c->isconnected)
	goto exit;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	len = MQTTV5Serialize_subscribe(c->buf, c->buf_size, 0, getNextPacketId(c), subproperties ,1, &topic, NULL, options);
	if (len <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
	goto exit;             // there was a problem
#if defined(WAIT_FOR_ACK)
	if (waitfor(c, SUBACK, &timer) == SUBACK)      // wait for suback
	{
		int count = 0;
		unsigned short mypacketid;
		data->grantedQoS = QOS0;
		if (MQTTV5Deserialize_suback(&mypacketid, subackproperties, 1, &count, (int*)&data->grantedQoS, c->readbuf, c->readbuf_size) == 1)
		{
			if (data->grantedQoS != 0x80)
			rc = MQTTSetMessageHandler(c, topicFilter, messageHandler);
		}
	}
	else
	rc = FAILURE;
#endif

exit:
	if (rc == FAILURE)
	MQTTCloseSession(c);
	return rc;
}

int MQTTV5Subscribe(MQTTClient* c, const char* topicFilter, struct subscribeOptions* options,
messageHandler messageHandler, MQTTProperties* subproperties)
{
	int rc = -1;
	MQTTSubackData data;
#if defined(WAIT_FOR_ACK)
	MQTTProperties subackproperties;
	subackproperties.count = 0;
	subackproperties.length = 0;
	//max properties returned for suback is 2
	MQTTProperty connack_props[2];
	subackproperties.array = connack_props;
	subackproperties.max_count = 2;
	rc = MQTTV5SubscribeWithResults(c, topicFilter, options, messageHandler, &data, subproperties, &subackproperties);
#else
	rc = MQTTV5SubscribeWithResults(c, topicFilter, options, messageHandler, &data, subproperties, NULL);
#endif
	return rc;

}

int MQTTV5Unsubscribe(MQTTClient* c, const char* topicFilter, MQTTProperties* unsubproperties, MQTTProperties* unsubackproperties, int* ReasonCode)
{
	int rc = FAILURE;
	Timer timer;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *)topicFilter;
	int len = 0;
	
	mqtt_printf(MQTT_INFO,"Unsubscribe to topic: %s", topic.cstring);
	if (!c->isconnected)
	goto exit;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	if ((len = MQTTV5Serialize_unsubscribe(c->buf, c->buf_size, 0, getNextPacketId(c), unsubproperties, 1, &topic)) <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
	goto exit; // there was a problem
#if defined(WAIT_FOR_ACK)
	if (waitfor(c, UNSUBACK, &timer) == UNSUBACK)
	{
		int count = 0;
		unsigned short mypacketid;  // should be the same as the packetid above
		if (MQTTV5Deserialize_unsuback(&mypacketid, unsubackproperties, 1, &count, ReasonCode, c->readbuf, c->readbuf_size) == 1)
		{
			/* remove the subscription message handler associated with this topic, if there is one */
			MQTTSetMessageHandler(c, topicFilter, NULL);
		}
	}
	else
	rc = FAILURE;
#endif

exit:
	if (rc == FAILURE)
	MQTTCloseSession(c);
	return rc;
}

int MQTTV5UnsubscribeHandle(MQTTClient* c, const char* topicFilter, MQTTProperties* unsubproperties)
{
	int rc = -1;
	int ReasonCode;
#if defined(WAIT_FOR_ACK)
	MQTTProperties unsubackproperties;
	unsubackproperties.count = 0;
	unsubackproperties.length = 0;
	//max 2 properties can be returned for UNSUBACK
	MQTTProperty props[2];
	unsubackproperties.array = props;
	unsubackproperties.max_count = 2;
	rc =  MQTTV5Unsubscribe(c,topicFilter,unsubproperties,&unsubackproperties,&ReasonCode);
#else
	rc =  MQTTV5Unsubscribe(c,topicFilter,unsubproperties,NULL,&ReasonCode);
#endif
	return rc;
}

int MQTTV5Publish(MQTTClient* c, const char* topicName, MQTTMessage* message, MQTTProperties* pubproperties, MQTTProperties* ack, int* reasoncode)
{
	int rc = FAILURE;
	Timer timer;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *)topicName;
	int len = 0;

	if (!c->isconnected)
	goto exit;

	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);
	//If a Client receives a Maximum QoS from a Server, it MUST NOT send PUBLISH packets at a QoS level 
	//exceeding the Maximum QoS level specified
	if (message->qos > gMAXQOSSpecified)
	{
		rc = FAILURE;
		mqtt_printf(MQTT_WARNING, "MAX QOS Specified = %d, Message QOS = %d",gMAXQOSSpecified, message->qos);
		goto exit;
	}
	//A Client receiving Retain Available set to 0 from the Server MUST NOT send a PUBLISH packet with the
	//RETAIN flag set to 1
	if (message->retained > gRetainAvailable)
	{
		rc = FAILURE;
		mqtt_printf(MQTT_WARNING, "Retailed Available = %d, Retain Flag = %d",gRetainAvailable, message->retained);
		goto exit;
	}
	
	if (message->qos == QOS1 || message->qos == QOS2)
	message->id = getNextPacketId(c);

	len = MQTTV5Serialize_publish(c->buf, c->buf_size, 0, message->qos, message->retained, message->id,
	topic, pubproperties, (unsigned char*)message->payload, message->payloadlen);
	if (len <= 0)
	goto exit;
	if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
	goto exit; // there was a problem
#if defined(WAIT_FOR_ACK)
	if (message->qos == QOS1)
	{
		if (waitfor(c, PUBACK, &timer) == PUBACK)
		{
			unsigned short mypacketid;
			unsigned char dup, type;
			if (MQTTV5Deserialize_ack(&type, &dup, &mypacketid, reasoncode, ack, c->readbuf, c->readbuf_size) != 1)
			rc = FAILURE;
		}
		else
		rc = FAILURE;
	}
	else if (message->qos == QOS2)
	{
		if (waitfor(c, PUBCOMP, &timer) == PUBCOMP)
		{
			unsigned short mypacketid;
			unsigned char dup, type;
			if (MQTTV5Deserialize_ack(&type, &dup, &mypacketid, reasoncode, ack, c->readbuf, c->readbuf_size) != 1)
			rc = FAILURE;
		}
		else
		rc = FAILURE;
	}
#endif
exit:
	if (rc == FAILURE)
	MQTTCloseSession(c);
	return rc;
}

int MQTTV5PublishHandle(MQTTClient* c, const char* topicName, MQTTMessage* message, MQTTProperties* pubproperties)
{
	int rc = -1;
	int ReasonCode;
#if defined(WAIT_FOR_ACK)
	MQTTProperties pubcompproperties;
	pubcompproperties.count = 0;
	pubcompproperties.length = 0;
	//max 2 properties can be returned for PUBCOMP
	MQTTProperty props[2];
	pubcompproperties.array = props;
	pubcompproperties.max_count = 2;
	rc =  MQTTV5Publish(c, topicName, message, pubproperties, &pubcompproperties, &ReasonCode);
#else
	rc =  MQTTV5Publish(c, topicName, message, pubproperties, NULL, &ReasonCode);
#endif
	return rc;
}  

int MQTTV5Disconnect(MQTTClient* c, int ReasonCode, MQTTProperties* DisconProperties)
{  
	int rc = FAILURE;
	Timer timer;     // we might wait for incomplete incoming publishes to complete
	int len = 0;
	
	TimerInit(&timer);
	TimerCountdownMS(&timer, c->command_timeout_ms);

	len = MQTTV5Serialize_disconnect(c->buf, c->buf_size, ReasonCode, DisconProperties);
	if (len > 0)
	rc = sendPacket(c, len, &timer);            // send the disconnect packet
	c->isconnected = 0;

	return rc;
}

const char* MQTTGetReason(int ReasonCode)
{
	int i;
	const char* rc;

	for (i = 0; i < ARRAY_SIZE(GetReason); ++i)
	{
		if (GetReason[i].code == ReasonCode)
		{
			rc = GetReason[i].reason;
			break;
		}
	}
	return rc;
} 
#endif

const char* MQTTPacketIDPrint(int packet_id)
{
	switch(packet_id)
	{
	case CONNECT:
		return "CONNECT";
	case CONNACK:
		return "CONNACK";
	case PUBLISH:
		return "PUBLISH";
	case PUBACK:
		return "PUBACK";
	case PUBREC:
		return "PUBREC";
	case PUBREL:
		return "PUBREL";
	case PUBCOMP:
		return "PUBCOMP";
	case SUBSCRIBE:
		return "SUBSCRIBE";
	case SUBACK:
		return "SUBACK";
	case UNSUBSCRIBE:
		return "UNSUBSCRIBE";
	case UNSUBACK:
		return "UNSUBACK";
	case PINGREQ:
		return "PINGREQ";
	case PINGRESP:
		return "PINGRESP";
	case DISCONNECT:
		return "DISCONNECT";
	default:
		return "";
	}
}