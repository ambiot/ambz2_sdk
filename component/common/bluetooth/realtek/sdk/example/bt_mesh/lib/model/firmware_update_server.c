/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     firmware_update_server.c
* @brief    Source file for firmware update server model.
* @details  Data types and external functions declaration.
* @author   bill
* @date     2018-5-21
* @version  v1.0
* *************************************************************************************
*/

#define MM_ID MM_MODEL

/* Add Includes here */
#include <string.h>
#include "mesh_api.h"
#include "firmware_update.h"
//#include "object_transfer.h"

mesh_model_info_t fw_update_server;
fw_update_server_ctx_t fw_update_server_ctx;

static mesh_msg_send_cause_t fw_update_server_send(mesh_msg_p pmesh_msg, uint8_t *pmsg,
                                                   uint16_t len)
{
    mesh_msg_t mesh_msg;
    mesh_msg.pmodel_info = pmesh_msg->pmodel_info;
    access_cfg(&mesh_msg);
    mesh_msg.pbuffer = pmsg;
    mesh_msg.msg_len = len;
    mesh_msg.dst = pmesh_msg->src;
    mesh_msg.app_key_index = pmesh_msg->app_key_index;
    return access_send(&mesh_msg);
}

mesh_msg_send_cause_t fw_info_stat(mesh_msg_p pmesh_msg, uint16_t company_id,
                                   fw_update_fw_id_t firmware_id,
                                   uint8_t update_url[], uint16_t url_len)
{
    mesh_msg_send_cause_t ret;
    fw_info_stat_t *pmsg = (fw_info_stat_t *)plt_malloc(MEMBER_OFFSET(fw_info_stat_t,
                                                                      update_url) + url_len, RAM_TYPE_DATA_OFF);
    if (pmsg == NULL)
    {
        return MESH_MSG_SEND_CAUSE_NO_MEMORY;
    }
    ACCESS_OPCODE_BYTE(pmsg->opcode, MESH_MSG_FW_INFO_STAT);
    pmsg->company_id = company_id;
    FW_UPDATE_FW_ID(pmsg->firmware_id, firmware_id);
    memcpy(pmsg->update_url, update_url, url_len);
    ret = fw_update_server_send(pmesh_msg, (uint8_t *)pmsg, MEMBER_OFFSET(fw_info_stat_t,
                                                                          update_url) + url_len);
    plt_free(pmsg, RAM_TYPE_DATA_OFF);
    return ret;
}

mesh_msg_send_cause_t fw_update_stat(mesh_msg_p pmesh_msg, fw_update_stat_stat_t stat,
                                     uint8_t phase, uint8_t addi_info, uint16_t company_id, fw_update_fw_id_t firmware_id,
                                     uint8_t object_id[8])
{
    fw_update_stat_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_FW_UPDATE_STAT);
    msg.stat = stat;
    msg.phase = phase;
    msg.addi_info = addi_info;
    msg.company_id = company_id;
    FW_UPDATE_FW_ID(msg.firmware_id, firmware_id);
    if (object_id)
    {
        memcpy(msg.object_id, object_id, sizeof(msg.object_id));
    }
    return fw_update_server_send(pmesh_msg, (uint8_t *)&msg,
                                 sizeof(fw_update_stat_t) - (object_id ? 0 : sizeof(msg.object_id)));
}

static fw_update_stat_stat_t fw_update_server_apply(void)
{
    fw_update_stat_stat_t stat;
    if (0 > fw_update_server.model_data_cb(&fw_update_server, FW_UPD_SERVER_CB_TYPE_APPLY,
                                           &fw_update_server_ctx))
    {
        stat = FW_UPDATE_STAT_COMPANY_FIRMWARE_APPLY_FAILED;
    }
    else
    {
        stat = FW_UPDATE_STAT_SUCCESS;
    }

    fw_update_server_ctx.phase = FW_UPDATE_PHASE_IDLE;
    if (fw_update_server_ctx.vendor_validate_data)
    {
        plt_free(fw_update_server_ctx.vendor_validate_data, RAM_TYPE_DATA_ON);
        fw_update_server_ctx.vendor_validate_data = NULL;
    }
    return stat;
}

