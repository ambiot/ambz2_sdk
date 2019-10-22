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
#include "dma_api.h"

/* notice: AmebaZ2 HW not support Multi-Block like Ameba1 */

//Single-Block Example Demo

#define DMA_CPY_LEN         256
#define DMA_SRC_OFFSET      0
#define DMA_DST_OFFSET      0

gdma_t gdma;
uint8_t TestBuf1[512] __attribute__((aligned(32)));
uint8_t TestBuf2[512] __attribute__((aligned(32)));
volatile uint8_t dma_done;

void dma_done_handler (uint32_t id)
{
    dbg_printf("DMA Copy Done!! \r\n");
    dma_done = 1;
}

int main (void)
{
    int i;
    int err;

    dbg_printf("\r\n   GDMA DEMO   \r\n");

    dma_memcpy_init(&gdma, dma_done_handler, (uint32_t)&gdma);
    for (i = 0; i < 512; i++) {
        TestBuf1[i] = i;
    }
    memset(TestBuf2, 0xff, 512);

    dma_done = 0;
    dma_memcpy(&gdma, (TestBuf2 + DMA_DST_OFFSET), (TestBuf1 + DMA_SRC_OFFSET), DMA_CPY_LEN);

    while (dma_done == 0);

    err = 0;
    for (i = 0; i < DMA_CPY_LEN; i++) {
        if (TestBuf2[(i + DMA_DST_OFFSET)] != TestBuf1[(i + DMA_SRC_OFFSET)]) {
            dbg_printf("DMA Copy Memory Compare Err, %d %x %x \r\n", i, TestBuf1[(i + DMA_SRC_OFFSET)], TestBuf2[(i + DMA_DST_OFFSET)]);
            err = 1;
            break;
        }
    }

    if (!err) {
        dbg_printf("DMA Copy Memory Compare OK!! %x \r\n", TestBuf2[(DMA_DST_OFFSET + DMA_CPY_LEN)]);
    }
    dma_memcpy_deinit(&(gdma));

    while(1);
}
