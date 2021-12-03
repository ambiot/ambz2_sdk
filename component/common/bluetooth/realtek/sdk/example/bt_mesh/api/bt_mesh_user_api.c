/**
*****************************************************************************************
*     Copyright(c) 2019, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     bt_mesh_user_api.c
  * @brief    Source file for provisioner cmd.
  * @details  User command interfaces.
  * @author   sherman
  * @date     2019-09-16
  * @version  v1.0
  * *************************************************************************************
  */
#include "bt_mesh_user_api.h"
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
#include "bt_mesh_provisioner_api.h"
#elif (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
    defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
#include "bt_mesh_device_api.h"
#endif

#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
#if ((defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
    (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE) || \
    (defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) || \
    (defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE))

CMD_MOD_INFO_S              btMeshCmdPriv;
INDICATION_ITEM             btMeshCmdIdPriv;
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
extern struct mesh_task_struct	meshProvisionerCmdThread;
#elif (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
    defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
extern struct mesh_task_struct	meshDeviceCmdThread;
#endif

static const mesh_cmd_entry mesh_cmd_table[] = {
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
    PARAM_MESH_CODE("pbadvcon", "\r pbadvcon [dev uuid]", "\r create a pb-adv link with the device uuid\n\r", GEN_MESH_CODE(_pb_adv_con))
    PARAM_MESH_CODE("prov", "\r prov [attn_dur] [manual]", "\r provision a new mesh device\n\r", GEN_MESH_CODE(_prov))
    PARAM_MESH_CODE("provs", "\r provs", "\r provision stop\n\r", GEN_MESH_CODE(_prov_stop))
    PARAM_MESH_CODE("aka", "\r aka [dst] [net_key_index] [app_key_index]", "\r app key add\n\r", GEN_MESH_CODE(_app_key_add))
    PARAM_MESH_CODE("mab", "\r mab [dst] [element index] [model_id] [app_key_index]", "\r model app bind\n\r", GEN_MESH_CODE(_model_app_bind))
    PARAM_MESH_CODE("goos", "\r goos [dst] [on/off] [ack] [app_key_index] [steps] [resolution] [delay]", "\r generic on off set\n\r", GEN_MESH_CODE(_generic_on_off_set))
    PARAM_MESH_CODE("goog", "\r goog [dst] [app_key_index]", "\r generic on off get\n\r", GEN_MESH_CODE(_generic_on_off_get))
    PARAM_MESH_CODE("nr", "\r nr [dst]", "\r node reset\n\r", GEN_MESH_CODE(_node_reset))
    PARAM_MESH_CODE("msd", "\r msd [dst] [element index] [model_id] <group addr>", "\r model subsribe delete\n\r", GEN_MESH_CODE(_model_sub_delete))
    PARAM_MESH_CODE("msa", "\r msa [dst] [element index] [model_id] [group addr]", "\r model subsribe add\n\r", GEN_MESH_CODE(_model_sub_add))
    PARAM_MESH_CODE("provdis", "\r provdis [conn id]", "\r Start discovery provisioning service\n\r", GEN_MESH_CODE(_prov_discover))
    PARAM_MESH_CODE("provcmd", "\r provcmd [char CCCD] [command: enable/disable]", "\r Provisioning notify/ind switch command\n\r", GEN_MESH_CODE(_prov_cccd_operate))
    PARAM_MESH_CODE("proxydis", "\r proxydis [conn id]", "\r Start discovery proxy service\n\r", GEN_MESH_CODE(_proxy_discover))
    PARAM_MESH_CODE("proxycmd", "\r proxycmd [char CCCD] [command: enable/disable]", "\r Proxy notify/ind switch command\n\r", GEN_MESH_CODE(_proxy_cccd_operate))
    PARAM_MESH_CODE("llg", "\r llg [dst] [app_key_index]", "\r light lightness get\n\r",GEN_MESH_CODE(_light_lightness_get))
    PARAM_MESH_CODE("lls", "\r lls [dst] [lightness] [ack] [app_key_index] [steps] [resolution] [delay]", "\r light lightness set\n\r", GEN_MESH_CODE(_light_lightness_set))
    PARAM_MESH_CODE("lllg", "\r lllg [dst] [app_key_index]", "\r light lightness linear get\n\r", GEN_MESH_CODE(_light_lightness_linear_get))
    PARAM_MESH_CODE("llls", "\r llls [dst] [lightness] [ack] [app_key_index] [steps] [resolution] [delay]", "\r light lightness linear set\n\r", GEN_MESH_CODE(_light_lightness_linear_set))
    PARAM_MESH_CODE("lllag", "\r lllag [dst] [app_key_index]", "\r light lightness last get\n\r", GEN_MESH_CODE(_light_lightness_last_get))
    PARAM_MESH_CODE("lldg", "\r lldg [dst] [app_key_index]", "\r light lightness default get\n\r", GEN_MESH_CODE(_light_lightness_default_get))
    PARAM_MESH_CODE("llds", "\r llds [dst] [lightness] [ack] [app_key_index]", "\r light lightness default set\n\r", GEN_MESH_CODE(_light_lightness_default_set))
    PARAM_MESH_CODE("llrg", "\r llrg [dst] [app_key_index]", "\r light lightness range get\n\r", GEN_MESH_CODE(_light_lightness_range_get))
    PARAM_MESH_CODE("llrs", "\r llrs [dst] [min] [max] [ack] [app_key_index]", "\r light lightness range set\n\r", GEN_MESH_CODE(_light_lightness_range_set))
    PARAM_MESH_CODE("lcg", "\r lcg [dst] [app_key_index]", "\r light ctl set\n\r", GEN_MESH_CODE(_light_ctl_get))
    PARAM_MESH_CODE("lcs", "\r lcs [dst] [lightness] [temperature] [delta uv] [ack] [app_key_index] [steps] [resolution] [delay]", "\r light ctl set\n\r", GEN_MESH_CODE(_light_ctl_set))
    PARAM_MESH_CODE("lctg", "\r lctg [dst] [app_key_index]", "\r light ctl temperature get\n\r", GEN_MESH_CODE(_light_ctl_temperature_get))
    PARAM_MESH_CODE("lcts", "\r lcts [dst] [temperature] [delta uv] [ack] [app_key_index] [steps] [resolution] [delay]", "\r light ctl temperature set\n\r", GEN_MESH_CODE(_light_ctl_temperature_set))
    PARAM_MESH_CODE("lctrg", "\r lctrg [dst] [app_key_index]", "\r light ctl temperature range get\n\r", GEN_MESH_CODE(_light_ctl_temperature_range_get))
    PARAM_MESH_CODE("lctrs", "\r lctrs [dst] [min] [max] [ack] [app_key_index]", "\r light ctl temperature range set\n\r", GEN_MESH_CODE(_light_ctl_temperature_range_set))
    PARAM_MESH_CODE("lcdg", "\r lcdg [dst] [app_key_index]", "\r light ctl default get\n\r", GEN_MESH_CODE(_light_ctl_default_get))
    PARAM_MESH_CODE("lcds", "\r lcds [dst] [lightness] [temperature] [delta uv] [ack] [app_key_index]", "\r light ctl default set\n\r", GEN_MESH_CODE(_light_ctl_default_set))
    PARAM_MESH_CODE("lhg", "\r lhg [dst] [app_key_index]", "\r light hsl get\n\r", GEN_MESH_CODE(_light_hsl_get))
    PARAM_MESH_CODE("lhs", "\r lhs [dst] [hue] [saturation] [lightness] [ack] [app_key_index] [steps] [resolution] [delay]", "\r light hsl set\n\r", GEN_MESH_CODE(_light_hsl_set))
    PARAM_MESH_CODE("lhtg", "\r lhtg [dst] [app_key_index]", "\r light hsl target get\n\r", GEN_MESH_CODE(_light_hsl_target_get))
    PARAM_MESH_CODE("lhhg", "\r lhhg [dst] [app_key_index]", "\r light hsl hue get\n\r", GEN_MESH_CODE(_light_hsl_hue_get))
    PARAM_MESH_CODE("lhhs", "\r lhhs [dst] [hue] [ack] [app_key_index] [steps] [resolution] [delay]", "\r light hsl hue set\n\r", GEN_MESH_CODE(_light_hsl_hue_set))
    PARAM_MESH_CODE("lhsg", "\r lhsg [dst] [app_key_index]", "\r light hsl saturation get\n\r", GEN_MESH_CODE(_light_hsl_saturation_get))
    PARAM_MESH_CODE("lhss", "\r lhss [dst] [saturation] [ack] [app_key_index] [steps] [resolution] [delay]", "\r light hsl saturation set\n\r", GEN_MESH_CODE(_light_hsl_saturation_set))
    PARAM_MESH_CODE("lhdg", "\r lhdg [dst] [app_key_index]", "\r light hsl default get\n\r", GEN_MESH_CODE(_light_hsl_default_get))
    PARAM_MESH_CODE("lhds", "\r lhds [dst] [lightness] [hue] [saturation] [ack] [app_key_index]", "\r light hsl default set\n\r", GEN_MESH_CODE(_light_hsl_default_set))
    PARAM_MESH_CODE("lhrg", "\r lhrg [dst] [app_key_index]", "\r light hsl range get\n\r", GEN_MESH_CODE(_light_hsl_range_get))
    PARAM_MESH_CODE("lhrs", "\r lhrs [dst] [hue min] [hue max] [saturation min] [saturation max] [ack] [app_key_index]", "\r light hsl range set\n\r", GEN_MESH_CODE(_light_hsl_range_set))
    PARAM_MESH_CODE("lxg", "\r lxg [dst] [app_key_index]\n\r", "light xyl get\n\r", GEN_MESH_CODE(_light_xyl_get))
    PARAM_MESH_CODE("lxs", "\r lxs [dst] [lightness] [xyl_x] [xyl_y] [ack] [app_key_index] [steps] [resolution] [delay]\n\r", "light xyl set\n\r", GEN_MESH_CODE(_light_xyl_set))
    PARAM_MESH_CODE("lxtg", "\r lxtg [dst] [app_key_index]\n\r", "light xyl target get\n\r", GEN_MESH_CODE(_light_xyl_target_get))
    PARAM_MESH_CODE("lxdg", "\r lxdg [dst] [app_key_index]\n\r", "light xyl default get\n\r", GEN_MESH_CODE(_light_xyl_default_get))
    PARAM_MESH_CODE("lxds", "\r lxds [dst] [lightness] [xyl_x] [xyl_y] [ack] [app_key_index]\n\r", "light xyl default set\n\r", GEN_MESH_CODE(_light_xyl_default_set))
    PARAM_MESH_CODE("lxrg", "\r lxrg [dst] [app_key_index]\n\r", "light xyl range get\n\r", GEN_MESH_CODE(_light_xyl_range_get))
    PARAM_MESH_CODE("lxrs", "\r lxrs [dst] [xyl_x min] [xyl_x max] [xyl_y min] [xyl_y max] [ack] [app_key_index]\n\r", "light xyl range set\n\r", GEN_MESH_CODE(_light_xyl_range_set))
    PARAM_MESH_CODE("ts", "\r ts [dst] [tai second low] [tai second high] [subsecond] [uncertainty] [time authority] [tai utc delta] [time zone offset] [app_key_index]\n\r", "time set\n\r", GEN_MESH_CODE(_time_set))
    PARAM_MESH_CODE("tg", "\r tg [dst] [app_key_index]\n\r", "time get\n\r", GEN_MESH_CODE(_time_get))
    PARAM_MESH_CODE("tzs", "\r tzs [dst] [time zone offset new] [tai of zone change low] [tai of zone change high] [app_key_index]\n\r", "time zone set\n\r", GEN_MESH_CODE(_time_zone_set))
    PARAM_MESH_CODE("tzg", "\r tzg [dst] [app_key_index]\n\r", "time zone get\n\r", GEN_MESH_CODE(_time_zone_get))
    PARAM_MESH_CODE("ttuds", "\r ttuds [dst] [tai utc delta new] [tai of delta change low] [tai of delta change high] [app_key_index]\n\r", "time tai utc delta set\n\r", GEN_MESH_CODE(_time_tai_utc_delta_set))
    PARAM_MESH_CODE("ttudg", "\r ttudg [dst] [app_key_index]\n\r", "time tai utc delta get\n\r", GEN_MESH_CODE(_time_tai_utc_delta_get))
    PARAM_MESH_CODE("trs", "\r trs [dst] [role] [app_key_index]\n\r", "time role set\n\r", GEN_MESH_CODE(_time_role_set))
    PARAM_MESH_CODE("trg", "\r trg [dst] [app_key_index]\n\r", "time role get\n\r", GEN_MESH_CODE(_time_role_get))
	PARAM_MESH_CODE("ss", "\r ss [dst] [scene number] [ack] [app_key_index]", "\r scene store\n\r", GEN_MESH_CODE(_scene_store))
	PARAM_MESH_CODE("sr", "\r sr [dst] [scene number] [ack] [app_key_index] [steps] [resolution] [delay]", "\r scene recall\n\r", GEN_MESH_CODE(_scene_recall))
	PARAM_MESH_CODE("sg", "\r sg [dst] [app_key_index]", "\r scene get\n\r", GEN_MESH_CODE(_scene_get))
	PARAM_MESH_CODE("srg", "\r srg [dst] [app_key_index]", "\r scene register get\n\r", GEN_MESH_CODE(_scene_register_get))
	PARAM_MESH_CODE("sd", "\r sd [dst] [scene number] [ack] [app_key_index]", "\r scene delete\n\r", GEN_MESH_CODE(_scene_delete))
	PARAM_MESH_CODE("scheg", "\r scheg [dst] [app_key_index]", "\r scheduler get\n\r", GEN_MESH_CODE(_scheduler_get))
	PARAM_MESH_CODE("scheag", "\r scheag [dst] [index] [app_key_index]", "\r scheduler action get\n\r", GEN_MESH_CODE(_scheduler_action_get))
	PARAM_MESH_CODE("scheas", "\r scheag [dst] [index] [app_key_index]", "\r scheduler action get\n\r", GEN_MESH_CODE(_scheduler_action_set))
#if defined(MESH_RPR) && MESH_RPR
	PARAM_MESH_CODE("rmtscan", "\r rmtscan [dst] [net key index] [scanned items limit] [scan timeout] [dev uuid]", "\r romte provision scan start\n\r", GEN_MESH_CODE(_rmt_prov_client_scan_start))
	PARAM_MESH_CODE("rmtcon", "\r rmtcon [dst] [net key index] [dev uuid] [link open timeout]", "\r romte link open for provision\n\r", GEN_MESH_CODE(_rmt_prov_client_link_open_prov))
	PARAM_MESH_CODE("rmtdisc", "\r rmtdisc [dst] [net_key_index] [reason]", "\r romte link close for provision\n\r", GEN_MESH_CODE(_rmt_prov_client_close))
#endif
#if defined(MESH_DFU) && MESH_DFU
    PARAM_MESH_CODE("fuig", "\r fuig [dst] [app_key_index] [first_index] [entries_limit]", "\r firmware update information get\n\r", GEN_MESH_CODE(_fw_update_info_get))
    PARAM_MESH_CODE("fus", "\r fus [dst] [app_key_index] [update ttl] [update timeout base] [blob id] [fw image idx] [fw metadata] [metadata len]", "\r firmware update start\n\r", GEN_MESH_CODE(_fw_update_start))
    PARAM_MESH_CODE("fuc", "\r fuc [dst] [app_key_index]\n\r", "\r firmware update cancel\n\r", GEN_MESH_CODE(_fw_update_cancel))
#endif
#endif
#if (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
    defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
    PARAM_MESH_CODE("nr", "\r nr [mode]\n\r", "\r node reset\n\r", GEN_MESH_CODE(_node_reset))
    PARAM_MESH_CODE("pcs", "\r pcs [public key oob] [static oob] [output size] [output action] [input size] [input action]\n\r", "\r provision capability set\n\r", GEN_MESH_CODE(_prov_capa_set)) 
    PARAM_MESH_CODE("lpninit", "\r lpninit [fn_num]\n\r", "\r low power node init\n\r", GEN_MESH_CODE(_lpn_init)) 
    PARAM_MESH_CODE("lpndeinit", "\r lpndeinit\n\r", "\r low power node deinit\n\r", GEN_MESH_CODE(_lpn_deinit)) 
    PARAM_MESH_CODE("lpnreq", "\r lpnreq [fn_index] [net_key_index] [poll int(100ms)] [poll to(100ms)] [rx delay(ms)] [rx widen(ms)]\n\r", "\r LPN request to estabish a friendship\n\r", GEN_MESH_CODE(_lpn_req)) 
    PARAM_MESH_CODE("lpnsub", "\r lpnsub [fn_index] [addr] [add/rm]\n\r", "\r LPN subsript list add or rm\n\r", GEN_MESH_CODE(_lpn_sub)) 
    PARAM_MESH_CODE("lpnclear", "\r lpnclear [fn_index]\n\r", "\r LPN clear\n\r", GEN_MESH_CODE(_lpn_clear)) 
    PARAM_MESH_CODE("dtn", "\r dtn [conn_id] [value...]\n\r", "\r data transmission notify\n\r", GEN_MESH_CODE(_data_transmission_notify))
#endif
    PARAM_MESH_CODE("dtw", "\r dtw [dst] [data...] [app_key_index] [ack]", "\r data transmission write data\n\r", GEN_MESH_CODE(_datatrans_write))
    PARAM_MESH_CODE("dtr", "\r dtr [dst] [len] [app_key_index]", "\r data transmission read data\n\r", GEN_MESH_CODE(_datatrans_read))
    PARAM_MESH_CODE("con", "\r con [bt addr] [addr type]", "\r connect to remote device\n\r", GEN_MESH_CODE(_connect))
    PARAM_MESH_CODE("disc", "\r disc [conn id]", "\r disconnect to remote device\n\r", GEN_MESH_CODE(_disconnect))
    PARAM_MESH_CODE("ls", "\r ls\n\r", "\rlist node state info\n\r", GEN_MESH_CODE(_list))
    PARAM_MESH_CODE("dis", "\r dis [1 on/0 off]\n\r", "\rdevice information show\n\r", GEN_MESH_CODE(_dev_info_show))
    PARAM_MESH_CODE("fninit", "\r fninit [lpn num] [queue size] [rx window(ms)]\n\r", "\r friend node init\n\r", GEN_MESH_CODE(_fn_init))
    PARAM_MESH_CODE("fndeinit", "\r fndeinit\n\r", "\r friend node deinit\n\r", GEN_MESH_CODE(_fn_deinit))
};

