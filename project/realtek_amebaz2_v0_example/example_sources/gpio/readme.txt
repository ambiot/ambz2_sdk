Example Description

This example describes how to use GPIO read/write by MBED API.

Requirement Components:
    a LED
    a push button
    a resister

Pin name PA_19 and PA_20 map to GPIOA_19 and GPIOA_20:
    1, PA_19 as input with internal pull-high, connect a resistor to ground in series. PA_19 also connect to a push button and the other side of the push button connected to 3.3V.
    2, PA_20 as output, connect a LED to GND in series.

In this example, the LED is on/off when the push button is pressed.

Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.
