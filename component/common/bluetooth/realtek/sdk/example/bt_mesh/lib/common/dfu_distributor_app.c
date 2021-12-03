/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      dfu_distributor_app.c
* @brief     Smart mesh dfu application
* @details
* @author    hector_huang
* @date      2020-11-06
* @version   v1.0
* *********************************************************************************************************
*/

#define MM_ID MM_MODEL

#include <stdlib.h>
#include "dfu_distributor_app.h"
#include "generic_types.h"
#include "app_msg.h"
#include "bt_mesh_provisioner_api.h"

#if MESH_DFU

#define DFU_DIST_NODE_NUM_MAX                               5
#define DFU_DIST_RETRY_TIMES                                5
#define DFU_DIST_UPDATE_RETRY_PERIOD                        2000
#define DFU_DIST_BLOB_TRANSFER_START_RETRY_PERIOD           2000
#define DFU_DIST_BLOB_BLOCK_START_RETRY_PERIOD              2000
#define DFU_DIST_BLOB_CHUNK_TRANSFER_RETRY_PERIOD           3000
#define DFU_DIST_BLOB_BLOCK_GET_RETRY_PERIOD                3000
#define DFU_DIST_BLOB_NEW_BLOCK_PERIOD                      8000

/* transfer capabilites */
#define DFU_BLOCK_SIZE_LOG                         10
#define DFU_BLOCK_SIZE                             1024
#define DFU_CHUNK_SIZE                             128
#define DFU_CLIENT_MTU                             256
#define DFU_CHUNK_NUM                              (DFU_BLOCK_SIZE / DFU_CHUNK_SIZE)

typedef enum
{
    DFU_DIST_PHASE_IDLE,
    DFU_DIST_PHASE_UPDATE_START,
    DFU_DIST_PHASE_BLOB_TRANSFER_START,
    DFU_DIST_PHASE_BLOB_BLOCK_START,
    DFU_DIST_PHASE_BLOB_CHUNK_TRANSFER,
    DFU_DIST_PHASE_VERIFY,
} dfu_dist_phase_t;

typedef enum
{
    DFU_NODE_PHASE_IDLE,
    DFU_NODE_PHASE_UPDATE_STARTING,
    DFU_NODE_PHASE_UPDATE_STARTED,
    DFU_NODE_PHASE_BLOB_TRANSFER_STARTING,
    DFU_NODE_PHASE_BLOB_TRANSFER_STARTED,
    DFU_NODE_PHASE_BLOB_BLOCK_STARTING,
    DFU_NODE_PHASE_BLOB_BLOCK_STARTED,
    DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERING,
    DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERED,
    DFU_NODE_PHASE_FAILED,
} dfu_node_phase_t;

typedef struct
{
    uint16_t addr;
    fw_update_phase_t phase;
} dfu_dist_node_info_t;

typedef struct _dfu_update_node_e_t
{
    struct _dfu_update_node_e_t *pnext;
    /* node information */
    uint16_t addr;
    uint8_t update_fw_image_idx;
    /* node update status */
    //uint8_t transfer_progress;
    dfu_node_phase_t node_phase;
} dfu_update_node_e_t;

typedef enum
{
    DFU_TIMER_IDLE,
    DFU_TIMER_FW_UPDATE_CLIENT_STATUS,
    DFU_TIMER_BLOB_TRANSFER_START,
    DFU_TIMER_BLOB_BLOCK_START,
    DFU_TIMER_BLOB_BLOCK_GET,
    DFU_TIMER_BLOB_CHUNK_TRANSFER
} dfu_timer_state_t;

struct
{
    plt_timer_t timer;
    dfu_timer_state_t timer_state;
    dfu_dist_phase_t dist_phase;
    uint8_t dist_retry_count;
    fw_image_data_get_t fw_image_data_get;
    uint32_t fw_image_size;
    uint32_t fw_image_left_size;
    plt_list_t dfu_update_node_list;
    dfu_update_node_e_t *pcur_update_node;
    uint8_t blob_id[8];
    uint16_t dist_app_key_index;
    uint8_t dist_ttl;
    uint16_t dist_timeout_base;
    blob_transfer_mode_t dist_transfer_mode;
    fw_update_policy_t update_policy;
    uint16_t dist_multicast_addr;
    uint8_t fw_metadata[255];
    uint8_t metadata_len;
    /* current transfer data */
    uint16_t block_num;
    uint16_t total_blocks;
    uint8_t *pblock_data;
    uint16_t block_size;
    uint16_t block_left_size;
    uint16_t chunk_num;
    uint16_t total_chunks;
    uint8_t *pchunk_data;
    uint16_t chunk_size;
} dfu_dist_ctx;


