/**************************************************************************//**
 * @file     main.c
 * @brief    main function example.
 * @version  V1.00
 * @date     2016-06-08
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2016 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 ******************************************************************************/

#include "cmsis.h"
#include "shell.h"
#include "cmsis_os.h"               // CMSIS RTOS header file
#include <math.h>
#include "hal_timer.h"
#include "string.h"
#include "crypto_api.h"


#define DEBUG_LOG_BUF_SIZE      256

log_buf_type_t debug_log;
char debug_log_buf[DEBUG_LOG_BUF_SIZE];

hal_timer_adapter_t test_timer;

#if CONFIG_CMSIS_RTX_EN | CONFIG_CMSIS_FREERTOS_EN
void cmd_shell_task (void const *argument);         // thread function
void log_buf_task (void const *argument);         // thread function

osThreadId tid_cmd_shell;                             // thread id
osThreadDef (cmd_shell_task, osPriorityNormal, 1, 2048);      // thread object

osThreadId tid_log_buf;                             // thread id
osThreadDef (log_buf_task, osPriorityNormal, 1, 0);      // thread object
#endif

#if defined(CONFIG_BUILD_NONSECURE) && (CONFIG_BUILD_NONSECURE==1)
extern s32 cmd_dump_word_s(u32 argc, u8  *argv[]);
extern s32 cmd_dump_byte_s(u32 argc, u8 *argv[]);
extern s32 cmd_dump_helfword_s(u32 argc, u8  *argv[]);
extern s32 cmd_write_byte_s(u32 argc, u8  *argv[]);
extern s32 cmd_write_word_s(u32 argc, u8  *argv[]);
#endif

int test_command1 (int argc, char** argv)
{
    int i;

    dbg_printf("\r\n%s ==>\r\n", __FUNCTION__);
    dbg_printf("argc = %d\r\n", argc);
    for (i = 0; i < argc; i++) {
      dbg_printf("argv[%d]=%s\r\n", i, argv[i]);
    }

    log_printf (&debug_log, "%s <==\r\n", __FUNCTION__);
    return 0;
}

void timer_test_callback (void *hid)
{
    dbg_printf ("sys_time: %lu\r\n", hal_read_systime_us ());
}

void timer_test_callback2 (void *hid)
{
    dbg_printf ("timer_test_callback2==>\r\n");
    hal_timer_init (&test_timer, 1);
    hal_timer_start_periodical (&test_timer, 1000000, (timer_callback_t)timer_test_callback, (void *)&test_timer);
}

int test_command2 (int argc, char** argv)
{
    uint32_t ret;

    // start a timer for test
    ret = hal_timer_init (&test_timer, 2);
    hal_timer_start_one_shot (&test_timer, 1000000, (timer_callback_t)timer_test_callback2, (void *)&test_timer);
//    hal_timer_init (&test_timer, 1);
//    hal_timer_start_periodical (&test_timer, 1000000, (timer_callback_t)timer_test_callback, (void *)&test_timer);
    return ret;
}

int test_command3 (int argc, char** argv)
{
    static int value;
    static void * ptr = &value;
    int stackValue;
    void * stackPtr = &stackValue;
    char buf[100];

    dbg_printf("Hello world {%u}\r\n",sizeof(unsigned long));
    dbg_printf("Hello %s\r\n","World");
    dbg_printf("String %4.4s\r\n","Large");
    dbg_printf("String %*.*s\r\n",4,4,"Hello");
    dbg_printf("integer %05d %+d %d %2d %5d\r\n",-7,7,-7,1234,1234);
    dbg_printf("Integer %+05d %-5d % 5d %05d\r\n",1234,1234,1234,1234);
    dbg_printf("Integer blank % d % d\r\n",1,-1);
    dbg_printf("Unsigned %u %lu\r\n",123,123Lu);
    dbg_printf("Hex with prefix %#x %#x %#X %#08x\r\n",0,1,2,12345678);
    dbg_printf("Octal %o %lo\r\n",123,123456L);
    dbg_printf("Octal with prefix %#o %#o\r\n",0,5);
    dbg_printf("Hex %x %X %lX\r\n",0x1234,0xf0ad,0xf2345678L);
    dbg_printf("Special char %%\r\n");
    dbg_printf("Size    of void * %u(%u)\r\n",(size_t)sizeof(void *),(size_t)sizeof(void *));
    dbg_printf("Sizeof char=%d short=%d int=%d long=%d void*=%u size_t=%u float=%u double=%u\r\n",
               sizeof(char),sizeof(short),sizeof(int),sizeof(long),sizeof(void *),sizeof(size_t), sizeof(float), sizeof(double));

    dbg_printf("Floating %f\r\n",-0.6);
    dbg_printf("Floating %6.2f\r\n",22.0/7.0);
    dbg_printf("Floating %6.2f\r\n",-22.0/7.0);
    dbg_printf("Floating %+6.1f %6.2f\r\n",3.999,-3.999);
    dbg_printf("Floating %6.1f %6.0f\r\n",3.999,-3.999);
    dbg_printf("Floating %5.0f\r\n",3.14);

    dbg_printf("*Sizeof of void * %zu\r\n",sizeof(void *));
    dbg_printf("*Binary number %b %#b\r\n",5,6);
    dbg_printf("*Stack  ptr %p %P\r\n",stackPtr,stackPtr);
    dbg_printf("*Static ptr %p %P\r\n",ptr,ptr);
    dbg_printf("*Text   ptr %p %P\r\n",test_command3, test_command3);
    dbg_printf("*boolean %B %B\r\n",1,0);
    dbg_printf("*Text pointer as sizeof %zX\r\n",test_command3);

    dbg_printf("long long int %lld\r\n",(long long)123);
    dbg_printf("long long int %lld\r\n",(long long)-123);
    dbg_printf("long long hex %#llx\r\n",(long long)0x123456789abcdef);
    dbg_printf("long long hex %#llX\r\n",(long long)0x123456789abcdef);
    dbg_sprintf(buf, "Floating > 32 bit %f\r\n",pow(2.0,32.0)+1.0);
    log_printf(&debug_log, "==>1 %s\r\n", buf);
    dbg_sprintf(buf, "Floating < 32 bit %f\r\n",-pow(2.0,32.0)-1.0);
    log_printf(&debug_log, "==>2 %s\r\n", buf);

    return 0;

}


