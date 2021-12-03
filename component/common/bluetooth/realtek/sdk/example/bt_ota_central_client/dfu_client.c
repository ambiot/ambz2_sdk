/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    dfu_client.c
  * @brief   dfu service client source file.
  * @details
  * @author  bill
  * @date    2017-8-28
  * @version v1.0
  ******************************************************************************
  * @attention
  * <h2><center>&copy; COPYRIGHT 2015 Realtek Semiconductor Corporation</center></h2>
  ******************************************************************************
  */

/** Add Includes here **/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT) && CONFIG_BT_OTA_CENTRAL_CLIENT
#include <string.h>
#include <stdbool.h>
#include "gap_conn_le.h"
#include "gap_scan.h"
#include "patch_header_check.h"
#include "mem_types.h"
#include "os_mem.h"
#include "trace.h"
#include "trace_app.h"
#include "dfu_client.h"
#include "crypto_api.h"
#include "device_lock.h"
#include "data_uart.h"
#include "platform_stdlib.h"
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
#include <insert_write.h>
#endif

#define DFU_HEADER_SIZE                 12
#define IMG_HEADER_SIZE                 1024

#define DFU_CLIENT_INVALID_CONN_ID      0xff

#define GATT_UUID128_DFU_DATA           0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0x87, 0x63, 0x00, 0x00
#define  GATT_UUID128_DFU_CONTROL_POINT 0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0x87, 0x64, 0x00, 0x00
#define GATT_UUID128_DFU_SERVICE        0x12, 0xA2, 0x4D, 0x2E, 0xFE, 0x14, 0x48, 0x8e, 0x93, 0xD2, 0x17, 0x3C, 0x87, 0x62, 0x00, 0x00


//convert img version bit map to deirectly copmare version value
#define IMG_VER_TO_LITTLE_ENDIAN(p)       (((p & 0xf) << 28 )+ (((p & 0xff0) >> 4) << 20) + (((p & 0x07fff000) >> 12) << 5) + ((p & 0xf8000000) >> 27))

#define MAJOR_IMG_VER(p)        (p & 0xf)
#define MINOR_IMG_VER(p)        ((p & 0xff0) >> 4)
#define REVISION_IMG_VER(p)     ((p & 0x07fff000) >> 12)
#define RESERVE_IMG_VER(p)      ((p & 0xf8000000) >> 27)

#undef  MIN
#define MIN(a, b)    ((a) > (b) ? (b) : (a))

/* PUT external 2 CHARS from local SHORT, Big-Endian format STANDARD NETWORK BYTE ORDER */
#define BE_WORD2EXTRN(p,w)                 \
    {*((p)+1) = (unsigned char)((w) & 0xff);         \
        *(p)     = /*lint -e(572,778)*/ (unsigned char)(((w)>>8) & 0xff);}

/* Get local WORD from external 2 BYTE, Little-Endian format */
#define LE_EXTRN2WORD(p) (((*(p)) & 0xff) + ((*((p)+1)) << 8))

/* Get local DWORD from external 4 BYTE, Little-Endian format  */
#define LE_EXTRN2DWORD(p) (((unsigned long)(*((p)+0)) & 0xff) + ((unsigned long)(*((p)+1)) << 8) \
                           + ((unsigned long)(*((p)+2)) << 16)  + ((unsigned long)(*((p)+3)) << 24))

/* PUT external 2 BYTE from local WORD, Little-Endian Format */
#define LE_WORD2EXTRN(p,w)                 \
    {*((unsigned char*)p)     = (unsigned char)((unsigned short)(w) & 0xff);      \
        *(((unsigned char*)p)+1) = /*lint -e(572,778)*/ (unsigned char)(((unsigned short)(w)>>8) & 0xff);}


const uint8_t dfu_data_uuid[16] = {GATT_UUID128_DFU_DATA};
const uint8_t dfu_control_point_uuid[16] = {GATT_UUID128_DFU_CONTROL_POINT};
const uint8_t dfu_service_uuid[16] = {GATT_UUID128_DFU_SERVICE};

T_DFU_CLIENT_FSM_WRITE g_dfu_fsm_write = DFU_FSM_WRITE_DO_NOT_CARE;

/*when ota target enable buffer check*/
uint16_t g_buf_check_pkt_num;
uint16_t g_buf_check_pkt_count;
uint16_t g_buf_check_crc;

/* for record data that target send by notification*/
T_DFU_CTX g_dfu_ctx;

uint8_t g_cp_msg[20];  //for cache control point masg
uint8_t g_cp_msg_len;

extern uint8_t* bt_ota_central_client_get_image(void);
extern uint8_t* bt_ota_central_client_get_key(void);

/********************************************************************************************************
* local static variables defined here, only used in this source file.
********************************************************************************************************/
/**
 * @brief  dfu client Link control block definition.
 */
typedef struct
{
    T_DFU_DISC_STATE disc_state;
    uint16_t         hdl_cache[HDL_DFU_CACHE_LEN];
} T_DFU_LINK, *P_DFU_LINK;


static P_DFU_LINK dfu_table;
static uint8_t dfu_link_num;

/**<  Callback used to send data to app from Dfu client layer. */
static P_FUN_GENERAL_APP_CB pf_dfu_client_cb = NULL;
static P_FUN_GENERAL_APP_CB pf_dfu_client_app_info_cb = NULL;

/**<  Dfu client ID. */
static T_CLIENT_ID dfu_client = CLIENT_PROFILE_GENERAL_ID;

/**<  Dfu discovery end handle control. */
static T_DFU_HANDLE_TYPE dfu_end_hdl_idx = HDL_DFU_CACHE_LEN;

/**<  Discovery State indicate which CCCD is in discovery. */
static T_DFU_CCCD_DISC_STATE dfu_cccd_disc_state = DFU_CCCD_DISC_START;

static void dfu_client_init(void);

/**
  * @brief  Used by application, to set the handles in Dfu handle cache.
  * @param  handle_type: handle types of this specific profile.
  * @param  handle_value: handle value to set.
  * @retval true--set success.
  *         flase--set failed.
  */
bool dfu_client_set_hdl_cache(uint8_t conn_id, uint16_t *p_hdl_cache, uint8_t len)
{
    if (conn_id >= dfu_link_num)
    {
        PROFILE_PRINT_ERROR1("dfu_client_set_hdl_cache: failed invalid conn_id %d", conn_id);
        return false;
    }
    if (dfu_table[conn_id].disc_state != DISC_DFU_IDLE)
    {
        PROFILE_PRINT_ERROR1("dfu_client_set_hdl_cache: failed invalid state %d",
                             dfu_table[conn_id].disc_state);
        return false;
    }
    if (len != sizeof(uint16_t) * HDL_DFU_CACHE_LEN)
    {
        PROFILE_PRINT_ERROR1("dfu_client_set_hdl_cache: failed invalid len %d", len);
        return false;
    }
    memcpy(dfu_table[conn_id].hdl_cache, p_hdl_cache, len);
    dfu_table[conn_id].disc_state = DISC_DFU_DONE;
    return true;
}