int32_t dfu_transfer_client_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    switch (type)
    {
    case BLOB_TRANSFER_CLIENT_TRANSFER_STATUS:
        {
            if (dfu_dist_ctx.timer_state == DFU_TIMER_BLOB_TRANSFER_START) {
                plt_timer_stop(dfu_dist_ctx.timer, 0);
            } else {
                printf("dfu_update_client_data %d, timer state %d \r\n", __LINE__, dfu_dist_ctx.timer_state);
            }
            blob_transfer_client_transfer_status_t *pdata = (blob_transfer_client_transfer_status_t *)pargs;
            dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            if (pdata->status == BLOB_TRANSFER_STATUS_SUCCESS)
            {     
                while (NULL != pentry)
                {
                    if (pentry->addr == pdata->src)
                    {
                       if (pentry->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTING) {
                            pentry->node_phase = DFU_NODE_PHASE_BLOB_TRANSFER_STARTED;
                            printf("dfu_transfer_client_data: node 0x%04x blob transfer start success \r\n", pdata->src);
                            break;
                        } else {
                            printf("dfu_transfer_client_data: node 0x%04x is not DFU_NODE_PHASE_BLOB_TRANSFER_STARTING, current state is %d \r\n", 
                                pdata->src, pentry->node_phase);
                            return MODEL_SUCCESS;
                        }
                    }
                    pentry = pentry->pnext;
                }
                if (pentry == NULL) {
                    printf("dfu_transfer_client_data: node 0x%04x is not in the update list \r\n", pdata->src);
                    return MODEL_SUCCESS;
                }
            }
            else
            {
                dfu_dist_ctx.pcur_update_node->node_phase = DFU_NODE_PHASE_FAILED;
                printf("dfu_transfer_client_data: node 0x%04x transfer start failed, reason %d \r\n", pdata->src,
                       pdata->status);
                dfu_dist_receiver_remove(pdata->src);
            }

            /* find active node and send blob transfer start */
            pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_UPDATE_STARTED)
                {
                    blob_transfer_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, BLOB_TRANSFER_MODE_PUSH,
                                        dfu_dist_ctx.blob_id, dfu_dist_ctx.fw_image_size, DFU_BLOCK_SIZE_LOG, DFU_CLIENT_MTU);
                    dfu_dist_ctx.dist_retry_count = 0;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_TRANSFER_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_TRANSFER_START;
                    pentry->node_phase = DFU_NODE_PHASE_BLOB_TRANSFER_STARTING;
                    dfu_dist_ctx.pcur_update_node = pentry;
                    return MODEL_SUCCESS;
                }
                if (pentry->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTING)
                {
                    return MODEL_SUCCESS;
                }
                pentry = pentry->pnext;
            }

            /* all node received blob transfer start message, begin blob block start */
            printf("dfu_transfer_client_data: block start, num %d \r\n", dfu_dist_ctx.block_num);
            dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_BLOCK_START;
            /* get block data */
            dfu_dist_ctx.block_size = (dfu_dist_ctx.fw_image_left_size >= DFU_BLOCK_SIZE) ? DFU_BLOCK_SIZE :
                                      dfu_dist_ctx.fw_image_left_size;
            printf("dfu_transfer_client_data: fetch firmware, dfu_dist_ctx.block_size = %d \r\n", dfu_dist_ctx.block_size);
            dfu_dist_ctx.fw_image_data_get(dfu_dist_ctx.block_size - DFU_BLOCK_SIGNATURE_SIZE,
                                           dfu_dist_ctx.pblock_data);
            dfu_block_signature(dfu_dist_ctx.pblock_data, dfu_dist_ctx.block_size - DFU_BLOCK_SIGNATURE_SIZE, 
                                    DFU_BLOCK_SIGNATURE_SIZE);
            dfu_dist_ctx.block_left_size = dfu_dist_ctx.block_size;
            printf("dfu_transfer_client_data: ========Sending one block========= \r\n");
            // for (uint32_t i = 0; i <  dfu_dist_ctx.block_size; i++) {
            //     if (i % 12 == 0) {
            //         printf("\r\n");
            //     }
            //     printf(" 0x%02x ", dfu_dist_ctx.pblock_data[i]);
            // }
            // printf("\r\n");
            /* block start */
            pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTED)
                {
                    blob_block_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.block_num,
                                     DFU_CHUNK_SIZE);
                    dfu_dist_ctx.dist_retry_count = 0;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_BLOCK_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_START;
                    pentry->node_phase = DFU_NODE_PHASE_BLOB_BLOCK_STARTING;
                    return MODEL_SUCCESS;
                }
                pentry = pentry->pnext;
            }
        }
        break;
    case BLOB_TRANSFER_CLIENT_BLOCK_STATUS:
        {
            blob_transfer_client_block_status_t *pdata = (blob_transfer_client_block_status_t *)pargs;
            dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            if (dfu_dist_ctx.dist_phase == DFU_DIST_PHASE_BLOB_BLOCK_START)
            {
                if (dfu_dist_ctx.timer_state == DFU_TIMER_BLOB_BLOCK_START) {
                    plt_timer_stop(dfu_dist_ctx.timer, 0);
                } else {
                    printf("dfu_update_client_data %d, timer state %d \r\n", __LINE__, dfu_dist_ctx.timer_state);
                }
                if (pdata->status == BLOB_TRANSFER_STATUS_SUCCESS)
                {
                    while (NULL != pentry)
                    {
                        if (pentry->addr == pdata->src)
                        {
                            if (pentry->node_phase == DFU_NODE_PHASE_BLOB_BLOCK_STARTING) {
                                pentry->node_phase = DFU_NODE_PHASE_BLOB_BLOCK_STARTED;
                                printf("dfu_transfer_client_data: node 0x%04x blob block start success \r\n", pdata->src);
                                break;
                            } else {
                                printf("dfu_transfer_client_data: node 0x%04x is not DFU_NODE_PHASE_BLOB_BLOCK_STARTING, current state is %d \r\n", 
                                    pdata->src, pentry->node_phase);
                                return MODEL_SUCCESS;
                            }
                        }
                        pentry = pentry->pnext;
                    } 
                    if (pentry == NULL) {
                        printf("dfu_transfer_client_data: node 0x%04x is not in the update list \r\n", pdata->src);
                        return MODEL_SUCCESS;
                    }
                }
                else
                {
                    dfu_dist_ctx.pcur_update_node->node_phase = DFU_NODE_PHASE_FAILED;
                    printf("dfu_transfer_client_data: node 0x%04x block start failed, reason %d \r\n", pdata->src,
                           pdata->status);
                    dfu_dist_receiver_remove(pdata->src);
                }

                /* find active node and send blob transfer start */
                pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                while (NULL != pentry)
                {
                    if (pentry->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTED)
                    {
                        blob_block_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.block_num,
                                         DFU_CHUNK_SIZE);
                        dfu_dist_ctx.dist_retry_count = 0;
                        plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_BLOCK_START_RETRY_PERIOD, 0);
                        dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_START;
                        pentry->node_phase = DFU_NODE_PHASE_BLOB_BLOCK_STARTING;
                        dfu_dist_ctx.pcur_update_node = pentry;
                        return MODEL_SUCCESS;
                    }
                    pentry = pentry->pnext;
                }

                /* check chun transfer */
                bool need_chunk_transfer = false;
                pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                while (NULL != pentry)
                {
                    if (pentry->node_phase == DFU_NODE_PHASE_BLOB_BLOCK_STARTED)
                    {
                        pentry->node_phase = DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERING;
                        need_chunk_transfer = true;
                    }
                    pentry = pentry->pnext;
                }
                if (!need_chunk_transfer)
                {
                    /* no node need to update */
                    printf("dfu_update_client_data: no node need chunk transfer, dfu procedure finish \r\n");
                    dfu_dist_clear();
                    return MODEL_SUCCESS;
                }

                /* all node received blob block start message, start chunk transfer */
                printf("dfu_transfer_client_data: chunk transfer \r\n");
                dfu_dist_ctx.pcur_update_node = NULL;
                dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_CHUNK_TRANSFER;
                /* set chunk data */
                dfu_dist_ctx.chunk_num = 0;
                dfu_dist_ctx.pchunk_data = dfu_dist_ctx.pblock_data;
                dfu_dist_ctx.chunk_size = (dfu_dist_ctx.block_left_size >= DFU_CHUNK_SIZE) ? DFU_CHUNK_SIZE :
                                          dfu_dist_ctx.block_left_size;
                dfu_dist_ctx.total_chunks = dfu_dist_ctx.block_size / dfu_dist_ctx.chunk_size;
                if (dfu_dist_ctx.block_size % dfu_dist_ctx.chunk_size)
                {
                    dfu_dist_ctx.total_chunks += 1;
                }
                /* chunk transfer start */
                dfu_dist_ctx.dist_retry_count = 0;
                if (dfu_dist_ctx.chunk_num == (DFU_CHUNK_NUM - 1)) {
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_NEW_BLOCK_PERIOD, 0);
                } else {
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_CHUNK_TRANSFER_RETRY_PERIOD, 0);
                }
                dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_GET;
                printf("dfu_transfer_client_data: blob_chunk_transfer, dst 0x%04x, chunk num %d, chunk size %d, retry count %d \r\n",
                       dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.chunk_num, dfu_dist_ctx.chunk_size,
                       dfu_dist_ctx.dist_retry_count);
                blob_chunk_transfer(dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.dist_app_key_index,
                                    dfu_dist_ctx.chunk_num, dfu_dist_ctx.pchunk_data, dfu_dist_ctx.chunk_size);
            }
            else
            {
                /* this case is for BLOB Block Get Message (BLOB Block Status ack to BLOB Block Get)*/
                pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                while (NULL != pentry)
                {
                    if (pentry->addr == pdata->src)
                    {
                        if (pdata->status == BLOB_TRANSFER_STATUS_SUCCESS)
                        {
                            printf("dfu_transfer_client_data: get node 0x%04x block status success \r\n", pdata->src);
                            switch (pdata->missing_format)
                            {
                            case BLOB_CHUNK_MISSING_FORMAT_ALL:
                                break;
                            case BLOB_CHUNK_MISSING_FORMAT_NONE:
                                pentry->node_phase = DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERED;
                                break;
                            case BLOB_CHUNK_MISSING_FORMAT_SOME:
                                for (uint16_t i = 0; i < pdata->missing_chunks_len; ++i)
                                {
                                    if (pdata->pmissing_chunks[i] == dfu_dist_ctx.chunk_num)
                                    {
                                        printf("missing chunk num %d \r\n", dfu_dist_ctx.chunk_num);
                                        return MODEL_SUCCESS;
                                    }
                                }
                                printf("dfu_transfer_client_data: node 0x%04x receive block %d, chunk %d \r\n", pdata->src,
                                       dfu_dist_ctx.block_num, dfu_dist_ctx.chunk_num);
                                pentry->node_phase = DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERED;
                                break;
                            default:
                                break;
                            }
                        }
                        else
                        {
                            pentry->node_phase = DFU_NODE_PHASE_FAILED;
                            printf("dfu_transfer_client_data: node 0x%04x get block status failed, reason %d \r\n", pdata->src,
                                   pdata->status);
                            dfu_dist_receiver_remove(pdata->src);
                        }
                        return MODEL_SUCCESS;
                    }
                    pentry = pentry->pnext;
                }
            }
        }
        break;
    case BLOB_TRANSFER_CLIENT_PARTIAL_BLOCK_REPORT:
        {
            /* this is for trunk transfer mode : PULL MODE (indicate server is ready to receive a chunk)*/
            //blob_transfer_client_partial_block_report_t *pdata = (blob_transfer_client_partial_block_report_t *)pargs;
        }
        break;
    case BLOB_TRANSFER_CLIENT_INFO_STATUS:
        {
            /* this is the ack message to BLOB information Get message */
            //blob_transfer_client_info_status_t *pdata = (blob_transfer_client_info_status_t *)pargs;
        }
        break;
    default:
        break;
    }
    return MODEL_SUCCESS;
}

