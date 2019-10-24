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
#include "serial_api.h"
#include "serial_ex_api.h"

#define UART_TX                 PA_14
#define UART_RX                 PA_13

#define TX_DMA_MODE             1       /*1: DMA mode; 0: interrupt mode*/
#define RX_DMA_MODE             1       /*1: DMA mode; 0: interrupt mode*/

#define BUF_SZ                  2048
#define UART_TIMEOUT_MS         5000    //ms

#define TASK_STACK_SIZE         2048
#define TASK_PRIORITY           (tskIDLE_PRIORITY + 1)

char tx_buf[BUF_SZ]__attribute__((aligned(32))) = {0};
char rx_buf[BUF_SZ]__attribute__((aligned(32))) = {0};

volatile uint32_t rx_bytes = 0;
volatile uint32_t tx_bytes = 0;

SemaphoreHandle_t UartHWSema;    // Uart HW resource
SemaphoreHandle_t UartRxSema;
SemaphoreHandle_t UartTxSema;

void uart_send_string_done (uint32_t id)
{
//    serial_t    *sobj = (void*)id;
    signed portBASE_TYPE xHigherPriorityTaskWoken;

    xSemaphoreGiveFromISR(UartTxSema, &xHigherPriorityTaskWoken);
}

void uart_recv_string_done (uint32_t id)
{
    serial_t *sobj = (void*)id;
    signed portBASE_TYPE xHigherPriorityTaskWoken;

    rx_bytes = sobj->uart_adp.rx_count;
    xSemaphoreGiveFromISR(UartRxSema, &xHigherPriorityTaskWoken);
}
/*
void uart_send_string (serial_t *sobj, char *pstr)
{
    int32_t ret=0;

    xSemaphoreTake(UartTxSema, portMAX_DELAY);
    
    ret = serial_send_stream(sobj, pstr, _strlen(pstr));
    if (ret != 0) {
        dbg_printf("%s Error(%d)\r\n", __FUNCTION__, ret);
        xSemaphoreGive( UartTxSema );    
    }
}*/

void uart_demo_tx (void *param)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	rtw_create_secure_context(256);
#endif		
    serial_t *psobj=param;    // UART object
//    int i;
    int tx_byte_timeout;
    int32_t ret;
//    unsigned int loop_cnt=0;
    unsigned int tx_size = 100;
    
    // Initial TX buffer
    //memset(tx_buf, 0xFF, BUF_SZ);
    memset(tx_buf, 0x55, BUF_SZ);
    tx_buf[0] = tx_size & 0xFF;
    tx_buf[1] = (tx_size >> 8) & 0xFF;
    tx_buf[tx_size + 2] = 0;    // end of string

    tx_size += 2;

    xSemaphoreGive(UartTxSema);    // Ready to TX
    while (1) {
        // Wait TX Rady (TX Done)
        if (xSemaphoreTake(UartTxSema, ((TickType_t)UART_TIMEOUT_MS / portTICK_RATE_MS)) != pdTRUE) {
            xSemaphoreTake(UartHWSema, portMAX_DELAY);    // get the semaphore before access the HW
            tx_byte_timeout = serial_send_stream_abort(psobj);
            xSemaphoreGive(UartHWSema);    // return the semaphore after access the HW

            dbg_printf("send timeout!! %d bytes sent \r\n", tx_byte_timeout);

            xSemaphoreGive(UartTxSema); // Ready to TX
        } else {
            xSemaphoreTake(UartHWSema, portMAX_DELAY);    // get the semaphore before access the HW
#if TX_DMA_MODE
            ret = serial_send_stream_dma(psobj, tx_buf, tx_size);
#else
            ret = serial_send_stream(psobj, tx_buf, tx_size);
#endif
            xSemaphoreGive(UartHWSema);    // return the semaphore after access the HW
            vTaskDelay(3000);
            if (ret != 0) {
                dbg_printf("uart_tx_thread: send error %d \r\n", ret);
                xSemaphoreGive(UartTxSema);    // Ready to TX
            }
        }
    }
}

