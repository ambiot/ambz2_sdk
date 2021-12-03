#if F_BT_DLPS_EN

#include <hrp_dlps.h>
#include <board.h>
#include <rtl876x_pinmux.h>
#include "rtl876x_io_dlps.h"
#include <dlps.h>
#include <trace_app.h>


HRP_DLPS_STATUS hrp_dlps_status = HRP_DLPS_STATUS_IDLE;
bool can_enter_dlps = true;
uint32_t enter_dlps_count = 0;


void data_uart_dlps_exit_cb(void)
{
#if 0
    //TODO(cheer): remove log later
    DBG_DIRECT("data_uart_dlps_exit_cb mode %d", lps_mode_get());
    Pad_ControlSelectValue(DATA_UART_TX_PIN, PAD_PINMUX_MODE);
    Pad_ControlSelectValue(DATA_UART_RX_PIN, PAD_PINMUX_MODE);
#endif
}

void data_uart_dlps_enter_cb(void)
{
#if 0
    //TODO(cheer): remove log later
    enter_dlps_count++ ;
    DBG_DIRECT("data_uart_dlps_enter_cb mode %d", lps_mode_get());

    Pad_ControlSelectValue(DATA_UART_TX_PIN, PAD_SW_MODE);
    Pad_ControlSelectValue(DATA_UART_RX_PIN, PAD_SW_MODE);
    System_WakeUpPinEnable(DATA_UART_RX_PIN, PAD_WAKEUP_POL_LOW, 0);
#endif

}
#if 0
bool data_uart_dlps_check_cb(void)
{
#if 1
    //TODO(cheer): remove log later
    static int i = 0;

    if (i == 2000)
    {
        DBG_DIRECT("data_uart_dlps_check_cb %x", can_enter_dlps);
        i = 0;
    }
    i++;
#endif

    if (can_enter_dlps)
    {
        return true;
    }
    else
    {
        DlpsErrorCode = (DLPSErrorCode)0xF0;
        return false;
    }
}

void System_Handler(void)
{
    DBG_DIRECT("System_Handler");
    if (System_WakeUpInterruptValue(DATA_UART_RX_PIN) == SET)
    {
        DBG_DIRECT("System_Handler: Rx wakeup");
        can_enter_dlps = false;
    }
    Pad_ClearAllWakeupINT();
}

void data_uart_dlps_init(void)
{
    DLPS_IORegister();
    if (dlps_check_cb_reg(data_uart_dlps_check_cb) == false)
    {
        APP_PRINT_ERROR0("data_uart_dlps_init: dlps_check_cb_reg register failed");
    }
    DLPS_IORegUserDlpsEnterCb(data_uart_dlps_enter_cb);
    DLPS_IORegUserDlpsExitCb(data_uart_dlps_exit_cb);
}

void hrp_dlps_allow_enter(bool enter)
{
    can_enter_dlps = enter;
}
#endif
void hrp_init_dlps(void)
{
#if 0
    data_uart_dlps_init();
    lps_mode_set(LPM_DLPS_MODE);
    lps_mode_pause();
    hrp_dlps_status = HRP_DLPS_STATUS_INITIATED;
    APP_PRINT_INFO0("dlps status initiated");
#endif
}

bool hrp_dlps_active_dlps(bool active)
{
#if 0
    if (active == 0)
    {
        if (hrp_dlps_status == HRP_DLPS_STATUS_ACTIVED)
        {
            lps_mode_pause();
            APP_PRINT_INFO0("dlps status paused");
            hrp_dlps_status = HRP_DLPS_STATUS_PAUSED;
        }
        else
        {
            APP_PRINT_ERROR1("hrp_enable_dlps: disable dlps in non-active status: %d", hrp_dlps_status);
            return false;
        }
    }
    else
    {
        if (hrp_dlps_status == HRP_DLPS_STATUS_IDLE)
        {
            APP_PRINT_INFO0("hrp_enable_dlps: init dlps mode");
            hrp_init_dlps();
        }
        else if (hrp_dlps_status == HRP_DLPS_STATUS_ACTIVED)
        {
            APP_PRINT_ERROR0("hrp_enable_dlps: enable dlps in active status: %d");
            return false;
        }

        lps_mode_resume();
        hrp_dlps_allow_enter(false);  /* not allow enter dlps right now,  hrp_app decide if allow enter*/
        APP_PRINT_INFO0("dlps status resumed");
        hrp_dlps_status = HRP_DLPS_STATUS_ACTIVED;
    }
#endif
    return true;
}

#endif
