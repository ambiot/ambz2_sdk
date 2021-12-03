#include <gap.h>
#include <os_mem.h>
#include <profile_server.h>
#include "bt_harmony_adapter_peripheral_app.h"
#include "ohos_bt_gatt_server.h"
#include "ohos_bt_gatt_client.h"
#include <string.h>
#include "platform_stdlib.h"

static uint8_t bt_harmony_char_read_value[HARMONY_READ_MAX_LEN];
static unsigned int bt_harmony_char_read_len = 0;
static uint8_t bt_harmony_char_write_value[HARMONY_WRITE_MAX_LEN];
BHA_SRV_DATABASE ble_srv_database[12] = {0};
uint8_t ble_srv_count = 0;

BHA_SERVICE_INFO bt_harmony_adapter_srvs_head;
BHA_SERVICE_INFO *bt_harmony_adapter_srv_p = &bt_harmony_adapter_srvs_head;
uint8_t bt_harmony_adapter_srvs_num = 0;
static P_FUN_SERVER_GENERAL_CB bt_harmony_service_cb = NULL;

bool bt_harmony_adapter_send_indication(uint8_t conn_id, uint8_t service_id, uint8_t handle,
										uint8_t *p_value, uint16_t length, bool type)
{
	if (p_value == NULL) {
		return false;
	}
	printf("\r\n[%s] service_id %d index %d", __FUNCTION__, service_id, handle);
	T_GATT_PDU_TYPE pdu_type;
	if (type == 1) {
		pdu_type = GATT_PDU_TYPE_INDICATION;
	} else {
		pdu_type = GATT_PDU_TYPE_NOTIFICATION;
	}
	return server_send_data(conn_id, service_id, handle, p_value, length, pdu_type);
}

void bt_harmony_adapter_cccd_update_cb(uint8_t conn_id, T_SERVER_ID service_id, uint16_t attrib_index,
									   uint16_t cccbits)
{
	T_HARMONY_CALLBACK_DATA callback_data;
	callback_data.msg_type = SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION;
	callback_data.conn_id = conn_id;
	callback_data.srv_id = service_id;
	callback_data.msg_data.cccd.attr_index = attrib_index;
	callback_data.msg_data.cccd.ccc_val = cccbits;

	/* Notify Application. */
	if (bt_harmony_service_cb) {
		bt_harmony_service_cb(service_id, (void *)&callback_data);
	}
}

T_APP_RESULT bt_harmony_adapter_attr_write_cb(uint8_t conn_id, T_SERVER_ID service_id,
		uint16_t attrib_index, T_WRITE_TYPE write_type, uint16_t length, uint8_t *p_value,
		P_FUN_WRITE_IND_POST_PROC *p_write_ind_post_proc)
{
	T_APP_RESULT  cause = APP_RESULT_SUCCESS;
	printf("\r\n[%s] service_id %d index 0x%x", __FUNCTION__, service_id, attrib_index);
	T_HARMONY_CALLBACK_DATA callback_data;

	/* Make sure written value size is valid. */
	if (p_value == NULL) {
		cause  = APP_RESULT_INVALID_VALUE_SIZE;
	} else {
		/* Notify Application. */
		callback_data.msg_type = SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE;
		callback_data.conn_id  = conn_id;
		callback_data.srv_id = service_id;
		callback_data.msg_data.write.write_type = write_type;
		memcpy(bt_harmony_char_write_value, p_value, length);
		callback_data.msg_data.write.p_value = bt_harmony_char_write_value;
		callback_data.msg_data.write.len = length;

		BHA_SERVICE_INFO *p = bt_harmony_adapter_srvs_head.next;
		while (p) {
			if (p->srvId == service_id) {
				break;
			} else {
				p = p->next;
			}
		}
		if (p) {
			callback_data.msg_data.write.write_cb = (p->cbInfo[attrib_index]).func.write;
		} else {
			callback_data.msg_data.write.write_cb = NULL;
		}
		if (bt_harmony_service_cb) {
			bt_harmony_service_cb(service_id, (void *)&callback_data);
		}
	}

	return (cause);
}


