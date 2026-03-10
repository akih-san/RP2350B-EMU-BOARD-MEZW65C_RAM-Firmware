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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/clocks.h"

#include "urt_dev.h"
#include "ff.h"
#include "w65.h"
#include "mezw65c.h"

///////////////////////////////////////
// global values
///////////////////////////////////////
uint8_t buffer[SECTOR_SIZE];
uint8_t	cpu_flg;				// CPU flg 0:W65C02 1:W65C816
uint8_t	wup_flg;
uint8_t	nmi_sig;
file_header fh;
drive_t drives[NUM_DRIVES];
uint8_t ctlq_ev;		// Ctrl+Q flag
uint8_t irq_flg;
uint8_t irqMask;
int maskTimer;

uint8_t cin_no;			// 0 : INPUT_UART 1 : INPUT_FILE

float clk_fs;
uint32_t bioreq_ubuffadr;
uint32_t bioreq_cbuffadr;

int (*get_char[2])(void) = {
	u_getch,
	in_file
};

unsigned int (*strin_func[2])(char *, uint8_t) = {
	get_str,
	str_inf
};

///////////////////////////////////////
// local values
///////////////////////////////////////
static FATFS fs;
static char *arg[arg_num];
static char line_buf[line_size];
static bios_inv binv;
static uint8_t mon;
static uint32_t raw_addr;
static uint8_t load_flg;
static bios_inv binv;
static uint8_t mon;
static uint16_t frd_ptr;		// read pointer : cin_file[frd_ptr]
static uint16_t fin_cnt;
static uint16_t fin_size;
static char fin_name[13];		// redirect file name
static FILINFO fileinfo;

static const uint32_t PIN_LED = 25;  // GPIO25 is board LED
static bool _led = false;

#define num_param 2
static sys_param t_conf[num_param] = {
	{"CLK_DIV", &clk_fs},
	{"REQ_HDR", (float *)&bioreq_ubuffadr}	// can NOT use pico, pico2
};

#define num_com 15
static com_param cmd[num_com+1] = {
	{"LS", scan_files},
	{"DIR",scan_files1},
	{"CD",change_directory},
	{"LOAD",load_file},
	{"CSTART",restart_prog},
	{"WSTART",wstart_prog},
	{"RETI",return_cpu},
	{"REG",print_reg},
	{"MONITOR", mon_prog},
	{"MDUMP",mem_dump},
	{"SHOW",see_file},
	{"FLASH",flash_apl},
	{"DOS65",open_dos65},
	{"HELP",print_com},
	{"?",print_com},
	{"",boot_file}
};

static const char *mezID = "MEZW65C";
static char *board_name = "RP2350B EMU BORD firmware Rev1.0 for MEZW65C_RAM Rev1.6";
static const TCHAR *conf02 = "PICO02.CFG";
static const TCHAR *conf16 = "PICO16.CFG";
static char *mon02 = "/MON02.SYS";
static char *mon16 = "/MON16.SYS";

static char *dosdir	= "DOS_DISK";
static char *dos65 = "/DOS65.SYS";

static FIL files[NUM_DRIVES];
static FIL in_fl;

//
// main program
//
int main(void)
{
	int c;
	char *buf, *res;
	FRESULT rs;
	
	cin_no	= INPUT_UART;	// input console (UART)
	wup_flg = 0;	// clear wakeup flag for NMI
	nmi_sig = 0;
	fh.nmi_sw = 0;
	raw_addr = 0;
	load_flg = 0;
	binv.sw = 0;
	clk_fs = W65_CLK_DIV_2;	// default 2MHz
	cpu_flg = 0;			// start up: 6502 mode

	stdio_init_all();
	sleep_ms(1000);

    // LED off
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    _set_led(false);

	//	clk_init();				// initial CLK = 2MHz
	port_init();
	setup_pio();

//#define USE_UART
	#ifdef USE_UART
	// initialize UART0
	init_uart();
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\r\n");
	#endif

	clk_init();				// initial CLK = 2MHz
	cpu_flg = reset_cpu();

	check_ram();			// ram check
	mount_sd_card();		// mount sd
	load_config();
	reset_clk();

	if (!cpu_flg) printf("CPU : W65C02\r\n");
	else printf("CPU : W65C816\r\n");
	printf("CPU CLOCK %2.2fMHz\r\n\n", clock_get_hz(clk_sys)/clk_fs/(WRAP_COUNT+1)/1000000);

	startIntervalTimer();

//test
//	gpio_put(46, 0);
//	sleep_us(1);
//	gpio_put(46, 1);
//test
	setup_monitor();
	printf("\r\n%s\n\r", board_name);

	/* print root directory */
	arg[0] = "";
	scan_files();

	for(;;) {
		f_getcwd(&line_buf[0], line_size);
		printf("%s> ", &line_buf[0]);								/* pirnt prompt */
		for(c = 0; c < arg_num; c++) arg[c] = 0;

		if (!get_line(&line_buf[0], line_size)) continue;			/* console input */
		/* search command */
		for( c=0;  c < num_com; c++ ) {
			if ((res = strstr(&line_buf[0], cmd[c].cmd_name ))) {
				buf = (char *)(res+strlen(cmd[c].cmd_name));		/* buf = next point */
				set_arg(buf);
				break;
			}
		}
		// do command operation

		if ( c == num_com ) set_arg(&line_buf[0]);
		rs = (*cmd[c].func)();								/* call cmd function */

		if( rs == (FRESULT)-2 ) printf("\r\nAssert NMI interrupt.\r\n");
		else if ( rs != FR_OK ) printf("Invalid( command | file | directory )\r\n");
	}

}

