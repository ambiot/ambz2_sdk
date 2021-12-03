/**
*****************************************************************************************
*     Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     scheduler_server_app.c
  * @brief    Source file for scheduler client application.
  * @details  User command interfaces.
  * @author   
  * @date     2020-4-1
  * @version  v1.0
  * *************************************************************************************
  */
#include "scheduler_server_app.h"
#include "scheduler.h"

uint16_t cur_schedulers = 0;
scheduler_server_get_action_t scheduler_action = {0};
scheduler_server_set_action_t scheduler_action_set_buffer = {0};

mesh_model_info_t scheduler_server_model;
mesh_model_info_t scheduler_setup_server_model;

static int32_t scheduler_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
        case SCHEDULER_SERVER_GET:
            {
            	scheduler_server_get_t *p_get_data = NULL;
				p_get_data = (scheduler_server_get_t *)pargs;

				if (p_get_data) {
					p_get_data->schedulers = cur_schedulers;
				}
				data_uart_debug("scheduler server receive: schedulers = %d\r\n", cur_schedulers);
            }
            break;
        case SCHEDULER_SERVER_GET_ACTION:
            {
			    scheduler_server_get_action_t *p_get_data = NULL;
				p_get_data = (scheduler_server_get_action_t *)pargs;

				if (p_get_data) {
					p_get_data->index = scheduler_action.index;
					(p_get_data->scheduler).index = scheduler_action.scheduler.index;
					(p_get_data->scheduler).year = scheduler_action.scheduler.year;
					(p_get_data->scheduler).month = scheduler_action.scheduler.month;
					(p_get_data->scheduler).day = scheduler_action.scheduler.day;
					(p_get_data->scheduler).hour = scheduler_action.scheduler.hour;
					(p_get_data->scheduler).minute = scheduler_action.scheduler.minute;
					(p_get_data->scheduler).second = scheduler_action.scheduler.second;
					(p_get_data->scheduler).day_of_week = scheduler_action.scheduler.day_of_week;
					(p_get_data->scheduler).action = scheduler_action.scheduler.action;
					(p_get_data->scheduler).num_steps = scheduler_action.scheduler.num_steps;
					(p_get_data->scheduler).step_resolution = scheduler_action.scheduler.step_resolution;
					(p_get_data->scheduler).scene_number = scheduler_action.scheduler.scene_number;
				}
                data_uart_debug(
					"scheduler server receive: index = %d, " \
					"index = %d, year = %d, month = %d, day = %d, hour = %d, minute = %d, second = %d, " \
					"day_of_week = %d, action = %d, num_steps = %d, step_resolution = %d, scene_number = %d\r\n", \
					scheduler_action.index, scheduler_action.scheduler.index, scheduler_action.scheduler.year, scheduler_action.scheduler.month, \
					scheduler_action.scheduler.day, scheduler_action.scheduler.hour, scheduler_action.scheduler.minute, scheduler_action.scheduler.second, \
					scheduler_action.scheduler.day_of_week, scheduler_action.scheduler.action, scheduler_action.scheduler.num_steps, \
					scheduler_action.scheduler.step_resolution, scheduler_action.scheduler.scene_number);
            }
            break;
        default:
            break;
    }

    return 0;
}

static int32_t scheduler_setup_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
        case SCHEDULER_SERVER_SET_ACTION:
            {
            	scheduler_server_set_action_t *p_get_data = pargs;
            	p_get_data = (scheduler_server_set_action_t *)pargs;

				if (p_get_data) {
					scheduler_action_set_buffer.index = p_get_data->index;
					scheduler_action_set_buffer.year = p_get_data->year;
					scheduler_action_set_buffer.month = p_get_data->month;
					scheduler_action_set_buffer.day = p_get_data->day;
					scheduler_action_set_buffer.hour = p_get_data->hour;
					scheduler_action_set_buffer.minute = p_get_data->minute;
					scheduler_action_set_buffer.second = p_get_data->second;
					scheduler_action_set_buffer.day_of_week = p_get_data->day_of_week;
					scheduler_action_set_buffer.action = p_get_data->action;
					scheduler_action_set_buffer.num_steps = p_get_data->num_steps;
					scheduler_action_set_buffer.step_resolution = p_get_data->step_resolution;
					scheduler_action_set_buffer.scene_number = p_get_data->scene_number;
				}
				data_uart_debug(
					"scheduler setup server receive: index = %d, " \
					"index = %d, year = %d, month = %d, day = %d, hour = %d, minute = %d, second = %d, " \
					"day_of_week = %d, action = %d, num_steps = %d, step_resolution = %d, scene_number = %d\r\n", \
					scheduler_action_set_buffer.index, scheduler_action_set_buffer.year, scheduler_action_set_buffer.month, \
					scheduler_action_set_buffer.day, scheduler_action_set_buffer.hour, scheduler_action_set_buffer.minute, \
					scheduler_action_set_buffer.second, scheduler_action_set_buffer.day_of_week, scheduler_action_set_buffer.action, \
					scheduler_action_set_buffer.num_steps, scheduler_action_set_buffer.step_resolution, scheduler_action_set_buffer.scene_number);
            }
            break;
        case SCHEDULER_SERVER_GET_ACTION:
            {
			scheduler_server_get_action_t *p_get_data = NULL;
			p_get_data = (scheduler_server_get_action_t *)pargs;

			if (p_get_data) {
				p_get_data->index = scheduler_action.index;
				(p_get_data->scheduler).index = scheduler_action.scheduler.index;
				(p_get_data->scheduler).year = scheduler_action.scheduler.year;
				(p_get_data->scheduler).month = scheduler_action.scheduler.month;
				(p_get_data->scheduler).day = scheduler_action.scheduler.day;
				(p_get_data->scheduler).hour = scheduler_action.scheduler.hour;
				(p_get_data->scheduler).minute = scheduler_action.scheduler.minute;
				(p_get_data->scheduler).second = scheduler_action.scheduler.second;
				(p_get_data->scheduler).day_of_week = scheduler_action.scheduler.day_of_week;
				(p_get_data->scheduler).action = scheduler_action.scheduler.action;
				(p_get_data->scheduler).num_steps = scheduler_action.scheduler.num_steps;
				(p_get_data->scheduler).step_resolution = scheduler_action.scheduler.step_resolution;
				(p_get_data->scheduler).scene_number = scheduler_action.scheduler.scene_number;
			}
			data_uart_debug(
				"scheduler server receive: index = %d, " \
				"index = %d, year = %d, month = %d, day = %d, hour = %d, minute = %d, second = %d, " \
				"day_of_week = %d, action = %d, num_steps = %d, step_resolution = %d, scene_number = %d\r\n", \
				scheduler_action.index, scheduler_action.scheduler.index, scheduler_action.scheduler.year, scheduler_action.scheduler.month, \
				scheduler_action.scheduler.day, scheduler_action.scheduler.hour, scheduler_action.scheduler.minute, scheduler_action.scheduler.second, \
				scheduler_action.scheduler.day_of_week, scheduler_action.scheduler.action, scheduler_action.scheduler.num_steps, \
				scheduler_action.scheduler.step_resolution, scheduler_action.scheduler.scene_number);
            }
            break;
        default:
            break;
    }

    return 0;
}

void scheduler_server_model_init(void)
{
    scheduler_setup_server_model.model_data_cb = scheduler_setup_server_data;
    scheduler_setup_server_reg(0, &scheduler_setup_server_model);

    scheduler_server_model.model_data_cb = scheduler_server_data;
    scheduler_server_reg(0, &scheduler_server_model);
}
