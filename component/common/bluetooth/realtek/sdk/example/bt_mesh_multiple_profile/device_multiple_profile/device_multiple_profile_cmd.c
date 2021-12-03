/**
************************************************************************************************************
*               Copyright(c) 2014, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file     device_cmd.c
* @brief    User defined Mesh test commands.
* @details  User command interfaces.
* @author
* @date     2015-03-19
* @version  v0.1
*************************************************************************************************************
*/
#include <string.h>
#include "trace.h"
#include "gap_wrapper.h"
#include "mesh_cmd.h"
#include "test_cmd.h"
#include "mesh_api.h"
#include "device_cmd.h"
#include "device_app.h"
#include "datatrans_model.h"
#include "datatrans_app.h"
#include "generic_on_off.h"
#include "datatrans_server.h"

extern mesh_model_info_t health_server_model;
extern mesh_model_info_t generic_on_off_server_model;
extern uint8_t lpn_disable_scan_flag;

static user_cmd_parse_result_t user_cmd_node_reset(user_cmd_parse_value_t *pparse_value)
{
    switch (pparse_value->dw_parameter[0])
    {
    case 0:
        mesh_node_reset();
        break;
    case 1:
        mesh_node_clean();
        break;
    case 2:
        mesh_node_restore();
        break;
    default:
        break;
    }
    return USER_CMD_RESULT_OK;
}

user_cmd_parse_result_t user_cmd_prov_capa_set(user_cmd_parse_value_t *pparse_value)
{
    prov_capabilities_t prov_capabilities;
    prov_capabilities.algorithm = PROV_CAP_ALGO_FIPS_P256_ELLIPTIC_CURVE;
    prov_capabilities.public_key = pparse_value->dw_parameter[0] ? PROV_CAP_PUBLIC_KEY_OOB : 0;
    prov_capabilities.static_oob = pparse_value->dw_parameter[1] ? PROV_CAP_STATIC_OOB : 0;
    prov_capabilities.output_oob_size = pparse_value->dw_parameter[2];
    prov_capabilities.output_oob_action = pparse_value->dw_parameter[3];
    prov_capabilities.input_oob_size = pparse_value->dw_parameter[4];
    prov_capabilities.input_oob_action = pparse_value->dw_parameter[5];
    if (prov_capabilities.output_oob_size > OUTPUT_OOB_SIZE_MAX ||
        prov_capabilities.input_oob_size > INPUT_OOB_SIZE_MAX)
    {
        return USER_CMD_RESULT_VALUE_OUT_OF_RANGE;
    }
    else
    {
        prov_params_set(PROV_PARAMS_CAPABILITIES, &prov_capabilities, sizeof(prov_capabilities_t));
        return USER_CMD_RESULT_OK;
    }
}

#if MESH_LPN
static user_cmd_parse_result_t user_cmd_lpn_init(user_cmd_parse_value_t *pparse_value)
{
    uint8_t fn_num = pparse_value->dw_parameter[0];
    return lpn_init(fn_num, lpn_cb) ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
}

