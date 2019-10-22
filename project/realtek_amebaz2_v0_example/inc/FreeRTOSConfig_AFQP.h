/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock; 
#endif 

#define configUSE_PREEMPTION						1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION	1
#define configCPU_CLOCK_HZ							( SystemCoreClock )
#define configTICK_RATE_HZ							( ( uint32_t ) 1000 )
#define configMAX_PRIORITIES						( 11 )
#define configMINIMAL_STACK_SIZE					( ( unsigned short ) 1024 )
#define configMAX_TASK_NAME_LEN					( 10 )
#define configUSE_16_BIT_TICKS						0
#define configIDLE_SHOULD_YIELD						1
#define configUSE_TASK_NOTIFICATIONS				1
#define configUSE_MUTEXES							1
#define configUSE_RECURSIVE_MUTEXES				1
#define configUSE_COUNTING_SEMAPHORES 			1
#define configUSE_ALTERNATIVE_API 					0
#define configUSE_QUEUE_SETS                    			1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configSTACK_DEPTH_TYPE						uint16_t

/* Memory allocation related definitions. */
#define configTOTAL_HEAP_SIZE						( ( size_t ) ( 160 * 1024 ) )	// use HEAP5

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     				0
#define configUSE_TICK_HOOK                     				0
#define configCHECK_FOR_STACK_OVERFLOW			2
#if !defined(CONFIG_BUILD_SECURE) || (CONFIG_BUILD_SECURE == 0)
#define configUSE_MALLOC_FAILED_HOOK				1
#endif
#define configUSE_DAEMON_TASK_STARTUP_HOOK		0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS				0
#define configUSE_TRACE_FACILITY					0
#if configGENERATE_RUN_TIME_STATS
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() //( ulHighFrequencyTimerTicks = 0UL )
#define portGET_RUN_TIME_COUNTER_VALUE() xTickCount //ulHighFrequencyTimerTicks
#undef	configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY			1
#define portCONFIGURE_STATS_PEROID_VALUE	1000 //unit Ticks
#endif
/* This demo makes use of one or more example stats formatting functions.  These
format the raw data provided by the uxTaskGetSystemState() function in to human
readable ASCII form.  See the notes in the implementation of vTaskList() within
FreeRTOS/Source/tasks.c for limitations. */
#define configUSE_STATS_FORMATTING_FUNCTIONS	1

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES						1
#define configMAX_CO_ROUTINE_PRIORITIES			( 2 )

/* Software timer related definitions. */
#define configUSE_TIMERS							1
#define configTIMER_TASK_PRIORITY					( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH					10
#define configTIMER_TASK_STACK_DEPTH				( 512 )

/* Interrupt nesting behaviour configuration. */
/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
	/* __NVIC_PRIO_BITS will be specified when CMSIS is being used. */
#if __NVIC_PRIO_BITS != 3
	#error "__NVIC_PRIO_BITS is NOT correct for RTL8710C"
#endif
	#define configPRIO_BITS       		__NVIC_PRIO_BITS
#else
	#define configPRIO_BITS       		3        /* 8 priority levels */
	//#error "__NVIC_PRIO_BITS must be defined!"
#endif
/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY	0x07
/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	2
/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Define to trap errors during development. */
#if !defined(__IASMARM__)
#include "diag.h"
#include "platform_stdlib.h"
#include "unity_internals.h"                                                        
extern void cli(void);
#define configASSERT( x )    if( ( x ) == 0 )  TEST_ABORT()
/* Map the FreeRTOS printf() to the logging task printf. */
    /* The function that implements FreeRTOS printf style output, and the macro
     * that maps the configPRINTF() macros to that function. */
extern void vLoggingPrintf( const char * pcFormat, ... );
#define configPRINTF( X )    vLoggingPrintf X

/* Non-format version thread-safe print. */
extern void vLoggingPrint( const char * pcMessage );
#define configPRINT( X )     vLoggingPrint( X )

/* Map the logging task's printf to the board specific output function. */
#define configPRINT_STRING( x )    __wrap_printf( x )


/* Sets the length of the buffers into which logging messages are written - so
 * also defines the maximum length of each log message. */
#define configLOGGING_MAX_MESSAGE_LENGTH            512

/* Set to 1 to prepend each log message with a message number, the task name,
 * and a time stamp. */
#define configLOGGING_INCLUDE_TIME_AND_TASK_NAME    1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION              1
#define configUSE_MALLOC_FAILED_HOOK 1

#define configECHO_SERVER_ADDR0 (192)
#define configECHO_SERVER_ADDR1 (168)
#define configECHO_SERVER_ADDR2 (1)
#define configECHO_SERVER_ADDR3 (108)
#define configTCP_ECHO_CLIENT_PORT (8883)
#endif

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet					1
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete						1
#define INCLUDE_vTaskSuspend					1
#define INCLUDE_xResumeFromISR				1
#define INCLUDE_vTaskDelayUntil					1
#define INCLUDE_vTaskDelay						1
#define INCLUDE_xTaskGetSchedulerState			1
#define INCLUDE_xTaskGetCurrentTaskHandle		1
#define INCLUDE_uxTaskGetStackHighWaterMark	0
#define INCLUDE_xTaskGetIdleTaskHandle			0
#define INCLUDE_eTaskGetState					0
#define INCLUDE_xEventGroupSetBitFromISR		1
#define INCLUDE_xTimerPendFunctionCall			1
#define INCLUDE_xTaskAbortDelay				0
#define INCLUDE_xTaskGetHandle					0
#define INCLUDE_xTaskResumeFromISR			1

/* use the low power tickless mode */
#define configUSE_TICKLESS_IDLE                 0
#if defined(configUSE_TICKLESS_IDLE) && configUSE_TICKLESS_IDLE
#if !defined(__IASMARM__) || (__IASMARM__ != 1)
#if !defined(CONFIG_BUILD_SECURE) || (CONFIG_BUILD_SECURE == 0)
extern void freertos_pre_sleep_processing(unsigned int *expected_idle_time);
extern void freertos_post_sleep_processing(unsigned int *expected_idle_time);
extern int  freertos_ready_to_sleep(void);

/* Enable tickless power saving. */
#define configPRE_SUPPRESS_TICKS_AND_SLEEP_PROCESSING( x )  do { \
                                                                                						if (freertos_ready_to_sleep() == FALSE)  {\
																		x = 0;\
																	}\
                                                                                                             } while(0)

/* In wlan usage, this value is suggested to use value less than 80 milliseconds */
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   2

/* It's magic trick that let us can use our own sleep function */
#define configPRE_SLEEP_PROCESSING( x )        do { \
                                                                                freertos_pre_sleep_processing((unsigned int *)&x);  \
                                                                             } while(0)

#define configPOST_SLEEP_PROCESSING( x )        do { \
                                                                                freertos_post_sleep_processing((unsigned int *)&x);  \
                                                                             } while(0)

/* It's magic trick that let us can enable/disable tickless dynamically */
#define traceLOW_POWER_IDLE_BEGIN()            do { 

#define traceLOW_POWER_IDLE_END()              } while (0)

/* It's FreeRTOS related feature but it's not included in FreeRTOS design. */
#define configUSE_WAKELOCK_PMU                  1

#undef configMINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE		( ( unsigned short ) 1024 )
#endif
#endif // #if (__IASMARM__ != 1)
#endif // #if defined(configUSE_TICKLESS_IDLE) && configUSE_TICKLESS_IDLE

/* Add by Realtek to re-arrange the FreeRTOS priority*/
#define PRIORITIE_OFFSET				( 4 )

#endif /* FREERTOS_CONFIG_H */
