Example Description

This example describes how to use SPI read DMA mode by MBED API.

The SPI Interface provides a "Serial Peripheral Interface" Master.

This interface can be used for communication with SPI Slave devices, such as FLASH memory, LCD screens, and other modules or integrated circuits.

In this example, we use config SPI_IS_AS_Master to decide if the device is Master or Slave.
    If SPI_IS_AS_Master is 1, then the device is Master.
    If SPI_IS_AS_Master is 0, then the device is Slave.

Connections:
    Master board                <---------->       Slave board
    Master's MOSI (PA_19)       <---------->       Slave's MOSI (PA_19)
    Master's MISO (PA_20)       <---------->       Slave's MISO (PA_20)
    Master's SCLK (PA_3)        <---------->       Slave's SCLK (PA_3)
    Master's CS   (PA_2)        <---------->       Slave's CS   (PA_2)
    Master's gpio (PA_17)       <---------->       Slave's gpio (PA_17)

This example shows Master sends data to Slave in by using the function "spi_Slave_read_stream_unfix_size()". "read_stream_unfix_size" means the Slave board is able to read data with unfixed size and then call back.
We bootup Slave first, and then bootup Master.
Then log will present that Master sending data to Slave.

Note
    1, Supported SPI_CS, SPI_SCL, SPI_MOSI and SPI_MISO
        SPI_CS,     PA_2, PA_7, PA_15
        SPI_SCL,    PA_3, PA_8, PA_16
        SPI_MOSI,   PA_4, PA_9, PA_19
        SPI_MISO,   PA_10, PA_20
    2, Please off JTAG/SWD, when using PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16. Please off log UART and configure PA_13 and PA_14 to be the new log UART. Please use FTDI USB cable to connect the new log UART to the terminal, in order to see the log.
    4, PA_7, PA_8, PA_9, and PA_10 only available on RTL8720CF.