void bt_mesh_io_msg_handler(T_IO_MSG io_msg)
{
    uint8_t ret = 1;
    CMD_ITEM_S *pmeshCmdItem_s = NULL;
    PUSER_ITEM puserItem = NULL;

    pmeshCmdItem_s = (CMD_ITEM_S *)io_msg.u.buf;
    puserItem = (PUSER_ITEM)pmeshCmdItem_s->pmeshCmdItem->userData;
    if ((btMeshCmdPriv.meshMode != BT_MESH_PROVISIONER) && (btMeshCmdPriv.meshMode != BT_MESH_DEVICE)) {
        printf("[BT_MESH] %s(): Error BT MESH mode %d \r\n",__func__);
        goto exit;
    }  
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshMode == BT_MESH_PROVISIONER) {
        if (io_msg.type < MAX_MESH_PROVISIONER_CMD) {
            ret = bt_mesh_user_cmd_hdl(io_msg.type, (CMD_ITEM_S *)io_msg.u.buf);
            if(ret != 0) {
                printf("[BT_MESH] %s(): bt_mesh_io_msg_handler Fail!\r\n",__func__);
            }
            return;
        } else {
            printf("[BT_MESH] %s(): Error mesh code %d \r\n",__func__, io_msg.type);
            goto exit;
        }
    } 
