#include <platform_stdlib.h>
#include <httpd/httpd.h>
#include "bt_mesh_app_http_intf.h"
#include "bt_mesh_app_list_intf.h"
#include "platform_os.h"
#include "os_task.h"

#define USE_HTTPS    0

#if USE_HTTPS
// use test_srv_crt, test_srv_key, test_ca_list in PolarSSL certs.c
#if (HTTPD_USE_TLS == HTTPD_TLS_POLARSSL)
#include <polarssl/certs.h>
#elif (HTTPD_USE_TLS == HTTPD_TLS_MBEDTLS)
#include <mbedtls/certs.h>
#endif
#endif

extern uint8_t stop_list_flushing_flag;
extern struct BT_MESH_PRIV bt_mesh_priv;

int chartodec(char value)
{
	int data = 0;
	if((value >= '0') && (value <= '9'))
		data = value - '0';
	else if((value >= 'a') && (value <= 'f'))
		data = value - 'a' + 10;
	else if((value >= 'A') && (value <= 'F'))
		data = value - 'A' + 10;

	return data;
}

int dectochar(uint8_t value)
{
	char data;
	if(value <= 9)
		data = value + '0';
	else if((value >= 10) && (value <= 15))
		data = value + 'a' - 10;
	
	return data;
}

extern void *httpd_malloc(size_t size);