static void _set_led(bool flag)
{
    gpio_put(PIN_LED, flag);
    _led = flag;
}

void stop_fw(int count)
{
	printf("Stop Firmware(%d)\n\r", count);
    while (true) {
        for (int i = 0; i < count; i++) {
            _set_led(true);
            sleep_ms(250);
            _set_led(false);
            sleep_ms(250);
        }
        _set_led(false);
        sleep_ms(500);
    }
}

//
// mount SD card
//
// FRESULT :
//	FR_OK					 (0) Function succeeded
//	FR_DISK_ERR,			 (1) A hard error occurred in the low level disk I/O layer
//	FR_INT_ERR,				 (2) Assertion failed
//	FR_NOT_READY,			 (3) The physical drive does not work
//	FR_NO_FILE,				 (4) Could not find the file
//	FR_NO_PATH,				 (5) Could not find the path
//	FR_INVALID_NAME,		 (6) The path name format is invalid
//	FR_DENIED,				 (7) Access denied due to a prohibited access or directory full
//	FR_EXIST,				 (8) Access denied due to a prohibited access
//	FR_INVALID_OBJECT,		 (9) The file/directory object is invalid
//	FR_WRITE_PROTECTED,		 (10) The physical drive is write protected
//	FR_INVALID_DRIVE,		 (11) The logical drive number is invalid
//	FR_NOT_ENABLED,			 (12) The volume has no work area
//	FR_NO_FILESYSTEM,		 (13) Could not find a valid FAT volume
//	FR_MKFS_ABORTED,		 (14) The f_mkfs function aborted due to some problem
//	FR_TIMEOUT,				 (15) Could not take control of the volume within defined period
//	FR_LOCKED,				 (16) The operation is rejected according to the file sharing policy
//	FR_NOT_ENOUGH_CORE,		 (17) LFN working buffer could not be allocated, given buffer size is insufficient or too deep path
//	FR_TOO_MANY_OPEN_FILES,	 (18) Number of open files > FF_FS_LOCK
//	FR_INVALID_PARAMETER	 (19) Given parameter is invalid

static void mount_sd_card(void) {

    FRESULT fr;     /* FatFs return code */

	for (int i = 0; i < 5; i++) {   
        fr = f_mount(&fs, "", 1);
        if (fr == FR_OK) break;
        printf("mount error fr(%d) -> retry\r\n", fr);
        pico_fatfs_reboot_spi();
    }
    if (fr != FR_OK) {
        printf("mount error fr(%d) -> retry\r\n", fr);
        stop_fw(MOUNT_SD_CARD_ERROR);		// halt!
    }
    printf("Operation  clk_slow: %3.1f KHz, clk_fast: %2.1f MHz\r\n",
        (float)pico_fatfs_get_clk_slow_freq() / 1000,
        (float)pico_fatfs_get_clk_fast_freq() / 1000000);
    printf("mount ok.\r\n");

    switch (fs.fs_type) {
        case FS_FAT12:
            printf("Type is FAT12\r\n");
            break;
        case FS_FAT16:
            printf("Type is FAT16\r\n");
            break;
        case FS_FAT32:
            printf("Type is FAT32\r\n");
            break;
        default:
            printf("Type is unknown\r\n");
	        stop_fw(MOUNT_SD_CARD_ERROR);		// halt!
    }
//    2097152 =  (1024 * 1024 * 1024) / 512    1Gbytes = 1024 * 1024 * 1024
    printf("Card size: %2.1f GB\r\n", (float)(fs.csize * fs.n_fatent) / 2097152);
}

