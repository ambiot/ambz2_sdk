/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     datatrans_client_app.c
  * @brief    Source file for data transmission client model.
  * @details  Data types and external functions declaration.
  * @author   hector_huang
  * @date     2018-10-30
  * @version  v1.0
  * *************************************************************************************
  */
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
#include "bt_mesh_user_api.h"
#endif
#include "datatrans_app.h"
#include "datatrans_model.h"
#if ((defined CONFIG_BT_MESH_PROVISIONER && CONFIG_BT_MESH_PROVISIONER) || \
    (defined CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE))
#include "bt_mesh_provisioner_api.h"
#endif
#if ((defined CONFIG_BT_MESH_DEVICE && CONFIG_BT_MESH_DEVICE) || \
    (defined CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE))
#include "bt_mesh_device_api.h"
#endif

#if defined(CONFIG_BT_MESH_TEST) && CONFIG_BT_MESH_TEST
#include "bt_mesh_device_test.h"
#include "bt_mesh_receive_response.h"
#endif

mesh_model_info_t datatrans;
static uint8_t sample_data[16];

static int32_t datatrans_data(const mesh_model_info_p pmodel_info,
                                     uint32_t type, void *pargs)
{
    UNUSED(pmodel_info);
    switch (type)
    {
    case DATATRANS_SERVER_WRITE:
        {
            datatrans_server_write_t *pdata = pargs;
            data_uart_debug("remote write %d bytes: ", pdata->data_len);
            data_uart_dump(pdata->data, pdata->data_len);
        }
        break;
    case DATATRANS_SERVER_READ:
        {
            datatrans_server_read_t *pdata = pargs;
            if (pdata->data_len > 16)
            {
                pdata->data_len = 16;
            }

            for (uint8_t i = 0; i < pdata->data_len; ++i)
            {
                sample_data[i] = i;
            }
            pdata->data = sample_data;
        }
        break;
    case DATATRANS_CLIENT_STATUS:
        {
            datatrans_client_status_t *pdata = pargs;
            data_uart_debug("written %d bytes, status: %d\r\n", pdata->written_len, pdata->status);
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
            if (!pdata->status) {
                if (bt_mesh_indication(GEN_MESH_CODE(_datatrans_write), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_datatrans_write));  
                }
            } else {
                if (bt_mesh_indication(GEN_MESH_CODE(_datatrans_write), BT_MESH_USER_CMD_FAIL, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_datatrans_write));  
                }
            }
#endif
        }
        break;
    case DATATRANS_CLIENT_DATA:
        {
            datatrans_client_data_t *pdata = pargs;
            data_uart_debug("read %d data from remote: ", pdata->data_len);
            data_uart_dump(pdata->data, pdata->data_len);
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
            if (pdata->data_len) {
                if (bt_mesh_indication(GEN_MESH_CODE(_datatrans_read), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_datatrans_read));  
                }
            } else {
                if (bt_mesh_indication(GEN_MESH_CODE(_datatrans_read), BT_MESH_USER_CMD_FAIL, NULL) != USER_API_RESULT_OK) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_datatrans_read));  
                }
            }
#endif
        }
        break;
    default:
        break;
    }

    return 0;
}

void datatrans_model_init(void)
{
#if defined(CONFIG_BT_MESH_TEST) && CONFIG_BT_MESH_TEST
	/* register data transmission model */
#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
     defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
    datatrans.model_data_cb = common_receive_response;
#endif

#if (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
     defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
    datatrans.model_data_cb = cmd_data_receive;
#endif
#else
	/* register data transmission model */
    datatrans.model_data_cb = datatrans_data;
#endif
    datatrans_reg(0, &datatrans);
}
