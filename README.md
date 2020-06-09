# Software UART

Software UART is a solution that simulate UART signal on 2 GPIO pins.

In Cortex-M develop board sometimes we don't have enough UART pins or UART pins conflicts with other needed peripheral pins. In this situation we can use 2 GPIO pins to simulate UART TX and RX signals.

This project based on the premise that the clock source referenced by GPIO is far slower than UART baudrate or CPU core clock. We can control GPIO signal by using CPU core clock with proper delay.

The reference board is [Realtek RTL8195A](https://www.amebaiot.com/en/ameba1/).