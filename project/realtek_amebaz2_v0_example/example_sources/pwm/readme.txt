Example Description

This example describes how to use PWM.

Requirement Components:
    1~4 LED

Connect LED to below PWM pins and GND, then the LED will gradually become brighter and then darker with different speed.
    1, connect PA_17 to a LED and GND in series
    2, connect PA_18 to a LED and GND in series
    3, connect PA_13 to a LED and GND in series
    4, connect PA_14 to a LED and GND in series

Note
    1, Supported PWM, PWM_0 to PWM_7.
        PWM_0, PA_0, PA_11, PA_20
        PWM_1, PA_1, PA_12
        PWM_2, PA_2, PA_14
        PWM_3, PA_3, PA_15
        PWM_4, PA_4, PA_16
        PWM_5, PA_17
        PWM_6, PA_18
        PWM_7, PA_13, PA_19, PA_23
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_11 and PA_12 only available on RTL8720CF.
