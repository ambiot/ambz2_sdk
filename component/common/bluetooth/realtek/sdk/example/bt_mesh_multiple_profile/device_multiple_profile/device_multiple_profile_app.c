/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      device_app.c
* @brief     Smart mesh demo application
* @details
* @author    bill
* @date      2015-11-12
* @version   v0.1
* *********************************************************************************************************
*/

#include <string.h>
#include <app_msg.h>
#include <trace.h>
#include <gap_scan.h>
#include <gap.h>
#include <gap_msg.h>
#include <gap_adv.h>
#include <gap_bond_le.h>
#include <profile_server.h>

#include "device_multiple_profile_app.h"
#include "trace_app.h"
#include "gap_wrapper.h"
#include "mesh_api.h"
#include "mesh_data_uart.h"
#include "mesh_user_cmd_parse.h"
#include "device_cmd.h"
#include "mesh_cmd.h"
#include "ping_app.h"
#include "datatrans_server.h"
#include "bt_flags.h"
#include "vendor_cmd.h"
#include "vendor_cmd_bt.h"

#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
#include "bt_mesh_user_api.h"
#include "bt_mesh_device_api.h"
#endif

#if defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL
#include <bas.h>
#include <simple_ble_service.h>
#include <gap_conn_le.h>
#include "platform_stdlib.h"
#include "ble_peripheral_at_cmd.h"
#include <gap_le_types.h>
#include <simple_ble_service.h>
#include <bas.h>
#endif

#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
#include <ble_central_link_mgr.h>
#include "ble_central_at_cmd.h"
#include <gcs_client.h>
#endif

#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
#include "platform_stdlib.h"
#include <ble_scatternet_link_mgr.h>
#include <ble_scatternet_user_cmd.h>
#include "gatt_builtin_services.h"
#include "ble_central_at_cmd.h"
#include "ble_peripheral_at_cmd.h"
#include <gap_le_types.h>
#include <simple_ble_service.h>
#include <bas.h>
#include <gcs_client.h>
#endif

#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
T_CLIENT_ID   bt_mesh_central_gcs_client_id;         /**< General Common Services client client id*/
#endif
int bt_mesh_multiple_profile_scan_state = 0;
#endif

#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
int bt_mesh_scatternet_peripheral_app_max_links = 0;
int bt_mesh_scatternet_central_app_max_links = 0;
T_CLIENT_ID bt_mesh_scatternet_gcs_client_id;         /**< General Common Services client client id*/
#endif

#if ((!defined(CONFIG_BT_MESH_CENTRAL) || !CONFIG_BT_MESH_CENTRAL) && \
    (!defined(CONFIG_BT_MESH_SCATTERNET) || !CONFIG_BT_MESH_SCATTERNET))
/**
 * @brief  Application Link control block defination.
 */
typedef struct
{
    T_GAP_CONN_STATE        conn_state;          /**< Connection state. */
    T_GAP_REMOTE_ADDR_TYPE  bd_type;             /**< remote BD type*/
    uint8_t                 bd_addr[GAP_BD_ADDR_LEN]; /**< remote BD */
} T_APP_LINK;
#endif

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined (CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
/** @brief  GAP - Advertisement data (max size = 31 bytes, best kept short to conserve power) */
const uint8_t adv_data[] =
{
    /* Flags */
    0x02,             /* length */
    GAP_ADTYPE_FLAGS, /* type="Flags" */
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    /* Service */
    0x03,             /* length */
    GAP_ADTYPE_16BIT_COMPLETE,
    LO_WORD(GATT_UUID_SIMPLE_PROFILE),
    HI_WORD(GATT_UUID_SIMPLE_PROFILE),
    /* Local name */
    0x0F,             /* length */
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'L', 'E', '_', 'P', 'E', 'R', 'I', 'P', 'H', 'E', 'R', 'A', 'L',
};
int array_count_of_adv_data = sizeof(adv_data) / sizeof(adv_data[0]);
T_SERVER_ID bt_mesh_simp_srv_id; /**< Simple ble service id*/
T_SERVER_ID bt_mesh_bas_srv_id;  /**< Battery service id */
#endif

T_GAP_DEV_STATE bt_mesh_device_multiple_profile_gap_dev_state = {0, 0, 0, 0, 0};                 /**< GAP device state */
#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
T_APP_LINK bt_mesh_multiple_profile_app_link_table[BLE_CENTRAL_APP_MAX_LINKS];
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
T_APP_LINK bt_mesh_multiple_profile_app_link_table[BLE_SCATTERNET_APP_MAX_LINKS];
#else
T_APP_LINK bt_mesh_multiple_profile_app_link_table[APP_MAX_LINKS];
#endif

prov_auth_value_type_t prov_auth_value_type;

void bt_mesh_device_multiple_profile_app_handle_gap_msg(T_IO_MSG *p_gap_msg);

static uint8_t datatrans_sample_data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xa, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

uint8_t lpn_disable_scan_flag = 0;

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
extern void mesh_le_adv_start(void);
extern void mesh_le_adv_stop(void);
#endif
/**
 * @brief    All the application messages are pre-handled in this function
 * @note     All the IO MSGs are sent to this function, then the event handling
 *           function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return   void
 */
void bt_mesh_device_multiple_profile_app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;
    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {
            bt_mesh_device_multiple_profile_app_handle_gap_msg(&io_msg);
        }
        break;
    case IO_MSG_TYPE_UART:
        {
            /* We handle user command informations from Data UART in this branch. */
            uint8_t data = io_msg.subtype;
            mesh_user_cmd_collect(&data, sizeof(data), device_cmd_table);
        }
        break;
    case PING_TIMEOUT_MSG:
        {
            ping_handle_timeout();
        }
        break;
    case PING_APP_TIMEOUT_MSG:
        {
            ping_app_handle_timeout();
        }
        break;
    case IO_MSG_TYPE_AT_CMD:
        {
            uint16_t subtype = io_msg.subtype;
            void *arg = io_msg.u.buf;
#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
            if (ble_central_app_handle_at_cmd(subtype, arg) != 1) {
                ble_peripheral_app_handle_at_cmd(subtype, arg);
            }
#elif defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
            ble_central_app_handle_at_cmd(subtype, arg);
#elif defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL
            ble_peripheral_app_handle_at_cmd(subtype, arg);
#endif
        }
        break;
    case IO_MSG_TYPE_QDECODE:
        {
            if (io_msg.subtype == 2) {
                gap_sched_scan(false);
            } else if (io_msg.subtype == 3) {
                gap_sched_scan(true);
            }
        }
        break;
#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined (CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    case IO_MSG_TYPE_ADV:
        {
            uint8_t *padv_data = gap_sched_task_get();

            if (NULL == padv_data)
            {
                printf("[BT Mesh Device] bt_mesh_multiple_profile_peripheral_adv_timer_handler allocate padv_data fail ! \n\r");
                break;
            }

            gap_sched_task_p ptask = CONTAINER_OF(padv_data, gap_sched_task_t, adv_data);

            rtw_memcpy(padv_data, (uint8_t *)adv_data, sizeof(adv_data));

            ptask->adv_len += 23;
            ptask->adv_type = GAP_SCHED_ADV_TYPE_IND;

            gap_sched_try(ptask); 
        }
        break;
#endif
    default:
        break;
    }
}

/**
 * @brief    Handle msg GAP_MSG_LE_DEV_STATE_CHANGE
 * @note     All the gap device state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] new_state  New gap device state
 * @param[in] cause GAP device state change cause
 * @return   void
 */
void bt_mesh_device_multiple_profile_app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO4("bt_mesh_device_multiple_profile_app_handle_dev_state_evt: init state  %d, adv state %d, scan state %d, cause 0x%x",
                    new_state.gap_init_state, new_state.gap_adv_state,
                    new_state.gap_scan_state, cause);
    if (bt_mesh_device_multiple_profile_gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO0("GAP stack ready");
            uint8_t bt_addr[6];
            gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
            data_uart_debug("bt addr: 0x%02x%02x%02x%02x%02x%02x\r\n>",
                            bt_addr[5], bt_addr[4], bt_addr[3],
                            bt_addr[2], bt_addr[1], bt_addr[0]);
#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
            /*stack ready*/
            mesh_le_adv_start();
#endif
        }
    }

#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    if (bt_mesh_device_multiple_profile_gap_dev_state.gap_adv_state != new_state.gap_adv_state)
    {
        if (new_state.gap_adv_state == GAP_ADV_STATE_IDLE)
        {
            if (new_state.gap_adv_sub_state == GAP_ADV_TO_IDLE_CAUSE_CONN)
            {
                APP_PRINT_INFO0("GAP adv stoped: because connection created");
				printf("\n\rGAP adv stoped: because connection created\n\r");
            }
            else
            {
                APP_PRINT_INFO0("GAP adv stoped");
				printf("\n\rGAP adv stopped\n\r");
            }
        }
        else if (new_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING)
        {
            APP_PRINT_INFO0("GAP adv start");
			printf("\n\rGAP adv start\n\r");
        }
    }
#endif

#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    if (bt_mesh_device_multiple_profile_gap_dev_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");
            //data_uart_debug("GAP scan stop\r\n");
        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");
            //data_uart_debug("GAP scan start\r\n");
        }
    }
#endif

    bt_mesh_device_multiple_profile_gap_dev_state = new_state;
}

uint8_t mesh_device_conn_state = 0;

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_STATE_CHANGE
 * @note     All the gap conn state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New gap connection state
 * @param[in] disc_cause Use this cause when new_state is GAP_CONN_STATE_DISCONNECTED
 * @return   void
 */
void bt_mesh_device_multiple_profile_app_handle_conn_state_evt(uint8_t conn_id, T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET    
    T_GAP_CONN_INFO conn_info;
#endif
#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
    if (conn_id >= BLE_CENTRAL_APP_MAX_LINKS)
    {
        return;
    }
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
    if (conn_id >= BLE_SCATTERNET_APP_MAX_LINKS)
    {
        return;
    }
#else
    if (conn_id >= APP_MAX_LINKS)
    {
        return;
    }
#endif

    APP_PRINT_INFO4("bt_mesh_device_multiple_profile_app_handle_conn_state_evt: conn_id %d, conn_state(%d -> %d), disc_cause 0x%x",
                    conn_id, bt_mesh_multiple_profile_app_link_table[conn_id].conn_state, new_state, disc_cause);

    bt_mesh_multiple_profile_app_link_table[conn_id].conn_state = new_state;
    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTED:
        {
            if ((disc_cause != (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                && (disc_cause != (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE)))
                {
                APP_PRINT_ERROR2("bt_mesh_device_multiple_profile_app_handle_conn_state_evt: connection lost, conn_id %d, cause 0x%x", conn_id,
                                 disc_cause);
            }
            data_uart_debug("Disconnect conn_id %d\r\n", conn_id);
            mesh_device_conn_state = 0;
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
            uint8_t ret = USER_API_RESULT_ERROR;
            ret = bt_mesh_indication(GEN_MESH_CODE(_connect), BT_MESH_USER_CMD_FAIL, NULL);
            if (ret != USER_API_RESULT_OK) {
                if (ret != USER_API_RESULT_INCORRECT_CODE) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_connect));
                    goto next;
                }  
            } else {
                goto next;
            }
            ret = bt_mesh_indication(GEN_MESH_CODE(_disconnect), BT_MESH_USER_CMD_FAIL, NULL);
            if (ret != USER_API_RESULT_OK) {
                if (ret != USER_API_RESULT_INCORRECT_CODE) {
                    data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_disconnect));
                    goto next;
                }  
            } else {
                goto next;
            }
#endif
next:
#if defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL
            mesh_le_adv_start();
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
            if (bt_mesh_multiple_profile_app_link_table[conn_id].role == GAP_LINK_ROLE_SLAVE) {
                /*stack ready*/
                mesh_le_adv_start();
            }
