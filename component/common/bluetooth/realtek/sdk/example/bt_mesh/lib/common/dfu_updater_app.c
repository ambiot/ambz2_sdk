/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      dfu_updater_app.c
* @brief     Smart mesh dfu application
* @details
* @author    bill
* @date      2018-6-5
* @version   v1.0
* *********************************************************************************************************
*/

#include "dfu_updater_app.h"
#include "ftl.h"
#include "generic_types.h"
#include "dfudep_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t metadata_server[] = {
    0x1F,
    0xEE
};

#if MESH_DFU

#if DFU_UPDATER_SUPPORT_POWER_OFF_GO_ON /* used for device power off but client is alive */
typedef struct
{
    uint8_t blob_id[8];
    uint32_t blob_size;
    uint32_t block_size;
    uint32_t current_block_num; /**< shall be 4 bytes alligned */
    uint16_t transfer_timeout_base;
    uint16_t transfer_ttl;
    uint16_t transfer_mtu_size;
    uint16_t updater_addr;
    uint16_t updater_app_key_index;
    uint16_t image_id;
    uint8_t transfer_mode;
    uint8_t padding[3];
} dfu_updater_nvm_info_t;
static dfu_updater_nvm_info_t dfu_updater_nvm_info;

void dfu_updater_nvm_load(void)
{
    uint32_t ret = ftl_load(&dfu_updater_nvm_info, DFU_UPDATER_NVM_OFFSET,
                            sizeof(dfu_updater_nvm_info));
    if (ret == 0 && dfu_updater_nvm_info.blob_size != 0)
    {
        printf("dfu_updater: power on go on! \r\n");
    }
    else
    {
        dfu_updater_nvm_info.blob_size = 0;
    }
}

void dfu_updater_nvm_clear(void)
{
    uint32_t ret;
    ret = ftl_load(&dfu_updater_nvm_info.blob_size,
                   DFU_UPDATER_NVM_OFFSET + MEMBER_OFFSET(dfu_updater_nvm_info_t, blob_size), 4);
    if (ret == 0 && dfu_updater_nvm_info.blob_size != 0)
    {
        dfu_updater_nvm_info.blob_size = 0;
        ftl_save(&dfu_updater_nvm_info.blob_size,
                 DFU_UPDATER_NVM_OFFSET + MEMBER_OFFSET(dfu_updater_nvm_info_t, blob_size), 4);
    }
}
#endif

struct
{
    uint16_t image_id;
} dfu_updater_app_ctx;


#if DFU_UPDATER_SUPPORT_POWER_OFF_GO_ON
void dfu_updater_load(void)
{
    if (UNPROV_DEVICE == mesh_node_state_restore())
    {
        dfu_updater_nvm_clear();
    }
    else
    {
        dfu_updater_nvm_load();
        if (dfu_updater_nvm_info.blob_size != 0)
        {
            dfu_updater_app_ctx.image_id = dfu_updater_nvm_info.image_id;
            if (fw_update_server_load(NULL, 0, 0, dfu_updater_nvm_info.transfer_ttl,
                                      dfu_updater_nvm_info.transfer_timeout_base, dfu_updater_nvm_info.blob_id))
            {
                blob_transfer_server_load(dfu_updater_nvm_info.blob_size, dfu_updater_nvm_info.block_size,
                                          dfu_updater_nvm_info.current_block_num,
                                          dfu_updater_nvm_info.transfer_mode, dfu_updater_nvm_info.transfer_mtu_size,
                                          dfu_updater_nvm_info.updater_addr,
                                          dfu_updater_nvm_info.updater_app_key_index);
            }
        }
    }
}

void dfu_updater_clear(void)
{
    dfu_updater_nvm_clear();
    fw_update_server_clear();
    blob_transfer_server_clear();
}
#endif

