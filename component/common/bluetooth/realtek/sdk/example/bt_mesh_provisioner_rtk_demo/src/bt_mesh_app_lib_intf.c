#include "platform_opts.h"
#include "bt_mesh_app_lib_intf.h"
#include "wifi_conf.h"
#include "gap_conn_le.h"
#include "mesh_api.h"
#include "generic_on_off.h"
#include "bt_mesh_provisioner_api.h"
#include "provisioner_app.h"
#include "platform_os.h"

#define CMD_REPEAT_TIMES	5
#define BT_MESH_APP_LIB_INTF_DEBUG		1

struct BT_MESH_LIB_PRIV bt_mesh_lib_priv;

uint8_t bt_mesh_user_cmd_cbk(uint16_t mesh_code, void *user_data)
{
    PUSER_ITEM puserItem = NULL;
    CMD_ITEM_S *pmeshCmdItem_s = NULL;

    pmeshCmdItem_s = (CMD_ITEM_S *)user_data;
    puserItem = pmeshCmdItem_s->pmeshCmdItem->userData;
    if (pmeshCmdItem_s->userApiMode == USER_API_SYNCH) {
        if (pmeshCmdItem_s->semaDownTimeOut) {
            printf("\r\n %s() mesh code = %d fail caused user cmd timeout",__func__, mesh_code);
            return USER_API_RESULT_TIMEOUT;
        }
    }
    printf("\r\n %s() mesh code = %d userCmdResult = %d",__func__, mesh_code, puserItem->userCmdResult);
    switch (mesh_code) {
        case GEN_MESH_CODE(_pb_adv_con):
        {
            if (puserItem->userCmdResult == BT_MESH_USER_CMD_SUCCESS) {
                bt_mesh_lib_priv.connect_device_flag = 1;
            }else {
                bt_mesh_lib_priv.connect_device_flag = 0;
            }
        }
        break;

        case GEN_MESH_CODE(_prov):
        {
            if (puserItem->userCmdResult == BT_MESH_USER_CMD_SUCCESS) {
                rtw_memcpy(&bt_mesh_lib_priv.connect_device_mesh_addr, (uint16_t *)puserItem->userParam, sizeof(uint16_t));
            }else {
                bt_mesh_lib_priv.connect_device_mesh_addr = 0;
            }
        }
        break;

        case GEN_MESH_CODE(_app_key_add):
        {
            if (puserItem->userCmdResult == BT_MESH_USER_CMD_SUCCESS) {
                bt_mesh_lib_priv.connect_device_flag = 1;
            }else {
                bt_mesh_lib_priv.connect_device_flag = 0;
            }
        }
        break;

        case GEN_MESH_CODE(_model_app_bind):
        {
            if (puserItem->userCmdResult == BT_MESH_USER_CMD_SUCCESS) {
                bt_mesh_lib_priv.connect_device_flag = 1;
            }else {
                bt_mesh_lib_priv.connect_device_flag = 0;
            }
        }
        break; 

        case GEN_MESH_CODE(_model_pub_set):
        {
            if (puserItem->userCmdResult == BT_MESH_USER_CMD_SUCCESS) {
                bt_mesh_lib_priv.connect_device_flag = 1;
                bt_mesh_lib_priv.connect_device_goog_mesh_addr = bt_mesh_lib_priv.connect_device_mesh_addr;
	            bt_mesh_lib_priv.connect_device_goog_light_state = 0;
            }else {
                bt_mesh_lib_priv.connect_device_flag = 0;
            }
        }
        break;

        case GEN_MESH_CODE(_generic_on_off_get):
        {

        }
        break;

        case GEN_MESH_CODE(_generic_on_off_set):
        {
            generic_on_off_client_status_t *pdata = (generic_on_off_client_status_t *)puserItem->userParam;
            /* maybe the bt_mesh_indication is from node's publish */
            if (bt_mesh_lib_priv.set_node_state_mesh_addr != pdata->src) {
                printf("\r\n %s() this goos indication is not matched",__func__);
                return USER_API_RESULT_INDICATION_NOT_MATCHED;
            }
            bt_mesh_lib_priv.set_node_state_light_state = puserItem->userCmdResult;
        }
        break;

        case GEN_MESH_CODE(_node_reset):
        {

        }
        break;
        
        case GEN_MESH_CODE(_model_sub_delete):
        {
            if (puserItem->userCmdResult == BT_MESH_USER_CMD_SUCCESS) {
                bt_mesh_lib_priv.set_node_group_flag = 1;
            }else {
                bt_mesh_lib_priv.set_node_group_flag = 0;
            }
        }
        break;

        case GEN_MESH_CODE(_model_sub_add):
        {
            if (puserItem->userCmdResult == BT_MESH_USER_CMD_SUCCESS) {
                bt_mesh_lib_priv.set_node_group_flag = 1;
            }else {
                bt_mesh_lib_priv.set_node_group_flag = 0;
            }
        }
        break;
            
        default:
        {
            printf("[BT_MESH] %s(): user cmd %d not found !\r\n", __func__, mesh_code);
        }
        break;        
    }

    return USER_API_RESULT_OK;
}

