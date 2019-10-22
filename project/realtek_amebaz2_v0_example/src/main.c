#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "main.h"
#include <example_entry.h>

extern void console_init(void);

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	/* Initialize log uart and at command service */
	console_init();

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