int32_t dfu_update_client_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    switch (type)
    {
    case FW_UPDATE_CLIENT_INFO_STATUS:
        {
            /* get firmware update information status in response to Firmware Update Information Get Message */
            fw_update_client_info_status_t *pdata = (fw_update_client_info_status_t *)pargs;
            fw_dist_receiver_t recv;

            if (dfu_check_fw_info(pdata->pfw_info) == DFU_RESULT_OK) {
                recv.addr = pdata->src;
                recv.update_fw_image_idx = pdata->first_index;
                printf("%s:get firmware information for node 0x%04x and first index is 0x%02x! \r\n", __func__, recv.addr, recv.update_fw_image_idx);
                dfu_dist_receiver_add(&recv);
            }
        }
        break;
    case FW_UPDATE_CLIENT_FW_METADATA_STATUS:
        {
            fw_update_client_fw_metadata_status_t *pdata = (fw_update_client_fw_metadata_status_t *)pargs;
            if (pdata->status == FW_UPDATE_STATUS_SUCCESS) {
                printf("%s:Metadata check success for node 0x%04x !\r\n", __func__, pdata->src);
            } else {
                printf("%s:Metadata check fail for node 0x%04x !\r\n", __func__, pdata->src);
                dfu_dist_receiver_remove(pdata->src);
            }
        }
        break;
    case FW_UPDATE_CLIENT_STATUS:
        {
            fw_update_client_status_t *pdata = (fw_update_client_status_t *)pargs;
            dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            switch (dfu_dist_ctx.dist_phase)
            {
            case DFU_DIST_PHASE_UPDATE_START:
                {
                    if (dfu_dist_ctx.timer_state == DFU_TIMER_FW_UPDATE_CLIENT_STATUS) {
                        plt_timer_stop(dfu_dist_ctx.timer, 0);
                    } else {
                        printf("dfu_update_client_data %d, timer state %d \r\n", __LINE__, dfu_dist_ctx.timer_state);
                    }
                    if (pdata->status == FW_UPDATE_STATUS_SUCCESS)
                    {
                        while (NULL != pentry)
                        {
                            if (pentry->addr == pdata->src)
                            {
                                if (pentry->node_phase == DFU_NODE_PHASE_IDLE \
                                    || pentry->node_phase == DFU_NODE_PHASE_UPDATE_STARTING) {
                                    pentry->node_phase = DFU_NODE_PHASE_UPDATE_STARTED;
                                    printf("dfu_update_client_data: node 0x%04x update start success \r\n", pdata->src);
                                    break;
                                } else {
                                    printf("dfu_update_client_data: node 0x%04x is not idle, current state is  \r\n", 
                                        pdata->src, pentry->node_phase);
                                    return MODEL_SUCCESS;
                                }
                            }
                            pentry = pentry->pnext;
                        } 
                        if (pentry == NULL) {
                            printf("dfu_transfer_client_data: node 0x%04x is not in the update list \r\n", pdata->src);
                            return MODEL_SUCCESS;
                        }
                    }
                    else
                    {
                        dfu_dist_ctx.pcur_update_node->node_phase = DFU_NODE_PHASE_FAILED;
                        printf("dfu_update_client_data: node 0x%04x update start failed, reason %d \r\n", pdata->src,
                               pdata->status);
                        dfu_dist_receiver_remove(pdata->src);
                    }

                    /* find idle node and send update start */
                    pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                    while (NULL != pentry)
                    {
                        if (pentry->node_phase == DFU_NODE_PHASE_IDLE \
                            || pentry->node_phase == DFU_NODE_PHASE_UPDATE_STARTING)
                        {
                            fw_update_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.dist_ttl,
                                            dfu_dist_ctx.dist_timeout_base, dfu_dist_ctx.blob_id,
                                            pentry->update_fw_image_idx, dfu_dist_ctx.fw_metadata, dfu_dist_ctx.metadata_len);
                            dfu_dist_ctx.dist_retry_count = 0;
                            pentry->node_phase = DFU_NODE_PHASE_UPDATE_STARTING;
                            plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_UPDATE_RETRY_PERIOD, 0);
                            dfu_dist_ctx.timer_state = DFU_TIMER_FW_UPDATE_CLIENT_STATUS;
                            dfu_dist_ctx.pcur_update_node = pentry;
                            return MODEL_SUCCESS;
                        }
                        pentry = pentry->pnext;
                    }

                    /* all node received update start message, begin blob transfer start */
                    printf("dfu_update_client_data: start firmware blob transfer start \r\n");
                    dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_TRANSFER_START;
                    pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                    while (NULL != pentry)
                    {
                        if (pentry->node_phase == DFU_NODE_PHASE_UPDATE_STARTED)
                        {
                            blob_transfer_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, BLOB_TRANSFER_MODE_PUSH,
                                                dfu_dist_ctx.blob_id, dfu_dist_ctx.fw_image_size, DFU_BLOCK_SIZE_LOG, DFU_CLIENT_MTU);
                            dfu_dist_ctx.dist_retry_count = 0;
                            plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_TRANSFER_START_RETRY_PERIOD, 0);
                            dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_TRANSFER_START;
                            pentry->node_phase = DFU_NODE_PHASE_BLOB_TRANSFER_STARTING;
                            dfu_dist_ctx.pcur_update_node = pentry;
                            return MODEL_SUCCESS;
                        }
                        pentry = pentry->pnext;
                    }

                    /* no node need to update */
                    printf("dfu_update_client_data: no node need to firmware blob transfer start, dfu procedure finish \r\n");
                    dfu_dist_clear();
                }
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }

    return MODEL_SUCCESS;
}

