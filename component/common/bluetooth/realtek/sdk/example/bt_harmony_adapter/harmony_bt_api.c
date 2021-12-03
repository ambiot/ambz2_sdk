#include <gap.h>
#include <os_mem.h>
#include "bt_harmony_adapter_peripheral_app.h"
#include "ohos_bt_gatt_server.h"
#include "ohos_bt_gatt_client.h"
#include <gap_msg.h>
#include "platform_stdlib.h"
#include "string.h"
#include <os_sched.h>
#include <os_sync.h>

uint8_t H_adv_data[31];
uint8_t H_scan_response_data[31];
uint16_t H_auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
uint8_t  H_auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
void *send_indication_sem = NULL;
extern BHA_SRV_DATABASE ble_srv_database[12];

extern uint8_t RT_remote_bd[OHOS_BD_ADDR_LEN];
extern int bt_get_mac_address(uint8_t *mac);
extern void bt_harmony_adapter_app_init(void);
extern void bt_harmony_adapter_app_deinit(void);
extern bool bt_harmony_adapter_app_send_api_msg(T_BHA_API_MSG_TYPE sub_type, void *buf);
int SetDeviceName(const char *name, unsigned int len)
{
	return OHOS_BT_STATUS_SUCCESS;
}

int InitBtStack(void)
{
	return OHOS_BT_STATUS_SUCCESS;
}

int EnableBtStack(void)
{
	bt_harmony_adapter_app_init();

	return OHOS_BT_STATUS_SUCCESS;
}

int DisableBtStack(void)
{
	bt_harmony_adapter_app_deinit();

	return OHOS_BT_STATUS_SUCCESS;
}

int BleStopAdv(int advId)
{
	int cause = false;
	cause = bt_harmony_adapter_app_send_api_msg(BHA_API_MSG_STOP_ADV, NULL);

	if (cause == false) {
		return OHOS_BT_STATUS_FAIL;
	} else {
		return OHOS_BT_STATUS_SUCCESS;
	}
}