static uint8_t del_space(char *bytes) {
	uint8_t pos = 0;
	uint8_t i = 0;
	char c;
	
	while( (c = bytes[i++]) ) {
		if (c == '\r' || c == '\n' || c == ' ') {
			continue;
		}
		bytes[pos++] = c;
	}
	bytes[pos] = c;		// save NULL code
	return pos;
}

static int get_line(char *s, int length) {
	char n;
	int c;
	
	for (c=0;;) {
		n = (char)u_getch();
		if ( n == BS ) {
			if ( c > 0) {
				putchar(BS);
				putchar(' ');
				putchar(BS);
				c--;
				s--;
			}
			continue;
		}
		if ( n == CR || n == LF ) {
			*s = 0x00;
			printf("\r\n");
			break;
		}
		if ( n < SPACE ) continue;
		if ( c <= length-1 ) {
			putchar(n);
			if ( n >='a' && n <='z' ) n -= 0x20;		// lower to upper
			*s++ = n;
			c++;
		}
	}
	return c;
}

static FRESULT scan_files(void) {
	FRESULT res;
	FILINFO fno;
	FIL fl;
	DIR dir;
	char *fn;

	res = f_opendir(&dir, arg[0]);
	if (res == FR_OK) {		// directory
		f_getcwd( &buffer[0], SZ_STR);
		// print directory name
		if ( *arg[0] == '\0' ) {	//open current directory
			printf("[ %s ]\r\n", &buffer[0] );
		}
		else { // open directory specified strings with arg[0]
			if ( !buffer[1] ) { //current directory is root directory
				printf("[ /%s ]\r\n", arg[0] );
			}
			else { //current directory is NOT root directory
				printf("[ %s/%s ]\r\n", &buffer[0], arg[0] );
			}
		}
		// print files
		for (;;) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0) break;
			if (fno.fname[0] == '.') continue;				/* ignore '.'  */
			fn = fno.fname;
			if (fno.fattrib & AM_DIR) {					/* Directory */
				printf("  %12s\t<DIR>\r\n", fno.fname);
			}
			else {									/* file */
				printf("  %12s\t%ld bytes.\r\n", fn, fno.fsize);
			}
		}
		f_closedir(&dir);
	}
	else {
		if (res == FR_NO_PATH) { // check file
			res = f_open(&fl, (const TCHAR *)arg[0], FA_READ);
			if ( res == FR_OK ) {
				printf("  %12s\t%ld bytes.\r\n", arg[0], f_size(&fl));
				f_close( &fl );
			}
		}
	}
	return res;
}

static FRESULT scan_files1(void) {
	FRESULT res;
	FILINFO fno;
	FIL fl;
	uint8_t	fcnt;
	char *fn, *path;
	DIR dir;

	path = arg[0];
	fcnt = 0;
	
	res = f_opendir(&dir, path);
	if (res == FR_OK) {		// directory
		f_getcwd(&buffer[0], SZ_STR);
		// print directory name
		if ( *path == '\0' ) printf("[ %s ]\r\n", &buffer[0] );
		else if ( !(buffer[1]) ) printf("[ /%s ]\r\n", path );
		else printf("[ %s/%s ]\r\n", &buffer[0], path );

		// print files
		for (;;) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0) {
				if ( fcnt ) {
					printf("\r\n");
					break;
				}
			}
			if (fno.fname[0] == '.') continue;				/* ignore '.'  */
			fn = fno.fname;
			if (fno.fattrib & AM_DIR) {					/* Directory */
				printf("[%12s] ", fn);
			}
			else {									/* file */
				printf(" %12s  ", fn);
			}
			fcnt += 1;
			if (fcnt == 5 ) {
				printf("\r\n");
				fcnt = 0;
			}
		}
		f_closedir(&dir);
	}
	else {
		if (res == FR_NO_PATH) { // check file
			res = f_open(&fl, (const TCHAR *)path, FA_READ);
			if ( res == FR_OK ) {
				printf("  %12s\t%ld bytes.\r\n", path, f_size(&fl));
				f_close( &fl );
			}
		}
	}
	return res;
}

static void set_arg(char *buf) {

	int i;
	
	i=0;
	while( i < arg_num ) {
		while( *buf == ' ' ) buf++;
		arg[i] = buf;
		if ( !*buf ) break;
		while( *buf && *buf != ' ' ) buf++;
		if ( !*buf ) break;
		if ( *buf == ' ' ) *buf++ = 0;
		i++;
	}
}

static FRESULT change_directory(void) { return(f_chdir( arg[0] )); } 

