#include <trace_app.h>
#include <hrp_btif_le_api.h>
#include <hrp_btif_entry.h>
#include <os_mem.h>
#include "gattsvc_dis.h"
#include "gattsvc_cts.h"
#include "gattsvc_bas.h"
#include "gattsvc_gls.h"
#include "gattsvc_ndcs.h"
#include "gattsvc_rtus.h"

#if F_BT_LE_BTIF_SUPPORT

extern T_GATT_DEV_INFO  t_gatt_dev_info;
extern T_GATT_SRV_TABLE  t_srv_table;
extern const T_ATTRIB_APPL gatt_dfindme_profile[];
extern const int gatt_svc_findme_nbr_of_attrib;
extern const int gatt_dfindme_profile_size;

void *hrp_gatt_srv_get(GATT_SRV_ID service_id, uint16_t *p_nbr_of_attrib)
{
    void *p_service = NULL;

    switch (service_id)
    {
    default:
        *p_nbr_of_attrib = 0;
        break;
    case BTIF_ApplicationDefined:
        p_service      = (void *)gatt_dfindme_profile;
        *p_nbr_of_attrib = gatt_svc_findme_nbr_of_attrib;
        break;

    case BTIF_ServiceBAS:
        p_service      = (void *)gatt_svc_bas;
        *p_nbr_of_attrib = gatt_svc_bas_nbr_of_attrib;
        break;

    case BTIF_ServiceDIS:
        p_service      = (void *)gatt_svc_dis;
        *p_nbr_of_attrib = gatt_svc_dis_nbr_of_attrib;
        break;

    case BTIF_ServiceRTUS:
        p_service      = (void *)gatt_svc_rtus;
        *p_nbr_of_attrib = gatt_svc_rtus_nbr_of_attrib;
        break;

    case BTIF_ServiceGLS:
        p_service      = (void *)gatt_svc_gls;
        *p_nbr_of_attrib = gatt_svc_gls_nbr_of_attrib;
        break;

    case BTIF_ServiceNDCS:
        p_service      = (void *)gatt_svc_ndcs;
        *p_nbr_of_attrib = gatt_svc_ndcs_nbr_of_attrib;
        break;

    case BTIF_ServiceCTS:
        p_service      = (void *)gatt_svc_cts;
        *p_nbr_of_attrib = gatt_svc_cts_nbr_of_attrib;
        break;

    }

    return p_service;
}

