/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     google_seamless.c
* @brief    google seamless setup ble service source file.
* @details  Interfaces to access google seamless setup service.
* @author
* @date
* @version  v1.0
*********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_GOOGLE_SEAMLESS) && CONFIG_BT_GOOGLE_SEAMLESS
#include "stdint.h"
#include "stddef.h"
#include "string.h"
#include "FreeRTOS.h"
#include "Timers.h"
#include "task.h"
#include "os_mem.h"
#include "app_msg.h"
#include "trace_app.h"
#include "ble_google_seamless_app.h"
#include "profile_server.h"
#include "google_seamless.h"
#include "os_msg.h"
#include "app_flags.h"

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
#define GATT_UUID_LIGHT									0xB00B
#define GATT_UUID_CHAR_PROVISON_STATE					0xB005
#define GATT_UUID_CHAR_LIGHT_ON_STATE					0xB006

#define GOOGLE_SEAMLESS_CHAR_V5_WRITE_INDEX 			0x02
#define GOOGLE_SEAMLESS_CHAR_V6_WRITE_INDEX				0x05
#define GOOGLE_SEAMLESS_CHAR_V6_NOTIFY_INDEX			0x05
#define GOOGLE_SEAMLESS_CHAR_NOTIFY_CCCD_V6_INDEX		(GOOGLE_SEAMLESS_CHAR_V6_NOTIFY_INDEX + 1)

/**<  Value of google seamless setup read characteristic. */
static uint16_t google_provision_char_read_len = 1;
static uint8_t google_provision_char_read_value[GOOGLE_SEAMLESS_READ_MAX_LEN];
static uint16_t google_on_char_read_len = 1;
static uint8_t google_on_char_read_value[GOOGLE_SEAMLESS_READ_MAX_LEN];
/**<  Value of google seamless setup provision status. */
bool google_seamless_provisioned = false;
/**<  Value of google seamless setup queue handler. */
extern void *google_seamless_evt_queue_handle;
extern void *google_seamless_io_queue_handle;
/**<  Function pointer used to send event to application from pxp profile. */
/**<  Initiated in PXP_AddService. */
static P_FUN_SERVER_GENERAL_CB pfn_google_seamless_cb = NULL;

/**< @brief  profile/service definition.  */
static const T_ATTRIB_APPL google_seamless_attr_tbl[] =
{
    /*----------------- Google light Service -------------------*/
    /* <<Primary Service>>, .. */
    {
        (ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_LE),   /* flags     */
        {                                           /* type_value */
            LO_WORD(GATT_UUID_PRIMARY_SERVICE),
            HI_WORD(GATT_UUID_PRIMARY_SERVICE),
            LO_WORD(GATT_UUID_LIGHT),              /* service UUID */
            HI_WORD(GATT_UUID_LIGHT)
        },
        UUID_16BIT_SIZE,                            /* bValueLen     */
        NULL,                                       /* p_value_context */
        GATT_PERM_READ                              /* permissions  */
    },
  
    /* <<Characteristic>>, demo for provision */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                          /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_WRITE_NO_RSP | GATT_CHAR_PROP_WRITE,  /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                         /* type_value */
            LO_WORD(GATT_UUID_CHAR_PROVISON_STATE),
            HI_WORD(GATT_UUID_CHAR_PROVISON_STATE)
        },
        0,                                          /* bValueLen */
        NULL,
		GATT_PERM_WRITE                             /* permissions */
    },
    /* client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,                 /* flags */
        {                                          /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* permissions */
    },

    /* <<Characteristic>>, demo for light on/off */
    {
        ATTRIB_FLAG_VALUE_INCL,                     /* flags */
        {                                          /* type_value */
            LO_WORD(GATT_UUID_CHARACTERISTIC),
            HI_WORD(GATT_UUID_CHARACTERISTIC),
            GATT_CHAR_PROP_NOTIFY | GATT_CHAR_PROP_WRITE_NO_RSP | GATT_CHAR_PROP_WRITE,  /* characteristic properties */
            /* characteristic UUID not needed here, is UUID of next attrib. */
        },
        1,                                          /* bValueLen */
        NULL,
        GATT_PERM_READ                              /* permissions */
    },
    {
        ATTRIB_FLAG_VALUE_APPL,                     /* flags */
        {                                         /* type_value */
            LO_WORD(GATT_UUID_CHAR_LIGHT_ON_STATE),
            HI_WORD(GATT_UUID_CHAR_LIGHT_ON_STATE)
        },
        0,                                          /* bValueLen */
        NULL,
		(GATT_PERM_NOTIF_IND | GATT_PERM_WRITE)                               /* permissions */
    },
    /* client characteristic configuration */
    {
        ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL,                 /* flags */
        {                                          /* type_value */
            LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG),
            /* NOTE: this value has an instantiation for each client, a write to */
            /* this attribute does not modify this default value:                */
            LO_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT), /* client char. config. bit field */
            HI_WORD(GATT_CLIENT_CHAR_CONFIG_DEFAULT)
        },
        2,                                          /* bValueLen */
        NULL,
        (GATT_PERM_READ | GATT_PERM_WRITE)          /* permissions */
    },
};
/**< @brief  google seamless setup service size definition.  */
static const uint16_t google_seamless_attr_tbl_size = sizeof(google_seamless_attr_tbl);

