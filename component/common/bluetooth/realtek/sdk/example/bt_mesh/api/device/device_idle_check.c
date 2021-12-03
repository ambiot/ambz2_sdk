/**
*****************************************************************************************
*     Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     device_idle_check.c
  * @brief    Source file of idel check function specific for scenario together with wifi.
  * @details  User command interfaces.
  * @author   sherman_sun
  * @date     2020_11_06
  * @version  v1.0
  * *************************************************************************************
  */

#include "device_idle_check.h"

#if defined(CONFIG_BT_MESH_IDLE_CHECK) && CONFIG_BT_MESH_IDLE_CHECK
#define ACCESS_DATA_MONITOR_INTERVAL 2000 //2s
#define ACCESS_DATA_MONITOR_THRESHOLD 14 //monitor timeout = (ACCESS_DATA_MONITOR_THRESHOLD+1) * ACCESS_DATA_MONITOR_INTERVAL

extern bool mesh_initial_state;
extern uint8_t mesh_device_conn_state;
uint8_t mesh_access_data_flow_state = 1;
uint8_t mesh_access_data_received = 0;
plt_timer_t bt_mesh_access_data_flow_monitor_timer = NULL;

void bt_mesh_access_data_flow_monitor_timer_handler(void *FunctionContext)
{
    /* avoid gcc compile warning */
    (void)FunctionContext;
    static uint8_t mesh_access_msg_dec_count = 0;

    if (!mesh_access_data_received) {
        if (++mesh_access_msg_dec_count > ACCESS_DATA_MONITOR_THRESHOLD) {
            mesh_access_data_flow_state = 0;
            //prevent mesh_access_msg_dec_count overflow
            mesh_access_msg_dec_count = ACCESS_DATA_MONITOR_THRESHOLD;
        }
        //printf("\r\n Enter bt_mesh_access_data_flow_monitor_timer_handler mesh_access_msg_dec_count = %d \r\n",mesh_access_msg_dec_count);
    } else {
        mesh_access_data_received = 0;
        mesh_access_msg_dec_count = 0;
        mesh_access_data_flow_state = 1;
        //printf("\r\n Enter bt_mesh_access_data_flow_monitor_timer_handler mesh_access_msg_dec_count = %d \r\n",mesh_access_msg_dec_count);
    }

    plt_timer_change_period(bt_mesh_access_data_flow_monitor_timer, ACCESS_DATA_MONITOR_INTERVAL, 0xFFFFFFFF);
}

int32_t bt_mesh_access_process(mesh_msg_p pmesh_msg)
{
    mesh_access_data_received = 1;
    //printf("\r\n Enter bt_mesh_access_process \r\n");
    return 0;
}

uint8_t bt_mesh_idle_check_init(void)
{
    //register mesh access detached function
    access_set_dispatch_preprocess((dispatch_preprocess_t)bt_mesh_access_process);
    bt_mesh_access_data_flow_monitor_timer = plt_timer_create("bt_mesh_access_data_flow_monitor_timer", 0xFFFFFFFF, FALSE, NULL, bt_mesh_access_data_flow_monitor_timer_handler);
    if (!bt_mesh_access_data_flow_monitor_timer) {
        printf("[BT Mesh Device] Create adv timer failed\n\r");
        return 0;
    }
    plt_timer_change_period(bt_mesh_access_data_flow_monitor_timer, 2000, 0xFFFFFFFF);

    return 1;
}

uint8_t bt_mesh_idle_check_deinit(void)
{
    plt_timer_delete(bt_mesh_access_data_flow_monitor_timer, 0xFFFFFFFF);
	bt_mesh_access_data_flow_monitor_timer = NULL;

    return 1;
}

device_idle_check_t bt_mesh_idle_check(void)
{
    device_idle_check_t ret = DEVICE_IDLE_CHECK_FALSE;
    
    if (!mesh_initial_state) {
        //check whether mesh is inited
        printf("[BT_MESH] %s(): Please init mesh firstly \r\n",__func__);
        goto exit;
    }

    if (!mesh_node.node_state) {
        //mesh device havn't been provisioned to a mesh net
        printf("[BT_MESH] %s(): Please do provisioning firstly \r\n",__func__);
        goto exit;
    }

    if (mesh_device_conn_state) {
        //mesh device is connecting to another device through gatt
        printf("[BT_MESH] %s(): Please disconnect gatt connection \r\n",__func__);
        goto exit;
    }

    if (mesh_access_data_flow_state) {
        //mesh access has data flow
        printf("[BT_MESH] %s(): Cannot enter power save cause mesh message interaction \r\n",__func__);
        goto exit;
    }

    ret = DEVICE_IDLE_CHECK_TRUE;

exit:
    return ret;
}
#endif
