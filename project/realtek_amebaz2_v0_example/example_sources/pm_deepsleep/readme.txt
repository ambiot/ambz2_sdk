Example Description

This example describes how to use Power Mode API for DeepSleep.

Requirement Components:
    1, wake up by Stimer
        NONE
    2, wake up by GPIO
        a push button
        a resister

In this example,
    When wake up by Sitmer,
        1, The system will enter DeepSleep mode by 5s and then reboot the system.
    When wake up by GPIO,
        a), if there is configured GPIO interrupt.
            1, The system will enter DeepSleep mode by 5s.
            2, if the GPIO interrupt has been activated the system will be rebooted.
        b), if there is no configured GPIO interrupt.
            1, PA_17 connect to a resistor series to 3.3V. PA_17 also connect to a push button then series to GND.
            2, The system will enter DeepSleep mode by 5s.
            3, Please set PA_17 as GPIO interrupt for waking up the system. When the push button is pressed, the system will be rebooted.

Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, Please off JTAG/SWD when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.
    5, When using PA_23 as the wake up source, please set the interrupt as "IRQ_RISE". PA_23 connect to GND. PA_17 also connect to a push button then series to a resister and 3.3V.