#endif 

#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
            if (bt_mesh_multiple_profile_app_link_table[conn_id].role == GAP_LINK_ROLE_MASTER) {
				bt_mesh_scatternet_central_app_max_links --;
			} else if (bt_mesh_multiple_profile_app_link_table[conn_id].role == GAP_LINK_ROLE_SLAVE) {
				bt_mesh_scatternet_peripheral_app_max_links --;
			}
#endif
            memset(&bt_mesh_multiple_profile_app_link_table[conn_id], 0, sizeof(T_APP_LINK));
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        {
            uint16_t conn_interval;
            uint16_t conn_latency;
            uint16_t conn_supervision_timeout;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_latency, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
            le_get_conn_addr(conn_id, bt_mesh_multiple_profile_app_link_table[conn_id].bd_addr,
                             &bt_mesh_multiple_profile_app_link_table[conn_id].bd_type);
            APP_PRINT_INFO5("GAP_CONN_STATE_CONNECTED:remote_bd %s, remote_addr_type %d, conn_interval 0x%x, conn_latency 0x%x, conn_supervision_timeout 0x%x",
                            TRACE_BDADDR(bt_mesh_multiple_profile_app_link_table[conn_id].bd_addr), bt_mesh_multiple_profile_app_link_table[conn_id].bd_type,
                            conn_interval, conn_latency, conn_supervision_timeout);
            data_uart_debug("Connected success conn_id %d\r\n", conn_id);
            mesh_device_conn_state = 1;
#if defined(CONFIG_BT_MESH_USER_API) && CONFIG_BT_MESH_USER_API
            if (bt_mesh_indication(GEN_MESH_CODE(_connect), BT_MESH_USER_CMD_SUCCESS, NULL) != USER_API_RESULT_OK) {
                data_uart_debug("[BT_MESH] %s(): user cmd %d fail !\r\n", __func__, GEN_MESH_CODE(_connect));  
            }
#endif
#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
            //get device role
            if (le_get_conn_info(conn_id, &conn_info)){
				bt_mesh_multiple_profile_app_link_table[conn_id].role = conn_info.role;
				if (bt_mesh_multiple_profile_app_link_table[conn_id].role == GAP_LINK_ROLE_MASTER)
					bt_mesh_scatternet_central_app_max_links ++;
				else if (bt_mesh_multiple_profile_app_link_table[conn_id].role == GAP_LINK_ROLE_SLAVE)
					bt_mesh_scatternet_peripheral_app_max_links ++;
            }

		    ////print bt address type
			uint8_t local_bd_type;
            //uint8_t features[8];
            uint8_t remote_bd_type;
            le_get_conn_param(GAP_PARAM_CONN_LOCAL_BD_TYPE, &local_bd_type, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_BD_ADDR_TYPE, &remote_bd_type, conn_id);
            APP_PRINT_INFO3("GAP_CONN_STATE_CONNECTED: conn_id %d, local_bd_type %d, remote_bd_type %d\n",
                            conn_id, local_bd_type, remote_bd_type);
			data_uart_debug("GAP_CONN_STATE_CONNECTED: conn_id %d, local_bd_type %d, remote_bd_type %d\n",
                            conn_id, local_bd_type, remote_bd_type);
#endif

#if F_BT_LE_5_0_SET_PHY_SUPPORT
            {
                uint8_t tx_phy;
                uint8_t rx_phy;
                le_get_conn_param(GAP_PARAM_CONN_RX_PHY_TYPE, &rx_phy, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_TX_PHY_TYPE, &tx_phy, conn_id);
                APP_PRINT_INFO2("GAP_CONN_STATE_CONNECTED: tx_phy %d, rx_phy %d", tx_phy, rx_phy);
            }
#endif

#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
            le_set_data_len(conn_id, 251, 2120);
#endif
#if defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL
            mesh_le_adv_stop();
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
            if (bt_mesh_multiple_profile_app_link_table[conn_id].role == GAP_LINK_ROLE_SLAVE) {
                mesh_le_adv_stop();
            }
#endif
        }
        break;

    default:
        break;

    }

}

/**
 * @brief    Handle msg GAP_MSG_LE_AUTHEN_STATE_CHANGE
 * @note     All the gap authentication state events are pre-handled in this function.
 *           Then the event handling function shall be called according to the new_state
 * @param[in] conn_id Connection ID
 * @param[in] new_state  New authentication state
 * @param[in] cause Use this cause when new_state is GAP_AUTHEN_STATE_COMPLETE
 * @return   void
 */
void bt_mesh_device_multiple_profile_app_handle_authen_state_evt(uint8_t conn_id, uint8_t new_state, uint16_t cause)
{
    APP_PRINT_INFO2("bt_mesh_device_multiple_profile_app_handle_authen_state_evt:conn_id %d, cause 0x%x", conn_id, cause);

    switch (new_state)
    {
    case GAP_AUTHEN_STATE_STARTED:
        {
            APP_PRINT_INFO0("bt_mesh_device_multiple_profile_app_handle_authen_state_evt: GAP_AUTHEN_STATE_STARTED");
        }
        break;

    case GAP_AUTHEN_STATE_COMPLETE:
        {
            if (cause == GAP_SUCCESS)
            {
                data_uart_debug("Pair success\r\n");
                APP_PRINT_INFO0("bt_mesh_device_multiple_profile_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair success");

            }
            else
            {
                data_uart_debug("Pair failed: cause 0x%x\r\n", cause);
                APP_PRINT_INFO0("bt_mesh_device_multiple_profile_app_handle_authen_state_evt: GAP_AUTHEN_STATE_COMPLETE pair failed");
            }
        }
        break;

    default:
        {
            APP_PRINT_ERROR1("bt_mesh_device_multiple_profile_app_handle_authen_state_evt: unknown newstate %d", new_state);
        }
        break;
    }
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_MTU_INFO
 * @note     This msg is used to inform APP that exchange mtu procedure is completed.
 * @param[in] conn_id Connection ID
 * @param[in] mtu_size  New mtu size
 * @return   void
 */
void bt_mesh_device_multiple_profile_app_handle_conn_mtu_info_evt(uint8_t conn_id, uint16_t mtu_size)
{
    APP_PRINT_INFO2("bt_mesh_device_multiple_profile_app_handle_conn_mtu_info_evt: conn_id %d, mtu_size %d", conn_id, mtu_size);
}

/**
 * @brief    Handle msg GAP_MSG_LE_CONN_PARAM_UPDATE
 * @note     All the connection parameter update change  events are pre-handled in this function.
 * @param[in] conn_id Connection ID
 * @param[in] status  New update state
 * @param[in] cause Use this cause when status is GAP_CONN_PARAM_UPDATE_STATUS_FAIL
 * @return   void
 */
void bt_mesh_device_multiple_profile_app_handle_conn_param_update_evt(uint8_t conn_id, uint8_t status, uint16_t cause)
{
    switch (status)
    {
    case GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS:
        {
            uint16_t conn_interval;
            uint16_t conn_slave_latency;
            uint16_t conn_supervision_timeout;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &conn_interval, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &conn_slave_latency, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &conn_supervision_timeout, conn_id);
            APP_PRINT_INFO4("bt_mesh_device_multiple_profile_app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
            data_uart_debug("\n\rbt_mesh_device_multiple_profile_app_handle_conn_param_update_evt update success:conn_id %d, conn_interval 0x%x, conn_slave_latency 0x%x, conn_supervision_timeout 0x%x \r\n",
                            conn_id, conn_interval, conn_slave_latency, conn_supervision_timeout);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_FAIL:
        {
            APP_PRINT_ERROR2("bt_mesh_device_multiple_profile_app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x",
                             conn_id, cause);
            data_uart_debug("\n\rbt_mesh_device_multiple_profile_app_handle_conn_param_update_evt update failed: conn_id %d, cause 0x%x\r\n",
                             conn_id, cause);
        }
        break;

    case GAP_CONN_PARAM_UPDATE_STATUS_PENDING:
        {
            APP_PRINT_INFO1("bt_mesh_device_multiple_profile_app_handle_conn_param_update_evt update pending: conn_id %d", conn_id);
            data_uart_debug("\n\rbt_mesh_device_multiple_profile_app_handle_conn_param_update_evt update pending: conn_id %d\r\n", conn_id);
        }
        break;

    default:
        break;
    }
}

bool mesh_initial_state = FALSE;

/**
 * @brief    All the BT GAP MSG are pre-handled in this function.
 * @note     Then the event handling function shall be called according to the
 *           sub_type of T_IO_MSG
 * @param[in] p_gap_msg Pointer to GAP msg
 * @return   void
 */
void bt_mesh_device_multiple_profile_app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    uint8_t conn_id;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));

    APP_PRINT_TRACE1("bt_mesh_device_multiple_profile_app_handle_gap_msg: sub_type %d", p_gap_msg->subtype);
    mesh_inner_msg_t mesh_inner_msg;
    mesh_inner_msg.type = MESH_BT_STATUS_UPDATE;
    mesh_inner_msg.sub_type = p_gap_msg->subtype;
    mesh_inner_msg.parm = p_gap_msg->u.param;
    gap_sched_handle_bt_status_msg(&mesh_inner_msg);
    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            if (!mesh_initial_state)
            {
                mesh_initial_state = TRUE;
                /** set device uuid according to bt address */
                uint8_t bt_addr[6];
                uint8_t dev_uuid[16] = MESH_DEVICE_UUID;
                gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
                memcpy(dev_uuid, bt_addr, sizeof(bt_addr));
                device_uuid_set(dev_uuid);
            }
            bt_mesh_device_multiple_profile_app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                     gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            bt_mesh_device_multiple_profile_app_handle_conn_state_evt(gap_msg.msg_data.gap_conn_state_change.conn_id,
                                      (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state,
                                      gap_msg.msg_data.gap_conn_state_change.disc_cause);
        }
        break;

    case GAP_MSG_LE_CONN_MTU_INFO:
        {
            bt_mesh_device_multiple_profile_app_handle_conn_mtu_info_evt(gap_msg.msg_data.gap_conn_mtu_info.conn_id,
                                         gap_msg.msg_data.gap_conn_mtu_info.mtu_size);
        }
        break;

    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        {
            bt_mesh_device_multiple_profile_app_handle_conn_param_update_evt(gap_msg.msg_data.gap_conn_param_update.conn_id,
                                             gap_msg.msg_data.gap_conn_param_update.status,
                                             gap_msg.msg_data.gap_conn_param_update.cause);
        }
        break;

    case GAP_MSG_LE_AUTHEN_STATE_CHANGE:
        {
            bt_mesh_device_multiple_profile_app_handle_authen_state_evt(gap_msg.msg_data.gap_authen_state.conn_id,
                                        gap_msg.msg_data.gap_authen_state.new_state,
                                        gap_msg.msg_data.gap_authen_state.status);
        }
        break;

    case GAP_MSG_LE_BOND_JUST_WORK:
        {
            conn_id = gap_msg.msg_data.gap_bond_just_work_conf.conn_id;
            le_bond_just_work_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            APP_PRINT_INFO0("GAP_MSG_LE_BOND_JUST_WORK");
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_DISPLAY:
        {
            uint32_t display_value = 0;
            conn_id = gap_msg.msg_data.gap_bond_passkey_display.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn_id %d, passkey %d",
                            conn_id, display_value);
            le_bond_passkey_display_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
            data_uart_debug("GAP_MSG_LE_BOND_PASSKEY_DISPLAY: conn_id %d, passkey %d\r\n",
                            conn_id,
                            display_value);
        }
        break;

    case GAP_MSG_LE_BOND_USER_CONFIRMATION:
        {
            uint32_t display_value = 0;
            conn_id = gap_msg.msg_data.gap_bond_user_conf.conn_id;
            le_bond_get_display_key(conn_id, &display_value);
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_USER_CONFIRMATION: conn_id %d, passkey %d",
                            conn_id, display_value);
            data_uart_debug("GAP_MSG_LE_BOND_USER_CONFIRMATION: conn_id %d, passkey %d\r\n",
                            conn_id,
                            display_value);
            //le_bond_user_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

    case GAP_MSG_LE_BOND_PASSKEY_INPUT:
        {
            //uint32_t passkey = 888888;
            conn_id = gap_msg.msg_data.gap_bond_passkey_input.conn_id;
            APP_PRINT_INFO2("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d, key_press %d",
                            conn_id, gap_msg.msg_data.gap_bond_passkey_input.key_press);
            data_uart_debug("GAP_MSG_LE_BOND_PASSKEY_INPUT: conn_id %d\r\n", conn_id);
            //le_bond_passkey_input_confirm(conn_id, passkey, GAP_CFM_CAUSE_ACCEPT);
        }
        break;

