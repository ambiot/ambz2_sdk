/**************************************************************************//**
 * @file     hal_pwm.c
 * @brief    This PWM HAL API functions.
 *
 * @version  V1.00
 * @date     2016-07-15
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include "hal_pwm.h"
#include "hal_pinmux.h"
#include "hal_timer.h"
// TODO: modification for 8710c: pin mux control/register for init/deinit
#if CONFIG_PWM_EN && CONFIG_GTIMER_EN

/**
 * @addtogroup hs_hal_pwm PWM
 * @{
 */

/**
 *  @brief To register a IRQ handler for the PWM common interrupt.
 *
 *  @param[in] irq_handler The IRQ handler.
 *
 *  @returns void
 */
void hal_pwm_comm_irq_reg (irq_handler_t irq_handler)
{
    hal_pwm_stubs.hal_pwm_comm_irq_reg (irq_handler);
}

/**
 *  @brief To un-register the PWM common IRQ handler.
 *
 *  @param void
 *
 *  @returns void
 */
void hal_pwm_comm_irq_unreg (void)
{
    hal_pwm_stubs.hal_pwm_comm_irq_unreg ();
}

/**
 *  @brief To initial the PWM devices common adapter. This function must be called first, before call
 *         any other PWM functions.
 *
 *  @param[in] ppwm_com_adp The PWM devices common adapter.
 *
 *  @returns void
 */
void hal_pwm_comm_init (hal_pwm_comm_adapter_t *ppwm_com_adp)
{
    hal_pwm_stubs.hal_pwm_comm_init (ppwm_com_adp);
}

/**
 *  @brief To de-initial the PWM devices common adapter.
 *         The whole will be disabled and the clock will be gated.
 *
 *  @returns void
 */
void hal_pwm_comm_deinit (void)
{
    hal_pwm_stubs.hal_pwm_comm_deinit();
}

/**
 *  @brief To give a list of g-timer ID as the tick source of PWM devices.
 *         The list should end with 0xFF
 *
 *  @param[in] timer_list The timer ID list those can be used as the PWM tick source.
 *
 *  @returns void
 */
void hal_pwm_comm_tick_source_list (uint8_t *timer_list)
{
    hal_pwm_stubs.hal_pwm_comm_tick_source_list (timer_list);
}

/**
 *  @brief To initial a PWM devices adapter. This is the first function must be called
 *         before to do any operation to the PWM devices.
 *
 *  @param[in] ppwm_adp The PWM devices adapter.
 *  @param[in] pwm_id The PWM devices index, valid value is 0 ~ (MaxPwmNum - 1).
 *  @param[in] duty_res_us The resolution for the duty duration setting, in us.
 *                     The value 0 means the duty resolution will be 1/4000 of PWM period.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_ERR_PARA:  Input arguments are invalid.
 *  @return     HAL_NOT_READY: Error with data not ready.
 */
hal_status_t hal_pwm_init (hal_pwm_adapter_t *ppwm_adp, pin_name_t pin_name, u16 duty_res_us)
{
    hal_status_t ret;
    uint8_t pwm_id;

    ret = hal_pwm_stubs.hal_pwm_init (ppwm_adp, pin_name, duty_res_us);
    if (ret == HAL_OK) {
        pwm_id = ppwm_adp->pwm_id;
        ret = hal_pinmux_register(pin_name, PID_PWM0+pwm_id);
    }

    return ret;
}

/**
 *  @brief To disable and de-initial a PWM devices adapter.
 *
 *  @param[in] ppwm_adp The PWM devices adapter.
 *
 *  @returns void
 */