T_APP_RESULT  bt_harmony_adapter_attr_read_cb(uint8_t conn_id, T_SERVER_ID service_id,
		uint16_t attrib_index, uint16_t offset, uint16_t *p_length, uint8_t **pp_value)
{
	(void)offset;
	T_APP_RESULT  cause  = APP_RESULT_SUCCESS;
	printf("\r\n[%s] service_id %d index 0x%x", __FUNCTION__, service_id, attrib_index);
	bt_harmony_char_read_len = HARMONY_READ_MAX_LEN;

	T_HARMONY_CALLBACK_DATA callback_data;
	callback_data.msg_type = SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE;
	callback_data.conn_id = conn_id;
	callback_data.srv_id = service_id;
	callback_data.msg_data.read.p_value = bt_harmony_char_read_value;
	callback_data.msg_data.read.p_len = &bt_harmony_char_read_len;
	BHA_SERVICE_INFO *p = bt_harmony_adapter_srvs_head.next;
	while (p) {
		if (p->srvId == service_id) {
			break;
		} else {
			p = p->next;
		}
	}
	if (p) {
		callback_data.msg_data.read.read_cb = (p->cbInfo[attrib_index]).func.read;
	} else {
		callback_data.msg_data.read.read_cb = NULL;
	}

	if (bt_harmony_service_cb) {
		bt_harmony_service_cb(service_id, (void *)&callback_data);
	}

	*pp_value = bt_harmony_char_read_value;
	*p_length = bt_harmony_char_read_len;

	return (cause);
}

const T_FUN_GATT_SERVICE_CBS bt_harmony_adapter_service_cbs = {
	bt_harmony_adapter_attr_read_cb,   /* Read callback function pointer */
	bt_harmony_adapter_attr_write_cb,  /* Write callback function pointer */
	bt_harmony_adapter_cccd_update_cb  /* CCCD update callback function pointer */
};

T_SERVER_ID bt_harmony_adapter_add_service(BHA_SERVICE_INFO *service_info, void *p_func)
{
	if (service_info == NULL || service_info->att_tbl == NULL) {
		return 0xff;
	}
	if (false == server_add_service(&(service_info->srvId),
									(uint8_t *) service_info->att_tbl,
									(service_info->att_num) * sizeof(T_ATTRIB_APPL),
									bt_harmony_adapter_service_cbs)) {
		printf("\r\n[%s] add service fail", __FUNCTION__);
		os_mem_free(service_info->att_tbl);
		os_mem_free(service_info->cbInfo);
		BHA_SERVICE_INFO *pnext = service_info->next;
		BHA_SERVICE_INFO *p_srv = &bt_harmony_adapter_srvs_head;
		while (p_srv) {
			if (p_srv->next == service_info) {
				p_srv->next = pnext;
				if (pnext == NULL) {
					bt_harmony_adapter_srv_p = p_srv->next;
				}
				break;
			} else {
				p_srv = p_srv->next;
			}
		}
		os_mem_free(service_info);
		return 0xff;
	} else {
		printf("\r\n[%s] add service %d success", __FUNCTION__, service_info->srvId);
		bt_harmony_adapter_srvs_num++;
		service_info->status = BHA_SERVICE_REG_SUCCESS;
	}

	if (bt_harmony_service_cb == NULL) {
		bt_harmony_service_cb = (P_FUN_SERVER_GENERAL_CB) p_func;
	}
	return service_info->srvId;
}

bool setup_ble_srv_info(uint8_t service_index, uint8_t att_handle, uint16_t char_index)
{
	if (bt_harmony_adapter_srv_p == &bt_harmony_adapter_srvs_head) {
		ble_srv_database[service_index].start_handle = 1;    //first service, handle start from 0x1
	} else {
		ble_srv_database[service_index].start_handle = bt_harmony_adapter_srv_p->start_handle + bt_harmony_adapter_srv_p->att_num;
	}
	ble_srv_database[service_index].srv_id = service_index;
	return true;
}
bool setup_ble_char_info(uint8_t service_index, BleGattAttr *attr, uint16_t att_handle, uint16_t char_index)
{
	ble_srv_database[service_index].rela_atthandle[char_index] = att_handle;
	return true;
}
uint32_t switch_perm(uint8_t perm)
{
	uint32_t permission = 0x0000;
	if (perm & 0x01) {
		permission = permission | GATT_PERM_READ;
	} else if (perm & 0x02) {
		permission = permission | GATT_PERM_READ_AUTHEN_REQ;
	} else if (perm & 0x04) {
		permission = permission | GATT_PERM_READ_AUTHEN_MITM_REQ;
	}

	if (perm & 0x10) {
		permission = permission | GATT_PERM_WRITE;
	} else if (perm & 0x20) {
		permission = permission | GATT_PERM_WRITE_AUTHEN_REQ;
	} else if (perm & 0x40) {
		permission = permission | GATT_PERM_WRITE_AUTHEN_MITM_REQ;
	} else if (perm & 0x80) {
		permission = permission | GATT_PERM_WRITE_AUTHOR_REQ;
	}
	return permission;
}

