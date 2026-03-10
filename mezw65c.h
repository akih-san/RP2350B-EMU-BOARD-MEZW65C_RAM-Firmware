/* 
 *  Target: weact_studio_rp2350b_core
 *  Written by Akihito Honda (Aki.h @akih_san)
 *  https://x.com/akih_san
 *  https://github.com/akih-san
 *
 * Date: 2026.02.01
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

#ifndef MEZW65C_DEF
#define MEZW65C_DEF 1

//explicit  Declaration
void stop_fw(int);
static void _set_led(bool);
static void mount_sd_card(void);
static uint8_t del_space(char *);
static int get_line(char *, int);
static FRESULT scan_files(void);
static FRESULT scan_files1(void);
static void set_arg(char *);
static FRESULT change_directory(void);
static FRESULT print_com(void);
static uint32_t get_hex(char *);

static void setup_monitor(void);
static int load_program(uint8_t *);
static uint8_t del_space(char *);
static void load_config(void);
static FRESULT drive_cpu(void);
static void prt_reload(void);
static FRESULT boot_file(void);
static FRESULT return_cpu(void);
static FRESULT print_reg(void);
static FRESULT load_file(void);
static FRESULT wstart_prog(void);
static FRESULT restart_prog(void);
static FRESULT mon_prog(void);
static FRESULT mem_dump(void);
static FRESULT see_file(void);
static FRESULT flash_apl(void);
static int in_file(void);
static unsigned int str_inf(char *, uint8_t);
static int chk_dsk(void);
static FRESULT open_dos65(void);
static int open_dskimg(void);
static FRESULT close_dos65(void);

//external reference
extern uart_inst_t *UART_ID;

extern int reset_clk(void);
extern void port_init(void);
extern void clk_init(void);
extern void start_cpu(void);
extern void write_sram(uint32_t, const uint8_t*, unsigned int);
extern void read_sram(uint32_t , uint8_t *, unsigned int);
extern void init_uart(void);
extern void setup_pio(void);
extern int pico_fatfs_reboot_spi(void);
extern uint pico_fatfs_get_clk_slow_freq(void);
extern uint pico_fatfs_get_clk_fast_freq(void);
extern void util_addrdump(const char *, uint32_t, const void *, unsigned int);
extern int reset_cpu(void);
extern uint32_t check_ram(void);
//extern void stopIntervalTimer(void);
extern void startIntervalTimer(void);
extern int u_getch(void);
extern unsigned int get_str(char*, uint8_t);

//------------
// event loop
//------------
extern void board_event_loop(void);
extern void continue_action(void);
extern void port_init(void);

extern bool timer_callback( repeating_timer_t * );

#define BINV_SIZE 7
#define INPUT_UART	0
#define INPUT_FILE	1
#define mezID_off 8

#define INPUT_UART	0
#define INPUT_FILE	1

// local type define 
typedef struct {
//---transfer data to monitor mezID area---
	uint8_t		sw;		// +0 0: user program none 
						//    1: user program exist
	uint16_t	addr;	// +1 (copied caddr or waddr)
	uint8_t		pbnk;	// +3 PBR
	uint8_t		dbnk;	// +4 DBR
	uint16_t	dp;		// +5 DPR
//----------------------------------------
	uint16_t	caddr;	// user program cold start
	uint16_t	waddr;	// user program warm start
	uint8_t		sw_816;	// 0 : W65C02, 1: W65C816 native mode, 2:works in both modes
	TCHAR	fname[30];	// BIOS program faile name
} bios_inv;

typedef struct {
	const TCHAR *conf;
	float *val;
} sys_param;

typedef struct {
	char *cmd_name;
	FRESULT (*func)(void);
} com_param;

typedef struct {
	uint16_t REGA;		//ds 2 ; Accumulator A
	uint16_t REGX;		//ds 2 ; Index register X
	uint16_t REGY;		//ds 2 ; Index register Y
	uint16_t REGSP;		//ds 2 ; Stack pointer SP
	uint16_t REGPC;		//ds 2 ; Program counter PC
	uint8_t  REGPSR;	//ds 1 ; Processor status register PSR
	uint8_t  REGPB;		//ds 1 ; Program Bank register
	uint8_t  REGDB;		//ds 1 ; Data Bank register
	uint16_t REGDP;		//ds 2 ; Direct Page register
} reg816;

typedef struct {
	uint8_t  REGA;		//ds 1 ; Accumulator A
	uint8_t  REGX;		//ds 1 ; Index register X
	uint8_t  REGY;		//ds 1 ; Index register Y
	uint8_t  REGSP;		//ds 1 ; Stack pointer SP
	uint16_t REGPC;		//ds 2 ; Program counter PC
	uint8_t  REGPSR;	//ds 1 ; Processor status register PSR
} reg02;

#endif //MEZW65C_DEF