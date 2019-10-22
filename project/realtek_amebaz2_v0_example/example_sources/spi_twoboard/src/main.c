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

#include "spi_api.h"
#include "spi_ex_api.h"
#include "sys_api.h"

#define SPI_IS_AS_MASTER    1

#define SPI0_MOSI           PA_19
#define SPI0_MISO           PA_20
#define SPI0_SCLK           PA_3
#define SPI0_CS             PA_2

extern void hal_ssi_toggle_between_frame(phal_ssi_adaptor_t phal_ssi_adaptor, u8 ctl);

int main (void)
{
    int TestingTimes = 10;
    int Counter      = 0;
    int TestData     = 0;

    dbg_printf("\r\n   SPI Twoboard DEMO   \r\n");

    if ((SPI0_SCLK == PA_3) || (SPI0_CS == PA_2)) {
        sys_jtag_off();
    }

#if SPI_IS_AS_MASTER
    spi_t spi_master;

    spi_init(&spi_master, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
    spi_format(&spi_master, DfsSixteenBits, ((int)SPI_SCLK_IDLE_LOW | (int)SPI_SCLK_TOGGLE_MIDDLE), 0);
    spi_frequency(&spi_master, 1000000);
    hal_ssi_toggle_between_frame(&spi_master.hal_ssi_adaptor, ENABLE);

    dbg_printf("-------------------------------------------------------- \r\n");
    for (Counter = 0, TestData=0xFF; Counter < TestingTimes; Counter++) {
        spi_master_write(&spi_master, TestData);
        dbg_printf("Master write: %02X \r\n", TestData);
        TestData--;
    }
    spi_free(&spi_master);

#else
    spi_t spi_slave;

    spi_init(&spi_slave, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
    spi_format(&spi_slave, DfsSixteenBits, ((int)SPI_SCLK_IDLE_LOW | (int)SPI_SCLK_TOGGLE_MIDDLE), 1);
    hal_ssi_toggle_between_frame(&spi_slave.hal_ssi_adaptor, ENABLE);

    dbg_printf("-------------------------------------------------------- \r\n");
    for (Counter = 0, TestData = 0xFF; Counter < TestingTimes; Counter++) {
        dbg_printf(ANSI_COLOR_CYAN"Slave  read : %02X \r\n"ANSI_COLOR_RESET,
        spi_slave_read(&spi_slave));
        TestData--;
    }
    spi_free(&spi_slave);
#endif

    dbg_printf("SPI Demo finished. \r\n");
    for(;;);
}
