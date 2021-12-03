/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     generic_user_property_server.c
* @brief    Source file for generic user property server model.
* @details  Data types and external functions declaration.
* @author   hector_huang
* @date     2019-07-01
* @version  v1.0
* *************************************************************************************
*/
#include "generic_property.h"
#if MODEL_ENABLE_DELAY_MSG_RSP
#include "delay_msg_rsp.h"
#endif

typedef struct
{
    generic_property_db_t *pproperties;
    uint16_t num_properties;
} generic_user_property_info_t;

void generic_user_property_server_set_db(mesh_model_info_t *pmodel_info,
                                         generic_property_db_t *pproperties, uint16_t num_properties)
{
    generic_user_property_info_t *pinfo = pmodel_info->pargs;
    pinfo->pproperties = pproperties;
    pinfo->num_properties = num_properties;
}

static mesh_msg_send_cause_t generic_user_property_send(mesh_model_info_p pmodel_info, uint16_t dst,
                                                        uint16_t app_key_index,
                                                        uint8_t *pmsg, uint16_t msg_len, uint16_t delay_time)
{
    mesh_msg_t mesh_msg;
    mesh_msg.pmodel_info = pmodel_info;
    access_cfg(&mesh_msg);
    mesh_msg.pbuffer = pmsg;
    mesh_msg.msg_len = msg_len;
    if (0 != dst)
    {
        mesh_msg.dst = dst;
        mesh_msg.app_key_index = app_key_index;
    }
    mesh_msg.delay_time = delay_time;
    return access_send(&mesh_msg);
}

static mesh_msg_send_cause_t generic_user_properties_status(mesh_model_info_p pmodel_info,
                                                            uint16_t dst, uint16_t app_key_index,
                                                            const generic_property_db_t *pproperties,
                                                            uint16_t num_properties, uint16_t delay_time)
{
    uint16_t msg_len = sizeof(generic_user_properties_status_t) + 2 * num_properties;
    generic_user_properties_status_t *pmsg = plt_malloc(msg_len, RAM_TYPE_DATA_ON);
    if (NULL == pmsg)
    {
        return MESH_MSG_SEND_CAUSE_NO_MEMORY;
    }
    ACCESS_OPCODE_BYTE(pmsg->opcode, MESH_MSG_GENERIC_USER_PROPERTIES_STATUS);
    for (uint16_t i = 0; i < num_properties; ++i)
    {
        if (GENERIC_PROPERTY_ACCESS_NOT_TO_USER != pproperties[i].property_access)
        {
            pmsg->property_ids[i] = pproperties[i].property_id;
        }
        else
        {
            msg_len -= 2;
        }
    }
    mesh_msg_send_cause_t ret = generic_user_property_send(pmodel_info, dst, app_key_index,
                                                           (uint8_t *)pmsg,
                                                           msg_len, delay_time);
    plt_free(pmsg, RAM_TYPE_DATA_ON);

    return ret;
}

generic_property_db_t *generic_user_property_find(mesh_model_info_p pmodel_info,
                                                  uint16_t property_id)
{
    generic_user_property_info_t *pinfo = pmodel_info->pargs;
    generic_property_db_t *pdb = NULL;
    for (uint16_t i = 0; i < pinfo->num_properties; ++i)
    {
        if (pinfo->pproperties[i].property_id == property_id)
        {
            pdb = &pinfo->pproperties[i];
        }
    }

    return pdb;
}