#if defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER
extern void *bt_mesh_provisioner_evt_queue_handle;  //!< Event queue handle
extern void *bt_mesh_provisioner_io_queue_handle;   //!< IO queue handle
#elif defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE
extern void *bt_mesh_provisioner_multiple_profile_evt_queue_handle;  //!< Event queue handle
extern void *bt_mesh_provisioner_multiple_profile_io_queue_handle;   //!< IO queue handle
#endif

static void dfu_timeout(void *ptimer)
{
    uint8_t event = EVENT_IO_TO_APP;
    T_IO_MSG msg;
    msg.type = DFU_DIST_APP_TIMEOUT_MSG;
    msg.u.buf = ptimer;
#if defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER
    if (os_msg_send(bt_mesh_provisioner_io_queue_handle, &msg, 0) == false)
    {
    }
    else if (os_msg_send(bt_mesh_provisioner_evt_queue_handle, &event, 0) == false)
    {
    }
#elif defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE
    if (os_msg_send(bt_mesh_provisioner_multiple_profile_io_queue_handle, &msg, 0) == false)
    {
    }
    else if (os_msg_send(bt_mesh_provisioner_multiple_profile_evt_queue_handle, &event, 0) == false)
    {
    }
#endif
}

void dfu_dist_handle_timeout(void)
{
    switch (dfu_dist_ctx.dist_phase)
    {
    case DFU_DIST_PHASE_UPDATE_START:
        {
            if (dfu_dist_ctx.pcur_update_node->node_phase == DFU_NODE_PHASE_UPDATE_STARTING)
            {
                if (dfu_dist_ctx.dist_retry_count >= DFU_DIST_RETRY_TIMES)
                {
                    /* update start failed, skip this node or generate dfu failed? */
                    printf("dfu_timeout_handle: node 0x%04x update start failed \r\n", dfu_dist_ctx.pcur_update_node->addr);
                    dfu_dist_ctx.pcur_update_node->node_phase = DFU_NODE_PHASE_FAILED;
                    dfu_dist_receiver_remove(dfu_dist_ctx.pcur_update_node->addr);
                }
                else
                {
                    fw_update_start(dfu_dist_ctx.pcur_update_node->addr, dfu_dist_ctx.dist_app_key_index,
                                    dfu_dist_ctx.dist_ttl, dfu_dist_ctx.dist_timeout_base, dfu_dist_ctx.blob_id,
                                    dfu_dist_ctx.pcur_update_node->update_fw_image_idx, dfu_dist_ctx.fw_metadata,
                                    dfu_dist_ctx.metadata_len);
                    dfu_dist_ctx.dist_retry_count ++;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_UPDATE_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_FW_UPDATE_CLIENT_STATUS;
                    printf("dfu_dist_timeout_handle: fw_update_start, dst 0x%04x, retry count %d \r\n",
                           dfu_dist_ctx.pcur_update_node->addr, dfu_dist_ctx.dist_retry_count);
                    return ;
                }
            }

            dfu_dist_ctx.dist_retry_count = 0;
            /* find idle node and send update start */
            dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_IDLE)
                {
                    fw_update_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.dist_ttl,
                                    dfu_dist_ctx.dist_timeout_base, dfu_dist_ctx.blob_id, pentry->update_fw_image_idx,
                                    dfu_dist_ctx.fw_metadata, dfu_dist_ctx.metadata_len);
                    pentry->node_phase = DFU_NODE_PHASE_UPDATE_STARTING;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_UPDATE_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_FW_UPDATE_CLIENT_STATUS;
                    dfu_dist_ctx.pcur_update_node = pentry;
                    printf("dfu_dist_timeout_handle: fw_update_start, dst 0x%04x, retry count %d \r\n",
                           dfu_dist_ctx.pcur_update_node->addr, dfu_dist_ctx.dist_retry_count);
                    return ;
                }

                pentry = pentry->pnext;
            }

            /* all node received update start message, begin blob transfer start */
            printf("dfu_timeout_handle: start firmware blob transfer start \r\n");
            dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_TRANSFER_START;
            pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_UPDATE_STARTED)
                {
                    blob_transfer_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, BLOB_TRANSFER_MODE_PUSH,
                                        dfu_dist_ctx.blob_id, dfu_dist_ctx.fw_image_size, DFU_BLOCK_SIZE_LOG, DFU_CLIENT_MTU);
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_TRANSFER_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_TRANSFER_START;
                    pentry->node_phase = DFU_NODE_PHASE_BLOB_TRANSFER_STARTING;
                    dfu_dist_ctx.pcur_update_node = pentry;
                    printf("dfu_dist_timeout_handle: blob_transfer_start, dst 0x%04x, retry count %d \r\n", pentry->addr,
                           dfu_dist_ctx.dist_retry_count);
                    return ;
                }
                pentry = pentry->pnext;
            }

            /* no node need to update */
            printf("dfu_update_client_data: no node need to firmware blob transfer start, dfu procedure finish \r\n");
            dfu_dist_clear();
        }
    case DFU_DIST_PHASE_BLOB_TRANSFER_START:
        {
            if (dfu_dist_ctx.pcur_update_node->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTING)
            {
                if (dfu_dist_ctx.dist_retry_count >= DFU_DIST_RETRY_TIMES)
                {
                    /* transfer start failed, skip this node or generate dfu failed? */
                    printf("dfu_timeout_handle: node 0x%04x blob transfer start failed \r\n",
                           dfu_dist_ctx.pcur_update_node->addr);
                    dfu_dist_ctx.pcur_update_node->node_phase = DFU_NODE_PHASE_FAILED;
                    dfu_dist_receiver_remove(dfu_dist_ctx.pcur_update_node->addr);
                }
                else
                {
                    blob_transfer_start(dfu_dist_ctx.pcur_update_node->addr, dfu_dist_ctx.dist_app_key_index,
                                        BLOB_TRANSFER_MODE_PUSH, dfu_dist_ctx.blob_id, dfu_dist_ctx.fw_image_size, DFU_BLOCK_SIZE_LOG,
                                        DFU_CLIENT_MTU);
                    dfu_dist_ctx.dist_retry_count ++;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_TRANSFER_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_TRANSFER_START;
                    printf("dfu_dist_timeout_handle: blob_transfer_start, dst 0x%04x, retry count %d \r\n",
                           dfu_dist_ctx.pcur_update_node->addr, dfu_dist_ctx.dist_retry_count);
                    return ;
                }
            }

            dfu_dist_ctx.dist_retry_count = 0;
            /* find active node and send blob transfer start */
            dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_UPDATE_STARTED)
                {
                    blob_transfer_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, BLOB_TRANSFER_MODE_PUSH,
                                        dfu_dist_ctx.blob_id, dfu_dist_ctx.fw_image_size, DFU_BLOCK_SIZE_LOG, DFU_CLIENT_MTU);
                    pentry->node_phase = DFU_NODE_PHASE_BLOB_TRANSFER_STARTING;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_TRANSFER_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_TRANSFER_START;
                    dfu_dist_ctx.pcur_update_node = pentry;
                    printf("dfu_dist_timeout_handle: blob_transfer_start, dst 0x%04x, retry count %d \r\n", pentry->addr,
                           dfu_dist_ctx.dist_retry_count);
                    return ;
                }

                pentry = pentry->pnext;
            }

            /* all node received blob transfer start message, begin blob block start */
            printf("blob_transfer_client_data: block start \r\n");
            dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_BLOCK_START;
            /* get block data */
            dfu_dist_ctx.block_size = (dfu_dist_ctx.fw_image_left_size >= DFU_BLOCK_SIZE) ? DFU_BLOCK_SIZE :
                                      dfu_dist_ctx.fw_image_left_size;
            dfu_dist_ctx.fw_image_data_get(dfu_dist_ctx.block_size - DFU_BLOCK_SIGNATURE_SIZE, dfu_dist_ctx.pblock_data);
            dfu_block_signature(dfu_dist_ctx.pblock_data, dfu_dist_ctx.block_size - DFU_BLOCK_SIGNATURE_SIZE, 
                                    DFU_BLOCK_SIGNATURE_SIZE);
            dfu_dist_ctx.block_left_size = dfu_dist_ctx.block_size;
            /* block start */
            pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTED)
                {
                    blob_block_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.block_num,
                                     DFU_CHUNK_SIZE);
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_BLOCK_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_START;
                    pentry->node_phase = DFU_NODE_PHASE_BLOB_BLOCK_STARTING;
                    dfu_dist_ctx.pcur_update_node = pentry;
                    printf("dfu_dist_timeout_handle: blob_block_start, dst 0x%04x, block num %d, retry count %d \r\n",
                           pentry->addr, dfu_dist_ctx.block_num, dfu_dist_ctx.dist_retry_count);
                    return ;
                }
                pentry = pentry->pnext;
            }

            /* no node need to update */
            printf("dfu_update_client_data: no node need to blob block start, dfu procedure finish \r\n");
            dfu_dist_clear();
        }
        break;
    case DFU_DIST_PHASE_BLOB_BLOCK_START:
        {
            if (dfu_dist_ctx.pcur_update_node->node_phase == DFU_NODE_PHASE_BLOB_BLOCK_STARTING)
            {
                if (dfu_dist_ctx.dist_retry_count >= DFU_DIST_RETRY_TIMES)
                {
                    /* transfer start failed, skip this node or generate dfu failed? */
                    printf("dfu_timeout_handle: node 0x%04x blob block start failed \r\n",
                           dfu_dist_ctx.pcur_update_node->addr);
                    dfu_dist_ctx.pcur_update_node->node_phase = DFU_NODE_PHASE_FAILED;
                    dfu_dist_receiver_remove(dfu_dist_ctx.pcur_update_node->addr);
                }
                else
                {
                    blob_block_start(dfu_dist_ctx.pcur_update_node->addr, dfu_dist_ctx.dist_app_key_index,
                                     dfu_dist_ctx.block_num, DFU_CHUNK_SIZE);
                    dfu_dist_ctx.dist_retry_count ++;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_BLOCK_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_START;
                    printf("dfu_dist_timeout_handle: blob_block_start, dst 0x%04x, block num %d, retry count %d \r\n",
                           dfu_dist_ctx.pcur_update_node->addr, dfu_dist_ctx.block_num, dfu_dist_ctx.dist_retry_count);
                    return ;
                }
            }

            dfu_dist_ctx.dist_retry_count = 0;
            /* find active node and send blob block start */
            dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTED)
                {
                    blob_block_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.block_num,
                                     DFU_CHUNK_SIZE);
                    pentry->node_phase = DFU_NODE_PHASE_BLOB_BLOCK_STARTING;
                    plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_BLOCK_START_RETRY_PERIOD, 0);
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_START;
                    dfu_dist_ctx.pcur_update_node = pentry;
                    printf("dfu_dist_timeout_handle: blob_block_start, dst 0x%04x, block num %d, retry count %d \r\n",
                           pentry->addr, dfu_dist_ctx.block_num, dfu_dist_ctx.dist_retry_count);
                    return ;
                }

                pentry = pentry->pnext;
            }


            /* check chun transfer */
            bool need_chunk_transfer = false;
            pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
            while (NULL != pentry)
            {
                if (pentry->node_phase == DFU_NODE_PHASE_BLOB_BLOCK_STARTED)
                {
                    pentry->node_phase = DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERING;
                    need_chunk_transfer = true;
                }
                pentry = pentry->pnext;
            }
            if (!need_chunk_transfer)
            {
                /* no node need to update */
                printf("dfu_update_client_data: no node need chunk transfer, dfu procedure finish \r\n");
                dfu_dist_clear();
                return ;
            }

            /* all node received blob block start message, start chunk transfer */
            printf("dfu_dist_timeout_handle: chunk transfer \r\n");
            dfu_dist_ctx.pcur_update_node = NULL;
            dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_CHUNK_TRANSFER;
            /* set chunk data */
            dfu_dist_ctx.chunk_num = 0;
            dfu_dist_ctx.pchunk_data = dfu_dist_ctx.pblock_data;
            dfu_dist_ctx.chunk_size = (dfu_dist_ctx.block_left_size >= DFU_CHUNK_SIZE) ? DFU_CHUNK_SIZE :
                                      dfu_dist_ctx.block_left_size;
            dfu_dist_ctx.total_chunks = dfu_dist_ctx.block_size / dfu_dist_ctx.chunk_size;
            if (dfu_dist_ctx.block_size % dfu_dist_ctx.chunk_size)
            {
                dfu_dist_ctx.total_chunks += 1;
            }
            /* chunk transfer start */
            if (dfu_dist_ctx.chunk_num == (DFU_CHUNK_NUM - 1)) {
                plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_NEW_BLOCK_PERIOD, 0);
            } else {
                plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_CHUNK_TRANSFER_RETRY_PERIOD, 0);
            }
            dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_GET;
            blob_chunk_transfer(dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.dist_app_key_index,
                                dfu_dist_ctx.chunk_num, dfu_dist_ctx.pchunk_data, dfu_dist_ctx.chunk_size);
            printf("dfu_dist_timeout_handle: blob_chunk_transfer, dst 0x%04x, chunk num %d, chunk size %d, retry count %d \r\n",
                   dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.chunk_num, dfu_dist_ctx.chunk_size,
                   dfu_dist_ctx.dist_retry_count);
        }
        break;
    case DFU_DIST_PHASE_BLOB_CHUNK_TRANSFER:
        {
            if (dfu_dist_ctx.timer_state == DFU_TIMER_BLOB_BLOCK_GET)
            {
                plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_BLOCK_GET_RETRY_PERIOD, 0);
                dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_CHUNK_TRANSFER;
                dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                while (NULL != pentry)
                {
                    if (pentry->node_phase == DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERING)
                    {
                        printf("dfu_dist_timeout_handle: blob_block_get, dst 0x%04x \r\n", pentry->addr);
                        blob_block_get(pentry->addr, dfu_dist_ctx.dist_app_key_index);
                    }
                    pentry = pentry->pnext;
                }
            }
            else if (dfu_dist_ctx.timer_state == DFU_TIMER_BLOB_CHUNK_TRANSFER)
            {
                dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                while (NULL != pentry)
                {
                    if (pentry->node_phase == DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERING)
                    {
                        if (dfu_dist_ctx.dist_retry_count >= DFU_DIST_RETRY_TIMES)
                        {
                            printf("dfu_timeout_handle: node 0x%04x receive chunk %d failed \r\n", pentry->addr, dfu_dist_ctx.chunk_num);
                            pentry->node_phase = DFU_NODE_PHASE_FAILED;
                            dfu_dist_receiver_remove(dfu_dist_ctx.pcur_update_node->addr);
                        }
                        else
                        {
                            dfu_dist_ctx.dist_retry_count ++;
                            if (dfu_dist_ctx.chunk_num == (DFU_CHUNK_NUM - 1)) {
                                plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_NEW_BLOCK_PERIOD, 0);
                            } else {
                                plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_CHUNK_TRANSFER_RETRY_PERIOD, 0);
                            }
                            dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_GET;
                            printf("dfu_dist_timeout_handle: blob_chunk_transfer, dst 0x%04x, chunk num %d, chunk size %d, retry count %d \r\n",
                                   dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.chunk_num, dfu_dist_ctx.chunk_size,
                                   dfu_dist_ctx.dist_retry_count);
                            blob_chunk_transfer(dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.dist_app_key_index,
                                                dfu_dist_ctx.chunk_num, dfu_dist_ctx.pchunk_data, dfu_dist_ctx.chunk_size);
                            return ;
                        }
                    }
                    pentry = pentry->pnext;
                }

                /* all node receive this chunk, send next chunk or block start */
                dfu_dist_ctx.dist_retry_count = 0;
                dfu_dist_ctx.block_left_size -= dfu_dist_ctx.chunk_size;
                if (dfu_dist_ctx.block_left_size == 0)
                {
                    printf("dfu_timeout_handle: block %d send complete \r\n", dfu_dist_ctx.block_num);
                    dfu_dist_ctx.block_num ++;
                    dfu_dist_ctx.fw_image_left_size = dfu_dist_ctx.fw_image_left_size - dfu_dist_ctx.block_size;
                    if (dfu_dist_ctx.fw_image_left_size == 0)
                    {
                        /* image send complete */
                        printf("dfu_timeout_handle: firmware send complete \r\n");
                        /* start apply */
                        printf("dfu_timeout_handle: start firmware apply \r\n");
                        fw_update_apply(dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.dist_app_key_index);
                        /* clear state */
                        dfu_dist_clear();
                    }
                    else
                    {
                        /* block send complete */
                        dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_BLOCK_START;
                        dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                        while (NULL != pentry)
                        {
                            if (pentry->node_phase != DFU_NODE_PHASE_FAILED)
                            {
                                pentry->node_phase = DFU_NODE_PHASE_BLOB_TRANSFER_STARTED;
                            }
                            pentry = pentry->pnext;
                        }

                        /* all node received blob transfer start message, begin blob block start */
                        dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_BLOB_BLOCK_START;
                        /* get block data */
                        dfu_dist_ctx.block_size = (dfu_dist_ctx.fw_image_left_size >= DFU_BLOCK_SIZE) ? DFU_BLOCK_SIZE :
                                                  dfu_dist_ctx.fw_image_left_size;
                        dfu_dist_ctx.fw_image_data_get(dfu_dist_ctx.block_size - DFU_BLOCK_SIGNATURE_SIZE, dfu_dist_ctx.pblock_data);
                        dfu_block_signature(dfu_dist_ctx.pblock_data, dfu_dist_ctx.block_size - DFU_BLOCK_SIGNATURE_SIZE, 
                                    DFU_BLOCK_SIGNATURE_SIZE);
                        dfu_dist_ctx.block_left_size = dfu_dist_ctx.block_size;
                        // printf("dfu_transfer_client_data: ========Sending one block========= \r\n");
                        // for (uint32_t i = 0; i <  dfu_dist_ctx.block_size; i++) {
                        //     if (i % 12 == 0) {
                        //         printf("\r\n");
                        //     }
                        //     printf(" 0x%02x ", dfu_dist_ctx.pblock_data[i]);
                        // }
                        // printf("\r\n");
                        /* block start */
                        printf("dfu_timeout_handle: block start %d/%d, size %d \r\n", dfu_dist_ctx.block_num,
                               dfu_dist_ctx.total_blocks, dfu_dist_ctx.block_size);
                        pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                        while (NULL != pentry)
                        {
                            if (pentry->node_phase == DFU_NODE_PHASE_BLOB_TRANSFER_STARTED)
                            {
                                blob_block_start(pentry->addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.block_num,
                                                 DFU_CHUNK_SIZE);
                                plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_BLOCK_START_RETRY_PERIOD, 0);
                                dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_START;
                                pentry->node_phase = DFU_NODE_PHASE_BLOB_BLOCK_STARTING;
                                dfu_dist_ctx.pcur_update_node = pentry;
                                return ;
                            }
                            pentry = pentry->pnext;
                        }

                        /* no node need to update */
                        printf("dfu_update_client_data: no node need to blob block start, dfu procedure finish \r\n");
                        dfu_dist_clear();
                    }
                }
                else
                {
                    printf("dfu_timeout_handle: chunk %d send complete \r\n", dfu_dist_ctx.chunk_num);
                    /* set chunk data */
                    dfu_dist_ctx.chunk_num ++;
                    dfu_dist_ctx.pchunk_data = dfu_dist_ctx.pchunk_data + dfu_dist_ctx.chunk_size;
                    dfu_dist_ctx.chunk_size = (dfu_dist_ctx.block_left_size >= DFU_CHUNK_SIZE) ? DFU_CHUNK_SIZE :
                                              dfu_dist_ctx.block_left_size;
                    dfu_dist_ctx.dist_retry_count = 0;
                    /* start chunk transfer */
                    dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
                    while (NULL != pentry)
                    {
                        if (pentry->node_phase == DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERED)
                        {
                            pentry->node_phase = DFU_NODE_PHASE_BLOB_CHUNK_TRANSFERING;
                        }
                        pentry = pentry->pnext;
                    }
                    if (dfu_dist_ctx.chunk_num == (DFU_CHUNK_NUM - 1)) {
                        plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_NEW_BLOCK_PERIOD, 0);
                    } else {
                        plt_timer_change_period(dfu_dist_ctx.timer, DFU_DIST_BLOB_CHUNK_TRANSFER_RETRY_PERIOD, 0);
                    }
                    dfu_dist_ctx.timer_state = DFU_TIMER_BLOB_BLOCK_GET;
                    printf("dfu_timeout_handle: chunk transfer %d:%d/%d, size %d \r\n", dfu_dist_ctx.block_num,
                           dfu_dist_ctx.chunk_num, dfu_dist_ctx.total_chunks, dfu_dist_ctx.chunk_size);
                    blob_chunk_transfer(dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.dist_app_key_index,
                                        dfu_dist_ctx.chunk_num, dfu_dist_ctx.pchunk_data, dfu_dist_ctx.chunk_size);
                }
            } else {
                printf("dfu_dist_handle_timeout %d, wrong timer state %d \r\n", __LINE__, dfu_dist_ctx.timer_state);
            }
        }
        break;
    default:
        break;
    }
}