#if F_BT_LE_SMP_OOB_SUPPORT
void hrp_remote_oob_req_cfm(uint16_t len, uint8_t *p_param_list)
{
    /*T_BTIF_REMOTE_OOB_REQ_CFM *p = (T_BTIF_REMOTE_OOB_REQ_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_REMOTE_OOB_REQ_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_REMOTE_OOB_REQ_CFM) = %d",
                         len, sizeof(T_BTIF_REMOTE_OOB_REQ_CFM));
        return;
    }*/
    uint8_t            bd_addr[6];
    uint8_t            data_c[16];
    uint8_t            cause;
    uint8_t addr_type;

    uint8_t pos = 0;
    memcpy(bd_addr, p_param_list + pos, 6); pos += 6;
    memcpy(data_c, p_param_list + pos, 16); pos += 16;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(addr_type, p_param_list + pos); pos++;

    btif_remote_oob_req_cfm(bd_addr, (T_BTIF_REMOTE_ADDR_TYPE)addr_type, data_c, (T_BTIF_CAUSE)cause);
}
#endif
void hrp_gatt_srv_reg_req(uint16_t len, uint8_t *p_param_list)
{
    /*T_BTIF_GATT_SRV_REG_REQ *p = (T_BTIF_GATT_SRV_REG_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_SRV_REG_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_SRV_REG_REQ) = %d",
                         len, sizeof(T_BTIF_GATT_SRV_REG_REQ));
        return;
    }*/
    uint8_t *p_buffer = p_param_list;
    uint16_t    num_attr;
    LE_ARRAY_TO_UINT16(num_attr, p_buffer); p_buffer += 2;
    void *p_srv = NULL;
    uint8_t serviceID = 0;
    p_srv = hrp_gatt_srv_get((GATT_SRV_ID)serviceID, &num_attr);

    if (p_srv != NULL)
    {
        btif_gatt_srv2_reg_req(num_attr, p_srv, 0);
    }

}
void hrp_gatt_attr_update_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_GATT_ATTR_UPDATE_REQ *p = (T_BTIF_GATT_ATTR_UPDATE_REQ *)p_param_list;

    uint8_t *p_buffer = p_param_list;
    uint8_t *p_data = NULL;
    uint16_t offset;
    uint16_t pool_id;

    uint8_t                   *p_srv_handle;
    uint8_t                   *p_req_handle;
    uint8_t                     type;

    uint16_t                link_id;
    uint16_t                attr_index;
    uint16_t                attr_len;
    uint16_t                gap;

    //memcpy(p_srv_handle, p_buffer, 4);
    p_srv_handle = (uint8_t *)&t_srv_table.p_srv_handle;
    p_buffer += 4;
    p_req_handle = NULL;     p_buffer += 4;

    LE_ARRAY_TO_UINT8(type, p_buffer); p_buffer++;

    LE_ARRAY_TO_UINT16(link_id, p_buffer); p_buffer += 2;
    LE_ARRAY_TO_UINT16(attr_index, p_buffer); p_buffer += 2;
    LE_ARRAY_TO_UINT16(attr_len, p_buffer); p_buffer += 2;
    LE_ARRAY_TO_UINT16(gap, p_buffer); p_buffer += 2;
    p_buffer += gap;
    pool_id = t_gatt_dev_info.le_ds_pool_id;
    offset = t_gatt_dev_info.le_ds_data_offset;

    offset += 3;

    p_data = btif_buffer_get(pool_id, attr_len, offset);

    APP_PRINT_TRACE5("hrp_gatt_attr_update_req :link_id= %d,  type= %d,attr_index =%d, attr_len=%d, gap=%d",
                     link_id, type, attr_index, attr_len,  gap);
    if (p_data == NULL)
    {
        APP_PRINT_ERROR2("hrp_gatt_attr_update_req:fail to get  buffer: pool_id = %d, offset=%d", pool_id,
                         offset);
        return;
    }
    memcpy(((uint8_t *)p_data) + offset, p_buffer, attr_len);


    btif_gatt_attr_update_req(p_data, link_id, p_srv_handle, p_req_handle, attr_index,
                              attr_len, offset, (T_BTIF_GATT_PDU_TYPE)type);

}
//to do: add all the apis here
void hrp_gatt_attr_update_status_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_UPDATE_STATUS_CFM *p = (T_BTIF_GATT_ATTR_UPDATE_STATUS_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_UPDATE_STATUS_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_UPDATE_STATUS_CFM )= %d",
                         len, sizeof(T_BTIF_GATT_ATTR_UPDATE_STATUS_CFM));
        return;
    }
    btif_gatt_attr_update_status_cfm(p->link_id, p->p_srv_handle, p->p_req_handle, p->attr_index);
}
void hrp_gatt_attr_read_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_READ_CFM *p = (T_BTIF_GATT_ATTR_READ_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_READ_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_READ_CFM) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_READ_CFM));
        return;
    }
    btif_gatt_attr_read_cfm(p->data, p->link_id, p->p_srv_handle, p->cause,
                            p->attr_index, p->attr_len, p->gap);
}
void hrp_gatt_attr_write_req_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_WRITE_REQ_CFM *p = (T_BTIF_GATT_ATTR_WRITE_REQ_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_WRITE_REQ_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_WRITE_REQ_CFM) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_WRITE_REQ_CFM));
        return;
    }
    btif_gatt_attr_write_req_cfm(p->link_id, p->p_srv_handle, p->cause,
                                 p->attr_index);
}
void hrp_gatt_attr_prep_write_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_PREP_WRITE_CFM *p = (T_BTIF_GATT_ATTR_PREP_WRITE_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_PREP_WRITE_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_PREP_WRITE_CFM) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_PREP_WRITE_CFM));
        return;
    }
    btif_gatt_attr_prep_write_cfm(p->data, p->link_id, p->p_srv_handle, p->cause,
                                  p->attr_index, p->attr_len, p->gap);
}
void hrp_gatt_attr_exec_write_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_EXEC_WRITE_CFM *p = (T_BTIF_GATT_ATTR_EXEC_WRITE_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_EXEC_WRITE_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_EXEC_WRITE_CFM )= %d",
                         len, sizeof(T_BTIF_GATT_ATTR_EXEC_WRITE_CFM));
        return;
    }
    btif_gatt_attr_exec_write_cfm(p->link_id, p->cause, p->handle);
}
#if F_BT_LE_GATT_CLIENT_SUPPORT
void hrp_gatt_discovery_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_GATT_DISCOVERY_REQ *p = (T_BTIF_GATT_DISCOVERY_REQ *)p_param_list;

    uint16_t                    link_id;
    uint8_t                       type;
    uint16_t                    start_handle;
    uint16_t                    end_handle;
    uint16_t                    uuid16;
    uint8_t                     uuid128[16];

    uint8_t pos = 0;

    LE_ARRAY_TO_UINT16(link_id, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT8(type, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT16(start_handle, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(end_handle, p_param_list + pos); pos += 2;
    LE_ARRAY_TO_UINT16(uuid16, p_param_list + pos); pos += 2;
    memcpy(uuid128, p_param_list + pos, 16);
    pos += 16;
    /*if (len != sizeof(T_BTIF_GATT_DISCOVERY_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_DISCOVERY_REQ )= %d",
                         len, sizeof(T_BTIF_GATT_DISCOVERY_REQ));
        return;
    }
    */

    btif_gatt_discovery_req(link_id, (T_BTIF_GATT_DISCOVERY_TYPE) type, start_handle, end_handle,
                            uuid16, uuid128);
}
void hrp_gatt_discovery_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_DISCOVERY_CFM *p = (T_BTIF_GATT_DISCOVERY_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_DISCOVERY_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_DISCOVERY_CFM) = %d",
                         len, sizeof(T_BTIF_GATT_DISCOVERY_CFM));
        return;
    }
    btif_gatt_discovery_cfm(p->link_id, p->type, p->start_handle, p->end_handle);
}
void hrp_gatt_attr_read_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_READ_REQ *p = (T_BTIF_GATT_ATTR_READ_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_READ_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_READ_REQ) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_READ_REQ));
        return;
    }
    btif_gatt_attr_read_req(p->link_id, p->read_type, p->read_offset, p->start_handle, p->end_handle,
                            p->uuid16, p->uuid128);
}
#if F_BT_LE_ATT_READ_MULTI_SUPPORT
void hrp_gatt_attr_read_multi_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_READ_MULTI_REQ *p = (T_BTIF_GATT_ATTR_READ_MULTI_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_READ_MULTI_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_READ_MULTI_REQ) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_READ_MULTI_REQ));
        return;
    }
    btif_gatt_attr_read_multi_req(p->link_id, p->num_handle, p->handles);
}
#endif
void hrp_gatt_attr_write_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_GATT_ATTR_WRITE_REQ *p = (T_BTIF_GATT_ATTR_WRITE_REQ *)p_param_list;
    uint8_t *p_buffer = p_param_list;
    uint8_t *p_data = NULL;
    uint16_t offset;
    uint16_t pool_id;

    uint16_t       link_id;
    uint8_t        write_type;
    uint16_t       attr_handle;
    uint16_t       attr_len;        /**< attrib. value length      */
    uint16_t       write_offset;    /**< write offset in attribute */
    uint16_t       gap;             /**< offset of attrib. value in data[] */

    LE_ARRAY_TO_UINT16(link_id, p_buffer); p_buffer += 2;

    LE_ARRAY_TO_UINT8(write_type, p_buffer); p_buffer++;
    LE_ARRAY_TO_UINT16(attr_handle, p_buffer); p_buffer += 2;

    LE_ARRAY_TO_UINT16(attr_len, p_buffer); p_buffer += 2;
    LE_ARRAY_TO_UINT16(write_offset, p_buffer); p_buffer += 2;
    LE_ARRAY_TO_UINT16(gap, p_buffer); p_buffer += 2;
    p_buffer += gap;

    //p_data = os_mem_alloc(RAM_TYPE_DATA_ON, attr_len);
    pool_id = t_gatt_dev_info.le_ds_pool_id;
    offset = t_gatt_dev_info.le_ds_data_offset;

    offset += 3;

    p_data = btif_buffer_get(pool_id, attr_len, offset);

    APP_PRINT_TRACE6("hrp_gatt_attr_write_req :link_id= %d,  write_type= %d,attr_handle =%d, attr_len=%d, write_offset=%d, gap=%d",
                     link_id, write_type, attr_handle, attr_len, write_offset, gap);
    if (p_data == NULL)
    {
        APP_PRINT_ERROR2("hrp_gatt_attr_write_req:fail to get  buffer: pool_id = %d, offset=%d", pool_id,
                         offset);
        return;
    }
    memcpy(((uint8_t *)p_data) + offset, p_buffer, attr_len);

    btif_gatt_attr_write_req(p_data, link_id, (T_BTIF_GATT_WRITE_TYPE)write_type, attr_handle,
                             attr_len, offset);

}
void hrp_gatt_attr_prep_write_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_WRITE_REQ *p = (T_BTIF_GATT_ATTR_WRITE_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_WRITE_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_WRITE_REQ) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_WRITE_REQ));
        return;
    }
    btif_gatt_attr_prep_write_req(p->data, p->link_id, p->attr_handle, p->attr_len, p->write_offset,
                                  p->gap);
}
void hrp_gatt_attr_exec_write_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_EXEC_WRITE_REQ *p = (T_BTIF_GATT_ATTR_EXEC_WRITE_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_EXEC_WRITE_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_EXEC_WRITE_REQ) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_EXEC_WRITE_REQ));
        return;
    }
    btif_gatt_attr_exec_write_req(p->link_id, p->flags);
}
void hrp_gatt_attr_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_ATTR_CFM *p = (T_BTIF_GATT_ATTR_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_ATTR_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_ATTR_CFM) = %d",
                         len, sizeof(T_BTIF_GATT_ATTR_CFM));
        return;
    }
    btif_gatt_attr_cfm(p->link_id);
}
#endif
void hrp_gatt_security_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_GATT_SECURITY_REQ *p = (T_BTIF_GATT_SECURITY_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_GATT_SECURITY_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_GATT_SECURITY_REQ) = %d",
                         len, sizeof(T_BTIF_GATT_SECURITY_REQ));
        return;
    }
    btif_gatt_security_req(p->link_id, p->requirements, p->min_key_size);
}
void hrp_gatt_server_store_cfm(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_GATT_SERVER_STORE_CFM *p = (T_BTIF_GATT_SERVER_STORE_CFM *)p_param_list;

    uint8_t    op;
    uint8_t    bd_addr[6];
    uint8_t    remote_addr_type;
    uint8_t    data_len;
    uint8_t    *p_data;
    uint8_t    cause;

    uint8_t pos = 0;

    LE_ARRAY_TO_UINT8(op, p_param_list + pos); pos++;
    memcpy(bd_addr, p_param_list + pos, 6); pos += 6;
    LE_ARRAY_TO_UINT8(remote_addr_type, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(data_len, p_param_list + pos); pos++;

    p_data = os_mem_alloc(RAM_TYPE_DATA_ON, data_len);

    memcpy(p_data, p_param_list + pos, data_len);
    pos += data_len;
    LE_ARRAY_TO_UINT8(cause, p_param_list + pos); pos++;


    btif_gatt_server_store_cfm((T_BTIF_GATT_STORE_OPCODE)op, bd_addr,
                               (T_BTIF_REMOTE_ADDR_TYPE)remote_addr_type,
                               data_len, p_data, (T_BTIF_CAUSE)cause);
    os_mem_free(p_data);
}
#if F_BT_LE_GAP_CENTRAL_SUPPORT
void hrp_le_conn_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CONN_REQ *p = (T_BTIF_LE_CONN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CONN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CONN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CONN_REQ));
        return;
    }
    btif_le_conn_req(p->use_ext_conn, p->bd_addr, p->remote_addr_type, p->local_addr_type,
                     p->init_phys, p->scan_timeout, p->conn_req_param);
}
#endif
void hrp_le_conn_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CONN_CFM *p = (T_BTIF_LE_CONN_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CONN_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CONN_CFM) = %d",
                         len, sizeof(T_BTIF_LE_CONN_CFM));
        return;
    }
    btif_le_conn_cfm(p->bd_addr, p->cause);
}
void hrp_le_disconn_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_DISCONN_REQ *p = (T_BTIF_LE_DISCONN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_DISCONN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_DISCONN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_DISCONN_REQ));
        return;
    }
    btif_le_disconn_req(p->link_id, p->cause);
}
void hrp_le_disconn_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_DISCONN_CFM *p = (T_BTIF_LE_DISCONN_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_LE_DISCONN_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_DISCONN_CFM) = %d",
                         len, sizeof(T_BTIF_LE_DISCONN_CFM));
        return;
    }
    btif_le_disconn_cfm(p->link_id);
}
void hrp_le_adv_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_ADV_REQ *p = (T_BTIF_LE_ADV_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_ADV_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_ADV_REQ) = %d",
                         len, sizeof(T_BTIF_LE_ADV_REQ));
        return;
    }
    APP_PRINT_INFO0("hrp_le_adv_req");
    btif_le_adv_req(p->adv_mode);
}
void hrp_le_adv_param_set_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_ADV_PARAM_SET_REQ *p = (T_BTIF_LE_ADV_PARAM_SET_REQ *)p_param_list;
    //if (len != sizeof(T_BTIF_LE_ADV_PARAM_SET_REQ))
    // {
    //     APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_ADV_PARAM_SET_REQ) = %d",
    //                     len, sizeof(T_BTIF_LE_ADV_PARAM_SET_REQ));
    //    return;
    //}

    btif_le_adv_param_set_req(p->adv_type, p->filter_policy, p->min_interval, p->max_interval,
                              p->local_addr_type,
                              p->bd_addr, p->remote_addr_type, p->chann_map);
}
void hrp_le_adv_data_set_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_LE_ADV_DATA_SET_REQ *p = (T_BTIF_LE_ADV_DATA_SET_REQ *)p_param_list;

    uint8_t data_type;
    uint8_t data_len;
    uint8_t *p_data = NULL;

    uint8_t pos = 0;

    LE_ARRAY_TO_UINT8(data_type, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(data_len, p_param_list + pos); pos++;
    p_data = os_mem_alloc(RAM_TYPE_DATA_ON, data_len);

    memcpy(p_data, p_param_list + pos, data_len);
    pos += data_len;
    APP_PRINT_TRACE3("hrp_le_adv_data_set_req :data_type= %d, data_len = %d,p_data= %d",
                     data_type, data_len, p_data);


    btif_le_adv_data_set_req((T_BTIF_LE_ADV_DATA_TYPE)data_type, data_len, p_data);

    os_mem_free(p_data);
}
void hrp_le_scan_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SCAN_REQ *p = (T_BTIF_LE_SCAN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SCAN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SCAN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SCAN_REQ));
        return;
    }
    btif_le_scan_req(p->mode, p->filter_duplicates);
}
void hrp_le_scan_param_set_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SCAN_PARAM_SET_REQ *p = (T_BTIF_LE_SCAN_PARAM_SET_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SCAN_PARAM_SET_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SCAN_PARAM_SET_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SCAN_PARAM_SET_REQ));
        return;
    }
    btif_le_scan_param_set_req(p->type, p->interval, p->window, p->filter_policy, p->local_addr_type);
}
void hrp_le_modify_white_list_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_MODIFY_WHITE_LIST_REQ *p = (T_BTIF_LE_MODIFY_WHITE_LIST_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_MODIFY_WHITE_LIST_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_MODIFY_WHITE_LIST_REQ) = %d",
                         len, sizeof(T_BTIF_LE_MODIFY_WHITE_LIST_REQ));
        return;
    }
    btif_le_modify_white_list_req(p->operation, p->bd_addr, p->remote_addr_type);
}
void hrp_le_conn_update_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CONN_UPDATE_REQ *p = (T_BTIF_LE_CONN_UPDATE_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CONN_UPDATE_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CONN_UPDATE_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CONN_UPDATE_REQ));
        return;
    }
    btif_le_conn_update_req(p->link_id, p->min_interval, p->max_interval, p->latency, p->supv_tout,
                            p->min_ce_len, p->max_ce_len);
}