void uart_demo_rx (void *param)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	rtw_create_secure_context(256);
#endif	
    serial_t *psobj=param;    // UART object

    int ret;
    int rx_len;

    while (1) {
        memset(rx_buf, 0, BUF_SZ);
        rx_bytes = 0;

        xSemaphoreTake(UartHWSema, portMAX_DELAY);    // get the semaphore before access the HW
#if RX_DMA_MODE
        ret = serial_recv_stream_dma(psobj, rx_buf, BUF_SZ);    // DMA mode
#else
        ret = serial_recv_stream(psobj, rx_buf, BUF_SZ);    // Interrupt mode
#endif
        if (ret != 0) {
            dbg_printf("ERRORS in Interrupt/DMA mode \r\n");
        }

        xSemaphoreGive(UartHWSema);    // return the semaphore after access the HW

        if (xSemaphoreTake(UartRxSema, (TickType_t)UART_TIMEOUT_MS/portTICK_RATE_MS) != pdTRUE ) {
            xSemaphoreTake(UartHWSema, portMAX_DELAY);    // get the semaphore before access the HW
            rx_bytes = serial_recv_stream_abort(psobj);
            xSemaphoreGive(UartHWSema);    // return the semaphore after access the HW
            dbg_printf("recv timeout!!! %d bytes recvied \r\n", rx_bytes);
        }

        rx_len = (rx_buf[1] << 8) | (rx_buf[0]);
        if (rx_bytes && (rx_len != (rx_bytes - 2))) {
            dbg_printf("RX length not match %d != %d \r\n", rx_len, (rx_bytes - 2));

            int i, j;
            for (i = 0; i < (1 + rx_bytes / 10); i++) {
                for (j = 0; j < 10; j++) {
                    dbg_printf(" %x", rx_buf[(i * 10 + j)] & 0xFF);
                }
                dbg_printf("\r\n");
            }
        }
    }
}

void uart_test_demo (void *param)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
	rtw_create_secure_context(512);
#endif	
    dbg_printf("\r\n   UART Stream TX RX Concurrent IAR Demo   \r\n");

    serial_t sobj;
    serial_init(&sobj, UART_TX, UART_RX);
    serial_baud(&sobj, 115200);
    serial_format(&sobj, 8, ParityNone, 1);

    serial_send_comp_handler(&sobj, (void*)uart_send_string_done, (uint32_t)&sobj);
    serial_recv_comp_handler(&sobj, (void*)uart_recv_string_done, (uint32_t)&sobj);

    // Hardware resource lock
    UartHWSema = xSemaphoreCreateBinary();
    xSemaphoreGive(UartHWSema);

    // Create semaphore for UART RX done(received espected bytes or timeout)
    UartRxSema = xSemaphoreCreateBinary();

    // Create semaphore for UART TX done
    UartTxSema = xSemaphoreCreateBinary();

    // create demo Task
    if (xTaskCreate((TaskFunction_t)uart_demo_rx, "UART RX DEMO", (TASK_STACK_SIZE / 4), (void *)&sobj, TASK_PRIORITY, NULL) != pdPASS) {
        dbg_printf("Cannot create rx demo task \r\n");
        goto end_demo;
    }
    if (xTaskCreate((TaskFunction_t)uart_demo_tx, "UART TX DEMO", (TASK_STACK_SIZE / 4), (void *)&sobj, TASK_PRIORITY, NULL) != pdPASS) {
        dbg_printf("Cannot create tx demo task \r\n");
        goto end_demo;
    }

end_demo:

    vTaskDelete(NULL);
}

int main (void)
{
    // create demo Task
    if(xTaskCreate((TaskFunction_t)uart_test_demo, "uart test demo", (2048 / 2), (void *)NULL, (tskIDLE_PRIORITY + 1), NULL) != pdPASS) {
        dbg_printf("Cannot create uart test demo task \r\n");
        goto end_demo;
    }

    vTaskStartScheduler();

end_demo:
    while(1);
}
