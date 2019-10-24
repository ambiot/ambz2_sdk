/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2014, Realtek
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */
#include "cmsis.h"
#include "sys_api.h"
#include "flash_api.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "hal_wdt.h"

#define FLASH_SECTOR_SIZE				0x1000

extern hal_uart_adapter_t log_uart;
extern void log_uart_port_init (int log_uart_tx, int log_uart_rx, uint32_t baud_rate);
extern void log_uart_flush_wait(void);
extern void hci_tp_close(void);
extern hal_status_t hal_wlan_pwr_off(void);

#if defined(CONFIG_BUILD_NONSECURE) && (CONFIG_BUILD_NONSECURE==1)
SECTION_NS_ENTRY_FUNC void NS_ENTRY get_fw_info_nsc(uint32_t *targetFWaddr, uint32_t *currentFWaddr, uint32_t *fw1_sn, uint32_t *fw2_sn);
SECTION_NS_ENTRY_FUNC uint32_t NS_ENTRY get_cur_fw_idx_nsc(void);
SECTION_NS_ENTRY_FUNC uint32_t NS_ENTRY get_number_of_fw_valid_nsc(void);
SECTION_NS_ENTRY_FUNC void NS_ENTRY hal_sys_set_fast_boot_nsc(uint32_t pstart_tbl, uint32_t func_idx);
#define get_fw_info get_fw_info_nsc
#define get_number_of_fw_valid get_number_of_fw_valid_nsc
#define get_cur_fw_idx get_cur_fw_idx_nsc
#define hal_sys_set_fast_boot hal_sys_set_fast_boot_nsc
#else
extern void get_fw_info(uint32_t *targetFWaddr, uint32_t *currentFWaddr, uint32_t *fw1_sn, uint32_t *fw2_sn);
extern uint32_t get_number_of_fw_valid(void);
extern uint32_t get_cur_fw_idx(void);
#endif

#if defined(CONFIG_BUILD_SECURE) && (CONFIG_BUILD_SECURE==1)
SECTION_NS_ENTRY_FUNC void NS_ENTRY get_fw_info_nsc(uint32_t *targetFWaddr, uint32_t *currentFWaddr, uint32_t *fw1_sn, uint32_t *fw2_sn)
{
	get_fw_info(targetFWaddr, currentFWaddr, fw1_sn, fw2_sn);
}
SECTION_NS_ENTRY_FUNC uint32_t NS_ENTRY get_number_of_fw_valid_nsc(void){
	return get_number_of_fw_valid();
}
SECTION_NS_ENTRY_FUNC uint32_t NS_ENTRY get_cur_fw_idx_nsc(void){
	return get_cur_fw_idx();
}

SECTION_NS_ENTRY_FUNC void NS_ENTRY hal_sys_set_fast_boot_nsc(uint32_t pstart_tbl, uint32_t func_idx){
	hal_sys_set_fast_boot(pstart_tbl, func_idx);
}
#endif

/**
  * @brief  Clear OTA signature so that boot code load default image.
  * @retval none
  */
void sys_clear_ota_signature(void)
{
	flash_t	flash;
	uint32_t targetFWaddr;
	uint8_t *pbuf = NULL;
	uint32_t currentFWaddr;
	uint32_t fw1_sn;
	uint32_t fw2_sn;

	if(get_number_of_fw_valid()==1){
		printf("\n\rOnly one valid fw, no target fw to clear");
		return;
	}
	get_fw_info(&targetFWaddr, &currentFWaddr, &fw1_sn, &fw2_sn);
	//erase current FW signature to make it boot from another FW image
	targetFWaddr = currentFWaddr;

	printf("\n\rtarget/current FW addr = 0x%08X", targetFWaddr);
	
	pbuf = malloc(FLASH_SECTOR_SIZE);
	if(!pbuf){
		printf("\n\rAllocate buf fail");
		return;
	}
	
	// need to enter critical section to prevent executing the XIP code at first sector after we erase it.
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, targetFWaddr, FLASH_SECTOR_SIZE, pbuf);
	// NOT the first byte of ota signature to make it invalid
	pbuf[0] = ~(pbuf[0]);
	flash_erase_sector(&flash, targetFWaddr);
	flash_burst_write(&flash, targetFWaddr, FLASH_SECTOR_SIZE, pbuf);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	
	free(pbuf);
	printf("\n\rClear OTA signature success.");
}

/**
  * @brief  Recover OTA signature so that boot code load upgraded image(ota image).
  * @retval none
  */