bool dfu_dist_receiver_add(fw_dist_receiver_t *precv)
{
    dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
    while (NULL != pentry)
    {
        if (pentry->addr == precv->addr)
        {
            printw("dfu_dist_receiver_add: receiver entry(0x%04x) already exists \r\n", precv->addr,
                   precv->update_fw_image_idx);
            return true;
        }
        pentry = pentry->pnext;
    }

    pentry = plt_zalloc(sizeof(dfu_update_node_e_t), RAM_TYPE_DATA_ON);
    if (NULL == pentry)
    {
        printf("dfu_dist_receiver_add: add receiver failed, out of memory \r\n");
        return false;
    }

    pentry->addr = precv->addr;
    pentry->update_fw_image_idx = precv->update_fw_image_idx;
    plt_list_push(&dfu_dist_ctx.dfu_update_node_list, pentry);
    printf("dfu_dist_receiver_add: index %d, addr 0x%04x, update image index %d \r\n",
           dfu_dist_ctx.dfu_update_node_list.count - 1, precv->addr, precv->update_fw_image_idx);

    return true;
}

void dfu_dist_receiver_remove(uint16_t addr)
{
    dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
    dfu_update_node_e_t *pentry_prev = NULL;
    while (NULL != pentry)
    {
        if (pentry->addr == addr)
        {
            plt_list_delete(&dfu_dist_ctx.dfu_update_node_list, pentry_prev, pentry);
            plt_free(pentry, RAM_TYPE_DATA_ON);
            printf("dfu_dist_receiver_remove: delete receiver, address 0x%04x \r\n", addr);
            return ;
        }
        pentry_prev = pentry;
        pentry = pentry->pnext;
    }
}

