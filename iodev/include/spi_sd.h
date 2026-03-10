/*
Date: 2026.01

This software uses a program created by elehobica.
The copyright of the original program belongs to elehobica (https://elehobica.blogspot.com/).
Akihito Honda is responsible for any modifications.

<< original program >>
https://elehobica.blogspot.com/2021/03/raspberry-pi-picosdxcexfat-spi-if.html
https://github.com/elehobica/pico_fatfs
*/

#ifndef SPI_SD_DEF
#define SPI_SD_DEF 1

#pragma once

#include "hardware/clocks.h"
#include "hardware/spi.h"

#define CLK_SLOW     (100 * KHZ)
//#define CLK_FAST     (50 * MHZ)
//#define CLK_FAST     (4 * MHZ)
//#define CLK_FAST     (8 * MHZ)
#define CLK_FAST     (10 * MHZ)
// CLK_FAST: actually set to clk_peri (= 125.0 MHz) / N,
// which is determined by spi_set_baudrate() in pico-sdk/src/rp2_common/hardware_spi/spi.c

/*
 * SPI pin default assignment
 *   Refer to _pin_miso_conf, _pin_sck_conf, _pin_mosi_conf in spi_sd.c
 *     for permitted pin assignment for SPI0 and SPI1 usecase
 *   If the pin assignment constraints for SPI0/SPI1 are not satisfied,
 *     SPI PIO will be configured instead of SPI function

#define PIN_SPI0_MISO_DEFAULT   4
#define PIN_SPI0_CS_DEFAULT     5
#define PIN_SPI0_SCK_DEFAULT    2
#define PIN_SPI0_MOSI_DEFAULT   3

#define PIN_SPI1_MISO_DEFAULT   8
#define PIN_SPI1_CS_DEFAULT     9
#define PIN_SPI1_SCK_DEFAULT    10
#define PIN_SPI1_MOSI_DEFAULT   11
 */
#define PIN_SPI0_MISO   16
#define PIN_SPI0_CS     17
#define PIN_SPI0_SCK    18
#define PIN_SPI0_MOSI   19

typedef struct _pico_fatfs_spi_config_t {
    spi_inst_t* spi_inst;  // spi0 or spi1
    uint        clk_slow;
    uint        clk_fast;
    uint        pin_miso;
    uint        pin_cs;
    uint        pin_sck;
    uint        pin_mosi;
    bool        pullup;     // miso, mosi pins only
} pico_fatfs_spi_config_t;

#endif //SPI_SD_DEF