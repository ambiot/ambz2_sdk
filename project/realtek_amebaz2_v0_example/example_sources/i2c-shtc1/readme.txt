Example Description

This example describes how to use I2C by using MBED API.

Work with Arduino extended board, which has SHTC1 temperature and humidity.
sensor

Connections
    1, I2C0 SDA (PA_20) to extended board's SDA
    2, I2C0 SCL (PA_19) to extended board's SCL

Note
    1, Supported I2C_SCL & I2C_SDA pins, PA_2 & PA_3, PA_11 & PA_12, PA_15 & PA_16, and PA_19 & PA_20.
    2, If defined PA_15 & PA_16 as I2C_SCL & I2C_SDA, please off log UART and configure PA_13 and PA_14 to be new log UART. Please use FTDI USB cable to connect the new log UART to the terminal, in order to see the log.
    3, If defined PA_2 & PA_3 as I2C_SCL & I2C_SDA, please off JTAG/SWD.
    4, PA_11 & PA_12 as I2C_SCL & I2C_SDA only available on RTL8720CF.
