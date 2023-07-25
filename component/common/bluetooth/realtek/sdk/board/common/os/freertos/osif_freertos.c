/**
 * Copyright (c) 2015, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include <FreeRTOS.h>

#include <task.h>
#include <timers.h>
#include <queue.h>
#include <semphr.h>

#include <osif.h>
#include "bt_board.h"
#if   defined ( __CC_ARM )
#include "cmsis_armcc.h"
/*
 * GNU Compiler
 */
#elif defined ( __GNUC__ )
#include "cmsis_gcc.h"
/*
 * IAR Compiler
 */
#elif defined ( __ICCARM__ )

#ifdef CONFIG_PLATFORM_8710C
#include "cmsis.h"
#else
#include "cmsis_iar.h"
#endif
#endif

#define CONFIG_OSIF_DEBUG        0

/****************************************************************************/
/* Check if in task context (true), or isr context (false)                  */
/****************************************************************************/
static inline bool osif_task_context_check(void)
{
    return (__get_IPSR() == 0);
}

/****************************************************************************/
/* Delay current task in a given milliseconds                               */
/****************************************************************************/
void osif_delay(uint32_t ms)
{
    vTaskDelay((TickType_t)((ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS));
}

/****************************************************************************/
/* Get system time in milliseconds                                          */
/****************************************************************************/
uint32_t osif_sys_time_get(void)
{
    if (osif_task_context_check() == true)
    {
        return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    }
    else
    {
        return (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    }
}

/****************************************************************************/
/* Start os kernel scheduler                                                */
/****************************************************************************/
bool osif_sched_start(void)
{
#if CONFIG_OSIF_DEBUG
    printf("%s enter\r\n", __FUNCTION__);
#endif
    vTaskStartScheduler();
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Stop os kernel scheduler                                                 */
/****************************************************************************/
bool osif_sched_stop(void)
{
#if CONFIG_OSIF_DEBUG
    printf("%s enter\r\n", __FUNCTION__);
#endif
    vTaskEndScheduler();
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Suspend os kernel scheduler                                              */
/****************************************************************************/
bool osif_sched_suspend(void)
{
#if CONFIG_OSIF_DEBUG
    printf("%s enter\r\n", __FUNCTION__);
#endif
    vTaskSuspendAll();
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Resume os kernel scheduler                                               */
/****************************************************************************/
bool osif_sched_resume(void)
{
#if CONFIG_OSIF_DEBUG
    printf("%s enter\r\n", __FUNCTION__);
#endif
    xTaskResumeAll();
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Create os level task routine                                             */
/****************************************************************************/
bool osif_task_create(void **pp_handle, const char *p_name, void (*p_routine)(void *),
                      void *p_param, uint16_t stack_size, uint16_t priority)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x p_name %s p_routine 0x%x p_param 0x%x stack_size %d priority %d\r\n", __FUNCTION__,
                                                    pp_handle, p_name, p_routine, p_param, stack_size, priority);
#endif

    BaseType_t ret;

    if (pp_handle == NULL)
    {
        printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    ret = xTaskCreate(p_routine, (const char *)p_name, stack_size / sizeof(portSTACK_TYPE),
                      p_param, priority, (TaskHandle_t *)pp_handle);
    if (ret == pdPASS)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s *pp_handle 0x%x\r\n", __FUNCTION__, *pp_handle);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Delete os level task routine                                             */
/****************************************************************************/
bool osif_task_delete(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    vTaskDelete((TaskHandle_t)p_handle);
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Suspend os level task routine                                            */
/****************************************************************************/
bool osif_task_suspend(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    vTaskSuspend((TaskHandle_t)p_handle);
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Resume os level task routine                                             */
/****************************************************************************/
bool osif_task_resume(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    vTaskResume((TaskHandle_t)p_handle);
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Yield current os level task routine                                      */
/****************************************************************************/
bool osif_task_yield(void)
{
#if CONFIG_OSIF_DEBUG
    printf("%s enter\r\n", __FUNCTION__);
#endif
    taskYIELD();
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Get current os level task routine handle                                 */
/****************************************************************************/
bool osif_task_handle_get(void **pp_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x\r\n", __FUNCTION__, pp_handle);
#endif
    if (pp_handle == NULL)
    {
        printf("%s fail!(*pp_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    *pp_handle = (void *)xTaskGetCurrentTaskHandle();
#if CONFIG_OSIF_DEBUG
    printf("%s *pp_handle 0x%x\r\n", __FUNCTION__, *pp_handle);
#endif
    return true;
}

/****************************************************************************/
/* Get os level task routine priority                                       */
/****************************************************************************/
bool osif_task_priority_get(void *p_handle, uint16_t *p_priority)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x, p_priority 0x%x\r\n", __FUNCTION__, p_handle, p_priority);
#endif
    if (p_priority == NULL)
    {
        printf("%s fail!(p_priority == NULL)\r\n", __FUNCTION__);
        return false;
    }

    *p_priority = uxTaskPriorityGet((TaskHandle_t)p_handle);
#if CONFIG_OSIF_DEBUG
    printf("%s *p_priority 0x%x\r\n", __FUNCTION__, *p_priority);
#endif
    return true;
}

/****************************************************************************/
/* Set os level task routine priority                                       */
/****************************************************************************/
bool osif_task_priority_set(void *p_handle, uint16_t priority)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x priority %d\r\n", __FUNCTION__, p_handle, priority);
#endif
    vTaskPrioritySet((TaskHandle_t)p_handle, priority);
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

static void *sig_handle;
bool osif_signal_init(void)
{
    sig_handle = (void *)xSemaphoreCreateCounting(1, 0);
    if (sig_handle != NULL)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s sig_handle 0x%x\r\n", __FUNCTION__, sig_handle);
#endif
        return true;
    }
    else
    {
        printf("%s fail! sig_handle = NULL\r\n", __FUNCTION__);
        return false;
    }
}

void osif_signal_deinit(void)
{
#if CONFIG_OSIF_DEBUG
    printf("%s sig_handle 0x%x\r\n", __FUNCTION__, sig_handle);
#endif
    if (sig_handle != NULL)
    {
        vSemaphoreDelete(sig_handle);
        sig_handle = NULL;
    }
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
}

/****************************************************************************/
/* Send signal to target task                                               */
/****************************************************************************/
bool osif_task_signal_send(void *p_handle, uint32_t signal)
{
#if CONFIG_OSIF_DEBUG
    printf("%s sig_handle 0x%x\r\n", __FUNCTION__, sig_handle);
#endif
    if (!sig_handle)
    {
        printf("osif_task_signal_send: sig_handle is null");
        return false;
    }



    if (osif_task_context_check() == true)
    {
        xSemaphoreGive((QueueHandle_t)sig_handle);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        xSemaphoreGiveFromISR((QueueHandle_t)sig_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;

}

/****************************************************************************/
/* Receive signal in target task                                            */
/****************************************************************************/
bool osif_task_signal_recv(uint32_t *p_handle, uint32_t wait_ms)
{
#if CONFIG_OSIF_DEBUG
    printf("%s sig_handle 0x%x wait_ms 0x%x\r\n", __FUNCTION__, sig_handle, wait_ms);
#endif
    BaseType_t ret;
    TickType_t wait_ticks;

    if (!sig_handle)
    {
        printf("osif_task_signal_recv: sig_handle is null");
        return false;
    }

    if (wait_ms == 0xFFFFFFFFUL)
    {
        wait_ticks = portMAX_DELAY;
    }
    else
    {
        wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
    }

    if (osif_task_context_check() == true)
    {
        ret = xSemaphoreTake((QueueHandle_t)sig_handle, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xSemaphoreTakeFromISR((QueueHandle_t)sig_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Clear signal in target task                                              */
/****************************************************************************/
bool osif_task_signal_clear(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x...\r\n", __FUNCTION__, p_handle);
#endif
    BaseType_t ret;

#ifndef ENABLE_AMAZON_COMMON
extern BaseType_t xTaskNotifyStateClear( TaskHandle_t xTask );
#endif
    ret = xTaskNotifyStateClear((TaskHandle_t)p_handle);
    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Lock critical section                                                    */
/****************************************************************************/
uint32_t osif_lock(void)
{
    uint32_t flags = 0U;


    if (osif_task_context_check() == true)
    {
        taskENTER_CRITICAL();
    }
    else
    {
        printf("warn: unexpected isr mode\r\n");
        //flags = taskENTER_CRITICAL_FROM_ISR();
    }

    return flags;
}

/****************************************************************************/
/* Unlock critical section                                                  */
/****************************************************************************/
void osif_unlock(uint32_t flags)
{

    if (osif_task_context_check() == true)
    {
        taskEXIT_CRITICAL();
    }
    else
    {
        printf("warn: unexpected isr mode\r\n");
        //taskEXIT_CRITICAL_FROM_ISR(flags);
    }

}

/****************************************************************************/
/* Create counting semaphore                                                */
/****************************************************************************/
bool osif_sem_create(void **pp_handle, uint32_t init_count, uint32_t max_count)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x init_count %d max_count %d\r\n", __FUNCTION__, pp_handle, init_count, max_count);
#endif
    if (pp_handle == NULL)
    {
        printf("%s fail!(pp_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    *pp_handle = (void *)xSemaphoreCreateCounting(max_count, init_count);
    if (*pp_handle != NULL)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s *pp_handle 0x%x\r\n", __FUNCTION__, *pp_handle);
#endif
        return true;
    }
    else
    {
        printf("%s fail! *pp_handle = NULL\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Delete counting semaphore                                                */
/****************************************************************************/
bool osif_sem_delete(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    vSemaphoreDelete((QueueHandle_t)p_handle);
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Take counting semaphore                                                  */
/****************************************************************************/
bool osif_sem_take(void *p_handle, uint32_t wait_ms)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x wait_ms 0x%x\r\n", __FUNCTION__, p_handle, wait_ms);
#endif
    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        ret = xSemaphoreTake((QueueHandle_t)p_handle, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xSemaphoreTakeFromISR((QueueHandle_t)p_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Give counting semaphore                                                  */
/****************************************************************************/
bool osif_sem_give(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    BaseType_t ret;

    if (osif_task_context_check() == true)
    {
        ret = xSemaphoreGive((QueueHandle_t)p_handle);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xSemaphoreGiveFromISR((QueueHandle_t)p_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Create recursive mutex                                                   */
/****************************************************************************/
bool osif_mutex_create(void **pp_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x\r\n", __FUNCTION__, pp_handle);
#endif
    if (pp_handle == NULL)
    {
        printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    *pp_handle = (void *)xSemaphoreCreateRecursiveMutex();
    if (*pp_handle != NULL)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s *pp_handle 0x%x\r\n", __FUNCTION__, *pp_handle);
#endif
        return true;
    }
    else
    {
        printf("%s fail! *pp_handle = NULL\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Delete recursive mutex                                                   */
/****************************************************************************/
bool osif_mutex_delete(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    if (xSemaphoreGetMutexHolder((QueueHandle_t)p_handle) == NULL)
    {
        vSemaphoreDelete((QueueHandle_t)p_handle);
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        /* Do not delete mutex if held by a task */
        printf("%s Do not delete mutex because it held by a task\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Take recursive mutex                                                     */
/****************************************************************************/
bool osif_mutex_take(void *p_handle, uint32_t wait_ms)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x wait_ms 0x%x\r\n", __FUNCTION__, p_handle, wait_ms);
#endif
    TickType_t wait_ticks;
    BaseType_t ret;

    if (wait_ms == 0xFFFFFFFFUL)
    {
        wait_ticks = portMAX_DELAY;
    }
    else
    {
        wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
    }

    ret = xSemaphoreTakeRecursive((QueueHandle_t)p_handle, wait_ticks);
    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Give recursive mutex                                                     */
/****************************************************************************/
bool osif_mutex_give(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    BaseType_t ret;

    ret = xSemaphoreGiveRecursive((QueueHandle_t)p_handle);
    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Create inter-thread message queue                                        */
/****************************************************************************/
bool osif_msg_queue_create(void **pp_handle, uint32_t msg_num, uint32_t msg_size)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x msg_num %d msg_size %d\r\n", __FUNCTION__, pp_handle, msg_num, msg_size);
#endif
    if (pp_handle == NULL)
    {
        printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    *pp_handle = (void *)xQueueCreate(msg_num, msg_size);

    if (*pp_handle != NULL)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s *pp_handle 0x%x\r\n", __FUNCTION__, *pp_handle);
#endif
        return true;
    }
    else
    {
        printf("%s fail! *pp_handle = NULL\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Delete inter-thread message queue                                        */
/****************************************************************************/
bool osif_msg_queue_delete(void *p_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x\r\n", __FUNCTION__, p_handle);
#endif
    if (p_handle == NULL)
    {
        printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }
    vQueueDelete((QueueHandle_t)p_handle);
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Peek inter-thread message queue's pending but not received msg number    */
/****************************************************************************/
bool osif_msg_queue_peek(void *p_handle, uint32_t *p_msg_num)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x p_msg_num 0x%x\r\n", __FUNCTION__, p_handle, p_msg_num);
#endif
    if (p_handle == NULL || p_msg_num == NULL )
    {
        printf("%s fail!(p_handle == NULL || p_msg_num == NULL)\r\n", __FUNCTION__);
        return false;
    }
    if (osif_task_context_check() == true)
    {
        *p_msg_num = (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)p_handle);
    }
    else
    {
        *p_msg_num = (uint32_t)uxQueueMessagesWaitingFromISR((QueueHandle_t)p_handle);
    }
#if CONFIG_OSIF_DEBUG
    printf("%s *p_msg_num 0x%x\r\n", __FUNCTION__, *p_msg_num);
#endif
    return true;
}

/****************************************************************************/
/* Send inter-thread message                                                */
/****************************************************************************/
bool osif_msg_send(void *p_handle, void *p_msg, uint32_t wait_ms)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x p_msg 0x%x wait_ms 0x%x\r\n", __FUNCTION__, p_handle, p_msg, wait_ms);
#endif
    BaseType_t ret;

    if (p_handle == NULL)
    {
        printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }
        ret = xQueueSendToBack((QueueHandle_t)p_handle, p_msg, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xQueueSendToBackFromISR((QueueHandle_t)p_handle, p_msg, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Receive inter-thread message                                             */
/****************************************************************************/
bool osif_msg_recv(void *p_handle, void *p_msg, uint32_t wait_ms)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x p_msg 0x%x wait_ms 0x%x\r\n", __FUNCTION__, p_handle, p_msg, wait_ms);
#endif
    BaseType_t ret;
    if (p_handle == NULL)
    {
        printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }
    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        ret = xQueueReceive((QueueHandle_t)p_handle, p_msg, wait_ticks);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xQueueReceiveFromISR((QueueHandle_t)p_handle, p_msg, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************/
/* Peek inter-thread message                                                */
/****************************************************************************/
bool osif_msg_peek(void *p_handle, void *p_msg, uint32_t wait_ms)
{
#if CONFIG_OSIF_DEBUG
    printf("%s p_handle 0x%x p_msg 0x%x wait_ms 0x%x\r\n", __FUNCTION__, p_handle, p_msg, wait_ms);
#endif
    BaseType_t ret;
    if (p_handle == NULL)
    {
        printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }
    if (osif_task_context_check() == true)
    {
        TickType_t wait_ticks;

        if (wait_ms == 0xFFFFFFFFUL)
        {
            wait_ticks = portMAX_DELAY;
        }
        else
        {
            wait_ticks = (TickType_t)((wait_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        ret = xQueuePeek((QueueHandle_t)p_handle, p_msg, wait_ticks);
    }
    else
    {
        ret = xQueuePeekFromISR((QueueHandle_t)p_handle, p_msg);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Allocate memory                                                          */
/****************************************************************************/
void *osif_mem_alloc(RAM_TYPE ram_type, size_t size)
{
    (void) ram_type;
    return pvPortMalloc(size);
}

/****************************************************************************/
/* Allocate aligned memory                                                  */
/****************************************************************************/
void *osif_mem_aligned_alloc(RAM_TYPE ram_type, size_t size, uint8_t alignment)
{
    (void) ram_type;
    void *p;
    void *p_aligned;

    if (alignment == 0)
    {
        alignment = portBYTE_ALIGNMENT;
    }

    p = pvPortMalloc(size + sizeof(void *) + alignment);
    if (p == NULL)
    {
        printf("%s fail!(p == NULL)\r\n", __FUNCTION__);
        return p;
    }

    p_aligned = (void *)(((size_t)p + sizeof(void *) + alignment) & ~(alignment - 1));

    memcpy((uint8_t *)p_aligned - sizeof(void *), &p, sizeof(void *));

    return p_aligned;
}

/****************************************************************************/
/* Free memory                                                              */
/****************************************************************************/
void osif_mem_free(void *p_block)
{
    if (p_block == NULL)
    {
        printf("%s fail!(p_block == NULL)\r\n", __FUNCTION__);
        return;
    }
    vPortFree(p_block);
}

/****************************************************************************/
/* Free aligned memory                                                      */
/****************************************************************************/
void osif_mem_aligned_free(void *p_block)
{
    void *p;
    if (p_block == NULL)
    {
        printf("%s fail!(p_block == NULL)\r\n", __FUNCTION__);
        return;
    }
    memcpy(&p, (uint8_t *)p_block - sizeof(void *), sizeof(void *));

    vPortFree(p);
}

/****************************************************************************/
/* Peek unused (available) memory size                                    */
/****************************************************************************/
size_t osif_mem_peek(RAM_TYPE ram_type)
{
    (void) ram_type;
    return xPortGetFreeHeapSize();
}

/****************************************************************************/
/* Get software timer ID                                                    */
/****************************************************************************/
bool osif_timer_id_get(void **pp_handle, uint32_t *p_timer_id)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x p_timer_id 0x%x\r\n", __FUNCTION__, pp_handle, p_timer_id);
#endif
    if (pp_handle == NULL || *pp_handle == NULL)
    {
        printf("%s fail!(pp_handle == NULL || *pp_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    *p_timer_id = (uint32_t)pvTimerGetTimerID((TimerHandle_t) * pp_handle);
#if CONFIG_OSIF_DEBUG
    printf("%s *p_timer_id %d\r\n", __FUNCTION__, *p_timer_id);
#endif
    return true;
}


/****************************************************************************/
/* Create software timer                                                    */
/****************************************************************************/
bool osif_timer_create(void **pp_handle, const char *p_timer_name, uint32_t timer_id,
                       uint32_t interval_ms, bool reload, void (*p_timer_callback)(void *))
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x p_timer_name %s timer_id %d interval_ms %d reload %d p_timer_callback 0x%x\r\n", __FUNCTION__,                                                     pp_handle, p_timer_name, timer_id, interval_ms, reload, p_timer_callback);
#endif
    TickType_t timer_ticks;

    if (pp_handle == NULL || p_timer_callback == NULL)
    {
        printf("%s fail!(pp_handle == NULL || p_timer_callback == NULL)\r\n", __FUNCTION__);
        return false;
    }

    timer_ticks = (TickType_t)((interval_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);

    if (*pp_handle == NULL)
    {
        *pp_handle = xTimerCreate(p_timer_name, timer_ticks, (BaseType_t)reload,
                                  (void *)timer_id, (TimerCallbackFunction_t)p_timer_callback);
#if CONFIG_OSIF_DEBUG
    printf("%s *pp_handle 0x%x\r\n", __FUNCTION__, *pp_handle);
#endif
        if (*pp_handle == NULL)
        {
            printf("%s fail!(p_handle == NULL)\r\n", __FUNCTION__);
            return false;
        }
    }
    else
    {
        printf("%s fail!(p_handle != NULL)\r\n", __FUNCTION__);
        return false;
    }
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}

/****************************************************************************/
/* Start software timer                                                     */
/****************************************************************************/
bool osif_timer_start(void **pp_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x *pp_handle 0x%x\r\n", __FUNCTION__, pp_handle, *pp_handle);
#endif
    BaseType_t ret;

    if (pp_handle == NULL || *pp_handle == NULL)
    {
        printf("%s fail!(pp_handle == NULL || *pp_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    if (osif_task_context_check() == true)
    {
        ret = xTimerStart((TimerHandle_t) * pp_handle, (TickType_t)0);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xTimerStartFromISR((TimerHandle_t) * pp_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__, ret);
        return false;
    }
}

/****************************************************************************/
/* Restart software timer                                                   */
/****************************************************************************/
bool osif_timer_restart(void **pp_handle, uint32_t interval_ms)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x *pp_handle 0x%x interval_ms %d\r\n", __FUNCTION__, pp_handle, *pp_handle, interval_ms);
#endif
    TickType_t timer_ticks;
    BaseType_t ret;

    if (pp_handle == NULL || *pp_handle == NULL)
    {
        printf("%s fail!(pp_handle == NULL || *pp_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    timer_ticks = (TickType_t)((interval_ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);

    if (osif_task_context_check() == true)
    {
        ret = xTimerChangePeriod((TimerHandle_t) * pp_handle, timer_ticks, (TickType_t)0);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xTimerChangePeriodFromISR((TimerHandle_t) * pp_handle, timer_ticks, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Stop software timer                                                      */
/****************************************************************************/
bool osif_timer_stop(void **pp_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x *pp_handle 0x%x\r\n", __FUNCTION__, pp_handle, *pp_handle);
#endif
    BaseType_t ret;

    if (pp_handle == NULL || *pp_handle == NULL)
    {
        printf("%s fail!(pp_handle == NULL || *pp_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    if (osif_task_context_check() == true)
    {
        ret = xTimerStop((TimerHandle_t) * pp_handle, (TickType_t)0);
    }
    else
    {
        BaseType_t task_woken = pdFALSE;

        ret = xTimerStopFromISR((TimerHandle_t) * pp_handle, &task_woken);

        portEND_SWITCHING_ISR(task_woken);
    }

    if (ret == pdTRUE)
    {
#if CONFIG_OSIF_DEBUG
        printf("%s exit\r\n", __FUNCTION__);
#endif
        return true;
    }
    else
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }
}

/****************************************************************************/
/* Delete software timer                                                    */
/****************************************************************************/
bool osif_timer_delete(void **pp_handle)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x *pp_handle 0x%x\r\n", __FUNCTION__, pp_handle, *pp_handle);
#endif
    if (pp_handle == NULL || *pp_handle == NULL)
    {
        return false;
    }

    if (xTimerDelete((TimerHandle_t)*pp_handle, (TickType_t)0) == pdFAIL)
    {
        printf("%s fail!\r\n", __FUNCTION__);
        return false;
    }

    *pp_handle = NULL;
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}
/****************************************************************************/
/* get timer state                                                          */
/****************************************************************************/
bool osif_timer_state_get(void **pp_handle, uint32_t *p_timer_state)
{
#if CONFIG_OSIF_DEBUG
    printf("%s pp_handle 0x%x p_timer_state 0x%x\r\n", __FUNCTION__, pp_handle, p_timer_state);
#endif
    if (pp_handle == NULL || *pp_handle == NULL)
    {
        printf("%s fail!(pp_handle == NULL || *pp_handle == NULL)\r\n", __FUNCTION__);
        return false;
    }

    if (osif_task_context_check() == true)
    {
        *p_timer_state = (uint32_t)xTimerIsTimerActive((TimerHandle_t) * pp_handle);
    }
    else
    {
        //*p_timer_state = (uint32_t)xTimerIsTimerActiveFromISR((TimerHandle_t) * pp_handle);
    }
#if CONFIG_OSIF_DEBUG
    printf("%s exit\r\n", __FUNCTION__);
#endif
    return true;
}
/****************************************************************************/
/* Dump software timer                                                      */
/****************************************************************************/
bool osif_timer_dump(void)
{
    //dumpAllUsedTimer();
    return true;
}

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
extern void rtw_create_secure_context(u32 secure_stack_size);
#endif
void osif_create_secure_context(uint32_t size)
{
#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
    rtw_create_secure_context(size);
#endif    
}