char *OneBytetoTwoByte(char *value, uint8_t flag)
{
	char *bt_mesh_event_value = NULL;
	int i = 0, j = 0,k = 0;
	uint8_t *ptr = NULL;
	//get length
	uint16_t length= 0;
	
	if(flag == RTW_BT_MESH_EVENT_PROVISIONER_STATE)
		length = 5;
	else if(flag == RTW_BT_MESH_EVENT_UNPROVISIONED_DEVICE)
		length = 4 + 1 + (*(value + 4)) * 16;
	else if(flag == RTW_BT_MESH_EVENT_DEVICE_CONNECT_INFO)
		length = 4 + 1 + (*(value + 4)) * 20;
	else if(flag == RTW_BT_MESH_EVENT_NODE_DELETE_INFO)
		length = 4 + 1 + (*(value + 4)) * 3;
	else if(flag == RTW_BT_MESH_EVENT_NODE_STATUS)
		length = 4 + 1 + (*(value + 4)) * 6;

	//malloc char type bt_mesh_event_value
	bt_mesh_event_value = (char *)httpd_malloc(2 * length + 1);
	memset(bt_mesh_event_value,0,2 * length + 1);

	///filled bt_mesh_event_value's event 0-1 byte
	*bt_mesh_event_value = (*(value) & 0xf0) >> 4; 
	*(bt_mesh_event_value + 1) = (*(value) & 0x0f);
	//filled bt_mesh_event_value's event id  2-3 byte
	*(bt_mesh_event_value + 2)= (*(value + 1) & 0xf0) >> 4; 
	*(bt_mesh_event_value + 3) = (*(value + 1) & 0x0f);
	
	///fill Length filed 4-7 byte
	*(bt_mesh_event_value + 4) = ((2 * length) & 0xf000) >> 12 ;
	*(bt_mesh_event_value + 5) = ((2 * length) & 0x0f00) >> 8 ;
	*(bt_mesh_event_value + 6) = ((2 * length) & 0x00f0) >> 4 ;
	*(bt_mesh_event_value + 7) = ((2 * length) & 0x000f) ;

	switch(flag){
	case RTW_BT_MESH_EVENT_PROVISIONER_STATE:
		*(bt_mesh_event_value + 8) = (*(value + 4) & 0xf0) >> 4;
		*(bt_mesh_event_value + 9) = (*(value + 4) & 0x0f);
		break;
	case RTW_BT_MESH_EVENT_UNPROVISIONED_DEVICE:
		///fill device number filed 8-9 byte
		*(bt_mesh_event_value + 8) = (*(value + 4) & 0xf0) >> 4;
		*(bt_mesh_event_value + 9) = (*(value + 4) & 0x0f);

		ptr = (uint8_t *)(value + 5);//point to first uuid
		for(i = 0; i < *(value + 4) * 16; i ++){
			*(bt_mesh_event_value + 10 + j) = (*(ptr + i) & 0xf0) >> 4;
			*(bt_mesh_event_value + 10 + j + 1) = (*(ptr + i) & 0x0f);
			j = j + 2;
		}

		break;
	case RTW_BT_MESH_EVENT_DEVICE_CONNECT_INFO:
		///fill device number filed 8-9 byte
		*(bt_mesh_event_value + 8) = (*(value + 4) & 0xf0) >> 4;
		*(bt_mesh_event_value + 9) = (*(value + 4) & 0x0f);
		ptr = (uint8_t *)(value + 5);//point to first uuid

		//fill uuid
		for(i = 0,j = 0; i < *(value + 4) * 20;){
			///fill uuid addr
			for(k = i; k < 16 + i; k ++){
				*(bt_mesh_event_value + 10 + j) = (*(ptr + k ) & 0xf0) >> 4;
				*(bt_mesh_event_value + 10 + j + 1) = (*(ptr + k ) & 0x0f);
				j = j + 2;
			}
			//fill connect status
			*(bt_mesh_event_value + 10 + j) = (*(ptr + k) & 0xf0) >> 4;
			*(bt_mesh_event_value + 10 + j + 1) = (*(ptr + k) & 0x0f);

			//fill mesh addr
			*(bt_mesh_event_value + 10 + j + 2) = (*(ptr + k + 2) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 3) = (*(ptr + k + 2) & 0x0f) ;
			*(bt_mesh_event_value + 10 + j + 4) = (*(ptr + k + 1) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 5) = (*(ptr + k + 1) & 0x0f) ;
	
			//fill light state
			*(bt_mesh_event_value + 10 + j + 6) = (*(ptr + k + 3) & 0xf0) >> 4;
			*(bt_mesh_event_value + 10 + j + 7) = (*(ptr + k + 3) & 0x0f);	

			i = i + 20;
			j = j + 8;
		}

		break;
	case RTW_BT_MESH_EVENT_NODE_DELETE_INFO:
		///fill device number filed 8-9 byte
		*(bt_mesh_event_value + 8) = (*(value + 4) & 0xf0) >> 4;
		*(bt_mesh_event_value + 9) = (*(value + 4) & 0x0f) ;
		ptr = (uint8_t *)(value + 5);
		for(i = 0; i < *(value + 4) * 3;){
			//fill mesh addr
			*(bt_mesh_event_value + 10 + j) = (*(ptr + i + 1) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 1) = (*(ptr + i  + 1) & 0x0f) ;
			*(bt_mesh_event_value + 10 + j + 2) = (*(ptr + i ) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 3) = (*(ptr + i) & 0x0f) ;
			//fill delete note
			*(bt_mesh_event_value + 10 + j + 4) = (*(ptr + i + 2) & 0xf0) >> 4;
			*(bt_mesh_event_value + 10 + j + 5) = (*(ptr + i + 2) & 0x0f);

			i = i + 3;
			j = j + 6;
		}
		break;
	case RTW_BT_MESH_EVENT_NODE_STATUS:
		///fill device number filed 8-9 byte
		*(bt_mesh_event_value + 8) = (*(value + 4) & 0xf0) >> 4;
		*(bt_mesh_event_value + 9) = (*(value + 4) & 0x0f);
		ptr = (uint8_t *)(value + 5);
		for(i = 0; i < *(value + 4) * 6;){
			//fill mesh addr
			*(bt_mesh_event_value + 10 + j) = (*(ptr + i + 1) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 1) = (*(ptr + i + 1 ) & 0x0f) ;
			*(bt_mesh_event_value + 10 + j + 2) = (*(ptr + i) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 3) = (*(ptr + i) & 0x0f) ;
			//fill group addr
			*(bt_mesh_event_value + 10 + j + 4) = (*(ptr + i + 3) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 5) = (*(ptr + i + 3) & 0x0f) ;
			*(bt_mesh_event_value + 10 + j + 6) = (*(ptr + i + 2) & 0xf0) >> 4 ;
			*(bt_mesh_event_value + 10 + j + 7) = (*(ptr + i + 2) & 0x0f) ;
			//fill active state
			*(bt_mesh_event_value + 10 + j + 8) = (*(ptr + i + 4) & 0xf0) >> 4;
			*(bt_mesh_event_value + 10 + j + 9) = (*(ptr + i + 4) & 0x0f);
			//fill light state
			*(bt_mesh_event_value + 10 + j + 10) = (*(ptr + i + 5) & 0xf0) >> 4;
			*(bt_mesh_event_value + 10 + j + 11) = (*(ptr + i + 5) & 0x0f);

			i = i + 6;
			j = j + 12;
		}
		break;
	default:
		printf("\r\n unknow flag \n");
	}
	///dec to char
	for(int i = 0; i < 2 * length; i ++)
	{
		*(bt_mesh_event_value + i) = dectochar(*(bt_mesh_event_value + i));
	}

	return bt_mesh_event_value;
}

