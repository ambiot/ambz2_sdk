Example Description

This example describes how to use Power Mode API for Standby.

Requirement Components:
    1, wake up by Stimer
        NONE
    2, wake up by GPIO
        a push button
        a resister
    3, wake up by UART
        a USB FTDI cable
    4, wake up by Gtimer
        NONE
    5, wake up by PWM
        NONE

In this example,
    When wake up by Sitmer,
        1, The system will enter Standby mode by 5s and then reboot the system.
    When wake up by GPIO,
        a), if there is configured GPIO interrupt.
            1, The system will enter Standby mode by 5s.
            2, if the GPIO interrupt has been activated the system will be rebooted.
        b), if there is no configured GPIO interrupt.
            1, PA_17 connect to a resistor series to 3.3V. PA_17 also connect to a push button then series to GND.
            2, The system will enter Standby mode by 5s.
            3, Please set PA_17 as GPIO interrupt for waking up the system. When the push button is pressed, the system will be rebooted.
    When wake up by UART,
        1, Connecting PA_14, PA_13 and GND(UART_0) to USB FTDI cable. Then connect to a terminal.
        2, The system will enter Standby mode by 5s.
        3, Please enter inputs at the UART_0 terminal for waking up the system.
    When wake up by Gtimer,
        1, The system will enter Standby mode by 5s and then resume the system.
    When wake up by PWM,
        1, The system will enter Standby mode by 5s and then resume the system.

Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, When using PA_23 as the GPIO wake up source, please set the interrupt as "IRQ_RISE". PA_23 connect to GND. PA_17 also connect to a push button then series to a resister and 3.3V.
    3, Supported Gtimer, TIMER0 to TIMER6.
    4, Supported PWM, PWM_0 to PWM_7.
        PWM_0, PA_0, PA_11, PA_20
        PWM_1, PA_1, PA_12
        PWM_2, PA_2, PA_14
        PWM_3, PA_3, PA_15
        PWM_4, PA_4, PA_16
        PWM_5, PA_17
        PWM_6, PA_18
        PWM_7, PA_13, PA_19, PA_23
    5, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    6, Please off log UART, when using PA_15, and PA_16.
    7, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.
