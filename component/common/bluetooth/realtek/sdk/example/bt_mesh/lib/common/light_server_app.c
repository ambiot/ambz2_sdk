/**
*****************************************************************************************
*     Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     light_server_app.c
  * @brief    Source file for light client application.
  * @details  User command interfaces.
  * @author   
  * @date     2020-3-25
  * @version  v1.0
  * *************************************************************************************
  */

#include "light_server_app.h"

mesh_model_info_t lighting_lightness_server_model;
mesh_model_info_t lighting_lightness_setup_server_model;
mesh_model_info_t lighting_ctl_server_model;
mesh_model_info_t lighting_ctl_setup_server_model;
mesh_model_info_t lighting_ctl_temperature_server_model;
mesh_model_info_t lighting_hsl_server_model;
mesh_model_info_t lighting_hsl_hue_server_model;
mesh_model_info_t lighting_hsl_saturation_server_model;
mesh_model_info_t lighting_hsl_setup_server_model;
mesh_model_info_t lighting_xyl_server_model;
mesh_model_info_t lighting_xyl_setup_server_model;


uint16_t cur_lightness = 0;
uint16_t cur_linear_lightness = 0;
uint16_t default_lightness = 0;
uint16_t last_lightness = 0;

light_lightness_server_get_range_t         lightness_range = {0};
generic_transition_time_t                  lightness_transition_time = {GENERIC_TRANSITION_NUM_STEPS_IMMEDIATE, 0};
light_lightness_server_set_t               lightness_set_buffer = {0};
light_lightness_server_set_t               linear_lightness_setting_buffer = {0};

light_ctl_server_get_t                     cur_light_ctl = {0};
light_ctl_server_get_default_t             default_light_ctl = {0};
light_ctl_server_get_temperature_range_t   light_ctl_temperature_range = {0};
generic_transition_time_t                  light_ctl_trans_time = {GENERIC_TRANSITION_NUM_STEPS_IMMEDIATE, 0};
light_ctl_server_set_t                     light_ctl_set_buffer = {0};
light_ctl_server_get_temperature_t         light_ctl_temperature = {0};
light_ctl_server_set_temperature_t         light_ctl_temperature_buffer = {0};
light_ctl_server_set_temperature_range_t   light_ctl_temperature_set_range_buffer = {0};
light_ctl_server_set_default_t             light_ctl_default = {0};

light_hsl_server_get_t                     cur_light_hsl = {0};
light_hsl_server_get_default_t             default_light_hsl = {0};
light_hsl_server_get_range_t               light_hsl_range = {0};
generic_transition_time_t                  light_hsl_trans_time = {0};
light_hsl_server_set_t                     light_hsl_trans_set_buffer = {0};

light_hsl_server_get_hue_t                 cur_light_hsl_hue = {0};
light_hsl_server_set_hue_t                 light_hsl_hue_set_buffer = {0};

light_hsl_server_get_saturation_t          cur_light_hsl_saturation = {0};
light_hsl_server_set_saturation_t          light_hsl_trans_set_saturation_buffer = {0};

light_hsl_server_set_default_t             light_hsl_default = {0};
light_hsl_server_set_range_t               light_hsl_set_range_buffer = {0};

light_xyl_server_get_t					   cur_light_xyl = {0};
light_xyl_server_get_default_t             default_light_xyl = {0};
light_xyl_server_get_range_t               light_xyl_range = {0};
generic_transition_time_t				   light_xyl_trans_time = {0};

light_xyl_server_set_t                     light_xyl_set_buffer = {0};
light_xyl_server_set_default_t             light_xyl_default = {0};
light_xyl_server_set_range_t               light_xyl_set_range_buffer = {0};



