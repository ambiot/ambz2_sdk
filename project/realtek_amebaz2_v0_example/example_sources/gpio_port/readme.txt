Example Description

This example describes how to use GPIO Port read/write by MBED API.

Requirement Components:
    7 LEDs
    2 boards

Connections:
    Output board        <---------->        Input board
    PA_23               <---------->        PA_23
    PA_20               <---------->        PA_20
    PA_19               <---------->        PA_19
    PA_18               <---------->        PA_18
    PA_17               <---------->        PA_17
    PA_14               <---------->        PA_14
    PA_13               <---------->        PA_13

Please connect input and output boards together. In addition each pins of output board connects a LED to ground in series.

Define "PORT_OUTPUT_TEST" value to set the board to be input or output. 1: output test, 0: input test.
The default port is PortA. "pin_mask" value enables the LED pins. Each bit map to 1 pin: 0: pin disable, 1: pin enable. For example, "pin_mask = 0x9E6000;", 0x9E6000 is binary 1001,1110,0110,0000,0000,0000, represents PA_23, PA_20, PA_19, PA_18, PA_17, PA_14, PA_13.

The LEDs will show the "led_pattern" and the log of input board will show every "pin_mask" of each "led_pattern".

Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.