#if F_BT_LE_SMP_OOB_SUPPORT
    case GAP_MSG_LE_BOND_OOB_INPUT:
        {
            uint8_t oob_data[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            conn_id = gap_msg.msg_data.gap_bond_oob_input.conn_id;
            APP_PRINT_INFO1("GAP_MSG_LE_BOND_OOB_INPUT: conn_id %d", conn_id);
            le_bond_set_param(GAP_PARAM_BOND_OOB_DATA, GAP_OOB_LEN, oob_data);
            le_bond_oob_input_confirm(conn_id, GAP_CFM_CAUSE_ACCEPT);
        }
        break;
#endif

    default:
        APP_PRINT_ERROR1("bt_mesh_device_app_handle_gap_msg: unknown sub_type %d", p_gap_msg->subtype);
        break;
    }
}

#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
/**
  * @brief Used to parse advertising data and scan response data
  * @param[in] scan_info point to scan information data.
  * @retval void
  */
void bt_mesh_central_app_parse_scan_info(T_LE_SCAN_INFO *scan_info)
{
    uint8_t buffer[32];
    uint8_t pos = 0;

    while (pos < scan_info->data_len)
    {
        /* Length of the AD structure. */
        uint8_t length = scan_info->data[pos++];
        uint8_t type;

        if ((length > 0x01) && ((pos + length) <= 31))
        {
            /* Copy the AD Data to buffer. */
            memcpy(buffer, scan_info->data + pos + 1, length - 1);
            /* AD Type, one octet. */
            type = scan_info->data[pos];

            APP_PRINT_TRACE2("ble_central_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d", type,
                             length - 1);
//            BLE_PRINT("ble_central_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d\n\r", type,
//                             length - 1);


            switch (type)
            {
            case GAP_ADTYPE_FLAGS:
                {
                    /* (flags & 0x01) -- LE Limited Discoverable Mode */
                    /* (flags & 0x02) -- LE General Discoverable Mode */
                    /* (flags & 0x04) -- BR/EDR Not Supported */
                    /* (flags & 0x08) -- Simultaneous LE and BR/EDR to Same Device Capable (Controller) */
                    /* (flags & 0x10) -- Simultaneous LE and BR/EDR to Same Device Capable (Host) */
                    uint8_t flags = scan_info->data[pos + 1];
                    APP_PRINT_INFO1("GAP_ADTYPE_FLAGS: 0x%x", flags);
					BLE_PRINT("GAP_ADTYPE_FLAGS: 0x%x\n\r", flags);

                }
                break;

            case GAP_ADTYPE_16BIT_MORE:
            case GAP_ADTYPE_16BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_16BIT:
                {
                    uint16_t *p_uuid = (uint16_t *)(buffer);
                    uint8_t i = length - 1;

                    while (i >= 2)
                    {
                        APP_PRINT_INFO1("GAP_ADTYPE_16BIT_XXX: 0x%x", *p_uuid);
                        BLE_PRINT("GAP_ADTYPE_16BIT_XXX: 0x%x\n\r", *p_uuid);
						p_uuid ++;
                        i -= 2;
                    }
                }
                break;

            case GAP_ADTYPE_32BIT_MORE:
            case GAP_ADTYPE_32BIT_COMPLETE:
                {
                    uint32_t *p_uuid = (uint32_t *)(buffer);
                    uint8_t    i     = length - 1;

                    while (i >= 4)
                    {
                        APP_PRINT_INFO1("GAP_ADTYPE_32BIT_XXX: 0x%x", *p_uuid);
                        BLE_PRINT("GAP_ADTYPE_32BIT_XXX: 0x%x\n\r", (unsigned int)*p_uuid);
						p_uuid ++;

                        i -= 4;
                    }
                }
                break;

            case GAP_ADTYPE_128BIT_MORE:
            case GAP_ADTYPE_128BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_128BIT:
                {
                    uint32_t *p_uuid = (uint32_t *)(buffer);
                    APP_PRINT_INFO4("GAP_ADTYPE_128BIT_XXX: 0x%8.8x%8.8x%8.8x%8.8x",
                                    p_uuid[3], p_uuid[2], p_uuid[1], p_uuid[0]);
					BLE_PRINT("GAP_ADTYPE_128BIT_XXX: 0x%8.8x%8.8x%8.8x%8.8x\n\r",
									(unsigned int)p_uuid[3], (unsigned int)p_uuid[2], (unsigned int)p_uuid[1], (unsigned int)p_uuid[0]);

                }
                break;

            case GAP_ADTYPE_LOCAL_NAME_SHORT:
            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                {
                    buffer[length - 1] = '\0';
                    APP_PRINT_INFO1("GAP_ADTYPE_LOCAL_NAME_XXX: %s", TRACE_STRING(buffer));
					BLE_PRINT("GAP_ADTYPE_LOCAL_NAME_XXX: %s\n\r", buffer);

                }
                break;

            case GAP_ADTYPE_POWER_LEVEL:
                {
                    APP_PRINT_INFO1("GAP_ADTYPE_POWER_LEVEL: 0x%x", scan_info->data[pos + 1]);
					BLE_PRINT("GAP_ADTYPE_POWER_LEVEL: 0x%x\n\r", scan_info->data[pos + 1]);

                }
                break;

            case GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE:
                {
                    uint16_t *p_min = (uint16_t *)(buffer);
                    uint16_t *p_max = p_min + 1;
                    APP_PRINT_INFO2("GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE: 0x%x - 0x%x", *p_min,
                                    *p_max);
					BLE_PRINT("GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE: 0x%x - 0x%x\n\r", *p_min,
									*p_max);

                }
                break;

            case GAP_ADTYPE_SERVICE_DATA:
                {
                    uint16_t *p_uuid = (uint16_t *)(buffer);
                    uint8_t data_len = length - 3;

                    APP_PRINT_INFO3("GAP_ADTYPE_SERVICE_DATA: UUID 0x%x, len %d, data %b", *p_uuid,
                                    data_len, TRACE_BINARY(data_len, &buffer[2]));
					BLE_PRINT("GAP_ADTYPE_SERVICE_DATA: UUID 0x%x, len %d\n\r", *p_uuid,
								data_len);

                }
                break;
            case GAP_ADTYPE_APPEARANCE:
                {
                    uint16_t *p_appearance = (uint16_t *)(buffer);
                    APP_PRINT_INFO1("GAP_ADTYPE_APPEARANCE: %d", *p_appearance);
					BLE_PRINT("GAP_ADTYPE_APPEARANCE: %d\n\r", *p_appearance);

                }
                break;

            case GAP_ADTYPE_MANUFACTURER_SPECIFIC:
                {
                    uint8_t data_len = length - 3;
                    uint16_t *p_company_id = (uint16_t *)(buffer);
                    APP_PRINT_INFO3("GAP_ADTYPE_MANUFACTURER_SPECIFIC: company_id 0x%x, len %d, data %b",
                                    *p_company_id, data_len, TRACE_BINARY(data_len, &buffer[2]));
					BLE_PRINT("GAP_ADTYPE_MANUFACTURER_SPECIFIC: company_id 0x%x, len %d\n\r",
									*p_company_id, data_len);

                }
                break;

            default:
                {
                    uint8_t i = 0;

                    for (i = 0; i < (length - 1); i++)
                    {
                        APP_PRINT_INFO1("  AD Data: Unhandled Data = 0x%x", scan_info->data[pos + i]);
//						BLE_PRINT("  AD Data: Unhandled Data = 0x%x\n\r", scan_info->data[pos + i]);

                    }
                }
                break;
            }
        }

        pos += length;
    }
}
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
/** @} */ /* End of group CENTRAL_CLIENT_GAP_MSG */

/** @defgroup  CENTRAL_CLIENT_GAP_CALLBACK GAP Callback Event Handler
    * @brief Handle GAP callback event
    * @{
    */
/**
  * @brief Used to parse advertising data and scan response data
  * @param[in] scan_info point to scan information data.
  * @retval void
  */
