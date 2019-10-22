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

/*******************************************************************************
 *  HRM.c - Eminent Heart Rate Module (HRM) routines via I2C
 *******************************************************************************/

#include "HRM_2197.h"
#include "heart_interface.h"
#include "i2c_api.h"

#define MBED_I2C_SLAVE_ADDR0        0x41
#define HR_MODE                     0x001b
#define LED_ENABLE                  0x3081
#define FRAME_ENABLE                0x4804
#define CHIP_RESET                  0x4000
#define CHIP_RUN                    0x4004
#define DATA_LOCK                   0x4005
#define DATA_UNLOCK                 0x4004
#define I2C_DATA_MAX_LENGTH         20
#define CLOCK_SET                   0x3800
#define MBED_I2C_MTR_SDA            PA_20
#define MBED_I2C_MTR_SCL            PA_19
#define MBED_I2C_INTB               PA_17
#define MBED_I2C_BUS_CLK            100000    //hz

uint8_t i2cdata_write[I2C_DATA_MAX_LENGTH];
uint8_t i2cdata_read[I2C_DATA_MAX_LENGTH];
uint16_t cmd;

i2c_t i2cmaster;

uint8_t integ_time = HR_INTEG_MIN;
int integ_time_array[] = {4, 6, 8, 10, 15, 20, 25, 30, 40, 55, 70, 90, 110, 150, 200, 250, 350, 450, 550};

//Step1. define the callback to handle event of heart rate update
/*******************************************************************************
 *  report heart rate every 1 second
 *******************************************************************************/
void on_heartrate_update(int heartrate)
{
    dbg_printf("heart rate %d \r\n", heartrate);
    //fflush(stdout);
}

char i2cdatasrc[3] = {0x68, 0x90, 0x98};

static void ePL_WriteCommand(uint16_t cmd)
{
    i2cdata_write[0] = (uint8_t)(cmd >> 8);
    i2cdata_write[1] = (uint8_t)(cmd & 0xFF);
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (const char *)&i2cdata_write[0], 2, 1);
}

uint16_t read_hrm(void)
{
    uint32_t raw, normalized_raw;
    int integ_time_changed = 0;
    ePL_WriteCommand(DATA_LOCK);

    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[1], 1, 1);
    i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[1], 2, 1);
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[2], 1, 1);
    i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[2], 2, 1);

    raw = i2cdata_read[1];
    raw |= (uint16_t) i2cdata_read[2] << 8;

    normalized_raw = raw >> 4;
    normalized_raw = normalized_raw * integ_time_array[HR_INTEG_BASE];
    normalized_raw = normalized_raw / integ_time_array[integ_time];

    if ((raw > HR_TH_HIGH) && (integ_time > HR_INTEG_MIN)) {
        integ_time -= 1;
        integ_time_changed = 1;
    } else if ((raw < HR_TH_LOW) && (integ_time < HR_INTEG_MAX)) {
        integ_time += 1;
        integ_time_changed = 1;
    }

    if (integ_time_changed == 1) {
        ePL_WriteCommand(((0x01<<3)<<8) | (HR_FILTER_4 | integ_time));
        ePL_WriteCommand(((0x08<<3)<<8) | (HR_RESETN_RESET));
    }

    ePL_WriteCommand(((0x08<<3)<<8) | (HR_RESETN_RUN));

    return normalized_raw;
}

/*******************************************************************************
 *  main function to read data, input to library,
 *  and calculate heart rate
 *******************************************************************************/
int main(void)
{
//    int i, length;
//    int *data;
    uint16_t result;

    dbg_printf("\r\n   I2C EPL2197 Heartrate DEMO   \r\n");

//    data = (int*) calloc(3000, sizeof(int));
//    load_ppg_signal(data, &length); //Load Test Data From File
    i2c_init(&i2cmaster, MBED_I2C_MTR_SDA ,MBED_I2C_MTR_SCL);
    i2c_frequency(&i2cmaster,MBED_I2C_BUS_CLK);
//Step2. delegate the event of heart rate update
    register_callback(on_heartrate_update);

//Step3. Set the data length of heart rate calculation= 2^9 = 512
    ePL_WriteCommand(((0x00<<3)<<8) | (HR_MODE_HRS | HR_OSR_1024 | HR_GAIN_MID));
    ePL_WriteCommand(((0x01<<3)<<8) | (HR_FILTER_4 | integ_time));
    ePL_WriteCommand(((0x09<<3)<<8) | (HR_PDRIVE_70MA));
    ePL_WriteCommand(((0x06<<3)<<8) | (HR_IR_ENABLE | HR_INT_FRAME));
    ePL_WriteCommand(((0x08<<3)<<8) | (HR_RESETN_RESET));
    while (1) {
//Step4. Add ppg data continuously, and the Lib will return the Heart Rate 1 time/sec
        result = read_hrm();
        if (result > 100) {
            add_PPG_XYZ(result, 0, 0, 0);
        }
        hal_delay_ms(40);//Simulate the ppg input time interval = 40ms
    }

//Step5. Stop
//    stop();
//    free(data);
}
