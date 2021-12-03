/******************************************************************************
 *
 * Copyright(c) 2019 - 2021 Realtek Corporation. All rights reserved.
 *                                        
 ******************************************************************************/
#include "dfudep_service.h"

#define DFUDEP_DBG(x, ...) do {} while(0)

#if defined(MESH_DFU) && MESH_DFU

extern struct dfudep_service_ops dfudep_service;

//server dfu service 
uint8_t dfu_metadata_check(void *pmetadata, uint32_t len)
{
    if (dfudep_service.dfu_metadata_check) {
        return dfudep_service.dfu_metadata_check(pmetadata, len);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_metadata_check");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_resources_check(void)
{
    if (dfudep_service.dfu_resources_check) {
        return dfudep_service.dfu_resources_check();
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_resources_check");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_init_dfu_resources(void)
{
    if (dfudep_service.dfu_init_dfu_resources) {
        return dfudep_service.dfu_init_dfu_resources();
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_init_dfu_resources");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_block_verify(uint8_t *data ,uint16_t len, uint8_t signature_size)
{
    if (dfudep_service.dfu_block_verify) {
        return dfudep_service.dfu_block_verify(data, len, signature_size);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_block_verify");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_verify(uint16_t image_id)
{
    if (dfudep_service.dfu_verify) {
        return dfudep_service.dfu_verify(image_id);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_verify");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_cancel(void)
{
    if (dfudep_service.dfu_cancel) {
        return dfudep_service.dfu_cancel();
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_cancel");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_verify_cancel(void)
{
    if (dfudep_service.dfu_verify_cancel) {
        return dfudep_service.dfu_verify_cancel();
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_verify_cancel");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_apply(uint8_t reason, uint32_t delay_ms)
{
    if (dfudep_service.dfu_apply) {
        return dfudep_service.dfu_apply(reason, delay_ms);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_apply");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_block_data_restore(uint8_t * data , uint32_t len, uint8_t signature_size)
{
    if (dfudep_service.dfu_block_data_restore) {
        return dfudep_service.dfu_block_data_restore(data, len, signature_size);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_block_data_restore");
    }

    return DFU_RESULT_FAIL;
}

void dfu_failsafe(void)
{
    if (dfudep_service.dfu_failsafe) {
        dfudep_service.dfu_failsafe();
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_failsafe");
    }

    return;
}

uint8_t is_dfu_enabled(void)
{
    if (dfudep_service.is_dfu_enabled) {
        return dfudep_service.is_dfu_enabled();
    } else {
        DFUDEP_DBG("Not implement dfu service: is_dfu_enabled");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_get_fw_info(fw_info_t *pfw_info)
{
    if (dfudep_service.dfu_get_fw_info) {
        return dfudep_service.dfu_get_fw_info(pfw_info);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_get_fw_info");
    }

    return DFU_RESULT_FAIL;
}

//client dfu service 
uint32_t dfu_fw_image_data_get(uint32_t len, uint8_t *pout)
{
    if (dfudep_service.dfu_fw_image_data_get) {
        return dfudep_service.dfu_fw_image_data_get(len, pout);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_fw_image_data_get");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_block_signature(uint8_t *data ,uint16_t len, uint8_t signature_size)
{
    if (dfudep_service.dfu_block_signature) {
        return dfudep_service.dfu_block_signature(data, len, signature_size);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_block_signature");
    }

    return DFU_RESULT_FAIL;
}

uint8_t dfu_check_fw_info(fw_info_t *pfw_info)
{
    if (dfudep_service.dfu_check_fw_info) {
        return dfudep_service.dfu_check_fw_info(pfw_info);
    } else {
        DFUDEP_DBG("Not implement dfu service: dfu_check_fw_info");
    }

    return DFU_RESULT_FAIL;
}
#endif