char *TwoBytetoOneByte(char *value, uint8_t flag)
{
	char *bt_mesh_event_value = NULL;
	uint8_t cmd_event = 0; //judge is a cmd or event
	uint8_t cmd_event_id = 0;
	uint16_t length = 0;
	//char *ptr = NULL;
	uint8_t device_number = 0;
	uint8_t dev_uuid[16] = {0};
	int i = 0,j = 0,dev_A = 0;
	uint16_t mesh_addr = 0, group_addr = 0;
	uint8_t light_state = 0;
	

	switch(flag){
	case RTW_BT_MESH_CMD_START_PROVISIONER:
		cmd_event = RTW_BT_MESH_EVENT;
		cmd_event_id = RTW_BT_MESH_EVENT_PROVISIONER_STATE;
		length = 1 + 1 + 2 + 1;
		
		//malloc  bt_mesh_event_value
		bt_mesh_event_value = (char *)httpd_malloc(length + 1);
		memset(bt_mesh_event_value,0,length + 1);
		///event 0 byte 
		*bt_mesh_event_value = cmd_event;
		//event id 1byte
		*(bt_mesh_event_value + 1) = cmd_event_id;

		//length 2-3 byte
		*(bt_mesh_event_value + 2) = (length & 0xff00) >> 8;
		*(bt_mesh_event_value + 3) = (length & 0x00ff);
		break;
	case RTW_BT_MESH_CMD_GET_UNPROVISIONED_DEVICE:
		cmd_event = RTW_BT_MESH_EVENT;
		cmd_event_id = RTW_BT_MESH_EVENT_UNPROVISIONED_DEVICE;
		length = 4 + 1 + 20 * 16;
		
		//malloc  bt_mesh_event_value
		bt_mesh_event_value = (char *)httpd_malloc(length + 1);
		memset(bt_mesh_event_value,0,length + 1);
		
		///event 0 byte
		*bt_mesh_event_value = cmd_event;
	
		//event id 1byte
		*(bt_mesh_event_value + 1) = cmd_event_id;
	
		//length 2-3 byte
		*(bt_mesh_event_value + 2) = (length & 0xff00) >> 4;
		*(bt_mesh_event_value + 3) = (length & 0x00ff);
		break;
	case RTW_BT_MESH_CMD_CONNECT_DEVICE:
		cmd_event = RTW_BT_MESH_EVENT;
		cmd_event_id = RTW_BT_MESH_EVENT_DEVICE_CONNECT_INFO;
		device_number = (chartodec(*(value)) << 4 ) + chartodec(*(value + 1));
		length = 4 + 1 + device_number * 20;

		//malloc  bt_mesh_event_value
		bt_mesh_event_value = (char *)httpd_malloc(length + 1);
		memset(bt_mesh_event_value,0,length + 1);
	
		//fill event 
		*bt_mesh_event_value = cmd_event;
		
		//event id 1byte
		*(bt_mesh_event_value + 1) = cmd_event_id;
		
		//length 2-3 byte
		*(bt_mesh_event_value + 2) = (length & 0xff00) >> 8;
		*(bt_mesh_event_value + 3) = (length & 0x00ff);
	
		//fill device number 4 byte
		*(bt_mesh_event_value + 4) = device_number;
		//value point the first mesh addr
		value = value + 2;
		//according to device_numberget the dev_uuid circulatory
		for(i = 0,j = 0;i < (device_number * 32);j ++){	
			*(dev_uuid + j) = (chartodec(*(value + i)) << 4 ) + chartodec(*(value + i + 1)) ;
			if((i + 1 - dev_A) % 31 == 0){
				dev_A = dev_A + 32;
				bt_mesh_cmd_connect_device_pre_handler(dev_uuid);
				memset(dev_uuid,0,16);
				j = -1;
			}
			i = i + 2;
		}	
		break;
	case RTW_BT_MESH_CMD_DELETE_NODE:
		cmd_event = RTW_BT_MESH_EVENT;
		cmd_event_id = RTW_BT_MESH_EVENT_NODE_DELETE_INFO;
		device_number = (chartodec(*(value)) << 4 ) + chartodec(*(value + 1));
		length = 4 + 1 + (2 + 1) * device_number;

		//malloc  bt_mesh_event_value
		bt_mesh_event_value = (char *)httpd_malloc(length + 1);
		memset(bt_mesh_event_value,0,length + 1);
	
		//fill event 
		*bt_mesh_event_value = cmd_event;
		
		//event id 1byte
		*(bt_mesh_event_value + 1) = cmd_event_id;
		
		//length 2-3 byte
		*(bt_mesh_event_value + 2) = (length & 0xff00) >> 8;
		*(bt_mesh_event_value + 3) = (length & 0x00ff);
	
		//fill device number 4 byte
		*(bt_mesh_event_value + 4) = device_number;
		value = value + 2;
		for(i = 0;i < device_number * 4;){
			mesh_addr = (chartodec(*(value + i)) << 12) + (chartodec(*(value + i + 1)) << 8) + (chartodec(*(value + i + 2)) << 4) + (chartodec(*(value + i + 3)));//analynis length filed
			i = i + 4;

			bt_mesh_cmd_delete_node_pre_handler(mesh_addr);
			mesh_addr = 0;
		}
		break;
	case RTW_BT_MESH_CMD_GET_NODE_STATUS:
		cmd_event = RTW_BT_MESH_EVENT;
		cmd_event_id = RTW_BT_MESH_EVENT_NODE_STATUS;
		length = 4 + 1 + 20 * 6;

		//malloc  bt_mesh_event_value
		bt_mesh_event_value = (char *)httpd_malloc(length + 1);
		memset(bt_mesh_event_value,0,length + 1);
	
		//fill event 
		*bt_mesh_event_value = cmd_event;
		
		//event id 1byte
		*(bt_mesh_event_value + 1) = cmd_event_id;
		
		//length 2-3 byte
		*(bt_mesh_event_value + 2) = (length & 0xff00) >> 8;
		*(bt_mesh_event_value + 3) = (length & 0x00ff);
		break;
	case RTW_BT_MESH_CMD_SET_NODE_STATE:
		cmd_event = RTW_BT_MESH_EVENT;
		cmd_event_id = RTW_BT_MESH_EVENT_NODE_STATUS;
		device_number = (chartodec(*(value)) << 4 ) + chartodec(*(value + 1));
		length = 4 + 1 + 20 * 6; 

		//malloc  bt_mesh_event_value
		bt_mesh_event_value = (char *)httpd_malloc(length + 1);
		memset(bt_mesh_event_value,0,length + 1);
	
		//fill event 
		*bt_mesh_event_value = cmd_event;
		
		//event id 1byte
		*(bt_mesh_event_value + 1) = cmd_event_id;
		
		//length 2-3 byte
		*(bt_mesh_event_value + 2) = (length & 0xff00) >> 8;
		*(bt_mesh_event_value + 3) = (length & 0x00ff);
	
		//fill device number 4 byte
		*(bt_mesh_event_value + 4) = device_number;
		value = value + 2;
		for(i = 0;i < device_number * 6;){
			mesh_addr = (chartodec(*(value + i)) << 12) + (chartodec(*(value + i + 1)) << 8) + (chartodec(*(value + i + 2)) << 4) + (chartodec(*(value + i + 3)));
			light_state = (chartodec(*(value + i + 4)) << 4) + chartodec(*(value + i + 5));

			i = i + 6;

			bt_mesh_cmd_set_node_state_pre_handler(mesh_addr,light_state);
			mesh_addr = 0;
			light_state = 0;
		}
		break;
	case RTW_BT_MESH_CMD_SET_NODE_GROUP:
		cmd_event = RTW_BT_MESH_EVENT;
		cmd_event_id = RTW_BT_MESH_EVENT_NODE_STATUS;
		device_number = (chartodec(*(value)) << 4 ) + chartodec(*(value + 1));
		length = 4 + 1 + device_number * 6;

		//malloc  bt_mesh_event_value
		bt_mesh_event_value = (char *)httpd_malloc(length + 1);
		memset(bt_mesh_event_value,0,length + 1);
	
		//fill event 
		*bt_mesh_event_value = cmd_event;
		
		//event id 1byte
		*(bt_mesh_event_value + 1) = cmd_event_id;
		
		//length 2-3 byte
		*(bt_mesh_event_value + 2) = (length & 0xff00) >> 8;
		*(bt_mesh_event_value + 3) = (length & 0x00ff);
	
		//fill device number 4 byte
		*(bt_mesh_event_value + 4) = device_number;
		value = value + 2;
		for(i = 0;i < device_number * 8;){
			mesh_addr = (chartodec(*(value + i)) << 12) + (chartodec(*(value + i + 1)) << 8) + (chartodec(*(value + i + 2)) << 4) + (chartodec(*(value + i + 3)));
			group_addr = (chartodec(*(value + i + 4)) << 12) + (chartodec(*(value + i + 5)) << 8) + (chartodec(*(value + i + 6)) << 4) + (chartodec(*(value + i + 7)));
			i = i + 8;

			bt_mesh_cmd_set_node_group_pre_handler(mesh_addr,group_addr);
			mesh_addr = 0;
			group_addr = 0;
		}
		break;

	default:
		printf("\r\nUnknow cmd id\n");
		break;
	}

	return bt_mesh_event_value;
}