static user_cmd_parse_result_t user_cmd_lpn_deinit(user_cmd_parse_value_t *pparse_value)
{
	/* avoid gcc compile warning */
    (void)pparse_value;
	lpn_deinit();
    if (lpn_disable_scan_flag){
        gap_sched_scan(true);
        lpn_disable_scan_flag = 0;
    }
	return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_lpn_req(user_cmd_parse_value_t *pparse_value)
{
    uint8_t fn_index = pparse_value->dw_parameter[0];
    uint16_t net_key_index = pparse_value->dw_parameter[1];
    lpn_req_params_t req_params = {50, 100, {1, 0, 0, 0}};

    if(le_get_active_link_num() != 0)
    {
        data_uart_debug("Have BLE active link, cannot establish friendship\n\r");
        return (USER_CMD_RESULT_ERROR);
    }

    if (pparse_value->dw_parameter[2])
    {
        req_params.poll_interval = pparse_value->dw_parameter[2];
    }
    if (pparse_value->dw_parameter[3])
    {
        req_params.poll_timeout = pparse_value->dw_parameter[3];
    }
    if (pparse_value->dw_parameter[4] >= 0xa && pparse_value->dw_parameter[4] <= 0xff)
    {
        mesh_node.frnd_rx_delay = pparse_value->dw_parameter[4];
    }
    if (pparse_value->para_count >= 6 && pparse_value->dw_parameter[5] <= 0xff)
    {
        mesh_node.frnd_rx_widen = pparse_value->dw_parameter[5];
    }
    return lpn_req(fn_index, net_key_index,
                   &req_params) == LPN_REQ_REASON_SUCCESS ? USER_CMD_RESULT_OK : USER_CMD_RESULT_ERROR;
}

static user_cmd_parse_result_t user_cmd_lpn_sub(user_cmd_parse_value_t *pparse_value)
{
    uint8_t fn_index = pparse_value->dw_parameter[0];
    uint16_t addr = pparse_value->dw_parameter[1];
    bool add_rm = pparse_value->dw_parameter[2];
    frnd_sub_list_add_rm(fn_index, &addr, 1, add_rm);
    return (USER_CMD_RESULT_OK);
}

static user_cmd_parse_result_t user_cmd_lpn_clear(user_cmd_parse_value_t *pparse_value)
{
    uint8_t fn_index = pparse_value->dw_parameter[0];
    lpn_clear(fn_index);
    return (USER_CMD_RESULT_OK);
}
#endif

#if MESH_RMT_PRO_CLIENT
static user_cmd_parse_result_t user_cmd_rmt_pro_scan(user_cmd_parse_value_t *pparse_value)
{
    uint16_t dst = pparse_value->dw_parameter[0];
    uint8_t mode = 0;
    if (pparse_value->para_count > 1)
    {
        mode = pparse_value->dw_parameter[1];
    }
    switch (mode)
    {
    case 0:
        RmtProClient_ScanStart(dst);
        break;
    case 1:
        {
            uint8_t dev_uuid[16] = MESH_DEVICE_UUID;
            RmtProClient_ScanStartWithFilter(dst, dev_uuid);
        }
        break;
    case 2:
        RmtProClient_ScanUnproDeviceNum(dst, 3);
        break;
    default:
        break;
    }

    return (USER_CMD_RESULT_OK);
}
#endif

static user_cmd_parse_result_t user_cmd_data_transmission_notify(user_cmd_parse_value_t
                                                                 *pparse_value)
{
    uint8_t conn_id = pparse_value->dw_parameter[0];
    uint8_t data_len = pparse_value->para_count - 1;
    uint8_t data[20];
    if (data_len > 20)
    {
        data_len = 20;
    }

    for (uint8_t i = 0; i < data_len; ++i)
    {
        data[i] = pparse_value->dw_parameter[i + 1];
    }

    datatrans_server_notify(conn_id, data, data_len);
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_generic_on_off_publish(user_cmd_parse_value_t *pparse_value)
{
    generic_on_off_t on_off = (generic_on_off_t)pparse_value->dw_parameter[0];
	mesh_model_p pmodel = generic_on_off_server_model.pmodel;

	pmodel->pub_params.pub_addr = 0xFEFF;
	pmodel->pub_params.pub_ttl = 0x3F;
	generic_on_off_publish(&generic_on_off_server_model, on_off);
    
    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_datatrans_write(user_cmd_parse_value_t *pparse_value)
{
    uint8_t para_count = pparse_value->para_count;

#if defined(CONFIG_BT_MESH_TEST) && CONFIG_BT_MESH_TEST
		uint8_t data[75];
#else
		uint8_t data[18];
#endif

    for (uint8_t i = 0; i < para_count - 3; ++i)
    {
        data[i] = pparse_value->dw_parameter[i + 1];
    }
    datatrans_write(&datatrans, pparse_value->dw_parameter[0],
                    pparse_value->dw_parameter[para_count - 2], para_count - 3, data,
                    pparse_value->dw_parameter[para_count - 1]);

    return USER_CMD_RESULT_OK;
}

static user_cmd_parse_result_t user_cmd_datatrans_read(user_cmd_parse_value_t *pparse_value)
{
    datatrans_read(&datatrans, pparse_value->dw_parameter[0],
                   pparse_value->dw_parameter[2], pparse_value->dw_parameter[1]);

    return USER_CMD_RESULT_OK;
}

/*----------------------------------------------------
 * command table
 * --------------------------------------------------*/
const user_cmd_table_entry_t device_cmd_table[] =
{
    // mesh common cmd
    MESH_COMMON_CMD,
    TEST_CMD,
    // device cmd
    {
        "nr",
        "nr [mode]\n\r",
        "node reset\n\r",
        user_cmd_node_reset
    },
    // prov
    {
        "pcs",
        "pcs [public key oob] [static oob] [output size] [output action] [input size] [input action]\n\r",
        "provision capability set\n\r",
        user_cmd_prov_capa_set
    },
#if MESH_LPN
    {
        "lpninit",
        "lpninit [fn_num]\n\r",
        "low power node init\n\r",
        user_cmd_lpn_init
    },
    {
        "lpndeinit",
        "lpndeinit\n\r",
        "low power node deinit\n\r",
        user_cmd_lpn_deinit
    },
    {
        "lpnreq",
        "lpnreq [fn_index] [net_key_index] [poll int(100ms)] [poll to(100ms)] [rx delay(ms)] [rx widen(ms)]\n\r",
        "LPN request to estabish a friendship\n\r",
        user_cmd_lpn_req
    },
    {
        "lpnsub",
        "lpnsub [fn_index] [addr] [add/rm]\n\r",
        "LPN subsript list add or rm\n\r",
        user_cmd_lpn_sub
    },
    {
        "lpnclear",
        "lpnclear [fn_index]\n\r",
        "LPN clear\n\r",
        user_cmd_lpn_clear
    },
#endif
#if MESH_RMT_PRO_CLIENT
    {
        "rmtproscan",
        "rmtproscan [dst addr]\n\r",
        "command remote server to scan\n\r",
        user_cmd_rmt_pro_scan
    },
#endif
    {
        "dtn",
        "dtn [conn_id] [value...]\n\r",
        "data transmission notify\n\r",
        user_cmd_data_transmission_notify
    },
    {
        "dtw",
        "dtw [dst] [data...] [app_key_index] [ack] \n\r",
        "data transmission write data\n\r",
        user_cmd_datatrans_write
    },
    {
        "dtr",
        "dtr [dst] [len] [app_key_index]\n\r",
        "data transmission read data\n\r",
        user_cmd_datatrans_read
    },
    /* MUST be at the end: */
    {
        0,
        0,
        0,
        0
    }
};

#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
const struct bt_mesh_api_hdl devicecmds[] = 
{
    GEN_MESH_HANDLER(_node_reset)
    GEN_MESH_HANDLER(_prov_capa_set)
#if MESH_LPN
    GEN_MESH_HANDLER(_lpn_init)
    GEN_MESH_HANDLER(_lpn_deinit)
    GEN_MESH_HANDLER(_lpn_req)
    GEN_MESH_HANDLER(_lpn_sub)
    GEN_MESH_HANDLER(_lpn_clear)
#endif
#if MESH_FN
	GEN_MESH_HANDLER(_fn_init)
	GEN_MESH_HANDLER(_fn_deinit)
#endif
    GEN_MESH_HANDLER(_data_transmission_notify)
    GEN_MESH_HANDLER(_generic_on_off_publish)
    GEN_MESH_HANDLER(_datatrans_write)
    GEN_MESH_HANDLER(_datatrans_read)
    GEN_MESH_HANDLER(_connect)
    GEN_MESH_HANDLER(_disconnect)
    GEN_MESH_HANDLER(_list)
    GEN_MESH_HANDLER(_dev_info_show)
};
#endif

