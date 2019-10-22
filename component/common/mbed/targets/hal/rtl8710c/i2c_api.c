/**************************************************************************//**
 * @file     i2c_api.c
 * @brief    This file implements the UART serial port Mbed HAL API functions.
 * 
 * @version  V1.00
 * @date     2017-09-15
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/
#include "objects.h"
#include "pinmap.h"
#include "hal_i2c.h"
#include "objects.h"
#include "i2c_api.h"
#include "i2c_ex_api.h"
#include "memory.h"

#if CONFIG_I2C_EN
#include "pinmap.h"
#include <string.h>

static const PinMap PinMap_I2C_SDA[] = {
    {PA_3,  RTL_PIN_PERI(PID_I2C0, 0, PinSel0), RTL_PIN_FUNC(PID_I2C0, PinSel0)},
    {PA_12,  RTL_PIN_PERI(PID_I2C0, 0, PinSel1), RTL_PIN_FUNC(PID_I2C0, PinSel1)},
    {PA_16,  RTL_PIN_PERI(PID_I2C0, 0, PinSel2), RTL_PIN_FUNC(PID_I2C0, PinSel2)},
    {PA_20,  RTL_PIN_PERI(PID_I2C0, 0, PinSel3), RTL_PIN_FUNC(PID_I2C0, PinSel3)},
    {PA_22,  RTL_PIN_PERI(PID_I2C0, 0, PinSel4), RTL_PIN_FUNC(PID_I2C0, PinSel4)},
    
    {NC,    NC,     0}
};

static const PinMap PinMap_I2C_SCL[] = {
    {PA_2,  RTL_PIN_PERI(PID_I2C0, 0, PinSel0), RTL_PIN_FUNC(PID_I2C0, PinSel0)},
    {PA_11,  RTL_PIN_PERI(PID_I2C0, 0, PinSel1), RTL_PIN_FUNC(PID_I2C0, PinSel1)},
    {PA_15,  RTL_PIN_PERI(PID_I2C0, 0, PinSel2), RTL_PIN_FUNC(PID_I2C0, PinSel2)},
    {PA_19,  RTL_PIN_PERI(PID_I2C0, 0, PinSel3), RTL_PIN_FUNC(PID_I2C0, PinSel3)},
    {PA_21,  RTL_PIN_PERI(PID_I2C0, 0, PinSel4), RTL_PIN_FUNC(PID_I2C0, PinSel4)},

    
    {NC,    NC,     0}
};

void i2c_init(i2c_t *obj, PinName sda, PinName scl)
{
    uint32_t i2c_sel = 0;
    uint32_t i2c_idx = 0;

    uint32_t i2c_sda = (uint32_t)pinmap_peripheral(sda, PinMap_I2C_SDA);
    uint32_t i2c_scl = (uint32_t)pinmap_peripheral(scl, PinMap_I2C_SCL);

    //DBG_INFO_MSG_ON(_DBG_I2C_);
    i2c_sel = (uint32_t)pinmap_merge(i2c_sda, i2c_scl);
    i2c_idx = RTL_GET_PERI_IDX(i2c_sel);
    if (unlikely(i2c_idx == NC)) {
        DBG_I2C_ERR("%s: Cannot find matched I2C\n", __FUNCTION__);
        return;
    }

    hal_i2c_load_default(&obj->i2c_adp, i2c_idx);
    
    //obj->i2c_adp.mst_spe_func |= I2CAddressRetry;
    hal_i2c_init(&obj->i2c_adp, (uint8_t)scl, (uint8_t)sda);
}

void i2c_frequency(i2c_t *obj, int hz)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    phal_i2c_adapter->init_dat.clock = hz/1000;
    hal_i2c_set_clk(phal_i2c_adapter);
}

int i2c_start(i2c_t *obj)
{
    return 0;
}

int i2c_stop(i2c_t *obj)
{
    return 0;
}

void i2c_reset(i2c_t *obj)
{
    hal_i2c_deinit(&obj->i2c_adp);
}

int i2c_byte_read(i2c_t *obj, int last)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    int i2c_rx_dat_tmp;
    
    phal_i2c_adapter->rx_dat.len = 1;
    phal_i2c_adapter->rx_dat.buf = (uint8_t *)&i2c_rx_dat_tmp;
    phal_i2c_adapter->rx_dat.addr = phal_i2c_adapter->init_dat.ack_addr0;
    
    if (!last) {
        phal_i2c_adapter->rx_dat.mst_stop = I2CDisable;
    } else {
        phal_i2c_adapter->rx_dat.mst_stop = I2CEnable;
    }

    hal_i2c_receive(phal_i2c_adapter);

    return (int)i2c_rx_dat_tmp;
}

int i2c_byte_write(i2c_t *obj, int data)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    int i2c_tx_dat_tmp = data;
    
    phal_i2c_adapter->tx_dat.len = 1;
    phal_i2c_adapter->tx_dat.buf = (uint8_t *)&i2c_tx_dat_tmp;
    phal_i2c_adapter->tx_dat.mst_stop = I2CEnable;
    phal_i2c_adapter->tx_dat.addr = phal_i2c_adapter->init_dat.ack_addr0;
    
    if (hal_i2c_send(phal_i2c_adapter) != HAL_OK) {
        return 0;
    }

    return 1;   
}

int i2c_read(i2c_t *obj, int address, char *data, int length, int stop)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    uint32_t i2c_time_start;
    
    if (phal_i2c_adapter->rx_dat.addr != address) {
        phal_i2c_adapter->rx_dat.addr = address;
    }

    phal_i2c_adapter->rx_dat.buf = (uint8_t *)data;
    phal_i2c_adapter->rx_dat.len = length;
    phal_i2c_adapter->rx_dat.mst_stop = stop;

    
    if (hal_i2c_receive(phal_i2c_adapter) != HAL_OK) {
        return (int)(length - phal_i2c_adapter->rx_dat.len);
    } else {
        i2c_time_start = hal_read_cur_time();
        while ((phal_i2c_adapter->status == I2CStatusRxReady) || (phal_i2c_adapter->status == I2CStatusRxing)) {
            if (hal_i2c_timeout_chk(phal_i2c_adapter, i2c_time_start)) {
                phal_i2c_adapter->status = I2CStatusTimeOut;
                break;
            }
        }

        return (int)(length - phal_i2c_adapter->rx_dat.len);
    }
}

int i2c_write(i2c_t *obj, int address, const char *data, int length, int stop)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    uint32_t i2c_time_start;

    if (phal_i2c_adapter->tx_dat.addr != address) {
        phal_i2c_adapter->tx_dat.addr = address;
    }

    phal_i2c_adapter->tx_dat.buf = (uint8_t *)data;
    phal_i2c_adapter->tx_dat.len = length;
    phal_i2c_adapter->tx_dat.mst_stop = stop;

    if (hal_i2c_send(phal_i2c_adapter) != HAL_OK) {
        return (int)(length - phal_i2c_adapter->tx_dat.len);
    } else {
        i2c_time_start = hal_read_cur_time();
        while ((phal_i2c_adapter->status == I2CStatusTxReady) || (phal_i2c_adapter->status == I2CStatusTxing)) {
            if (hal_i2c_timeout_chk(phal_i2c_adapter, i2c_time_start)) {
                phal_i2c_adapter->status = I2CStatusTimeOut;
                break;
            }
        }

        return (int)(length - phal_i2c_adapter->tx_dat.len);
    }
}

int i2c_enable_control(i2c_t *obj, int enable)
{
    return (int)(hal_i2c_en_ctrl((hal_i2c_adapter_t *)(&obj->i2c_adp), enable));
}

void i2c_restart_enable(i2c_t *obj)
{
    hal_i2c_mst_restr_sw_ctrl((hal_i2c_adapter_t *)(&obj->i2c_adp), I2CEnable);
    hal_i2c_mst_restr_ctrl((hal_i2c_adapter_t *)(&obj->i2c_adp), I2CEnable);
}

void i2c_restart_disable(i2c_t *obj)
{
    hal_i2c_mst_restr_sw_ctrl((hal_i2c_adapter_t *)(&obj->i2c_adp), I2CDisable);
    hal_i2c_mst_restr_ctrl((hal_i2c_adapter_t *)(&obj->i2c_adp), I2CDisable);
}

void i2c_slave_mode(i2c_t *obj, int enable_slave)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);

    if (phal_i2c_adapter->init_dat.master == I2CMasterMode) {
        hal_i2c_deinit(phal_i2c_adapter);
    }

    phal_i2c_adapter->init_dat.master = I2CSlaveMode;
    if (enable_slave) {
        hal_i2c_init(phal_i2c_adapter, phal_i2c_adapter->pltf_dat.scl_pin, phal_i2c_adapter->pltf_dat.sda_pin);
    } else {       
        hal_i2c_deinit(phal_i2c_adapter);
    }
}

void i2c_slave_address(i2c_t *obj, int idx, uint32_t address, uint32_t mask)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    phal_i2c_adapter->init_dat.ack_addr0 = (address & mask);
    hal_i2c_set_sar(phal_i2c_adapter, 0, (address & mask));
}

int i2c_slave_receive(i2c_t *obj)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    
    return (phal_i2c_adapter->status);
}

int i2c_slave_read(i2c_t *obj, char *data, int length)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    uint32_t i2c_time_start;
    
    phal_i2c_adapter->rx_dat.buf = (uint8_t *)data;
    phal_i2c_adapter->rx_dat.len = length;
    
    if (hal_i2c_slv_recv(phal_i2c_adapter) != HAL_OK) {
        return (int)(length - phal_i2c_adapter->rx_dat.len);
    } else {
        i2c_time_start = hal_read_cur_time();
        while ((phal_i2c_adapter->status == I2CStatusRxReady) || (phal_i2c_adapter->status == I2CStatusRxing)) {
            if (hal_i2c_timeout_chk(phal_i2c_adapter, i2c_time_start)) {
                phal_i2c_adapter->status = I2CStatusTimeOut;
                break;
            }
        }

        return (int)(length - phal_i2c_adapter->rx_dat.len);
    }
}

int i2c_slave_write(i2c_t *obj, const char *data, int length)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);

    phal_i2c_adapter->tx_dat.buf = (uint8_t *)data;
    phal_i2c_adapter->tx_dat.len = length;

    if (hal_i2c_slv_send(phal_i2c_adapter) != HAL_OK) {
        return (int)(length - phal_i2c_adapter->tx_dat.len);
    } else {
        return 0;
    }
}

int i2c_slave_set_for_rd_req(i2c_t *obj, int set)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    if (set) {
        hal_i2c_slv_set_for_mst_rd_cmd(phal_i2c_adapter);
    } else {
        hal_i2c_slv_clear_for_mst_rd_cmd(phal_i2c_adapter);
    }

    return 0;
}

int i2c_slave_set_for_data_nak(i2c_t *obj, int set_nak)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);

    hal_i2c_slv_no_ack_ctrl(phal_i2c_adapter, set_nak);
    return 0;
}

void i2c_set_user_callback(i2c_t *obj, I2CCallback i2ccb, void(*i2c_callback)(void *))
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    
    if ((i2ccb >= I2C_TX_COMPLETE) && (i2ccb <= I2C_ERR_OCCURRED)) {
        switch (i2ccb) {
            case I2C_TX_COMPLETE:
                phal_i2c_adapter->usr_cb.txc.cb = i2c_callback;
                break;
            case I2C_RX_COMPLETE:
                phal_i2c_adapter->usr_cb.rxc.cb = i2c_callback;
                break;
            case I2C_RD_REQ_COMMAND:
                phal_i2c_adapter->usr_cb.rd_req.cb = i2c_callback;
                break;
            case I2C_ERR_OCCURRED:
                phal_i2c_adapter->usr_cb.err.cb = i2c_callback;
                break;
            default:
                break;
        }
    }
}

void i2c_clear_user_callback(i2c_t *obj, I2CCallback i2ccb)
{
    hal_i2c_adapter_t *phal_i2c_adapter = (hal_i2c_adapter_t *)(&obj->i2c_adp);
    
    if ((i2ccb >= I2C_TX_COMPLETE) && (i2ccb <= I2C_ERR_OCCURRED)) {
        switch (i2ccb) {
            case I2C_TX_COMPLETE:
                phal_i2c_adapter->usr_cb.txc.cb = NULL;
                break;
            case I2C_RX_COMPLETE:
                phal_i2c_adapter->usr_cb.rxc.cb = NULL;
                break;
            case I2C_RD_REQ_COMMAND:
                phal_i2c_adapter->usr_cb.rd_req.cb = NULL;
                break;
            case I2C_ERR_OCCURRED:
                phal_i2c_adapter->usr_cb.err.cb = NULL;
                break;
            default:
                break;
        }
    }
}
#endif