char *ErrorOneBytetoTwoByte(char *value)
{
	char *bt_mesh_event_value = NULL;
	uint16_t length = (*(value + 2) << 8) + (*(value + 3));

	bt_mesh_event_value = (char *)httpd_malloc(2 * length + 1);
	memset(bt_mesh_event_value, 0, 2 * length + 1);

	*bt_mesh_event_value = (*(value) & 0xf0) >> 4; 
	*(bt_mesh_event_value + 1) = (*(value) & 0x0f);

	*(bt_mesh_event_value + 2)= (*(value + 1) & 0xf0) >> 4; 
	*(bt_mesh_event_value + 3) = (*(value + 1) & 0x0f);

	*(bt_mesh_event_value + 4) = ((2 * length) & 0xf000) >> 12;
	*(bt_mesh_event_value + 5) = ((2 * length) & 0x0f00) >> 8;
	*(bt_mesh_event_value + 6) = ((2 * length) & 0x00f0) >> 4;
	*(bt_mesh_event_value + 7) = ((2 * length) & 0x000f);

	*(bt_mesh_event_value + 8) = (*(value + 4) & 0xf0) >> 4;
	*(bt_mesh_event_value + 9) = (*(value + 4) & 0x0f);

	for(int i = 0; i < 2 * length; i ++)
	{
		*(bt_mesh_event_value + i) = dectochar(*(bt_mesh_event_value + i));
	}

	return bt_mesh_event_value;
}