static FRESULT print_com(void) { 
	
	printf("\r\n<< MEZW65C_RAM Firmware Built-in command >>\r\n");
	printf("  DOS65  (Key CTL+T : terminate DOS65)\r\n");
	printf("  LS | DIR [file name | directory name\r\n");
	printf("  CD [directory name]\n\r");
	printf("  LOAD [L=load address(Hex)] file name\r\n");
	printf("  MDUMP address(Hex)\r\n");
	printf("  MONITOR [W]\r\n");
	printf("  CSTART\r\n");
	printf("  WSTART\r\n");
	printf("  RETI\r\n");
	printf("  REG\r\n");
	printf("  SHOW file name\r\n");
	printf("  FLASH\r\n");
	printf("  HELP | ?\r\n");
	return FR_OK;
}

static uint32_t get_hex(char *buf) {

	uint32_t hexval;
	char ch;
	uint32_t d;
	int c;
	
	hexval = 0;
	c = 0;
	while( *buf ) {
		ch = *buf++;
		if (ch >= 'A' && ch <= 'F') d = (uint32_t)(ch-'A'+10);
		else if (ch >= '0' && ch <= '9') d = (uint32_t)(ch-'0');
		else break;
		hexval = hexval*16 + d;
		if (++c == 8) break;
	}
	return hexval;
}

//
// setup monitor at starting MESW65C_RAM
//
static void setup_monitor(void) {

	int	rs;
	uint8_t dat;
	
	printf("Install Monitor Program..........\r\n");

	if (cpu_flg) {
		arg[0] = mon16;
		mon = 1;
	}
	else {
		arg[0] = mon02;
		mon = 0;
	}
	rs = load_program((uint8_t *)arg[0]);
	if ( rs ) stop_fw(SETUP_MONITOR_ERROR);

	//
	// Start CPU
	//
//test
//	gpio_put(46, 0);
//	sleep_us(1);
//	gpio_put(46, 1);
//test
	start_cpu();
	drive_cpu();
	return;
}

//
// load program from SD card
//
static int load_program(uint8_t *fname) {
	
	FRESULT	fr;
	FIL fl;
	uint16_t file_size;
	
	UINT	btr, br;
	uint16_t	cnt;
	uint32_t	adr, adr0;
	file_header *header;
	
	header = (file_header *)&buffer[0];

	fr = f_open(&fl, (TCHAR *)fname, FA_READ);
	if ( fr != FR_OK ) {
		printf("File Open Error.\r\n");
		return((int)fr);
	}
	adr = 0;
	cnt = file_size = (uint16_t)f_size(&fl);				// get file size
	btr = SECTOR_SIZE;										// default 512byte
	while( cnt ) {
		fr = f_read(&fl, (void *)&buffer[0], btr, &br);
		if (fr == FR_OK) {
			if ( !adr ) {		// if read sector is 1st sector then ...
				if ( raw_addr ) {	// if specified load address by raw_addr then ...
					adr = adr0 = raw_addr;
				}
				else {
					if (!strstr((const char *)header->mezID, mezID)) {	//check MEZW65C_RAM file ID
						printf("Invalid MEZW65C file.\r\n");
						fr = FR_INVALID_OBJECT;
						break;
					}
					if (header->sw_816==1 && !cpu_flg) {
						printf("Work on only W65C816..\r\n");
						fr = FR_INVALID_OBJECT;
						break;
					}
					adr = adr0 = header->load_p;
					if (!load_flg) {
						if ( header->bios_sw == 1 ) {	// file is user program
							binv.sw = header->bios_sw;
							binv.caddr = header->cstart_addr;
							binv.waddr = header->wstart_addr;
							binv.dp = header->direct_page;			//direct page
							binv.pbnk = header->op1;				//program bank
							binv.dbnk = header->op2;				//data bank
							binv.sw_816 = header->sw_816;
							sprintf((char *)binv.fname, "%s", fname);
						}
						else {
							// copy file header */
							fh.bios_sw = header->bios_sw;
							fh.load_p = header->load_p;
							fh.sw_816 = header->sw_816;
							fh.cstart_addr = header->cstart_addr;
							fh.wstart_addr = header->wstart_addr;
							fh.picif_p = header->picif_p;
							fh.irq_sw = header->irq_sw;
							fh.reg_tblp = header->reg_tblp;
							fh.reg_tsize = header->reg_tsize;
							fh.nmi_sw = header->nmi_sw;
							/* PIC common memory address */
							bioreq_ubuffadr = fh.picif_p;
							bioreq_cbuffadr = bioreq_ubuffadr + 2;
							if ( !fh.bios_sw ) binv.sw = 0;		// standalone(clear bios call flag)
						}
					}
				}
			}
			write_sram(adr, &buffer[0], (unsigned int)br);
			adr += (uint32_t)br;
			cnt -= (uint16_t)br;
			if (btr > (UINT)cnt) btr = (UINT)cnt;
		}
		else break;
	}
	if (fr == FR_OK) {
		printf("Load %s : Adr = $%06lX, Size = $%04X\r\n", fname, adr0, file_size);
	}
	else {
		if (fr != FR_INVALID_OBJECT) printf("File Load Error.\r\n");
	}
	f_close(&fl);
	return((int)fr);
}

