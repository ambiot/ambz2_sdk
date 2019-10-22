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

#define MBED_I2C_MTR_SDA            PA_20
#define MBED_I2C_MTR_SCL            PA_19
#define MBED_I2C_INTB               PA_17
#define MBED_I2C_SLAVE_ADDR0        0x5D
#define MBED_I2C_BUS_CLK            40000    //hz
#define I2C_DATA_MAX_LENGTH         20
#define malloc                      pvPortMalloc
#define free                        vPortFree

uint8_t i2cdata_write[I2C_DATA_MAX_LENGTH];
uint8_t i2cdata_read[I2C_DATA_MAX_LENGTH];
uint16_t cmd;

i2c_t i2cmaster;
int count = 0;
//sensor command
#define SENSOR_START 0x20A0
#define FIFO 0x2E41
#define REBOOT 0x2110
#define READ 0x2101
#define BYPASS 0x2E00

char i2cdatasrc[9] = {0x27, 0x28, 0x29, 0x2A};
//char i2cdatasrc[7] = {0x40, 0x48, 0x50, 0x27, 0x28, 0x29, 0x2A};

static void ePL_WriteCommand(uint16_t cmd)
{
    i2cdata_write[0] = (uint8_t)(cmd >> 8);
    i2cdata_write[1] = (uint8_t)(cmd & 0xFF);
    i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (const char*)&i2cdata_write[0], 2, 1);
}

/*
struct node
{
    int info;
    struct node *ptr;
}*front,*rear,*temp,*front1;
*/
//int frontelement();
//void enq(int data);
//void deq();
/*
void enq(int data)
{
    if (rear == NULL)
    {
        rear = (struct node *)malloc(1*sizeof(struct node));
        if(rear == NULL)
        {
              dbg_printf("\n\rmalloc rear failed!\r\n");
              return;
        }
        rear->ptr = NULL;
        rear->info = data;
        front = rear;
        //dbg_printf("front info: %d\n", front->info);
    }
    else
    {
        temp=(struct node *)malloc(1*sizeof(struct node));
        rear->ptr = temp;
        temp->info = data;
        temp->ptr = NULL;
 
        rear = temp;
        //dbg_printf("rear info: %d\n", rear->info);
    }
    count++;
}

void deq()
{
    front1 = front;
    //dbg_printf("front info before deq: %d\n", front->info);
    if (front1 == NULL)
    {
        dbg_printf("Error: Trying to display elements from empty queue\r\n");
        return;
    }
    else
    {
        if (front1->ptr != NULL)
        {
            front1 = front1->ptr;
            //dbg_printf("\nDequed value : %d\n", front->info);
            free(front);
            front = front1;
        }
        else
        {
            //dbg_printf("\nDequed value : %d\n", front->info);
            free(front);
            front = NULL;
            rear = NULL;
        }
        count--;
    }
}
*/

int main(void)
{
//       int result;
    int data;
//       int temprature;
//       int flag = 0;
//       int sum = 0;
//       int average = 0;
//       struct node *output;
//       char intertupt;

    dbg_printf("\r\n   I2C LPS25HB Pressure DEMO   \r\n");

    dbg_printf("Sensor_Init \r\n");
    //for(i=0; i<16; i++)
    //dbg_printf("ouput before: %d\n", i2cdata_read[i]);
    i2c_init(&i2cmaster, MBED_I2C_MTR_SDA ,MBED_I2C_MTR_SCL);
    i2c_frequency(&i2cmaster,MBED_I2C_BUS_CLK);

    ePL_WriteCommand(SENSOR_START);
    ePL_WriteCommand(REBOOT);
    //ePL_WriteCommand(BYPASS);

    while (1) {
        //i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[3], 1, 1);
        //i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[3], 2, 1);
        //dbg_printf("Status Reg: %d\n", i2cdata_read[3]);
        i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[1], 1, 1);
        i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[1], 2, 1);
        //dbg_printf("--------pressure output LSB: %d\n", i2cdata_read[4]);
        i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[2], 1, 1);
        i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[2], 2, 1);
        //dbg_printf("--------pressure output MID: %d\n", i2cdata_read[5]);
        i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, &i2cdatasrc[3], 1, 1);
        i2c_read(&i2cmaster, MBED_I2C_SLAVE_ADDR0, (char*)&i2cdata_read[3], 2, 1);
        //dbg_printf("--------pressure output MSB: %d\n", i2cdata_read[6]);
        hal_delay_ms(2000);
        data = (i2cdata_read[3]*256*256*100+i2cdata_read[2]*256*100+i2cdata_read[1]*100)/4128;
        dbg_printf("pressure: %dPa \r\n", data);
        /*
        if (count == 20) {
            deq();
        }
        enq(data);
        output = front;
        sum = front->info;
        while (output->ptr != NULL) {
            output = output->ptr;
            sum = sum + output->info; 
        }
        //dbg_printf("------count = %d---------\n", count);
        average = sum / count;
        //dbg_printf("---final output: %d---\n", average);
        */
    }
    //hal_delay_ms(1000);
}