void bt_mesh_scatternet_app_parse_scan_info(T_LE_SCAN_INFO *scan_info)
{
    uint8_t buffer[32];
    uint8_t pos = 0;

    while (pos < scan_info->data_len)
    {
        /* Length of the AD structure. */
        uint8_t length = scan_info->data[pos++];
        uint8_t type;

        if ((length > 0x01) && ((pos + length) <= 31))
        {
            /* Copy the AD Data to buffer. */
            memcpy(buffer, scan_info->data + pos + 1, length - 1);
            /* AD Type, one octet. */
            type = scan_info->data[pos];

            APP_PRINT_TRACE2("ble_scatternet_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d", type,
                             length - 1);
//            BLE_PRINT("ble_scatternet_app_parse_scan_info: AD Structure Info: AD type 0x%x, AD Data Length %d\n\r", type,
//                             length - 1);


            switch (type)
            {
            case GAP_ADTYPE_FLAGS:
                {
                    /* (flags & 0x01) -- LE Limited Discoverable Mode */
                    /* (flags & 0x02) -- LE General Discoverable Mode */
                    /* (flags & 0x04) -- BR/EDR Not Supported */
                    /* (flags & 0x08) -- Simultaneous LE and BR/EDR to Same Device Capable (Controller) */
                    /* (flags & 0x10) -- Simultaneous LE and BR/EDR to Same Device Capable (Host) */
                    uint8_t flags = scan_info->data[pos + 1];
                    APP_PRINT_INFO1("GAP_ADTYPE_FLAGS: 0x%x", flags);
					BLE_PRINT("GAP_ADTYPE_FLAGS: 0x%x\n\r", flags);

                }
                break;

            case GAP_ADTYPE_16BIT_MORE:
            case GAP_ADTYPE_16BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_16BIT:
                {
                    uint16_t *p_uuid = (uint16_t *)(buffer);
                    uint8_t i = length - 1;

                    while (i >= 2)
                    {
                        APP_PRINT_INFO1("GAP_ADTYPE_16BIT_XXX: 0x%x", *p_uuid);
                        BLE_PRINT("GAP_ADTYPE_16BIT_XXX: 0x%x\n\r", *p_uuid);
						p_uuid ++;
                        i -= 2;
                    }
                }
                break;

            case GAP_ADTYPE_32BIT_MORE:
            case GAP_ADTYPE_32BIT_COMPLETE:
                {
                    uint32_t *p_uuid = (uint32_t *)(buffer);
                    uint8_t    i     = length - 1;

                    while (i >= 4)
                    {
                        APP_PRINT_INFO1("GAP_ADTYPE_32BIT_XXX: 0x%x", *p_uuid);
                        BLE_PRINT("GAP_ADTYPE_32BIT_XXX: 0x%x\n\r", (unsigned int)*p_uuid);
						p_uuid ++;

                        i -= 4;
                    }
                }
                break;

            case GAP_ADTYPE_128BIT_MORE:
            case GAP_ADTYPE_128BIT_COMPLETE:
            case GAP_ADTYPE_SERVICES_LIST_128BIT:
                {
                    uint32_t *p_uuid = (uint32_t *)(buffer);
                    APP_PRINT_INFO4("GAP_ADTYPE_128BIT_XXX: 0x%8.8x%8.8x%8.8x%8.8x",
                                    p_uuid[3], p_uuid[2], p_uuid[1], p_uuid[0]);
					BLE_PRINT("GAP_ADTYPE_128BIT_XXX: 0x%8.8x%8.8x%8.8x%8.8x\n\r",
									(unsigned int)p_uuid[3], (unsigned int)p_uuid[2], (unsigned int)p_uuid[1], (unsigned int)p_uuid[0]);

                }
                break;

            case GAP_ADTYPE_LOCAL_NAME_SHORT:
            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                {
                    buffer[length - 1] = '\0';
                    APP_PRINT_INFO1("GAP_ADTYPE_LOCAL_NAME_XXX: %s", TRACE_STRING(buffer));
					BLE_PRINT("GAP_ADTYPE_LOCAL_NAME_XXX: %s\n\r", buffer);

                }
                break;

            case GAP_ADTYPE_POWER_LEVEL:
                {
                    APP_PRINT_INFO1("GAP_ADTYPE_POWER_LEVEL: 0x%x", scan_info->data[pos + 1]);
					BLE_PRINT("GAP_ADTYPE_POWER_LEVEL: 0x%x\n\r", scan_info->data[pos + 1]);

                }
                break;

            case GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE:
                {
                    uint16_t *p_min = (uint16_t *)(buffer);
                    uint16_t *p_max = p_min + 1;
                    APP_PRINT_INFO2("GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE: 0x%x - 0x%x", *p_min,
                                    *p_max);
					BLE_PRINT("GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE: 0x%x - 0x%x\n\r", *p_min,
									*p_max);

                }
                break;

            case GAP_ADTYPE_SERVICE_DATA:
                {
                    uint16_t *p_uuid = (uint16_t *)(buffer);
                    uint8_t data_len = length - 3;

                    APP_PRINT_INFO3("GAP_ADTYPE_SERVICE_DATA: UUID 0x%x, len %d, data %b", *p_uuid,
                                    data_len, TRACE_BINARY(data_len, &buffer[2]));
					BLE_PRINT("GAP_ADTYPE_SERVICE_DATA: UUID 0x%x, len %d\n\r", *p_uuid,
								data_len);

                }
                break;
            case GAP_ADTYPE_APPEARANCE:
                {
                    uint16_t *p_appearance = (uint16_t *)(buffer);
                    APP_PRINT_INFO1("GAP_ADTYPE_APPEARANCE: %d", *p_appearance);
					BLE_PRINT("GAP_ADTYPE_APPEARANCE: %d\n\r", *p_appearance);

                }
                break;

            case GAP_ADTYPE_MANUFACTURER_SPECIFIC:
                {
                    uint8_t data_len = length - 3;
                    uint16_t *p_company_id = (uint16_t *)(buffer);
                    APP_PRINT_INFO3("GAP_ADTYPE_MANUFACTURER_SPECIFIC: company_id 0x%x, len %d, data %b",
                                    *p_company_id, data_len, TRACE_BINARY(data_len, &buffer[2]));
					BLE_PRINT("GAP_ADTYPE_MANUFACTURER_SPECIFIC: company_id 0x%x, len %d\n\r",
									*p_company_id, data_len);

                }
                break;

            default:
                {
                    uint8_t i = 0;

                    for (i = 0; i < (length - 1); i++)
                    {
                        APP_PRINT_INFO1("  AD Data: Unhandled Data = 0x%x", scan_info->data[pos + i]);
//						BLE_PRINT("  AD Data: Unhandled Data = 0x%x\n\r", scan_info->data[pos + i]);

                    }
                }
                break;
            }
        }

        pos += length;
    }
}
#endif

/**
  * @brief Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval result @ref T_APP_RESULT
  */
T_APP_RESULT bt_mesh_device_multiple_profile_app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;
#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    char adv_type[20];
	char remote_addr_type[10];
#endif

    switch (cb_type)
    {
    /* common msg*/
    case GAP_MSG_LE_READ_RSSI:
        APP_PRINT_INFO3("GAP_MSG_LE_READ_RSSI:conn_id 0x%x cause 0x%x rssi %d",
                        p_data->p_le_read_rssi_rsp->conn_id,
                        p_data->p_le_read_rssi_rsp->cause,
                        p_data->p_le_read_rssi_rsp->rssi);
        break;

#if F_BT_LE_4_2_DATA_LEN_EXT_SUPPORT
    case GAP_MSG_LE_DATA_LEN_CHANGE_INFO:
        APP_PRINT_INFO3("GAP_MSG_LE_DATA_LEN_CHANGE_INFO: conn_id %d, tx octets 0x%x, max_tx_time 0x%x",
                        p_data->p_le_data_len_change_info->conn_id,
                        p_data->p_le_data_len_change_info->max_tx_octets,
                        p_data->p_le_data_len_change_info->max_tx_time);
        break;
#endif

    case GAP_MSG_LE_BOND_MODIFY_INFO:
        APP_PRINT_INFO1("GAP_MSG_LE_BOND_MODIFY_INFO: type 0x%x",
                        p_data->p_le_bond_modify_info->type);
        break;

    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("GAP_MSG_LE_MODIFY_WHITE_LIST: operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
        break;
    /* central reference msg*/
    case GAP_MSG_LE_SCAN_INFO:
        APP_PRINT_INFO5("GAP_MSG_LE_SCAN_INFO:adv_type 0x%x, bd_addr %s, remote_addr_type %d, rssi %d, data_len %d",
                        p_data->p_le_scan_info->adv_type,
                        TRACE_BDADDR(p_data->p_le_scan_info->bd_addr),
                        p_data->p_le_scan_info->remote_addr_type,
                        p_data->p_le_scan_info->rssi,
                        p_data->p_le_scan_info->data_len);       
#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
        if (bt_mesh_multiple_profile_scan_state) {
            /* If you want to parse the scan info, please reference function ble_central_app_parse_scan_info. */
    		sprintf(adv_type,"%s",(p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_UNDIRECTED)? "CON_UNDIRECT":
    							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_DIRECTED)? "CON_DIRECT":
    							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_SCANNABLE)? "SCANABLE_UNDIRCT":
    							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_NON_CONNECTABLE)? "NON_CONNECTABLE":
    							  (p_data->p_le_scan_info->adv_type ==GAP_ADV_EVT_TYPE_SCAN_RSP)? "SCAN_RSP":"unknown");
    		sprintf(remote_addr_type,"%s",(p_data->p_le_scan_info->remote_addr_type == GAP_REMOTE_ADDR_LE_PUBLIC)? "public":
    							   (p_data->p_le_scan_info->remote_addr_type == GAP_REMOTE_ADDR_LE_RANDOM)? "random":"unknown");

    		BLE_PRINT("ADVType\t\t\t| AddrType\t|%s\t\t\t|rssi\n\r","BT_Addr");
    		BLE_PRINT("%s\t\t%s\t"BD_ADDR_FMT"\t%d\n\r",adv_type,remote_addr_type,BD_ADDR_ARG(p_data->p_le_scan_info->bd_addr),
    												p_data->p_le_scan_info->rssi);
#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
            bt_mesh_central_app_parse_scan_info(p_data->p_le_scan_info);
#elif defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET  
            bt_mesh_scatternet_app_parse_scan_info(p_data->p_le_scan_info);
#endif
        }
#endif
        gap_sched_handle_adv_report(p_data->p_le_scan_info);
        break;

#if F_BT_LE_GAP_CENTRAL_SUPPORT
    case GAP_MSG_LE_CONN_UPDATE_IND:
        APP_PRINT_INFO5("GAP_MSG_LE_CONN_UPDATE_IND: conn_id %d, conn_interval_max 0x%x, conn_interval_min 0x%x, conn_latency 0x%x,supervision_timeout 0x%x",
                        p_data->p_le_conn_update_ind->conn_id,
                        p_data->p_le_conn_update_ind->conn_interval_max,
                        p_data->p_le_conn_update_ind->conn_interval_min,
                        p_data->p_le_conn_update_ind->conn_latency,
                        p_data->p_le_conn_update_ind->supervision_timeout);
        /* if reject the proposed connection parameter from peer device, use APP_RESULT_REJECT. */
        result = APP_RESULT_ACCEPT;
        break;

    case GAP_MSG_LE_SET_HOST_CHANN_CLASSIF:
        APP_PRINT_INFO1("GAP_MSG_LE_SET_HOST_CHANN_CLASSIF: cause 0x%x",
                        p_data->p_le_set_host_chann_classif_rsp->cause);
        break;
#endif
    /* peripheral reference msg*/
    case GAP_MSG_LE_ADV_UPDATE_PARAM:
        APP_PRINT_INFO1("GAP_MSG_LE_ADV_UPDATE_PARAM: cause 0x%x",
                        p_data->p_le_adv_update_param_rsp->cause);
        gap_sched_adv_params_set_done();
        break;
		/*
    case GAP_MSG_LE_VENDOR_ONE_SHOT_ADV:
        APP_PRINT_INFO1("GAP_MSG_LE_VENDOR_ONE_SHOT_ADV: cause 0x%x",
                        p_data->le_cause.cause);
        gap_sched_adv_done(GAP_SCHED_ADV_END_TYPE_SUCCESS);
        break;
    case GAP_MSG_LE_DISABLE_SLAVE_LATENCY:
        APP_PRINT_INFO1("GAP_MSG_LE_DISABLE_SLAVE_LATENCY: cause 0x%x",
                        p_data->p_le_disable_slave_latency_rsp->cause);
        break;

    case GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:
        APP_PRINT_INFO1("GAP_MSG_LE_UPDATE_PASSED_CHANN_MAP:cause 0x%x",
                        p_data->p_le_update_passed_chann_map_rsp->cause);
        break;
		*/
#if F_BT_LE_5_0_SET_PHY_SUPPORT
    case GAP_MSG_LE_PHY_UPDATE_INFO:
        APP_PRINT_INFO4("GAP_MSG_LE_PHY_UPDATE_INFO:conn_id %d, cause 0x%x, rx_phy %d, tx_phy %d",
                        p_data->p_le_phy_update_info->conn_id,
                        p_data->p_le_phy_update_info->cause,
                        p_data->p_le_phy_update_info->rx_phy,
                        p_data->p_le_phy_update_info->tx_phy);
        data_uart_print("GAP_MSG_LE_PHY_UPDATE_INFO:conn_id %d, cause 0x%x, rx_phy %d, tx_phy %d\r\n",
						p_data->p_le_phy_update_info->conn_id,
						p_data->p_le_phy_update_info->cause,
						p_data->p_le_phy_update_info->rx_phy,
						p_data->p_le_phy_update_info->tx_phy);
        break;

    case GAP_MSG_LE_REMOTE_FEATS_INFO:
        {
            uint8_t  remote_feats[8];
            APP_PRINT_INFO3("GAP_MSG_LE_REMOTE_FEATS_INFO: conn id %d, cause 0x%x, remote_feats %b",
                            p_data->p_le_remote_feats_info->conn_id,
                            p_data->p_le_remote_feats_info->cause,
                            TRACE_BINARY(8, p_data->p_le_remote_feats_info->remote_feats));
            if (p_data->p_le_remote_feats_info->cause == GAP_SUCCESS)
            {
                memcpy(remote_feats, p_data->p_le_remote_feats_info->remote_feats, 8);
                if (remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] & LE_SUPPORT_FEATURES_LE_2M_MASK_BIT)
                {
                    APP_PRINT_INFO0("GAP_MSG_LE_REMOTE_FEATS_INFO: support 2M");
                }
                if (remote_feats[LE_SUPPORT_FEATURES_MASK_ARRAY_INDEX1] & LE_SUPPORT_FEATURES_LE_CODED_PHY_MASK_BIT)
                {
                    APP_PRINT_INFO0("GAP_MSG_LE_REMOTE_FEATS_INFO: support CODED");
                }
            }
        }
        break;