static mesh_msg_send_cause_t generic_user_property_status_internal(mesh_model_info_p pmodel_info,
                                                                   uint16_t dst, uint16_t app_key_index,
                                                                   uint16_t property_id,
                                                                   const generic_property_db_t *pdb,
                                                                   bool send_value, uint16_t delay_time)
{
    mesh_msg_send_cause_t ret;
    if (NULL != pdb)
    {
        uint16_t msg_len = 0;
        if (send_value)
        {
            msg_len = sizeof(generic_user_property_status_t) + pdb->value_len;
        }
        else
        {
            msg_len = sizeof(generic_user_property_status_t);
        }

        generic_user_property_status_t *pmsg = plt_malloc(msg_len, RAM_TYPE_DATA_ON);
        if (NULL == pmsg)
        {
            return MESH_MSG_SEND_CAUSE_NO_MEMORY;
        }
        ACCESS_OPCODE_BYTE(pmsg->opcode, MESH_MSG_GENERIC_USER_PROPERTY_STATUS);
        pmsg->property_id = property_id;
        pmsg->property_access = pdb->property_access;
        if (send_value)
        {
            for (uint16_t i = 0; i < pdb->value_len; ++i)
            {
                pmsg->property_value[i] = pdb->pproperty_value[i];
            }
        }
        ret = generic_user_property_send(pmodel_info, dst, app_key_index, (uint8_t *)pmsg,
                                         msg_len, delay_time);
        plt_free(pmsg, RAM_TYPE_DATA_ON);
    }
    else
    {
        generic_user_property_status_t msg;
        ACCESS_OPCODE_BYTE(msg.opcode, MESH_MSG_GENERIC_USER_PROPERTY_STATUS);
        msg.property_id = property_id;
        ret = generic_user_property_send(pmodel_info, dst, app_key_index, (uint8_t *)&msg,
                                         MEMBER_OFFSET(generic_user_property_status_t, property_access), delay_time);
    }

    return ret;
}

static mesh_msg_send_cause_t generic_user_property_status(mesh_model_info_p pmodel_info,
                                                          uint16_t dst, uint16_t app_key_index,
                                                          uint16_t property_id, uint16_t delay_time)
{
    generic_property_db_t *pdb = generic_user_property_find(pmodel_info, property_id);
    bool send_value = TRUE;
    if (GENERIC_PROPERTY_ACCESS_WRITE == pdb->property_access)
    {
        send_value = FALSE;
    }
    return generic_user_property_status_internal(pmodel_info, dst, app_key_index, property_id, pdb,
                                                 send_value, delay_time);
}

static mesh_msg_send_cause_t generic_user_property_delay_publish(const mesh_model_info_p
                                                                 pmodel_info,
                                                                 const generic_property_db_t *pdb,
                                                                 uint32_t delay_time)
{
    mesh_msg_send_cause_t ret = MESH_MSG_SEND_CAUSE_INVALID_DST;
    if (mesh_model_pub_check(pmodel_info))
    {
        ret = generic_user_property_status_internal(pmodel_info, 0, 0, pdb->property_id, pdb, TRUE,
                                                    delay_time);
    }

    return ret;
}

mesh_msg_send_cause_t generic_user_property_publish(const mesh_model_info_p pmodel_info,
                                                    const generic_property_db_t *pdb)
{
    return generic_user_property_delay_publish(pmodel_info, pdb, 0);
}