#elif (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
    defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshMode == BT_MESH_DEVICE) {
        if (io_msg.type < MAX_MESH_DEVICE_CMD) {
            ret = bt_mesh_user_cmd_hdl(io_msg.type, (CMD_ITEM_S *)io_msg.u.buf);
            if(ret != 0) {
                printf("[BT_MESH] %s(): bt_mesh_io_msg_handler Fail!\r\n",__func__);
            }
            return;
        } else {
            printf("[BT_MESH] %s(): Error mesh code %d \r\n",__func__, io_msg.type);
            goto exit;
        }
    }
#endif

exit:
    if (pmeshCmdItem_s->userApiMode == USER_API_ASYNCH) {
        bt_mesh_free_hdl(puserItem);
        bt_mesh_cmdunreg(pmeshCmdItem_s);
    } else if (pmeshCmdItem_s->userApiMode == USER_API_CMDLINE) {
        bt_mesh_cmdunreg(pmeshCmdItem_s);
    }

    return;
}

extern int bt_mesh_send_io_msg(T_IO_MSG *p_io_msg);

int bt_mesh_user_cmd(uint16_t mesh_code, void *pmesh_cmd_item_s)
{
    T_IO_MSG io_msg;
    CMD_ITEM_S *pmeshCmdItem_s = (CMD_ITEM_S *)pmesh_cmd_item_s;
    int ret;

    io_msg.type = mesh_code;
	io_msg.u.buf = (void *)pmeshCmdItem_s;
    pmeshCmdItem_s->msgRecvFlag = 0;
	ret = bt_mesh_send_io_msg(&io_msg);

    return ret;
}