static int setup_ble_char_usd_attr(T_ATTRIB_APPL *attr, uint8_t *val, uint8_t len, uint8_t perm)
{
	attr->permissions = switch_perm(perm);
	attr->flags = ATTRIB_FLAG_VOID | ATTRIB_FLAG_ASCII_Z;
	attr->type_value[0] = LO_WORD(GATT_UUID_CHAR_USER_DESCR);
	attr->type_value[1] = HI_WORD(GATT_UUID_CHAR_USER_DESCR);
	attr->value_len = len;
	attr->p_value_context = os_mem_alloc(0, attr->value_len);
	memset(attr->p_value_context, 0, attr->value_len);
	memcpy(attr->p_value_context, (void *)val, attr->value_len);
	attr->permissions = switch_perm(perm);
	return 0;
}

static int setup_ble_char_ccc_attr(T_ATTRIB_APPL *attr)
{
	attr->flags = ATTRIB_FLAG_VALUE_INCL | ATTRIB_FLAG_CCCD_APPL;
	attr->type_value[0] = LO_WORD(GATT_UUID_CHAR_CLIENT_CONFIG);
	attr->type_value[1] = HI_WORD(GATT_UUID_CHAR_CLIENT_CONFIG);
	attr->type_value[2] = 0;
	attr->type_value[3] = 0;
	attr->p_value_context = NULL;
	attr->value_len = 2;
	attr->permissions = GATT_PERM_READ | GATT_PERM_WRITE;
	return 0;
}

static int setup_ble_char_val_attr(T_ATTRIB_APPL *attr, uint8_t *uuid, UuidType uuid_type, uint8_t perm)
{
	attr->permissions = switch_perm(perm);
	if (uuid_type == OHOS_UUID_TYPE_128_BIT) {
		attr->flags = ATTRIB_FLAG_UUID_128BIT | ATTRIB_FLAG_VALUE_APPL;
		memcpy(attr->type_value, uuid, 16);
	} else if (uuid_type == OHOS_UUID_TYPE_16_BIT) {
		attr->flags = ATTRIB_FLAG_VALUE_APPL;
		attr->type_value[0] = uuid[0];
		attr->type_value[1] = uuid[1];
	}
	attr->value_len = 0;
	attr->p_value_context = NULL;
	return 0;
}

static int setup_ble_char_dec_attr(T_ATTRIB_APPL *attr, uint8_t prop)
{
	attr->flags = ATTRIB_FLAG_VALUE_INCL;
	attr->type_value[0] = LO_WORD(GATT_UUID_CHARACTERISTIC);
	attr->type_value[1] = HI_WORD(GATT_UUID_CHARACTERISTIC);
	attr->type_value[2] = prop;
	attr->value_len = 1;
	attr->p_value_context = NULL;
	attr->permissions = GATT_PERM_READ;
	return 0;
}

static int setup_ble_serv_dec_attr(T_ATTRIB_APPL *attr, uint8_t *uuid, UuidType uuid_type)
{
	attr->type_value[0] = LO_WORD(GATT_UUID_PRIMARY_SERVICE);       /* type */
	attr->type_value[1] = HI_WORD(GATT_UUID_PRIMARY_SERVICE);
	if (uuid_type == OHOS_UUID_TYPE_128_BIT) {
		attr->flags = ATTRIB_FLAG_LE | ATTRIB_FLAG_VOID;
		attr->value_len = 16;
		attr->p_value_context = os_mem_alloc(0, attr->value_len);
		memset(attr->p_value_context, 0, attr->value_len);
		memcpy(attr->p_value_context, (void *)uuid, attr->value_len);
	} else if (uuid_type == OHOS_UUID_TYPE_16_BIT) {
		attr->flags = ATTRIB_FLAG_LE | ATTRIB_FLAG_VALUE_INCL;
		attr->type_value[2] = uuid[0];        /* value */
		attr->type_value[3] = uuid[1];
		attr->p_value_context = NULL;
		attr->value_len = 2;
	}
	attr->permissions = GATT_PERM_READ;
	return 0;
}

