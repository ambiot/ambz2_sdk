
#include <string.h>
#include "os_timer.h"
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"

#include "user_cmd.h"
#include "user_cmd_parse.h"
#include "gap_adv.h"


#include <os_mem.h>
#include <ble_auto_test_case.h>

#include <tc_common.h>
#include <tc_1200_sut.h>
#include <link_mgr.h>

#if F_BT_LE_5_0_AE_ADV_SUPPORT
#include <gap_ext_adv.h>
#endif

#define GATT_UUID_SIMPLE_PROFILE 0xA00A

#if F_BT_LE_5_0_AE_ADV_SUPPORT

static const uint8_t ext_large_adv_data[] =
{
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4
};

static const uint8_t ext_large_scan_data[] =
{
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
#endif


#if TC_1204_SUT_SUPPORT

void tc_1204_sut_start(uint32_t count)
{
    uint8_t update_flags = EXT_ADV_SET_ADV_PARAS;
    uint8_t adv_handle = 0;
    uint16_t adv_event_prop = GAP_EXT_ADV_EVT_PROP_CONNECTABLE_ADV | GAP_EXT_ADV_EVT_PROP_SCANNABLE_ADV
                              | GAP_EXT_ADV_EVT_PROP_USE_LEGACY_ADV;
    uint32_t primary_adv_interval_min = 320;
    uint32_t primary_adv_interval_max = 320;
    uint8_t  primary_adv_channel_map = GAP_ADVCHAN_ALL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t *p_peer_address = NULL;
    uint8_t filter_policy = 0;
    uint8_t tx_power = 127;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    uint8_t secondary_adv_max_skip = 0x00;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;
    uint8_t adv_sid = 0;
    bool scan_req_notification_enable = false;

    primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    secondary_adv_phy = GAP_PHYS_1M;

    adv_handle = le_ext_adv_add_adv_param(adv_event_prop,
                                          primary_adv_interval_min,
                                          primary_adv_interval_max,
                                          primary_adv_channel_map,
                                          own_address_type,
                                          peer_address_type,
                                          p_peer_address,
                                          (T_GAP_ADV_FILTER_POLICY)filter_policy,
                                          tx_power,
                                          primary_adv_phy,
                                          secondary_adv_max_skip,
                                          secondary_adv_phy,
                                          adv_sid,
                                          scan_req_notification_enable);

    le_ext_adv_set_adv_data(adv_handle, sizeof(ext_large_adv_data), (uint8_t *)ext_large_adv_data);

    le_ext_adv_set_scan_response_data(adv_handle, sizeof(ext_large_scan_data),
                                      (uint8_t *)ext_large_scan_data);

    le_ext_adv_start_setting(adv_handle, update_flags);

}

void tc_1204_sut_link_connected(uint8_t conn_id)
{

}

void tc_1204_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1204_MULTI_LINK_4_MASTER, reason);
    tc_update_disc_reason(reason);
    data_uart_print(" TC 1204 Test fail\r\n");
    {
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_1204_MULTI_LINK_4_MASTER, 0, NULL);
        }
    }

}

void tc_1204_sut_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1204_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif



#if TC_1205_SUT_SUPPORT
void tc_1205_sut_start(uint32_t count)
{
    uint8_t update_flags = EXT_ADV_SET_ADV_PARAS;
    uint8_t adv_handle = 0;
    uint16_t adv_event_prop = GAP_EXT_ADV_EVT_PROP_CONNECTABLE_ADV | GAP_EXT_ADV_EVT_PROP_SCANNABLE_ADV
                              | GAP_EXT_ADV_EVT_PROP_USE_LEGACY_ADV;
    uint32_t primary_adv_interval_min = 320;
    uint32_t primary_adv_interval_max = 320;
    uint8_t  primary_adv_channel_map = GAP_ADVCHAN_ALL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t *p_peer_address = NULL;
    uint8_t filter_policy = 0;
    uint8_t tx_power = 127;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    uint8_t secondary_adv_max_skip = 0x00;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;
    uint8_t adv_sid = 0;
    bool scan_req_notification_enable = false;

    primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    secondary_adv_phy = GAP_PHYS_2M;

    adv_handle = le_ext_adv_add_adv_param(adv_event_prop,
                                          primary_adv_interval_min,
                                          primary_adv_interval_max,
                                          primary_adv_channel_map,
                                          own_address_type,
                                          peer_address_type,
                                          p_peer_address,
                                          (T_GAP_ADV_FILTER_POLICY)filter_policy,
                                          tx_power,
                                          primary_adv_phy,
                                          secondary_adv_max_skip,
                                          secondary_adv_phy,
                                          adv_sid,
                                          scan_req_notification_enable);

    le_ext_adv_set_adv_data(adv_handle, sizeof(ext_large_adv_data), (uint8_t *)ext_large_adv_data);

    le_ext_adv_set_scan_response_data(adv_handle, sizeof(ext_large_scan_data),
                                      (uint8_t *)ext_large_scan_data);

    le_ext_adv_start_setting(adv_handle, update_flags);

}

