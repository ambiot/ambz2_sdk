/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     light_xyl_setup_server.c
* @brief    Source file for light xyl setup server model.
* @details  Data types and external functions declaration.
* @author   hector_huang
* @date     2019-07-09
* @version  v1.0
* *************************************************************************************
*/
#include "light_xyl.h"
#if MODEL_ENABLE_DELAY_MSG_RSP
#include "delay_msg_rsp.h"
#endif

#if 0
typedef struct
{
    uint8_t tid;
    light_xyl_t target_xyl;
    generic_transition_time_t trans_time;
    uint32_t delay_time;
#if MODEL_ENABLE_DELAY_MSG_RSP
    uint32_t delay_pub_time;
#endif
} light_xyl_info_t;
#endif

extern mesh_msg_send_cause_t light_xyl_default_status(mesh_model_info_p pmodel_info, uint16_t dst,
                                                      uint16_t app_key_index, light_xyl_t xyl, uint32_t delay_time);
extern mesh_msg_send_cause_t light_xyl_range_status(mesh_model_info_p pmodel_info, uint16_t dst,
                                                    uint16_t app_key_index, generic_stat_t status, uint16_t xyl_x_range_min, uint16_t xyl_x_range_max,
                                                    uint16_t xyl_y_range_min, uint16_t xyl_y_range_max, uint32_t delay_time);

static bool light_xyl_setup_server_receive(mesh_msg_p pmesh_msg)
{
    bool ret = TRUE;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;

    switch (pmesh_msg->access_opcode)
    {
    case MESH_MSG_LIGHT_XYL_DEFAULT_SET:
    case MESH_MSG_LIGHT_XYL_DEFAULT_SET_UNACK:
        if (pmesh_msg->msg_len == sizeof(light_xyl_default_set_t))
        {
            light_xyl_default_set_t *pmsg = (light_xyl_default_set_t *)pbuffer;
            light_xyl_server_set_default_t set_default = pmsg->xyl;
            if (NULL != pmesh_msg->pmodel_info->model_data_cb)
            {
                pmesh_msg->pmodel_info->model_data_cb(pmesh_msg->pmodel_info, LIGHT_XYL_SERVER_SET_DEFAULT,
                                                      &set_default);
            }

            if (MESH_MSG_LIGHT_XYL_DEFAULT_SET == pmesh_msg->access_opcode)
            {
                uint32_t delay_rsp_time = 0;
#if MODEL_ENABLE_DELAY_MSG_RSP
                delay_rsp_time = delay_msg_get_rsp_delay(pmesh_msg->dst);
#endif
                light_xyl_default_status(pmesh_msg->pmodel_info, pmesh_msg->src, pmesh_msg->app_key_index,
                                         pmsg->xyl, delay_rsp_time);
            }
        }
        break;
    case MESH_MSG_LIGHT_XYL_RANGE_SET:
    case MESH_MSG_LIGHT_XYL_RANGE_SET_UNACK:
        if (pmesh_msg->msg_len == sizeof(light_xyl_range_set_t))
        {
            light_xyl_range_set_t *pmsg = (light_xyl_range_set_t *)pbuffer;
            if ((pmsg->xyl_x_range_min < pmsg->xyl_x_range_max) &&
                (pmsg->xyl_y_range_min < pmsg->xyl_y_range_max))
            {
                light_xyl_server_set_range_t set_range = {pmsg->xyl_x_range_min, pmsg->xyl_x_range_max, pmsg->xyl_y_range_min, pmsg->xyl_y_range_max};
                if (NULL != pmesh_msg->pmodel_info->model_data_cb)
                {
                    pmesh_msg->pmodel_info->model_data_cb(pmesh_msg->pmodel_info, LIGHT_XYL_SERVER_SET_RANGE,
                                                          &set_range);
                }

                if (MESH_MSG_LIGHT_XYL_RANGE_SET == pmesh_msg->access_opcode)
                {
                    uint32_t delay_rsp_time = 0;
#if MODEL_ENABLE_DELAY_MSG_RSP
                    delay_rsp_time = delay_msg_get_rsp_delay(pmesh_msg->dst);
#endif
                    light_xyl_range_status(pmesh_msg->pmodel_info, pmesh_msg->src,
                                           pmesh_msg->app_key_index, GENERIC_STAT_SUCCESS,
                                           pmsg->xyl_x_range_min, pmsg->xyl_x_range_max,
                                           pmsg->xyl_y_range_min, pmsg->xyl_y_range_max, delay_rsp_time);
                }
            }
        }
        break;
    default:
        ret = FALSE;
        break;
    }
    return ret;
}

#if MESH_MODEL_ENABLE_DEINIT
static void light_xyl_setup_server_deinit(mesh_model_info_t *pmodel_info)
{
    if (pmodel_info->model_receive == light_xyl_setup_server_receive)
    {
#if 0
        /* now we can remove */
        if (NULL != pmodel_info->pargs)
        {
            plt_free(pmodel_info->pargs, RAM_TYPE_DATA_ON);
            pmodel_info->pargs = NULL;
        }
#endif
        pmodel_info->model_receive = NULL;
    }
}
#endif

bool light_xyl_setup_server_reg(uint8_t element_index, mesh_model_info_p pmodel_info)
{
    if (NULL == pmodel_info)
    {
        return FALSE;
    }

    pmodel_info->model_id = MESH_MODEL_LIGHT_XYL_SETUP_SERVER;
    if (NULL == pmodel_info->model_receive)
    {
#if 0
		light_xyl_info_t *pxyl_info = plt_malloc(sizeof(light_xyl_info_t), RAM_TYPE_DATA_ON);
        if (NULL == pxyl_info)
        {
            printe("light_xyl_setup_server_reg: fail to allocate memory for the new model extension data!");
            return FALSE;
        }
        memset(pxyl_info, 0, sizeof(light_xyl_info_t));
#endif
        pmodel_info->model_receive = light_xyl_setup_server_receive;
        if (NULL == pmodel_info->model_data_cb)
        {
            printw("light_xyl_setup_server_reg: missing model data process callback!");
        }
#if MESH_MODEL_ENABLE_DEINIT
        pmodel_info->model_deinit = light_xyl_setup_server_deinit;
#endif
    }

    return mesh_model_reg(element_index, pmodel_info);
}