/**
 * @brief send notification message
 *
 * @param void
 * @return void
*/
bool  google_seamless_send_msg(GOOGLE_SEAMLESS_NOTIFICATION_PARAM* param)
{
	uint8_t event = EVENT_IO_TO_APP;
	T_IO_MSG io_msg;
	bool ret = true;
	io_msg.type = IO_MSG_TYPE_QDECODE;
	io_msg.subtype = GOOGLE_SEAMLESS_MSG_SEND_INDICATION;
	io_msg.u.buf = (void*)param;

	if (google_seamless_evt_queue_handle != NULL && google_seamless_io_queue_handle != NULL) {
		if (os_msg_send(google_seamless_io_queue_handle, &io_msg, 0) == false) {
			printf("bt at cmd send msg fail: subtype 0x%x", io_msg.subtype);
			return false;
		} else if (os_msg_send(google_seamless_evt_queue_handle, &event, 0) == false) {
			printf("bt at cmd send event fail: subtype 0x%x", io_msg.subtype);
			return false;
		}
		printf("notification has been send\n\r");
	}
	else
	{
		return false;
	}
	return ret;
}

/**
 * @brief check whether write value match the value in google firebase. if match, send back notification value to Google home device.
 *
 * @param len				Receive write value length
 * @param p_value	        Receive write value.
 * @return void
*/
bool google_seamless_edit_notify_arg( uint16_t len, uint8_t * p_value)
{
	int argc = 0;
	char *argv[20];
	static const char pcTimerName[] = "google update status";
	static TimerHandle_t *pp_handle = NULL;
	BaseType_t xTimerStarted = pdFALSE;
	bool receive_match_word  = false;
	GOOGLE_SEAMLESS_NOTIFICATION_PARAM *param;
	param = (GOOGLE_SEAMLESS_NOTIFICATION_PARAM *)os_mem_alloc(0,sizeof(GOOGLE_SEAMLESS_NOTIFICATION_PARAM));
	param->val = (char *)os_mem_alloc(0,google_on_char_read_len*sizeof(char));

	if((param == NULL)||(param->val == NULL))
	{
		return false;
	}
	param->conn_id = 0;//we only connect to 1 master
	param->srv_id = 1;// only 1 profile available in this example
	param->attrib_index = GOOGLE_SEAMLESS_CHAR_V6_NOTIFY_INDEX;
	param->type = 1;
	param->len = google_on_char_read_len;

	if((google_on_char_read_len == 2)&&(strncmp((char *)p_value,"on",2)==0))
	{
		strcpy(param->val,"on");
		printf("Google light is ON\n\r");
		receive_match_word = true;
	}
	else if ((google_on_char_read_len==3)&&(strncmp((char *)p_value,"off",3)==0))
	{
		strcpy(param->val,"off");
		printf("Google light is OFF\n\r");
		receive_match_word = true;
	}

	if(receive_match_word == true)
	{
		if(google_seamless_send_msg(param)!=true)
		{
			os_mem_free(param->val);
			os_mem_free(param);
			return false;
		}
	}
	else
	{
		printf( " ERROR: wrong command has been received.\r\n" );
	}
	return true;
}
/**
 * @brief update the provision status
 *
 * @param len				Receive write value length
 * @param p_value	        Receive write value.
 * @return void
*/
void google_seamless_write_provision_status(uint8_t * p_value)
{
	if(strncmp((char *)p_value,"{true}",6)==0)
	{
		google_seamless_provisioned = true;
		printf("Google light is provisioned\n\r");
	}
	else
	{
		google_seamless_provisioned = false;
		printf("Google light is un-provision\n\r");
	}
}