//
// mount SD card
//
static int disk_init(void)
{
    if (f_mount(&fs, "0://", 1) != FR_OK) {
        printf("Failed to mount SD Card.\n\r");
        return -2;
    }

    return 0;
}

static void load_config(void)
{
	FIL fl;
	FRESULT	fr;
	char *buf, *dummy;
	const TCHAR *conf;
	uint16_t cnt, size;
	uint16_t adr;
	int i;
	TCHAR *str;
	float a, *vari;
	
	str = (TCHAR *)&buffer[0];
	conf = ( cpu_flg ) ? conf16 : conf02;
	
	printf("\r\nLoad config file: %s\r\n",(const char *)conf);
	
	fr = f_open(&fl, conf, FA_READ);
	if ( fr != FR_OK ) {
		printf("%s not found!\r\n", conf);
		return;
	}

	while ( f_gets(str, 80, &fl) ) {	// max 80 characters
		// delete space
		del_space(str);
		if (str[0] == ';' || str[0] == 0 ) continue;

		// search keywoard
		for( i=0; i<num_param; i++ ) {
			if (!strstr(str, t_conf[i].conf)) continue;
			if (str[strlen(t_conf[i].conf)] != '=') continue;
			// get value
			
			vari = t_conf[i].val;
			buf = &str[strlen(t_conf[i].conf)+1];			// get address of value string
			a = (float)strtof((const char *)buf, &dummy);	// get value

			if (!a) printf("Error! Illegal Formatting value!!\r\n");
			else *vari = a;		// set value

			printf("Set %s = %2.2f\r\n", (const char *)t_conf[i].conf, *vari);
		}
	}
	if (!f_eof(&fl)) {
		printf("CONFIG KEY NOT FOUND!\r\n");
	}
	f_close( &fl );
	return;
}

static FRESULT drive_cpu(void) { 

	FRESULT rs;

	ctlq_ev = 0;	// clear Ctrl+Q key flag
	nmi_sig = 0;
	irq_flg = 0;

	if( !fh.irq_sw ) irqMask = 1;
	else irqMask = 0;
	board_event_loop();
	
	// wup_flg = 0xFF : NMI, wup_flg = 1 : sleep( or terminate )
	rs = (FRESULT)(wup_flg - 1);		// get wakeup reason -2:NMI 
	wup_flg = 0;						// clear wakeup flag for NMI

	return rs;
}

static void prt_reload(void) { 
	printf("Reload monitor %s\r\n", arg[0]);
}

static FRESULT boot_file(void) { 

	FRESULT rs;
	uint8_t flag, m;
	UINT cnt;
	
	if ( strstr((const char *)arg[0], mon02+1) ) m = 0;
	else if ( strstr((const char *)arg[0], mon16+1) ) m = 1;
	else m = 2;
	if ( m != 2) {
		mon = m;
		prt_reload();
		binv.sw = 0;	//clear user program
		rs = (FRESULT)load_program((uint8_t *)arg[0]);
		if ( rs ) return 0;

		start_cpu();
		rs = drive_cpu();
		return rs;
	}
	
	// check mark '<' for FILE INPUT modde
	if ( *arg[1] == '<' ) {
		sprintf(&fin_name[0], "%s", arg[2]);
		fin_cnt = 0xffff;	// set open flag
		cin_no = INPUT_FILE;								// change console input UART to FILE
	}

	//check DOS65.SYS
	if ( strstr((const char *)arg[0], dos65+1) ) {
		rs = open_dos65();
		return rs;
	}
	
	flag = 0;
	printf("Flie(%s) loading...\r\n", arg[0]);
	
	rs = (FRESULT)load_program((uint8_t *)arg[0]);
	if ( rs ) return 0;

	if ( binv.sw ) {	// user program
		if ( cpu_flg ) {	// 65816
			if ( mon ) {	//mon16
				if ( !binv.sw_816 ) {
					arg[0] = mon02;
					mon = 0;
					flag = 1;
					printf("Reload %s for Emulation Mode...\r\n", arg[0]);
				}
			}
			else {	// mon02
				if ( binv.sw_816 ) {
					arg[0] = mon16;
					mon = 1;
					flag = 1;
					printf("Reload %s for Native Mode...\r\n", arg[0]);
				}
			}
			if ( flag ) {	// reroad monitor
				rs = (FRESULT)load_program((uint8_t *)arg[0]);
				if ( rs ) return 0;
			}
		}
		binv.addr = binv.caddr;
		write_sram( fh.load_p+mezID_off, (uint8_t *)&binv, BINV_SIZE );		// address of monitor's mezID
	}

	write_sram(0xFFFC, (uint8_t *)&fh.cstart_addr, 2);
	start_cpu();	//set monitor cold start
	rs = drive_cpu();
	if ( !rs ) {
		if ( !fh.bios_sw ) {	// if standalone prpgram is terminated, then need reload monitor
			if (cpu_flg) {
				arg[0] = mon16;
				mon = 1;
			}
			else {
				arg[0] = mon02;
				mon = 0;
			}
			prt_reload();
			rs = (FRESULT)load_program((uint8_t *)arg[0]);
			if ( rs ) return 0;
			start_cpu();
			rs = drive_cpu();
		}
		binv.sw = 0;	// terminate bios_call program
	}
	return rs;
}