void fw_update_server_handle_fw_update_prepare(mesh_msg_p pmesh_msg)
{
    fw_update_stat_stat_t stat;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    fw_update_prepare_t *pmsg = (fw_update_prepare_t *)pbuffer;

    if (pmsg->company_id != fw_update_server_ctx.company_id)
    {
        stat = FW_UPDATE_STAT_WRONG_COMPANY_FIRMWARE_COMBINATION;
        goto end;
    }

    //if (pmsg->firmware_id <= fw_update_server_ctx.firmware_id)
    for (int8_t loop = FW_UPDATE_FW_ID_LEN - 1; loop >= 0; loop--)
    {
        if (pmsg->firmware_id[loop] > fw_update_server_ctx.firmware_id[loop])
        {
            break;
        }
        else if (pmsg->firmware_id[loop] < fw_update_server_ctx.firmware_id[loop])
        {
            stat = FW_UPDATE_STAT_NEWER_FW_VERSION_PRESENT;
            goto end;
        }
        else if (loop == 0)
        {
            stat = FW_UPDATE_STAT_NEWER_FW_VERSION_PRESENT;
            goto end;
        }
    }

    if (fw_update_server_ctx.phase != FW_UPDATE_PHASE_IDLE)
    {
        if (0 != memcmp(fw_update_server_ctx.object_id, pmsg->object_id,
                        sizeof(fw_update_server_ctx.object_id)))
        {
            //stat = FW_UPDATE_STAT_DIFF_OBJECT_TRANSFER_ALREADY_ONGOING;
            //goto end;
            if (fw_update_server.model_data_cb)
            {
                fw_update_server.model_data_cb(&fw_update_server, FW_UPD_SERVER_CB_TYPE_ABORT,
                                               &fw_update_server_ctx);
                printw("fw_update_server_handle_fw_update_prepare: fw update preemtived!");
            }
            else
            {
                stat = FW_UPDATE_STAT_DIFF_OBJECT_TRANSFER_ALREADY_ONGOING;
                goto end;
            }
        }
        else
        {
            /* duplicate prepare message, what to do? */
            stat = FW_UPDATE_STAT_SUCCESS;
            goto end;
        }
    }
    else
    {
        if (fw_update_server.model_data_cb)
        {
            if (0 > fw_update_server.model_data_cb(&fw_update_server, FW_UPD_SERVER_CB_TYPE_PREPARE,
                                                   &fw_update_server_ctx))
            {
                stat = FW_UPDATE_STAT_DIFF_OBJECT_TRANSFER_ALREADY_ONGOING;
                goto end;
            }
        }
        else
        {
            stat = FW_UPDATE_STAT_NOT_AVAILABLE;
            goto end;
        }
    }

    stat = FW_UPDATE_STAT_SUCCESS;
    fw_update_server_ctx.phase = FW_UPDATE_PHASE_PREPARED;
    FW_UPDATE_FW_ID(fw_update_server_ctx.firmware_id_new, pmsg->firmware_id);
    memcpy(fw_update_server_ctx.object_id, pmsg->object_id, sizeof(fw_update_server_ctx.object_id));
    fw_update_server_ctx.validate_len = pmesh_msg->msg_len - MEMBER_OFFSET(fw_update_prepare_t,
                                                                           vendor_validate_data);
    if (fw_update_server_ctx.validate_len)
    {
        if (fw_update_server_ctx.vendor_validate_data)
        {
            plt_free(fw_update_server_ctx.vendor_validate_data, RAM_TYPE_DATA_ON);
        }
        fw_update_server_ctx.vendor_validate_data = plt_malloc(fw_update_server_ctx.validate_len,
                                                               RAM_TYPE_DATA_ON);
        if (0 == fw_update_server_ctx.vendor_validate_data)
        {
            fw_update_server_ctx.phase = FW_UPDATE_PHASE_IDLE;
            stat = FW_UPDATE_STAT_NOT_AVAILABLE;
            goto end;
        }
        memcpy(fw_update_server_ctx.vendor_validate_data, pmsg->vendor_validate_data,
               fw_update_server_ctx.validate_len);
    }
end:
    fw_update_server_ctx.stat = stat;
    fw_update_stat(pmesh_msg, stat, fw_update_server_ctx.phase, fw_update_server_ctx.addi_info_8,
                   fw_update_server_ctx.company_id, fw_update_server_ctx.firmware_id_new,
                   fw_update_server_ctx.object_id);
}

