Example Description

This example describes how to use UART TX to simulate clock source

Required Components:
    Oscilloscope

Connect to PC
    1, Connect Ground: connect to GND of the oscilloscope
    2, Use UART0
        1), PA_13 as UART0_RX connect NOTHING
        2), PA_14 as UART0_TX connect to probe of the oscilloscope

This example shows:
    Clock signal output from UART1_TX to the oscilloscope

Note
    1, Supported UART, UART0 to UART2
        UART0_RX,   PA_12, PA_13
        UART0_TX,   PA_11, PA_14
        UART0_CTS,  PA_10
        UART0_RTS,  PA_9
        UART1_RX,   PA_0, PA_2
        UART1_TX,   PA_1, PA_3
        UART1_CTS,  PA_4
        UART1_RTS,  NA
        UART2_RX,   PA_15
        UART2_TX,   PA_16
        UART2_CTS,  PA_19
        UART2_RTS,  PA_20
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, PA_9, PA_10, PA_11, and PA_12 only available on RTL8720CF.
