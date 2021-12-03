
#include <os_sched.h>
#include <stdio.h>
#include <trace_app.h>
#include <hrp.h>
#include <hrp_application.h>
#include <gap_config.h>
#include <bte.h>
#include "trace_uart.h"

/*
#include <gap_msg.h>
#include <gap_legacy.h>
#include <gap_timer.h>

#include <module_bredr_profiles_init.h>
extern void ltp_TimerCallBack(void *xtimer);


void BtStack_Init_Gap(void)
{

#ifdef TEST_PROFILE
    module_bredr_profile_Init_Gap();
#endif

}

void Profile_Init(void)
{

#ifdef TEST_PROFILE
    module_bredr_profile_Profile_Init();
#endif

}
*/
/**
 * @brief  Config bt stack related feature
 *
 * NOTE: This function shall be called before @ref bte_init is invoked.
 * @return void
 */
void bt_stack_config_init(void)
{
    gap_config_max_le_link_num(APP_MAX_LINKS);
}

void ble_app_main(void)
{
    bt_trace_init();
    bt_stack_config_init();
    bte_init();
    hrp_init();
    hrp_task_init();

    return;
}



