#include "FreeRTOS.h"
#include "task.h"

#include "PinNames.h"

static void sw_uart_test_thread(void *arg)
{
    uint8_t c;

    sw_uart_init(PB_4, PB_5, 9600);

    while(1) {
        sw_uart_getc(&c);
        sw_uart_putc(c);
    }
}

void main(void)
{
    xTaskCreate( sw_uart_test_thread, "sw uart", 512, NULL, tskIDLE_PRIORITY + 1, NULL );
    vTaskStartScheduler();
}