/**
  * @brief  Used by application, to get handle cache.
  * @param[in]  conn_id connection ID.
  * @param[in]  p_hdl_cache pointer of the handle cache table
  * @param[in]  len the length of handle cache table
  * @retval true success.
  * @retval false failed.
  */
bool dfu_client_get_hdl_cache(uint8_t conn_id, uint16_t *p_hdl_cache, uint8_t len)
{
    if (conn_id >= dfu_link_num)
    {
        PROFILE_PRINT_ERROR1("dfu_client_get_hdl_cache: failed invalid conn_id %d", conn_id);
        return false;
    }
    if (dfu_table[conn_id].disc_state != DISC_DFU_DONE)
    {
        PROFILE_PRINT_ERROR1("dfu_client_get_hdl_cache: failed invalid state %d",
                             dfu_table[conn_id].disc_state);
        return false;
    }
    if (len != sizeof(uint16_t) * HDL_DFU_CACHE_LEN)
    {
        PROFILE_PRINT_ERROR1("dfu_client_get_hdl_cache: failed invalid len %d", len);
        return false;
    }
    memcpy(p_hdl_cache, dfu_table[conn_id].hdl_cache, len);
    return true;
}


/**
  * @brief  Used by application, to start the discovery procedure of Dfu server.
  * @retval true--send request to upper stack success.
  *         flase--send request to upper stack failed.
  */
bool dfu_client_start_discovery(uint8_t conn_id)
{
    if (conn_id >= dfu_link_num)
    {
        PROFILE_PRINT_ERROR1("dfu_client_start_discovery: failed invalid conn_id %d", conn_id);
        return false;
    }

    APP_PRINT_INFO0("dfu_client_start_discovery");
	data_uart_print("dfu_client_start_discovery\n\r");
    /* First clear handle cache. */
    memset(&dfu_table[conn_id], 0, sizeof(T_DFU_LINK));
    dfu_table[conn_id].disc_state = DISC_DFU_START;
    if (client_by_uuid128_srv_discovery(conn_id, dfu_client,
                                        (uint8_t *)dfu_service_uuid) == GAP_CAUSE_SUCCESS)
    {
        return true;
    }
    return false;
}

/**
  * @brief  Used internal, start the discovery of Dfu characteristics.
  *         NOTE--user can offer this interface for application if required.
  * @retval true--send request to upper stack success.
  *         flase--send request to upper stack failed.
  */
static bool dfu_client_start_all_char_discovery(uint8_t conn_id)
{
    uint16_t start_handle;
    uint16_t end_handle;

    PROFILE_PRINT_INFO0("dfu_client_start_all_char_discovery");
    start_handle = dfu_table[conn_id].hdl_cache[HDL_DFU_SRV_START];
    end_handle = dfu_table[conn_id].hdl_cache[HDL_DFU_SRV_END];
    return (client_all_char_discovery(conn_id, dfu_client, start_handle,
                                      end_handle) == GAP_CAUSE_SUCCESS);
}

/**
  * @brief  Used internal, start the discovery of Dfu characteristics descriptor.
  *         NOTE--user can offer this interface for application if required.
  * @retval true--send request to upper stack success.
  *         flase--send request to upper stack failed.
  */
static bool dfu_client_start_all_char_descriptor_discovery(uint8_t conn_id, uint16_t start_handle,
                                                           uint16_t end_handle)
{
    DFU_PRINT_INFO0("dfu_client_start_all_char_descriptor_discovery");
    return (client_all_char_descriptor_discovery(conn_id, dfu_client, start_handle,
                                                 end_handle) == GAP_CAUSE_SUCCESS);
}

/**
  * @brief  Used by application, read data from server by using handles.
  * @param  readCharType: one of characteristic that has the readable property.
  * @retval true--send request to upper stack success.
  *         flase--send request to upper stack failed.
  */