uint8_t bt_mesh_user_cmd_hdl(uint16_t mesh_code, CMD_ITEM_S *pmesh_cmd_item_s)
{
    uint8_t ret = 1;
    user_cmd_parse_result_t (*cmd_hdl)(user_cmd_parse_value_t *pparse_value);
    CMD_ITEM_S *pmeshCmdItem_s = (CMD_ITEM_S *)pmesh_cmd_item_s;
    CMD_ITEM *pmeshCmdItem = NULL;
    PUSER_ITEM puserItem = NULL;
    user_cmd_parse_value_t *pparse_value = NULL;

    pmeshCmdItem = pmeshCmdItem_s->pmeshCmdItem;
    puserItem = (PUSER_ITEM)pmeshCmdItem->userData;
    if ((btMeshCmdPriv.meshMode != BT_MESH_PROVISIONER) && (btMeshCmdPriv.meshMode != BT_MESH_DEVICE)) {
        printf("[BT_MESH] %s(): Error BT MESH mode %d \r\n",__func__);
        goto exit;
    }
    if (pmeshCmdItem_s->msgRecvFlag) {
        printf("[BT_MESH] %s(): mesh code %d send msg fail \r\n", __func__, pmeshCmdItem->meshCmdCode);
        goto exit;
    }
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshMode == BT_MESH_PROVISIONER) {
        cmd_hdl = provisionercmds[mesh_code].mesh_func;
    } 
#elif (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
    defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshMode == BT_MESH_DEVICE) {
        cmd_hdl = devicecmds[mesh_code].mesh_func;
    }
#endif
    if (pmeshCmdItem_s->userApiMode != USER_API_ASYNCH && pmeshCmdItem_s->semaDownTimeOut) {
        printf("[BT_MESH] %s(): mesh code %d timeout \r\n", __func__, pmeshCmdItem->meshCmdCode);
        goto exit;
    }

    os_mutex_take(btMeshCmdPriv.ppvalueMutex, 0xFFFFFFFF);
    pparse_value = puserItem->pparseValue;
    if (pparse_value == NULL) {
        printf("[BT_MESH] %s(): pparse_value is NULL!\r\n",__func__);
        os_mutex_give(btMeshCmdPriv.ppvalueMutex);
        goto exit;
    }
    ret = cmd_hdl(pparse_value);
    os_mutex_give(btMeshCmdPriv.ppvalueMutex);

    if (pmeshCmdItem_s->userApiMode == USER_API_CMDLINE) {
        if (!pmeshCmdItem_s->semaDownTimeOut) {
            os_sem_give(puserItem->cmdLineSema);
        }
    }
    if (ret != USER_CMD_RESULT_OK) {
        printf("[BT_MESH] %s(): provisioner cmd fail! (%d)\r\n",__func__,ret);
    }

exit:
    pmeshCmdItem_s->msgRecvFlag = 1;
    if (pmeshCmdItem_s->userApiMode == USER_API_CMDLINE) {
        bt_mesh_cmdunreg(pmeshCmdItem_s);
    } else if (pmeshCmdItem_s->userApiMode == USER_API_ASYNCH) {
        bt_mesh_free_hdl(puserItem);
        bt_mesh_cmdunreg(pmeshCmdItem_s);
    }

    if (btMeshCmdPriv.cmdTransmittedNum) {
        btMeshCmdPriv.cmdTransmittedNum --;
    }
    
    return ret;    
}