static bool generic_user_property_server_receive(mesh_msg_p pmesh_msg)
{
    bool ret = TRUE;
    mesh_model_info_p pmodel_info = pmesh_msg->pmodel_info;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;

    switch (pmesh_msg->access_opcode)
    {
    case MESH_MSG_GENERIC_USER_PROPERTIES_GET:
        if (pmesh_msg->msg_len == sizeof(generic_user_properties_get_t))
        {
            generic_user_property_info_t *pinfo = pmodel_info->pargs;
            uint32_t delay_rsp_time = 0;
#if MODEL_ENABLE_DELAY_MSG_RSP
            delay_rsp_time = delay_msg_get_rsp_delay(pmesh_msg->dst);
#endif
            generic_user_properties_status(pmodel_info, pmesh_msg->src, pmesh_msg->app_key_index,
                                           pinfo->pproperties, pinfo->num_properties, delay_rsp_time);
        }
        break;
    case MESH_MSG_GENERIC_USER_PROPERTY_GET:
        if (pmesh_msg->msg_len == sizeof(generic_user_property_get_t))
        {
            generic_user_property_get_t *pmsg = (generic_user_property_get_t *)pbuffer;
            if (pmsg->property_id != GENERIC_PROPERTY_ID_PROHIBITED)
            {
                uint32_t delay_rsp_time = 0;
#if MODEL_ENABLE_DELAY_MSG_RSP
                delay_rsp_time = delay_msg_get_rsp_delay(pmesh_msg->dst);
#endif
                generic_user_property_status(pmodel_info, pmesh_msg->src, pmesh_msg->app_key_index,
                                             pmsg->property_id, delay_rsp_time);
            }
        }
        break;
    case MESH_MSG_GENERIC_USER_PROPERTY_SET:
    case MESH_MSG_GENERIC_USER_PROPERTY_SET_UNACK:
        {
            generic_user_property_set_t *pmsg = (generic_user_property_set_t *)pbuffer;
            generic_property_db_t *pdb = generic_user_property_find(pmodel_info, pmsg->property_id);
            bool send_value = FALSE;
            bool valid = TRUE;
            if (NULL != pdb)
            {
                uint16_t value_len = pmesh_msg->msg_len - MEMBER_OFFSET(generic_user_property_set_t,
                                                                        property_value);
                if (pdb->value_len != value_len)
                {
                    /* invalid parameters */
                    valid = FALSE;
                }
                else
                {
                    if (GENERIC_PROPERTY_ACCESS_READ != pdb->property_access)
                    {
                        memcpy(pdb->pproperty_value, pmsg->property_value, value_len);
                        pdb->value_len = value_len;
                        /* notify app user property changed */
                        generic_user_property_server_set_t set_data;
                        set_data.property_id = pmsg->property_id;
                        set_data.pproperty_value = pdb->pproperty_value;
                        set_data.value_len = pdb->value_len;
                        if (NULL != pmodel_info->model_data_cb)
                        {
                            pmodel_info->model_data_cb(pmodel_info, GENERIC_USER_PROPERTY_SERVER_SET, &set_data);
                        }
                        send_value = TRUE;
                    }
                }
            }

            if (valid)
            {
                uint32_t delay_rsp_time = 0;
                if (pmesh_msg->access_opcode == MESH_MSG_GENERIC_USER_PROPERTY_SET)
                {
#if MODEL_ENABLE_DELAY_MSG_RSP
                    delay_rsp_time = delay_msg_get_rsp_delay(pmesh_msg->dst);
#endif
                    generic_user_property_status_internal(pmodel_info, pmesh_msg->src, pmesh_msg->app_key_index,
                                                          pmsg->property_id, pdb, send_value, delay_rsp_time);
                }

                uint32_t delay_pub_time = 0;

#if MODEL_ENABLE_DELAY_MSG_RSP
                bool ack = (pmesh_msg->access_opcode == MESH_MSG_GENERIC_USER_PROPERTY_SET_UNACK) ? FALSE : TRUE;
                generic_transition_time_t trans_time = {0, 0};
                delay_pub_time = delay_msg_get_trans_delay(0, trans_time, delay_rsp_time, TRUE, ack);
#endif
                if ((NULL != pdb) && (GENERIC_PROPERTY_ACCESS_READ != pdb->property_access))
                {
                    generic_user_property_delay_publish(pmodel_info, pdb, delay_pub_time);
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
static void generic_user_property_deinit(mesh_model_info_t *pmodel_info)
{
    if (pmodel_info->model_receive == generic_user_property_server_receive)
    {
        if (NULL != pmodel_info->pargs)
        {
            plt_free(pmodel_info->pargs, RAM_TYPE_DATA_ON);
            pmodel_info->pargs = NULL;
        }
        pmodel_info->model_receive = NULL;
    }
}
#endif

bool generic_user_property_server_reg(uint8_t element_index, mesh_model_info_p pmodel_info)
{
    if (NULL == pmodel_info)
    {
        return FALSE;
    }

    pmodel_info->model_id = MESH_MODEL_GENERIC_USER_PROPERTY_SERVER;
    if (NULL == pmodel_info->model_receive)
    {
        pmodel_info->pargs = plt_malloc(sizeof(generic_user_property_info_t), RAM_TYPE_DATA_ON);
        if (NULL == pmodel_info->pargs)
        {
            printe("generic_user_property_server_reg: fail to allocate memory for the new model extension data!");
            return FALSE;
        }
        memset(pmodel_info->pargs, 0, sizeof(generic_user_property_info_t));

        pmodel_info->model_receive = generic_user_property_server_receive;
        if (NULL == pmodel_info->model_data_cb)
        {
            printw("generic_user_property_server_reg: missing model data process callback!");
        }
#if MESH_MODEL_ENABLE_DEINIT
        pmodel_info->model_deinit = generic_user_property_deinit;
#endif
    }

    return mesh_model_reg(element_index, pmodel_info);
}