#if F_BT_LE_GAP_CENTRAL_SUPPORT
void hrp_le_conn_update_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CONN_UPDATE_CFM *p = (T_BTIF_LE_CONN_UPDATE_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CONN_UPDATE_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CONN_UPDATE_CFM) = %d",
                         len, sizeof(T_BTIF_LE_CONN_UPDATE_CFM));
        return;
    }
    btif_le_conn_update_cfm(p->link_id, p->cause);
}
#endif
#if F_BT_LE_4_2_CONN_PARAM_UPDATE_SUPPORT
void hrp_le_conn_param_req_reply_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CONN_PARAM_REQ_REPLY_REQ *p = (T_BTIF_LE_CONN_PARAM_REQ_REPLY_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CONN_PARAM_REQ_REPLY_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CONN_PARAM_REQ_REPLY_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CONN_PARAM_REQ_REPLY_REQ));
        return;
    }
    btif_le_conn_param_req_reply_req(p->link_id, p->min_interval, p->max_interval, p->latency,
                                     p->supv_tout,
                                     p->min_ce_len, p->max_ce_len);
}
void hrp_le_conn_param_req_neg_reply_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CONN_PARAM_REQ_NEG_REPLY_REQ *p = (T_BTIF_LE_CONN_PARAM_REQ_NEG_REPLY_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CONN_PARAM_REQ_NEG_REPLY_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CONN_PARAM_REQ_NEG_REPLY_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CONN_PARAM_REQ_NEG_REPLY_REQ));
        return;
    }
    btif_le_conn_param_req_neg_reply_req(p->link_id, p->reason);
}
#endif
#if F_BT_LE_4_1_CBC_SUPPORT
void hrp_le_credit_based_conn_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_CONN_REQ *p = (T_BTIF_LE_CREDIT_BASED_CONN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_CONN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_CONN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_CONN_REQ));
        return;
    }
    btif_le_credit_based_conn_req(p->link_id, p->le_psm, p->mtu, p->init_credits, p->credits_threshold);
}
void hrp_le_credit_based_conn_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_CONN_CFM *p = (T_BTIF_LE_CREDIT_BASED_CONN_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_CONN_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_CONN_CFM) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_CONN_CFM));
        return;
    }
    btif_le_credit_based_conn_cfm(p->link_id, p->channel, p->mtu, p->init_credits, p->credits_threshold,
                                  p->cause);
}
void hrp_le_credit_based_disconn_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_DISCONN_REQ *p = (T_BTIF_LE_CREDIT_BASED_DISCONN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_DISCONN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_DISCONN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_DISCONN_REQ));
        return;
    }
    btif_le_credit_based_disconn_req(p->link_id, p->channel);
}
void hrp_le_credit_based_disconn_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_DISCONN_CFM *p = (T_BTIF_LE_CREDIT_BASED_DISCONN_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_DISCONN_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_DISCONN_CFM) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_DISCONN_CFM));
        return;
    }
    btif_le_credit_based_disconn_cfm(p->link_id, p->channel);
}
void hrp_le_send_credit_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SEND_CREDIT_REQ *p = (T_BTIF_LE_SEND_CREDIT_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SEND_CREDIT_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SEND_CREDIT_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SEND_CREDIT_REQ));
        return;
    }
    btif_le_send_credit_req(p->link_id, p->channel, p->credits);
}
void hrp_le_credit_based_data_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_DATA_REQ *p = (T_BTIF_LE_CREDIT_BASED_DATA_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_DATA_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_DATA_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_DATA_REQ));
        return;
    }
    btif_le_credit_based_data_req(p->data, p->link_id, p->channel, p->len, p->gap);
}
void hrp_le_credit_based_data_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_DATA_CFM *p = (T_BTIF_LE_CREDIT_BASED_DATA_CFM *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_DATA_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_DATA_CFM) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_DATA_CFM));
        return;
    }
    btif_le_credit_based_data_cfm(p->link_id, p->channel, p->cause);
}
void hrp_le_credit_based_security_reg_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_SECURITY_REG_REQ *p = (T_BTIF_LE_CREDIT_BASED_SECURITY_REG_REQ *)
                                                 p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_SECURITY_REG_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_SECURITY_REG_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_SECURITY_REG_REQ));
        return;
    }
    btif_le_credit_based_security_reg_req(p->le_psm, p->active, p->mode, p->key_size);
}
void hrp_le_credit_based_psm_reg_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CREDIT_BASED_PSM_REG_REQ *p = (T_BTIF_LE_CREDIT_BASED_PSM_REG_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CREDIT_BASED_PSM_REG_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CREDIT_BASED_PSM_REG_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CREDIT_BASED_PSM_REG_REQ));
        return;
    }
    btif_le_credit_based_psm_reg_req(p->le_psm, p->action);
}
#endif
#if F_BT_LE_GAP_CENTRAL_SUPPORT
void hrp_le_set_chann_classif_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_CHANN_CLASSIF_REQ *p = (T_BTIF_LE_SET_CHANN_CLASSIF_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_CHANN_CLASSIF_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_CHANN_CLASSIF_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_CHANN_CLASSIF_REQ));
        return;
    }
    btif_le_set_chann_classif_req(p->channel_map);
}
#endif
#if F_BT_LE_READ_CHANN_MAP
void hrp_le_read_chann_map_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_READ_CHANN_MAP_REQ *p = (T_BTIF_LE_READ_CHANN_MAP_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_READ_CHANN_MAP_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_READ_CHANN_MAP_REQ) = %d",
                         len, sizeof(T_BTIF_LE_READ_CHANN_MAP_REQ));
        return;
    }
    btif_le_read_chann_map_req(p->link_id);
}
#endif
#if F_BT_LE_4_0_DTM_SUPPORT
void hrp_le_receiver_test_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_RECEIVER_TEST_REQ *p = (T_BTIF_LE_RECEIVER_TEST_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_RECEIVER_TEST_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_RECEIVER_TEST_REQ) = %d",
                         len, sizeof(T_BTIF_LE_RECEIVER_TEST_REQ));
        return;
    }
    btif_le_receiver_test_req(p->rx_chann);
}
void hrp_le_transmitter_test_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_TRANSMITTER_TEST_REQ *p = (T_BTIF_LE_TRANSMITTER_TEST_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_TRANSMITTER_TEST_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_TRANSMITTER_TEST_REQ) = %d",
                         len, sizeof(T_BTIF_LE_TRANSMITTER_TEST_REQ));
        return;
    }
    btif_le_transmitter_test_req(p->tx_chann, p->data_len, p->pkt_pl);
}
void hrp_le_test_end_req(uint16_t len, uint8_t *p_param_list)
{
    btif_le_test_end_req();
}
#endif
#if F_BT_LE_READ_ADV_TX_POWRE_SUPPORT
void hrp_le_read_adv_tx_power_req(uint16_t len, uint8_t *p_param_list)
{
    btif_le_read_adv_tx_power_req();
}
#endif
void hrp_le_set_rand_addr_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_RAND_ADDR_REQ *p = (T_BTIF_LE_SET_RAND_ADDR_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_RAND_ADDR_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_RAND_ADDR_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_RAND_ADDR_REQ));
        return;
    }
    btif_le_set_rand_addr_req(p->random_addr);
}
//*********** ???????
#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
void hrp_le_read_max_data_len_req(uint16_t len, uint8_t *p_param_list)
{
    //btif_le_read_max_data_len_req();
}
void hrp_le_read_default_data_len_req(uint16_t len, uint8_t *p_param_list)
{
    //btif_le_read_default_data_len_req();
}
void hrp_le_write_default_data_len_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_LE_WRITE_DEFAULT_DATA_LEN_REQ *p = (T_BTIF_LE_WRITE_DEFAULT_DATA_LEN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_WRITE_DEFAULT_DATA_LEN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_WRITE_DEFAULT_DATA_LEN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_WRITE_DEFAULT_DATA_LEN_REQ));
        return;
    }
    //btif_le_write_default_data_len_req(p->tx_oct, p->tx_time);
}
//**********
void hrp_le_set_data_len_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_DATA_LEN_REQ *p = (T_BTIF_LE_SET_DATA_LEN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_DATA_LEN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_DATA_LEN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_DATA_LEN_REQ));
        return;
    }
    btif_le_set_data_len_req(p->link_id, p->tx_oct, p->tx_time);
}
#endif
#if F_BT_LE_PRIVACY_SUPPORT
void hrp_le_modify_resolv_list_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_MODIFY_RESOLV_LIST_REQ *p = (T_BTIF_LE_MODIFY_RESOLV_LIST_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_MODIFY_RESOLV_LIST_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_MODIFY_RESOLV_LIST_REQ) = %d",
                         len, sizeof(T_BTIF_LE_MODIFY_RESOLV_LIST_REQ));
        return;
    }
    btif_le_modify_resolv_list_req(p->operation, p->peer_ident_addr_type, p->peer_ident_addr,
                                   p->peer_irk, p->local_irk);
}
void hrp_le_read_peer_resolv_addr_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_READ_PEER_RESOLV_ADDR_REQ *p = (T_BTIF_LE_READ_PEER_RESOLV_ADDR_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_READ_PEER_RESOLV_ADDR_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_READ_PEER_RESOLV_ADDR_REQ) = %d",
                         len, sizeof(T_BTIF_LE_READ_PEER_RESOLV_ADDR_REQ));
        return;
    }
    btif_le_read_peer_resolv_addr_req(p->peer_ident_addr_type, p->peer_ident_addr);
}
void hrp_le_read_local_resolv_addr_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_READ_LOCAL_RESOLV_ADDR_REQ *p = (T_BTIF_LE_READ_LOCAL_RESOLV_ADDR_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_READ_LOCAL_RESOLV_ADDR_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_READ_LOCAL_RESOLV_ADDR_REQ) = %d",
                         len, sizeof(T_BTIF_LE_READ_LOCAL_RESOLV_ADDR_REQ));
        return;
    }
    btif_le_read_local_resolv_addr_req(p->peer_ident_addr_type, p->peer_ident_addr);
}
void hrp_le_set_resolution_enable_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_RESOLUTION_ENABLE_REQ *p = (T_BTIF_LE_SET_RESOLUTION_ENABLE_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_RESOLUTION_ENABLE_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_RESOLUTION_ENABLE_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_RESOLUTION_ENABLE_REQ));
        return;
    }
    btif_le_set_resolution_enable_req(p->enable);
}
void hrp_le_set_resolv_priv_addr_tout_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_RESOLV_PRIV_ADDR_TOUT_REQ *p = (T_BTIF_LE_SET_RESOLV_PRIV_ADDR_TOUT_REQ *)
                                                 p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_RESOLV_PRIV_ADDR_TOUT_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_RESOLV_PRIV_ADDR_TOUT_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_RESOLV_PRIV_ADDR_TOUT_REQ));
        return;
    }
    btif_le_set_resolv_priv_addr_tout_req(p->timeout);
}
#endif
#if F_BT_LE_LOCAL_IRK_SETTING_SUPPORT
void hrp_le_config_local_irk_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_CONFIG_LOCAL_IRK_REQ *p = (T_BTIF_LE_CONFIG_LOCAL_IRK_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_CONFIG_LOCAL_IRK_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_CONFIG_LOCAL_IRK_REQ) = %d",
                         len, sizeof(T_BTIF_LE_CONFIG_LOCAL_IRK_REQ));
        return;
    }
    btif_le_config_local_irk_req(p->local_irk);
}
#endif
#if F_BT_LE_PRIVACY_SUPPORT
void hrp_le_set_privacy_mode_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_PRIVACY_MODE_REQ *p = (T_BTIF_LE_SET_PRIVACY_MODE_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_PRIVACY_MODE_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_PRIVACY_MODE_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_PRIVACY_MODE_REQ));
        return;
    }
    btif_le_set_privacy_mode_req(p->peer_ident_addr_type, p->peer_ident_addr, p->privacy_mode);
}
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
void hrp_le_set_adv_set_rand_addr_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_ADV_SET_RAND_ADDR_REQ *p = (T_BTIF_LE_SET_ADV_SET_RAND_ADDR_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_ADV_SET_RAND_ADDR_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_ADV_SET_RAND_ADDR_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_ADV_SET_RAND_ADDR_REQ));
        return;
    }
    btif_le_set_adv_set_rand_addr_req(p->rand_addr, p->adv_handle);
}