char *ErrorTwoBytetoOneByte(uint8_t flag)
{
	char *bt_mesh_event_value = NULL;
	uint8_t cmd_event = 0;
	uint8_t cmd_event_id = 0;
	uint16_t length = 0;
	uint8_t device_number = 0;

	cmd_event = RTW_BT_MESH_EVENT;

	switch(flag) {
		case RTW_BT_MESH_CMD_GET_UNPROVISIONED_DEVICE:
			cmd_event_id = RTW_BT_MESH_EVENT_UNPROVISIONED_DEVICE;
			break;
		case RTW_BT_MESH_CMD_CONNECT_DEVICE:
			cmd_event_id = RTW_BT_MESH_EVENT_DEVICE_CONNECT_INFO;
			break;
		case RTW_BT_MESH_CMD_DELETE_NODE:
			cmd_event_id = RTW_BT_MESH_EVENT_NODE_DELETE_INFO;
			break;
		case RTW_BT_MESH_CMD_GET_NODE_STATUS:
		case RTW_BT_MESH_CMD_SET_NODE_STATE:
		case RTW_BT_MESH_CMD_SET_NODE_GROUP:
			cmd_event_id = RTW_BT_MESH_EVENT_NODE_STATUS;
			break;
	}

	length = 1 + 1 + 2 + 1;
	device_number = 0xff;

	bt_mesh_event_value = (char *)httpd_malloc(length + 1);
	memset(bt_mesh_event_value, 0, length + 1);

	*bt_mesh_event_value = cmd_event;

	*(bt_mesh_event_value + 1) = cmd_event_id;

	*(bt_mesh_event_value + 2) = (length & 0xff00) >> 4;
	*(bt_mesh_event_value + 3) = (length & 0x00ff);

	*(bt_mesh_event_value + 4) = device_number;

	return bt_mesh_event_value;
}

char * bt_mesh_cmd_start_provisioner_parsing(void)
{
	char *bt_mesh_event_value_one = NULL, *bt_mesh_event_value_two = NULL;

	bt_mesh_event_value_one = TwoBytetoOneByte(NULL, RTW_BT_MESH_CMD_START_PROVISIONER);

	bt_mesh_cmd_start_provisioner_handler((uint8_t *)bt_mesh_event_value_one);

	bt_mesh_event_value_two = OneBytetoTwoByte(bt_mesh_event_value_one, RTW_BT_MESH_EVENT_PROVISIONER_STATE);

	if(bt_mesh_event_value_one)
		httpd_free(bt_mesh_event_value_one);

	return bt_mesh_event_value_two;
}

