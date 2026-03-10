/* 
 *  Target: weact_studio_rp2350b_core
 *  Written by Akihito Honda (Aki.h @akih_san)
 *  https://x.com/akih_san
 *  https://github.com/akih-san
 *
 * Date: 2026.02.18
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

#ifndef W65_DEF
#define W65_DEF 1

#define BS		0x08
#define CR		0x0d
#define LF		0x0a
#define SPACE	0x20
#define CTL_Q	0x11

#define arg_num 5
#define line_size 81

#define SECTOR_SIZE	512
#define BUF_SIZE 256
#define SZ_STR 128
#define URT_BUF_SIZE 128

#define NUM_DRIVES		8

#define TIM20240101	16071	// 16071days from 1980

#ifdef PICO_RP2040		// 125MHz
#define W65_CLK_DIV_2 31.25	// division rate for 2MHz
#define W65_CLK_DIV_5 12.5	// division rate for 5MHz 
#else  // rp2350 150MHz
#define W65_CLK_DIV_2 37.5	// division rate for 2MHz
#define W65_CLK_DIV_5 15.0	// division rate for 5MHz
#endif
#define WRAP_COUNT 1		// count 0, 1

#define FW_OK 0
#define RESET_CPU_ERROR 1
#define FW_MOUNT_ERROR 2
#define CHECK_RAM_ERROR 3
#define MOUNT_SD_CARD_ERROR 4
#define SETUP_MONITOR_ERROR 5

//;--------- MEZW65C_RAM file header --------------------------
//
//#pragma pack(push)
#pragma pack(1)
typedef struct {

	/*
	*	*** NOTE***
	*
	*	When bios_sw=1, the following parameters are not used and are reserved.
	*
	*		picif_p
	*		irq_sw
	*		reg_tblp
	*		reg_tsize
	*		nmi_sw
	*/
	uint8_t		op1;			// if bios_sw=1, PROGRAM BANK:(W65C816), 0:(W65C02)
								// if bios_sw=0, JMP opecode
	uint16_t	cstart_addr;
	uint8_t		op2;			// if bios_sw=1, DATA BANK:(W65C816), 0:(W65C02)
								// if bios_sw=0, JMP opecode
	uint16_t	wstart_addr;
	uint16_t	direct_page;	// DIRECT PAGE : (W65C816), reserve : (W65C02)
	uint8_t		mezID[8];		// Unique ID "MEZW65C",0 (This area is used by monitor for invoking bios_call program)
	uint32_t	load_p;			// Load program address 24bit:(W65C816), 16bit:(W65C02)
	uint32_t	picif_p;		// pic i/o shared memory address
	uint8_t		sw_816;			// 0 : W65C02, 1: W65C816 native mode, 2:works in both modes
	uint8_t		irq_sw;			// 0 : no use IRQ console I/O
								// 1 : use IRQ timer interrupt driven console I/O
	uint16_t	reg_tblp;		// register save pointer( NMI )
	uint16_t	reg_tsize;		// register table size
	uint8_t		nmi_sw;			// 0 : No NMI support, 1: NMI support
	uint8_t		bios_sw;		// 0 : standalone program
								// 1 : user program (bios call program or DOS65)
								// 2 : monitor program( .SYS file)
} file_header;
//#pragma pack(pop)

#include "ff.h"
typedef struct {
    int exist;
    FIL *filep;
} drive_t;

#endif //W65_DEF