void hrp_le_ext_adv_param_set_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_EXT_ADV_PARAM_SET_REQ *p = (T_BTIF_LE_EXT_ADV_PARAM_SET_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_EXT_ADV_PARAM_SET_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_EXT_ADV_PARAM_SET_REQ) = %d",
                         len, sizeof(T_BTIF_LE_EXT_ADV_PARAM_SET_REQ));
        return;
    }
    btif_le_ext_adv_param_set_req(p->adv_handle, p->adv_event_prop, p->prim_adv_interval_min,
                                  p->prim_adv_interval_max,
                                  p->prim_adv_chann_map, p->own_addr_type, p->peer_addr_type, p->peer_addr, p->filter_policy,
                                  p->tx_power,
                                  p->prim_adv_phy, p->secondary_adv_max_skip, p->secondary_adv_phy, p->adv_sid,
                                  p->scan_req_notif_enable);
}
void hrp_le_ext_adv_data_set_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_LE_EXT_ADV_DATA_SET_REQ *p = (T_BTIF_LE_EXT_ADV_DATA_SET_REQ *)p_param_list;

    uint8_t         data_type;
    uint8_t         adv_handle;
    uint8_t         op;
    uint8_t         frag_preference;
    uint8_t         data_len;
    uint8_t         *p_data;

    uint8_t pos = 0;

    LE_ARRAY_TO_UINT8(data_type, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(adv_handle, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(op, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(frag_preference, p_param_list + pos); pos++;
    LE_ARRAY_TO_UINT8(data_len, p_param_list + pos); pos++;


    p_data = os_mem_alloc(RAM_TYPE_DATA_ON, data_len);

    memcpy(p_data, p_param_list + pos, data_len);
    pos += data_len;
    APP_PRINT_INFO2("hrp_le_ext_adv_data_set_req:len=%d,pos=%d", len, pos);

    btif_le_ext_adv_data_set_req((T_BTIF_LE_EXT_ADV_DATA_TYPE)data_type, adv_handle,
                                 (T_BTIF_LE_ADV_FRAG_OP_TYPE)op, (T_BTIF_LE_ADV_FRAG_PREFERENCE_TYPE)frag_preference, data_len,
                                 p_data);
    os_mem_free(p_data);

}
void hrp_le_ext_adv_enable_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_EXT_ADV_ENABLE_REQ *p = (T_BTIF_LE_EXT_ADV_ENABLE_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_EXT_ADV_ENABLE_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_EXT_ADV_ENABLE_REQ) = %d",
                         len, sizeof(T_BTIF_LE_EXT_ADV_ENABLE_REQ));
        return;
    }
    btif_le_ext_adv_enable_req(p->mode, p->num_of_sets, p->adv_set_param);
}
#endif
#if  F_BT_LE_5_0_AE_SCAN_SUPPORT
void hrp_le_ext_scan_param_set_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_EXT_SCAN_PARAM_SET_REQ *p = (T_BTIF_LE_EXT_SCAN_PARAM_SET_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_EXT_SCAN_PARAM_SET_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_EXT_SCAN_PARAM_SET_REQ) = %d",
                         len, sizeof(T_BTIF_LE_EXT_SCAN_PARAM_SET_REQ));
        return;
    }
    btif_le_ext_scan_param_set_req(p->local_addr_type, p->filter_policy, p->scan_phys, p->scan_param);
}
void hrp_le_ext_scan_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_EXT_SCAN_REQ *p = (T_BTIF_LE_EXT_SCAN_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_EXT_SCAN_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_EXT_SCAN_REQ) = %d",
                         len, sizeof(T_BTIF_LE_EXT_SCAN_REQ));
        return;
    }
    btif_le_ext_scan_req(p->mode, p->filter_duplicates, p->duration, p->period);
}
#endif
#if F_BT_LE_5_0_SET_PHYS_SUPPORT
void hrp_le_set_default_phy_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_DEFAULT_PHY_REQ *p = (T_BTIF_LE_SET_DEFAULT_PHY_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_DEFAULT_PHY_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_DEFAULT_PHY_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_DEFAULT_PHY_REQ));
        return;
    }
    btif_le_set_default_phy_req(p->all_phys, p->tx_phys, p->rx_phys);
}
void hrp_le_set_phy_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_SET_PHY_REQ *p = (T_BTIF_LE_SET_PHY_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_PHY_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_PHY_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_PHY_REQ));
        return;
    }
    btif_le_set_phy_req(p->link_id, p->all_phys, p->tx_phys, p->rx_phys, p->phy_options);
}
#endif
#if F_BT_LE_5_0_DTM_SUPPORT
void hrp_le_enhanced_receiver_test_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_ENHANCED_RECEIVER_TEST_REQ *p = (T_BTIF_LE_ENHANCED_RECEIVER_TEST_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_ENHANCED_RECEIVER_TEST_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_ENHANCED_RECEIVER_TEST_REQ) = %d",
                         len, sizeof(T_BTIF_LE_ENHANCED_RECEIVER_TEST_REQ));
        return;
    }
    btif_le_enhanced_receiver_test_req(p->rx_chann, p->phy, p->modulation_index);
}
void hrp_le_enhanced_transmitter_test_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_ENHANCED_TRANSMITTER_TEST_REQ *p = (T_BTIF_LE_ENHANCED_TRANSMITTER_TEST_REQ *)
                                                 p_param_list;
    if (len != sizeof(T_BTIF_LE_ENHANCED_TRANSMITTER_TEST_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_ENHANCED_TRANSMITTER_TEST_REQ) = %d",
                         len, sizeof(T_BTIF_LE_ENHANCED_TRANSMITTER_TEST_REQ));
        return;
    }
    btif_le_enhanced_transmitter_test_req(p->tx_chann, p->data_len, p->pkt_pl, p->phy);
}
#endif
void hrp_le_modify_periodic_adv_list_req(uint16_t len, uint8_t *p_param_list)
{
#if F_BT_LE_5_0_PERIODIC_ADV_SUPPORT
    T_BTIF_LE_MODIFY_PERIODIC_ADV_LIST_REQ *p = (T_BTIF_LE_MODIFY_PERIODIC_ADV_LIST_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_MODIFY_PERIODIC_ADV_LIST_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_MODIFY_PERIODIC_ADV_LIST_REQ) = %d",
                         len, sizeof(T_BTIF_LE_MODIFY_PERIODIC_ADV_LIST_REQ));
        return;
    }
    btif_le_modify_periodic_adv_list_req(p->op, p->advertiser_addr_type, p->advertiser_addr,
                                         p->adv_sid);
