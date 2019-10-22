Example Description

This example describes how to use another GPIO to replace CS line of SPI Master.

The SPI Interface provides a "Serial Peripheral Interface" Master.

This interface can be used for communication with SPI slave devices, such as FLASH memory, LCD screens, and other modules or integrated circuits.

This example shows how users can use GPIO to replace SPI's default slave select pin for SPI Master.
Users are allowed to select another available GPIO pin as a slave select line by defining the value of SPI_GPIO_CS.
Before any Master operation that requires the slave select to pull up, users should call the function gpio_write(&spi_cs, 1); as demonstrated in the example code. Slave select line then pulls low by calling the function gpio_write(&spi_cs, 0); in the interrupt, function Master_cs_tr_done_callback to indicate the operation is done.

Note
    1, Supported SPI_CS, SPI_SCL, SPI_MOSI and SPI_MISO
        SPI_CS,     PA_2, PA_7, PA_15
        SPI_SCL,    PA_3, PA_8, PA_16
        SPI_MOSI,   PA_4, PA_9, PA_19
        SPI_MISO,   PA_10, PA_20
    2, Please off JTAG/SWD, when using PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16. Please off log UART and configure PA_13 and PA_14 to be the new log UART. Please use FTDI USB cable to connect the new log UART to the terminal, in order to see the log.
    4, PA_7, PA_8, PA_9, and PA_10 only available on RTL8720CF.