extern int bt_mesh_provisioner_app_main(void);
extern int bt_mesh_provisioner_multiple_profile_app_main(void);
extern void bt_coex_init(void);
extern void wifi_btcoex_set_bt_on(void);

uint8_t bt_mesh_cmd_start_provisioner_api(void)
{
    T_GAP_DEV_STATE new_state;
    
    /*Wait WIFI init complete*/
    while(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
        os_delay(1000);
    }
    //judge BLE central is already on
    le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
    if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY) {
        printf("[BT Mesh Provisioner]BT Stack already on\n\r");
        return 0;
    }
    else
#if defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER
        bt_mesh_provisioner_app_main();
#elif defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE
        bt_mesh_provisioner_multiple_profile_app_main();
#endif
    bt_coex_init();

    /*Wait BT init complete*/
    do {
        os_delay(100);
        le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
    }while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

    /*Start BT WIFI coexistence*/
    wifi_btcoex_set_bt_on();

#if defined(CONFIG_BT_MESH_PROVISIONER_RTK_DEMO) && CONFIG_BT_MESH_PROVISIONER_RTK_DEMO
    if (bt_mesh_provisioner_api_init()) {
        printf("[BT Mesh Provisioner] bt_mesh_provisioner_api_init fail ! \n\r");
        return 0;
    }
#endif

    return 1;

}