void fw_update_server_handle_fw_update_start(mesh_msg_p pmesh_msg)
{
    fw_update_stat_stat_t stat;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    fw_update_start_t *pmsg = (fw_update_start_t *)pbuffer;

    if (pmsg->company_id != fw_update_server_ctx.company_id)
    {
        stat = FW_UPDATE_STAT_WRONG_COMPANY_FIRMWARE_COMBINATION;
        goto end;
    }

    //if (pmsg->firmware_id != fw_update_server_ctx.firmware_id_new)
    if (0 != memcmp(pmsg->firmware_id, fw_update_server_ctx.firmware_id_new, FW_UPDATE_FW_ID_LEN))
    {
        stat = FW_UPDATE_STAT_NOT_AVAILABLE;
        goto end;
    }

    if (fw_update_server_ctx.phase != FW_UPDATE_PHASE_PREPARED &&
        fw_update_server_ctx.phase != FW_UPDATE_PHASE_IN_PROGRESS)
    {
        stat = FW_UPDATE_STAT_NOT_AVAILABLE;
        goto end;
    }

    stat = FW_UPDATE_STAT_SUCCESS;
    if (fw_update_server_ctx.phase == FW_UPDATE_PHASE_IN_PROGRESS)
    {
        goto end;
    }
    fw_update_server_ctx.phase = FW_UPDATE_PHASE_IN_PROGRESS;
    fw_update_server_ctx.policy = pmsg->policy;
    if (fw_update_server.model_data_cb)
    {
        fw_update_server.model_data_cb(&fw_update_server, FW_UPD_SERVER_CB_TYPE_START,
                                       &fw_update_server_ctx);
    }
end:
    fw_update_server_ctx.stat = stat;
    fw_update_stat(pmesh_msg, stat, fw_update_server_ctx.phase, fw_update_server_ctx.addi_info_8,
                   fw_update_server_ctx.company_id, fw_update_server_ctx.firmware_id, fw_update_server_ctx.object_id);
}

void fw_update_server_handle_fw_update_abort(mesh_msg_p pmesh_msg)
{
    fw_update_stat_stat_t stat;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    fw_update_abort_t *pmsg = (fw_update_abort_t *)pbuffer;

    if (pmsg->company_id != fw_update_server_ctx.company_id)
    {
        stat = FW_UPDATE_STAT_WRONG_COMPANY_FIRMWARE_COMBINATION;
        goto end;
    }

    if (fw_update_server_ctx.phase == FW_UPDATE_PHASE_IDLE)
    {
        //FW_UPDATE_STAT_NOT_AVAILABLE;
        stat = FW_UPDATE_STAT_SUCCESS;
        goto end;
    }

    //if (pmsg->firmware_id != fw_update_server_ctx.firmware_id_new)
    if (0 != memcmp(pmsg->firmware_id, fw_update_server_ctx.firmware_id_new, FW_UPDATE_FW_ID_LEN))
    {
        stat = FW_UPDATE_STAT_NOT_AVAILABLE;
        goto end;
    }

    stat = FW_UPDATE_STAT_SUCCESS;
    fw_update_server_ctx.phase = FW_UPDATE_PHASE_IDLE;
    if (fw_update_server_ctx.vendor_validate_data)
    {
        plt_free(fw_update_server_ctx.vendor_validate_data, RAM_TYPE_DATA_ON);
        fw_update_server_ctx.vendor_validate_data = NULL;
    }
    /* abort now */
    if (fw_update_server.model_data_cb)
    {
        fw_update_server.model_data_cb(&fw_update_server, FW_UPD_SERVER_CB_TYPE_ABORT,
                                       &fw_update_server_ctx);
    }
end:
    fw_update_server_ctx.stat = stat;
    fw_update_stat(pmesh_msg, stat, fw_update_server_ctx.phase, fw_update_server_ctx.addi_info_8,
                   fw_update_server_ctx.company_id, fw_update_server_ctx.firmware_id, fw_update_server_ctx.object_id);
}

void fw_update_server_handle_fw_update_apply(mesh_msg_p pmesh_msg)
{
    fw_update_stat_stat_t stat;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    fw_update_apply_t *pmsg = (fw_update_apply_t *)pbuffer;
    if (pmsg->company_id != fw_update_server_ctx.company_id)
    {
        stat = FW_UPDATE_STAT_WRONG_COMPANY_FIRMWARE_COMBINATION;
        goto end;
    }

    if (0 != memcmp(pmsg->firmware_id, fw_update_server_ctx.firmware_id_new, FW_UPDATE_FW_ID_LEN))
    {
        stat = FW_UPDATE_STAT_NOT_AVAILABLE;
        goto end;
    }

    if (fw_update_server_ctx.phase != FW_UPDATE_PHASE_DFU_READY)
    {
        stat = fw_update_server_ctx.phase == FW_UPDATE_PHASE_IDLE ? FW_UPDATE_STAT_SUCCESS :
               FW_UPDATE_STAT_NOT_AVAILABLE;
        goto end;
    }

    /* validate the image */
    /* the app shall reboot later after the response has been send. */
    stat = fw_update_server_apply();

end:
    fw_update_server_ctx.stat = stat;
    fw_update_stat(pmesh_msg, stat, fw_update_server_ctx.phase, fw_update_server_ctx.addi_info_8,
                   fw_update_server_ctx.company_id, fw_update_server_ctx.firmware_id, fw_update_server_ctx.object_id);
}

