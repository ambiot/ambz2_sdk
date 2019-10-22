Example Description

This example demos the function of Auto Flow control.
Please connect 2 boards to run this example.

Required Components:
    2 EV boards

The UART2 is default Log UART.
In this example, the log UART is shifted to UART1.
UART2 is tested for the example demo (auto Flow control).

Disconnection for both boards
    PA15          <---------->        log_uart_rx
    PA16          <---------->        log_uart_tx

Connection for both boards
    PA13          <---------->        log_uart_rx
    PA14          <---------->        log_uart_tx

Connect to 2 boards
    Board1        <---------->        Board2
    PA19          <---------->        PA20
    PA20          <---------->        PA19
    PA15          <---------->        PA16
    PA16          <---------->        PA15
    GND           <---------->        GND

This example shows:
The first powered board will be the TX side, the other one will be the RX side.
The RX side will make some delay every 16-bytes received, by this way we can trigger the flow control mechanism.

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