char *bt_mesh_cmd_get_unprovisioned_device_parsing(void)
{
	char *bt_mesh_event_value_one = NULL, *bt_mesh_event_value_two = NULL;

	if(bt_mesh_priv.provisioner_started_flag == _FALSE) {
		bt_mesh_event_value_one = ErrorTwoBytetoOneByte(RTW_BT_MESH_CMD_GET_UNPROVISIONED_DEVICE);

		bt_mesh_event_value_two = ErrorOneBytetoTwoByte(bt_mesh_event_value_one);
	} else {
		bt_mesh_event_value_one = TwoBytetoOneByte(NULL, RTW_BT_MESH_CMD_GET_UNPROVISIONED_DEVICE);

		bt_mesh_cmd_get_unprovisioned_device_handler((uint8_t *)bt_mesh_event_value_one);

		bt_mesh_event_value_two = OneBytetoTwoByte(bt_mesh_event_value_one, RTW_BT_MESH_EVENT_UNPROVISIONED_DEVICE);
	}

	if(bt_mesh_event_value_one)
		httpd_free(bt_mesh_event_value_one);

	return bt_mesh_event_value_two;
}

char *bt_mesh_cmd_connect_device_parsing(char ** value)
{
	char *bt_mesh_event_value_one = NULL, *bt_mesh_event_value_two = NULL;
	char *ptr = NULL;

	if(bt_mesh_priv.provisioner_started_flag == _FALSE) {
		bt_mesh_event_value_one = ErrorTwoBytetoOneByte(RTW_BT_MESH_CMD_CONNECT_DEVICE);

		bt_mesh_event_value_two = ErrorOneBytetoTwoByte(bt_mesh_event_value_one);
	} else {
		ptr = *value;
		bt_mesh_event_value_one = TwoBytetoOneByte(ptr, RTW_BT_MESH_CMD_CONNECT_DEVICE);

		bt_mesh_cmd_connect_device_handler();

		bt_mesh_cmd_connect_device_post_handler((uint8_t *)bt_mesh_event_value_one);

		bt_mesh_event_value_two = OneBytetoTwoByte(bt_mesh_event_value_one, RTW_BT_MESH_EVENT_DEVICE_CONNECT_INFO);
	}

	if(bt_mesh_event_value_one)
		httpd_free(bt_mesh_event_value_one);

	return bt_mesh_event_value_two;
}

char *bt_mesh_cmd_delete_node_parsing(char ** value)
{
	char *bt_mesh_event_value_one = NULL, *bt_mesh_event_value_two = NULL;
	char *ptr = NULL;

	if(bt_mesh_priv.provisioner_started_flag == _FALSE) {
		bt_mesh_event_value_one = ErrorTwoBytetoOneByte(RTW_BT_MESH_CMD_DELETE_NODE);

		bt_mesh_event_value_two = ErrorOneBytetoTwoByte(bt_mesh_event_value_one);
	} else {
		ptr = *value;
		bt_mesh_event_value_one = TwoBytetoOneByte(ptr, RTW_BT_MESH_CMD_DELETE_NODE);

		bt_mesh_cmd_delete_node_handler();

		bt_mesh_cmd_delete_node_post_handler((uint8_t *)bt_mesh_event_value_one);

		bt_mesh_event_value_two = OneBytetoTwoByte(bt_mesh_event_value_one, RTW_BT_MESH_EVENT_NODE_DELETE_INFO);
	}

	if(bt_mesh_event_value_one)
		httpd_free(bt_mesh_event_value_one);

	return bt_mesh_event_value_two;
}

