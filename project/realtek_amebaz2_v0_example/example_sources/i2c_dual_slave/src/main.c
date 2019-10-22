/*
 * Copyright(c) 2007 - 2019 Realtek Corporation. All rights reserved.
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
 */

#include <string.h>
#include "i2c_api.h"

//I2C1_SEL S0
#define MBED_I2C_MTR_SDA        PA_20
#define MBED_I2C_MTR_SCL        PA_19

//I2C0_SEL S0
#define MBED_I2C_SLV_SDA        PA_20
#define MBED_I2C_SLV_SCL        PA_19

#define MBED_I2C_SLAVE_ADDR0    0xAA
#define MBED_I2C_BUS_CLK        100000    //hz

#define I2C_DATA_LENGTH         127
uint8_t i2cdatasrc[I2C_DATA_LENGTH];
uint8_t i2cdatadst[I2C_DATA_LENGTH];
uint8_t i2cdatardsrc[I2C_DATA_LENGTH];
uint8_t i2cdatarddst[I2C_DATA_LENGTH];

//#define I2C_MASTER_DEVICE

// RESTART verification
#define I2C_RESTART_DEMO

i2c_t i2cmaster;
i2c_t i2cslave;

void i2c_slave_rx_check(void)
{
    int i2clocalcnt;
    int result = 0;

    dbg_printf("check slave received data>>> \r\n");
    for (i2clocalcnt = 0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt += 2) {
        //dbg_printf("i2c data: %02x \t %02x\n",i2cdatadst[i2clocalcnt],i2cdatadst[i2clocalcnt+1]);
    }
    //HalDelayUs(5000);

    // verify result
    result = 1;
#if !defined(I2C_RESTART_DEMO)
    for (i2clocalcnt = 0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt++) {
        if (i2cdatasrc[i2clocalcnt] != i2cdatadst[i2clocalcnt]) {
            result = 0;
            break;
        }
    }
#else
    if (i2cdatasrc[0] == i2cdatadst[0]) {
        if (i2cdatasrc[0] != i2cdatadst[0]) {
            result = 0;
        }
    } else if (i2cdatasrc[1] == i2cdatadst[0]) {
        for (i2clocalcnt = 1; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt++) {
            if (i2cdatasrc[i2clocalcnt] != i2cdatadst[i2clocalcnt - 1]) {
                dbg_printf("idx:%d, src:%x, dst:%x \r\n", i2clocalcnt, i2cdatasrc[i2clocalcnt], i2cdatadst[i2clocalcnt]);
                for (i2clocalcnt = 0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt += 2) {
                    if (i2clocalcnt != (I2C_DATA_LENGTH - 1)) {
                        dbg_printf("i2c data: %02x \t %02x \r\n", i2cdatadst[i2clocalcnt], i2cdatadst[i2clocalcnt + 1]);
                    }
                }
                result = 0;
                break;
            }
        }
    } else {
        for (i2clocalcnt = 0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt++) {
            if (i2cdatasrc[i2clocalcnt] != i2cdatadst[i2clocalcnt]) {
                dbg_printf("idx:%d, src:%x, dst:%x \r\n", i2clocalcnt, i2cdatasrc[i2clocalcnt], i2cdatadst[i2clocalcnt]);
                result = 0;
                break;
            }
        }
    }
#endif

    dbg_printf("\r\nSlave receive: Result is %s \r\n", (result) ? "success" : "fail");
    memset(&i2cdatadst[0], 0x00, I2C_DATA_LENGTH);
}

void i2c_master_rx_check(void)
{
    int i2clocalcnt;
    int result = 0;

    dbg_printf("check master received data>>> \r\n");
    for (i2clocalcnt = 0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt += 2) {
        //dbg_printf("i2c data: %02x \t %02x\n",i2cdatarddst[i2clocalcnt],i2cdatarddst[i2clocalcnt+1]);
    }

    // verify result
    result = 1;
    for (i2clocalcnt = 0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt++) {
        if (i2cdatarddst[i2clocalcnt] != i2cdatardsrc[i2clocalcnt]) {
            result = 0;
            break;
        }
    }
    dbg_printf("\r\nMaster receive: Result is %s \r\n", (result) ? "success" : "fail");
}

int main(void)
{
    int i2clocalcnt;

    dbg_printf("\r\n   I2C Dual Slave DEMO   \r\n");

    // prepare for transmission
    memset(&i2cdatasrc[0], 0x00, I2C_DATA_LENGTH);
    memset(&i2cdatadst[0], 0x00, I2C_DATA_LENGTH);
    memset(&i2cdatardsrc[0], 0x00, I2C_DATA_LENGTH);
    memset(&i2cdatarddst[0], 0x00, I2C_DATA_LENGTH);

    for (i2clocalcnt=0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt++){
        i2cdatasrc[i2clocalcnt] = i2clocalcnt + 0x2;
    }

    for (i2clocalcnt=0; i2clocalcnt < I2C_DATA_LENGTH; i2clocalcnt++){
        i2cdatardsrc[i2clocalcnt] = i2clocalcnt + 1;
    }

#ifdef I2C_MASTER_DEVICE
    dbg_printf("Slave addr=%x \r\n",MBED_I2C_SLAVE_ADDR0);
    memset(&i2cmaster, 0x00, sizeof(i2c_t));
    i2c_init(&i2cmaster, MBED_I2C_MTR_SDA, MBED_I2C_MTR_SCL);
    i2c_frequency(&i2cmaster, MBED_I2C_BUS_CLK);
#ifdef I2C_RESTART_DEMO
    i2c_restart_enable(&i2cmaster);
#endif

    // Master write - Slave read
    dbg_printf("\r\nMaster write>>> \r\n");
#ifdef I2C_RESTART_DEMO
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[0], 1, 0);
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[1], (I2C_DATA_LENGTH - 1), 1);
#else
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[0], I2C_DATA_LENGTH, 1);
#endif
    
    // Master read - Slave write
    dbg_printf("Master read>>> \r\n");
#ifdef I2C_RESTART_DEMO
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[0], 1, 0);
#endif
    i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatarddst[0], I2C_DATA_LENGTH, 1);
    i2c_master_rx_check();

#else //I2C_SLAVE_DEVICE
    dbg_printf("Slave addr=%x \r\n",MBED_I2C_SLAVE_ADDR0);
    memset(&i2cslave, 0x00, sizeof(i2c_t));
    i2c_init(&i2cslave, MBED_I2C_SLV_SDA, MBED_I2C_SLV_SCL);
    i2c_frequency(&i2cslave, MBED_I2C_BUS_CLK);
    i2c_slave_address(&i2cslave, 0, MBED_I2C_SLAVE_ADDR0, 0xFF);
    i2c_slave_mode(&i2cslave, 1);

    // Master write - Slave read
    dbg_printf("\r\nSlave read>>> \r\n");
    i2c_slave_read(&i2cslave, (char*)&i2cdatadst[0], I2C_DATA_LENGTH);

    // Master read - Slave write
    dbg_printf("Slave write>>> \r\n");
#ifdef I2C_RESTART_DEMO
    i2c_slave_read(&i2cslave, (char*)&i2cdatadst[0], 1);
#endif

    i2c_slave_rx_check();
    i2c_slave_set_for_rd_req(&i2cslave, 1);
    i2c_slave_write(&i2cslave, (const char*)&i2cdatardsrc[0], I2C_DATA_LENGTH);
#endif // #ifdef I2C_SLAVE_DEVICE

    while(1);
}