static FRESULT return_cpu(void) { 
	
	FRESULT rs;

	continue_action();
	rs = drive_cpu();
	return rs;
}

static FRESULT print_reg(void) { 
	reg816 *reg_816;
	reg02 *reg_02;

	read_sram((uint32_t)fh.reg_tblp, &buffer[0], (unsigned int)fh.reg_tsize);

	if (fh.sw_816) {	/* 65816 */
		reg_816 = (reg816 *)&buffer[0];
		printf("A=$%04X X=$%04X Y=$%04X SP=$%04X PC=$%04X PSR=$%02X\r\n",
				reg_816->REGA, reg_816->REGX,reg_816->REGY,reg_816->REGSP,reg_816->REGPC,reg_816->REGPSR);
		printf("PBR=$%02X DBR=$%02X DPR=$%04X\r\n",reg_816->REGPB, reg_816->REGDB, reg_816->REGDP);
	}
	else {
		reg_02 = (reg02 *)&buffer[0];
		printf("A=$%02X X=$%02X Y=$%02X SP=$01%02X PC=$%04X PSR=$%02X\r\n",
				reg_02->REGA, reg_02->REGX,reg_02->REGY,reg_02->REGSP,reg_02->REGPC,reg_02->REGPSR);
	}
	return FR_OK;
}

static FRESULT load_file(void) {
	char *p;
	
	load_flg = 1;
	if ((p = strstr(arg[0], "L=" ))) {
		raw_addr = get_hex((char *)(p+2));
		load_program((uint8_t *)arg[1]);
	}
	else {
		raw_addr = 0;
		load_program((uint8_t *)arg[0]);
	}
	load_flg = 0;
	raw_addr = 0;
	return FR_OK;
}

static FRESULT wstart_prog(void) {
	FRESULT rs;

	rs = FR_NO_FILE;
	if ( binv.sw == 1) {
		binv.addr = binv.waddr;
		write_sram( fh.load_p+mezID_off, (uint8_t *)&binv, BINV_SIZE );		// address of monitor's mezID
		
		write_sram(0xFFFC, (uint8_t *)&fh.cstart_addr, 2);
		start_cpu();
		rs = drive_cpu();
	}
	return rs;
}

static FRESULT restart_prog(void) {
	FRESULT rs;

	rs = FR_NO_FILE;
	if ( binv.sw ==1 ) {
		binv.addr = binv.caddr;
		write_sram( fh.load_p+mezID_off, (uint8_t *)&binv, BINV_SIZE );		// address of monitor's mezID

		write_sram(0xFFFC, (uint8_t *)&fh.cstart_addr, 2);
		start_cpu();
		rs = drive_cpu();
	}
	return rs;
}

static FRESULT mon_prog(void) {
	FRESULT rs;
	char *p;
	uint8_t	sw;
	
	rs = FR_OK;

	sw = 0;					// monitor wakeup signal
	write_sram( fh.load_p+mezID_off, &sw, 1 );			// address of  fh.mezID

	if ((p = strstr(arg[0], "W" ))) write_sram(0xFFFC, (uint8_t *)&fh.wstart_addr, 2);
	else write_sram(0xFFFC, (uint8_t *)&fh.cstart_addr, 2);

	start_cpu();
	rs = drive_cpu();
	printf("\r\n");
	return rs;
}

static FRESULT mem_dump(void) {

	uint32_t addr;
	char *p;

	p = arg[0];
	addr = get_hex( p );

	read_sram(addr, &buffer[0], SZ_STR);		//128 bytes read memory
	util_addrdump("Mem ", addr, (const void *)&buffer[0], SZ_STR);
	return FR_OK;
}