#endif

}
#if F_BT_LE_5_0_RF_PATH_SUPPORT
void hrp_le_read_rf_path_compensation_req(uint16_t len, uint8_t *p_param_list)
{
    btif_le_read_rf_path_compensation_req();
}

void hrp_le_write_rf_path_compensation_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_WRITE_RF_PATH_COMPENSATION_REQ *p = (T_BTIF_LE_WRITE_RF_PATH_COMPENSATION_REQ *)
                                                  p_param_list;
    if (len != sizeof(T_BTIF_LE_WRITE_RF_PATH_COMPENSATION_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_WRITE_RF_PATH_COMPENSATION_REQ) = %d",
                         len, sizeof(T_BTIF_LE_WRITE_RF_PATH_COMPENSATION_REQ));
        return;
    }
    btif_le_write_rf_path_compensation_req(p->rf_tx_path_compensation, p->rf_rx_path_compensation);
}
#endif
#if F_BT_LE_5_0_AE_ADV_SUPPORT
void hrp_le_modify_adv_set_req(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_LE_MODIFY_ADV_SET_REQ *p = (T_BTIF_LE_MODIFY_ADV_SET_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_MODIFY_ADV_SET_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_MODIFY_ADV_SET_REQ) = %d",
                         len, sizeof(T_BTIF_LE_MODIFY_ADV_SET_REQ));
        return;
    }
    btif_le_modify_adv_set_req(p->operation, p->adv_handle);
}
#endif
//**********????
void hrp_le_set_periodic_adv_param_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_LE_SET_PERIODIC_ADV_PARAM_REQ *p = (T_BTIF_LE_SET_PERIODIC_ADV_PARAM_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_PERIODIC_ADV_PARAM_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_PERIODIC_ADV_PARAM_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_PERIODIC_ADV_PARAM_REQ));
        return;
    }
    //btif_le_set_periodic_adv_param_req(p->adv_handle, p->adv_interval_min, p->adv_interval_max,
    //                                   p->adv_prop);
}
void hrp_le_set_periodic_adv_data_req(uint16_t len, uint8_t *p_param_list)
{
    // T_BTIF_LE_SET_PERIODIC_ADV_DATA_REQ *p = (T_BTIF_LE_SET_PERIODIC_ADV_DATA_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_PERIODIC_ADV_DATA_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_PERIODIC_ADV_DATA_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_PERIODIC_ADV_DATA_REQ));
        return;
    }
    //btif_le_set_periodic_adv_data_req(p->adv_handle, p->op, p->data_len, p->p_data);
}
void hrp_le_set_periodic_adv_enable_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_LE_SET_PERIODIC_ADV_ENABLE_REQ *p = (T_BTIF_LE_SET_PERIODIC_ADV_ENABLE_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_SET_PERIODIC_ADV_ENABLE_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_SET_PERIODIC_ADV_ENABLE_REQ) = %d",
                         len, sizeof(T_BTIF_LE_SET_PERIODIC_ADV_ENABLE_REQ));
        return;
    }
    //btif_le_set_periodic_adv_enable_req(p->mode, p->adv_handle);
}
void hrp_le_periodic_adv_create_sync_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_LE_PERIODIC_ADV_CREATE_SYNC_REQ *p = (T_BTIF_LE_PERIODIC_ADV_CREATE_SYNC_REQ *)p_param_list;
    if (len != sizeof(T_BTIF_LE_PERIODIC_ADV_CREATE_SYNC_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_PERIODIC_ADV_CREATE_SYNC_REQ) = %d",
                         len, sizeof(T_BTIF_LE_PERIODIC_ADV_CREATE_SYNC_REQ));
        return;
    }
    //btif_le_periodic_adv_create_sync_req(p->filter_policy, p->adv_sid, p->adv_addr_type, p->adv_add,
    //                                     p->skip,
    //                                     p->sync_timeout, p->unused);
}
void hrp_le_periodic_adv_create_sync_cancel_req(uint16_t len, uint8_t *p_param_list)
{
    //btif_le_periodic_adv_create_sync_cancel_req();
}
void hrp_le_periodic_adv_terminate_sync_req(uint16_t len, uint8_t *p_param_list)
{
    //T_BTIF_LE_PERIODIC_ADV_TERMINATE_SYNC_REQ *p = (T_BTIF_LE_PERIODIC_ADV_TERMINATE_SYNC_REQ *)
    //                                               p_param_list;
    if (len != sizeof(T_BTIF_LE_PERIODIC_ADV_TERMINATE_SYNC_REQ))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_LE_PERIODIC_ADV_TERMINATE_SYNC_REQ) = %d",
                         len, sizeof(T_BTIF_LE_PERIODIC_ADV_TERMINATE_SYNC_REQ));
        return;
    }
    //btif_le_periodic_adv_terminate_sync_req(p->sync_handle);
}
//*******????
#if F_BT_LE_5_0_AE_ADV_SUPPORT
void hrp_le_enable_ext_adv_mode_req(uint16_t len, uint8_t *p_param_list)
{
    btif_le_enable_ext_adv_mode_req();
}
#endif