void hal_pwm_deinit (hal_pwm_adapter_t *ppwm_adp)
{
    #if 1
    hal_pwm_adapter_t *ptmp_pwm_adp;
    u8 pwm_id;
    hal_pwm_comm_adapter_t *ppwm_comm_adapter = *(hal_pwm_stubs.pppwm_comm_adp);
    if ((ppwm_adp->pwm_clk_sel != PwmClkSrc_None) && (ppwm_adp->pwm_clk_sel != PwmClkSrc_SClk)) {
        // a G-timer has been assign to this PWM already, we should free this timer and reallocate a new timer
        for (pwm_id = 0; pwm_id < MaxPwmNum; pwm_id++) {
            ptmp_pwm_adp = ppwm_comm_adapter->pwm_adapter[pwm_id];
            if ((ptmp_pwm_adp != NULL) && 
                (ptmp_pwm_adp != ppwm_adp)) {
                if (ptmp_pwm_adp->pwm_clk_sel == ppwm_adp->pwm_clk_sel) {
                    // other PWM still using the same G-Timer, cannot disable this timer
                    break;  // break for loop
                }
            }
        }
    
        if (pwm_id == MaxPwmNum) {
            // no other PWM use the same G-timer, so we can disable it
            if ((ppwm_adp->pwm_clk_sel) < PwmClkSrc_SClk) {
                hal_timer_event_deinit ((ppwm_adp->pwm_clk_sel));        
            }
            //DBG_PWM_ERR ("gtimer_to_disable = (%d)\r\n", ppwm_adp->pwm_clk_sel);
            ppwm_adp->pwm_clk_sel = PwmClkSrc_None;            
        }
    }
    #endif
    hal_pinmux_unregister(ppwm_adp->pin_name, PID_PWM0+ppwm_adp->pwm_id);
    hal_pwm_stubs.hal_pwm_deinit (ppwm_adp);
}

/**
 *  @brief To read a PWM devive enable status
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *
 *  @returns 1: Enable.
 *  @returns 0: Disable.
 */
BOOLEAN hal_pwm_enable_sts (hal_pwm_adapter_t *ppwm_adp)
{
    return hal_pwm_stubs.hal_pwm_enable_sts (ppwm_adp);
}

/**
 *  @brief To enable multiple PWM devive simultaneously. If multiple PWM need to
 *         keep the phase offset, then they should be enabled simultaneously.
 *
 *  @param[in] en_ctrl The bit map for the PWM enable control. Bit [0 .. 7] map to PWM 0 .. 7.
 *
 *  @returns void
 */
void hal_pwm_comm_enable (u32 en_ctrl)
{
    hal_pwm_stubs.hal_pwm_comm_enable (en_ctrl);
}

/**
 *  @brief To disable multiple PWM devive simultaneously.
 *
 *  @param[in] dis_ctrl The bit map for the PWM disable control. Bit [0 .. 7] map to PWM 0 .. 7.
 *
 *  @returns void
 */
void hal_pwm_comm_disable (u32 dis_ctrl)
{
    hal_pwm_stubs.hal_pwm_comm_disable (dis_ctrl);
}

/**
 *  @brief To enable a PWM devive.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *
 *  @returns void
 */
void hal_pwm_enable (hal_pwm_adapter_t *ppwm_adp)
{
    hal_pwm_stubs.hal_pwm_enable (ppwm_adp);
}

/**
 *  @brief To disable a PWM devive.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *
 *  @returns void
 */
void hal_pwm_disable (hal_pwm_adapter_t *ppwm_adp)
{
    hal_pwm_stubs.hal_pwm_disable (ppwm_adp);
}

/**
 *  @brief To set the PWM tick source selection.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] clk_sel The PWM tick source selection
 *
 *  @returns void
 */
void hal_pwm_set_clk_sel (hal_pwm_adapter_t *ppwm_adp, pwm_clk_sel_t clk_sel)
{
    hal_pwm_stubs.hal_pwm_set_clk_sel (ppwm_adp, clk_sel);
}

/**
 *  @brief To wait the PWM HW ready to set new PWM period/duty/offset.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *
 *  @returns void
 */
void hal_pwm_wait_ctrl_ready (hal_pwm_adapter_t *ppwm_adp)
{
    hal_pwm_stubs.hal_pwm_wait_ctrl_ready (ppwm_adp);
}

/**
 *  @brief To set the tick time (resolution) of a PWM device.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] tick_p5us The PWM tick time, unit is 500ns. It should be a even number.
 *
 *  @returns HAL_OK: Setting succeed.
 *  @returns HAL_BUSY: Busy.
 *  @returns HAL_NOT_READY: Error with data not ready.
 */
hal_status_t hal_pwm_set_tick_time (hal_pwm_adapter_t *ppwm_adp, u32 tick_p5us)
{
    return hal_pwm_stubs.hal_pwm_set_tick_time (ppwm_adp, tick_p5us);
}

