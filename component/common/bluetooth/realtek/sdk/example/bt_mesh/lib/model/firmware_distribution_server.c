/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     firmware_distribution_server.c
* @brief    Source file for firmware update server model.
* @details  Data types and external functions declaration.
* @author   bill
* @date     2018-5-21
* @version  v1.0
* *************************************************************************************
*/

/* Add Includes here */
#include <string.h>
#include "mesh_api.h"
#include "firmware_distribution.h"

static mesh_msg_send_cause_t fw_dist_server_send(mesh_msg_p pmesh_msg, uint8_t *pmsg, uint16_t len)
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

mesh_msg_send_cause_t fw_dist_stat(mesh_msg_p pmesh_msg, fw_dist_stat_stat_t stat,
                                   uint16_t company_id, uint16_t firmware_id)
{
    fw_dist_stat_t msg;
    ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_FW_DIST_STAT);
    msg.stat = stat;
    msg.company_id = company_id;
    msg.firmware_id = firmware_id;
    return fw_dist_server_send(pmesh_msg, (uint8_t *)&msg, sizeof(fw_dist_stat_t));
}

mesh_msg_send_cause_t fw_dist_details_list(mesh_msg_p pmesh_msg, fw_dist_details_unit_t details[],
                                           uint16_t node_num)
{
    mesh_msg_send_cause_t ret;
    fw_dist_details_list_t *pmsg = (fw_info_stat_t *)plt_malloc(MEMBER_OFFSET(fw_dist_details_list_t,
                                                                              details) + node_num * sizeof(fw_dist_details_unit_t), RAM_TYPE_DATA_OFF);
    if (pmsg == NULL)
    {
        return MESH_MSG_SEND_CAUSE_NO_MEMORY;
    }
    ACCESS_OPCODE_BYTE(pmsg->opcode, MESH_MSG_FW_DIST_DETAILS_LIST);
    memcpy(pmsg->details, details, node_num * sizeof(fw_dist_details_unit_t));
    ret = fw_dist_server_send(pmesh_msg, (uint8_t *)pmsg, MEMBER_OFFSET(fw_dist_details_list_t,
                                                                        details) + node_num * sizeof(fw_dist_details_unit_t));
    plt_free(pmsg, RAM_TYPE_DATA_OFF);
    return ret;
}

/* Sample */
bool fw_dist_server_receive(mesh_msg_p pmesh_msg)
{
    bool ret = TRUE;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;

    switch (pmesh_msg->access_opcode)
    {
    case MESH_MSG_FW_DIST_GET:
        if (pmesh_msg->msg_len == sizeof(fw_dist_get_t))
        {

        }
        break;
    case MESH_MSG_FW_DIST_START:
        break;
    case MESH_MSG_FW_DIST_STOP:
        if (pmesh_msg->msg_len == sizeof(fw_dist_stop_t))
        {

        }
        break;
    case MESH_MSG_FW_DIST_DETAILS_GET:
        if (pmesh_msg->msg_len == sizeof(fw_dist_details_get_t))
        {

        }
        break;
    default:
        ret = FALSE;
        break;
    }
    return ret;
}

