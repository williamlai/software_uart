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

#ifndef _SW_UART_H_
#define _SW_UART_H_

#include <stdint.h>
#include "PinNames.h"

#define E_SW_UART_SUCCESS               (0)
#define E_SW_UART_INVALID_INPUT         (-1)
#define E_SW_UART_RESOURCE_UNAVAILABLE  (-2)
#define E_SW_UART_OS_ERR                (-3)
#define E_SW_UART_PIN_WAS_NOT_DECLARED  (-4)

/**
 * @brief Initialize software uart.
 * 
 * @param[in] tx UART TX pin
 * @param[in] rx UART RX pin
 * @param[in] baudrate These baudrate are verified: 9600, 19200, 38400, 57600, 115200
 *
 * @returns error number
 */
int sw_uart_init(PinName tx, PinName rx, uint32_t baudrate);

/**
 * @brief Put one character to TX.
 * 
 * @param[in] c the character that needs sent to TX
 *
 * @returns error number
 */
int sw_uart_putc(uint8_t c);

/**
 * @brief Send a buffer to TX.
 * 
 * @param[in] data the data buffer that needs sent to TX
 * @param[in] len the length of data buffer
 *
 * @returns error number
 */
int sw_uart_send(uint8_t *data, uint32_t len);

/**
 * @brief Get one character from RX.
 * 
 * @note It's a blocking call inside a critical section. Don't use it unless
 *       you pretty sure there will be data comes from RX.
 * 
 * @param[in] c the character that comes from RX
 *
 * @returns error number
 */
int sw_uart_getc(uint8_t *c);

/**
 * @brief Get characters from RX.
 * 
 * @note It's a blocking call inside a critical section. Don't use it unless
 *       you pretty sure there will be data comes from RX.
 * 
 * @param[in] data the characters buffer that stores data comes from RX
 * @param[in] len the buffer length
 *
 * @returns error number
 */
int sw_uart_recv(uint8_t *data, uint32_t len);

#endif