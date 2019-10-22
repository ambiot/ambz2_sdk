Example Description

This example describes how to use HC-SR04 ultrasonic.

Requirement Components:
    a HC-SR04 ultrasonic

HC-SR04 4 pin connections:
    VCC        <---------->        5V
    TRIG       <---------->        PA_19
    ECHO       <---------->        PA_20 (with level converter from 5V to 3.3V)
    GND        <---------->        GND

HC-SR04 use ultrasonic to raging distance.
We send a pulse HIGH on TRIG pin for more than 10us, then HC-SR04 return a pulse HIGH on ECHO pin which corresponds distance.
The speed of the sound wave is 340 m/s, which means it takes 29us for 1cm.
Thus the distance of result is:
    distance (in cm) = time (in us) / (29 * 2)

Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.