uint8_t bt_mesh_enqueue_cmd(struct list_head *queue, uint8_t head_or_tail)
{
    if (!btMeshCmdPriv.meshCmdEnable) {
        printf("[BT_MESH] %s(): Mesh user command is disabled!\r\n", __func__);
		return 1;;
    } 
    os_mutex_take(btMeshCmdPriv.cmdMutex, 0xFFFFFFFF);
    if (head_or_tail) {
        rtw_list_insert_head(queue, &btMeshCmdPriv.meshCmdList);
    } else {
        rtw_list_insert_tail(queue, &btMeshCmdPriv.meshCmdList);
    }
    btMeshCmdPriv.cmdListNum ++;
    os_mutex_give(btMeshCmdPriv.cmdMutex);
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshCmdEnable) {
        os_sem_give(meshProvisionerCmdThread.wakeup_sema);
    }
#elif (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
    defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshCmdEnable) {
        os_sem_give(meshDeviceCmdThread.wakeup_sema);
    }
#endif
    else {
        printf("[BT_MESH] %s(): Mesh user command is disabled\r\n");
        return 2;
    }
    
    return 0;
}

struct list_head *bt_mesh_dequeue_cmd(void)
{
    struct list_head *queue = NULL;
    
    os_mutex_take(btMeshCmdPriv.cmdMutex, 0xFFFFFFFF);
    if (rtw_is_list_empty(&btMeshCmdPriv.meshCmdList)) {
        printf("[BT_MESH] %s(): btMeshCmdPriv.meshCmdList is empty !\r\n", __func__);
        os_mutex_give(btMeshCmdPriv.cmdMutex);
        return NULL;
    } else {
        queue = get_next(&btMeshCmdPriv.meshCmdList);
        rtw_list_delete(queue);
    }
    btMeshCmdPriv.cmdListNum --;
    os_mutex_give(btMeshCmdPriv.cmdMutex); 

    return queue;
}

CMD_ITEM_S* bt_mesh_cmdreg(uint16_t mesh_code, user_cmd_parse_value_t *pparse_value, bt_mesh_func func, user_cmd_cbk cbk, void *user_data)
{
    PUSER_ITEM puserItem = NULL;
    CMD_ITEM   *pmeshCmdItem = NULL;
    CMD_ITEM_S *pmeshCmdItem_s = NULL;

    if (!btMeshCmdPriv.meshCmdEnable) {
        printf("[BT_MESH] %s(): Mesh user command is disabled!\r\n");
		return NULL;
    }
    puserItem = (PUSER_ITEM)user_data;
    pmeshCmdItem = (CMD_ITEM *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(CMD_ITEM));
    if (pmeshCmdItem == NULL) {
        printf("[BT_MESH] %s(): alloc pmeshCmdItem for mesh code %d fail !\r\n", __func__, mesh_code);
        return NULL;
    }
    pmeshCmdItem->meshCmdCode = mesh_code;
    pmeshCmdItem->pparseValue = pparse_value;
    pmeshCmdItem->meshFunc = func;
    pmeshCmdItem->userCbk = cbk;
    pmeshCmdItem->userData = user_data;
    pmeshCmdItem_s = (CMD_ITEM_S *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(CMD_ITEM_S));
    if (pmeshCmdItem_s == NULL) {
        os_mem_free((uint8_t *)pmeshCmdItem);
        printf("[BT_MESH] %s(): alloc pmeshCmdItem_s for mesh code %d fail !\r\n", __func__, mesh_code);
        return NULL;
    }
    pmeshCmdItem_s->pmeshCmdItem = pmeshCmdItem;
    pmeshCmdItem_s->semaDownTimeOut = 0;
    pmeshCmdItem_s->userApiMode = puserItem->userApiMode;
    if (bt_mesh_enqueue_cmd(&pmeshCmdItem_s->list, 0) == 1) {
        printf("[BT_MESH] %s(): enqueue cmd for mesh code %d fail !\r\n", __func__, mesh_code);
        os_mem_free((uint8_t *)pmeshCmdItem);
        os_mem_free((uint8_t *)pmeshCmdItem_s);
        return NULL;
    }

    return pmeshCmdItem_s;
}

uint8_t bt_mesh_cmdunreg(CMD_ITEM_S *pmesh_cmd_item_s)
{
    CMD_ITEM *pmeshCmdItem = NULL;
    CMD_ITEM_S *pmeshCmdItem_s = pmesh_cmd_item_s;

    os_mutex_take(btMeshCmdPriv.cmdItemsMutex, 0xFFFFFFFF);
    pmeshCmdItem = pmeshCmdItem_s->pmeshCmdItem;
    os_mem_free((uint8_t *)pmeshCmdItem);
    os_mem_free((uint8_t *)pmeshCmdItem_s);
    os_mutex_give(btMeshCmdPriv.cmdItemsMutex);

    return 0;
}

user_api_parse_result_t bt_mesh_set_user_cmd(uint16_t mesh_code, user_cmd_parse_value_t *pparse_value, user_cmd_cbk cbk, void *user_data)
{
    PUSER_ITEM puserItem = NULL;
    CMD_ITEM_S *pmeshCmdItem_s = NULL;
    uint8_t ret = USER_API_RESULT_ERROR;
    uint8_t user_api_mode = 0;
    uint32_t time_out = 4000;

    puserItem = (PUSER_ITEM)user_data;
    if (!puserItem) {
        printf("[BT_MESH] %s(): puserItem is null!\r\n");
        return USER_API_RESULT_ERROR;
    }
    if (!btMeshCmdPriv.meshCmdEnable) {
        printf("[BT_MESH] %s(): Mesh user command is disabled!\r\n");
        ret = USER_API_RESULT_NOT_ENABLE;
        goto exit;
    }
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshMode == BT_MESH_PROVISIONER) {
        if (mesh_code == MAX_MESH_PROVISIONER_CMD || mesh_code > MAX_MESH_PROVISIONER_CMD ) {
            printf("[BT_MESH] %s(): user cmd %d illegal !\r\n", __func__, mesh_code);
            ret = USER_API_RESULT_NOT_FOUND;
            goto exit;
        }
    } else {
        printf("[BT_MESH] %s(): Error BT MESH mode %d \r\n",__func__);
        ret = USER_API_RESULT_ERROR_MESH_MODE;
        goto exit;
    }
