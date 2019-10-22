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

#include "gpio_api.h"

#define DHT11 11
#define DHT22 22
#define DHT21 21

// define your DHT type
#define DHTTYPE DHT11

#define DHT_DATA_PIN PA_19

uint32_t expect_pulse (uint8_t port, uint8_t pin, uint32_t expect_level, uint32_t max_cycle, gpio_t gpio_dht)
{
    uint32_t cycle = 1;
    while (expect_level == gpio_read(&gpio_dht)) {
        if (cycle++ >= max_cycle) {
            return 0;
        }
    }
    return cycle;
}

int main (void)
{
    int i;
    uint32_t cycles[80];
    uint32_t low_cycles, high_cycles;

    uint8_t data[5];
    float humidity = 0;
    float temperature = 0;

    gpio_t gpio_dht;
    uint8_t port_num;
    uint8_t pin_num;

    dbg_printf("\r\n   GPIO DHT Temp Humidity DEMO   \r\n");

    gpio_init(&gpio_dht, DHT_DATA_PIN);
    gpio_dir(&gpio_dht, PIN_INPUT);
    gpio_mode(&gpio_dht, PullUp);

    port_num = gpio_dht.adapter.port_idx;
    pin_num = gpio_dht.adapter.pin_idx;

    while (1) {
        hal_delay_us(2 * 1000 * 1000);

        data[0] = data[1] = data[2] = data[3] = data[4] = 0;

        gpio_dir(&gpio_dht, PIN_OUTPUT);
        gpio_write(&gpio_dht, 1);
        hal_delay_us(250 * 1000);

        // toggle down to turn DHT from power saving mode to high speed mode
        gpio_write(&gpio_dht, 0);
        hal_delay_us(20 * 1000);
        gpio_write(&gpio_dht, 1);
        hal_delay_us(40);

        gpio_dir(&gpio_dht, PIN_INPUT);
        gpio_mode(&gpio_dht, PullNone);

        // wait DHT toggle down to ready
        if (expect_pulse(port_num, pin_num, 0, 1000, gpio_dht) == 0) {
            dbg_printf("Timeout waiting for start signal low pulse. \r\n");
            continue;
        }
        if (expect_pulse(port_num, pin_num, 1, 1000, gpio_dht) == 0) {
            dbg_printf("Timeout waiting for start signal high pulse. \r\n");
            continue;
        }

        for (i = 0; i < 80; i += 2) {
            cycles[i]   = expect_pulse(port_num, pin_num, 0, 1000, gpio_dht);
            cycles[i+1] = expect_pulse(port_num, pin_num, 1, 1000, gpio_dht);
        }

        for (i = 0; i < 40; i++) {
            low_cycles = cycles[(2 * i)];
            high_cycles = cycles[(2 * i + 1)];
            if ((low_cycles == 0) || (high_cycles == 0)) {
                break;
            }
            data[i/8] <<= 1;
            if (high_cycles > low_cycles) {
                data[(i / 8)] |= 1;
            }
        }

        if (i != 40) {
            dbg_printf("Timeout waiting for pulse. \r\n");
            continue;
        }

        if ( ((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4] ) {
            dbg_printf("Checksum failure! \r\n");
            continue;
        }

        switch (DHTTYPE) {
            case DHT11:
                humidity = data[0];
                temperature = data[2];
                break;
            case DHT22:
            case DHT21:
                humidity = data[0];
                humidity *= 256;
                humidity += data[1];
                humidity *= 0.1;
                temperature = data[2] & 0x7F;
                temperature *= 256;
                temperature += data[3];
                temperature *= 0.1;
                if (data[2] & 0x80) {
                    temperature *= -1;
                }
                break;
        }
        dbg_printf("Humidity: %.2f %%\t Temperature: %.2f *C \r\n", humidity, temperature);
    }
}