uint8_t bt_mesh_cmd_connect_device_api(uint8_t *dev_uuid, uint16_t *mesh_addr, uint8_t *light_state)
{
    PUSER_ITEM puserItem = NULL;
    uint8_t ret;
    char devUuid[40];

    /*_pb_adv_con */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		goto exit;
    }
    memset(devUuid, 0, sizeof(devUuid));
    user_cmd_array2string(dev_uuid, 16, devUuid);
    puserItem->pparseValue->pparameter[0] = devUuid;
    puserItem->pparseValue->para_count = 1;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_pb_adv_con), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		goto exit;
    }

    /* _prov */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		goto exit;
    }
    puserItem->pparseValue->dw_parameter[0] = 0;//attn_dur
    puserItem->pparseValue->dw_parameter[1] = 0;//prov_manual
    puserItem->pparseValue->para_count = 2;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_prov), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        if (ret == USER_API_RESULT_TIMEOUT) {
            /* _prov_stop */
            puserItem = bt_mesh_alloc_hdl(USER_API_ASYNCH);
            if (!puserItem) {
                printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
        		goto exit;
            }
            puserItem->pparseValue->para_count = 0;
            ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_prov_stop), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
        }
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		goto exit;
    }

    /* _app_key_add */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		goto nr_exit;
    }
    puserItem->pparseValue->dw_parameter[0] = bt_mesh_lib_priv.connect_device_mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = 0;//net_key_index
    puserItem->pparseValue->dw_parameter[2] = 0;//app_key_index
    puserItem->pparseValue->para_count = 3;
    //printf("\r\n %s() mesh_adr = %x",__func__,bt_mesh_lib_priv.connect_device_mesh_addr);
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_app_key_add), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		goto nr_exit;
    }

    /* _model_app_bind */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		goto nr_exit;
    }
    puserItem->pparseValue->dw_parameter[0] = bt_mesh_lib_priv.connect_device_mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = 0;//element_index
    puserItem->pparseValue->dw_parameter[2] = MESH_MODEL_GENERIC_ON_OFF_SERVER;//model_id
    puserItem->pparseValue->dw_parameter[3] = 0;//app_key_index
    puserItem->pparseValue->para_count = 4;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_model_app_bind), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		goto nr_exit;
    }

    /* _model_pub_set */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		goto nr_exit;
    }
    puserItem->pparseValue->dw_parameter[0] = bt_mesh_lib_priv.connect_device_mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = bt_mesh_lib_priv.connect_device_mesh_addr + 0;//mesh_addr + mesh_index
    puserItem->pparseValue->dw_parameter[2] = 0;//va_flag
    puserItem->pparseValue->dw_parameter[3] = 0xFEFF;//element_addr
    puserItem->pparseValue->dw_parameter[4] = 0;//app key index
    puserItem->pparseValue->dw_parameter[5] = 0;//frnd_flag
    puserItem->pparseValue->dw_parameter[6] = 0x3F;//pub_ttl
    puserItem->pparseValue->dw_parameter[7] = 0x86;
    puserItem->pparseValue->dw_parameter[8] = 0;
    puserItem->pparseValue->dw_parameter[9] = 3;
    puserItem->pparseValue->dw_parameter[10] = MESH_MODEL_GENERIC_ON_OFF_SERVER;
    puserItem->pparseValue->para_count = 11;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_model_pub_set), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		goto nr_exit;
    }
    
    /* _generic_on_off_get */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		goto nr_exit;
    }
    puserItem->pparseValue->dw_parameter[0] = bt_mesh_lib_priv.connect_device_mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = 0;//app_key_index
    puserItem->pparseValue->para_count = 2;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_generic_on_off_get), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if ((ret != USER_API_RESULT_OK) && (ret != USER_API_RESULT_ERROR)) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		goto nr_exit;
    }
	*mesh_addr = bt_mesh_lib_priv.connect_device_goog_mesh_addr;
	*light_state = bt_mesh_lib_priv.connect_device_goog_light_state;
	bt_mesh_lib_priv.connect_device_goog_mesh_addr = 0;
	bt_mesh_lib_priv.connect_device_mesh_addr = 0;
    
	return 1;

nr_exit:
    /* nr */
    if (bt_mesh_lib_priv.connect_device_mesh_addr == 0) {
        printf("[BT_MESH_DEMO] There is no device provisioned!\r\n");
        goto exit;
    }
    bt_mesh_lib_priv.connect_device_nr_mesh_addr = bt_mesh_lib_priv.connect_device_mesh_addr;
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
        bt_mesh_lib_priv.connect_device_nr_mesh_addr = 0;
		goto exit;
    }
    puserItem->pparseValue->dw_parameter[0] = bt_mesh_lib_priv.connect_device_mesh_addr;//mesh_adr
    puserItem->pparseValue->para_count = 1;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_node_reset), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
        bt_mesh_lib_priv.connect_device_nr_mesh_addr = 0;
		goto exit;
    }
    bt_mesh_lib_priv.connect_device_nr_mesh_addr = 0;

exit:
    bt_mesh_lib_priv.connect_device_mesh_addr = 0;
    return 0;

}

uint8_t bt_mesh_cmd_delete_node_api(uint16_t mesh_addr)
{
    PUSER_ITEM puserItem = NULL;
    uint8_t ret;

    bt_mesh_lib_priv.delete_node_mesh_addr = mesh_addr;
    
    /* _node_reset */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		return 0;
    }
    puserItem->pparseValue->dw_parameter[0] = mesh_addr;//attn_dur
    puserItem->pparseValue->para_count = 1;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_node_reset), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		return 0;
    }
	bt_mesh_lib_priv.delete_node_mesh_addr = 0;
    
	return 1;
}