void sys_recover_ota_signature(void)
{
	flash_t	flash;
	uint32_t targetFWaddr, currentFWaddr;
	uint8_t *pbuf = NULL;
	uint32_t fw1_sn;
	uint32_t fw2_sn;
	if(get_number_of_fw_valid()==2){
		printf("\n\rBoth fw valid, no target fw to recover");
		return;
	}
	get_fw_info(&targetFWaddr, &currentFWaddr, &fw1_sn, &fw2_sn);

	printf("\n\rcurrent FW addr = 0x%08X", currentFWaddr);
	printf("\n\rtarget  FW addr = 0x%08X", targetFWaddr);

	pbuf = malloc(FLASH_SECTOR_SIZE);
	if(!pbuf){
		printf("\n\rAllocate buf fail");
		return;
	}

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, targetFWaddr, FLASH_SECTOR_SIZE, pbuf);
	// NOT the first byte of ota signature to make it valid
	pbuf[0] = ~(pbuf[0]);
	flash_erase_sector(&flash, targetFWaddr);
	flash_burst_write(&flash, targetFWaddr, FLASH_SECTOR_SIZE, pbuf);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	
	free(pbuf);
	printf("\n\rRecover OTA signature success.");
}

/**
  * @brief  get current fw idx.
  * @retval idx
  */
uint32_t sys_update_ota_get_curr_fw_idx(void)
{
	return get_cur_fw_idx();
}

/**
  * @brief  get fw1 serial number.
  * @retval sn
  */
uint32_t sys_update_ota_get_fw1_sn(void)
{
	uint32_t targetFWaddr;
	uint32_t currentFWaddr;
	uint32_t fw1_sn;
	uint32_t fw2_sn;
	get_fw_info(&targetFWaddr, &currentFWaddr, &fw1_sn, &fw2_sn);
	return fw1_sn;
}

/**
  * @brief  get fw2 serial number.
  * @retval sn
  */
uint32_t sys_update_ota_get_fw2_sn(void)
{
	uint32_t targetFWaddr;
	uint32_t currentFWaddr;
	uint32_t fw1_sn;
	uint32_t fw2_sn;
	get_fw_info(&targetFWaddr, &currentFWaddr, &fw1_sn, &fw2_sn);
	return fw2_sn;
}

// for application assign the boot idx usage but not use sn as default.
int32_t sys_update_ota_set_boot_fw_idx(uint32_t boot_idx)
{
	flash_t	flash;
	uint32_t targetFWaddr;
	uint8_t *pbuf = NULL;
	uint8_t cur_idx;
	uint32_t currentFWaddr;
	uint32_t fw1_sn;
	uint32_t fw2_sn;
	cur_idx = get_cur_fw_idx();
	if(cur_idx == boot_idx) {
		return 0;
	}
	if(boot_idx != 1 && boot_idx != 2) {
		return -1;
	}
		
	get_fw_info(&targetFWaddr, &currentFWaddr, &fw1_sn, &fw2_sn);
	pbuf = rtw_zmalloc(FLASH_SECTOR_SIZE);
	if(!pbuf){
		printf("\n\rAllocate buf fail");
		return -1;
	}
	
	// need to enter critical section to prevent executing the XIP code at first sector after we erase it.
	rtw_enter_critical(NULL, NULL);
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, currentFWaddr, FLASH_SECTOR_SIZE, pbuf);
	// NOT the first byte of ota signature to make it invalid
	pbuf[0] = ~(pbuf[0]);
	flash_erase_sector(&flash, currentFWaddr);
	flash_burst_write(&flash, currentFWaddr, FLASH_SECTOR_SIZE, pbuf);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	rtw_exit_critical(NULL, NULL);
	rtw_free(pbuf);
	return 0; 
}

uint32_t sys_update_ota_prepare_addr(void)
{
	uint32_t NewFWAddr; 
	uint32_t targetFWaddr;
	uint32_t currentFWaddr;
	uint32_t fw1_sn;
	uint32_t fw2_sn;
	get_fw_info(&targetFWaddr, &currentFWaddr, &fw1_sn, &fw2_sn);
	NewFWAddr = targetFWaddr;
	printf("\n\r[%s]fw1 sn is %d, fw2 sn is %d\r\n", __FUNCTION__, fw1_sn,fw2_sn);
	printf("\n\r[%s] NewFWAddr %08X\n\r", __FUNCTION__, NewFWAddr);
	return NewFWAddr;
}

/**
  * @brief  disable system fast boot.
  * @retval none
  */
void sys_disable_fast_boot(void){
	hal_sys_set_fast_boot(NULL, 0);
}

/**
  * @brief  system software reset.
  * @retval none
  */
void sys_reset(void)
{
	sys_disable_fast_boot();
	hal_misc_rst_by_wdt();
}

void software_reset(void)
{
	sys_disable_fast_boot();
	hci_tp_close();  
	hal_wlan_pwr_off();
	__disable_irq();
	hal_misc_cpu_rst();
}
/**
  * @brief  Turn off the JTAG function.
  * @retval none
  */
void sys_jtag_off(void)
{
	hal_misc_jtag_pin_ctrl(0);
}

/**
  * @brief  open log uart.
  * @retval none
  */
void sys_log_uart_on(void)
{
	log_uart_port_init(STDIO_UART_TX_PIN, STDIO_UART_RX_PIN, ((uint32_t)115200));
}

/**
  * @brief  close log uart.
  * @retval none
  */
void sys_log_uart_off(void)
{
	log_uart_flush_wait();
	hal_gpio_pull_ctrl (log_uart.rx_pin, Pin_PullNone);
	hal_uart_deinit(&log_uart);
}
