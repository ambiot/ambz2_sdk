Example Description:
	This example demonstrates the function of UART Auto Flow control.
	To run this example 2 boards have to be connected via UART.

Details:
	By default UART2 is the Log UART.
	In this example, UART 2 will be disabled as the Log UART and instead UART 0 will be enabled as the Log UART. 
	UART 2 will be enabled as the UART channel between the both boards.

	The first board that is powered on will be the TX side, the second board that is powered on will be the RX side. 
	The difference between the power on times must not be longer than 5 seconds.
	The RX side will make some delay every 16-byte received, by this way we can trigger the flow control mechanism.

Required Components:
	1. 2 EV boards
	2. jumper cables

Setup:
	UART and pin mapping:
		UART 0:
			PA13: UART 0  RX
			PA14: UART 0  TX
		UART 2:
			PA15: UART 2  RX
			PA16: UART 2  TX
			PA19: UART 2  CTS
			PA20: UART 2  RTS

	Connection between the 2 boards:
	Board1                      <---------->        Board2
	UART2  CTS    PA19          <---------->        PA20  UART2  RTS
	UART2  RTS    PA20          <---------->        PA19  UART2  CTS
	UART2  RX     PA15          <---------->        PA16  UART2  TX
	UART2  TX     PA16          <---------->        PA15  UART2  RX
	GND                         <---------->        GND

	Log UART connection:
		UART 0:
			PA13: UART0  RX
			PA14: UART0  TX

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