#elif (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
    defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
    if (btMeshCmdPriv.meshMode == BT_MESH_DEVICE) {
        if (mesh_code == MAX_MESH_DEVICE_CMD || mesh_code > MAX_MESH_DEVICE_CMD ) {
            printf("[BT_MESH] %s(): user cmd %d illegal !\r\n", __func__, mesh_code);
            ret = USER_API_RESULT_NOT_FOUND;
            goto exit;
        }
    } else {
        printf("[BT_MESH] %s(): Error BT MESH mode %d \r\n",__func__);
        ret = USER_API_RESULT_ERROR_MESH_MODE;
        goto exit;
    }
#endif
    user_api_mode = puserItem->userApiMode;
    pmeshCmdItem_s = bt_mesh_cmdreg(mesh_code, pparse_value, bt_mesh_user_cmd, cbk, user_data);
    if (!pmeshCmdItem_s) {
        printf("[BT_MESH] %s(): user cmd %d regiter fail !\r\n", __func__, mesh_code);
        ret = USER_API_RESULT_ERROR;
        goto exit;
    }
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
    /* proivsioning takes more time */
    if (mesh_code == GEN_MESH_CODE(_prov)) {
        time_out = 20000;
    } else {
        time_out = 4000;
    }
#else
    time_out = 4000;
#endif
    if (user_api_mode == USER_API_SYNCH) {
        if (os_sem_take(puserItem->userSema, time_out) == _FAIL) {
            /* if timeout, puserItem will be free by mesh_cmd_thread or bt_mesh_user_cmd_cbk */
            os_mutex_take(btMeshCmdPriv.cmdItemsMutex, 0xFFFFFFFF);
            pmeshCmdItem_s->semaDownTimeOut = 1;
            os_mutex_give(btMeshCmdPriv.cmdItemsMutex);
            printf("[BT_MESH] %s(): user cmd %d timeout !\r\n", __func__, mesh_code);
            ret = USER_API_RESULT_TIMEOUT;
            goto exit;
        }else {
            if (puserItem->userCmdResult) {
                ret = USER_API_RESULT_OK;
                goto exit;
            } else {
                printf("[BT_MESH] %s():mesh cmd = %d FAIL!\r\n", __func__, mesh_code);
                ret = USER_API_RESULT_ERROR;
                goto exit;                     
            }   
        } 
    } else if (user_api_mode == USER_API_CMDLINE) {
        if (os_sem_take(puserItem->cmdLineSema, time_out) == _FAIL) {
            /* if timeout, puserItem will be free by mesh_cmd_thread or bt_mesh_user_cmd_cbk */
            os_mutex_take(btMeshCmdPriv.cmdItemsMutex, 0xFFFFFFFF);
            pmeshCmdItem_s->semaDownTimeOut = 1;
            os_mutex_give(btMeshCmdPriv.cmdItemsMutex);
            printf("[BT_MESH] %s(): user cmd %d timeout !\r\n", __func__, mesh_code);
            ret = USER_API_RESULT_TIMEOUT;
            goto exit;
        }else {
            ret = USER_API_RESULT_OK;
            goto exit;
        } 
    } else {
        /* if timeout, puserItem will be free by mesh_cmd_thread or bt_mesh_user_cmd_cbk */
        return USER_API_RESULT_OK;
    } 

exit:
    bt_mesh_free_hdl(puserItem);
    return ret;
}

user_api_parse_result_t bt_mesh_indication(uint16_t mesh_code, uint8_t state, void *pparam)
{
    CMD_ITEM_S *pmeshCmdItem_s = NULL;
    PUSER_ITEM puserItem;
    uint8_t ret;

    if ((btMeshCmdPriv.meshMode != BT_MESH_PROVISIONER) && (btMeshCmdPriv.meshMode != BT_MESH_DEVICE)) {
        printf("[BT_MESH] %s(): BT mesh indication is not eable !\r\n", __func__);
        return USER_API_RESULT_OK;
    }
    //printf("\r\n %s()",__func__);
    if (btMeshCmdIdPriv.userApiMode != USER_API_SYNCH) {
        printf("[BT_MESH] %s(): BT mesh indication is not USER_API_SYNCH !\r\n", __func__);
        return USER_API_RESULT_OK;
    }
    if (mesh_code != btMeshCmdIdPriv.meshCmdCode) {
        printf("[BT_MESH] %s(): user cmd %d not found !\r\n", __func__, mesh_code);
        //rtw_up_sema(&btMeshCmdPriv.meshThreadSema);
        return USER_API_RESULT_INCORRECT_CODE;//Cause there are several cmd use the same in prov_cb
    }
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) 
    if (btMeshCmdIdPriv.meshCmdCode == GEN_MESH_CODE(_prov)) {
        if ((plt_time_read_ms() < btMeshCmdIdPriv.startTime) || ((plt_time_read_ms() - btMeshCmdIdPriv.startTime) > 10000)) {
            printf("[BT_MESH] %s(): BT mesh code start time is not reasonable !\r\n", __func__);
            return USER_API_RESULT_TIMEOUT;
        }
    } else {
        if ((plt_time_read_ms() < btMeshCmdIdPriv.startTime) || ((plt_time_read_ms() - btMeshCmdIdPriv.startTime) > 2000)) {
            printf("[BT_MESH] %s(): BT mesh code start time is not reasonable !\r\n", __func__);
            return USER_API_RESULT_TIMEOUT;
        }
    }
#elif (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
       defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
    if ((plt_time_read_ms() < btMeshCmdIdPriv.startTime) || ((plt_time_read_ms() - btMeshCmdIdPriv.startTime) > 2000)) {
        printf("[BT_MESH] %s(): BT mesh code start time is not reasonable !\r\n", __func__);
        return USER_API_RESULT_TIMEOUT;
    }
