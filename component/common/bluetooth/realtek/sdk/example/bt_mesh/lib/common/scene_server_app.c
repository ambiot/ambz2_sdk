/**
*****************************************************************************************
*     Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     scene_server_app.c
  * @brief    Source file for scene client application.
  * @details  User command interfaces.
  * @author   
  * @date     2020-4-1
  * @version  v1.0
  * *************************************************************************************
  */
#include "scene_server_app.h"
#include "scene.h"

uint16_t cur_scene = 0;
uint16_t deleted_scene = 0;
scene_status_code_t cur_status = 0;

scene_server_recall_t cur_scene_recall = {0};
generic_transition_time_t scene_trans_time = {0};
scene_server_store_t scene_store_buffer = {0};

mesh_model_info_t scene_server_model;
mesh_model_info_t scene_setup_server_model;

static scene_storage_memory_t scene_storage_memory[] =
{
    {3, NULL},
    {4, NULL},
    {5, NULL},
    {6, NULL}
};

static int32_t scene_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
        case SCENE_SERVER_GET:
            {
            	scene_server_get_t *p_get_data = NULL;
				
            	p_get_data = (scene_server_get_t *)pargs;

				if (p_get_data) {
					p_get_data->current_scene = cur_scene;
				}
				data_uart_debug("scene server receive: current_scene = %d\r\n", cur_scene);
            }
            break;
        case SCENE_SERVER_RECALL:
            {
            	scene_server_recall_t *p_get_data = NULL;
				p_get_data = (scene_server_recall_t *)pargs;

				if (p_get_data) {
					p_get_data->scene_number = cur_scene_recall.scene_number;
					(p_get_data->total_time).num_steps = cur_scene_recall.total_time.num_steps;
					(p_get_data->total_time).step_resolution = cur_scene_recall.total_time.step_resolution;
					(p_get_data->remaining_time).num_steps = cur_scene_recall.remaining_time.num_steps;
					(p_get_data->remaining_time).step_resolution = cur_scene_recall.remaining_time.step_resolution;
				}
                data_uart_debug(
					"scene server receive: scene_number = %d, total time = step(%d), resolution(%d)," \
					"remaining time = step(%d), resolution(%d)\r\n", \
					cur_scene_recall.scene_number, cur_scene_recall.total_time.num_steps, cur_scene_recall.total_time.step_resolution, \
					cur_scene_recall.remaining_time.num_steps, cur_scene_recall.remaining_time.step_resolution);
            }
            break;
        case SCENE_SERVER_GET_DEFAULT_TRANSITION_TIME:
            {
            }
            break;
        case SCENE_SERVER_GET_REGISTER_STATUS:
            {
            	scene_server_get_register_status_t *p_get_data = NULL;
				p_get_data = (scene_server_get_register_status_t *)pargs;

				if (p_get_data) {
					p_get_data->status = cur_status;
				}
                data_uart_debug("scene server receive: status = %d\r\n", cur_status);
            }
            break;
        default:
            break;
    }

    return 0;
}

static int32_t scene_setup_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
        case SCENE_SERVER_STORE:
            {
            	scene_server_store_t *p_get_data = NULL;
            	p_get_data = (scene_server_store_t *)pargs;

				if (p_get_data) {
					scene_store_buffer.status = p_get_data->status;
					scene_store_buffer.scene_number = p_get_data->scene_number;
				}
                data_uart_debug("scene setup server receive: status = %d, scene number = %d\r\n", \
								scene_store_buffer.status, scene_store_buffer.scene_number);
            }
            break;
        case SCENE_SERVER_DELETE:
            {
            	scene_server_delete_t *p_get_data = pargs;
				p_get_data = (scene_server_delete_t *)pargs;

				if (p_get_data) {
					p_get_data->scene_number = deleted_scene;
				}
                data_uart_debug("scene setup server receive: scene number = %d\r\n", deleted_scene);
            }
            break;
        default:
            break;
    }

    return 0;
}

void scene_server_model_init(void)
{
    scene_setup_server_model.model_data_cb = scene_setup_server_data;
    scene_setup_server_reg(0, &scene_setup_server_model);

    scene_server_model.model_data_cb = scene_server_data;
    scene_server_reg(0, &scene_server_model);
	
	scene_server_set_storage_memory(&scene_server_model, scene_storage_memory,
                                sizeof(scene_storage_memory) / sizeof(scene_storage_memory_t));
    scene_setup_server_set_storage_memory(&scene_setup_server_model, scene_storage_memory,
                                          sizeof(scene_storage_memory) / sizeof(scene_storage_memory_t));
}
