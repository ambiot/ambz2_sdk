#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include "bt_datatrans_app_flags.h"
#if CENTRAL_MODE
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "bt_datatrans_central_application.h"
#include "bt_datatrans_multilink_manager.h"
#include "bt_datatrans_client.h"
#include "bt_datatrans_module_param_config.h"
#include "bt_datatrans_at_hci_cmd_process.h"
#include <platform/platform_stdlib.h>
extern T_CLIENT_ID   bt_datatrans_client_id;

/**
  * @brief  Callback will be called when data sent from specific client module.
  * @param  client_id: the ID distinguish which module sent the data.
  * @param  conn_id: connection ID.
  * @param  pData: pointer to data.
  * @retval T_APP_RESULT
  */
extern uint8_t sendbuffer[255];

T_APP_RESULT bt_datatrans_app_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("bt_datatrans_app_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
	//printf("bt_datatrans_app_client_callback: client_id %d, conn_id %d\n",
    //                client_id, conn_id); //debug

    if (client_id == CLIENT_PROFILE_GENERAL_ID)
    {
        T_CLIENT_APP_CB_DATA *p_client_app_cb_data = (T_CLIENT_APP_CB_DATA *)p_data;
        switch (p_client_app_cb_data->cb_type)
        {
        case CLIENT_APP_CB_TYPE_DISC_STATE:
            if (p_client_app_cb_data->cb_content.disc_state_data.disc_state == DISC_STATE_SRV_DONE)
            {
                APP_PRINT_INFO0("Discovery All Service Procedure Done.");
                bt_datatrans_client_start_discovery(conn_id);
            }
            else
            {
                APP_PRINT_INFO0("Discovery state send to application directly.");
            }
            break;
        case CLIENT_APP_CB_TYPE_DISC_RESULT:
            if (p_client_app_cb_data->cb_content.disc_result_data.result_type == DISC_RESULT_ALL_SRV_UUID16)
            {
                APP_PRINT_INFO3("Discovery All Primary Service: UUID16 0x%x, start handle 0x%x, end handle 0x%x.",
                                p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->uuid16,
                                p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->att_handle,
                                p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->end_group_handle);
                if (dataTransInfo.uuid_info.length == 2)
                {
                    uint16_t service_uuid = (dataTransInfo.uuid_info.uuid[0] << 8) | dataTransInfo.uuid_info.uuid[1];
                    if (p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid16_disc_data->uuid16 ==
                        service_uuid)
                    {
                        DataTransLinkTable[conn_id].client_id = bt_datatrans_client_id;
                    }
                }
            }
            else if (p_client_app_cb_data->cb_content.disc_result_data.result_type ==
                     DISC_RESULT_ALL_SRV_UUID128)
            {
                if (dataTransInfo.uuid_info.length == 16)
                {
                    if (!memcmp(dataTransInfo.uuid_info.uuid,
                                p_client_app_cb_data->cb_content.disc_result_data.result_data.p_srv_uuid128_disc_data->uuid128, 16))
                    {
                        DataTransLinkTable[conn_id].client_id = bt_datatrans_client_id;
                    }
                }
            }
            else
            {
                APP_PRINT_INFO0("Discovery result send to application directly.");
            }
            break;
        default:
            break;
        }
    }
    else if (client_id == bt_datatrans_client_id)
    {
        T_DTS_CLIENT_CB_DATA *p_dts_client_cb_data = (T_DTS_CLIENT_CB_DATA *)p_data;
        uint16_t value_size;
        uint8_t *p_value;
        switch (p_dts_client_cb_data->cb_type)
        {
        case DTS_CLIENT_CB_TYPE_DISC_STATE:
            switch (p_dts_client_cb_data->cb_content.disc_state)
            {
            case DISC_DTS_DONE:
                APP_PRINT_INFO0("bt_datatrans_app_client_callback: discover dts procedure done.");
                dts_client_set_data_notify(conn_id, true); //after discover all, client send Write Request(set data_notify cccd)
                break;
            case DISC_DTS_FAILED:
                /* Discovery Request failed. */
                APP_PRINT_INFO0("bt_datatrans_app_client_callback: discover dts request failed.");
                break;
            default:
                break;
            }
            break;
        case DTS_CLIENT_CB_TYPE_READ_RESULT:
            switch (p_dts_client_cb_data->cb_content.read_result.type)
            {
            case DTS_READ_DATA_NOTIFY_CCCD:
                if (p_dts_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("DTS_READ_DATA_NOTIFY_CCCD: notify %d",
                                    p_dts_client_cb_data->cb_content.read_result.data.data_notify_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("DTS_READ_DATA_NOTIFY_CCCD: failed cause 0x%x",
                                     p_dts_client_cb_data->cb_content.read_result.cause);
                };
                break;
            case DTS_READ_FLOW:
                if (p_dts_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    value_size = p_dts_client_cb_data->cb_content.read_result.data.flow_read.value_size;
                    p_value = p_dts_client_cb_data->cb_content.read_result.data.flow_read.p_value;
                    APP_PRINT_INFO2("DTS_READ_FLOW: value_size %d, value %b",
                                    value_size, TRACE_BINARY(value_size, p_value));
                }
                else
                {
                    APP_PRINT_ERROR1("DTS_READ_FLOW: failed cause 0x%x",
                                     p_dts_client_cb_data->cb_content.read_result.cause);
                }
                break;
            case DTS_READ_FLOW_NOTIFY_CCCD:
                if (p_dts_client_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
                {
                    APP_PRINT_INFO1("DTS_READ_FLOW_NOTIFY_CCCD: notify %d",
                                    p_dts_client_cb_data->cb_content.read_result.data.flow_notify_cccd);
                }
                else
                {
                    APP_PRINT_ERROR1("DTS_READ_FLOW_NOTIFY_CCCD: failed cause 0x%x",
                                     p_dts_client_cb_data->cb_content.read_result.cause);
                };
                break;
            default:
                break;
            }
            break;
        case DTS_CLIENT_CB_TYPE_WRITE_RESULT:
            switch (p_dts_client_cb_data->cb_content.write_result.type)
            {
            case DTS_WRITE_DATA:
                APP_PRINT_INFO1("DTS_WRITE_DATA: write result 0x%x",
                                p_dts_client_cb_data->cb_content.write_result.cause);
				//printf("DTS_WRITE_DATA: write result 0x%x\r\n",
                //                p_dts_client_cb_data->cb_content.write_result.cause);//debug
                if (IO_Receive.datalen >= MTU_SIZE - 3) //buffer length > MTU size - 3, send immediately
                {
                    if (transferConfigInfo.select_io == UART_AT)
                    {
                        Setsendbuffer(MTU_SIZE - 3);
                        kns_client_write_data_char(CON_ID, MTU_SIZE - 3, sendbuffer, GATT_WRITE_TYPE_CMD);
                    }
                    else
                    {
                        kns_client_write_data_char(CON_ID, MTU_SIZE - 3, IO_Receive.buf + IO_Receive.ReadOffset,
                                                   GATT_WRITE_TYPE_CMD);
                        IO_Receive.ReadOffset += MTU_SIZE - 3;
                        IO_Receive.datalen = IO_Receive.datalen - (MTU_SIZE - 3);
                    }

                }
                else if (IO_Receive.datalen > 0)      //buffer length < MTU size - 3
                {
                    if (transferConfigInfo.select_io == UART_AT && transferConfigInfo.uart_idle)
                    {
                        uint16_t len = IO_Receive.datalen;
                        Setsendbuffer(IO_Receive.datalen);
                        kns_client_write_data_char(CON_ID, len, sendbuffer, GATT_WRITE_TYPE_CMD);
                        transferConfigInfo.uart_idle = 0;
                    }
                }
                break;
            case DTS_WRITE_DATA_NOTIFY_CCCD:
                APP_PRINT_INFO1("DTS_WRITE_DATA_NOTIFY_CCCD: write result 0x%x",
                                p_dts_client_cb_data->cb_content.write_result.cause);
                dts_client_set_flow_notify(conn_id, true); //after write data_notify cccd, client send Write Request(set flow_ctrl_notify cccd)
                break;
            case DTS_WRITE_CTRL:
                APP_PRINT_INFO1("DTS_WRITE_CTRL: write result 0x%x",
                                p_dts_client_cb_data->cb_content.write_result.cause);
                break;
            case DTS_WRITE_FLOW_NOTIFY_CCCD: //complete write flow_ctrl_notify cccd
                APP_PRINT_INFO1("DTS_WRITE_FLOW_NOTIFY_CCCD: write result 0x%x",
                                p_dts_client_cb_data->cb_content.write_result.cause);
                break;
            default:
                break;
            }
            break;
        case DTS_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
            switch (p_dts_client_cb_data->cb_content.notif_ind_data.type)
            {
            case DTS_DATA_NOTIFY:
                value_size = p_dts_client_cb_data->cb_content.notif_ind_data.data.value_size;
                p_value = p_dts_client_cb_data->cb_content.notif_ind_data.data.p_value;
                result = HandleBTReceiveData(conn_id, value_size, p_value);
                //APP_PRINT_INFO2("DTS_DATA_NOTIFY(client received data_notify value and then send to uart): value_size %d, value %b",
                //                value_size, TRACE_BINARY(value_size, p_value));
				//printf("DTS_DATA_NOTIFY: value_size %d\r\n", value_size); //debug
                break;
            case DTS_FLOW_NOTIFY:
                value_size = p_dts_client_cb_data->cb_content.notif_ind_data.data.value_size;
                p_value = p_dts_client_cb_data->cb_content.notif_ind_data.data.p_value;
                //add it for new feature
                APP_PRINT_INFO2("DTS_FLOW_NOTIFY: value_size %d, value %b",
                                value_size, TRACE_BINARY(value_size, p_value));
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
    }

    return result;
}
#endif

#endif // end of CONFIG_BT_DATATRANS