static FRESULT see_file(void) {
	
	FIL fl;
	FRESULT	fr;
	UINT	br;
	file_header *header;
	uint16_t file_size;
	
	header = (file_header *)&buffer[0];
	fr = f_open(&fl, (TCHAR *)arg[0], FA_READ);
	if ( fr != FR_OK ) {
		printf("\r\nFile Open Error.\r\n");
		return 0;
	}

	file_size = (uint16_t)f_size(&fl);		// get file size
	fr = f_read(&fl, (void *)&buffer[0], SECTOR_SIZE, &br);
	if (fr == FR_OK) {

		printf("\r\n%s : Size = $%04X bytes.\r\n", arg[0], file_size);
		if (!strstr((const char *)header->mezID, mezID)) {	// not MEZW65C_RAM format
			printf("Not MEZW65C_RAM format file.\r\n");
		}
		else {
			printf("FIle load address : $%06lX\r\n", (unsigned long)header->load_p );
			printf("File Type : ");
			switch (header->bios_sw) {
				case 0:		// standalone program
				case 2:		// MONITOR program
					if ( !header->bios_sw ) printf("Stand-alone\n\r");
					else printf("MONITOR\n\r");
					printf("Operational Mode : ");
					if (!header->sw_816) printf("W65C02 (Emulation Mode)\n\r");
					else printf("W65C816 Native mode\n\r");
					printf("CSTART : $%04X\r\n",header->cstart_addr);
					printf("WSTART : $%04X\r\n",header->wstart_addr);
					if (header->sw_816 == 1) printf("DPR : $%04X PBR : $00 DBR : $00\r\n",header->direct_page);
					printf("Shared memory : $%06lX\r\n",(unsigned long)header->picif_p);
					if ( header->bios_sw == 2 ) {
						if( header->irq_sw ) printf("IRQ : Support, ");
						else printf("IRQ : No support, ");
						if( header->nmi_sw ) printf("MNI : Support\r\n");
						else printf("MNI : No support\r\n");
					}
					break;
				case 1:		// user program
					printf("User Program.\n\r");
					printf("Operational Mode : ");
					switch (header->sw_816) {
						case 0:
							printf("W65C02 (Emulation Mode)\n\r");
							break;
						case 1:
							printf("W65C816 Native mode\n\r");
							break;
						case 2:
							printf("Both W65C02 and W65C816\n\r");
					}
					printf("CSTART : $%04X\r\n",header->cstart_addr);
					printf("WSTART : $%04X\r\n",header->wstart_addr);
					if (header->sw_816) printf("DPR : $%04X PBR : $%02X DBR : $%02X\r\n",header->direct_page, header->op1, header->op2);
			}
		}
	}
	printf("\r\n");
	f_close(&fl);
	return 0;
}

static FRESULT flash_apl(void) {
	if ( binv.sw ) {
		printf("Terminate %s\r\n",binv.fname);
		binv.sw = 0;
		// close disk images if DOS65 was suspended by NMI
		close_dos65();
	}
	else printf("No Program to terminate.\r\n");

	return FR_OK;
}

static int in_file(void) {

	uint8_t chr;

	if ( fin_cnt == 0xffff ) { // init : open file
		// open input file & set console input from file
		if ( f_open(&in_fl, &fin_name[0], FA_READ) ) {
			printf("File Open Error. %s\r\n", &fin_name[0]);
			cin_no = INPUT_UART;						// change console input to UART
			return 0;
		}

		fin_cnt=0;
		frd_ptr = 0;
		fin_size = (uint16_t)f_size(&in_fl);				// get file size
	}

next_char:

	if ( !fin_cnt ) {
		frd_ptr = 0;
		if ( f_read( &in_fl, &buffer[0], SECTOR_SIZE, (UINT *)&fin_cnt ) ) {
			printf("File read error.\r\n");
			f_close( &in_fl );
			cin_no = INPUT_UART;			// set console to UART
			return 0;
		}
	}

	chr = buffer[frd_ptr++];
	fin_cnt--;
	fin_size--;
	
	if ( !fin_size ) {
		f_close( &in_fl );
		cin_no = INPUT_UART;			// set console to UART
		if ( chr < CR ) chr = CR;
		return chr;
	}

	if ( chr < CR ) goto next_char;
	return chr;
}