BHA_SERVICE_INFO *bt_harmony_adapter_parse_srv_tbl(BleGattService *profile)
{
	BHA_SERVICE_INFO *new_srv = (BHA_SERVICE_INFO *) os_mem_alloc(0, sizeof(BHA_SERVICE_INFO));
	memset(new_srv, 0, sizeof(BHA_SERVICE_CALLBACK_INFO));
	new_srv->status = BHA_SERVICE_REG_PENDING;
	uint8_t srv_index = 0;


	uint8_t att_count = profile->attrNum;
	BleGattAttr *att_list = profile->attrList;
	new_srv->att_tbl = (T_ATTRIB_APPL *) os_mem_alloc(0, (4 * att_count  - 3) * sizeof(T_ATTRIB_APPL));
	memset(new_srv->att_tbl, 0, (4 * att_count  - 3) * sizeof(T_ATTRIB_APPL));
	new_srv->cbInfo = (BHA_SERVICE_CALLBACK_INFO *) os_mem_alloc(0, (4 * att_count  - 3) * sizeof(BHA_SERVICE_CALLBACK_INFO));
	memset(new_srv->cbInfo, 0, (4 * att_count  - 3) * sizeof(BHA_SERVICE_CALLBACK_INFO));
	uint16_t j = 0;
	for (int i = 0; i < att_count; i++) {
		if (att_list[i].attrType == OHOS_BLE_ATTRIB_TYPE_SERVICE) {
			srv_index = ble_srv_count ++;
			setup_ble_serv_dec_attr(&new_srv->att_tbl[j], att_list[i].uuid, att_list[i].uuidType);
			new_srv->cbInfo[j].att_handle = j;
			memcpy(&(new_srv->cbInfo[j].func), &(att_list[i].func), sizeof(BleGattOperateFunc));
			setup_ble_srv_info(srv_index, j, i);
			j ++;
		} else if (att_list[i].attrType ==  OHOS_BLE_ATTRIB_TYPE_CHAR) {
			setup_ble_char_dec_attr(&new_srv->att_tbl[j], att_list[i].properties);
			new_srv->cbInfo[j].att_handle = j;
			memcpy(&(new_srv->cbInfo[j].func), &(att_list[i].func), sizeof(BleGattOperateFunc));
			j ++;
			setup_ble_char_val_attr(&new_srv->att_tbl[j], att_list[i].uuid, att_list[i].uuidType, att_list[i].permission);
			setup_ble_char_info(srv_index, &att_list[i], j, i);
			new_srv->cbInfo[j].att_handle = j;
			memcpy(&(new_srv->cbInfo[j].func), &(att_list[i].func), sizeof(BleGattOperateFunc));
			j ++;
			if ((att_list[i].properties & OHOS_GATT_CHARACTER_PROPERTY_BIT_NOTIFY) || (att_list[i].properties & OHOS_GATT_CHARACTER_PROPERTY_BIT_INDICATE)) {
				setup_ble_char_ccc_attr(&new_srv->att_tbl[j]);
				new_srv->cbInfo[j].att_handle = j;
				memcpy(&(new_srv->cbInfo[j].func), &(att_list[i].func), sizeof(BleGattOperateFunc));
				j ++;
			}
		} else if (att_list[i].attrType ==  OHOS_BLE_ATTRIB_TYPE_CHAR_USER_DESCR) {
			setup_ble_char_usd_attr(&new_srv->att_tbl[j], att_list[i].value, att_list[i].valLen, att_list[i].permission);
			new_srv->cbInfo[j].att_handle = j;
			memcpy(&(new_srv->cbInfo[j].func), &(att_list[i].func), sizeof(BleGattOperateFunc));
			j ++;
		} else {
			printf("\r\n[%s] Unknown Attribute Type", __FUNCTION__);
		}
	}
	new_srv->att_num = j;
	ble_srv_database[srv_index].chrc_num = j;
	if (bt_harmony_adapter_srv_p == &bt_harmony_adapter_srvs_head) {
		new_srv->start_handle = 1;    //first service, handle start from 0x1
	} else {
		new_srv->start_handle = bt_harmony_adapter_srv_p->start_handle + bt_harmony_adapter_srv_p->att_num;
	}
	bt_harmony_adapter_srv_p->next = new_srv;
	bt_harmony_adapter_srv_p = bt_harmony_adapter_srv_p->next;
	return new_srv;
}
