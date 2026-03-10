/* 
 *  Target: weact_studio_rp2350b_core
 *  Written by Akihito Honda (Aki.h @akih_san)
 *  https://x.com/akih_san
 *  https://github.com/akih-san
 *
 * Date: 2026.02.07
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "w65_irq.pio.h"

#include "w65.h"
#include "iodev.h"

extern float clk_fs;

#define IRQ_WIDTH 6		// set 6 cpu clocks
#define SM 0

// define PIO number
static PIO pio = pio0;
static uint32_t width;


// this is a raw helper function for use by the user which sets up the GPIO output,
// and configures the SM to output on a particular pin
static void make_irq_program_init(PIO pio, uint sm, uint offset, uint pin) {
   pio_gpio_init(pio, pin);
   pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
   pio_sm_config c = make_irq_program_get_default_config(offset);
   sm_config_set_set_pins(&c, pin, 1);
   pio_sm_init(pio, sm, offset, &c);
}

static void start_make_irq_pio(PIO pio, uint sm, uint offset, uint pin) {
    make_irq_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);
	width = (uint32_t)(clk_fs*(WRAP_COUNT+1)*(IRQ_WIDTH) - 8 );		// -8 : 8 machin words without loop
    printf("PIO IRQ width count(%d)\r\n",width);
}

void setup_pio(void)
{
	uint offset = pio_add_program(pio, &make_irq_program);
    printf("PIO Loaded program at %d\n", offset);
	start_make_irq_pio(pio, SM, offset, W65_IRQ);
}

// Make IRQ pulse
void make_irq(void)
{
uint32_t w;
	
	w = pio-> rxf[SM];		// preread rx fifo
	pio->txf[SM] = width;
	w = pio_sm_get_blocking(pio,SM);	// wait data in rx fifo
}