void dfu_dist_receiver_remove_by_index(uint16_t index)
{
    if (index >= dfu_dist_ctx.dfu_update_node_list.count)
    {
        printf("dfu_dist_receiver_remove_by_index: invalid index %d-%d \r\n", index,
               dfu_dist_ctx.dfu_update_node_list.count);
        return ;
    }

    uint16_t cur_idx = 0;
    dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
    dfu_update_node_e_t *pentry_prev = NULL;
    while (NULL != pentry)
    {
        if (cur_idx == index)
        {
            printf("dfu_dist_receiver_remove_by_index: delete receiver, index %d, addr 0x%04x \r\n", index,
                   pentry->addr);
            plt_list_delete(&dfu_dist_ctx.dfu_update_node_list, pentry_prev, pentry);
            plt_free(pentry, RAM_TYPE_DATA_ON);
            return ;
        }
        pentry_prev = pentry;
        pentry = pentry->pnext;
        cur_idx ++;
    }
}

void dfu_dist_receiver_remove_all(void)
{
    dfu_update_node_e_t *pentry;
    uint32_t count = dfu_dist_ctx.dfu_update_node_list.count;
    for (uint8_t i = 0; i < count; ++i)
    {
        pentry = plt_list_pop(&dfu_dist_ctx.dfu_update_node_list);
        if (NULL != pentry)
        {
            plt_free(pentry, RAM_TYPE_DATA_ON);
        }
    }
    printf("dfu_dist_receiver_remove_all: remove all receivers \r\n");
}

