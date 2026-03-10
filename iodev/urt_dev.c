/* 
 *  Target: weact_studio_rp2350b_core
 *  Written by Akihito Honda (Aki.h @akih_san)
 *  https://x.com/akih_san
 *  https://github.com/akih-san
 *
 * Date: 2026.02.07
*/

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "urt_dev.h"

uart_inst_t *UART_ID = uart0;

void init_uart(void) {
    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}