char *bt_mesh_cmd_get_node_status_parsing(char ** value)
{
	char *bt_mesh_event_value_one = NULL, *bt_mesh_event_value_two = NULL;
	char *ptr = NULL;

	if(bt_mesh_priv.provisioner_started_flag == _FALSE) {
		bt_mesh_event_value_one = ErrorTwoBytetoOneByte(RTW_BT_MESH_CMD_GET_NODE_STATUS);

		bt_mesh_event_value_two = ErrorOneBytetoTwoByte(bt_mesh_event_value_one);
	} else {
		ptr = *value;
		bt_mesh_event_value_one = TwoBytetoOneByte(ptr, RTW_BT_MESH_CMD_GET_NODE_STATUS);

		bt_mesh_cmd_get_node_status_handler((uint8_t *)bt_mesh_event_value_one);

		bt_mesh_event_value_two = OneBytetoTwoByte(bt_mesh_event_value_one, RTW_BT_MESH_EVENT_NODE_STATUS);
	}

	if(bt_mesh_event_value_one)
		httpd_free(bt_mesh_event_value_one);

	return bt_mesh_event_value_two;
}

char *bt_mesh_cmd_set_node_state_parsing(char ** value)
{
	char *bt_mesh_event_value_one = NULL, *bt_mesh_event_value_two = NULL;
	char *ptr = NULL;

	if(bt_mesh_priv.provisioner_started_flag == _FALSE) {
		bt_mesh_event_value_one = ErrorTwoBytetoOneByte(RTW_BT_MESH_CMD_SET_NODE_STATE);

		bt_mesh_event_value_two = ErrorOneBytetoTwoByte(bt_mesh_event_value_one);
	} else {
		ptr = *value;
		bt_mesh_event_value_one = TwoBytetoOneByte(ptr, RTW_BT_MESH_CMD_SET_NODE_STATE);

		bt_mesh_cmd_set_node_state_handler();

		bt_mesh_cmd_set_node_state_post_handler((uint8_t *)bt_mesh_event_value_one);

		bt_mesh_event_value_two = OneBytetoTwoByte(bt_mesh_event_value_one, RTW_BT_MESH_EVENT_NODE_STATUS);
	}

	if(bt_mesh_event_value_one)
		httpd_free(bt_mesh_event_value_one);

	return bt_mesh_event_value_two;
}

char *bt_mesh_cmd_set_node_group_parsing(char ** value)
{
	char *bt_mesh_event_value_one = NULL, *bt_mesh_event_value_two = NULL;
	char *ptr = NULL;

	if(bt_mesh_priv.provisioner_started_flag == _FALSE) {
		bt_mesh_event_value_one = ErrorTwoBytetoOneByte(RTW_BT_MESH_CMD_SET_NODE_GROUP);

		bt_mesh_event_value_two = ErrorOneBytetoTwoByte(bt_mesh_event_value_one);
	} else {
		ptr = *value;
		bt_mesh_event_value_one = TwoBytetoOneByte(ptr, RTW_BT_MESH_CMD_SET_NODE_GROUP);

		bt_mesh_cmd_set_node_group_handler();

		bt_mesh_cmd_set_node_group_post_handler((uint8_t *)bt_mesh_event_value_one);

		bt_mesh_event_value_two = OneBytetoTwoByte(bt_mesh_event_value_one, RTW_BT_MESH_EVENT_NODE_STATUS);
	}

	if(bt_mesh_event_value_one)
		httpd_free(bt_mesh_event_value_one);

	return bt_mesh_event_value_two;
}