void tc_1205_sut_link_connected(uint8_t conn_id)
{

}

void tc_1205_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1204_MULTI_LINK_4_MASTER, reason);
    tc_update_disc_reason(reason);
    data_uart_print(" TC 1204 Test fail\r\n");
    {
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_1204_MULTI_LINK_4_MASTER, 0, NULL);
        }
    }

}

void tc_1205_sut_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1204_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif



#if TC_1206_SUT_SUPPORT

void tc_1206_sut_start(uint32_t count)
{
    uint8_t update_flags = EXT_ADV_SET_ADV_PARAS;
    uint8_t adv_handle = 0;
    uint16_t adv_event_prop = GAP_EXT_ADV_EVT_PROP_CONNECTABLE_ADV | GAP_EXT_ADV_EVT_PROP_SCANNABLE_ADV
                              | GAP_EXT_ADV_EVT_PROP_USE_LEGACY_ADV;
    uint32_t primary_adv_interval_min = 320;
    uint32_t primary_adv_interval_max = 320;
    uint8_t  primary_adv_channel_map = GAP_ADVCHAN_ALL;
    T_GAP_LOCAL_ADDR_TYPE own_address_type = GAP_LOCAL_ADDR_LE_PUBLIC;
    T_GAP_REMOTE_ADDR_TYPE peer_address_type = GAP_REMOTE_ADDR_LE_PUBLIC;
    uint8_t *p_peer_address = NULL;
    uint8_t filter_policy = 0;
    uint8_t tx_power = 127;
    T_GAP_PHYS_PRIM_ADV_TYPE primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    uint8_t secondary_adv_max_skip = 0x00;
    T_GAP_PHYS_TYPE secondary_adv_phy = GAP_PHYS_2M;
    uint8_t adv_sid = 0;
    bool scan_req_notification_enable = false;

    primary_adv_phy = GAP_PHYS_PRIM_ADV_1M;
    secondary_adv_phy = GAP_PHYS_CODED;

    adv_handle = le_ext_adv_add_adv_param(adv_event_prop,
                                          primary_adv_interval_min,
                                          primary_adv_interval_max,
                                          primary_adv_channel_map,
                                          own_address_type,
                                          peer_address_type,
                                          p_peer_address,
                                          (T_GAP_ADV_FILTER_POLICY)filter_policy,
                                          tx_power,
                                          primary_adv_phy,
                                          secondary_adv_max_skip,
                                          secondary_adv_phy,
                                          adv_sid,
                                          scan_req_notification_enable);

    le_ext_adv_set_adv_data(adv_handle, sizeof(ext_large_adv_data), (uint8_t *)ext_large_adv_data);

    le_ext_adv_set_scan_response_data(adv_handle, sizeof(ext_large_scan_data),
                                      (uint8_t *)ext_large_scan_data);

    le_ext_adv_start_setting(adv_handle, update_flags);

}

void tc_1206_sut_link_connected(uint8_t conn_id)
{

}

void tc_1206_sut_link_disconnected(uint8_t conn_id, uint16_t reason)
{
    tc_check_remote_disc_reason(TC_1204_MULTI_LINK_4_MASTER, reason);
    tc_update_disc_reason(reason);
    data_uart_print(" TC 1204 Test fail\r\n");
    {
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_1204_MULTI_LINK_4_MASTER, 0, NULL);
        }
    }

}

void tc_1206_sut_add_case(uint32_t max_count)
{
    T_TC_1200_IN_PARAM_DATA *p_tc_1200_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_1200_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_1200_IN_PARAM_DATA));

    p_tc_1200_param_data->id = TC_1204_MULTI_LINK_4_MASTER;
    p_tc_1200_param_data->total_test_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_1200_param_data;

    os_queue_in(&tc_q, p_tc_param);
}
#endif



