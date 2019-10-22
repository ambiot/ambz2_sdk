Example Description

This example describes how to implement a high/low level trigger on 1 GPIO pin.

Pin name PA_19 and PA_20 map to GPIOA_19 and GPIOA_20:
Connect PA_19 and PA_20
    1, PA_19 as GPIO input high/low level trigger.
    2, PA_20 as GPIO output

In this example, PA_20 is a signal source that changes the level to high and low periodically.

PA_19 setup to listen to low level events in initial.
When PA_19 catch low level events, it disables the IRQ to avoid receiving duplicate events.
(NOTE: the level events will keep invoked if level keeps in the same level)

Then PA_19 is configured to listen to high level events and enable IRQ.
As PA_19 catches high level events, it changes back to listen to low level events.

Thus PA_19 can handle both high/low level events.

In this example, you will see a log that prints high/low level event periodically.

Note
    1, Supported GPIO pins, PA_0, PA_1, PA_2, PA_3, PA_4, PA_7, PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15, PA_16, PA_17, PA_18, PA_19, PA_20, and PA_23.
    2, Please off JTAG/SWD, when using PA_0, PA_1, PA_2, PA_3, and PA_4.
    3, Please off log UART, when using PA_15, and PA_16.
    4, PA_7, PA_8, PA_9, PA_10, PA_11 and PA_12 only available on RTL8720CF.