void httpd_get_cb(struct httpd_conn *conn)
{
	stop_list_flushing_flag = 1;
	// GET /test_post
	if(httpd_request_is_method(conn, "GET")) {
		char *bt_mesh_cmd_value = NULL,*ptr = NULL; //content after "bt_mesh="
		char *bt_mesh_event_value = NULL; //response to app
		int cmd_event; //judge is a cmd or event
		int cmd_event_id;
		int colength_content = 0;
		int length = 0;
		// get 'bt_mesh' in query string
		if(httpd_request_get_query_key(conn, "bt_mesh", &bt_mesh_cmd_value) != -1){
#if 1
			printf("\n\rPrintf the httpd request there:");
			printf("%s",bt_mesh_cmd_value);
			printf("\n\r");
#endif
			///original address of bt_mesh_cmd_value
			ptr = bt_mesh_cmd_value;
			//cmd event 0-1 byte
			cmd_event = (chartodec(*ptr) << 4) + chartodec(*(ptr + 1));
			//cmd id    2-3 byte
			cmd_event_id = (chartodec(*(ptr + 2)) << 4) + chartodec(*(ptr + 3));
			///length   4-7 byte
			length = (chartodec(*(ptr + 4)) << 12) + (chartodec(*(ptr + 5)) << 8) + (chartodec(*(ptr + 6)) << 4) + (chartodec(*(ptr + 7)));//analynis length filed
			colength_content = strlen(ptr); //the length of content
			if(colength_content != length){
				printf("\r\nERROR: The Length is not fit\n");
				httpd_response_bad_request(conn, "Bad Request - The Length is not fit");
				return;
			}
			printf("\r\n %s() cmd_event=%d, cmd_event_id=%d, line=%d",__func__,cmd_event,cmd_event_id,__LINE__);
			/////content filed--point to length's next byte 
			ptr = ptr + 8;	
		///analysis CMD field
			switch (cmd_event){
				case RTW_BT_MESH_CMD://is a cmd from app to provisioner
				switch (cmd_event_id){
					case RTW_BT_MESH_CMD_START_PROVISIONER:
						bt_mesh_event_value = bt_mesh_cmd_start_provisioner_parsing();
						break;
					case RTW_BT_MESH_CMD_GET_UNPROVISIONED_DEVICE:
						bt_mesh_event_value = bt_mesh_cmd_get_unprovisioned_device_parsing();
						break;
					case RTW_BT_MESH_CMD_CONNECT_DEVICE:
						bt_mesh_event_value = bt_mesh_cmd_connect_device_parsing(&ptr);
						break;
					case RTW_BT_MESH_CMD_DELETE_NODE:
						bt_mesh_event_value = bt_mesh_cmd_delete_node_parsing(&ptr);
						break;
					case RTW_BT_MESH_CMD_GET_NODE_STATUS:
						bt_mesh_event_value = bt_mesh_cmd_get_node_status_parsing(&ptr);
						break;
					case RTW_BT_MESH_CMD_SET_NODE_STATE:
						bt_mesh_event_value = bt_mesh_cmd_set_node_state_parsing(&ptr);
						break;
					case RTW_BT_MESH_CMD_SET_NODE_GROUP:
						bt_mesh_event_value = bt_mesh_cmd_set_node_group_parsing(&ptr);
						break;
					default:
						printf("\r\nUnknow cmd id \n");
				}
		}			
#if 1
			printf("\n\rPrintf the httpd response there:");
			printf("%s",bt_mesh_event_value);
			printf("\n\r");
#endif
///write httpd response to app			
			httpd_response_write_header_start(conn, "200 OK", "text/plain", 0);
			httpd_response_write_header(conn, "Connection", "close");
			httpd_response_write_header_finish(conn);
			httpd_response_write_data(conn, (uint8_t*)"bt_mesh= ", strlen("bt_mesh= "));
			httpd_response_write_data(conn, (uint8_t*)bt_mesh_event_value, strlen(bt_mesh_event_value));
	
		}
		else {
			// HTTP/1.1 400 Bad Request
			httpd_response_bad_request(conn, "Bad Request - bt_mesh is not in query string");
		}
		if(bt_mesh_cmd_value)
			httpd_free(bt_mesh_cmd_value);
		if(bt_mesh_event_value)
			httpd_free(bt_mesh_event_value);
	}
	else {
		// HTTP/1.1 405 Method Not Allowed
		httpd_response_method_not_allowed(conn, NULL);
	}
	stop_list_flushing_flag = 0;
	httpd_conn_close(conn);
}

void httpd_demo_init_thread(void *param)
{
	/* To avoid gcc warnings */
	( void ) param;
#if USE_HTTPS
#if (HTTPD_USE_TLS == HTTPD_TLS_POLARSSL)
	if(httpd_setup_cert(test_srv_crt, test_srv_key, test_ca_crt) != 0) {
#elif (HTTPD_USE_TLS == HTTPD_TLS_MBEDTLS)
	if(httpd_setup_cert(mbedtls_test_srv_crt, mbedtls_test_srv_key, mbedtls_test_ca_crt) != 0) {
#endif
		printf("\nERROR: httpd_setup_cert\n");
		goto exit;
	}
#endif
	httpd_reg_page_callback("/test_get", httpd_get_cb);
#if USE_HTTPS
	if(httpd_start(443, 5, 4096, HTTPD_THREAD_SINGLE, HTTPD_SECURE_TLS) != 0) {
#else
	if(httpd_start(80, 5, 4096, HTTPD_THREAD_SINGLE, HTTPD_SECURE_NONE) != 0) {
#endif
		printf("ERROR: httpd_start");
		httpd_clear_page_callbacks();
	}
    if (bt_mesh_demo_start_drv_thread()) {
        printf("ERROR: bt_mesh_demo_start_drv_thread fail !");
    }
#if USE_HTTPS
exit:
#endif
	os_task_delete(NULL);
}