uint8_t bt_mesh_cmd_set_node_state_api(uint16_t mesh_addr, uint16_t group_addr, uint8_t light_state, uint8_t flag)
{
    PUSER_ITEM puserItem = NULL;
    uint8_t ret;

    bt_mesh_lib_priv.set_node_state_mesh_addr = mesh_addr;
	bt_mesh_lib_priv.set_node_state_light_state = 0;
    
    /* _generic_on_off_set */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		return 0;
    }
    if (flag && MESH_IS_GROUP_ADDR(group_addr)) {
        puserItem->pparseValue->dw_parameter[0] = group_addr;
    } else {
        puserItem->pparseValue->dw_parameter[0] = mesh_addr;
    }	
    puserItem->pparseValue->dw_parameter[1] = light_state;//light_state
    puserItem->pparseValue->dw_parameter[2] = 1;//ack
    puserItem->pparseValue->dw_parameter[3] = 0;//app_key_index
    puserItem->pparseValue->dw_parameter[4] = 0;//trans_time
    puserItem->pparseValue->dw_parameter[5] = 0;//trans_time
    puserItem->pparseValue->dw_parameter[6] = 0;//delay
    puserItem->pparseValue->para_count = 4;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_generic_on_off_set), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if ((ret != USER_API_RESULT_OK) && (ret != USER_API_RESULT_ERROR)) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
        return 0xFF;
    }
	ret = bt_mesh_lib_priv.set_node_state_light_state;
	bt_mesh_lib_priv.set_node_state_mesh_addr = 0;
    
	return ret;
}

uint8_t bt_mesh_cmd_get_node_state_api(uint16_t mesh_addr)
{
    PUSER_ITEM puserItem = NULL;
    uint8_t ret;
    
    /* _generic_on_off_get */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		return 0xFF;
    }
    puserItem->pparseValue->dw_parameter[0] = mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = 0;//app_key_index
    puserItem->pparseValue->para_count = 2;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_generic_on_off_get), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if ((ret != USER_API_RESULT_OK) && (ret != USER_API_RESULT_ERROR)) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		return 0xFF;
    }
    
	return ret;
}

uint8_t bt_mesh_cmd_set_node_group_api(uint16_t mesh_addr, uint16_t group_addr)
{
	PUSER_ITEM puserItem = NULL;
    uint8_t ret;

	bt_mesh_lib_priv.set_node_group_flag = 0;
	bt_mesh_lib_priv.set_node_group_mesh_addr = mesh_addr;

    /* _model_sub_delete */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		return 0;
    }
    puserItem->pparseValue->dw_parameter[0] = mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = 0;//element_index
    puserItem->pparseValue->dw_parameter[2] = MESH_MODEL_GENERIC_ON_OFF_SERVER;//model_id
    puserItem->pparseValue->para_count = 3;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_model_sub_delete), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		return 0;
    }

    if (group_addr == 0) {
        printf("[BT_MESH_DEMO] Delete mesh_addr 0x%x from Group \r\n", mesh_addr);
        return 1;
    }
    /* _model_sub_add */
    puserItem = bt_mesh_alloc_hdl(USER_API_SYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		return 0;
    }
    puserItem->pparseValue->dw_parameter[0] = mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = 0;//element_index
    puserItem->pparseValue->dw_parameter[2] = MESH_MODEL_GENERIC_ON_OFF_SERVER;//model_id
    puserItem->pparseValue->dw_parameter[3] = group_addr;//group_addr
    puserItem->pparseValue->para_count = 4;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_model_sub_add), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		return 0;
    }
	bt_mesh_lib_priv.set_node_group_mesh_addr = 0;
    
	return 1;
}

uint8_t bt_mesh_cmd_get_node_group_api(uint16_t mesh_addr)
{
    PUSER_ITEM puserItem = NULL;
    uint8_t ret;

    /* _model_sub_get */
    puserItem = bt_mesh_alloc_hdl(USER_API_ASYNCH);
    if (!puserItem) {
        printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
		return 0;
    }
    puserItem->pparseValue->dw_parameter[0] = mesh_addr;//mesh_adr
    puserItem->pparseValue->dw_parameter[1] = 0;//element_index
    puserItem->pparseValue->dw_parameter[2] = MESH_MODEL_GENERIC_ON_OFF_SERVER;//model_id
    puserItem->pparseValue->para_count = 3;
    ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_model_sub_get), puserItem->pparseValue, bt_mesh_user_cmd_cbk, puserItem);
    if (ret != USER_API_RESULT_OK) {
        printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
		return 0;
    }

    return 1;
}