int BleSetSecurityIoCap(BleIoCapMode mode)
{
	switch (mode) {
	case OHOS_BLE_IO_CAP_OUT:
		H_auth_io_cap = GAP_IO_CAP_DISPLAY_ONLY;
		break;
	case OHOS_BLE_IO_CAP_IO:
		H_auth_io_cap = GAP_IO_CAP_DISPLAY_YES_NO;
		break;
	case OHOS_BLE_IO_CAP_IN:
		H_auth_io_cap = GAP_IO_CAP_KEYBOARD_ONLY;
		break;
	case OHOS_BLE_IO_CAP_NONE:
		H_auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
		break;
	case OHOS_BLE_IO_CAP_KBDISP:
		H_auth_io_cap = GAP_IO_CAP_KEYBOARD_DISPLAY;
		break;
	default:
		printf("\n\r[%s]:invalid IO Capability\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
		break;
	}
	return OHOS_BT_STATUS_SUCCESS;
}

int BleSetSecurityAuthReq(BleAuthReqMode mode)
{
	switch (mode) {
	case OHOS_BLE_AUTH_NO_BOND:
		//no bond,no MITM,no SC
		H_auth_flags = GAP_AUTHEN_BIT_NONE;
		break;
	case OHOS_BLE_AUTH_BOND:
		//bond,no MITM,no SC
		H_auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
		break;
	case OHOS_BLE_AUTH_REQ_MITM:
		//no bond,no SC
		H_auth_flags = GAP_AUTHEN_BIT_MITM_FLAG;
		break;
	case OHOS_BLE_AUTH_REQ_SC_ONLY:
		//no bond,no MITM,SC only
		H_auth_flags = GAP_AUTHEN_BIT_SC_FLAG | GAP_AUTHEN_BIT_SC_ONLY_FLAG;
		break;
	case OHOS_BLE_AUTH_REQ_SC_BOND:
		//bond,sc,no MITM
		H_auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
		break;
	case OHOS_BLE_AUTH_REQ_SC_MITM:
		//no bond,SC,MITM
		H_auth_flags = GAP_AUTHEN_BIT_MITM_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
		break;
	case OHOS_BLE_AUTH_REQ_SC_MITM_BOND:
		//bond,SC,MITM
		H_auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG | GAP_AUTHEN_BIT_MITM_FLAG | GAP_AUTHEN_BIT_SC_FLAG;
		break;
	default:
		printf("\n\r[%s]:invalid authreqmode\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
		break;
	}
	return OHOS_BT_STATUS_SUCCESS;

}

int BleGattSecurityRsp(BdAddr bdAddr, bool accept)
{
	int cause = false;
	if (memcmp(RT_remote_bd, bdAddr.addr, OHOS_BD_ADDR_LEN) == 0) {
		H_SecurityRsp_param *SecurityRsp = (H_SecurityRsp_param *)os_mem_alloc(0, sizeof(H_SecurityRsp_param));
		SecurityRsp->accept = accept;
		memcpy(SecurityRsp->Address.addr, bdAddr.addr, OHOS_BD_ADDR_LEN);
		cause = bt_harmony_adapter_app_send_api_msg(BHA_API_MSG_AUTH_RESPOND, SecurityRsp);
		if (cause == false) {
			os_mem_free(SecurityRsp);
			return OHOS_BT_STATUS_FAIL;
		} else {
			return OHOS_BT_STATUS_SUCCESS;
		}
	} else {
		printf("\n\r[%s]:invalid param bdAddr\r\n", __func__, __LINE__);
		return OHOS_BT_STATUS_PARM_INVALID;
	}
}

int BleGattsDisconnect(int serverId, BdAddr bdAddr, int connId)
{
	uint8_t conn_id = 0;
	int cause = false;
	if (connId == conn_id) {
		if (memcmp(RT_remote_bd, bdAddr.addr, OHOS_BD_ADDR_LEN) == 0) {
			bt_harmony_adapter_app_send_api_msg(BHA_API_MSG_DISCONNECT, &bdAddr.addr);
			if (cause == false) {
				return OHOS_BT_STATUS_FAIL;
			} else {
				return OHOS_BT_STATUS_SUCCESS;
			}
		} else {
			printf("\n\r[%s]:invalid bt address\r\n", __func__);
			return OHOS_BT_STATUS_PARM_INVALID;
		}
	} else {
		printf("\n\r[%s]:invalid connId\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
	}
}

int BleGattsSetEncryption(BdAddr bdAddr, BleSecAct secAct)
{
	printf("\n\r[%s]:we not support this feature \r\n", __func__);
	return OHOS_BT_STATUS_SUCCESS;
}


int ReadBtMacAddr(unsigned char *mac, unsigned int len)
{
	if ((mac == NULL) || (len != 6)) {
		return OHOS_BT_STATUS_PARM_INVALID;
	}

	bt_get_mac_address(mac);

	return OHOS_BT_STATUS_SUCCESS;
}

int BleStartAdvEx(int *advId, const StartAdvRawData rawData, BleAdvParams advParam)
{

	T_GAP_DEV_STATE new_state = {0};
	H_adv_param *h_adv_param = (H_adv_param *)os_mem_alloc(0, sizeof(H_adv_param));
	int cause = false;

	if ((rawData.advDataLen > 31) || (rawData.rspDataLen > 31)) {
		printf("\n\r[%s]:invalid advdata length or scan resp length\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
	}
	if ((advParam.minInterval < 0x20) || (advParam.maxInterval > 0x4000) || (advParam.minInterval > advParam.maxInterval)) {
		printf("\n\r[%s]:invalid adv interval param\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
	}
	if ((advParam.ownAddrType != 0) && (advParam.ownAddrType != 1) && (advParam.ownAddrType != 2) && (advParam.ownAddrType != 3)) {
		printf("\n\r[%s]:invalid owner address type\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
	}
	if ((advParam.peerAddrType != 0) && (advParam.peerAddrType != 1)) {
		printf("\n\r[%s]:invalid peer address type\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
	}
	if (((advParam.channelMap & (~GAP_ADVCHAN_ALL)) != 0) || (advParam.channelMap == 0)) {
		printf("\n\r[%s]:invalid channel map\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
	}

	switch (advParam.advType) {
	case OHOS_BLE_ADV_IND:
		h_adv_param->H_adv_evt_type = GAP_ADTYPE_ADV_IND;
		break;
	case OHOS_BLE_ADV_DIRECT_IND_HIGH:
		h_adv_param->H_adv_evt_type = GAP_ADTYPE_ADV_HDC_DIRECT_IND;
		break;
	case OHOS_BLE_ADV_SCAN_IND:
		h_adv_param->H_adv_evt_type = GAP_ADTYPE_ADV_SCAN_IND;
		break;
	case OHOS_BLE_ADV_NONCONN_IND:
		h_adv_param->H_adv_evt_type = GAP_ADTYPE_ADV_NONCONN_IND;
		break;
	case OHOS_BLE_ADV_DIRECT_IND_LOW:
		h_adv_param->H_adv_evt_type = GAP_ADTYPE_ADV_LDC_DIRECT_IND;
		break;
	default:
		printf("\n\r[%s]:invalid adv type\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
		break;
	}

	switch (advParam.advFilterPolicy) {
	case OHOS_BLE_ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY:
		h_adv_param->H_adv_filter_policy = GAP_ADV_FILTER_ANY;
		break;
	case OHOS_BLE_ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY:
		h_adv_param->H_adv_filter_policy = GAP_ADV_FILTER_WHITE_LIST_SCAN;
		break;
	case OHOS_BLE_ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST:
		h_adv_param->H_adv_filter_policy = GAP_ADV_FILTER_WHITE_LIST_CONN;
		break;
	case OHOS_BLE_ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST:
		h_adv_param->H_adv_filter_policy = GAP_ADV_FILTER_WHITE_LIST_ALL;
		break;
	default:
		printf("\n\r[%s]:invalid Filter Policy\r\n", __func__);
		return OHOS_BT_STATUS_PARM_INVALID;
		break;
	}

	le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	if (new_state.gap_adv_state == GAP_ADV_STATE_START) {
		while (new_state.gap_adv_state != GAP_ADV_STATE_ADVERTISING) {
			os_delay(1);
			le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
		}
	}
	if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING) {
		cause = bt_harmony_adapter_app_send_api_msg(BHA_CALLBACK_MSG_ADV_STOPED, NULL); //stop adv
		if (cause == false) {
			return OHOS_BT_STATUS_FAIL;
		}
	}
	while (new_state.gap_adv_state != GAP_ADV_STATE_IDLE) {
		os_delay(1);
		le_get_gap_param(GAP_PARAM_DEV_STATE, &new_state);
	}

	memcpy(H_adv_data, rawData.advData, rawData.advDataLen);
	memcpy(H_scan_response_data, rawData.rspData, rawData.rspDataLen);

	h_adv_param->H_adv_int_min = advParam.minInterval;
	h_adv_param->H_adv_int_max = advParam.maxInterval;
	h_adv_param->H_local_addr_type = advParam.ownAddrType;
	h_adv_param->H_adv_direct_type = advParam.peerAddrType;
	memcpy(h_adv_param->H_adv_direct_addr, advParam.peerAddr.addr, OHOS_BD_ADDR_LEN);
	h_adv_param->H_adv_chann_map = advParam.channelMap;
	h_adv_param->H_duration = advParam.duration;
	h_adv_param->adv_datalen = rawData.advDataLen;
	h_adv_param->scanrep_datalen = rawData.rspDataLen;
	cause = bt_harmony_adapter_app_send_api_msg(BHA_API_MSG_START_ADV, h_adv_param);
	if (cause == false) {
		os_mem_free(h_adv_param);
		return OHOS_BT_STATUS_FAIL;
	}

	return OHOS_BT_STATUS_SUCCESS;
}

BtGattCallbacks BH_GattCallbacks ;
BtGattServerCallbacks BH_GattServerCallbacks;

int BleGattRegisterCallbacks(BtGattCallbacks *func)
{
	if (func == NULL) {
		return OHOS_BT_STATUS_PARM_INVALID;
	}

	BH_GattCallbacks.advEnableCb = func->advEnableCb;
	BH_GattCallbacks.advDisableCb = func->advDisableCb;
	BH_GattCallbacks.securityRespondCb = func->securityRespondCb;
	return OHOS_BT_STATUS_SUCCESS;
}

int BleGattsRegisterCallbacks(BtGattServerCallbacks *func)
{
	if (func == NULL) {
		return OHOS_BT_STATUS_PARM_INVALID;
	}
	BH_GattServerCallbacks.connectServerCb = func->connectServerCb;
	BH_GattServerCallbacks.disconnectServerCb = func->disconnectServerCb;
	BH_GattServerCallbacks.mtuChangeCb = func->mtuChangeCb;
	BH_GattServerCallbacks.serviceStartCb = func->serviceStartCb;
	return OHOS_BT_STATUS_SUCCESS;
}

int BleGattsStartServiceEx(int *srvcHandle, BleGattService *srvcInfo)
{
	if (srvcInfo == NULL || srvcInfo->attrList == NULL) {
		return OHOS_BT_STATUS_PARM_INVALID;
	}

	if (bt_harmony_adapter_srvs_num >= BT_HARMONY_ADAPTER_SERVICE_MAX_NUM) {
		printf("\r\n[%s] BLE Stack only support up to 12 Services!", __FUNCTION__);
		return OHOS_BT_STATUS_FAIL;
	}

	BHA_SERVICE_INFO *srv_info = bt_harmony_adapter_parse_srv_tbl(srvcInfo);
	*srvcHandle = srv_info->start_handle;
	if (bt_harmony_adapter_app_send_api_msg(BHA_API_MSG_ADD_SERVICE, srv_info)) {
		return OHOS_BT_STATUS_SUCCESS;
	} else {
		return OHOS_BT_STATUS_FAIL;
	}
}

int BleGattsSendIndication(int serverId, GattsSendIndParam *param)
{
	if (param == NULL || param->value == NULL || param->connectId != 0) {
		return OHOS_BT_STATUS_PARM_INVALID;
	}
	if (send_indication_sem == NULL) {
		if (os_sem_create(&send_indication_sem, 0, 1) == true) {
			printf("\n\r[%s]:create mutex success \n", __func__);
		} else {
			printf("\n\r[%s]:create mutex fail \n", __func__);
		}
	}
	for (int i = 0; i < BT_HARMONY_ADAPTER_SERVICE_MAX_NUM; i ++) {
		if ((param->attrHandle > ble_srv_database[i].start_handle) && (param->attrHandle < (ble_srv_database[i].start_handle + ble_srv_database[i].chrc_num))) {
			serverId = i;
		}
	}

	int real_handle = 0;
	real_handle = param->attrHandle - ble_srv_database[serverId].start_handle;
	BHA_INDICATION_PARAM *indi = (BHA_INDICATION_PARAM *) os_mem_alloc(0, sizeof(BHA_INDICATION_PARAM));
	indi->srv_id = serverId;
	indi->type = param->confirm ? 1 : 0;
	indi->att_handle = ble_srv_database[serverId].rela_atthandle[real_handle];
	indi->len = param->valueLen;
	indi->val = (char *) os_mem_alloc(0, param->valueLen);
	memcpy(indi->val, param->value, param->valueLen);
	if (bt_harmony_adapter_app_send_api_msg(BHA_API_MSG_SEND_INDICATION, indi)) {

		if(os_sem_take(send_indication_sem, 30000) == true){
			return OHOS_BT_STATUS_SUCCESS;
		}
	}else{
		os_mem_free(indi->val);
		os_mem_free(indi);
	} 
	return OHOS_BT_STATUS_FAIL;
}

int BleGattsStopServiceEx(int srvcHandle)
{
	printf("\r\n[%s] BLE Stack do not support stop service!", __FUNCTION__);
	return OHOS_BT_STATUS_FAIL;
}