#if CONFIG_CMSIS_RTX_EN | CONFIG_CMSIS_FREERTOS_EN
void cmd_shell_task (void const *argument)
{
    dbg_printf("cmd_shell_task==>\r\n");
    while (1) {
        shell_task ();
    }
}

void log_buf_task (void const *argument)
{
    dbg_printf("log_buf_task==>\r\n");
    while (1) {
        log_buf_show (&debug_log);
    }
}
#endif  // #if CONFIG_RTOS_RTX_EN

int vrf_crypto_rng (int argc, char** argv)
{
    int ret;
    uint32_t rng_buf;

#if !defined(CONFIG_BUILD_NONSECURE)    
    static uint32_t crypto_inited = 0;
    if (crypto_inited == 0) {
        ret = crypto_init();
        if (ret != SUCCESS) {
            dbg_printf("crypto_init fail!%d\r\n",ret);
            goto verify_crypto_rng_end;
        } else {
            crypto_inited = 1;
            dbg_printf("crypto_init success!\r\n");
        }
    }
#endif
    while(1) {        
        ret = crypto_random_generate(&rng_buf,4);
        if (ret != SUCCESS) {
            dbg_printf("crypto_random_generate fail! ret = %d\r\n",ret);
            break;
        } else {
            dbg_printf("0x%08x\r\n",rng_buf);
        }
    }
verify_crypto_rng_end:
    return ret;
}

/// default main
int main (void)
{
    uint32_t ret;

    log_buf_init (&debug_log);
    log_buf_set_msg_buf (&debug_log, debug_log_buf, DEBUG_LOG_BUF_SIZE);

    // register new shell command
#if defined(CONFIG_BUILD_NONSECURE) && (CONFIG_BUILD_NONSECURE==1)
    // Secure entry function demo
    shell_register((shell_program_t)cmd_dump_byte_s, "DBS", "\t Dump Secure memory in byte\r\n");
    shell_register((shell_program_t)cmd_dump_helfword_s, "DHWS", "\t Dump Secure memory in half word\r\n");
    shell_register((shell_program_t)cmd_dump_word_s, "DWS", "\t Dump Secure memory in word\r\n");
    shell_register((shell_program_t)cmd_write_byte_s, "EBS", "\t Write Secure memory by byte\r\n");
    shell_register((shell_program_t)cmd_write_word_s, "EWS", "\t Write Secure memory by word\r\n");
#endif
    shell_register((shell_program_t)test_command1, "test1", "\t This is a test command\r\n");
    shell_register((shell_program_t)test_command2, "test2", "\t This is a GTimer test command\r\n");
    shell_register((shell_program_t)test_command3, "test3", "\t printf test\r\n");
    shell_register((shell_program_t)vrf_crypto_rng, "vrf_rng", "\t verify crypto_rng\r\n");

#if CONFIG_CMSIS_RTX_EN | CONFIG_CMSIS_FREERTOS_EN
    ret = osKernelInitialize ();                    // initialize CMSIS-RTOS
    if (ret != osOK) {
        dbg_printf ("KernelInitErr(0x%x)\r\n", ret);
        while(1);
    }

    // initialize peripherals here
    // create 'thread' functions that start executing,
    // example: tid_name = osThreadCreate (osThread(name), NULL);
    tid_cmd_shell = osThreadCreate (osThread(cmd_shell_task), NULL);
    if (tid_cmd_shell == 0) {
        dbg_printf ("Create shell task error\r\n");
    }

    tid_log_buf = osThreadCreate (osThread(log_buf_task), NULL);
    if (tid_cmd_shell == 0) {
        dbg_printf ("Create log buffer task error\r\n");
    }
    ret = osKernelStart ();                         // start thread execution
    while(1);
#else
    while (1) {
        shell_task ();
        log_buf_show (&debug_log);
    }
#endif
    return 0;
}

