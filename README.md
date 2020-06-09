# Software UART

Software UART is a solution that simulate UART signal on 2 GPIO pins.

In Cortex-M develop board sometimes we don't have enough UART pins or UART pins conflicts with other needed peripheral pins. In this situation we can use 2 GPIO pins to simulate UART TX and RX signals.

This project based on the premise that the clock source referenced by GPIO is far slower than UART baudrate or CPU core clock. We can control GPIO signal by using CPU core clock with proper delay.

The reference board is [Realtek RTL8195A](https://www.amebaiot.com/en/ameba1/).

The design document is here:

Chinese edition  
https://medium.com/@redmilk/%E4%BD%BF%E7%94%A8-gpio-%E5%AF%A6%E4%BD%9C-software-uart-737e217a1d99

English edition  
https://medium.com/@redmilk/implement-software-uart-from-2-gpio-1150e96c3d18