/**
 *  @brief To set the duty ratio of the PWM
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] period_us The PWM cycle period, unit is us
 *  @param[in] duty_us The PWM on duty duration, unit is us
 *  @param[in] start_offset_us The on duty start timing offset from the start of the PWM periodof, unit is us
 *
 *  @returns HAL_OK: Setting succeed.
 *  @returns HAL_NOT_READY: Error with data not ready.
 */
hal_status_t hal_pwm_set_duty (hal_pwm_adapter_t *ppwm_adp, u32 period_us,
                                u32 duty_us, u32 start_offset_us)
{
    return hal_pwm_stubs.hal_pwm_set_duty (ppwm_adp, period_us, duty_us, start_offset_us);
}

/**
 *  @brief To set the duty_ns ratio of the PWM
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] period_ns The PWM cycle period, unit is ns
 *  @param[in] duty_ns The PWM on duty duration, unit is ns
 *  @param[in] start_offset_ns The on duty start timing offset from the start of the PWM periodof, unit is ns
 *
 *  @returns HAL_OK: Setting succeed.
 *  @returns HAL_NOT_READY: Error with data not ready.
 */
hal_status_t hal_pwm_set_duty_ns (hal_pwm_adapter_t *ppwm_adp, u32 period_ns,
                                u32 duty_ns, u32 start_offset_ns)
{
    return hal_pwm_stubs.hal_pwm_set_duty_ns (ppwm_adp, period_ns, duty_ns, start_offset_ns);
}

/**
 *  @brief To read the time period of current duty of the PWM
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *
 *  @returns The PWM on duty duration, unit is us
 */
u32 hal_pwm_read_duty (hal_pwm_adapter_t *ppwm_adp)
{
    return hal_pwm_stubs.hal_pwm_read_duty (ppwm_adp);
}

/**
 *  @brief To change the duty ratio of the PWM only and keep other setting.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] duty_us The PWM on duty duration, unit is us
 *
 *  @returns void
 */
void hal_pwm_change_duty (hal_pwm_adapter_t *ppwm_adp, u32 duty_us)
{
    hal_pwm_stubs.hal_pwm_change_duty (ppwm_adp, duty_us);
}

/**
 *  @brief To set the PWM on duty boundary (up limit / down limit) of the duty
 *         auto-adjustment.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] max_duty_us The up limit of the duty time, in us.
 *  @param[in] min_duty_us The down limit of the duty time, in us.
 *
 *  @returns HAL_OK: Setting succeed.
 *  @returns HAL_ERR_PARA: Error with invaild parameters.
 */
hal_status_t hal_pwm_set_duty_limit (hal_pwm_adapter_t *ppwm_adp, u32 max_duty_us, u32 min_duty_us)
{
    return hal_pwm_stubs.hal_pwm_set_duty_limit (ppwm_adp, max_duty_us, min_duty_us);
}

/**
 *  @brief To setup the PWM duty auto-adjustment registers.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] pauto_duty The duty auto-adjustment configuration. Includes maximum / minum duty size, duty
 *                    increasing / decreasing step size, adjustment period and the callback function for
 *                    the adjustment done indication.
 *  @returns void
 */
void hal_pwm_set_auto_duty_adj (hal_pwm_adapter_t *ppwm_adp, hal_pwm_auto_duty_adj_t *pauto_duty)
{
    hal_pwm_stubs.hal_pwm_set_auto_duty_adj (ppwm_adp, pauto_duty);
}

/**
 *  @brief To enable or disable the PWM duty auto-adjustment HW.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] enable The duty auto-adjustment enable control (0: disable, 1: enable)
 *
 *  @returns void
 */
void hal_pwm_auto_duty_en (hal_pwm_adapter_t *ppwm_adp, BOOLEAN enable)
{
    hal_pwm_stubs.hal_pwm_auto_duty_en (ppwm_adp, enable);
}

/**
 *  @brief To configure the PWM duty auto-adjustment for duty duration increasing.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] max_duty_us The up limit of the duty duration, in us.
 *  @param[in] step_sz_us The step size of each duty duration increasing, in us.
 *  @param[in] step_period_cnt The stay time of each duty duration increasing step, uint is PWM period.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_ERR_PARA:  Error with invaild parameters.
 */
