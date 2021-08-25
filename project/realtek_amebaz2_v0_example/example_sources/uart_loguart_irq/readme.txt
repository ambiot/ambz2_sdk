Example Description

This example describes how to use LOG_UART as common UART to communicate with PC.

Required Components:
    USBtoTTL adapter

Connect to PC
    1, Connect Ground: connect to GND pin via USBtoTTL adapter
    2, Use UART2
        1), PA_15 as UART2_RX connect to TX of USBtoTTL adapter
        2), PA_16 as UART0_TX connect to RX of USBtoTTL adapter

Open Super terminal or Teraterm and set baud rate to 115200, 1 stopbit, no parity, no flow control.

This example shows:
    1, The RX data ready interrupt service routine is used to receive characters from the PC, and then loopback them to the PC.
    2, The TX done interrupt service routine will send a prompt string "8710c$" to the PC.

Note
    Difference with example "uart_irq":
	1, 	#define UART_TX    PA_16	//PINMUX of LOG_UART
		#define UART_RX    PA_15
	2,	extern hal_uart_adapter_t log_uart;	//extern LOG_UART adapter.
	3,	hal_uart_deinit(&log_uart);		//deinit LOG_UART(which was initialized in the boot phase).
										//For avoid some ERROE_logs cause Initializing an initialized LOG_UART.