/**
 * @brief       Set a google seamless setup service parameter.
 *
 *              NOTE: You can call this function with a google seamless setup service parameter type and
 *              it will set thegoogle seamless setup service parameter.  Google seamless setup service
 *              parameters are defined in @ref T_GOOGLE_SEAMLESS_PARAM_TYPE.
 *              If the "len" field sets to the size of a "uint16_t" ,the "p_value" field must point to
 *              a data with type of "uint16_t".
 *
 * @param[in]   param_type   google seamless setup service parameter type: @ref T_GOOGLE_SEAMLESS_PARAM_TYPE
 * @param[in]   length       Length of data to write
 * @param[in]   p_value Pointer to data to write.  This is dependent on
 *                      the parameter type and WILL be cast to the appropriate
 *                      data type (For example: if data type of param is uint16_t, p_value will be cast to
 *                      pointer of uint16_t).
 *
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t *p_value;
        google_seamless_set_parameter(GOOGLE_SEAMLESS_BLE_SERVICE_PARAM_V5_READ_CHAR_VAL, 1, &p_value);
    }
 * \endcode
 */
bool google_seamless_set_parameter(T_GOOGLE_SEAMLESS_PARAM_TYPE param_type, uint8_t length, uint8_t *p_value)
{
	bool ret = true;

	switch (param_type)
	{
		default:
		{
			ret = false;
			APP_PRINT_ERROR0("bas_set_parameter failed");
	        break;
		}
		case GOOGLE_SEAMLESS_BLE_SERVICE_PARAM_V5_WRITE_CHAR_VAL:
		{
			memset(google_provision_char_read_value, 0x00, GOOGLE_SEAMLESS_READ_MAX_LEN);
			memcpy(google_provision_char_read_value, p_value, length);
			google_provision_char_read_len = length;
			google_seamless_write_provision_status(p_value);
			break;
		}
		case GOOGLE_SEAMLESS_BLE_SERVICE_PARAM_V6_WRITE_CHAR_VAL:
		{
			memset(google_on_char_read_value, 0, GOOGLE_SEAMLESS_READ_MAX_LEN);
			memcpy(google_on_char_read_value, p_value, length);
			google_on_char_read_len = length;
			if(google_seamless_edit_notify_arg(length,p_value) != true)
			{
				printf("Send notification fail");
			}
			break;
		}
	}
	return ret;
}


