/*
 *  This source is for MEZW65C_RAM on PPICO_BOARD weact_studio_rp2350b_core.
 *  W65C02S or W65C816S is controled by rp2350b_core.
 *
 *  https://x.com/akih_san
 *  https://github.com/akih-san
 *
 *  Target: MEZW65C02_RAM
 *  Date. 2026.2.08

; Copyright (c) 2026 Akihito Honda
;
; Released under the MIT license
;
; Permission is hereby granted, free of charge, to any person obtaining a copy of this
; software and associated documentation files (the “Software”), to deal in the Software
; without restriction, including without limitation the rights to use, copy, modify, merge,
; publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
; to whom the Software is furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all copies or
; substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
; BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
; ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
; CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
*/
#include <stdio.h>
//#include <stdlib.h>
//#include <assert.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "w65.h"
#include "iodev.h"

#define rom_org	(uint32_t)0xffe7
#define cpu_type (uint32_t)0xfff9

static const uint8_t rom[] = {
	0xA9,0x00,      //    lda  #$00
	0xA2,0xFF,      //    ldx  #$ff
	0x9A,           //    txs              ; set SP (W65C02 OPECODE)
	0x1B,           //    tas              ; set SP (W65C816 OPECODE)
	0xEA,           //    nop
	0xEA,           //    nop
	0xBA,           //    tsx              ; SP -> X (W65C02 OPECODE)
	0xE8,           //    inx
	0x8E,0xF9,0xFF, //    stx  cpu_type    ; 0:W65C02 1:W65C816
	0x18,           //    clc              ; set native mode
	0xFB,           //    xce              ; if cpu=W65C02 then xce = nop operation
	0xCB,           //    wai              ; wait CPU
// loop:
	0x80, 0xFE,     //    bra  loop
// 0xfff9
	0xFF,           //cpu_type:	db	$FF
	0xF6,0xFF,      //    FDB  NMIBRK		; NMI
	0xE7,0xFF,      //    FDB  RESET		; RESET
	0xF6,0xFF       //    FDB  IRQBRK		; IRQ/BRK
};

static void bus_hold_req(void) {
	// Set address bus as output
	gpio_set_dir_masked64(ADR_MASK, ADR_DIR_OUT);		//A(0 - 15) for direction output
    gpio_set_dir_masked64(W65_DTBS, DBUS_DIR_OUT);		// DBUS set as output
	gpio_put_masked64(ADR_MASK, 0x0000);	// set address = 0x0000
	gpio_put_masked64(W65_DTBS, 0x00);		// set data bus = 0
	gpio_set_dir(W65_RW, GPIO_OUT);						// R/W# for direction output
	gpio_put(W65_RW, 1);								// SRAM READ mode
}

static void bus_release_req(void) {
	// Set address bus as input
	gpio_set_dir_masked64(ADR_MASK, ADR_DIR_IN);		//A(0 - 15) for direction input
	gpio_set_dir_masked64(W65_DTBS, DBUS_DIR_IN);		// DBUS set as input
	gpio_set_dir(W65_RW, GPIO_IN);						// R/W# for direction input
}

void start_cpu(void) {
	bus_release_req();
	gpio_put(W65_BE, 1);				// active cpu bus & RWB
	gpio_put(W65_RES, 0);				// cpu reset
	sleep_us(100);
	gpio_put(W65_RES, 1);				// cpu release reset
}

int reset_cpu(void)
{
	int i, c;
	uint8_t buf[sizeof(rom)];

	c = 0;
	printf("CHECK CPU TYPE ....");

	while ( c < 10 ) {
		// write cpu emulation mode operation program
		write_sram(rom_org, &rom[0], sizeof(rom));
		read_sram(rom_org, &buf[0], sizeof(buf));
		if (memcmp(&rom[0], &buf[0], sizeof(rom)) != 0) {
			printf("Memory Write Error\r\n");
	        util_addrdump("WR: ", rom_org, &rom[0], sizeof(rom));
	        util_addrdump("RD: ", rom_org, &buf[0], sizeof(buf));
			stop_fw(RESET_CPU_ERROR);
		}

		start_cpu();
		sleep_us(100);
		gpio_put(W65_BE, 0);				// BE=0 : CPU bus high
		bus_hold_req();
		read_sram(cpu_type, &buf[0], 1);
		switch (buf[0]) {
			case 0:
				printf("W65C02\r\n");		// CPU : W65C02
				return 0;
			case 1:
				printf("W65C816\r\n");		// CPU : W65C816
				return 1;
		}
		c++;
		printf("Unknown CPU type(%02x)\r\n", buf[0]);
		printf("RESET CPU...");
		sleep_ms(300);
	}
	printf("ERROR! CPU NOT WORKING...\r\n");
	stop_fw(RESET_CPU_ERROR);
}

//------------
// event loop
//------------
void board_event_loop(void) {

	for (;;) {
		maskTimer = 0;
		for (;;) {
			if (!gpio_get(W65_RDY)) break;
			if (gpio_get(W65_RDY) && irq_flg) {
				make_irq();		// IRQ 1 -> 0 -> 1 : IRQ interrupt occurs
				irq_flg = 0;
			}
		}
		maskTimer = 1;

		// PAUSE CPU until IRQ 1-> 0 -> 1
		gpio_put(W65_BE, 0);	// BE=0
		bus_hold_req();			// PICO becomes a busmaster
		bus_master_operation();

		if(wup_flg) {
			//test
			//printf("wup_flg(%02x)\r\n",wup_flg);
			//test
			return;		// abort event loop. Return to main()
		}
		bus_release_req();
		// Release BUS
		gpio_put(W65_BE, 1);	// BE=1
		make_irq();				// IRQ 1 -> 0 -> 1 : START CPU again
		if (nmi_sig) {
			//test
			//printf("nmi_sig(%02x)\r\n",nmi_sig);
			//test
			gpio_put(W65_NMI, 0);
			gpio_put(W65_NMI, 1);
			nmi_sig = 0;
		}
	}
}

void continue_action(void) {

	bus_release_req();
	// Release BUS
	gpio_put(W65_BE, 1);	// BE=1
	make_irq();				// IRQ 1 -> 0 -> 1 : START CPU again
}
