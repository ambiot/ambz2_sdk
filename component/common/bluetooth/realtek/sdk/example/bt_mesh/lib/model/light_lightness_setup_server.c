/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     light_lightness_setup_server.c
* @brief    Source file for light lightness setup server model.
* @details  Data types and external functions declaration.
* @author   hector_huang
* @date     2018-7-30
* @version  v1.0
* *************************************************************************************
*/

/* Add Includes here */
#include "light_lightness.h"
#if MODEL_ENABLE_DELAY_MSG_RSP
#include "delay_msg_rsp.h"
#endif

extern mesh_msg_send_cause_t light_lightness_default_stat(mesh_model_info_p pmodel_info,
                                                          uint16_t dst,
                                                          uint16_t app_key_index, uint16_t lightness, uint32_t delay_time);
extern mesh_msg_send_cause_t light_lightness_range_stat(mesh_model_info_p pmodel_info, uint16_t dst,
                                                        uint16_t app_key_index, generic_stat_t stat, uint16_t range_min, uint16_t range_max,
                                                        uint32_t delay_time);

#if 0
typedef struct
{
    uint8_t tid;
    uint16_t target_lightness;
    uint16_t target_lightness_linear;
    generic_transition_time_t trans_time;
    uint32_t delay_time;
    generic_transition_time_t trans_time_linear;
    uint32_t delay_time_linear;
#if MODEL_ENABLE_DELAY_MSG_RSP
    uint32_t delay_pub_time;
    uint32_t delay_pub_time_linear;
#endif
} light_lightness_info_t;
#endif

static bool light_lightness_setup_server_receive(mesh_msg_p pmesh_msg)
{
    bool ret = TRUE;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;

    switch (pmesh_msg->access_opcode)
    {
    case MESH_MSG_LIGHT_LIGHTNESS_DEFAULT_SET:
    case MESH_MSG_LIGHT_LIGHTNESS_DEFAULT_SET_UNACK:
        if (pmesh_msg->msg_len == sizeof(light_lightness_default_set_t))
        {
            light_lightness_default_set_p pmsg = (light_lightness_default_set_p)pbuffer;
            light_lightness_server_set_default_t set_default = {pmsg->lightness};
            if (NULL != pmesh_msg->pmodel_info->model_data_cb)
            {
                pmesh_msg->pmodel_info->model_data_cb(pmesh_msg->pmodel_info, LIGHT_LIGHTNESS_SERVER_SET_DEFAULT,
                                                      &set_default);
            }

            if (MESH_MSG_LIGHT_LIGHTNESS_DEFAULT_SET == pmesh_msg->access_opcode)
            {
                uint32_t delay_rsp_time = 0;
#if MODEL_ENABLE_DELAY_MSG_RSP
                delay_rsp_time = delay_msg_get_rsp_delay(pmesh_msg->dst);
#endif
                light_lightness_default_stat(pmesh_msg->pmodel_info, pmesh_msg->src, pmesh_msg->app_key_index,
                                             pmsg->lightness, delay_rsp_time);
            }

        }
        break;
    case MESH_MSG_LIGHT_LIGHTNESS_RANGE_SET:
    case MESH_MSG_LIGHT_LIGHTNESS_RANGE_SET_UNACK:
        if (pmesh_msg->msg_len == sizeof(light_lightness_range_set_t))
        {
            light_lightness_range_set_p pmsg = (light_lightness_range_set_p)pbuffer;
            if ((pmsg->range_min < pmsg->range_max) &&
                (0 != pmsg->range_min) &&
                (0 != pmsg->range_max))
            {
                light_lightness_server_set_range_t set_range = {pmsg->range_min, pmsg->range_max};
                if (NULL != pmesh_msg->pmodel_info->model_data_cb)
                {
                    pmesh_msg->pmodel_info->model_data_cb(pmesh_msg->pmodel_info, LIGHT_LIGHTNESS_SERVER_SET_RANGE,
                                                          &set_range);
                }

                if (MESH_MSG_LIGHT_LIGHTNESS_RANGE_SET == pmesh_msg->access_opcode)
                {
                    uint32_t delay_rsp_time = 0;
#if MODEL_ENABLE_DELAY_MSG_RSP
                    delay_rsp_time = delay_msg_get_rsp_delay(pmesh_msg->dst);
#endif
                    light_lightness_range_stat(pmesh_msg->pmodel_info, pmesh_msg->src,
                                               pmesh_msg->app_key_index, GENERIC_STAT_SUCCESS,
                                               pmsg->range_min, pmsg->range_max, delay_rsp_time);
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
static void light_lightness_setup_server_deinit(mesh_model_info_t *pmodel_info)
{
    if (pmodel_info->model_receive == light_lightness_setup_server_receive)
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

bool light_lightness_setup_server_reg(uint8_t element_index, mesh_model_info_p pmodel_info)
{
    if (NULL == pmodel_info)
    {
        return FALSE;
    }

    pmodel_info->model_id = MESH_MODEL_LIGHT_LIGHTNESS_SETUP_SERVER;
    if (NULL == pmodel_info->model_receive)
    {
#if 0
        pmodel_info->pargs = plt_malloc(sizeof(light_lightness_info_t), RAM_TYPE_DATA_ON);
        if (NULL == pmodel_info->pargs)
        {
            printe("light_lightness_setup_server_reg: fail to allocate memory for the new model extension data!");
            return FALSE;
        }
        memset(pmodel_info->pargs, 0, sizeof(light_lightness_info_t));
#endif
        pmodel_info->model_receive = light_lightness_setup_server_receive;
        if (NULL == pmodel_info->model_data_cb)
        {
            printw("light_lightness_setup_server_reg: missing model data process callback!");
        }
#if MESH_MODEL_ENABLE_DEINIT
        pmodel_info->model_deinit = light_lightness_setup_server_deinit;
#endif
    }
    return mesh_model_reg(element_index, pmodel_info);
}

