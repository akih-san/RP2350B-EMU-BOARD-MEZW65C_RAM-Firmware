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

#ifndef IODEV_DEF
#define IODEV_DEF 1

// define GPIO
#define W65_ADR  (uint)0		//pull up
#define W65_CLK  (uint)20
#define W65_DCK  (uint)21		//pull up
#define W65_RDY  (uint)22		//pull up
#define W65_BE   (uint)24		//pull down
#define W65_IRQ  (uint)26		//pull up
#define W65_RW   (uint)27		//pull up
#define W65_NMI  (uint)28		//pull up
#define W65_RES  (uint)29		//none pulldown (pull down by MEZW65C02_RAM)
#define W65_SDCS (uint)38		//pull up
#define W65_DBUS (uint)30		//pull down

#define W65_CLK_MASK  ((uint64_t)1 << W65_CLK)
#define W65_DCK_MASK  ((uint64_t)1 << W65_DCK)
#define W65_RDY_MASK  ((uint64_t)1 << W65_RDY)
#define W65_BE_MASK   ((uint64_t)1 << W65_BE)
#define W65_IRQ_MASK  ((uint64_t)1 << W65_IRQ)
#define W65_RW_MASK   ((uint64_t)1 << W65_RW)
#define W65_NMI_MASK  ((uint64_t)1 << W65_NMI)
#define W65_RES_MASK  ((uint64_t)1 << W65_RES)
#define W65_SDCS_MASK ((uint64_t)1 << W65_SDCS)

#define ADR_WIDTH 16
#define DBUS_WIDTH 8

#define ADR_MASK  (uint64_t)0xffff
#define DBUS_MASK ((uint64_t)0xff << W65_DBUS)
#define W65_DTBS DBUS_MASK

#define ADR_DIR_OUT (uint64_t)0xffff
#define ADR_DIR_IN (uint64_t)0

#define DBUS_DIR_OUT ((uint64_t)0xff << W65_DBUS)
#define DBUS_DIR_IN (uint64_t)0

extern file_header fh;
extern float clk_fs;

extern uint8_t	cpu_flg;	// CPU flg 0:W65C02 1:W65C816
extern uint8_t	wup_flg;
extern uint8_t	nmi_sig;
extern uint8_t irq_flg;
extern uint8_t ctlq_ev;		// Ctrl+Q flag;
extern uint8_t irqMask;
extern int maskTimer;

extern void bus_master_operation(void);
extern void make_irq(void);
extern void util_addrdump(const char *, uint32_t , const void *, unsigned int);
extern void stop_fw(int);

// read/write memory
extern void write_sram(uint32_t, const uint8_t*, unsigned int);
extern void read_sram(uint32_t , uint8_t *, unsigned int);

#endif  //IODEV_DEF