static int32_t dfu_update_server_data(const mesh_model_info_p pmodel_info, uint32_t type,
                                      void *pargs)
{
    switch (type)
    {
    case FW_UPDATE_SERVER_METADATA_CHECK:
        {
            /* check whether metadata received is matched with local one*/
            /* return value
                check fail:    FW_UPDATE_STATUS_METADATA_CHECK_FAILED
                check success: FW_UPDATE_STATUS_SUCCESS
                invalid index: this case has been checked within mesh stack*/
            fw_update_server_metadata_check_t *pdata = (fw_update_server_metadata_check_t *)pargs;
            {
                uint8_t *pmetadata = pdata->pmetadata;
                *(pdata->pstatus) = FW_UPDATE_STATUS_SUCCESS;

                if (pmetadata == NULL) {
                    printf("%s:Metadata is not present!\r\n", __func__);
                    *(pdata->pstatus) = FW_UPDATE_STATUS_METADATA_CHECK_FAILED;
                    break;
                }

                if (dfu_metadata_check(pmetadata, pdata->metadata_len) != DFU_RESULT_OK) {
                    printf("%s:Metadata check fail!\r\n", __func__);
                    *(pdata->pstatus) = FW_UPDATE_STATUS_METADATA_CHECK_FAILED;
                    break;
                } else {
                    printf("%s:Metadata check success!\r\n", __func__);
                }
            }  
        }
        break;
    case FW_UPDATE_SERVER_START:
        {
            fw_update_server_start_t *pdata = (fw_update_server_start_t *)pargs;
 
            /* A condition on the server is preventing the update (e.g., low battery level)*/
            /* return value
                check fail:    DFU_RESULT_FAIL
                check success: DFU_RESULT_OK */
            if (is_dfu_enabled() != DFU_RESULT_OK) {
                printf("%s:Currently is unavailable\r\n", __func__);
                *(pdata->pcan_update) = false;
                break;
            }

            /* check whether there is sufficient resources to store the firmware*/
            /* return value
                check fail:    DFU_RESULT_FAIL
                check success: DFU_RESULT_OK */
            if (dfu_resources_check() != DFU_RESULT_OK) {
                printf("%s:There is insufficient resources to store the firmware \r\n", __func__);
                *(pdata->pcan_update) = false;
                break;
            }

#if DFU_UPDATER_SUPPORT_POWER_OFF_GO_ON
            memcpy(dfu_updater_nvm_info.blob_id, blob_transfer_server_ctx.blob_id, 8);
            dfu_updater_nvm_info.blob_size = blob_transfer_server_ctx.blob_size;
            dfu_updater_nvm_info.block_size = blob_transfer_server_ctx.block_size;
            dfu_updater_nvm_info.current_block_num = 0;
            dfu_updater_nvm_info.transfer_timeout_base = blob_transfer_server_ctx.transfer_timeout_base;
            dfu_updater_nvm_info.transfer_ttl = blob_transfer_server_ctx.transfer_ttl;
            dfu_updater_nvm_info.transfer_mtu_size = blob_transfer_server_ctx.transfer_mtu_size;
            dfu_updater_nvm_info.updater_addr = blob_transfer_server_ctx.updater_addr;
            dfu_updater_nvm_info.updater_app_key_index = blob_transfer_server_ctx.updater_app_key_index;
            dfu_updater_nvm_info.transfer_mode = blob_transfer_server_ctx.mode;
            ftl_save(&dfu_updater_nvm_info, DFU_UPDATER_NVM_OFFSET, sizeof(dfu_updater_nvm_info));
#endif
        }
        break;
    case FW_UPDATE_SERVER_VERIFY:
        {
            /* check firmware specific action*/
            bool check_result = false;

            if (dfu_verify(dfu_updater_app_ctx.image_id) == DFU_RESULT_OK) {
                check_result = true;
            }

#if DFU_UPDATER_SUPPORT_POWER_OFF_GO_ON
            dfu_updater_nvm_clear();
#endif
            if (check_result)
            {
                printf("dfu_update_server_data: update success, verify pass! \r\n");
            }
            else
            {
                printf("dfu_update_server_data: verify fail! \r\n");
            }
            fw_update_server_set_verify_result(check_result);
        }
        break;
    case FW_UPDATE_SERVER_VERIFY_CANCEL:
        /* mesh stack will operate the specific actions */
        dfu_verify_cancel();
        break;
    case FW_UPDATE_SERVER_APPLY:
        /* operate the reload latest firmware action*/
        printf("dfu_update_server_data: FW update Apply! \r\n");
        dfu_apply(0, 4000);
        break;
    case FW_UPDATE_SERVER_BLOCK_DATA:
        {
            fw_update_server_block_data_t *pdata = (fw_update_server_block_data_t *)pargs;

            if (pdata->block_num == 0) {
                /* maybe do some firmware update resouces initialization*/
                if (dfu_init_dfu_resources() != DFU_RESULT_OK) {
                    printf("%s:Initialization of dfu resources fail \r\n", __func__);
                    fw_update_server_clear();
                    dfu_failsafe();
                    break;
                }
            }

            /* saving the block image file*/
            /* firstly, if necessary, check block integrity */
            if (dfu_block_verify(pdata->pdata, pdata->data_len, DFU_BLOCK_SIGNATURE_SIZE) != DFU_RESULT_OK) {
                printf("dfu_update_server_data: block check fail ! \r\n");
                fw_update_server_clear();
                dfu_failsafe();
                break;
            }
            /* flash image file */
            if (dfu_block_data_restore(pdata->pdata, pdata->data_len, DFU_BLOCK_SIGNATURE_SIZE) != DFU_RESULT_OK) {
                printf("dfu_update_server_data: flash block %d fail ! \r\n", pdata->block_num);
                fw_update_server_clear();
                dfu_failsafe();
                break;
            }
            
            if (pdata->block_num == 0)
            {
                printf("dfu_update_server_data: Received the first block data! \r\n");
                dfu_updater_app_ctx.image_id = 0x0001;
#if DFU_UPDATER_SUPPORT_POWER_OFF_GO_ON
                dfu_updater_nvm_info.current_block_num = 1;
                dfu_updater_nvm_info.image_id = dfu_updater_app_ctx.image_id;
                ftl_save(&dfu_updater_nvm_info, DFU_UPDATER_NVM_OFFSET, sizeof(dfu_updater_nvm_info));
#endif
            }
            else
            {
                printf("dfu_update_server_data: ========Received one block %d ========= \r\n", pdata->block_num);
#if DFU_UPDATER_SUPPORT_POWER_OFF_GO_ON
                dfu_updater_nvm_info.current_block_num = pdata->block_num + 1;
                ftl_save(&dfu_updater_nvm_info.current_block_num,
                         DFU_UPDATER_NVM_OFFSET + MEMBER_OFFSET(dfu_updater_nvm_info_t, current_block_num), 4);
#endif
            }
        }
        break;
    case FW_UPDATE_SERVER_FAIL:
        {
            dfu_failsafe();
#if DFU_UPDATER_SUPPORT_POWER_OFF_GO_ON
            dfu_updater_nvm_clear();
#endif
        }
        break;
    default:
        break;
    }

    return MODEL_SUCCESS;
}

void dfu_updater_models_init(void)
{
    fw_update_server_reg(0, dfu_update_server_data);
    blob_transfer_server_reg(0, fw_update_handle_blob_server_data);

    /* Firstly, read firmware info from specific flash room and call fw_update_server_add_info to notify the mesh stack*/
    {
        fw_info_t fw_info;

        dfu_get_fw_info(&fw_info);
        if (fw_info.update_uri_len) {
            fw_update_server_add_info(&fw_info.fw_id, fw_info.fw_id_len, fw_info.update_uri, fw_info.update_uri_len);
        } else {
            fw_update_server_add_info(&fw_info.fw_id, fw_info.fw_id_len, NULL, 0);
        }
        
    } 
}

#endif /* MESH_DFU */