static int32_t lighting_lightness_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_LIGHTNESS_SERVER_GET:
        {
            light_lightness_server_get_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->lightness = cur_lightness;
            }
            data_uart_debug("light lightness server receive: lightness %d \r\n", cur_lightness);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_GET_LINEAR:
        {
            light_lightness_server_get_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->lightness = cur_linear_lightness;
            }
            data_uart_debug("light lightness server receive: linear lightness %d \r\n", cur_linear_lightness);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_GET_DEFAULT:
        {
            light_lightness_server_get_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->lightness = default_lightness;
            }
            data_uart_debug("light lightness server receive: default lightness %d \r\n", default_lightness);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_GET_LAST:
        {
            light_lightness_server_get_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->lightness = last_lightness;
            }
            data_uart_debug("light lightness server receive: last lightness %d \r\n", last_lightness);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_GET_RANGE:
        {
            light_lightness_server_get_range_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_get_range_t *)pargs;
            if (p_get_data) {
                p_get_data->range_max = lightness_range.range_max;
                p_get_data->range_min = lightness_range.range_min;
            }
            data_uart_debug("light lightness server receive: lightness_max %d, lightness_min %d \r\n", 
                            lightness_range.range_max, 
                            lightness_range.range_min);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_GET_DEFAULT_TRANSITION_TIME:
        {
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_SET:
        {
            light_lightness_server_set_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_set_t *)pargs;
            if (p_get_data) {
                lightness_set_buffer.lightness = p_get_data->lightness;
                lightness_set_buffer.total_time = p_get_data->total_time;
                lightness_set_buffer.remaining_time = p_get_data->remaining_time;
            }
            data_uart_debug("light lightness server receive: set lightness %d, total_time %d, remaining_time %d \r\n", 
                            lightness_set_buffer.lightness, 
                            lightness_set_buffer.total_time, 
                            lightness_set_buffer.remaining_time);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_SET_LINEAR:
        {
            light_lightness_server_set_t * p_get_data = NULL;
            p_get_data = (light_lightness_server_set_t *)pargs;
            if (p_get_data) {
                linear_lightness_setting_buffer.lightness = p_get_data->lightness;
                linear_lightness_setting_buffer.total_time = p_get_data->total_time;
                linear_lightness_setting_buffer.remaining_time = p_get_data->remaining_time;
            }
            data_uart_debug("light lightness server receive: set linear lightness %d, total_time %d, remaining_time %d \r\n", 
                            linear_lightness_setting_buffer.lightness, 
                            linear_lightness_setting_buffer.total_time, 
                            linear_lightness_setting_buffer.remaining_time);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_SET_LAST:
        {
            light_lightness_server_get_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_get_t *)pargs;
            if (p_get_data) {
                last_lightness = p_get_data->lightness;
            }
            data_uart_debug("light lightness server receive: set last lightness %d \r\n", last_lightness);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_lightness_setup_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_LIGHTNESS_SERVER_SET_DEFAULT:
        {
            light_lightness_server_set_default_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_set_default_t *)pargs;
            if (p_get_data) {
                default_lightness = p_get_data->lightness;
            }
            data_uart_debug("light lightness server receive: set default lightness %d \r\n", default_lightness);
        }
        break;
    case LIGHT_LIGHTNESS_SERVER_SET_RANGE:
        {
            light_lightness_server_set_range_t *p_get_data = NULL;
            p_get_data = (light_lightness_server_set_range_t *)pargs;
            if (p_get_data) {
                lightness_range.range_max = p_get_data->range_max;
                lightness_range.range_min = p_get_data->range_min;
            }
            data_uart_debug("light lightness server receive: set lightness_max %d, lightness_min %d \r\n", 
                            lightness_range.range_max, 
                            lightness_range.range_min);
        }
        break;
    default:
        break;
    }

    return 0;
}