hal_status_t hal_pwm_set_auto_duty_inc (hal_pwm_adapter_t *ppwm_adp, u32 max_duty_us,
                                        u32 step_sz_us, u32 step_period_cnt)
{
    return hal_pwm_stubs.hal_pwm_set_auto_duty_inc (ppwm_adp, max_duty_us, step_sz_us, step_period_cnt);
}

/**
 *  @brief To configure the PWM duty auto-adjustment for duty duration increasing.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] max_duty_us The up limit of the duty duration, in us.
 *  @param[in] step_sz_us The step size of each duty duration increasing, in us.
 *  @param[in] step_period_cnt The stay time of each duty duration increasing step, uint is PWM period.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_ERR_PARA:  Error with invaild parameters.
 */
hal_status_t hal_pwm_set_auto_duty_dec (hal_pwm_adapter_t *ppwm_adp, u32 min_duty_us,
                                        u32 step_sz_us, u32 step_period_cnt)
{
    return hal_pwm_stubs.hal_pwm_set_auto_duty_dec (ppwm_adp, min_duty_us, step_sz_us, step_period_cnt);
}

/**
 *  @brief To configure the PWM duty auto-adjustment loop mode.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] ini_dir The initial direction for the duty-adjustment loop. 1 -> duty increasing, 0 -> duty decreasing.
 *  @param[in] loop_cnt The number of duty-adjustment loop to run. 1 loop means from min duty to max duty
 *                  or from max duty to min duty.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_ERR_PARA:  Error with invaild parameters.
 */
hal_status_t hal_pwm_set_auto_duty_loop (hal_pwm_adapter_t *ppwm_adp, u8 ini_dir, u32 loop_cnt)
{
    return hal_pwm_stubs.hal_pwm_set_auto_duty_loop (ppwm_adp, ini_dir, loop_cnt);
}

/**
 *  @brief To enable the PWM period end interrupt.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] callback The callback function. It will be called when the interrupt is accurred.
 *  @param[in] arg The argument of the callback function.
 *  @param[in] int_en To enable(1) or disable(0) the interrupt. For interrupt disable, the arguments
 *                'callback' & 'arg' are ignored.
 *
 *  @returns void
 */
void hal_pwm_set_period_int (hal_pwm_adapter_t *ppwm_adp, pwm_period_callback_t callback, void *arg, u8 int_en)
{
    hal_pwm_stubs.hal_pwm_set_period_int (ppwm_adp, callback, arg, int_en);
}

/**
 *  @brief To setup the PWM duty auto adjustment interrupt.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] callback The callback function. It will be called when the interrupt is accurred.
 *  @param[in] arg The argument of the callback function.
 *  @param[in] int_en The bit map to enable/disable the interrupt. Bit 0 control the interrupt of duty duration
 *                reachs the up limit. Bit 1 control the interrupt of duty duration reachs the down limit.
 *
 *  @returns void
 */
void hal_pwm_set_autoadj_int (hal_pwm_adapter_t *ppwm_adp, pwm_lim_callback_t callback,
                                void *arg, u8 int_en)
{
    hal_pwm_stubs.hal_pwm_set_autoadj_int (ppwm_adp, callback, arg, int_en);
}


/**
 *  @brief To setup the PWM duty auto adjustment interrupt callback for loop mode.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] callback The callback function. It will be called when the duty adjustment loop count down to 0.
 *  @param[in] arg The argument of the callback function.
 *
 *  @returns void
 */
void hal_pwm_set_autoadj_loop_int (hal_pwm_adapter_t *ppwm_adp, pwm_lo_callback_t callback, void *arg)
{
    hal_pwm_stubs.hal_pwm_set_autoadj_loop_int (ppwm_adp, callback, arg);
}

/**
 *  @brief To start the PWM duty auto-adjustment for duty duration increasing.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] max_duty_us The up limit of the duty duration, in us.
 *  @param[in] step_sz_us The step size of each duty duration increasing, in us.
 *  @param[in] step_period_cnt The stay time of each duty duration increasing step, uint is PWM period.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_BUSY:  BUSY.
 */