#endif
	os_mutex_take(btMeshCmdPriv.cmdItemsMutex, 0xFFFFFFFF);
    pmeshCmdItem_s = btMeshCmdIdPriv.pmeshCmdItem_s;
    if (pmeshCmdItem_s->semaDownTimeOut) {
        printf("[BT_MESH] %s(): mesh_code %d indication is timeout !\r\n", __func__, mesh_code);
        os_mutex_give(btMeshCmdPriv.cmdItemsMutex);
        return USER_API_RESULT_TIMEOUT;
    }
    if (!pmeshCmdItem_s->msgRecvFlag) {
        printf("[BT_MESH] %s(): This indication is not matched !\r\n", __func__, mesh_code);
        os_mutex_give(btMeshCmdPriv.cmdItemsMutex);
        return USER_API_RESULT_ERROR;
    }
    if (pmeshCmdItem_s->pmeshCmdItem->userData && pmeshCmdItem_s->pmeshCmdItem->userCbk) {
        /* whatever modification to state is user difined, ex: memcpy state to pmeshCmdItem_s->pmeshCmdItem->userData*/
        os_mutex_take(btMeshCmdPriv.ppvalueMutex, 0xFFFFFFFF);
        puserItem = (USER_ITEM *)pmeshCmdItem_s->pmeshCmdItem->userData;
        puserItem->userCmdResult = state;
        printf("\r\n %s() userCmdResult = %d",__func__,puserItem->userCmdResult);
        puserItem->userParam = pparam;
        ret = pmeshCmdItem_s->pmeshCmdItem->userCbk(mesh_code, (void *)pmeshCmdItem_s);
		os_mutex_give(btMeshCmdPriv.ppvalueMutex);
        if (ret == USER_API_RESULT_INDICATION_NOT_MATCHED) {
            printf("[BT_MESH] %s(): user cmd %d not matched !\r\n", __func__, mesh_code);
            os_mutex_give(btMeshCmdPriv.cmdItemsMutex);
            return USER_API_RESULT_ERROR;
        }
    } else {
        printf("[BT_MESH] %s(): user cmd %d pmeshCmdItem_s->pmeshCmdItem->userData is NULL !\r\n", __func__, mesh_code);
    }
    os_mutex_give(btMeshCmdPriv.cmdItemsMutex);
    if (!btMeshCmdIdPriv.semaDownTimeOut) {
        os_sem_give(btMeshCmdPriv.meshThreadSema);
    } else {
        printf("[BT_MESH] %s(): user cmd %d timeout !\r\n", __func__, mesh_code);
    }

    return USER_API_RESULT_OK;
}

PUSER_ITEM bt_mesh_alloc_hdl(uint8_t user_api_mode)
{
    PUSER_ITEM puserItem;

    if (!btMeshCmdPriv.meshCmdEnable) {
        printf("[BT_MESH] Mesh user command is disabled!\r\n");
		return NULL;
    }
    puserItem = (PUSER_ITEM)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(USER_ITEM));
    if (!puserItem) {
        printf("[BT_MESH] PUSER_ITEM alloc fail!\r\n");
		return NULL;
    }
    puserItem->userApiMode = user_api_mode;
    puserItem->pparseValue = (user_cmd_parse_value_t *)os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(user_cmd_parse_value_t));
    if (!puserItem->pparseValue) {
        printf("[BT_MESH] puserItem->pparse_value alloc fail!\r\n");
        os_mem_free((uint8_t *)puserItem);
		return NULL;
    }
    if (user_api_mode == USER_API_SYNCH) {
        os_sem_create(&puserItem->userSema, 0, 0xffffffff);
    	if (puserItem->userSema == NULL) {
    		printf("[BT_MESH] puserItem->user_sema init fail!\r\n");
            os_mem_free((uint8_t *)puserItem->pparseValue);
            os_mem_free((uint8_t *)puserItem);
    		return NULL;
    	}
    }
    if (user_api_mode == USER_API_CMDLINE) {
        os_sem_create(&puserItem->cmdLineSema, 0, 0xffffffff);
    	if (puserItem->cmdLineSema == NULL) {
    		printf("[BT_MESH] puserItem->cmdLineSema init fail!\r\n");
            os_mem_free((uint8_t *)puserItem->pparseValue);
            os_mem_free((uint8_t *)puserItem);
    		return NULL;
    	}
    }

    return puserItem;
}

void bt_mesh_free_hdl(PUSER_ITEM puser_item)
{
    PUSER_ITEM puserItem;
	
	puserItem = puser_item;
    os_mutex_take(btMeshCmdPriv.ppvalueMutex, 0xFFFFFFFF);
    if (puserItem->userApiMode == USER_API_SYNCH) {
        os_sem_delete(puserItem->userSema);
    } else if (puserItem->userApiMode == USER_API_CMDLINE) {
        os_sem_delete(puserItem->cmdLineSema);
    }
    os_mem_free((uint8_t *)puserItem->pparseValue);
    os_mem_free((uint8_t *)puserItem);
    os_mutex_give(btMeshCmdPriv.ppvalueMutex);
}

static void mesh_user_cmd_list(void)
{
    uint8_t i = 0;

    /* find command in table */
    for (i = 0; i < sizeof(mesh_cmd_table) / sizeof(mesh_cmd_table[0]); i ++)
    {
        data_uart_debug(mesh_cmd_table[i].poption);
        data_uart_debug("  *");
        data_uart_debug(mesh_cmd_table[i].phelp);
    };
    
    return;
}