static int32_t lighting_ctl_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_CTL_SERVER_GET:
        {
            light_ctl_server_get_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->lightness = cur_light_ctl.lightness;
                p_get_data->temperature = cur_light_ctl.temperature;
            }
            data_uart_debug("lighting ctl server receive: lightness %d, temperature %d \r\n", 
                            cur_light_ctl.lightness, 
                            cur_light_ctl.temperature);
        }
        break;
    case LIGHT_CTL_SERVER_GET_DEFAULT:
        {
            light_ctl_server_get_default_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_get_default_t *)pargs;
            if (p_get_data) {
                p_get_data->lightness = default_light_ctl.lightness;
                p_get_data->temperature = default_light_ctl.temperature;
                p_get_data->delta_uv = default_light_ctl.delta_uv;
            }
            data_uart_debug("lighting ctl server receive: lightness %d, temperature %d, delta_uv %d \r\n", 
                            default_light_ctl.lightness, 
                            default_light_ctl.temperature, 
                            default_light_ctl.delta_uv);
        }
        break;
    case LIGHT_CTL_SERVER_GET_TEMPERATURE_RANGE:
        {
            light_ctl_server_get_temperature_range_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_get_temperature_range_t *)pargs;
            if (p_get_data) {
                p_get_data->range_max = light_ctl_temperature_range.range_max;
                p_get_data->range_min = light_ctl_temperature_range.range_min;
            }
            data_uart_debug("lighting ctl server receive: temperature range_max %d, range_min %d \r\n", 
                            light_ctl_temperature_range.range_max, 
                            light_ctl_temperature_range.range_min);
        }
        break;
    case LIGHT_CTL_SERVER_GET_DEFAULT_TRANSITION_TIME:
        {
        }
        break;
    case LIGHT_CTL_SERVER_SET:
        {
            light_ctl_server_set_t light_ctl_set_buffer;
            light_ctl_server_set_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_set_t *)pargs;
            if (p_get_data) {
                light_ctl_set_buffer.delta_uv = p_get_data->delta_uv;
                light_ctl_set_buffer.lightness = p_get_data->lightness;
                light_ctl_set_buffer.remaining_time = p_get_data->remaining_time;
                light_ctl_set_buffer.temperature = p_get_data->temperature;
                light_ctl_set_buffer.total_time = p_get_data->total_time;
            }
            data_uart_debug("lighting ctl server receive: set delta_uv %d, lightness %d, remaining_time %d, temperature %d, total_time %d \r\n", 
                            light_ctl_set_buffer.delta_uv, 
                            light_ctl_set_buffer.lightness, 
                            light_ctl_set_buffer.remaining_time, 
                            light_ctl_set_buffer.temperature, 
                            light_ctl_set_buffer.total_time);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_ctl_temperature_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_CTL_SERVER_GET_TEMPERATURE:
        {
            light_ctl_server_get_temperature_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_get_temperature_t *)pargs;
            if (p_get_data) {
                p_get_data->temperature = light_ctl_temperature.temperature;
                p_get_data->delta_uv = light_ctl_temperature.delta_uv;
            }
            data_uart_debug("lighting ctl temperature server receive: set temperature %d, delta_uv %d \r\n", 
                            light_ctl_temperature.temperature, 
                            light_ctl_temperature.delta_uv);
        }
        break;
    case LIGHT_CTL_SERVER_SET_TEMPERATURE:
        {
            light_ctl_server_set_temperature_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_set_temperature_t *)pargs;
            if (p_get_data) {
                light_ctl_temperature_buffer.temperature = p_get_data->temperature; 
                light_ctl_temperature_buffer.delta_uv = p_get_data->delta_uv;
                light_ctl_temperature_buffer.total_time = p_get_data->total_time;
                light_ctl_temperature_buffer.remaining_time = p_get_data->remaining_time;
            }
            data_uart_debug("lighting ctl temperature server receive: set temperature %d, delta_uv %d, total_time %d, remaining_time %d \r\n", 
                            light_ctl_temperature_buffer.temperature, 
                            light_ctl_temperature_buffer.delta_uv, 
                            light_ctl_temperature_buffer.total_time, 
                            light_ctl_temperature_buffer.remaining_time);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_ctl_setup_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_CTL_SERVER_SET_TEMPERATURE_RANGE:
        {
            light_ctl_server_set_temperature_range_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_set_temperature_range_t *)pargs;
            if (p_get_data) {
                light_ctl_temperature_set_range_buffer.range_max = p_get_data->range_max;
                light_ctl_temperature_set_range_buffer.range_min = p_get_data->range_min;
            }
            data_uart_debug("lighting ctl setup server receive: set temperature range_max %d, range_min %d \r\n", 
                            light_ctl_temperature_set_range_buffer.range_max, 
                            light_ctl_temperature_set_range_buffer.range_min);
        }
        break;
    case LIGHT_CTL_SERVER_SET_DEFAULT:
        {
            light_ctl_server_set_default_t *p_get_data = NULL;
            p_get_data = (light_ctl_server_set_default_t *)pargs;
            if (p_get_data) {
                light_ctl_default.delta_uv = p_get_data->delta_uv;
                light_ctl_default.lightness = p_get_data->lightness;
                light_ctl_default.temperature = p_get_data->temperature;
            }
            data_uart_debug("lighting ctl setup server receive: set default delta_uv %d, lightness %d, temperature %d \r\n", 
                            light_ctl_default.delta_uv, 
                            light_ctl_default.lightness, 
                            light_ctl_default.temperature);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_hsl_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_HSL_SERVER_GET:
        {
            light_hsl_server_get_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->hue = cur_light_hsl.hue;
                p_get_data->lightness = cur_light_hsl.lightness;
                p_get_data->saturation = cur_light_hsl.saturation;
            }
            data_uart_debug("lighting hsl server receive: hue %d, lightness %d, saturation %d \r\n", 
                            cur_light_hsl.hue, 
                            cur_light_hsl.lightness, 
                            cur_light_hsl.saturation);
        }
        break;
    case LIGHT_HSL_SERVER_GET_DEFAULT:
        {
            light_hsl_server_get_default_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_get_default_t *)pargs;
            if (p_get_data) {
                p_get_data->hue = default_light_hsl.hue;
                p_get_data->lightness = default_light_hsl.lightness;
                p_get_data->saturation = default_light_hsl.saturation;
            }
            data_uart_debug("lighting hsl server receive: default hue %d, lightness %d, saturation %d \r\n", 
                            default_light_hsl.hue, 
                            default_light_hsl.lightness, 
                            default_light_hsl.saturation);
        }
        break;
    case LIGHT_HSL_SERVER_GET_RANGE:
        {
            light_hsl_server_get_range_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_get_range_t *)pargs;
            if (p_get_data) {
                p_get_data->hue_range_min = light_hsl_range.hue_range_min;
                p_get_data->hue_range_max = light_hsl_range.hue_range_max;
                p_get_data->saturation_range_min = light_hsl_range.saturation_range_min;
                p_get_data->saturation_range_max = light_hsl_range.saturation_range_max;
            }
            data_uart_debug("lighting hsl server receive: hue_range_min %d, hue_range_max %d, saturation_range_min %d, saturation_range_max %d \r\n", 
                            light_hsl_range.hue_range_min, 
                            light_hsl_range.hue_range_max, 
                            light_hsl_range.saturation_range_min, 
                            light_hsl_range.saturation_range_max);
        }
        break;
    case LIGHT_HSL_SERVER_GET_DEFAULT_TRANSITION_TIME:
        {
        }
        break;
    case LIGHT_HSL_SERVER_SET:
        {
            light_hsl_server_set_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_set_t *)pargs;
            if (p_get_data) {
                light_hsl_trans_set_buffer.hue = p_get_data->hue;
                light_hsl_trans_set_buffer.lightness = p_get_data->lightness;
                light_hsl_trans_set_buffer.remaining_time = p_get_data->remaining_time;
                light_hsl_trans_set_buffer.saturation = p_get_data->saturation;
                light_hsl_trans_set_buffer.total_time = p_get_data->total_time;
            }
            data_uart_debug("lighting hsl server receive: set hue %d, lightness %d, remaining_time %d, saturation %d, total_time %d \r\n", 
                            light_hsl_trans_set_buffer.hue, 
                            light_hsl_trans_set_buffer.lightness, 
                            light_hsl_trans_set_buffer.remaining_time, 
                            light_hsl_trans_set_buffer.saturation, 
                            light_hsl_trans_set_buffer.total_time);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_hsl_hue_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_HSL_SERVER_GET_HUE:
        {
            light_hsl_server_get_hue_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_get_hue_t *)pargs;
            if (p_get_data) {
                p_get_data->hue = cur_light_hsl_hue.hue;
            }
            data_uart_debug("lighting hsl hue server receive: hue %d \r\n", cur_light_hsl_hue.hue);
        }
        break;
    case LIGHT_HSL_SERVER_SET_HUE:
        {
            light_hsl_server_set_hue_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_set_hue_t *)pargs;
            if (p_get_data) {
                light_hsl_hue_set_buffer.hue = p_get_data->hue;
                light_hsl_hue_set_buffer.remaining_time = p_get_data->remaining_time;
                light_hsl_hue_set_buffer.total_time = p_get_data->total_time;
            }
            data_uart_debug("lighting hsl hue server receive: set hue %d, remaining_time %d, total_time %d \r\n", 
                            light_hsl_hue_set_buffer.hue, 
                            light_hsl_hue_set_buffer.remaining_time, 
                            light_hsl_hue_set_buffer.total_time);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_hsl_saturation_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_HSL_SERVER_GET_SATURATION:
        {
            light_hsl_server_get_saturation_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_get_saturation_t *)pargs;
            if (p_get_data) {
                p_get_data->saturation = cur_light_hsl_saturation.saturation;
            }
            data_uart_debug("lighting hsl saturation server receive: saturation %d \r\n", cur_light_hsl_saturation.saturation);
        }
        break;
    case LIGHT_HSL_SERVER_SET_SATURATION:
        {
            light_hsl_server_set_saturation_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_set_saturation_t *)pargs;
            if (p_get_data) {
                light_hsl_trans_set_saturation_buffer.saturation = p_get_data->saturation;
                light_hsl_trans_set_saturation_buffer.remaining_time = p_get_data->remaining_time;
                light_hsl_trans_set_saturation_buffer.total_time = p_get_data->total_time;
            }
            data_uart_debug("lighting hsl saturation server receive: set saturation %d, remaining_time %d, total_time %d \r\n", 
                            light_hsl_trans_set_saturation_buffer.saturation, 
                            light_hsl_trans_set_saturation_buffer.remaining_time, 
                            light_hsl_trans_set_saturation_buffer.total_time);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_hsl_setup_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_HSL_SERVER_SET_DEFAULT:
        {
            light_hsl_server_set_default_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_set_default_t *)pargs;
            if (p_get_data) {
                light_hsl_default.lightness = p_get_data->lightness;
                light_hsl_default.hue = p_get_data->hue;
                light_hsl_default.saturation = p_get_data->saturation;
            }
            data_uart_debug("lighting hsl setup server receive: set default lightness %d, hue %d, saturation %d \r\n", 
                            light_hsl_default.lightness, 
                            light_hsl_default.hue, 
                            light_hsl_default.saturation);
        }
        break;
    case LIGHT_HSL_SERVER_SET_RANGE:
        {
            light_hsl_server_set_range_t *p_get_data = NULL;
            p_get_data = (light_hsl_server_set_range_t *)pargs;
            if (p_get_data) {
                light_hsl_set_range_buffer.hue_range_min = p_get_data->hue_range_min;
                light_hsl_set_range_buffer.hue_range_max = p_get_data->hue_range_max;
                light_hsl_set_range_buffer.saturation_range_min = p_get_data->saturation_range_min;
                light_hsl_set_range_buffer.saturation_range_max = p_get_data->saturation_range_max;
            }
            data_uart_debug("lighting hsl setup server receive: set hue_range_min %d, hue_range_max %d, saturation_range_min %d, saturation_range_max %d \r\n", 
                            light_hsl_set_range_buffer.hue_range_min, 
                            light_hsl_set_range_buffer.hue_range_max, 
                            light_hsl_set_range_buffer.saturation_range_min, 
                            light_hsl_set_range_buffer.saturation_range_max);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_xyl_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_XYL_SERVER_GET:
         {
            light_xyl_server_get_t *p_get_data = NULL;
            p_get_data = (light_xyl_server_get_t *)pargs;
            if (p_get_data) {
                p_get_data->xyl_lightness = cur_light_xyl.xyl_lightness;
                p_get_data->xyl_x = cur_light_xyl.xyl_x;
				p_get_data->xyl_y = cur_light_xyl.xyl_y;
            }
            data_uart_debug("lighting xyl server receive: xyl_lightness %d, xyl_x %d, xyl_y %d \r\n", 
                            cur_light_xyl.xyl_lightness, 
                            cur_light_xyl.xyl_x,
                            cur_light_xyl.xyl_y);
        }
        break;
    case LIGHT_XYL_SERVER_GET_DEFAULT:
         {
            light_xyl_server_get_default_t *p_get_data = NULL;
            p_get_data = (light_xyl_server_get_default_t *)pargs;
            if (p_get_data) {
                p_get_data->xyl_lightness = default_light_xyl.xyl_lightness;
                p_get_data->xyl_x = default_light_xyl.xyl_x;
                p_get_data->xyl_y = default_light_xyl.xyl_y;
            }
            data_uart_debug("lighting xyl server receive: default xyl_lightness %d, xyl_x %d, xyl_y %d \r\n", 
                            default_light_xyl.xyl_lightness, 
                            default_light_xyl.xyl_x, 
                            default_light_xyl.xyl_y);
        }
        break;
    case LIGHT_XYL_SERVER_GET_RANGE:
        {
            light_xyl_server_get_range_t *p_get_data = NULL;
            p_get_data = (light_xyl_server_get_range_t *)pargs;
            if (p_get_data) {
                p_get_data->xyl_x_range_max = light_xyl_range.xyl_x_range_max;
                p_get_data->xyl_x_range_min = light_xyl_range.xyl_x_range_min;
				p_get_data->xyl_y_range_max = light_xyl_range.xyl_y_range_max;
				p_get_data->xyl_y_range_min = light_xyl_range.xyl_y_range_min;
            }
            data_uart_debug("lighting xyl server receive: xyl_x_range_max %d, xyl_x_range_min %d, xyl_y_range_max %d, xyl_y_range_min %d \r\n", 
                            light_xyl_range.xyl_x_range_max, 
                            light_xyl_range.xyl_x_range_min,
                            light_xyl_range.xyl_y_range_max,
                            light_xyl_range.xyl_y_range_min);
        }
        break;
    case LIGHT_XYL_SERVER_GET_DEFAULT_TRANSITION_TIME:
        {
        }
        break;
    case LIGHT_XYL_SERVER_SET:
        {
            light_xyl_server_set_t light_xyl_set_buffer;
            light_xyl_server_set_t *p_get_data = NULL;
            p_get_data = (light_xyl_server_set_t *)pargs;
            if (p_get_data) {
                light_xyl_set_buffer.xyl = p_get_data->xyl;
                light_xyl_set_buffer.total_time = p_get_data->total_time;
                light_xyl_set_buffer.remaining_time = p_get_data->remaining_time;
            }
            data_uart_debug("lighting xyl server receive: set xyl %d, total_time %d, remaining_time %d \r\n", 
                            light_xyl_set_buffer.xyl, 
                            light_xyl_set_buffer.total_time, 
                            light_xyl_set_buffer.remaining_time);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int32_t lighting_xyl_setup_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    /* avoid gcc compile warning */
    (void)pmodel_info;
    
    switch (type)
    {
    case LIGHT_XYL_SERVER_SET_DEFAULT:
         {
            light_xyl_server_set_default_t *p_get_data = NULL;
            p_get_data = (light_xyl_server_set_default_t *)pargs;
            if (p_get_data) {
                p_get_data->xyl_lightness = light_xyl_default.xyl_lightness;
                p_get_data->xyl_x = light_xyl_default.xyl_x;
                p_get_data->xyl_y = light_xyl_default.xyl_y;
            }
            data_uart_debug("lighting xyl server receive: default xyl_lightness %d, xyl_x %d, xyl_y %d \r\n", 
                            light_xyl_default.xyl_lightness, 
                            light_xyl_default.xyl_x, 
                            light_xyl_default.xyl_y);
        }
        break;
    case LIGHT_XYL_SERVER_SET_RANGE:
        {
            light_xyl_server_set_range_t *p_get_data = NULL;
            p_get_data = (light_xyl_server_set_range_t *)pargs;
            if (p_get_data) {
                light_xyl_set_range_buffer.xyl_x_range_max = p_get_data->xyl_x_range_max;
                light_xyl_set_range_buffer.xyl_x_range_min = p_get_data->xyl_x_range_min;
                light_xyl_set_range_buffer.xyl_y_range_max = p_get_data->xyl_y_range_max;
                light_xyl_set_range_buffer.xyl_y_range_min = p_get_data->xyl_y_range_min;
            }
            data_uart_debug("lighting xyl setup server receive: set xyl_x_range_max %d, xyl_x_range_min %d, xyl_y_range_max %d, xyl_y_range_min %d \r\n", 
                            light_xyl_set_range_buffer.xyl_x_range_max, 
                            light_xyl_set_range_buffer.xyl_x_range_min, 
                            light_xyl_set_range_buffer.xyl_y_range_max, 
                            light_xyl_set_range_buffer.xyl_y_range_min);
        }
        break;
    default:
        break;
    }

    return 0;
}

void light_server_models_init(void)
{
    lighting_lightness_server_model.model_data_cb = lighting_lightness_server_data;
    light_lightness_server_reg(0, &lighting_lightness_server_model);

    lighting_lightness_setup_server_model.model_data_cb = lighting_lightness_setup_server_data;
    light_lightness_setup_server_reg(0, &lighting_lightness_setup_server_model);

    lighting_ctl_server_model.model_data_cb = lighting_ctl_server_data;
    light_ctl_server_reg(0, &lighting_ctl_server_model);

    lighting_ctl_temperature_server_model.model_data_cb = lighting_ctl_temperature_server_data;
    light_ctl_temperature_server_reg(0, &lighting_ctl_temperature_server_model);

    lighting_ctl_setup_server_model.model_data_cb = lighting_ctl_setup_server_data;
    light_ctl_setup_server_reg(0, &lighting_ctl_setup_server_model);

    lighting_hsl_server_model.model_data_cb = lighting_hsl_server_data;
    light_hsl_server_reg(0, &lighting_hsl_server_model);

    lighting_hsl_hue_server_model.model_data_cb = lighting_hsl_hue_server_data;
    light_hsl_hue_server_reg(0, &lighting_hsl_hue_server_model);

    lighting_hsl_saturation_server_model.model_data_cb = lighting_hsl_saturation_server_data;
    light_hsl_saturation_server_reg(0, &lighting_hsl_saturation_server_model);

    lighting_hsl_setup_server_model.model_data_cb = lighting_hsl_setup_server_data;
    light_hsl_setup_server_reg(0, &lighting_hsl_setup_server_model);

	 lighting_xyl_server_model.model_data_cb = lighting_xyl_server_data;
    light_xyl_server_reg(0, &lighting_xyl_server_model);

	lighting_xyl_setup_server_model.model_data_cb = lighting_xyl_setup_server_data;
    light_xyl_setup_server_reg(0, &lighting_xyl_setup_server_model);
}
