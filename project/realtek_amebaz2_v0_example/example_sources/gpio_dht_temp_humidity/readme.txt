Example Description

This example describes how to use DHT11/DHT22/DHT21 temperature and humidity sensor.
Since DHT require microseconds level GPIO operation, we use GPIO register to perform GPIO read.

Requirement Components:
    DHT11/DHT22/DHT21

DHT series may have 3 pin or 4 pin product.

3 pin DHT has pin layout:
    ┌─┬┬┬┬┬┬─ GND
    │ ├┼┼┼┼┤
    ├─┼┼┼┼┼┼─ 3.3V
    │ ├┼┼┼┼┤
    └─┴┴┴┴┴┴─ DATA

4 pin DHT has pin layout:
    ┌─┬┬┬┬┬┐ 
    │ ├┼┼┼┼┼─ GND
    ├┬┼┼┼┼┼┼─ N/A
    ├┴┼┼┼┼┼┼─ DATA
    │ ├┼┼┼┼┼─ 3.3V
    └─┴┴┴┴┴┘

All we need is 3.3V, GND, and DATA (connect to PA_19).
DATA has default level high.

To get data, it has 3 stages:
    1, Turn DHT from power saving to high speed mode:
        Ameba toggle low on DATA pin.
    2, Wait for DHT ready:
        DHT toggle low on DATA pin.
    3, Repeatedly get 40 bits of data:
        If level high has a shorter length than level low, then it's bit 0.
        If level high has a longer length than level high, then it's bit 1.

             _____          _____________
    ________/     \________/             \_________
             bit 0          bit 1

Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.