extern mesh_node_t mesh_node;

void dfu_dist_clear(void)
{
    printf("dfu_dist_clear \r\n");
    if (NULL != dfu_dist_ctx.timer)
    {
        plt_timer_delete(dfu_dist_ctx.timer, 0);
    }

    if (NULL != dfu_dist_ctx.pblock_data)
    {
        plt_free(dfu_dist_ctx.pblock_data, RAM_TYPE_DATA_ON);
    }

    dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
    while (NULL != pentry)
    {
        pentry->node_phase = DFU_NODE_PHASE_IDLE;
        pentry = pentry->pnext;
    }

    memset(&dfu_dist_ctx, 0, sizeof(dfu_dist_ctx));

    mesh_node.trans_tx_queue_size = 3;
}

bool dfu_dist_start(uint16_t dst, uint16_t app_key_index, uint16_t update_timeout_base,
                    uint8_t *pfw_metadata, uint8_t metadata_len,
                    uint32_t fw_image_size, fw_image_data_get_t fw_image_data_get)
{
    // fw_image_start_addr delete
    if (DFU_DIST_PHASE_IDLE != dfu_dist_ctx.dist_phase)
    {
        printf("dfu_dist_start: busy, phase %d \r\n", dfu_dist_ctx.dist_phase);
        return false;
    }

    if (0 == dfu_dist_ctx.dfu_update_node_list.count)
    {
        printf("dfu_dist_start: there is no node need to update \r\n");
        return false;
    }

    if (NULL == fw_image_data_get)
    {
        printf("dfu_dist_start: firmware image data callback shall exist! \r\n");
        return false;
    }

    if (fw_image_size == 0) {
        printf("dfu_dist_start: firmware size is 0 ! \r\n");
        return false;
    }

    /* start timeout timer */
    if (NULL == dfu_dist_ctx.timer)
    {
        dfu_dist_ctx.timer = plt_timer_create("dfu", DFU_DIST_UPDATE_RETRY_PERIOD, false, 0, dfu_timeout);
        if (NULL == dfu_dist_ctx.timer)
        {
            printf("dfu_dist_start: create timer failed \r\n");
            return false;
        }
        dfu_dist_ctx.timer_state = DFU_TIMER_IDLE;
    }

    if (NULL == dfu_dist_ctx.pblock_data)
    {
        dfu_dist_ctx.pblock_data = plt_malloc(DFU_BLOCK_SIZE, RAM_TYPE_DATA_ON);
        if (NULL == dfu_dist_ctx.pblock_data)
        {
            printf("dfu_dist_start: create block data buffer failed \r\n");
            return false;
        }
    }

    dfu_dist_ctx.dist_phase = DFU_DIST_PHASE_UPDATE_START;
    dfu_dist_ctx.dist_multicast_addr = dst;
    dfu_dist_ctx.dist_app_key_index = app_key_index;
    dfu_dist_ctx.dist_timeout_base = update_timeout_base;
    dfu_dist_ctx.dist_ttl = mesh_node.ttl;
    /* if use block signature, total transfer blob = image file + block num * DFU_BLOCK_SIGNATURE_SIZE */
    dfu_dist_ctx.total_blocks = fw_image_size / (DFU_BLOCK_SIZE - DFU_BLOCK_SIGNATURE_SIZE);
    if (fw_image_size % (DFU_BLOCK_SIZE - DFU_BLOCK_SIGNATURE_SIZE))
    {
        dfu_dist_ctx.total_blocks += 1;
    }
    /* dfu_dist_ctx.fw_image_size should be firmware image + total block signature size */
    dfu_dist_ctx.fw_image_size = fw_image_size + dfu_dist_ctx.total_blocks * DFU_BLOCK_SIGNATURE_SIZE;
    dfu_dist_ctx.fw_image_left_size = dfu_dist_ctx.fw_image_size;
    dfu_dist_ctx.fw_image_data_get = fw_image_data_get;
    dfu_dist_ctx.dist_retry_count = 0;
    memcpy(dfu_dist_ctx.fw_metadata, pfw_metadata, metadata_len);
    dfu_dist_ctx.metadata_len = metadata_len;
    uint32_t *pid = (uint32_t *)dfu_dist_ctx.blob_id;
    *pid ++ = rand();
    *pid ++ = rand();

    dfu_update_node_e_t *pentry = (dfu_update_node_e_t *)dfu_dist_ctx.dfu_update_node_list.pfirst;
    pentry->node_phase = DFU_NODE_PHASE_UPDATE_STARTING;
    mesh_node.trans_tx_queue_size = 1;
    /*_fw_update_start */
    {
      PUSER_ITEM puserItem = NULL;
      uint8_t ret;
      puserItem = bt_mesh_alloc_hdl(USER_API_ASYNCH);
      if (!puserItem) {
          printf("[BT_MESH_DEMO] bt_mesh_alloc_hdl fail!\r\n");
                  return false;
      }
      puserItem->pparseValue->dw_parameter[0] = dst;
      puserItem->pparseValue->dw_parameter[1] = app_key_index;
      puserItem->pparseValue->dw_parameter[2] = mesh_node.ttl;
      puserItem->pparseValue->dw_parameter[3] = update_timeout_base;
      puserItem->pparseValue->pparameter[4] = dfu_dist_ctx.blob_id;
      puserItem->pparseValue->dw_parameter[5] = pentry->update_fw_image_idx;
      puserItem->pparseValue->pparameter[6] = dfu_dist_ctx.fw_metadata;
      puserItem->pparseValue->dw_parameter[7] = metadata_len;
      puserItem->pparseValue->dw_parameter[8] = 0; //indicate blob_id and metadata is not string type
      puserItem->pparseValue->para_count = 9;
      ret = bt_mesh_set_user_cmd(GEN_MESH_CODE(_fw_update_start), puserItem->pparseValue, NULL, puserItem);
      if (ret != USER_API_RESULT_OK) {
          printf("[BT_MESH_DEMO] bt_mesh_set_user_cmd fail! %d\r\n", ret);
          return false;
      }
    }
    plt_timer_start(dfu_dist_ctx.timer, 0);
    dfu_dist_ctx.timer_state = DFU_TIMER_FW_UPDATE_CLIENT_STATUS;
    dfu_dist_ctx.pcur_update_node = pentry;

    printf("dfu_dist_start: dst 0x%04x, app_key_index %d, update_timeout_base %d, fw_image_size %d, total blocks %d \r\n",
           dfu_dist_ctx.dist_multicast_addr, dfu_dist_ctx.dist_app_key_index, dfu_dist_ctx.dist_timeout_base,
           dfu_dist_ctx.fw_image_size, dfu_dist_ctx.total_blocks);
    printf("dfu_dist_start: firmware metadata = 0x%02x, 0x%02x \r\n", dfu_dist_ctx.fw_metadata[0], dfu_dist_ctx.fw_metadata[1]);
    printf("dfu_dist_start: blob id = ");
    for (uint8_t i = 0; i < 8; i++) {
        printf(" 0x%02x ", dfu_dist_ctx.blob_id[i]);
    }

    return true;
}

uint8_t dfu_dist_model_enabled = 0;

void dfu_dist_models_init(void)
{
    dfu_dist_model_enabled = 1;
    fw_update_client_reg(0, dfu_update_client_data);
    blob_transfer_client_reg(0, dfu_transfer_client_data);
}
#endif
