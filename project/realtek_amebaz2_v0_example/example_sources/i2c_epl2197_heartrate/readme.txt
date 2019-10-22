Example Description

This example is used to measure the heart rate of human.

Requirement Components:
    extend board

Work with Arduino extended board, which has a heart rate sensor during the measurement, the user has to lie his pulp on the sensor and do not rock the sensor.

The test code will return back the heart rate.

Connections
    1, I2C0 SDA (PA_20) to extended board's SDA
    2, I2C0 SCL (PA_19) to extended board's SCL

Build code
IAR:
    1, Please be sure to copy inc\heart_interface.h, inc\HRM_2197.h
    2, Include hr_library.a in the project. Add hr_library.a into folder "lib" in the project.
GCC:
    1, Please be sure to copy inc\heart_interface.h, inc\HRM_2197.h
    2, In project\realtek_amebaz2_v0_example\GCC-RELEASE\application.is.mk, modify LIBFLAGS part by adding below line:

	LIBFLAGS += -Wl,--start-group ../../../project/realtek_amebaz2_v0_example/example_sources/i2c_epl2197_heartrate/src/hr_library.a -Wl,--end-group

Note
    1, Supported I2C_SCL & I2C_SDA pins, PA_2 & PA_3, PA_11 & PA_12, PA_15 & PA_16, and PA_19 & PA_20.
    2, If defined PA_15 & PA_16 as I2C_SCL & I2C_SDA, please off log UART and configure PA_13 and PA_14 to be new log UART. Please use FTDI USB cable to connect the new log UART to the terminal, in order to see the log.
    3, If defined PA_2 & PA_3 as I2C_SCL & I2C_SDA, please off JTAG/SWD.
    4, PA_11 & PA_12 as I2C_SCL & I2C_SDA only available on RTL8720CF.