bool dfu_client_read_by_handle(uint8_t conn_id, T_DFU_READ_TYPE type)
{
    bool hdl_valid = false;
    uint16_t handle = 0;

    if (conn_id >= dfu_link_num)
    {
        PROFILE_PRINT_ERROR1("dfu_client_read_by_handle: failed invalid conn_id %d", conn_id);
        return false;
    }
    switch (type)
    {
    case DFU_READ_CP_CCCD:
        if (dfu_table[conn_id].hdl_cache[DFU_READ_CP_CCCD])
        {
            handle = dfu_table[conn_id].hdl_cache[DFU_READ_CP_CCCD];
            hdl_valid = true;
        }
        break;
    default:
        return false;
    }

    if (hdl_valid)
    {
        if (client_attr_read(conn_id, dfu_client, handle) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    DFU_PRINT_INFO0("dfu_client_read_by_handle: Request fail! Please check!");
    return false;
}

/**
  * @brief  Used by application, to enable or disable the notification of peer server's V3 Notify Characteristic.
  * @param  command: 0--disable the notification, 1--enable the notification.
  * @retval true--send request to upper stack success.
  *         flase--send request to upper stack failed.
  */
bool dfu_client_cp_cccd_set(uint8_t conn_id, bool command)
{
    uint16_t handle;
    uint16_t length;
    uint8_t *pdata;
    uint16_t cccd_value;
    handle = dfu_table[conn_id].hdl_cache[HDL_DFU_CP_CCCD];
    if (handle)
    {
        cccd_value = command ? 1 : 0;
        length = sizeof(uint16_t);
        pdata = (uint8_t *)&cccd_value;
        if (client_attr_write(conn_id, dfu_client, GATT_WRITE_TYPE_REQ, handle, length,
                              pdata) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }

    DFU_PRINT_INFO0("dfu_client_cp_cccd_set: Request fail! Please check!");
    return false;
}

/**
  * @brief  Used internal, to send write request to peer server's V5 Control Point Characteristic.
  * @param  ctl_pnt_ptr: pointer of control point data to write.
  * @retval true--send request to upper stack success.
  *         flase--send request to upper stack failed.
  */
bool dfu_client_cp_write(uint8_t conn_id, uint8_t *pdata, uint16_t length)
{
    uint16_t handle;
    DFU_PRINT_INFO2("==>dfu_client_cp_write: opcode = %d, datalen = %d.", pdata[0],
                    length); //opcode = pdata[0]
    g_dfu_fsm_write = DFU_FSM_WRITE_WAIT_WRITE_RESP;
    handle = dfu_table[conn_id].hdl_cache[HDL_DFU_CP];
    if (handle)
    {
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT	
        if(if_queue_in(0, conn_id, dfu_client, handle, length, pdata) != 0)
        {
            printf("(conn_id %d)no dfu write request send\r\n", conn_id);
            return false;
        }

        request_in_process_flag[conn_id] = 1;
        if (client_attr_write(conn_id, dfu_client, GATT_WRITE_TYPE_REQ, handle, length, pdata) == GAP_CAUSE_SUCCESS)
            return true;
        else
            request_in_process_flag[conn_id] = 0; //if write request fail then reset flag=0

#else
        if (client_attr_write(conn_id, dfu_client, GATT_WRITE_TYPE_REQ, handle, length, pdata) == GAP_CAUSE_SUCCESS)
            return true;
#endif
    }
    DFU_PRINT_INFO0("==>dfu_client_cp_write: Request fail! Please check!");
    printf("==>dfu_client_cp_write: Request fail! Please check!\r\n");
    return false;
}

/**
  * @brief  Used internal, to send write request to peer server's V5 Control Point Characteristic.
  * @param  ctl_pnt_ptr: pointer of control point data to write.
  * @retval true--send request to upper stack success.
  *         flase--send request to upper stack failed.
  */
bool dfu_client_data_write(uint8_t conn_id, uint8_t *pdata, uint16_t length)
{
    uint16_t handle;
    g_dfu_fsm_write = DFU_FSM_WRITE_DFU_DATA;
    handle = dfu_table[conn_id].hdl_cache[HDL_DFU_DATA];
    DFU_PRINT_INFO2("==>dfu_client_data_write: datalen = %d, conn_id=%d", length, conn_id);
    if (handle)
    {
        if (client_attr_write(conn_id, dfu_client, GATT_WRITE_TYPE_CMD, handle, length,
                              pdata) == GAP_CAUSE_SUCCESS)
        {
            return true;
        }
    }
    DFU_PRINT_INFO0("dfu_client_data_write: command fail! Please check!");
    return false;
}

/**
  * @brief  Used internal, switch to the next CCCD handle to be discovered.
  *         NOTE--this function only used when peer service has more than one CCCD.
  * @param  pSwitchState: CCCD discovery state.
  * @retval none.
  */
static void dfu_client_switch_next_descriptor(uint8_t conn_id, T_DFU_CCCD_DISC_STATE *cccd_state)
{
    T_DFU_CCCD_DISC_STATE new_state;
    switch (*cccd_state)
    {
    case DFU_CCCD_DISC_START:
        if (dfu_table[conn_id].hdl_cache[HDL_DFU_CP_END])
        {
            new_state = DFU_CCCD_DISC_CP_NOTIFY;
        }
        else
        {
            new_state = DFU_CCCD_DISC_END;
        }
        break;
    case DFU_CCCD_DISC_CP_NOTIFY:
        new_state = DFU_CCCD_DISC_END;
        break;
    default:
        new_state = DFU_CCCD_DISC_END;
        break;
    }

    *cccd_state = new_state;
}

/**
  * @brief  Called by profile client layer, when discover state of discovery procedure changed.
  * @param  discoveryState: current service discovery state.
  * @retval ProfileResult_Success--procedure OK.
  *         other--procedure exception.
  */
static void dfu_client_disc_state_cb(uint8_t conn_id, T_DISCOVERY_STATE disc_state)
{
    bool cb_flag = false;
    bool descriptor_disc_flag = false;

    uint16_t start_handle;
    uint16_t end_handle;

    T_DFU_CLIENT_CB_DATA cb_data;
    cb_data.cb_type = DFU_CLIENT_CB_TYPE_DISC_STATE;

    DFU_PRINT_INFO1("dfu_client_disc_state_cb: disc_state = %d", disc_state);
	printf("dfu_client_disc_state_cb: disc_state = %d\n", disc_state);
    if (dfu_table[conn_id].disc_state == DISC_DFU_START)
    {
        switch (disc_state)
        {
        case DISC_STATE_SRV_DONE:
            /* Indicate that service handle found. Start discover characteristic. */
            if ((dfu_table[conn_id].hdl_cache[HDL_DFU_SRV_START] != 0)
                || (dfu_table[conn_id].hdl_cache[HDL_DFU_SRV_END] != 0))
            {
                if (dfu_client_start_all_char_discovery(conn_id) == false)
                {
                    cb_data.cb_content.disc_state = DISC_DFU_FAIL;
                    cb_flag = true;
                }
            }
            /* No dfu service handle found. Discover procedure complete. */
            else
            {
                cb_data.cb_content.disc_state = DISC_DFU_FAIL;
                cb_flag = true;
            }
            break;
        case DISC_STATE_CHAR_DONE:
            /* We should store the last char end handle if needed. */
            if (dfu_end_hdl_idx < HDL_DFU_CACHE_LEN)
            {
                dfu_table[conn_id].hdl_cache[dfu_end_hdl_idx] = dfu_table[conn_id].hdl_cache[HDL_DFU_SRV_END];
                dfu_end_hdl_idx = HDL_DFU_CACHE_LEN;
            }
            /* Find the first descriptor to be discovered. */
            dfu_cccd_disc_state = DFU_CCCD_DISC_START;
            dfu_client_switch_next_descriptor(conn_id, &dfu_cccd_disc_state);
            break;
        case DISC_STATE_CHAR_DESCRIPTOR_DONE:
            /* Find the next descriptor to be discovered. */
            dfu_client_switch_next_descriptor(conn_id, &dfu_cccd_disc_state);
            break;
        default:
            DFU_PRINT_INFO0("dfu_client_disc_state_cb: Invalid Discovery State!");
            break;
        }
    }

    /* Switch different char descriptor discovery, if has multi char descriptors. */
    switch (dfu_cccd_disc_state)
    {
    case DFU_CCCD_DISC_CP_NOTIFY:
        /* Need to discover notify char descriptor. */
        start_handle = dfu_table[conn_id].hdl_cache[HDL_DFU_CP] + 1;
        end_handle = dfu_table[conn_id].hdl_cache[HDL_DFU_CP_END];
        descriptor_disc_flag = true;
        break;
    case DFU_CCCD_DISC_END:
        cb_data.cb_content.disc_state = DISC_DFU_DONE;
        cb_flag = true;
        break;
    default:
        /* No need to send char descriptor discovery. */
        break;
    }
    if (descriptor_disc_flag)
    {
        if (dfu_client_start_all_char_descriptor_discovery(conn_id, start_handle, end_handle) == false)
        {
            cb_data.cb_content.disc_state = DISC_DFU_FAIL;
            cb_flag = true;
        }
    }

    /* Send discover state to application if needed. */
    if (cb_flag && pf_dfu_client_cb)
    {
        (*pf_dfu_client_cb)(dfu_client, conn_id, &cb_data);
    }
}

/**
  * @brief  Called by profile client layer, when discover result fetched.
  * @param  resultType: indicate which type of value discovered in service discovery procedure.
  * @param  resultData: value discovered.
  * @retval ProfileResult_Success--procedure OK.
  *         other--procedure exception.
  */
static void dfu_client_disc_result_cb(uint8_t conn_id, T_DISCOVERY_RESULT_TYPE result_type,
                                      T_DISCOVERY_RESULT_DATA result_data)
{
    DFU_PRINT_INFO1("==>dfu_client_disc_result_cb: result_type = %d", result_type);
	printf("==>dfu_client_disc_result_cb: result_type = %d\n", result_type);
    switch (result_type)
    {
    case DISC_RESULT_SRV_DATA:
        /* send service handle range to application. */
        dfu_table[conn_id].hdl_cache[HDL_DFU_SRV_START] = result_data.p_srv_disc_data->att_handle;
        dfu_table[conn_id].hdl_cache[HDL_DFU_SRV_END] = result_data.p_srv_disc_data->end_group_handle;
        break;
    case DISC_RESULT_CHAR_UUID128:
        /* When use clientAPI_AllCharDiscovery. */
        if (dfu_end_hdl_idx < HDL_DFU_CACHE_LEN)
        {
            DFU_PRINT_INFO2("==>dfu_client_disc_result_cb: dfu_end_hdl_idx = %d, hnadle=0x%4x",
                            dfu_end_hdl_idx, result_data.p_char_uuid128_disc_data->decl_handle - 1);
            dfu_table[conn_id].hdl_cache[dfu_end_hdl_idx] = result_data.p_char_uuid128_disc_data->decl_handle -
                                                            1;
            dfu_end_hdl_idx = HDL_DFU_CACHE_LEN;

        }
        /* we should inform intrested handles to upper application. */
        if (0 == memcmp(result_data.p_char_uuid128_disc_data->uuid128, dfu_data_uuid, 16))
        {
            DFU_PRINT_INFO1("==>dfu_client_disc_result_cb: set dfu_data handle=0x%4x",
                            result_data.p_char_uuid128_disc_data->value_handle);
            dfu_table[conn_id].hdl_cache[HDL_DFU_DATA] = result_data.p_char_uuid128_disc_data->value_handle;
        }
        else if (0 == memcmp(result_data.p_char_uuid128_disc_data->uuid128, dfu_control_point_uuid, 16))
        {
            DFU_PRINT_INFO1("==>dfu_client_disc_result_cb: set dfu_control_point handle=0x%4x",
                            result_data.p_char_uuid128_disc_data->value_handle);
            dfu_table[conn_id].hdl_cache[HDL_DFU_CP] = result_data.p_char_uuid128_disc_data->value_handle;
            dfu_end_hdl_idx = HDL_DFU_CP_END;
        }
        break;
    case DISC_RESULT_CHAR_DESC_UUID16:
        /* When use clientAPI_AllCharDescriptorDiscovery. */
        if (result_data.p_char_desc_uuid16_disc_data->uuid16 ==
            GATT_UUID_CHAR_CLIENT_CONFIG)  //GATT Client Characteristic Configuration descriptors uuid
        {
            uint16_t temp_handle = result_data.p_char_desc_uuid16_disc_data->handle;
            if ((temp_handle > dfu_table[conn_id].hdl_cache[HDL_DFU_CP])
                && (temp_handle <= dfu_table[conn_id].hdl_cache[HDL_DFU_CP_END]))
            {
                DFU_PRINT_INFO1("==>dfu_client_disc_result_cb: set dfu_cccd handle=0x%4x", temp_handle);
                dfu_table[conn_id].hdl_cache[HDL_DFU_CP_CCCD] = temp_handle;
            }
            else
            {
                /* have no intrest in this handle. */
            }
        }
        break;
    default:
        DFU_PRINT_INFO0("dfu_client_disc_result_cb: Invalid Discovery Result Type!");
        break;
    }
}

/**
  * @brief  Called by profile client layer, when read request responsed.
  * @param  reqResult: read request from peer device success or not.
  * @param  wHandle: handle of the value in read response.
  * @param  iValueSize: size of the value in read response.
  * @param  pValue: pointer to the value in read response.
  * @retval ProfileResult_Success--procedure OK.
  *         other--procedure exception.
  */
static void dfu_client_read_result_cb(uint8_t conn_id,  uint16_t cause, uint16_t handle,
                                      uint16_t len, uint8_t *pvalue)
{
    T_DFU_CLIENT_CB_DATA cb_data;
    cb_data.cb_type = DFU_CLIENT_CB_TYPE_READ_RESULT;
    cb_data.cb_content.read_result.cause = cause;
    DFU_PRINT_INFO3("==>dfu_client_read_result_cb: cause=%d, handle=0x%x, size=%d", cause,
                    handle, len);
	printf("==>dfu_client_read_result_cb: cause=%d, handle=0x%x, size=%d\n", cause,
                    handle, len);
    /* If read req success, branch to fetch value and send to application. */
    if (handle == dfu_table[conn_id].hdl_cache[HDL_DFU_CP_CCCD])
    {
        cb_data.cb_content.read_result.type = DFU_READ_CP_CCCD;
        if (cause == GAP_SUCCESS && len == sizeof(uint16_t))
        {
            uint16_t cccd_value = LE_EXTRN2WORD(pvalue);
            cb_data.cb_content.read_result.data.dfu_cp_cccd = cccd_value &
                                                              GATT_CLIENT_CHAR_CONFIG_NOTIFY;
        }
    }

    /* Inform application the read result. */
    if (pf_dfu_client_cb)
    {
        (*pf_dfu_client_cb)(dfu_client, conn_id, &cb_data);
    }
}

/**
  * @brief  Called by profile client layer, when write request complete.
  * @param  reqResult: write request send success or not.
  * @retval ProfileResult_Success--procedure OK.
  *         other--procedure exception.
  */
static void dfu_client_write_result_cb(uint8_t conn_id, T_GATT_WRITE_TYPE type, uint16_t handle,
                                       uint16_t cause, uint8_t credits)
{
    T_DFU_CLIENT_CB_DATA cb_data;
    cb_data.cb_type = DFU_CLIENT_CB_TYPE_WRITE_RESULT;
    cb_data.cb_content.write_result.cause = cause;

    DFU_PRINT_INFO4("==>dfu_client_write_result_cb: cause=%d, handle=0x%x, type=%d, credits=%d",
                    cause, handle, type, credits);
	printf("==>dfu_client_write_result_cb: cause=%d, handle=0x%x, type=%d, credits=%d\n",
                    cause, handle, type, credits);
    /* If write req success, branch to fetch value and send to application. */
    if (handle == dfu_table[conn_id].hdl_cache[HDL_DFU_CP_CCCD])

    {
        cb_data.cb_content.write_result.type = DFU_WRITE_CP_CCCD;
    }
    else if (handle == dfu_table[conn_id].hdl_cache[HDL_DFU_CP])
    {
        cb_data.cb_content.write_result.type = DFU_WRITE_CP;
    }
    else if (handle == dfu_table[conn_id].hdl_cache[HDL_DFU_DATA])
    {
        cb_data.cb_content.write_result.type = DFU_WRITE_DATA;
    }
    else
    {
        return;
    }

    /* Inform application the write result. */
    if (pf_dfu_client_cb)
    {
        (*pf_dfu_client_cb)(dfu_client, conn_id, &cb_data);
    }
}

/**
  * @brief  Called by profile client layer, when notification or indication arrived.
  * @param  wHandle: handle of the value in received data.
  * @param  iValueSize: size of the value in received data.
  * @param  pValue: pointer to the value in received data.
  * @retval ProfileResult_Success--procedure OK.
  *         other--procedure exception.
  */
static T_APP_RESULT dfu_client_notify_indicate_cb(uint8_t conn_id, bool notify,
                                                  uint16_t handle, uint16_t len, uint8_t *pvalue)
{
    T_APP_RESULT app_result = APP_RESULT_REJECT;
    T_DFU_CLIENT_CB_DATA cb_data;

    DFU_PRINT_INFO3("==>dfu_client_notify_indicate_cb: handle=0x%x, len=%d, is notify=%d",
                    handle, len, notify);
	printf("==>dfu_client_notify_indicate_cb: handle=0x%x, len=%d, is notify=%d\n",
                    handle, len, notify);
    if (handle == dfu_table[conn_id].hdl_cache[HDL_DFU_CP])
    {
        cb_data.cb_type = DFU_CLIENT_CB_TYPE_NOTIF_IND_RESULT;
        cb_data.cb_content.notif_ind_data.type = DFU_NOTIF_RECEIVE_DFU_CP_NOTIFY;
        cb_data.cb_content.notif_ind_data.value.len = len;
        cb_data.cb_content.notif_ind_data.value.pdata = pvalue;

        /* Inform application the notif/ind result. */
        if (pf_dfu_client_cb)
        {
            (*pf_dfu_client_cb)(dfu_client, conn_id, &cb_data);
        }
        app_result = APP_RESULT_SUCCESS;
    }

    return app_result;
}

/**
  * @brief  Called by profile client layer, when link disconnected.
  *         NOTE--we should reset some state when disconnected.
  * @retval ProfileResult_Success--procedure OK.
  *         other--procedure exception.
  */
void dfu_client_disconnect_cb(uint8_t conn_id)
{
    DFU_PRINT_INFO1("dfu_client_disconnect_cb: conn_id=%d", conn_id);

    dfu_end_hdl_idx = HDL_DFU_CACHE_LEN;
    dfu_cccd_disc_state = DFU_CCCD_DISC_START;
    dfu_client_init();

    if (g_dfu_ctx.fsm_client == DFU_CB_START)
    {
        g_dfu_ctx.fsm_client = DFU_CB_NONE;
        if (pf_dfu_client_app_info_cb)
        {
            T_DFU_CB_MSG cb_msg;
            cb_msg.type = DFU_CB_FAIL;
            pf_dfu_client_app_info_cb(dfu_client, conn_id, &cb_msg);
        }
    }
}


static void swap_buf(uint8_t *dst, const uint8_t *src, uint16_t len)
{
    int i;
    if (dst == NULL || src == NULL)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        dst[len - 1 - i] = src[i];
    }
}

unsigned char bt_ota_central_client_tmp_image[16] __attribute__((aligned(32))) = 
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

unsigned char bt_ota_central_client_tmp_encrypted[16] __attribute__((aligned(32))) = 
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

unsigned char bt_ota_central_client_tmp_key[32] __attribute__((aligned(32))) = 
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

bool dfu_encrypt(uint8_t image[16])
{
    int ret = false;

#if defined(CONFIG_PLATFORM_8710C)
	const u8 *key;

	swap_buf(bt_ota_central_client_tmp_key, bt_ota_central_client_get_key(), 32);
	key = bt_ota_central_client_tmp_key;

	memcpy(bt_ota_central_client_tmp_image, image, 16);
	
	device_mutex_lock(RT_DEV_LOCK_CRYPTO);
	ret = crypto_aes_ecb_init(key, 32);
	if(ret != 0){
		printf("crypto_aes_ecb_init: ret=%d\n", ret);
		return ret;
	}
	ret = crypto_aes_ecb_encrypt(bt_ota_central_client_tmp_image, 16, NULL, 0, bt_ota_central_client_tmp_encrypted);
	if(ret !=0){
		printf("crypto_aes_ecb_encrypt: ret=%d\n", ret);
		return ret;
	}
	device_mutex_unlock(RT_DEV_LOCK_CRYPTO);

	memcpy(image, bt_ota_central_client_tmp_encrypted, 16);	
#endif

    return ret;
}

uint16_t dfu_client_crc_cal(uint8_t *buf, uint32_t length, uint16_t checksum16)
{
    uint32_t i;
    uint16_t *p16;
    p16 = (uint16_t *)buf;
    for (i = 0; i < length / 2; ++i)
    {
        checksum16 = checksum16 ^ (*p16);
        ++p16;
    }
    return checksum16;
}

bool dfu_client_push_image(uint8_t conn_id)
{
    uint16_t mtu_size = 0;
    uint16_t push_img_size = 0;
    uint32_t image_size = 0;
    uint8_t *pheader;
    uint8_t *pdata;

    DFU_PRINT_INFO0("==>dfu_client_push_image");
    if (g_dfu_fsm_write == DFU_FSM_WRITE_WAIT_WRITE_RESP)
    {
        // in case that the credit is bigger then the buffer check packet num
        DFU_PRINT_INFO0("==>dfu_client_push_image: current waiting cp response");
        return false;
    }

#if 0
    mtu_size = 20;
#else
    le_get_conn_param(GAP_PARAM_CONN_MTU_SIZE, &mtu_size, 0);
    DFU_PRINT_INFO1("==>dfu_client_push_image: mtu_size=%d", mtu_size);
    mtu_size -= 3;
    mtu_size &= 0xfff0;  //mtu size down align 16 bytes
#endif

    image_size = g_dfu_ctx.image_length - g_dfu_ctx.curr_offset;
    push_img_size = MIN(image_size, mtu_size);
    DFU_PRINT_INFO2("==>dfu_client_push_image: push_img_size=%d, img_size=%d", push_img_size,
                    image_size);

#if (SILENT_OTA == 0)
    if (!g_dfu_ctx.mode.buffercheck)
    {
        image_size = NORMAL_OTA_MAX_BUFFER_SIZE - (g_dfu_ctx.curr_offset % NORMAL_OTA_MAX_BUFFER_SIZE);
        push_img_size = MIN(image_size, push_img_size);
        DFU_PRINT_INFO2("==>dfu_client_push_image: push_img_size=%d, img_size=%d", push_img_size,
                        image_size);
    }

#endif

    DFU_PRINT_INFO3("==>dfu_client_push_image: image_length=%d, g_dfu_ctx.curr_offset=%d, push_img_size=%d",
                    g_dfu_ctx.image_length, g_dfu_ctx.curr_offset, push_img_size);

    //pheader = (uint8_t *)TEMP_OTA_IMG_ADDR;
	pheader = bt_ota_central_client_get_image();
    pdata = os_mem_alloc(RAM_TYPE_DATA_ON, push_img_size);  //send one packet(mtu size) or left img size
    if (pdata == NULL)
    {
        return false;
    }
    memcpy(pdata, pheader + g_dfu_ctx.curr_offset, push_img_size);
    g_dfu_ctx.curr_offset += push_img_size;

    //aes encrypt when ota  and all data is encrypted
    if (g_dfu_ctx.mode.aesflg && g_dfu_ctx.mode.aesmode)
    {
        uint8_t *ptmp = pdata;
        for (uint16_t loop = 0; loop < push_img_size / 16; loop++)  //only encrypt 16 bytes align
        {
            dfu_encrypt(ptmp);
            ptmp += 16;
        }
    }
    dfu_client_data_write(conn_id, pdata, push_img_size);

    //target enable buffer check
    if (g_dfu_ctx.mode.buffercheck)
    {
        if (!g_buf_check_pkt_count)
        {
            g_buf_check_crc = 0;
        }
        g_buf_check_pkt_count++;
        g_buf_check_crc = dfu_client_crc_cal(pdata, push_img_size, g_buf_check_crc);
        if (g_buf_check_pkt_count == g_buf_check_pkt_num || g_dfu_ctx.curr_offset == g_dfu_ctx.image_length)
        {
            uint8_t data[5];
            data[0] = DFU_OPCODE_REPORT_BUFFER_CRC;
            LE_WORD2EXTRN(data + 1, (g_buf_check_pkt_count - 1) * mtu_size + push_img_size);
            BE_WORD2EXTRN(data + 3, g_buf_check_crc);
            dfu_client_cp_write(conn_id, data, DFU_LENGTH_REPORT_BUFFER_CRC);
            g_buf_check_pkt_count = 0;
        }
    }

    os_mem_free(pdata);
    return true;
}

void dfu_client_handle_cp_msg(uint8_t conn_id, uint8_t *pdata, uint16_t len)
{
    uint8_t opcode = pdata[0];
    uint8_t req_opcode = pdata[1];
    DFU_PRINT_INFO2("==>dfu_client_handle_cp_msg: opcode=%d len=%d.", req_opcode, len);
	printf("==>dfu_client_handle_cp_msg: opcode=%d len=%d.\n", req_opcode, len);
    if (opcode != DFU_OPCODE_NOTIFICATION)
    {
        DFU_PRINT_INFO1("==>handle cp msg: invalid notify opcode=%d!", opcode);
        return;
    }

    if (req_opcode == DFU_OPCODE_REPORT_TARGET_INFO && len == DFU_NOTIFY_LENGTH_REPORT_TARGET_INFO)
    {
#if (RCU_SILENT_OTA == 1)
		uint16_t target_version = LE_EXTRN2WORD(pdata + 3);  //obey git_ver format
		uint32_t cur_offset = LE_EXTRN2DWORD(pdata + 5);
		printf("handle cp msg: report target info(bee2 rcu)\r\n");//debug
#else

        uint32_t target_version = LE_EXTRN2DWORD(pdata + 3);  //obey git_ver format
        uint32_t cur_offset = LE_EXTRN2DWORD(pdata + 7);
        PROFILE_PRINT_INFO5("handle cp msg: report target info, target_version=%d.%d.%d.%d, cur_offset=0x%x",
                            MAJOR_IMG_VER(target_version), MINOR_IMG_VER(target_version),
                            REVISION_IMG_VER(target_version), RESERVE_IMG_VER(target_version), cur_offset);

#if (ONLY_SUPPORT_UPGRADE == 1)
        if (IMG_VER_TO_LITTLE_ENDIAN(LE_EXTRN2DWORD(g_dfu_ctx.dfu_img_version) <= IMG_VER_TO_LITTLE_ENDIAN(
                                         LE_EXTRN2DWORD(pdata + 3)))
    	{
        	DBG_DIRECT("handle cp msg: Img no need ota!");
            return;
    	}
#endif
#endif

        //uint8_t *pheader = (uint8_t *)TEMP_OTA_IMG_ADDR;
		uint8_t *pheader = bt_ota_central_client_get_image();
        uint8_t data[DFU_LENGTH_START_DFU];
        data[0] = DFU_OPCODE_START_DFU;
        memcpy(data + 1, pheader, DFU_HEADER_SIZE);
        memset(data + 1 + DFU_HEADER_SIZE, 0, 4);

        //aes encrypt when ota
    	if (g_dfu_ctx.mode.aesflg)
    	{
	        DFU_PRINT_TRACE1("===>Data before encryped: %b", TRACE_BINARY(16, data + 1));
	        dfu_encrypt(data + 1);
            DFU_PRINT_TRACE1("===>Data after encryped: %b", TRACE_BINARY(16, data + 1));
        }

        dfu_client_cp_write(conn_id, data, DFU_LENGTH_START_DFU);
        g_dfu_ctx.curr_offset = DFU_HEADER_SIZE;
        g_dfu_ctx.image_length = ((T_IMG_CTRL_HEADER_FORMAT *)pheader)->payload_len + IMG_HEADER_SIZE;
        DFU_PRINT_INFO4("==>handle cp msg: otaing img_id 0x%x, verision 0x%x->0x%x, dfu_img_total_len=%d",
                        g_dfu_ctx.signature, target_version, g_dfu_ctx.dfu_img_version, g_dfu_ctx.image_length);

        /*notify application*/
        g_dfu_ctx.fsm_client = DFU_CB_START;
        if (pf_dfu_client_app_info_cb)
	    {
	        T_DFU_CB_MSG cb_msg;
	        cb_msg.type = DFU_CB_START;
	        pf_dfu_client_app_info_cb(dfu_client, conn_id, &cb_msg);
        }
    }
    else if (req_opcode == DFU_OPCODE_START_DFU && len == DFU_NOTIFY_LENGTH_ARV)
    {
        if (g_dfu_ctx.mode.buffercheck)
        {
            DFU_PRINT_INFO0("==>handle cp msg: Enable buffer check");
            uint8_t data[1];
            data[0] = DFU_OPCODE_BUFFER_CHECK_EN;
            dfu_client_cp_write(conn_id, data, DFU_LENGTH_BUFFER_CHECK_EN);
        }
        else
        {
            DFU_PRINT_INFO0("==>handle cp msg: Disable buffer check");
            //start push image, make use of credits
            for (uint8_t loop = 0; loop < 4; loop++)
            {
                dfu_client_push_image(conn_id);
            }
        }
    }
    else if (req_opcode == DFU_OPCODE_BUFFER_CHECK_EN && len == DFU_NOTIFY_LENGTH_BUFFER_CHECK_EN)
    {
        uint16_t buffer_check_len = LE_EXTRN2WORD(pdata + 3);
        uint16_t mtu_size = LE_EXTRN2WORD(pdata + 5);

        g_buf_check_pkt_num = buffer_check_len / ((mtu_size - 3) & 0xfff0);  //buffer check paket num
        g_buf_check_pkt_count = 0;

        DFU_PRINT_INFO3("==>handle cp msg: buffer_check_lengeth=%d, mtu_size=%d, g_buf_check_pkt_num=%d",
                        buffer_check_len, mtu_size, g_buf_check_pkt_num);

        //start push image, make use of credits
        for (uint8_t loop = 0; loop < 8; loop++)
        {
            dfu_client_push_image(conn_id);
        }
    }
    else if (req_opcode == DFU_OPCODE_REPORT_BUFFER_CRC && len == DFU_NOTIFY_LENGTH_REPORT_BUFFER_CRC)
    {
        uint32_t offset = LE_EXTRN2DWORD(pdata + 3);
        DFU_PRINT_INFO2("==>handle cp msg: Target offset=%d,g_dfu_ctx.curr_offset=%d", offset,
                        g_dfu_ctx.curr_offset);
        g_dfu_ctx.curr_offset = offset;
        if (g_dfu_ctx.image_length > g_dfu_ctx.curr_offset)
        {
            dfu_client_push_image(conn_id);
        }
        else
        {
            DFU_PRINT_INFO0("==>handle cp msg: Img trans done(BufCheckEn)!");
            uint8_t data[DFU_LENGTH_VALID_FW];
            data[0] = DFU_OPCODE_VALID_FW;
            LE_WORD2EXTRN(data + 1, g_dfu_ctx.signature);
            dfu_client_cp_write(conn_id, data, DFU_LENGTH_VALID_FW);
        }
    }
    else if (req_opcode == DFU_OPCODE_VALID_FW && len == DFU_NOTIFY_LENGTH_ARV)
    {
        DFU_PRINT_INFO1("==>handle cp msg: verify image rst=%d(1 success)", pdata[2]);
        uint8_t data[DFU_LENGTH_ACTIVE_IMAGE_RESET];
        data[0] = pdata[2] == DFU_ARV_SUCCESS ? DFU_OPCODE_ACTIVE_IMAGE_RESET : DFU_OPCODE_SYSTEM_RESET;
        dfu_client_cp_write(conn_id, data, DFU_LENGTH_ACTIVE_IMAGE_RESET);
        g_dfu_ctx.fsm_client = DFU_CB_NONE;
        if (pf_dfu_client_app_info_cb)
        {
            T_DFU_CB_MSG cb_msg;
            cb_msg.type = pdata[2] == DFU_ARV_SUCCESS ? DFU_CB_END : DFU_CB_FAIL;
            pf_dfu_client_app_info_cb(dfu_client, conn_id, &cb_msg);
        }
    }
}

/*profile callback to handle msg*/
T_APP_RESULT dfu_client_handle_msg(T_CLIENT_ID client_id, uint8_t conn_id, void *p_data)
{
    T_DFU_CLIENT_CB_DATA *pcb_data = (T_DFU_CLIENT_CB_DATA *)p_data;
    DFU_PRINT_INFO2("==>dfu client hdl msg: type=%d, g_dfu_fsm_write=%d.", pcb_data->cb_type,
                    g_dfu_fsm_write);
	printf("==>dfu client hdl msg: client_id=%d, type=%d, g_dfu_fsm_write=%d.\n", client_id, pcb_data->cb_type,
                    g_dfu_fsm_write);
    switch (pcb_data->cb_type)
    {
    case DFU_CLIENT_CB_TYPE_DISC_STATE:
        switch (pcb_data->cb_content.disc_state)
        {
        case DISC_DFU_DONE:
            /* Discovery Simple BLE service procedure successfully done. */
            DFU_PRINT_INFO0("==>dfu client hdl msg: discover procedure done.");
			
            if (dfu_client_cp_cccd_set(conn_id, 1)) //enable server notification
            {
                g_dfu_fsm_write = DFU_FSM_WRITE_CCCD_ENABLE;
                DFU_PRINT_INFO0("==>dfu client hdl msg: Enable CCCD.");
            }
            break;
        case DISC_DFU_FAIL:
            /* Discovery Request failed. */
            DFU_PRINT_INFO0("==>dfu client hdl msg: discover request failed.");
            break;
        default:
            break;
        }
        break;
    case DFU_CLIENT_CB_TYPE_READ_RESULT:
        switch (pcb_data->cb_content.read_result.type)
        {
        case DFU_READ_CP_CCCD:
            DFU_PRINT_INFO1("==>dfu client hdl msg: dfu_cp_cccd=%d.",
                            pcb_data->cb_content.read_result.data.dfu_cp_cccd);
            break;
        default:
            break;
        }
        break;
    case DFU_CLIENT_CB_TYPE_WRITE_RESULT:
        if (pcb_data->cb_content.write_result.cause == 0)
        {
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
            if(pcb_data->cb_content.write_result.type == DFU_WRITE_CP_CCCD || pcb_data->cb_content.write_result.type == DFU_WRITE_CP)
            {
                request_in_process_flag[conn_id] = 0;
                if_queue_out_and_send(conn_id);
            }
#endif
            switch (pcb_data->cb_content.write_result.type)
            {
            case DFU_WRITE_CP_CCCD:
                DFU_PRINT_INFO1("==>dfu client hdl msg: CB Type(DFU_WRITE_CP_CCCD),g_dfu_fsm_write=%d.",
                                g_dfu_fsm_write);
                if (g_dfu_fsm_write == DFU_FSM_WRITE_CCCD_ENABLE)
                {
                    //T_IMG_HEADER_FORMAT *p_header = (T_IMG_HEADER_FORMAT *)TEMP_OTA_IMG_ADDR;
                    uint8_t *ota_app_image = bt_ota_central_client_get_image();
    				T_IMG_HEADER_FORMAT *p_header = (T_IMG_HEADER_FORMAT *)ota_app_image;

                    g_dfu_ctx.signature = p_header->ctrl_header.image_id;
                    uint8_t data[DFU_LENGTH_REPORT_TARGET_INFO];
                    data[0] = DFU_OPCODE_REPORT_TARGET_INFO;
                    LE_WORD2EXTRN(data + 1, g_dfu_ctx.signature);					

                    /*after enable CCCD, start handle control point procedure*/
                    dfu_client_cp_write(conn_id, data, DFU_LENGTH_REPORT_TARGET_INFO); 
                }
                break;
            case DFU_WRITE_DATA:
                DFU_PRINT_INFO1("==>dfu client hdl msg: CB Type(DFU_WRITE_DATA),g_dfu_fsm_write=%d.",
                                g_dfu_fsm_write);
                if (g_dfu_fsm_write == DFU_FSM_WRITE_DFU_DATA)
                {
                    if (g_dfu_ctx.image_length > g_dfu_ctx.curr_offset)
                    {
                        DFU_PRINT_INFO0("==>dfu client hdl msg: CB Type(DFU_WRITE_DATA)");
                        dfu_client_push_image(conn_id);
                    }
                    else
                    {
                        if (!g_dfu_ctx.mode.buffercheck)
                        {
                            /*when not enable buf check, img trans done, verify img*/
                            DFU_PRINT_INFO0("==>dfu client hdl msg: Img trans done(BufCheckDisable)");
                            uint8_t data[DFU_LENGTH_VALID_FW];
                            data[0] = DFU_OPCODE_VALID_FW;
                            LE_WORD2EXTRN(data + 1, g_dfu_ctx.signature);
                            dfu_client_cp_write(conn_id, data, DFU_LENGTH_VALID_FW);
                        }
                    }
                }
                break;
            case DFU_WRITE_CP:
                DFU_PRINT_INFO1("==>dfu client hdl msg: DFU_WRITE_CP, g_dfu_fsm_write=%d.",
                                g_dfu_fsm_write);
                if (g_dfu_fsm_write == DFU_FSM_WRITE_WAIT_WRITE_RESP)
                {
                    g_dfu_fsm_write = DFU_FSM_WRITE_DO_NOT_CARE;
                    if (g_cp_msg_len)
                    {
                        DFU_PRINT_INFO2("==>dfu client hdl msg: DFU_WRITE_CP, g_dfu_fsm_write=%d, g_cp_msg_len=%d",
                                        g_dfu_fsm_write, g_cp_msg_len);
                        dfu_client_handle_cp_msg(conn_id, g_cp_msg, g_cp_msg_len);
                        g_cp_msg_len = 0;
                    }
                }
                break;
            default:
                break;
            }
        }
        else
        {
            DFU_PRINT_INFO3("==>dfu client hdl msg: fail to send msg! type=%d, cause=0x%x g_dfu_fsm_write=%d",
                            pcb_data->cb_content.write_result.type, pcb_data->cb_content.write_result.cause, g_dfu_fsm_write);
            DFU_PRINT_INFO0("==>dfu client hdl msg: le_disconnect");
            le_disconnect(conn_id);
        }
        break;
    case DFU_CLIENT_CB_TYPE_NOTIF_IND_RESULT:
        switch (pcb_data->cb_content.notif_ind_data.type)
        {
        case DFU_NOTIF_RECEIVE_DFU_CP_NOTIFY:
            {
                DFU_PRINT_INFO1("==>dfu client hdl msg: NOTIF_IND_RESULT, g_dfu_fsm_write=%d.",
                                g_dfu_fsm_write);
                if (g_dfu_fsm_write == DFU_FSM_WRITE_DO_NOT_CARE)
                {
                    DFU_PRINT_INFO1("==>dfu client hdl msg: NOTIF_IND_RESULT, g_dfu_fsm_write=%d(if 0 hdl cp msg)",
                                    g_dfu_fsm_write);
                    dfu_client_handle_cp_msg(conn_id, pcb_data->cb_content.notif_ind_data.value.pdata,
                                             pcb_data->cb_content.notif_ind_data.value.len);
                }
                else
                {
                    //cache
                    g_cp_msg_len = pcb_data->cb_content.notif_ind_data.value.len;
                    memcpy(g_cp_msg, pcb_data->cb_content.notif_ind_data.value.pdata,
                           g_cp_msg_len > 20 ? 20 : g_cp_msg_len);
                    DFU_PRINT_INFO1("==>dfu client hdl msg: Cached Received Notify Data, len=%d", g_cp_msg_len);

                }
            }
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
    return APP_RESULT_SUCCESS;
}

/**
 * @brief Dfu Client Callbacks.
*/
const T_FUN_CLIENT_CBS dfu_client_cbs =
{
    dfu_client_disc_state_cb,   //!< Discovery State callback function pointer
    dfu_client_disc_result_cb,  //!< Discovery result callback function pointer
    dfu_client_read_result_cb,      //!< Read response callback function pointer
    dfu_client_write_result_cb,     //!< Write result callback function pointer
    dfu_client_notify_indicate_cb,  //!< Notify Indicate callback function pointer
    NULL //dfu_client_disconnect_cb       //!< Link disconnection callback function pointer
};

static void dfu_client_init(void)
{
    //T_IMG_HEADER_FORMAT *p_header = (T_IMG_HEADER_FORMAT *)TEMP_OTA_IMG_ADDR;
    uint8_t *ota_app_image = bt_ota_central_client_get_image();
    T_IMG_HEADER_FORMAT *p_header = (T_IMG_HEADER_FORMAT *)ota_app_image;
	
    if (p_header && p_header->ctrl_header.payload_len)
    {
        T_IMG_ID dfu_img_id = (T_IMG_ID)p_header->ctrl_header.image_id;
        if (dfu_img_id < IMAGE_MAX && dfu_img_id >= SecureBoot)
        {
            DFU_PRINT_INFO5("==>dfu_client_init: will dfu img:0x%x, version:%d.%d.%d.%d", dfu_img_id,
                            MAJOR_IMG_VER(p_header->git_ver.ver_info.version),
                            MINOR_IMG_VER(p_header->git_ver.ver_info.version),
                            REVISION_IMG_VER(p_header->git_ver.ver_info.version),
                            RESERVE_IMG_VER(p_header->git_ver.ver_info.version));

            g_dfu_ctx.dfu_img_version = p_header->git_ver.ver_info.version;
        }
        else
        {
            DFU_PRINT_ERROR1("==>dfu_client_init: Temp bank img_id:0x%x Invalid!", dfu_img_id);
        }


    }

#if defined(CONFIG_PLATFORM_8710C)
	if(crypto_init()) printf("crypto_init fail!\n");
#endif	
}

/**
  * @brief  add Dfu client to application.
  * @param  appCB: pointer of app callback function to handle specific client module data.
  * @retval Client ID of the specific client module.
  */
T_CLIENT_ID dfu_add_client(P_FUN_GENERAL_APP_CB app_cb, uint8_t link_num)
{
    uint16_t size;
    if (link_num > DFU_MAX_LINKS)
    {
        PROFILE_PRINT_ERROR1("dfu_add_client: invalid link_num %d", link_num);
        return 0xff;
    }
    if (false == client_register_spec_client_cb(&dfu_client, &dfu_client_cbs))
    {
        dfu_client = CLIENT_PROFILE_GENERAL_ID;
        DFU_PRINT_ERROR0("dfu_client_add: fail!");
        return dfu_client;
    }
    DFU_PRINT_INFO1("dfu_client_add: client ID = %d", dfu_client);

    dfu_link_num = link_num;
    size = dfu_link_num * sizeof(T_DFU_LINK);
    dfu_table = os_mem_zalloc(RAM_TYPE_DATA_ON, size);

    /* register callback for profile to inform application that some events happened. */
    pf_dfu_client_cb = dfu_client_handle_msg;
    pf_dfu_client_app_info_cb = app_cb;
    dfu_client_init();
	
    return dfu_client;
}

void dfu_delete_client(void)
{
    if (dfu_table != NULL) {
        os_mem_free(dfu_table);
        dfu_table = NULL;
    }
}


/**
  * @brief  dfu client connect target device.
  * @param  p_le_scan_info: filtered scan info
  */
void dfu_client_connect_device(T_LE_SCAN_INFO *p_le_scan_info)
{
    PROFILE_PRINT_INFO1("dfu_client_connect_device: p_le_scan_info->adv_type = %d",
                        p_le_scan_info->adv_type);

    /* corresponding to silent ota or normal ota */
    if (p_le_scan_info->adv_type == GAP_ADV_EVT_TYPE_UNDIRECTED ||
        p_le_scan_info->adv_type == GAP_ADV_EVT_TYPE_SCAN_RSP)
    {
        T_GAP_LE_CONN_REQ_PARAM conn_req_param;
        conn_req_param.scan_interval = 0x10;
        conn_req_param.scan_window = 0x10;
        conn_req_param.conn_interval_min = 0x80;
        conn_req_param.conn_interval_max = 0x80;
        conn_req_param.conn_latency = 0;
        conn_req_param.supv_tout = 1000;
        conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
        conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
        le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);
        T_GAP_CAUSE cause;
        if (GAP_CAUSE_SUCCESS == (cause = le_connect(GAP_PHYS_CONN_INIT_1M_BIT, p_le_scan_info->bd_addr,
                                                     p_le_scan_info->remote_addr_type,
                                                     GAP_LOCAL_ADDR_LE_PUBLIC, 1000)))
        {

            PROFILE_PRINT_INFO2("==>dfu_client_connect_device: try connecting %s, remote_addr_type=%d",
                                TRACE_BDADDR(p_le_scan_info->bd_addr), p_le_scan_info->remote_addr_type);
        }
        else
        {
            PROFILE_PRINT_WARN1("==>dfu_client_connect_device: FAIL cause=%d!", cause);
        }
    }
    else
    {
        PROFILE_PRINT_ERROR1("dfu_client_connect_device: invalid adv type=%d!", p_le_scan_info->adv_type);
    }

    return;
}

#endif
