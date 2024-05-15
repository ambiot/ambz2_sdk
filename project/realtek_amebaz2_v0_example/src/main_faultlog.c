#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "main.h"
#include <example_entry.h>
#include <flash_api.h>
#include <device_lock.h>

extern void console_init(void);

#if defined(ENABLE_AMAZON_COMMON) || defined(CHIP_PROJECT)
static void *app_mbedtls_calloc_func(size_t nelements, size_t elementSize)
{
	size_t size;
	void *ptr = NULL;

	size = nelements * elementSize;
	ptr = pvPortMalloc(size);

	if (ptr) {
		memset(ptr, 0, size);
	}

	return ptr;
}

void app_mbedtls_init(void)
{
	mbedtls_platform_set_calloc_free(app_mbedtls_calloc_func, vPortFree);
}
#endif

// NOTE: this offset is safe on pure SDK, and may conflict with user's layout
//       please check layout to solve confliction before using
#define FAULT_LOG1				(0x200000 - 0x64000) //Store fault log(max size: 8K, 0xF64000~0xF66000)
#define FAULT_LOG2				(0x200000 - 0x66000) //Store fault log(max size: 8K, 0xF66000~0xF68000)
#define FAULT_FLASH_SECTOR_SIZE	(0x1000)
extern void fault_handler_override(void(*fault_log)(char *msg, int len), void(*bt_log)(char *msg, int len));
void fault_log(char *msg, int len)
{
	flash_t	fault_flash;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&fault_flash, FAULT_LOG1);
	flash_stream_write(&fault_flash, FAULT_LOG1, len, msg);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
}

void bt_log(char *msg, int len)
{
	flash_t	fault_flash;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&fault_flash, FAULT_LOG2);
	flash_stream_write(&fault_flash, FAULT_LOG2, len, msg);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
}

void read_last_fault_log(void)
{
	flash_t	fault_flash;
	uint8_t *log = malloc(FAULT_FLASH_SECTOR_SIZE);
	if (!log)	{
		return;
	}
	memset(log, 0xff, FAULT_FLASH_SECTOR_SIZE);
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&fault_flash, FAULT_LOG1, FAULT_FLASH_SECTOR_SIZE, log);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	if (log[0] != 0xFF) {
		dbg_printf("\n\r>>>>>>> Dump fault log <<<<<<<<\n\r");
		dbg_printf("%s", log);
	}

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&fault_flash, FAULT_LOG2, FAULT_FLASH_SECTOR_SIZE, log);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	if (log[0] != 0xFF) {
		dbg_printf("\n\r>>>>>>> Dump Backtrace log <<<<<<<<\n\r");
		dbg_printf("%s", log);
	}

	memset(log, 0xff, FAULT_FLASH_SECTOR_SIZE);
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&fault_flash, FAULT_LOG1);
	flash_erase_sector(&fault_flash, FAULT_LOG2);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	free(log);
}

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	/* Read last fault data and redeclare fault handler to use ram code */
	read_last_fault_log();
	fault_handler_override(fault_log, bt_log);

	/* Initialize log uart and at command service */
	console_init();

#if defined(ENABLE_AMAZON_COMMON) || defined(CHIP_PROJECT)
	/* Initial amazon mbedtls*/
	app_mbedtls_init();
#endif

	/* pre-processor of application example */
	pre_example_entry();

	/* wlan intialization */
	wlan_network();

	/* Execute application example */
	example_entry();

	/* Enable Schedule, Start Kernel */
	vTaskStartScheduler();

	/* Should NEVER reach here */
	return 0;
}
