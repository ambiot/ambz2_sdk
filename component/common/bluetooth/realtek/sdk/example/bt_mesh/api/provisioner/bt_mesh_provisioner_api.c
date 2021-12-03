/**
*****************************************************************************************
*     Copyright(c) 2019, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     bt_mesh_provisioner_api.c
  * @brief    Source file for provisioner cmd.
  * @details  User command interfaces.
  * @author   sherman
  * @date     2019-09-16
  * @version  v1.0
  * *************************************************************************************
  */
#include "bt_mesh_provisioner_api.h"

#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
    defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)

struct mesh_task_struct   meshProvisionerCmdThread;
extern CMD_MOD_INFO_S     btMeshCmdPriv;
extern INDICATION_ITEM    btMeshCmdIdPriv;

thread_return mesh_provisioner_cmd_thread(thread_context context)
{
    /* avoid gcc compile warning */
    (void)context;
	uint8_t count = 0;
    uint32_t time_out = 2000;
    uint16_t mesh_code = MAX_MESH_PROVISIONER_CMD;
    CMD_ITEM *pmeshCmdItem = NULL;
    CMD_ITEM_S *pmeshCmdItem_s = NULL;
    PUSER_ITEM puserItem = NULL;
    uint8_t userApiMode = USER_API_DEFAULT_MODE;
    //user_cmd_parse_value_t *pparse_value = NULL;
    struct list_head *plist;
	struct mesh_task_struct *pcmdtask = &(meshProvisionerCmdThread);    
    
    printf("[BT_MESH] %s(): mesh cmd thread enter !\r\n", __func__);

	while(1)
	{		
		if (os_sem_take(pcmdtask->wakeup_sema, 0xFFFFFFFF) == _FAIL) {
            printf("[BT_MESH] %s(): down wakeup_sema fail !\r\n", __func__);
            break;
		}
		if (pcmdtask->blocked == _TRUE) {
            /* wait mesh stack handling mesh cmds */
            while (btMeshCmdPriv.cmdTransmittedNum) {
                printf(" waiting mesh command's implementation by mesh stack \r\n");
                os_delay(100);
            }
			printf("[BT_MESH] %s(): blocked(%d) !\r\n", __func__, pcmdtask->blocked);
			break;
		}
        plist = bt_mesh_dequeue_cmd();
        if (!plist) {
            printf("[BT_MESH] %s(): bt_mesh_dequeue_cmd fail !\r\n", __func__);
			goto _next;
        }
        pmeshCmdItem_s = (CMD_ITEM_S *)plist;
        pmeshCmdItem = pmeshCmdItem_s->pmeshCmdItem;
        mesh_code = pmeshCmdItem->meshCmdCode;
        //pparse_value = pmeshCmdItem->pparseValue;
        puserItem = (PUSER_ITEM)pmeshCmdItem->userData;
        userApiMode = pmeshCmdItem_s->userApiMode;
        if (pmeshCmdItem_s->semaDownTimeOut) {
            printf("[BT_MESH] %s(): mesh cmd %d has already timeout !\r\n", __func__, mesh_code);
            bt_mesh_cmdunreg(pmeshCmdItem_s);
            goto _next;
        }
        BT_MESH_CMD_ID_PRIV_MOD(mesh_code, pmeshCmdItem_s, puserItem->userApiMode, 1, plt_time_read_ms());
        if (pmeshCmdItem_s->pmeshCmdItem->meshFunc) {
            if (pmeshCmdItem_s->pmeshCmdItem->meshFunc(mesh_code, pmeshCmdItem_s)) {
                if (userApiMode == USER_API_ASYNCH) {
                    bt_mesh_free_hdl(puserItem);
                    bt_mesh_cmdunreg(pmeshCmdItem_s);
                    BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
                    goto _next;
                } else if (userApiMode == USER_API_CMDLINE) {
                    bt_mesh_cmdunreg(pmeshCmdItem_s);
                    BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
                    goto _next;
                }
            }
            if (userApiMode != USER_API_SYNCH) {
                BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
                goto _next;
            }
            btMeshCmdIdPriv.semaDownTimeOut = 0;
            /* proivsioning takes more time */
            if (mesh_code == GEN_MESH_CODE(_prov)) {
                time_out = 10000;
            } else {
                time_out = 2000;
            }
            /* guarantee the integrity of each mesh_code cmd */
            if (os_sem_take(btMeshCmdPriv.meshThreadSema, time_out) == _FAIL) {
                count++;
                BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
                /* guarantee that previous msg has already been handled */
                while (!pmeshCmdItem_s->msgRecvFlag) {
                    printf("[BT_MESH] %s(): Wait bt_mesh_user_cmd_hdl implementation %d, %d !\r\n", __func__, mesh_code, btMeshCmdPriv.cmdTransmittedNum);
                    os_delay(100);
                }
                if (count == BT_MESH_PROV_CMD_RETRY_COUNT) {
                    count = 0;
                    printf("[BT_MESH] %s(): down sema timeout mesh cmd = %d !\r\n", __func__, mesh_code);
                    bt_mesh_cmdunreg(pmeshCmdItem_s);
                } else {
                    /* enqueue to head for another try */
                    printf("\r\n enqueue again 0x%x \r\n", plist);
                    if (bt_mesh_enqueue_cmd(plist, 1) == 1) {
                        printf("[BT_MESH] %s(): enqueue cmd for mesh code %d fail !\r\n", __func__, mesh_code);
                        bt_mesh_cmdunreg(pmeshCmdItem_s);
                    }
                }
            }else {
                count = 0;
                if (pmeshCmdItem_s->semaDownTimeOut) {
                    printf("[BT_MESH] %s(): mesh_code %d indication is timeout !\r\n", __func__, mesh_code);
                    bt_mesh_cmdunreg(pmeshCmdItem_s);
                } else {
                    os_sem_give(puserItem->userSema);
                    printf("[BT_MESH] %s():mesh cmd = %d puserItem->userCmdResult = %d !\r\n", __func__, mesh_code, puserItem->userCmdResult);
                    bt_mesh_cmdunreg(pmeshCmdItem_s);
                    BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
                }
            }    
        } else {
            printf("[BT_MESH] %s(): meshFunc is NULL mesh_code = %d !\r\n", __func__, mesh_code);
            bt_mesh_cmdunreg(pmeshCmdItem_s);
            BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
        } 
_next:
        /* make sure os shedule can switch out */
        os_delay(10);
	}
    /* prevent previous cmd involking bt_mesh_indication */
    BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
	/* free all pmeshCmdItem_s resources */
	do{
		plist = bt_mesh_dequeue_cmd();
		if (plist == NULL) {
            break;
        }
		bt_mesh_cmdunreg((CMD_ITEM_S *)plist);
	}while(1);	
	os_sem_give(pcmdtask->terminate_sema);
    printf("[BT_MESH] %s(): mesh cmd thread exit !\r\n", __func__);
    os_task_delete(NULL);
}