static unsigned int str_inf(char *buf, uint8_t cnt) {
	unsigned int c, i;
	char a;
	
	if ( fin_cnt == 0xffff ) { // init : open file
		// open input file & set console input from file
		if ( f_open(&in_fl, &fin_name[0], FA_READ) ) {
			printf("File Open Error. %s\r\n", &fin_name[0]);
			cin_no = INPUT_UART;						// change console input to UART
			return 0;
		}

		fin_cnt=0;
		frd_ptr = 0;
		fin_size = (uint16_t)f_size(&in_fl);				// get file size
	}

	if ( !fin_cnt ) {
		frd_ptr = 0;
		if ( f_read( &in_fl, &buffer[0], SECTOR_SIZE, (UINT *)&fin_cnt ) ) {
			printf("File read error.\r\n");
			f_close( &in_fl );
			cin_no = INPUT_UART;			// set console to UART
			return 0;
		}
	}

	i = ( (unsigned int)cnt > fin_cnt ) ? fin_cnt : (unsigned int)cnt;

	fin_cnt -= i;
	fin_size -= i;
	c = i;
	while(i--) {
		a = buffer[frd_ptr++];
		if ( a < CR ) {
			c--;
			continue;
		}
		else *buf++ = a;
	}
	
	if ( !fin_size ) {
		f_close( &in_fl );
		cin_no = INPUT_UART;			// set console to UART
	}
	
	return c;
}

//
// check dsk
// 0  : No CPMDISKS directory
// 1  : CPMDISKS directory exist
//
static int chk_dsk(void)
{
    int selection;
    uint8_t c;
	DIR fsdir;
	
    //
    // Select disk image folder
    //
	selection = 0;
    if (f_opendir(&fsdir, "/")  != FR_OK) {
        printf("Failed to open SD Card.\n\r");
		return selection;
    }

	f_rewinddir(&fsdir);
	while (f_readdir(&fsdir, &fileinfo) == FR_OK && fileinfo.fname[0] != 0) {
		if (strcmp(fileinfo.fname, dosdir) == 0) {
			selection = 1;
			printf("Detect %s\n\r", fileinfo.fname);
			break;
		}
	}
	f_closedir(&fsdir);
	
	return(selection);
}

static FRESULT open_dos65(void) {

	FRESULT	rs;
	char drive_letter;

	uint16_t drv;

	if ( !chk_dsk() ) {
		printf("DOS_DISK directory not found.\r\n");
		return FR_OK;
	}
	if ( open_dskimg() < 0 ) {
		printf("Drive A not found.\n\r");
		return FR_OK;
	}

	// load DOS65.SYS
	if ( (FRESULT)load_program((uint8_t *)dos65) ) {
		printf("%s not found.\r\n", dos65);
		return FR_OK;
	}

	// check monitor
	if ( mon ) {
		mon = 0;
		printf("Reload %s for Emulation Mode...\r\n", mon02);
		if ( (FRESULT)load_program((uint8_t *)mon02) ) {
			printf("%s not found.\r\n", mon02);
			return FR_OK;
		}
	}

	binv.addr = binv.caddr;
	write_sram( fh.load_p+mezID_off, (uint8_t *)&binv, BINV_SIZE );		// address of monitor's mezID

	write_sram(0xFFFC, (uint8_t *)&fh.cstart_addr, 2);
	start_cpu();	//set monitor cold start
	rs = drive_cpu();
	if ( !rs ) {
		binv.sw = 0;	// terminate bios_call program
		// close disk images
		rs = close_dos65();
    }
	return rs;
}

//
// Open disk images
//
static int open_dskimg(void) {
	
	uint16_t drv;
	char drive_letter;
	char *buf;
	
	for (drv = 0; drv < NUM_DRIVES; drv++) {
        drive_letter = (char)('A' + drv);
        buf = &buffer[0];
        sprintf(buf, "%s/DOS65%c.DSK", fileinfo.fname, drive_letter);
        if (f_open(&files[drv], buf, FA_READ|FA_WRITE) == FR_OK) {
        	printf("Image file %s/DOS65%c.DSK is assigned to drive %c\n\r",
                   fileinfo.fname, drive_letter, drive_letter);
        	drives[drv].filep = &files[drv];
        	drives[drv].exist = 1;
        }
		else drives[drv].exist = 0;
    }
	if ( !drives[0].exist ) return -4;
	return 0;
}

// close DOS65 disk image
static FRESULT close_dos65(void) {
	FRESULT	rs;
	char drive_letter;

	uint16_t drv;

	rs = FR_OK;
	for (drv = 0; drv < NUM_DRIVES; drv++) {
        drive_letter = (char)('A' + drv);
		if ( drives[drv].exist ) {
			rs = f_close( drives[drv].filep );
       		printf("\r\nClose disk Image file /%s/DOS65%c.DSK", dosdir, drive_letter);
			drives[drv].exist = 0;
        }
	}
	printf("\r\n");
	
	return rs;
}