void hrp_just_work_req_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_JUST_WORK_REQ_CFM *p = (T_BTIF_JUST_WORK_REQ_CFM *) p_param_list;
    if (len != sizeof(T_BTIF_JUST_WORK_REQ_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_JUST_WORK_REQ_CFM) = %d",
                         len, sizeof(T_BTIF_JUST_WORK_REQ_CFM));
        return;
    }
    APP_PRINT_INFO0("hrp_just_work_req_cfm");
    btif_just_work_req_cfm(p->bd_addr, p->remote_addr_type,
                           p->cause);
}

void hrp_user_passkey_notif_cfm(uint16_t len, uint8_t *p_param_list)
{
    T_BTIF_PASSKEY_NOTIF_CFM *p = (T_BTIF_PASSKEY_NOTIF_CFM *) p_param_list;
    if (len != sizeof(T_BTIF_PASSKEY_NOTIF_CFM))
    {
        APP_PRINT_ERROR2("len= %d, sizeof(T_BTIF_PASSKEY_NOTIF_CFM) = %d",
                         len, sizeof(T_BTIF_PASSKEY_NOTIF_CFM));
        return;
    }
    APP_PRINT_INFO0("hrp_user_passkey_notif_cfm");

    btif_user_passkey_notif_cfm(p->bd_addr, p->remote_addr_type,
                                p->cause);
}


#endif