uint8_t bt_mesh_provisioner_api_init(void)
{
    if (btMeshCmdPriv.meshMode == BT_MESH_PROVISIONER) {
        printf("[BT_MESH] %s(): MESH PROVISIONER is already running !\r\n", __func__);
        return 1;
    } else if (btMeshCmdPriv.meshMode == BT_MESH_DEVICE) {
        printf("[BT_MESH] %s(): MESH Device  is running, need deinitialization firstly !\r\n", __func__);
        return 1;
    }
    rtw_init_listhead(&btMeshCmdPriv.meshCmdList);
    os_mutex_create(&btMeshCmdPriv.cmdMutex);
    os_mutex_create(&btMeshCmdPriv.ppvalueMutex);
	os_mutex_create(&btMeshCmdPriv.cmdItemsMutex);
    os_sem_create(&btMeshCmdPriv.meshThreadSema, 0, 0xffffffff);
    BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
    meshProvisionerCmdThread.blocked = 0;
    os_sem_create(&meshProvisionerCmdThread.wakeup_sema, 0, 0xffffffff);
	os_sem_create(&meshProvisionerCmdThread.terminate_sema, 0, 0xffffffff);
    if (os_task_create(&meshProvisionerCmdThread.task, "mesh_provisioner_cmd_thread", mesh_provisioner_cmd_thread,
                    NULL, 1024, 3) != true) {
        meshProvisionerCmdThread.blocked = 1;
        os_sem_delete(meshProvisionerCmdThread.wakeup_sema);
        os_sem_delete(meshProvisionerCmdThread.terminate_sema);
        printf("[BT_MESH] %s(): create mesh_provisioner_cmd_thread fail !\r\n", __func__);
        os_mutex_delete(btMeshCmdPriv.cmdMutex);
        os_sem_delete(btMeshCmdPriv.meshThreadSema);
        return 1;
    }
    btMeshCmdPriv.cmdListNum = 0;
    btMeshCmdPriv.cmdTransmittedNum = 0;
    btMeshCmdPriv.meshMode = BT_MESH_PROVISIONER;
    btMeshCmdPriv.meshCmdEnable = 1;

    return 0;
}

void bt_mesh_provisioner_api_deinit(void)
{
    if (btMeshCmdPriv.meshMode != BT_MESH_PROVISIONER) {
        printf("[BT_MESH] %s(): MESH PROVISIONER is not running !\r\n", __func__);
        return;
    }
    btMeshCmdPriv.meshCmdEnable = 0;
    BT_MESH_CMD_ID_PRIV_MOD(MAX_MESH_PROVISIONER_CMD, NULL, USER_API_DEFAULT_MODE, 1, 0);
    meshProvisionerCmdThread.blocked = 1;
	os_sem_give(meshProvisionerCmdThread.wakeup_sema);
	os_sem_take(meshProvisionerCmdThread.terminate_sema, 0xFFFFFFFF);
    btMeshCmdPriv.meshMode = 0;
	os_sem_delete(meshProvisionerCmdThread.wakeup_sema);
    os_sem_delete(meshProvisionerCmdThread.terminate_sema);
    os_mutex_delete(btMeshCmdPriv.cmdMutex);
    os_mutex_delete(btMeshCmdPriv.ppvalueMutex);
	os_mutex_delete(btMeshCmdPriv.cmdItemsMutex);
    os_sem_delete(btMeshCmdPriv.meshThreadSema);
}

#endif
#endif
