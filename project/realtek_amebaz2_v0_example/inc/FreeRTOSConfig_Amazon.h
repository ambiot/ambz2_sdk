/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

/******************************************************************************
	See http://www.freertos.org/a00110.html for an explanation of the
	definitions contained in this file.
******************************************************************************/

#ifndef FREERTOS_CONFIG_AMAZON_H
#define FREERTOS_CONFIG_AMAZON_H


/* FreeRTOS v10.2.0 config for Amazon usage */
#define configUSE_POSIX_ERRNO 1

#undef configMINIMAL_STACK_SIZE
#undef configMINIMAL_SECURE_STACK_SIZE
#undef configPRINTF
/* Constants that describe the hardware and memory usage. */
#define configMINIMAL_STACK_SIZE                    ( ( unsigned short ) 256 ) //number of double word
#define configMINIMAL_SECURE_STACK_SIZE         ( ( unsigned short ) 64*4 ) //number of byte

extern void cli(void);
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

#if defined(configENABLE_TRUSTZONE) && (configENABLE_TRUSTZONE == 1)
extern void rtw_create_secure_context(u32 secure_stack_size);
#endif

#endif /* FREERTOS_CONFIG_AMAZON_H */