static user_cmd_parse_result_t mesh_user_cmd_parse(user_cmd_parse_value_t *pparse_value, char *mesh_user_cmd_line)
{
    int32_t              i;
    user_cmd_parse_result_t  Result;
    char            *p, *q;

    /* clear all results */
    Result = USER_CMD_RESULT_OK;
    pparse_value->pcommand            = NULL;
    pparse_value->para_count     = 0;
    for (i = 0 ; i < USER_CMD_MAX_PARAMETERS; i ++)
    {
        pparse_value->pparameter[i]     = NULL;
        pparse_value->dw_parameter[i]    = 0;
    }
    /* Parse line */
    p = mesh_user_cmd_line;
    /*ignore leading spaces */
    p = user_cmd_skip_spaces(p);
    if (*p == '\0')                      /* empty command line ? */
    {
        Result = USER_CMD_RESULT_EMPTY_CMD_LINE;
    }
    else
    {
        /* find end of word */
        q = user_cmd_find_end_of_word(p);
        if (p == q)                        /* empty command line ? */
        {
            Result = USER_CMD_RESULT_EMPTY_CMD_LINE;
        }
        else                                /* command found */
        {
            pparse_value->pcommand = p;
            *q = '\0';                        /* mark end of command */
            p = q + 1;
            /* parse parameters */
            if (*p != '\0')                   /* end of line ? */
            {
                uint8_t j = 0;
                do
                {
                    uint32_t d;
                    /* ignore leading spaces */
                    p = user_cmd_skip_spaces(p);
                    d = user_cmd_string2uint32(p);
                    pparse_value->pparameter[j]    = p;
                    pparse_value->dw_parameter[j++] = d;
                    if (j >= USER_CMD_MAX_PARAMETERS)
                    {
                        break;
                    }
                    /* find next parameter */
                    p  = user_cmd_find_end_of_word(p);
                    *p++ = '\0';                        /* mark end of parameter */
                }
                while (*p != '\0');
                pparse_value->para_count = j;
            }
        }
    }

    return (Result);
}

void bt_mesh_param_user_cmd(unsigned int argc, char **argv)
{
    uint8_t i ,j ,k = 0;
    uint8_t found = 0;
    char meshUserCmdLine[USER_CMD_MAX_COMMAND_LINE + 2];
    PUSER_ITEM puserItem = NULL;
    
    if (strcmp(argv[0], "pro") == 0) {
        if (btMeshCmdPriv.meshMode != BT_MESH_PROVISIONER) {
            printf("[BT_MESH] Currently mode is not provisioner\r\n");
            return;
        }
    } else if (strcmp(argv[0], "dev") == 0) {
        if (btMeshCmdPriv.meshMode != BT_MESH_DEVICE) {
            printf("[BT_MESH] Currently mode is not device\r\n");
            return;
        }
    }
    if (strcmp((const char *)argv[1], (const char *)"?") == 0) {
        mesh_user_cmd_list();
        return;
    }
    for (i = 0; i < sizeof(mesh_cmd_table) / sizeof(mesh_cmd_table[0]); i ++) {
		if (strcmp((const char *)argv[1], (const char *)(mesh_cmd_table[i].pcommand)) == 0) {
            if (argc > 2) {
                if (strcmp(argv[2], "?") == 0) {
                    data_uart_debug(mesh_cmd_table[i].poption);
                    data_uart_debug("  *");
                    data_uart_debug(mesh_cmd_table[i].phelp);
                    return;
                }
            }
            k = 0;
            memset(meshUserCmdLine, 0, sizeof(meshUserCmdLine));
            for (j = 1; j < argc; j ++) {
                if (strlen(argv[j]) < (size_t)(USER_CMD_MAX_COMMAND_LINE - k)) {
                    memcpy(&meshUserCmdLine[k], argv[j], strlen(argv[j]));
                    k += strlen(argv[j]);
                    strcpy(&meshUserCmdLine[k++], " ");
                } else {
                    printf("[BT_MESH] No Enough buffer for user cmd\r\n");
                    return;
                }
            }
            puserItem = bt_mesh_alloc_hdl(USER_API_CMDLINE);
            if (!puserItem) {
                printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
        		return;
            }
            mesh_user_cmd_parse(puserItem->pparseValue, meshUserCmdLine);
            bt_mesh_set_user_cmd(mesh_cmd_table[i].meshCode, puserItem->pparseValue, NULL, puserItem);
			found = 1;
			break;
		}
	}
    if (!found) {
        printf("\r\n [BT_MESH] unknown command %s", argv[1]);
    }

    return;
}

extern char *_strncat(char *dest, char const *src, size_t count);

void user_cmd_array2string(uint8_t *buf, unsigned int buflen, char *out)
{
    /* larger than 32 for "/0" */
    char strBuf[40] = {0};
	char pbuf[2];
	uint8_t i;

    if (buflen >16) {
        printf("\r\n [BT_MESH] input pbuf buflen %d is to large", buflen);
        return;
    }
	for(i = 0; i < buflen; i++)
	{
	    sprintf(pbuf, "%02X", buf[i]);
	    strncat(strBuf, pbuf, 2);
	}
	strncpy(out, strBuf, buflen * 2);
}

#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
#include "firmware_distribution.h"
#include "dfudep_service.h"

#if defined(MESH_DFU) && MESH_DFU
uint8_t metadata_client[] = {
    0x1F,
    0xEE
};

typedef uint32_t (*fw_image_data_get_t)(uint32_t len, uint8_t *pout);
extern bool dfu_dist_start(uint16_t dst, uint16_t app_key_index, uint16_t update_timeout_base,
                    uint8_t *pfw_metadata, uint8_t metadata_len,
                    uint32_t fw_image_size, fw_image_data_get_t fw_image_data_get);

extern uint8_t dfu_dist_model_enabled;

void bt_mesh_dfu_param_user_cmd(unsigned int argc, char **argv)
{
    uint16_t dst;
    uint16_t app_key_index;
    uint16_t update_timeout_base;
    uint32_t blob_size;

    if (!dfu_dist_model_enabled) {
        printf("[BT_MESH] device firmware update model is not enabled \r\n");
        return;
    }
    if (strcmp((const char *)argv[1], (const char *)"?") == 0) {
        printf("[BT_MESH] ATBM=mesh_ota,dst,app_key_index,update_timeout_base,blob_size \r\n");
        printf("[BT_MESH] Start firmware update procedure to node \r\n");
        return;
    }
    if (argc != 5) {
        printf("[BT_MESH] Param num is incorrect, Please tap in >> ATBM = mesh_ota,? << for help \r\n");
        return;
    }
    dst = (uint16_t)user_cmd_string2uint32(argv[1]);
    app_key_index = (uint16_t)user_cmd_string2uint32(argv[2]);
    update_timeout_base = (uint16_t)user_cmd_string2uint32(argv[3]);
    blob_size = (uint32_t)user_cmd_string2uint32(argv[4]);
    dfu_dist_start(dst, app_key_index, update_timeout_base, (uint8_t *)metadata_client, sizeof(metadata_client), blob_size, 
																			(fw_image_data_get_t)dfu_fw_image_data_get);
}
#endif
#endif
#endif
#endif
