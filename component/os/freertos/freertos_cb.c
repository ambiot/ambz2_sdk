#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "platform_opts.h"

void vAssertCalled( uint32_t ulLine, const char *pcfile )
{
	// 
    volatile int lock_assert = 1;
    rt_printf("line %d file: %s\n\r", ulLine, pcfile);
	while(lock_assert);
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	asm(" nop");
    rt_printf("=== stack overflow === \n\r");
    rt_printf("Task name : %s, TCB : %x\n\r", pcTaskName, xTask);
    rt_printf("PSP %x PSPLIM %x\n\r", __get_PSP(), __get_PSPLIM());
    rt_printf("MSP %x MSPLIM %x\n\r", __get_MSP(), __get_MSPLIM());
    rt_printf("====================== \n\r");
#if (defined(CONFIG_MIIO)&&(CONFIG_MIIO))
	#define CRASH_FLAG_IN_FLASH    0xE32C04EDUL
	uint32_t crash_flag = CRASH_FLAG_IN_FLASH;
	extern int arch_psm_set_value(const char*, const char*, const void*, uint32_t);
	arch_psm_set_value("arch", "crash_flag", &crash_flag, sizeof(uint32_t));
#endif	

}

void vApplicationTickHook( void )
{
	asm(" nop");
}

void vApplicationMallocFailedHook( void )
{
    char *pcCurrentTask = "NoTsk";
    if( xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED )
    {
        pcCurrentTask = pcTaskGetName( NULL );
    }
    rt_printf( "[%s]Malloc failed [free heap size: %d]\r\n",  pcCurrentTask, xPortGetFreeHeapSize() );
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

// defined in port.c
void vPortUsageFaultHandler( void );
void osUsageFaultHook(void)
{
    vPortUsageFaultHandler();
}   
