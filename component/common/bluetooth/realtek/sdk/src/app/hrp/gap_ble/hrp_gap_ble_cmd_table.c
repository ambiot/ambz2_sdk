#include <string.h>
#include <hrp.h>
#include <hrp_gap_ble_cmd_table.h>
#include <hrp_gap_ble_api.h>
#include <hrp_gap_ble_msg.h>

void (*(hrp_gap_ble_handle_adv_req[]))(uint16_t len, uint8_t *p_param_list) =
{
    hrp_gap_ble_handle_adv_0000_perf_config_req,
    hrp_gap_ble_handle_adv_0000_perf_config_rsp,
    hrp_gap_ble_handle_adv_0000_perf_start_req,
    hrp_gap_ble_handle_adv_0000_perf_start_rsp,
    hrp_gap_ble_handle_adv_0000_perf_cmpl_info,
};



void hrp_gap_ble_handle_req(uint8_t cmd_group, uint8_t cmd_index, uint16_t param_list_len,
                            uint8_t *p_param_list)
{

    switch (cmd_group)
    {
    case HRP_GAP_BLE_CMD_GROUP_ADV:
        hrp_gap_ble_handle_adv_req[cmd_index](param_list_len, p_param_list);
        break;

    case HRP_GAP_BLE_CMD_GROUP_SCAN:
        break;

    case HRP_GAP_BLE_CMD_GROUP_CONN:
        break;

    case HRP_GAP_BLE_CMD_GROUP_PAIR:
        break;

    default:
        break;
    }
}

