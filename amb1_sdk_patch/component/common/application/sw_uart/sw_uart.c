/*
MIT License

Copyright(c) 2020 William Lai

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "sw_uart.h"
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "device.h"
#include "gpio_api.h"

#define SW_UART_TASK_EVENT_TX 0x01
#define SW_UART_TASK_EVENT_RX 0x02

static uint8_t sw_uart_event = 0;
static uint32_t bit_length = 0;

#define GPIO_PORT_OUTPUT_REG(P) ( (volatile uint32_t *)( 0x40001000 + (P) * 0x0C ) )
#define GPIO_PORT_INPUT_REG(P)  ( (volatile uint32_t *)( 0x40001050 + (P) * 4 ) )
#define GPIO_PORT_MODE_REG(P)   ( (volatile uint32_t *)( 0x40001004 + (P) * 0x0C ) )

#define GPIO_WRITE_HIGH(port,mask) ( *GPIO_PORT_OUTPUT_REG(port) |= (mask) )
#define GPIO_WRITE_LOW(port,mask) ( *GPIO_PORT_OUTPUT_REG(port) &= ~(mask) )

#define GPIO_READ(port,mask) ( *GPIO_PORT_INPUT_REG(port) & (mask) )

static PinName sw_uart_tx_pinname = NC;
static PinName sw_uart_rx_pinname = NC;
static gpio_t gpio_sw_uart_tx;
static gpio_t gpio_sw_uart_rx;

static uint32_t gpio_port_tx = 0;
static uint32_t gpio_mask_tx = 0;
static uint32_t gpio_port_rx = 0;
static uint32_t gpio_mask_rx = 0;

static void sw_uart_thread( void *pArgument );

static SemaphoreHandle_t xSemaSwUartBegin = NULL;
static SemaphoreHandle_t xSemaSwUartEnd = NULL;
static SemaphoreHandle_t xSemaSwUartMutex = NULL;

static uint8_t txc = 0;
static uint8_t *tx_data = NULL;
static uint32_t tx_data_len = 0;
static uint8_t *rx_data = NULL;
static uint32_t rx_data_len = 0;

static void delay_us( uint32_t us )
{
    int i;
    int dfactor = 0;

#if defined (CONFIG_PLATFORM_8195A)
#if defined (__ICCARM__)
    dfactor = 27 * us - 10 + ( 81 * us / 100);
#endif
#endif

    for ( i = 0; i < dfactor; i++ ) {
        asm("nop");
    }
}

int sw_uart_init( PinName tx, PinName rx, uint32_t baudrate )
{
    int res = E_SW_UART_SUCCESS;
    uint32_t pin_name;

    if ( baudrate == 9600 )
    {
        bit_length = 104;
    }
    else if ( baudrate == 19200 )
    {
        bit_length = 52;
    }
    else if ( baudrate == 38400 )
    {
        bit_length = 26;
    }
    else if ( baudrate == 57600 )
    {
        bit_length = 17;
    }
    else if ( baudrate == 115200 )
    {
        bit_length = 9;
    }
    else
    {
        bit_length = ( 1000000 / baudrate ) + 1;
    }

    xSemaSwUartBegin = xSemaphoreCreateBinary();
    if ( xSemaSwUartBegin == NULL )
    {
        res = E_SW_UART_RESOURCE_UNAVAILABLE;
    }

    xSemaSwUartEnd = xSemaphoreCreateBinary();
    if ( xSemaSwUartEnd == NULL )
    {
        res = E_SW_UART_RESOURCE_UNAVAILABLE;
    }

    xSemaSwUartMutex = xSemaphoreCreateMutex();
    if ( xSemaSwUartMutex == NULL )
    {
        res = E_SW_UART_RESOURCE_UNAVAILABLE;
    }

    if ( xTaskCreate( sw_uart_thread, "sw_uart_thread", 512, NULL, tskIDLE_PRIORITY + 1, NULL ) != pdPASS )
    {
        res = E_SW_UART_RESOURCE_UNAVAILABLE;
    }

    if ( tx != NC )
    {
        sw_uart_tx_pinname = tx;

        gpio_init( &gpio_sw_uart_tx, tx );
        gpio_dir( &gpio_sw_uart_tx, PIN_OUTPUT );
        gpio_mode( &gpio_sw_uart_tx, PullNone );
        gpio_write( &gpio_sw_uart_tx, 1 );

        pin_name = HAL_GPIO_GetPinName( tx );
        gpio_port_tx = HAL_GPIO_GET_PORT_BY_NAME( pin_name );
        gpio_mask_tx = 1 << ( HAL_GPIO_GET_PIN_BY_NAME( pin_name ) );
    }

    if ( rx != NC )
    {
        sw_uart_rx_pinname = rx;

        gpio_init( &gpio_sw_uart_rx, rx );
        gpio_dir( &gpio_sw_uart_rx, PIN_INPUT );
        gpio_mode( &gpio_sw_uart_rx, PullUp );

        pin_name = HAL_GPIO_GetPinName( rx );
        gpio_port_rx = HAL_GPIO_GET_PORT_BY_NAME( pin_name );
        gpio_mask_rx = 1 << ( HAL_GPIO_GET_PIN_BY_NAME( pin_name ) );
    }

    return res;
}

int sw_uart_putc( uint8_t c )
{
    txc = c;
    return sw_uart_send( &txc, 1 );
}

int sw_uart_send( uint8_t *data, uint32_t len )
{
    int res = E_SW_UART_SUCCESS;

    do
    {
        if ( sw_uart_tx_pinname == NC )
        {
            res = E_SW_UART_PIN_WAS_NOT_DECLARED;
            break;
        }

        if ( xSemaphoreTake( xSemaSwUartMutex, portMAX_DELAY ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }

        tx_data = data;
        tx_data_len = len;
        sw_uart_event |= SW_UART_TASK_EVENT_TX;

        if ( xSemaphoreGive( xSemaSwUartBegin ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }

        if ( xSemaphoreTake( xSemaSwUartEnd, portMAX_DELAY ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }

        if ( xSemaphoreGive( xSemaSwUartMutex ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }
    } while (0);

    return res;
}

int sw_uart_getc( uint8_t *c )
{
    return sw_uart_recv( c, 1 );
}

int sw_uart_recv( uint8_t *data, uint32_t len )
{
    int res = E_SW_UART_SUCCESS;

    do
    {
        if ( sw_uart_rx_pinname == NC )
        {
            res = E_SW_UART_PIN_WAS_NOT_DECLARED;
            break;
        }

        if ( xSemaphoreTake( xSemaSwUartMutex, portMAX_DELAY ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }

        rx_data = data;
        rx_data_len = len;
        sw_uart_event |= SW_UART_TASK_EVENT_RX;

        if ( xSemaphoreGive( xSemaSwUartBegin ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }

        if ( xSemaphoreTake( xSemaSwUartEnd, portMAX_DELAY ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }

        if ( xSemaphoreGive( xSemaSwUartMutex ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            break;
        }
    } while (0);

    return res;
}

static void do_sw_uart_send( uint8_t *data, uint32_t len )
{
    taskENTER_CRITICAL();

    for ( int i = 0; i < len; i++ )
    {
        uint8_t c = data[i];
        
        // pull down as start bit
        GPIO_WRITE_LOW( gpio_port_tx, gpio_mask_tx );
        delay_us(bit_length);

        for ( int j = 0; j < 8; j++ )
        {
            if ( c & 0x01 )
            {
                GPIO_WRITE_HIGH( gpio_port_tx, gpio_mask_tx );
            }
            else
            {
                GPIO_WRITE_LOW( gpio_port_tx, gpio_mask_tx );
            }
            delay_us(bit_length);
            c >>= 1;
        }

        // pull high as stop bit
        GPIO_WRITE_HIGH( gpio_port_tx, gpio_mask_tx );
        delay_us(bit_length);
    }

    taskEXIT_CRITICAL();
}

static void do_sw_uart_recv( uint8_t *data, uint32_t len )
{
    taskENTER_CRITICAL();

    for ( int i = 0; i < len; i++ )
    {
        uint8_t b[8] = { 0 };
        uint32_t c = 0;

        // wait for start bit
        while ( GPIO_READ( gpio_port_rx, gpio_mask_rx ) );
        delay_us(bit_length/2);

        for ( int j = 0; j < 8; j++ )
        {
            delay_us(bit_length);
            b[j] = ( GPIO_READ( gpio_port_rx, gpio_mask_rx ) ) ? 1 : 0;
        }
        
        for ( int j = 0; j < 8; j++ )
        {
            c |= (b[j] << j);
        }
        data[i] = c;

        // wait for stop bit
        while ( GPIO_READ( gpio_port_rx, gpio_mask_rx ) == 0 );
    }

    taskEXIT_CRITICAL();
}

static void sw_uart_thread( void *pArgument )
{
    int res;

    while(1) {
        if ( xSemaphoreTake( xSemaSwUartBegin, portMAX_DELAY ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
            continue;
        }

        if (sw_uart_event & SW_UART_TASK_EVENT_TX )
        {
            do_sw_uart_send( tx_data, tx_data_len );
        }

        if (sw_uart_event & SW_UART_TASK_EVENT_RX )
        {
            do_sw_uart_recv( rx_data, rx_data_len );
        }

        sw_uart_event = 0;

        if ( xSemaphoreGive( xSemaSwUartEnd ) != pdTRUE )
        {
            res = E_SW_UART_OS_ERR;
        }
    }
}