hal_status_t hal_pwm_auto_duty_inc (hal_pwm_adapter_t *ppwm_adp, u32 max_duty_us,
                                    u32 step_sz_us, u32 step_period_cnt)
{
    return hal_pwm_stubs.hal_pwm_auto_duty_inc (ppwm_adp, max_duty_us, step_sz_us, step_period_cnt);
}

/**
 *  @brief To start the PWM duty auto-adjustment for duty duration increasing.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] max_duty_us The up limit of the duty duration, in us.
 *  @param[in] step_sz_us The step size of each duty duration increasing, in us.
 *  @param[in] step_period_cnt The stay time of each duty duration increasing step, uint is PWM period.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_BUSY:  BUSY.
 */
hal_status_t hal_pwm_auto_duty_dec (hal_pwm_adapter_t *ppwm_adp, u32 min_duty_us,
                                    u32 step_sz_us, u32 step_period_cnt)
{
    return hal_pwm_stubs.hal_pwm_auto_duty_dec (ppwm_adp, min_duty_us, step_sz_us, step_period_cnt);
}

/**
 *  @brief To start the PWM duty auto-adjustment loop mode.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] ini_duty_us The initial value for the loop-mode auto duty adjustment. If this value is 0xFFFFFFFF
 *                       it means use current duty as the initial duty.
 *  @param[in] ini_dir The initial direction for the duty-adjustment loop. 1 -> duty increasing, 0 -> duty decreasing.
 *  @param[in] loop_cnt The number of duty-adjustment loop to run. 1 loop means from min duty to max duty
 *                  or from max duty to min duty.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_BUSY:  BUSY.
 */
hal_status_t hal_pwm_auto_duty_loop (hal_pwm_adapter_t *ppwm_adp, u32 ini_duty_us, u8 ini_dir, u32 loop_cnt)
{
    return hal_pwm_stubs.hal_pwm_auto_duty_loop (ppwm_adp, ini_duty_us, ini_dir, loop_cnt);
}

/**
 *  @brief To stop the PWM duty auto-adjustment loop mode.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] stop_now Is stop the PWM auto duty loop immediately(1) or stop at the next duty limit(0).
 *
 *  @returns void
 */
void hal_pwm_stop_duty_loop (hal_pwm_adapter_t *ppwm_adp, u8 stop_now)
{
    hal_pwm_stubs.hal_pwm_stop_duty_loop (ppwm_adp, stop_now);
}

/**
 *  @brief To start the PWM duty auto-adjustment for duty duration increasing.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] max_duty_ns The up limit of the duty duration, in ns.
 *  @param[in] step_sz_ns The step size of each duty duration increasing, in ns.
 *  @param[in] step_period_cnt The stay time of each duty duration increasing step, uint is PWM period.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_BUSY:  BUSY.
 */
hal_status_t hal_pwm_auto_duty_ns_inc (hal_pwm_adapter_t *ppwm_adp, u32 max_duty_ns,
                                    u32 step_sz_ns, u32 step_period_cnt)
{
    return  hal_pwm_stubs.hal_pwm_auto_duty_ns_inc (ppwm_adp, max_duty_ns, step_sz_ns, step_period_cnt);
}

/**
 *  @brief To start the PWM duty auto-adjustment for duty duration increasing.
 *
 *  @param[in] ppwm_adp The PWM device adapter.
 *  @param[in] max_duty_ns The up limit of the duty duration, in ns.
 *  @param[in] step_sz_ns The step size of each duty duration increasing, in ns.
 *  @param[in] step_period_cnt The stay time of each duty duration increasing step, uint is PWM period.
 *
 *  @return     HAL_OK:  Setting succeed.
 *  @return     HAL_BUSY:  BUSY.
 */
hal_status_t hal_pwm_auto_duty_ns_dec (hal_pwm_adapter_t *ppwm_adp, u32 min_duty_ns,
                                    u32 step_sz_ns, u32 step_period_cnt)
{
    return  hal_pwm_stubs.hal_pwm_auto_duty_ns_dec (ppwm_adp, min_duty_ns, step_sz_ns, step_period_cnt);
}

/** @} */ /* End of group hs_hal_pwm */

#endif  // end of "#if CONFIG_PWM_EN && CONFIG_GTIMER_EN"

