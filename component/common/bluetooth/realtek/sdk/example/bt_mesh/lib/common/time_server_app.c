/**
*****************************************************************************************
*     Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     time_server_app.c
  * @brief    Source file for time client application.
  * @details  User command interfaces.
  * @author   
  * @date     2020-3-30
  * @version  v1.0
  * *************************************************************************************
  */

#include "time_server_app.h"

mesh_model_info_t time_server_model;
mesh_model_info_t time_setup_server_model;


time_server_get_t						   cur_time = {0};
time_server_get_zone_t         			   time_zone = {0};
time_server_get_tai_utc_delta_t            time_tai_utc_delta = {0};
time_server_get_role_t					   time_role = {0};
time_server_set_t              			   time_set_buffer = {0};
time_server_set_zone_t                 	   time_zone_buffer = {0};
time_server_set_tai_utc_delta_t            time_tai_utc_delta_buffer = {0};
time_server_set_role_t					   time_role_buffer = {0};
time_server_status_set_t 				   time_status_buffer ={0};


static int32_t time_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case TIME_SERVER_GET:
        {
            time_server_get_t *p_get_data = NULL;
            p_get_data = (time_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->tai_seconds[4] = cur_time.tai_seconds[4];
				p_get_data->subsecond = cur_time.subsecond;
				p_get_data->uncertainty = cur_time.uncertainty;
				p_get_data->time_authority = cur_time.time_authority;
				p_get_data->tai_utc_delta = cur_time.tai_utc_delta;
				p_get_data->time_zone_offset = cur_time.time_zone_offset;
            }
            data_uart_debug("time server receive: tai seconds %d, subsecond %d, uncertainty %d, time authority %d, tai utc delta %d, time zone offset %d \r\n", 
            				cur_time.tai_seconds[4],
            				cur_time.subsecond,
            				cur_time.uncertainty,
            				cur_time.time_authority,
            				cur_time.tai_utc_delta,
            				cur_time.time_zone_offset);
        }
        break;
    case TIME_SERVER_GET_ZONE:
        {
            time_server_get_zone_t *p_get_data = NULL;
            p_get_data = (time_server_get_zone_t *)pargs;
            if (p_get_data) {
                p_get_data->time_zone_offset_current = time_zone.time_zone_offset_current;
				p_get_data->time_zone_offset_new = time_zone.time_zone_offset_new;
				p_get_data->tai_of_zone_change[4] = time_zone.tai_of_zone_change[4];
            }
            data_uart_debug("time server receive: current time zone offset %d, new time zone offset %d, tai of zone change %d \r\n", 
            				time_zone.time_zone_offset_current,
            				time_zone.time_zone_offset_new,
            				time_zone.tai_of_zone_change[4]);
        }
        break;
    case TIME_SERVER_GET_TAI_UTC_DELTA:
        {
            time_server_get_tai_utc_delta_t *p_get_data = NULL;
            p_get_data = (time_server_get_tai_utc_delta_t *)pargs;
            if (p_get_data) {
                p_get_data->tai_utc_delta_current = time_tai_utc_delta.tai_utc_delta_current,
				p_get_data->padding1 = time_tai_utc_delta.padding1,
				p_get_data->tai_utc_delta_new = time_tai_utc_delta.tai_utc_delta_new,
				p_get_data->padding2 = time_tai_utc_delta.padding2,
				p_get_data->tai_of_delta_change[4] = time_tai_utc_delta.tai_of_delta_change[4];
				
            }
            data_uart_debug("time server receive: current tai utc delta %d, padding1 %d, new tai utc delta %d, padding2 %d, tai of delta change %d \r\n",
            				time_tai_utc_delta.tai_utc_delta_current,
            				time_tai_utc_delta.padding1,
            				time_tai_utc_delta.tai_utc_delta_new,
            				time_tai_utc_delta.padding2,
            				time_tai_utc_delta.tai_of_delta_change[4]);
        }
        break;
    case TIME_SERVER_STATUS_SET:
        {
            time_server_status_set_t *p_get_data = NULL;
            p_get_data = (time_server_status_set_t *)pargs;
            if (p_get_data) {
                time_status_buffer.tai_seconds[4] = p_get_data->tai_seconds[4];
				time_status_buffer.subsecond = p_get_data->subsecond;
				time_status_buffer.uncertainty = p_get_data->uncertainty;
				time_status_buffer.tai_utc_delta = p_get_data->tai_utc_delta;
				time_status_buffer.time_zone_offset = p_get_data->time_zone_offset;
            }
            data_uart_debug("time server receive: tai seconds %d, subsecond %d, uncertainty %d, tai utc delta %d, time zone offset %d \r\n", 
            				time_status_buffer.tai_seconds[4],
            				time_status_buffer.subsecond,
            				time_status_buffer.uncertainty,
            				time_status_buffer.tai_utc_delta,
            				time_status_buffer.time_zone_offset);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t time_setup_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case TIME_SERVER_SET:
        {
            time_server_set_t *p_get_data = NULL;
            p_get_data = (time_server_set_t *)pargs;
            if (p_get_data) {
                time_set_buffer.tai_seconds[4] = p_get_data->tai_seconds[4];
				time_set_buffer.subsecond = p_get_data->subsecond ;
				time_set_buffer.uncertainty = p_get_data->uncertainty;
				time_set_buffer.time_authority = p_get_data->time_authority;
				time_set_buffer.tai_utc_delta = p_get_data->tai_utc_delta;
				time_set_buffer.time_zone_offset = p_get_data->time_zone_offset;
            }
            data_uart_debug("time server receive: set tai seconds %d, subsecond %d, uncertainty %d, time authority %d, tai utc delta %d, time zone offset %d \r\n", 
            				time_set_buffer.tai_seconds[4],
            				time_set_buffer.subsecond,
            				time_set_buffer.uncertainty,
            				time_set_buffer.time_authority,
            				time_set_buffer.tai_utc_delta,
            				time_set_buffer.time_zone_offset);
        }
        break;
    case TIME_SERVER_SET_ZONE:
        {
            time_server_set_zone_t *p_get_data = NULL;
            p_get_data = (time_server_set_zone_t *)pargs;
            if (p_get_data) {
                time_zone_buffer.tai_of_zone_change[4] = p_get_data->tai_of_zone_change[4];
                time_zone_buffer.time_zone_offset_new = p_get_data->time_zone_offset_new;
            }
            data_uart_debug("time setup server receive: set tai of zone change %d, new time zone offset new %d \r\n", 
                            time_zone_buffer.tai_of_zone_change[4], 
                            time_zone_buffer.time_zone_offset_new);
        }
        break;
	 case TIME_SERVER_GET_ROLE:
        {
            time_server_get_role_t *p_get_data = NULL;
            p_get_data = (time_server_get_role_t *)pargs;
            if (p_get_data) {
                p_get_data->role = time_role.role;
            }
            data_uart_debug("time setup server receive: role %d \r\n", 
                            time_role.role);
        }
        break;
	  case TIME_SERVER_SET_ROLE:
        {
            time_server_set_role_t *p_get_data = NULL;
            p_get_data = (time_server_set_role_t *)pargs;
            if (p_get_data) {
                time_role_buffer.role = p_get_data->role;
            }
            data_uart_debug("time setup server receive: set role %d \r\n", 
                            time_role_buffer.role);
        }
        break;
	   case TIME_SERVER_SET_TAI_UTC_DELTA:
        {
            time_server_set_tai_utc_delta_t *p_get_data = NULL;
            p_get_data = (time_server_set_tai_utc_delta_t *)pargs;
            if (p_get_data) {
                time_tai_utc_delta_buffer.tai_of_delta_change[4] = p_get_data->tai_of_delta_change[4];
                time_tai_utc_delta_buffer.tai_utc_delta_new = p_get_data->tai_utc_delta_new;
                time_tai_utc_delta_buffer.padding = p_get_data->padding;
            }
            data_uart_debug("time setup server receive: set tai of delta change %d, new time utc delta %d, padding %d \r\n", 
                            time_tai_utc_delta_buffer.tai_of_delta_change[4], 
                            time_tai_utc_delta_buffer.tai_utc_delta_new,
                            time_tai_utc_delta_buffer.padding);
        }
        break;
    default:
        break;
    }

    return 0;
}



void time_server_models_init(void)
{
    time_server_model.model_data_cb = time_server_data;
    time_server_reg(0, &time_server_model);

    time_setup_server_model.model_data_cb = time_setup_server_data;
    time_setup_server_reg(0, &time_setup_server_model);
}
