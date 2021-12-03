/******************************************************************************
 *
 * Copyright(c) 2019 - 2021 Realtek Corporation. All rights reserved.
 *                                        
 ******************************************************************************/
#include "dfudep_service.h"
#include "os_sched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#if defined(MESH_DFU) && MESH_DFU
uint8_t customer_dfu_metadata_check(void *pmetadata, uint32_t len)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_resources_check(void)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_init_dfu_resources(void)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_block_verify(uint8_t *data ,uint16_t len, uint8_t signature_size)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_verify(uint16_t image_id)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_cancel(void)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_verify_cancel(void)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_apply(uint8_t reason, uint32_t delay_ms)
{
    os_delay(delay_ms);
    //reboot

    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_block_data_restore(uint8_t *data , uint32_t len, uint8_t signature_size)
{
    return DFU_RESULT_FAIL;
}

void customer_dfu_failsafe(void)
{
    return;
}

uint8_t customer_is_dfu_enabled(void)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_get_fw_info(fw_info_t *pfw_info)
{   
    return DFU_RESULT_FAIL;
}

uint32_t customer_dfu_fw_image_data_get(uint32_t len, uint8_t *pout)
{
	return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_block_signature(uint8_t *data ,uint16_t len, uint8_t signature_size)
{
    return DFU_RESULT_FAIL;
}

uint8_t customer_dfu_check_fw_info(fw_info_t *pfw_info)
{
    return DFU_RESULT_FAIL;
}

const struct dfudep_service_ops dfudep_service = {
    /* updater server api */
    customer_dfu_metadata_check,                //dfu_metadata_check
    customer_dfu_resources_check,               //dfu_resources_check
    customer_dfu_init_dfu_resources,            //dfu_init_dfu_resources
    customer_dfu_block_verify,                  //dfu_block_verify
    customer_dfu_verify,                        //dfu_verify
    customer_dfu_cancel,                        //dfu_cancel
    customer_dfu_verify_cancel,                 //dfu_verify_cancel
    customer_dfu_apply,                         //dfu_apply
    customer_dfu_block_data_restore,            //dfu_block_data_restore
    customer_dfu_failsafe,                      //dfu_failsafe
    customer_is_dfu_enabled,                    //is_dfu_enabled
    customer_dfu_get_fw_info,                   //dfu_get_fw_info
    /* updater client api */
    customer_dfu_fw_image_data_get,             //dfu_fw_image_data_get
    customer_dfu_block_signature,               //dfu_block_signature
    customer_dfu_check_fw_info                  //dfu_check_fw_info
};
#endif