/* Sample */
bool fw_update_server_receive(mesh_msg_p pmesh_msg)
{
    bool ret = TRUE;
    //uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    switch (pmesh_msg->access_opcode)
    {
    case MESH_MSG_FW_INFO_GET:
        if (pmesh_msg->msg_len == sizeof(fw_info_get_t))
        {
            fw_info_stat(pmesh_msg, fw_update_server_ctx.company_id, fw_update_server_ctx.firmware_id,
                         fw_update_server_ctx.update_url, fw_update_server_ctx.url_len);
        }
        break;
    case MESH_MSG_FW_UPDATE_GET:
        if (pmesh_msg->msg_len == sizeof(fw_update_get_t))
        {
            fw_update_stat(pmesh_msg, fw_update_server_ctx.stat, fw_update_server_ctx.phase,
                           fw_update_server_ctx.addi_info_8, fw_update_server_ctx.company_id, fw_update_server_ctx.firmware_id,
                           fw_update_server_ctx.object_id);
        }
        break;
    case MESH_MSG_FW_UPDATE_PREPARE:
        if (pmesh_msg->msg_len >= MEMBER_OFFSET(fw_update_prepare_t, vendor_validate_data))
        {
            fw_update_server_handle_fw_update_prepare(pmesh_msg);
        }
        break;
    case MESH_MSG_FW_UPDATE_START:
        if (pmesh_msg->msg_len == sizeof(fw_update_start_t))
        {
            fw_update_server_handle_fw_update_start(pmesh_msg);
        }
        break;
    case MESH_MSG_FW_UPDATE_ABORT:
        if (pmesh_msg->msg_len == sizeof(fw_update_abort_t))
        {
            fw_update_server_handle_fw_update_abort(pmesh_msg);
        }
        break;
    case MESH_MSG_FW_UPDATE_APPLY:
        if (pmesh_msg->msg_len == sizeof(fw_update_apply_t))
        {
            fw_update_server_handle_fw_update_apply(pmesh_msg);
        }
        break;
    default:
        ret = FALSE;
        break;
    }
    if (ret)
    {
        printi("fw_update_server_receive: opcode = 0x%x, phase = %d", pmesh_msg->access_opcode,
               fw_update_server_ctx.phase);
    }
    return ret;
}

void fw_update_server_reg(uint16_t company_id, fw_update_fw_id_t firmware_id,
                          model_data_cb_pf model_data_cb)
{
    fw_update_server.model_id = MESH_MODEL_FW_UPDATE_SERVER;
    fw_update_server.model_receive = fw_update_server_receive;
    fw_update_server.model_data_cb = model_data_cb;
    mesh_model_reg(0, &fw_update_server);
    fw_update_server_ctx.company_id = company_id;
    FW_UPDATE_FW_ID(fw_update_server_ctx.firmware_id, firmware_id);
}

void fw_update_server_load(uint8_t object_id[8], fw_update_fw_id_t firmware_id_new,
                           fw_update_policy_t policy)
{
    memcpy(fw_update_server_ctx.object_id, object_id, 8);
    FW_UPDATE_FW_ID(fw_update_server_ctx.firmware_id_new, firmware_id_new);
    fw_update_server_ctx.policy = policy;
    fw_update_server_ctx.phase = FW_UPDATE_PHASE_PREPARED;
    fw_update_server_ctx.stat = FW_UPDATE_STAT_SUCCESS;
}

void fw_update_server_clear(void)
{
    fw_update_server_ctx.phase = FW_UPDATE_PHASE_IDLE;
}

bool fw_update_server_ready(void)
{
    if (fw_update_server_ctx.phase == FW_UPDATE_PHASE_IN_PROGRESS)
    {
        if (fw_update_server_ctx.policy == FW_UPDATE_POLICY_AUTO_UPDATE)
        {
            fw_update_server_ctx.stat = fw_update_server_apply();
        }
        else
        {
            fw_update_server_ctx.phase = FW_UPDATE_PHASE_DFU_READY;
        }
        return true;
    }
    else
    {
        return false;
    }
}