#endif

    default:
        APP_PRINT_ERROR1("bt_mesh_device_app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}

#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
#if F_BT_GAPS_CHAR_WRITEABLE
/** @defgroup  SCATTERNET_GAPS_WRITE GAP Service Callback Handler
    * @brief Use @ref F_BT_GAPS_CHAR_WRITEABLE to open
    * @{
    */
/**
 * @brief    All the BT GAP service callback events are handled in this function
 * @param[in] service_id  Profile service ID
 * @param[in] p_para      Pointer to callback data
 * @return   Indicates the function call is successful or not
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT bt_mesh_scatternet_gap_service_callback(T_SERVER_ID service_id, void *p_para)
{
    (void) service_id;
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    T_GAPS_CALLBACK_DATA *p_gap_data = (T_GAPS_CALLBACK_DATA *)p_para;
    APP_PRINT_INFO2("bt_mesh_scatternet_gap_service_callback: conn_id = %d msg_type = %d\n", p_gap_data->conn_id,
                    p_gap_data->msg_type);
	APP_PRINT_INFO2("bt_mesh_scatternet_gap_service_callback: len = 0x%x,opcode = %d\n", p_gap_data->msg_data.len,
                    p_gap_data->msg_data.opcode);
    if (p_gap_data->msg_type == SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE)
    {
        switch (p_gap_data->msg_data.opcode)
        {
        case GAPS_WRITE_DEVICE_NAME:
            {
                T_LOCAL_NAME device_name;
                memcpy(device_name.local_name, p_gap_data->msg_data.p_value, p_gap_data->msg_data.len);
                device_name.local_name[p_gap_data->msg_data.len] = 0;
				//data_uart_debug("GAPS_WRITE_DEVICE_NAME:device_name = %s \n\r",device_name.local_name);
                flash_save_local_name(&device_name);
            }
            break;

        case GAPS_WRITE_APPEARANCE:
            {
                uint16_t appearance_val;
                T_LOCAL_APPEARANCE appearance;

                LE_ARRAY_TO_UINT16(appearance_val, p_gap_data->msg_data.p_value);
                appearance.local_appearance = appearance_val;
				//data_uart_debug("GAPS_WRITE_APPEARANCE:appearance = %s \n\r",appearance.local_appearance);
                flash_save_local_appearance(&appearance);
            }
            break;
        default:
			APP_PRINT_ERROR1("bt_mesh_scatternet_gap_service_callback: unhandled msg_data.opcode 0x%x", p_gap_data->msg_data.opcode);
			//data_uart_debug("bt_mesh_scatternet_gap_service_callback: unhandled msg_data.opcode 0x%x", p_gap_data->msg_data.opcode);
            break;
        }
    }
    return result;
}
/** @} */
#endif

/** @defgroup  GCS_CLIIENT_CALLBACK GCS Client Callback Event Handler
    * @brief Handle profile client callback event
    * @{
    */
void bt_mesh_scatternet_gcs_handle_discovery_result(uint8_t conn_id, T_GCS_DISCOVERY_RESULT discov_result)
{
    uint16_t i;
    T_GCS_DISCOV_RESULT *p_result_table;
    uint16_t    properties;
    switch (discov_result.discov_type)
    {
    case GCS_ALL_PRIMARY_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_PRIMARY_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_ALL_SRV_UUID16:
                APP_PRINT_INFO4("ALL SRV UUID16[%d]: service range: 0x%x-0x%x, uuid16 0x%x",
                                i, p_result_table->result_data.srv_uuid16_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.end_group_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.uuid16);
                data_uart_debug("ALL SRV UUID16[%d]: service range: 0x%x-0x%x, uuid16 0x%x\n\r",
               				 i, p_result_table->result_data.srv_uuid16_disc_data.att_handle,
               				 p_result_table->result_data.srv_uuid16_disc_data.end_group_handle,
               				 p_result_table->result_data.srv_uuid16_disc_data.uuid16);
                break;
            case DISC_RESULT_ALL_SRV_UUID128:
                APP_PRINT_INFO4("ALL SRV UUID128[%d]: service range: 0x%x-0x%x, service=<%b>",
                                i, p_result_table->result_data.srv_uuid128_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid128_disc_data.end_group_handle,
                                TRACE_BINARY(16, p_result_table->result_data.srv_uuid128_disc_data.uuid128));
                data_uart_debug("ALL SRV UUID128[%d]: service range: 0x%x-0x%x, service="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.srv_uuid128_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid128_disc_data.end_group_handle,
                                UUID_128(p_result_table->result_data.srv_uuid128_disc_data.uuid128));
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
				data_uart_debug("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_BY_UUID128_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID128_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        data_uart_debug("conn_id %d, GCS_BY_UUID128_SRV_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);

        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_SRV_DATA:
                APP_PRINT_INFO3("SRV DATA[%d]: service range: 0x%x-0x%x",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                data_uart_debug("SRV DATA[%d]: service range: 0x%x-0x%x\n\r",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                data_uart_debug("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_BY_UUID_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        data_uart_debug("conn_id %d, GCS_BY_UUID_SRV_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_SRV_DATA:
                APP_PRINT_INFO3("SRV DATA[%d]: service range: 0x%x-0x%x",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                data_uart_debug("SRV DATA[%d]: service range: 0x%x-0x%x\n\r",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                data_uart_debug("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_ALL_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        data_uart_debug("conn_id %d, GCS_ALL_CHAR_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_CHAR_UUID16:
                properties = p_result_table->result_data.char_uuid16_disc_data.properties;
                APP_PRINT_INFO5("CHAR UUID16[%d]: decl_handle 0x%x, properties 0x%x, value_handle 0x%x, uuid16 0x%x",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\r\n",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY);
                data_uart_debug("CHAR UUID16[%d]: decl_handle 0x%x, properties 0x%x, value_handle 0x%x, uuid16 0x%x\n\r",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                data_uart_debug("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY);
                break;

            case DISC_RESULT_CHAR_UUID128:
                properties = p_result_table->result_data.char_uuid128_disc_data.properties;
                APP_PRINT_INFO5("CHAR UUID128[%d]:  decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_uuid128_disc_data.uuid128));
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                data_uart_debug("CHAR UUID128[%d]:  decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                UUID_128(p_result_table->result_data.char_uuid128_disc_data.uuid128));
                data_uart_debug("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                break;
            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                data_uart_debug("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_BY_UUID_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        data_uart_debug("conn_id %d, GCS_BY_UUID_CHAR_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);

        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_BY_UUID16_CHAR:
                properties = p_result_table->result_data.char_uuid16_disc_data.properties;
                APP_PRINT_INFO5("UUID16 CHAR[%d]: Characteristics by uuid16, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid16=<0x%x>",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                data_uart_debug("UUID16 CHAR[%d]: Characteristics by uuid16, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid16=<0x%x>\n\r",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                data_uart_debug("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                data_uart_debug("Invalid Discovery Result Type!");
                break;
            }
        }
        break;

    case GCS_BY_UUID128_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID128_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        data_uart_debug("conn_id %d, GCS_BY_UUID128_CHAR_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_BY_UUID128_CHAR:
                properties = p_result_table->result_data.char_uuid128_disc_data.properties;
                APP_PRINT_INFO5("UUID128 CHAR[%d]: Characteristics by uuid128, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_uuid128_disc_data.uuid128));
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                data_uart_debug("UUID128 CHAR[%d]: Characteristics by uuid128, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                UUID_128(p_result_table->result_data.char_uuid128_disc_data.uuid128));
                data_uart_debug("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                BLE_PRINT("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_ALL_CHAR_DESC_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_CHAR_DESC_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        data_uart_debug("conn_id %d, GCS_ALL_CHAR_DESC_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_CHAR_DESC_UUID16:
                APP_PRINT_INFO3("DESC UUID16[%d]: Descriptors handle=0x%x, uuid16=<0x%x>",
                                i, p_result_table->result_data.char_desc_uuid16_disc_data.handle,
                                p_result_table->result_data.char_desc_uuid16_disc_data.uuid16);
                data_uart_debug("DESC UUID16[%d]: Descriptors handle=0x%x, uuid16=<0x%x>\n\r",
                                i, p_result_table->result_data.char_desc_uuid16_disc_data.handle,
                                p_result_table->result_data.char_desc_uuid16_disc_data.uuid16);
                break;
            case DISC_RESULT_CHAR_DESC_UUID128:
                APP_PRINT_INFO3("DESC UUID128[%d]: Descriptors handle=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_desc_uuid128_disc_data.handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_desc_uuid128_disc_data.uuid128));
                data_uart_debug("DESC UUID128[%d]: Descriptors handle=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_desc_uuid128_disc_data.handle,
                                UUID_128(p_result_table->result_data.char_desc_uuid128_disc_data.uuid128));
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                data_uart_debug("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    default:
        APP_PRINT_ERROR2("Invalid disc type: conn_id %d, discov_type %d",
                         conn_id, discov_result.discov_type);
        data_uart_debug("Invalid disc type: conn_id %d, discov_type %d\n\r",
                         conn_id, discov_result.discov_type);
        break;
    }
}

/**
 * @brief  Callback will be called when data sent from gcs client.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT bt_mesh_scatternet_gcs_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("bt_mesh_scatternet_gcs_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
    if (client_id == bt_mesh_scatternet_gcs_client_id)
    {
        T_GCS_CLIENT_CB_DATA *p_gcs_cb_data = (T_GCS_CLIENT_CB_DATA *)p_data;
        switch (p_gcs_cb_data->cb_type)
        {
        case GCS_CLIENT_CB_TYPE_DISC_RESULT:
            bt_mesh_scatternet_gcs_handle_discovery_result(conn_id, p_gcs_cb_data->cb_content.discov_result);
            break;
        case GCS_CLIENT_CB_TYPE_READ_RESULT:
            APP_PRINT_INFO3("READ RESULT: cause 0x%x, handle 0x%x, value_len %d",
                            p_gcs_cb_data->cb_content.read_result.cause,
                            p_gcs_cb_data->cb_content.read_result.handle,
                            p_gcs_cb_data->cb_content.read_result.value_size);
            data_uart_debug("READ RESULT: cause 0x%x, handle 0x%x, value_len %d\n\r",
                            p_gcs_cb_data->cb_content.read_result.cause,
                            p_gcs_cb_data->cb_content.read_result.handle,
                            p_gcs_cb_data->cb_content.read_result.value_size);

            if (p_gcs_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO1("READ VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.read_result.value_size,
                                             p_gcs_cb_data->cb_content.read_result.p_value));
                data_uart_debug("READ VALUE: ");
                for(int i=0; i< p_gcs_cb_data->cb_content.read_result.value_size; i++)
                    data_uart_debug("0x%2x ", *(p_gcs_cb_data->cb_content.read_result.p_value + i));
                data_uart_debug("\n\r");
            }
            break;
        case GCS_CLIENT_CB_TYPE_WRITE_RESULT:
            APP_PRINT_INFO3("WRITE RESULT: cause 0x%x, handle 0x%x, type %d",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);
            data_uart_debug("WRITE RESULT: cause 0x%x, handle 0x%x, type %d\n\r",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);
            break;
        case GCS_CLIENT_CB_TYPE_NOTIF_IND:
            if (p_gcs_cb_data->cb_content.notif_ind.notify == false)
            {
                APP_PRINT_INFO2("INDICATION: handle 0x%x, value_size %d",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                APP_PRINT_INFO1("INDICATION VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
                data_uart_debug("INDICATION: handle 0x%x, value_size %d\r\n",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                data_uart_debug("INDICATION VALUE: ");
                for (int i = 0; i < p_gcs_cb_data->cb_content.notif_ind.value_size; i++) {
                    data_uart_debug("0x%2x ", *(p_gcs_cb_data->cb_content.notif_ind.p_value+ i));
                }
                data_uart_debug("\n\r");
            }
            else
            {
                APP_PRINT_INFO2("NOTIFICATION: handle 0x%x, value_size %d",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                APP_PRINT_INFO1("NOTIFICATION VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
                data_uart_debug("NOTIFICATION: handle 0x%x, value_size %d\r\n",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                data_uart_debug("NOTIFICATION VALUE: ");
                for (int i = 0; i < p_gcs_cb_data->cb_content.notif_ind.value_size; i++) {
                    data_uart_debug("0x%2x ", *(p_gcs_cb_data->cb_content.notif_ind.p_value+ i));
                }
                data_uart_debug("\n\r");
            }
            break;
        default:
            break;
        }
    }

    return result;
}
#endif

/**
 * @brief  Callback will be called when data sent from profile client layer.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT bt_mesh_device_multiple_profile_app_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("bt_mesh_device_multiple_profile_app_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
    if (client_id == CLIENT_PROFILE_GENERAL_ID)
    {
        T_CLIENT_APP_CB_DATA *p_client_app_cb_data = (T_CLIENT_APP_CB_DATA *)p_data;
        switch (p_client_app_cb_data->cb_type)
        {
        case CLIENT_APP_CB_TYPE_DISC_STATE:
            if (p_client_app_cb_data->cb_content.disc_state_data.disc_state == DISC_STATE_SRV_DONE)
            {
                APP_PRINT_INFO0("Discovery All Service Procedure Done.");
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
    return result;
}

/**
    * @brief    All the BT Profile service callback events are handled in this function
    * @note     Then the event handling function shall be called according to the
    *           service_id
    * @param    service_id  Profile service ID
    * @param    p_data      Pointer to callback data
    * @return   T_APP_RESULT, which indicates the function call is successful or not
    * @retval   APP_RESULT_SUCCESS  Function run successfully
    * @retval   others              Function run failed, and return number indicates the reason
    */
T_APP_RESULT bt_mesh_device_multiple_profile_app_profile_callback(T_SERVER_ID service_id, void *p_data)
{
    T_APP_RESULT app_result = APP_RESULT_SUCCESS;
    if (service_id == SERVICE_PROFILE_GENERAL_ID)
    {
        T_SERVER_APP_CB_DATA *p_param = (T_SERVER_APP_CB_DATA *)p_data;
        switch (p_param->eventId)
        {
        case PROFILE_EVT_SRV_REG_COMPLETE:// srv register result event.
            APP_PRINT_INFO1("PROFILE_EVT_SRV_REG_COMPLETE: result %d",
                            p_param->event_data.service_reg_result);
            break;

        case PROFILE_EVT_SEND_DATA_COMPLETE:
            APP_PRINT_INFO5("PROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
            data_uart_debug("\n\rPROFILE_EVT_SEND_DATA_COMPLETE: conn_id %d, cause 0x%x, service_id %d, attrib_idx 0x%x, credits %d\r\n",
                            p_param->event_data.send_data_result.conn_id,
                            p_param->event_data.send_data_result.cause,
                            p_param->event_data.send_data_result.service_id,
                            p_param->event_data.send_data_result.attrib_idx,
                            p_param->event_data.send_data_result.credits);
            if (p_param->event_data.send_data_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO0("PROFILE_EVT_SEND_DATA_COMPLETE success");
                data_uart_debug("PROFILE_EVT_SEND_DATA_COMPLETE success\r\n");
            }
            else
            {
                APP_PRINT_ERROR0("PROFILE_EVT_SEND_DATA_COMPLETE failed");
                data_uart_debug("PROFILE_EVT_SEND_DATA_COMPLETE failed\r\n");
            }
            break;

        default:
            break;
        }
    }
    else if (service_id == datatrans_server_id)
    {
        datatrans_server_data_t *pdata = p_data;
        switch (pdata->type)
        {
        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            pdata->len = sizeof(datatrans_sample_data);
            pdata->data = datatrans_sample_data;
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            if (pdata->len > sizeof(datatrans_sample_data))
            {
                pdata->len = sizeof(datatrans_sample_data);
            }
            memcpy(datatrans_sample_data, pdata->data, pdata->len);
            break;
        default:
            break;
        }
    }
#if ((defined(CONFIG_BT_MESH_PERIPHERAL) && CONFIG_BT_MESH_PERIPHERAL) || \
    (defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
    else  if (service_id == bt_mesh_simp_srv_id)
    {
        TSIMP_CALLBACK_DATA *p_simp_cb_data = (TSIMP_CALLBACK_DATA *)p_data;
        switch (p_simp_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_simp_cb_data->msg_data.notification_indification_index)
                {
                case SIMP_NOTIFY_INDICATE_V3_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_ENABLE");
                        data_uart_debug("\n\rSIMP_NOTIFY_INDICATE_V3_ENABLE\r\n");
                    }
                    break;

                case SIMP_NOTIFY_INDICATE_V3_DISABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V3_DISABLE");
                        data_uart_debug("\n\rSIMP_NOTIFY_INDICATE_V3_DISABLE\r\n");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_ENABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_ENABLE");
                        data_uart_debug("\n\rSIMP_NOTIFY_INDICATE_V4_ENABLE\r\n");
                    }
                    break;
                case SIMP_NOTIFY_INDICATE_V4_DISABLE:
                    {
                        APP_PRINT_INFO0("SIMP_NOTIFY_INDICATE_V4_DISABLE");
                        data_uart_debug("\n\rSIMP_NOTIFY_INDICATE_V4_DISABLE\r\n");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_simp_cb_data->msg_data.read_value_index == SIMP_READ_V1)
                {
                    uint8_t value[2] = {0x01, 0x02};
                    APP_PRINT_INFO0("SIMP_READ_V1");
                    data_uart_debug("\n\rSIMP_READ_V1\r\n");
                    simp_ble_service_set_parameter(SIMPLE_BLE_SERVICE_PARAM_V1_READ_CHAR_VAL, 2, &value);
                }
            }
            break;
        case SERVICE_CALLBACK_TYPE_WRITE_CHAR_VALUE:
            {
                switch (p_simp_cb_data->msg_data.write.opcode)
                {
                case SIMP_WRITE_V2:
                    {
                        APP_PRINT_INFO2("SIMP_WRITE_V2: write type %d, len %d", p_simp_cb_data->msg_data.write.write_type,
                                        p_simp_cb_data->msg_data.write.len);
                        data_uart_debug("\n\rSIMP_WRITE_V2 value:");
                        for(int i = 0; i < p_simp_cb_data->msg_data.write.len; i ++){
                            data_uart_debug("0x%2x ", *(p_simp_cb_data->msg_data.write.p_value + i));
                        }
                        data_uart_debug("\n\r");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        default:
            break;
        }
    }
    else if (service_id == bt_mesh_bas_srv_id)
    {
        T_BAS_CALLBACK_DATA *p_bas_cb_data = (T_BAS_CALLBACK_DATA *)p_data;
        switch (p_bas_cb_data->msg_type)
        {
        case SERVICE_CALLBACK_TYPE_INDIFICATION_NOTIFICATION:
            {
                switch (p_bas_cb_data->msg_data.notification_indification_index)
                {
                case BAS_NOTIFY_BATTERY_LEVEL_ENABLE:
                    {
                        APP_PRINT_INFO0("BAS_NOTIFY_BATTERY_LEVEL_ENABLE");
                        data_uart_debug("\n\rBAS_NOTIFY_BATTERY_LEVEL_ENABLE\r\n");
                    }
                    break;

                case BAS_NOTIFY_BATTERY_LEVEL_DISABLE:
                    {
                        APP_PRINT_INFO0("BAS_NOTIFY_BATTERY_LEVEL_DISABLE");
                        data_uart_debug("\n\rBAS_NOTIFY_BATTERY_LEVEL_DISABLE\r\n");
                    }
                    break;
                default:
                    break;
                }
            }
            break;

        case SERVICE_CALLBACK_TYPE_READ_CHAR_VALUE:
            {
                if (p_bas_cb_data->msg_data.read_value_index == BAS_READ_BATTERY_LEVEL)
                {
                    uint8_t battery_level = 90;
                    APP_PRINT_INFO1("BAS_READ_BATTERY_LEVEL: battery_level %d", battery_level);
                     data_uart_debug("\n\rBAS_READ_BATTERY_LEVEL: battery_level %d\r\n", battery_level);
                    bas_set_parameter(BAS_PARAM_BATTERY_LEVEL, 1, &battery_level);
                }
            }
            break;

        default:
            break;
        }
    }
#endif

    return app_result;
}

/******************************************************************
 * @fn      device_info_cb
 * @brief   device_info_cb callbacks are handled in this function.
 *
 * @param   cb_data  -  @ref prov_cb_data_t
 * @return  void
 */
void device_info_cb(uint8_t bt_addr[6], uint8_t bt_addr_type, int8_t rssi, device_info_t *pinfo)
{
    if (!dev_info_show_flag)
    {
        return;
    }
    data_uart_debug("bt addr=0x%02x%02x%02x%02x%02x%02x type=%d rssi=%d ", bt_addr[5], bt_addr[4],
                    bt_addr[3], bt_addr[2], bt_addr[1], bt_addr[0], bt_addr_type, rssi);
    switch (pinfo->type)
    {
    case DEVICE_INFO_UDB:
        data_uart_debug("udb=");
        data_uart_dump(pinfo->pbeacon_udb->dev_uuid, 16);
        break;
    case DEVICE_INFO_PROV_ADV:
        data_uart_debug("prov=");
        data_uart_dump(pinfo->pservice_data->provision.dev_uuid, 16);
        break;
    case DEVICE_INFO_PROXY_ADV:
        data_uart_debug("proxy=");
        data_uart_dump((uint8_t *)&pinfo->pservice_data->proxy, pinfo->len);
        break;
    default:
        break;
    }
}

/******************************************************************
 * @fn      prov_cb
 * @brief   Provisioning callbacks are handled in this function.
 *
 * @param   cb_data  -  @ref TProvisioningCbData
 * @return  the operation result
 */
bool prov_cb(prov_cb_type_t cb_type, prov_cb_data_t cb_data)
{
    APP_PRINT_INFO1("prov_cb: type = %d", cb_type);

    switch (cb_type)
    {
    case PROV_CB_TYPE_PB_ADV_LINK_STATE:
        switch (cb_data.pb_generic_cb_type)
        {
        case PB_GENERIC_CB_LINK_OPENED:
            data_uart_debug("PB-ADV Link Opened!\r\n>");
            break;
        case PB_GENERIC_CB_LINK_OPEN_FAILED:
            data_uart_debug("PB-ADV Link Open Failed!\r\n>");
            break;
        case PB_GENERIC_CB_LINK_CLOSED:
            data_uart_debug("PB-ADV Link Closed!\r\n>");
            break;
        default:
            break;
        }
        break;
    case PROV_CB_TYPE_UNPROV:
        data_uart_debug("unprov device!\r\n>");
        break;
    case PROV_CB_TYPE_START:
        data_uart_debug("being prov-ed!\r\n");
        break;
    case PROV_CB_TYPE_PUBLIC_KEY:
        {
            uint8_t public_key[64] = {0xf4, 0x65, 0xe4, 0x3f, 0xf2, 0x3d, 0x3f, 0x1b, 0x9d, 0xc7, 0xdf, 0xc0, 0x4d, 0xa8, 0x75, 0x81, 0x84, 0xdb, 0xc9, 0x66, 0x20, 0x47, 0x96, 0xec, 0xcf, 0x0d, 0x6c, 0xf5, 0xe1, 0x65, 0x00, 0xcc, 0x02, 0x01, 0xd0, 0x48, 0xbc, 0xbb, 0xd8, 0x99, 0xee, 0xef, 0xc4, 0x24, 0x16, 0x4e, 0x33, 0xc2, 0x01, 0xc2, 0xb0, 0x10, 0xca, 0x6b, 0x4d, 0x43, 0xa8, 0xa1, 0x55, 0xca, 0xd8, 0xec, 0xb2, 0x79};
            uint8_t private_key[32] = {0x52, 0x9a, 0xa0, 0x67, 0x0d, 0x72, 0xcd, 0x64, 0x97, 0x50, 0x2e, 0xd4, 0x73, 0x50, 0x2b, 0x03, 0x7e, 0x88, 0x03, 0xb5, 0xc6, 0x08, 0x29, 0xa5, 0xa3, 0xca, 0xa2, 0x19, 0x50, 0x55, 0x30, 0xba};
            prov_params_set(PROV_PARAMS_PUBLIC_KEY, public_key, sizeof(public_key));
            prov_params_set(PROV_PARAMS_PRIVATE_KEY, private_key, sizeof(private_key));
            APP_PRINT_INFO0("prov_cb: Please show the public key to the provisioner");
        }
        break;
    case PROV_CB_TYPE_AUTH_DATA:
        {
            prov_start_p pprov_start = cb_data.pprov_start;
            prov_auth_value_type = prov_auth_value_type_get(pprov_start);
            /* use cmd to set auth data */
            data_uart_debug("auth method=%d[nsoi] action=%d size=%d type=%d[nbNa]\r\n>",
                            pprov_start->auth_method,
                            pprov_start->auth_action, pprov_start->auth_size, prov_auth_value_type);
            //uint8_t auth_data[16] = {1};
            switch (pprov_start->auth_method)
            {
            case PROV_AUTH_METHOD_STATIC_OOB:
                //prov_auth_value_set(auth_data, sizeof(auth_data));
                APP_PRINT_INFO0("prov_cb: Please exchange the oob data with the provisioner");
                break;
            case PROV_AUTH_METHOD_OUTPUT_OOB:
                //prov_auth_value_set(auth_data, pprov_start->auth_size.output_oob_size);
                APP_PRINT_INFO2("prov_cb: Please output the oob data to the provisioner, output size = %d, action = %d",
                                pprov_start->auth_size.output_oob_size, pprov_start->auth_action.output_oob_action);
                break;
            case PROV_AUTH_METHOD_INPUT_OOB:
                //prov_auth_value_set(auth_data, pprov_start->auth_size.input_oob_size);
                APP_PRINT_INFO2("prov_cb: Please input the oob data provided by the provisioner, input size = %d, action = %d",
                                pprov_start->auth_size.input_oob_size, pprov_start->auth_action.input_oob_action);
                break;
            default:
                break;
            }
        }
        break;
    case PROV_CB_TYPE_COMPLETE:
        {
            mesh_node.iv_timer_count = MESH_IV_INDEX_48W;
            prov_data_p pprov_data = cb_data.pprov_data;
            data_uart_debug("been prov-ed with addr 0x%04x!\r\n", pprov_data->unicast_address);
        }
        break;
    case PROV_CB_TYPE_FAIL:
        data_uart_debug("provision fail, type=%d!\r\n", cb_data.prov_fail.fail_type);
        break;
    case PROV_CB_TYPE_PROV:
        /* stack ready */
        data_uart_debug("ms addr: 0x%04x\r\n>", mesh_node.unicast_addr);
        break;
    default:
        break;
    }
    return true;
}

/******************************************************************
 * @fn      fn_cb
 * @brief   fn callbacks are handled in this function.
 *
 * @param   frnd_index
 * @param   type
 * @param   fn_addr
 * @return  void
 */
void fn_cb(uint8_t frnd_index, fn_cb_type_t type, uint16_t lpn_addr)
{
    /* avoid gcc compile warning */
    (void)frnd_index;
    char *string[] = {"establishing with lpn 0x%04x\r\n", "no poll from 0x%04x\r\n", "established with lpn 0x%04x\r\n", "lpn 0x%04x lost\r\n"};
    data_uart_debug(string[type], lpn_addr);
    if (type == FN_CB_TYPE_ESTABLISH_SUCCESS || type == FN_CB_TYPE_FRND_LOST)
    {
        user_cmd_time(NULL);
    }
}

/******************************************************************
 * @fn      lpn_cb
 * @brief   lpn callbacks are handled in this function.
 *
 * @param   frnd_index
 * @param   type
 * @param   fn_addr
 * @return  void
 */
void lpn_cb(uint8_t frnd_index, lpn_cb_type_t type, uint16_t fn_addr)
{
    /* avoid gcc compile warning */
    (void)frnd_index;
    char *string[] = {"established with fn 0x%04x\r\n", "no frnd offer\r\n", "no frnd update\r\n", "fn 0x%04x lost\r\n"};
    data_uart_debug(string[type], fn_addr);
    if (type == LPN_CB_TYPE_ESTABLISH_SUCCESS || type == LPN_CB_TYPE_FRIENDSHIP_LOST)
    {
        user_cmd_time(NULL);
    }

    if (type == LPN_CB_TYPE_ESTABLISH_SUCCESS)
    {
        gap_sched_scan(false);
        lpn_disable_scan_flag = 1;
        mesh_service_adv_stop();
    }
    else if (type == LPN_CB_TYPE_FRIENDSHIP_LOST)
    {
        if (lpn_disable_scan_flag){
            gap_sched_scan(true);
            lpn_disable_scan_flag = 0;
        }
        mesh_service_adv_start();
    }
}

/******************************************************************
 * @fn      hb_cb
 * @brief   heartbeat callbacks are handled in this function.
 *
 * @param   type
 * @param   pdata
 * @return  void
 */
void hb_cb(hb_data_type_t type, void *pargs)
{
    switch (type)
    {
    case HB_DATA_PUB_TIMER_STATE:
        {
            hb_data_timer_state_t *pdata = pargs;
            if (HB_TIMER_STATE_START == pdata->state)
            {
                data_uart_debug("heartbeat publish timer start, period = %d\r\n", pdata->period);
            }
            else
            {
                data_uart_debug("heartbeat publish timer stop\r\n");
            }
        }
        break;
    case HB_DATA_SUB_TIMER_STATE:
        {
            hb_data_timer_state_t *pdata = pargs;
            if (HB_TIMER_STATE_START == pdata->state)
            {
                data_uart_debug("heartbeat subscription timer start, period = %d\r\n", pdata->period);
            }
            else
            {
                data_uart_debug("heartbeat subscription timer stop\r\n");
            }
        }
        break;
    case HB_DATA_PUB_COUNT_UPDATE:
        {
            hb_data_pub_count_update_t *pdata = pargs;
            data_uart_debug("heartbeat publish count update: %d\r\n", pdata->count);
        }
        break;
    case HB_DATA_SUB_PERIOD_UPDATE:
        {
            hb_data_sub_period_update_t *pdata = pargs;
            data_uart_debug("heartbeat subscription period update: %d\r\n", pdata->period);
        }
        break;
    case HB_DATA_SUB_RECEIVE:
        {
            hb_data_sub_receive_t *pdata = pargs;
            data_uart_debug("receive heartbeat: src = %d, init_ttl = %d, features = %d-%d-%d-%d, ttl = %d\r\n",
                            pdata->src, pdata->init_ttl, pdata->features.relay, pdata->features.proxy,
                            pdata->features.frnd, pdata->features.lpn, pdata->ttl);
        }
        break;
    default:
        break;
    }
}

void app_vendor_callback(uint8_t cb_type, void *p_cb_data)
{
    T_GAP_VENDOR_CB_DATA cb_data;
    memcpy(&cb_data, p_cb_data, sizeof(T_GAP_VENDOR_CB_DATA));
    APP_PRINT_INFO1("app_vendor_callback: command 0x%x", cb_data.p_gap_vendor_cmd_rsp->command);
    switch (cb_type)
    {
    case GAP_MSG_VENDOR_CMD_RSP:
        switch(cb_data.p_gap_vendor_cmd_rsp->command)
        { 
#if BT_VENDOR_CMD_ONE_SHOT_SUPPORT
            case HCI_LE_VENDOR_EXTENSION_FEATURE2:
                //if(cb_data.p_gap_vendor_cmd_rsp->param[0] == HCI_EXT_SUB_ONE_SHOT_ADV)
                {
                    APP_PRINT_ERROR1("One shot adv resp: cause 0x%x", cb_data.p_gap_vendor_cmd_rsp->cause);
                    gap_sched_adv_done(GAP_SCHED_ADV_END_TYPE_SUCCESS);
                }
                break;
#endif
            case HCI_LE_VENDOR_EXTENSION_FEATURE:
                switch(cb_data.p_gap_vendor_cmd_rsp->param[0])
                {
#if BT_VENDOR_CMD_ADV_TX_POWER_SUPPORT
                    case HCI_EXT_SUB_SET_ADV_TX_POWER:
                        APP_PRINT_INFO1("HCI_EXT_SUB_SET_ADV_TX_POWER: cause 0x%x", cb_data.p_gap_vendor_cmd_rsp->cause);
                        break;
#endif
#if BT_VENDOR_CMD_CONN_TX_POWER_SUPPORT
                    case HCI_EXT_SUB_SET_LINK_TX_POW:
                        APP_PRINT_INFO1("HCI_EXT_SUB_SET_LINK_TX_POW: cause 0x%x", cb_data.p_gap_vendor_cmd_rsp->cause);
                        break;
#endif
                }
                break;
            default:
                break;
        }
        break;

    default:
        break;
    }

    return;
}

#if defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL
/** @defgroup  GCS_CLIIENT_CALLBACK GCS Client Callback Event Handler
    * @brief Handle profile client callback event
    * @{
    */
void bt_mesh_central_gcs_handle_discovery_result(uint8_t conn_id, T_GCS_DISCOVERY_RESULT discov_result)
{
    uint16_t i;
    T_GCS_DISCOV_RESULT *p_result_table;
    uint16_t    properties;
    switch (discov_result.discov_type)
    {
    case GCS_ALL_PRIMARY_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_PRIMARY_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_ALL_SRV_UUID16:
                APP_PRINT_INFO4("ALL SRV UUID16[%d]: service range: 0x%x-0x%x, uuid16 0x%x",
                                i, p_result_table->result_data.srv_uuid16_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.end_group_handle,
                                p_result_table->result_data.srv_uuid16_disc_data.uuid16);
                BLE_PRINT("ALL SRV UUID16[%d]: service range: 0x%x-0x%x, uuid16 0x%x\n\r",
               				 i, p_result_table->result_data.srv_uuid16_disc_data.att_handle,
               				 p_result_table->result_data.srv_uuid16_disc_data.end_group_handle,
               				 p_result_table->result_data.srv_uuid16_disc_data.uuid16);
                break;
            case DISC_RESULT_ALL_SRV_UUID128:
                APP_PRINT_INFO4("ALL SRV UUID128[%d]: service range: 0x%x-0x%x, service=<%b>",
                                i, p_result_table->result_data.srv_uuid128_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid128_disc_data.end_group_handle,
                                TRACE_BINARY(16, p_result_table->result_data.srv_uuid128_disc_data.uuid128));
                BLE_PRINT("ALL SRV UUID128[%d]: service range: 0x%x-0x%x, service="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.srv_uuid128_disc_data.att_handle,
                                p_result_table->result_data.srv_uuid128_disc_data.end_group_handle,
                                UUID_128(p_result_table->result_data.srv_uuid128_disc_data.uuid128));

                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
				BLE_PRINT("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_BY_UUID128_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID128_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        BLE_PRINT("conn_id %d, GCS_BY_UUID128_SRV_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);

        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_SRV_DATA:
                APP_PRINT_INFO3("SRV DATA[%d]: service range: 0x%x-0x%x",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                BLE_PRINT("SRV DATA[%d]: service range: 0x%x-0x%x\n\r",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);

                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                BLE_PRINT("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_BY_UUID_SRV_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID_SRV_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        BLE_PRINT("conn_id %d, GCS_BY_UUID_SRV_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);

        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_SRV_DATA:
                APP_PRINT_INFO3("SRV DATA[%d]: service range: 0x%x-0x%x",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);
                BLE_PRINT("SRV DATA[%d]: service range: 0x%x-0x%x\n\r",
                                i, p_result_table->result_data.srv_disc_data.att_handle,
                                p_result_table->result_data.srv_disc_data.end_group_handle);

                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                BLE_PRINT("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_ALL_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        BLE_PRINT("conn_id %d, GCS_ALL_CHAR_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);

        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_CHAR_UUID16:
                properties = p_result_table->result_data.char_uuid16_disc_data.properties;
                APP_PRINT_INFO5("CHAR UUID16[%d]: decl_handle 0x%x, properties 0x%x, value_handle 0x%x, uuid16 0x%x",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\r\n",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY);
                BLE_PRINT("CHAR UUID16[%d]: decl_handle 0x%x, properties 0x%x, value_handle 0x%x, uuid16 0x%x\n\r",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                BLE_PRINT("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY);


                break;

            case DISC_RESULT_CHAR_UUID128:
                properties = p_result_table->result_data.char_uuid128_disc_data.properties;
                APP_PRINT_INFO5("CHAR UUID128[%d]:  decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_uuid128_disc_data.uuid128));
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                BLE_PRINT("CHAR UUID128[%d]:  decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                UUID_128(p_result_table->result_data.char_uuid128_disc_data.uuid128));
                BLE_PRINT("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );

                break;
            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                BLE_PRINT("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_BY_UUID_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        BLE_PRINT("conn_id %d, GCS_BY_UUID_CHAR_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);

        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_BY_UUID16_CHAR:
                properties = p_result_table->result_data.char_uuid16_disc_data.properties;
                APP_PRINT_INFO5("UUID16 CHAR[%d]: Characteristics by uuid16, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid16=<0x%x>",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                BLE_PRINT("UUID16 CHAR[%d]: Characteristics by uuid16, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid16=<0x%x>\n\r",
                                i, p_result_table->result_data.char_uuid16_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid16_disc_data.properties,
                                p_result_table->result_data.char_uuid16_disc_data.value_handle,
                                p_result_table->result_data.char_uuid16_disc_data.uuid16);
                BLE_PRINT("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );

                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                BLE_PRINT("Invalid Discovery Result Type!");
                break;
            }
        }
        break;

    case GCS_BY_UUID128_CHAR_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_BY_UUID128_CHAR_DISCOV, is_success %d",
                        conn_id, discov_result.is_success);
        BLE_PRINT("conn_id %d, GCS_BY_UUID128_CHAR_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);

        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_BY_UUID128_CHAR:
                properties = p_result_table->result_data.char_uuid128_disc_data.properties;
                APP_PRINT_INFO5("UUID128 CHAR[%d]: Characteristics by uuid128, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_uuid128_disc_data.uuid128));
                APP_PRINT_INFO5("properties:indicate %d, read %d, write cmd %d, write %d, notify %d",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );
                BLE_PRINT("UUID128 CHAR[%d]: Characteristics by uuid128, decl hndl=0x%x, prop=0x%x, value hndl=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_uuid128_disc_data.decl_handle,
                                p_result_table->result_data.char_uuid128_disc_data.properties,
                                p_result_table->result_data.char_uuid128_disc_data.value_handle,
                                UUID_128(p_result_table->result_data.char_uuid128_disc_data.uuid128));
                BLE_PRINT("properties:indicate %d, read %d, write cmd %d, write %d, notify %d\n\r",
                                properties & GATT_CHAR_PROP_INDICATE,
                                properties & GATT_CHAR_PROP_READ,
                                properties & GATT_CHAR_PROP_WRITE_NO_RSP,
                                properties & GATT_CHAR_PROP_WRITE,
                                properties & GATT_CHAR_PROP_NOTIFY
                               );

                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                BLE_PRINT("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    case GCS_ALL_CHAR_DESC_DISCOV:
        APP_PRINT_INFO2("conn_id %d, GCS_ALL_CHAR_DESC_DISCOV, is_success %d\r\n",
                        conn_id, discov_result.is_success);
        BLE_PRINT("conn_id %d, GCS_ALL_CHAR_DESC_DISCOV, is_success %d\n\r",
                        conn_id, discov_result.is_success);
        for (i = 0; i < discov_result.result_num; i++)
        {
            p_result_table = &(discov_result.p_result_table[i]);
            switch (p_result_table->result_type)
            {
            case DISC_RESULT_CHAR_DESC_UUID16:
                APP_PRINT_INFO3("DESC UUID16[%d]: Descriptors handle=0x%x, uuid16=<0x%x>",
                                i, p_result_table->result_data.char_desc_uuid16_disc_data.handle,
                                p_result_table->result_data.char_desc_uuid16_disc_data.uuid16);
                BLE_PRINT("DESC UUID16[%d]: Descriptors handle=0x%x, uuid16=<0x%x>\n\r",
                                i, p_result_table->result_data.char_desc_uuid16_disc_data.handle,
                                p_result_table->result_data.char_desc_uuid16_disc_data.uuid16);

                break;
            case DISC_RESULT_CHAR_DESC_UUID128:
                APP_PRINT_INFO3("DESC UUID128[%d]: Descriptors handle=0x%x, uuid128=<%b>",
                                i, p_result_table->result_data.char_desc_uuid128_disc_data.handle,
                                TRACE_BINARY(16, p_result_table->result_data.char_desc_uuid128_disc_data.uuid128));
                BLE_PRINT("DESC UUID128[%d]: Descriptors handle=0x%x, uuid128="UUID_128_FORMAT"\n\r",
                                i, p_result_table->result_data.char_desc_uuid128_disc_data.handle,
                                UUID_128(p_result_table->result_data.char_desc_uuid128_disc_data.uuid128));
                break;

            default:
                APP_PRINT_ERROR0("Invalid Discovery Result Type!");
                BLE_PRINT("Invalid Discovery Result Type!\n\r");
                break;
            }
        }
        break;

    default:
        APP_PRINT_ERROR2("Invalid disc type: conn_id %d, discov_type %d",
                         conn_id, discov_result.discov_type);
        BLE_PRINT("Invalid disc type: conn_id %d, discov_type %d\n\r",
                         conn_id, discov_result.discov_type);
        break;
    }
}

/**
 * @brief  Callback will be called when data sent from gcs client.
 * @param  client_id the ID distinguish which module sent the data.
 * @param  conn_id connection ID.
 * @param  p_data  pointer to data.
 * @retval   result @ref T_APP_RESULT
 */
T_APP_RESULT bt_mesh_central_gcs_client_callback(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_APP_RESULT  result = APP_RESULT_SUCCESS;
    APP_PRINT_INFO2("ble_central_gcs_client_callback: client_id %d, conn_id %d",
                    client_id, conn_id);
    if (client_id == bt_mesh_central_gcs_client_id)
    {
        T_GCS_CLIENT_CB_DATA *p_gcs_cb_data = (T_GCS_CLIENT_CB_DATA *)p_data;
        switch (p_gcs_cb_data->cb_type)
        {
        case GCS_CLIENT_CB_TYPE_DISC_RESULT:
            bt_mesh_central_gcs_handle_discovery_result(conn_id, p_gcs_cb_data->cb_content.discov_result);
            break;
        case GCS_CLIENT_CB_TYPE_READ_RESULT:
            APP_PRINT_INFO3("READ RESULT: cause 0x%x, handle 0x%x, value_len %d",
                            p_gcs_cb_data->cb_content.read_result.cause,
                            p_gcs_cb_data->cb_content.read_result.handle,
                            p_gcs_cb_data->cb_content.read_result.value_size);
            data_uart_debug("READ RESULT: cause 0x%x, handle 0x%x, value_len %d\n\r",
                            p_gcs_cb_data->cb_content.read_result.cause,
                            p_gcs_cb_data->cb_content.read_result.handle,
                            p_gcs_cb_data->cb_content.read_result.value_size);

            if (p_gcs_cb_data->cb_content.read_result.cause == GAP_SUCCESS)
            {
                APP_PRINT_INFO1("READ VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.read_result.value_size,
                                             p_gcs_cb_data->cb_content.read_result.p_value));
                data_uart_debug("READ VALUE: ");
                for(int i=0; i< p_gcs_cb_data->cb_content.read_result.value_size; i++)
                    data_uart_debug("0x%2x ", *(p_gcs_cb_data->cb_content.read_result.p_value + i));
                data_uart_debug("\n\r");
            }
            break;
        case GCS_CLIENT_CB_TYPE_WRITE_RESULT:
            APP_PRINT_INFO3("WRITE RESULT: cause 0x%x, handle 0x%x, type %d",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);
            data_uart_debug("WRITE RESULT: cause 0x%x, handle 0x%x, type %d\n\r",
                            p_gcs_cb_data->cb_content.write_result.cause,
                            p_gcs_cb_data->cb_content.write_result.handle,
                            p_gcs_cb_data->cb_content.write_result.type);
            break;
        case GCS_CLIENT_CB_TYPE_NOTIF_IND:
            if (p_gcs_cb_data->cb_content.notif_ind.notify == false)
            {
                APP_PRINT_INFO2("INDICATION: handle 0x%x, value_size %d",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                APP_PRINT_INFO1("INDICATION VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
                data_uart_debug("INDICATION: handle 0x%x, value_size %d\r\n",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                data_uart_debug("INDICATION VALUE: ");
                for (int i = 0; i < p_gcs_cb_data->cb_content.notif_ind.value_size; i++) {
                    data_uart_debug("0x%2x ", *(p_gcs_cb_data->cb_content.notif_ind.p_value+ i));
                }
                data_uart_debug("\n\r");
            }
            else
            {
                APP_PRINT_INFO2("NOTIFICATION: handle 0x%x, value_size %d",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                APP_PRINT_INFO1("NOTIFICATION VALUE: %b",
                                TRACE_BINARY(p_gcs_cb_data->cb_content.notif_ind.value_size,
                                             p_gcs_cb_data->cb_content.notif_ind.p_value));
                data_uart_debug("NOTIFICATION: handle 0x%x, value_size %d\r\n",
                                p_gcs_cb_data->cb_content.notif_ind.handle,
                                p_gcs_cb_data->cb_content.notif_ind.value_size);
                data_uart_debug("NOTIFICATION VALUE: ");
                for (int i = 0; i < p_gcs_cb_data->cb_content.notif_ind.value_size; i++) {
                    data_uart_debug("0x%2x ", *(p_gcs_cb_data->cb_content.notif_ind.p_value+ i));
                }
                data_uart_debug("\n\r");
            }
            break;
        default:
            break;
        }
    }

    return result;
}
#endif
