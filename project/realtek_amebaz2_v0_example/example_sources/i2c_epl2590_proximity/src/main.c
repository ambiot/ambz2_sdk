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

#include "i2c_api.h"

#define MBED_I2C_MTR_SDA        PA_20
#define MBED_I2C_MTR_SCL        PA_19
#define MBED_I2C_INTB           PA_17
#define MBED_I2C_SLAVE_ADDR0    0x49
#define MBED_I2C_BUS_CLK        100000    //hz
#define I2C_DATA_MAX_LENGTH     20

uint8_t i2cdata_write[I2C_DATA_MAX_LENGTH];
uint8_t i2cdata_read[I2C_DATA_MAX_LENGTH];
uint16_t cmd;

i2c_t i2cmaster;
//sensor command
#define WAKE_UP                 0x1102
#define CHIP_REFRESH1           0xFD8E
#define CHIP_REFRESH2           0xFE22
#define CHIP_REFRESH3           0xFE02
#define CHIP_REFRESH4           0xFD00
#define PS_MODE                 0x0002
#define ALS1_MODE               0x0072
#define ALS2_MODE               0x503E
#define ALS3_MODE               0x583E
#define POWER_UP                0x1102
#define CHIP_RESET              0x1100
#define CHANGE_TIME             0x0851
#define SETTING_1               0x0F19
#define SETTING_2               0x0D10
#define INT                     0x3022

char i2cdatasrc[5] = {0x1B, 0x1E, 0x1F, 0x80, 0x88};

static void ePL_WriteCommand(uint16_t cmd)
{
    i2cdata_write[0] = (uint8_t)(cmd >> 8);
    i2cdata_write[1] = (uint8_t)(cmd & 0xFF);
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (const char*)&i2cdata_write[0], 2, 1);
}

int main(void)
{
//    int result;
//    int i;
    int flag = 0;
//    char intertupt;

    dbg_printf("\r\n   I2C EPL2590 Proximity DEMO   \r\n");

    dbg_printf("Sensor_Init \r\n");
    i2c_init(&i2cmaster, MBED_I2C_MTR_SDA ,MBED_I2C_MTR_SCL);
    i2c_frequency(&i2cmaster,MBED_I2C_BUS_CLK);

    ePL_WriteCommand(WAKE_UP);
    ePL_WriteCommand(CHIP_REFRESH1);
    ePL_WriteCommand(CHIP_REFRESH2);
    ePL_WriteCommand(CHIP_REFRESH3);
    ePL_WriteCommand(CHIP_REFRESH4);
    ePL_WriteCommand(PS_MODE);
    ePL_WriteCommand(SETTING_1);
    ePL_WriteCommand(SETTING_2);
    ePL_WriteCommand(CHIP_RESET);
    ePL_WriteCommand(POWER_UP);

    hal_delay_ms(240);
    while (1) {
        //ePL_WriteCommand(DATA_LOCK);
        i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[0], 1, 1);
        i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[0], 2, 1);
        i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[1], 1, 1);
        i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[1], 2, 1);
        i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[2], 1, 1);
        i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[2], 2, 1);
        //dbg_printf("PS LOW: %d\n", i2cdata_read[1]);
        //dbg_printf("PS HIGH: %d\n", i2cdata_read[2]);
        flag = (i2cdata_read[0] & 8) ? 1:0;
        //int ret = (i2cdata_read[0] & 4)? 1:0;
        //dbg_printf("flag: %d\n", flag);
        //dbg_printf("ret: %d\n", ret);

        if (flag) {
            dbg_printf("the object is far \r\n");
        } else {
            dbg_printf("the object is near \r\n");
        }

        //ePL_WriteCommand(POWER_UP);
        hal_delay_ms(1000);
    }
}
