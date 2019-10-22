 /**************************************************************************//**
  * @file     pwmout_api.c
  * @brief    This file implements the PWM API functions.
  *
  * @version  V1.00
  * @date     2017-05-25
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

#include "device.h"
#include "objects.h"
#include "pinmap.h"
#include "string.h"

#if DEVICE_PWMOUT

#if CONFIG_PWM_EN
#include "pwmout_api.h"
#include "pwmout_ex_api.h"

static const PinMap PinMap_PWM[] = {
    {PA_0,  RTL_PIN_PERI(PID_PWM0, 0, PinSel0), RTL_PIN_FUNC(PID_PWM0, PinSel0)},
    {PA_1,  RTL_PIN_PERI(PID_PWM1, 1, PinSel0), RTL_PIN_FUNC(PID_PWM1, PinSel0)},
    {PA_2,  RTL_PIN_PERI(PID_PWM2, 2, PinSel0), RTL_PIN_FUNC(PID_PWM2, PinSel0)},
    {PA_3,  RTL_PIN_PERI(PID_PWM3, 3, PinSel0), RTL_PIN_FUNC(PID_PWM3, PinSel0)},
    {PA_4,  RTL_PIN_PERI(PID_PWM4, 4, PinSel0), RTL_PIN_FUNC(PID_PWM4, PinSel0)},
    {PA_5,  RTL_PIN_PERI(PID_PWM5, 5, PinSel0), RTL_PIN_FUNC(PID_PWM5, PinSel0)},
    {PA_6,  RTL_PIN_PERI(PID_PWM6, 6, PinSel0), RTL_PIN_FUNC(PID_PWM6, PinSel0)},

    {PA_11,  RTL_PIN_PERI(PID_PWM0, 0, PinSel1), RTL_PIN_FUNC(PID_PWM0, PinSel1)},
    {PA_12,  RTL_PIN_PERI(PID_PWM1, 1, PinSel1), RTL_PIN_FUNC(PID_PWM1, PinSel1)},
    {PA_13,  RTL_PIN_PERI(PID_PWM7, 7, PinSel0), RTL_PIN_FUNC(PID_PWM7, PinSel0)},
    {PA_14,  RTL_PIN_PERI(PID_PWM2, 2, PinSel1), RTL_PIN_FUNC(PID_PWM2, PinSel1)},
    {PA_15,  RTL_PIN_PERI(PID_PWM3, 3, PinSel1), RTL_PIN_FUNC(PID_PWM3, PinSel1)},
    {PA_16,  RTL_PIN_PERI(PID_PWM4, 4, PinSel1), RTL_PIN_FUNC(PID_PWM4, PinSel1)},
    {PA_17,  RTL_PIN_PERI(PID_PWM5, 5, PinSel1), RTL_PIN_FUNC(PID_PWM5, PinSel1)},
    {PA_18,  RTL_PIN_PERI(PID_PWM6, 6, PinSel1), RTL_PIN_FUNC(PID_PWM6, PinSel1)},
    {PA_19,  RTL_PIN_PERI(PID_PWM7, 7, PinSel1), RTL_PIN_FUNC(PID_PWM7, PinSel1)},
    {PA_20,  RTL_PIN_PERI(PID_PWM0, 0, PinSel2), RTL_PIN_FUNC(PID_PWM0, PinSel2)},
    {PA_21,  RTL_PIN_PERI(PID_PWM1, 1, PinSel2), RTL_PIN_FUNC(PID_PWM1, PinSel2)},
    {PA_22,  RTL_PIN_PERI(PID_PWM2, 2, PinSel2), RTL_PIN_FUNC(PID_PWM2, PinSel2)},
    {PA_23,  RTL_PIN_PERI(PID_PWM7, 7, PinSel2), RTL_PIN_FUNC(PID_PWM7, PinSel2)},

    {NC,    NC,     0}
};

static u8 timer_for_pwm[] = {GTimer1, GTimer2, GTimer3, GTimer4, GTimer5, GTimer6, GTimer7, 0xff};  // the timer ID list those can be used as PWM tick source
static hal_pwm_comm_adapter_t pwm_com_handler;
void pwmout_init(pwmout_t *obj, PinName pin)
{
    uint32_t peripheral;
    u32 pwm_idx;
    u32 pin_sel;
    u16 duty_res = 0;
    static u8 pwm_com_initialed;

    if (!pwm_com_initialed) {
        hal_pwm_comm_init (&pwm_com_handler);
        hal_pwm_comm_tick_source_list (timer_for_pwm);
        pwm_com_initialed = 1;
    }


    DBG_PWM_INFO("%s: Init PWM for pin(0x%x)\n", __func__, pin);
    // Get the peripheral name from the pin and assign it to the object
    peripheral = pinmap_peripheral(pin, PinMap_PWM);

    if (unlikely(peripheral == NC)) {
        DBG_PWM_ERR("%s: Cannot find matched pwm for this pin(0x%x)\n", __func__, pin);
        return;
    }

    pwm_idx = RTL_GET_PERI_IDX(peripheral);
    pin_sel = RTL_GET_PERI_SEL(peripheral);

    obj->pwm_idx = pwm_idx;
    obj->pin_sel = pin_sel;
    obj->period = 0;
    obj->is_init = 0;
    obj->pulse = 0;
    obj->offset_us = 0;
    memset((void *)&obj->pwm_hal_adp, 0, sizeof(hal_pwm_adapter_t));

    if (hal_pwm_init (&obj->pwm_hal_adp, pin, duty_res) != HAL_OK) {
        DBG_PWM_ERR("pwmout_init Err!\n");
        return;
    }
    obj->is_init = 1; // Initialize Success
    pwmout_period_us(obj, 20000); // 20 ms per default
    hal_pwm_enable ((&obj->pwm_hal_adp));
}

void pwmout_free(pwmout_t *obj)
{
    hal_pwm_deinit (&obj->pwm_hal_adp);
}

void pwmout_write(pwmout_t *obj, float percent)
{
    if (percent < (float)0.0) {
        percent = 0.0;
    } else if (percent > (float)1.0) {
        percent = 1.0;
    }

    obj->pulse = (uint32_t)((float)obj->period * percent);
    //DBG_PWM_ERR("obj->period! %d\n", obj->period);
    hal_pwm_set_duty (&obj->pwm_hal_adp, obj->period, obj->pulse, obj->offset_us);
}

float pwmout_read(pwmout_t *obj)
{
    float value = 0;

    if (obj->period > 0) {
        value = (float)(obj->pulse) / (float)(obj->period);
    }
    return ((value > (float)1.0) ? (float)(1.0) : (value));
}

void pwmout_period(pwmout_t *obj, float seconds)
{
    pwmout_period_us(obj, (int)(seconds * 1000000.0f));
}

void pwmout_period_ms(pwmout_t *obj, int ms)
{
    pwmout_period_us(obj, (int)(ms * 1000));
}

void pwmout_period_us(pwmout_t *obj, int us)
{
    float dc = pwmout_read(obj);

    if (us == 0) {
        DBG_PWM_ERR("The period register Cannot be set zero value\n");
    } else {
        obj->period = us;
        // Set duty cycle again
        pwmout_write(obj, dc);
    }
}

void pwmout_pulsewidth(pwmout_t *obj, float seconds)
{
    pwmout_pulsewidth_us(obj, (int)(seconds * 1000000.0f));
}

void pwmout_pulsewidth_ms(pwmout_t *obj, int ms)
{
    pwmout_pulsewidth_us(obj, ms * 1000);
}

void pwmout_pulsewidth_us(pwmout_t *obj, int us)
{
    float value = (float)us / (float)obj->period;

    pwmout_write(obj, value);
}

void pwmout_startoffset(pwmout_t *obj, float seconds)
{
    pwmout_startoffset_us(obj, (int)(seconds * 1000000.0f));
}

void pwmout_startoffset_ms(pwmout_t *obj, int ms)
{
    pwmout_startoffset_us(obj, ms * 1000);
}

void pwmout_startoffset_us(pwmout_t *obj, int us)
{
    float dc = pwmout_read(obj);

    obj->offset_us = us;
    pwmout_write(obj, dc);
}

void pwmout_period_int(pwmout_t *obj, pwm_period_callback_t callback, u8 enable)
{
    DBG_PWM_INFO("%s: Init PWM Interrupt\n", __func__);
    if (enable) {
        hal_pwm_set_period_int (&obj->pwm_hal_adp, (pwm_period_callback_t)callback, (void *)&obj->pwm_hal_adp, 1);
    } else {
        hal_pwm_set_period_int (&obj->pwm_hal_adp, 0, 0, 0);
    }
}

void pwmout_autoadj_int (pwmout_t *obj, pwm_lim_callback_t callback, u8 direction, u8 enable)
{
    DBG_PWM_INFO("%s: Init PWM duty adjustment Interrupt\n", __func__);
    if (enable) {
        if (direction == PwmAdjIntDnLim) {
            hal_pwm_set_autoadj_int (&obj->pwm_hal_adp, (pwm_lim_callback_t)callback, (void *)&obj->pwm_hal_adp, PwmAdjIntDnLim);
        } else if (direction == PwmAdjIntUpLim) {
            hal_pwm_set_autoadj_int (&obj->pwm_hal_adp, (pwm_lim_callback_t)callback, (void *)&obj->pwm_hal_adp, PwmAdjIntUpLim);
        } else {
            DBG_PWM_ERR("%s: The int_en parameter out of range\n", __func__);
        }
    } else {
        hal_pwm_set_autoadj_int (&obj->pwm_hal_adp, 0, 0, 0);
    }
}

void pwmout_autoadj_inc(pwmout_t *obj, u32 max_duty_us, u32 step_sz_us, u32 step_period_cnt)
{
    hal_pwm_auto_duty_inc (&obj->pwm_hal_adp, max_duty_us, step_sz_us, step_period_cnt);
}

void pwmout_autoadj_dec(pwmout_t *obj, u32 min_duty_us, u32 step_sz_us, u32 step_period_cnt)
{
    hal_pwm_auto_duty_dec (&obj->pwm_hal_adp, min_duty_us, step_sz_us, step_period_cnt);
}

void pwmout_start(pwmout_t *obj)
{
    hal_pwm_enable ((&obj->pwm_hal_adp));
}

void pwmout_stop(pwmout_t *obj)
{
    hal_pwm_disable ((&obj->pwm_hal_adp));
}

void pwmout_multi_start(u8 pin_ctrl)
{
    hal_pwm_comm_disable(pin_ctrl);
    hal_pwm_comm_enable(pin_ctrl);
}

#endif // #ifdef CONFIG_PWM_EN
#endif