void google_seamless_write_post_callback(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index,
							uint16_t length, uint8_t *p_value)
{
	(void)p_value;
	APP_PRINT_INFO4("google_seamless_write_post_callback: conn_id %d, service_id %d, attrib_index 0x%x, length %d",
					conn_id, service_id, attrib_index, length);
}
/**
 * @brief write characteristic data from service.
 *
 * @param conn_id
 * @param service_id        ServiceID to be written.
 * @param attrib_index      Attribute index of characteristic.
 * @param length            length of value to be written.
 * @param p_value           value to be written.
 * @return Profile procedure result
*/
T_APP_RESULT google_seamless_attr_write_cb(uint8_t conn_id, T_SERVER_ID service_id,
											uint16_t attrib_index, T_WRITE_TYPE write_type, uint16_t length, uint8_t *p_value,
											P_FUN_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
	T_GOOGLE_SEAMLESS_CALLBACK_DATA callback_data;
	T_APP_RESULT  cause = APP_RESULT_SUCCESS;
	APP_PRINT_INFO1("google_seamless_attr_write_cb write_type = 0x%x", write_type);
	*p_write_ind_post_proc = google_seamless_write_post_callback;
	if (GOOGLE_SEAMLESS_CHAR_V5_WRITE_INDEX == attrib_index)
	{
		/* Make sure written value size is valid. */
		if (p_value == NULL)
		{
			cause  = APP_RESULT_INVALID_VALUE_SIZE;
		}
		else
		{
			/* Notify Application. */
			callback_data.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;
			callback_data.conn_id  = conn_id;
			callback_data.msg_data.write.opcode = GOOGLE_SEAMLESS_WRITE_V5;
			callback_data.msg_data.write.write_type = write_type;
			callback_data.msg_data.write.len = length;
			callback_data.msg_data.write.p_value = p_value;

			if (pfn_google_seamless_cb)
			{
				pfn_google_seamless_cb(service_id, (void *)&callback_data);
			}
		}
	}
    else if (GOOGLE_SEAMLESS_CHAR_V6_WRITE_INDEX == attrib_index)
	{
		/* Make sure written value size is valid. */
		if (p_value == NULL)
		{
			cause  = APP_RESULT_INVALID_VALUE_SIZE;
		}
		else
		{
			/* Notify Application. */
			callback_data.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;
			callback_data.conn_id  = conn_id;
			callback_data.msg_data.write.opcode = GOOGLE_SEAMLESS_WRITE_V6;
			callback_data.msg_data.write.write_type = write_type;
			callback_data.msg_data.write.len = length;
			callback_data.msg_data.write.p_value = p_value;

			if (pfn_google_seamless_cb)
			{
				pfn_google_seamless_cb(service_id, (void *)&callback_data);
			}
		}
	}
	else{
		APP_PRINT_ERROR2("simp_ble_service_attr_write_cb Error: attrib_index 0x%x, length %d",
						 attrib_index,
						 length);
		cause = APP_RESULT_ATTR_NOT_FOUND;
	}
    return cause;
}
/**
 * @brief update CCCD bits from stack.
 *
 * @param conn_id           Connection ID.
 * @param service_id        Service ID.
 * @param index             Attribute index of characteristic data.
 * @param ccc_bits          CCCD bits from stack.
 * @return None
*/
void google_seamless_cccd_update_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t index, uint16_t ccc_bits)
{
	T_GOOGLE_SEAMLESS_CALLBACK_DATA callback_data;
	callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;
	callback_data.conn_id = conn_id;
	bool handle = true;
	APP_PRINT_INFO2("google_seamless_cccd_update_cb index = %d ccc_bits %x", index, ccc_bits);
	switch (index)
	{
		case GOOGLE_SEAMLESS_CHAR_NOTIFY_CCCD_V6_INDEX:
		{
			if (ccc_bits & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
			{
				// Enable Notification
				callback_data.msg_data.notification_indification_index = GOOGLE_SEAMLESS_NOTIFY_INDICATE_V6_ENABLE;
			}
			else
			{
				// Disable Notification
				callback_data.msg_data.notification_indification_index = GOOGLE_SEAMLESS_NOTIFY_INDICATE_V6_DISABLE;
			}
			handle = true;
			break;
		}
		default:
		{
			handle = false;
			break;
		}

	}

	if (pfn_google_seamless_cb && (handle == true))
	{
		pfn_google_seamless_cb(service_id, (void *)&callback_data);
	}

	return;
}

/**
 * @brief BAS Service Callbacks.
*/
const T_FUN_GATT_SERVICE_CBS google_seamless_cbs =
{
	NULL,
	google_seamless_attr_write_cb, // Write callback function pointer
	google_seamless_cccd_update_cb  // CCCD update callback function pointer
};

/**
  * @brief       Add google seamless setup service to the BLE stack database.
  *
  *
  * @param[in]   p_func  Callback when service attribute was read, write or cccd update.
  * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
  * @retval 0xFF Operation failure.
  * @retval Others Service id assigned by stack.
  *
  * <b>Example usage</b>
  * \code{.c}
     void profile_init()
     {
         server_init(1);
         google_seamless_srv_id = google_seamless_add_service(app_handle_profile_message);
     }
  * \endcode
  */
T_SERVER_ID google_seamless_add_service(void *p_func)
{
	T_SERVER_ID service_id;
	if (false == server_add_service(&service_id,
									(uint8_t *)google_seamless_attr_tbl,
									google_seamless_attr_tbl_size,
									google_seamless_cbs))
	{
		APP_PRINT_ERROR1("google_seamless_add_service: service_id %d", service_id);
		service_id = 0xff;
	}
	pfn_google_seamless_cb = (P_FUN_SERVER_GENERAL_CB)p_func;
	return service_id;
}

#endif
