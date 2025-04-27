/*
 * Copyright:
 * ----------------------------------------------------------------
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *   (C) COPYRIGHT 2014, 2016 ARM Limited
 *       ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 * ----------------------------------------------------------------
 * File:     main.c
 * Release Information : Cortex-M3 DesignStart-r0p1-00rel0
 * ----------------------------------------------------------------
 *
 */

/*
 * --------Included Headers--------
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "xtmrctr_l.h"
// Xilinx specific headers
#include "xparameters.h"
#include "xgpio.h"

#include "m3_for_arty.h"        // Project specific header
#include "gpio.h"
#include "uart.h"
#include "spi.h"
#include "atomic.h"
#include "xuartlite_l.h"
#include "xuartlite.h"
#include "xil_printf.h"
#include "sha.h"

#define SHA3_BASEADDR XPAR_IPSHA3_BLOCK_0_S00_AXI_BASEADDR

u32 reverse_bytes(u32 value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

void print_output() {
    u32 out, inverted_num, i;
    for (i = 72; i < 134; i = i + 4) {
        out = Xil_In32(SHA3_BASEADDR + i);
        inverted_num = reverse_bytes(out);
        xil_printf("%x ", inverted_num);
    }
    xil_printf("\r\n");
}

int check_done() {
    u32 out;
    out = Xil_In32(SHA3_BASEADDR + 140);
    if (out == 0) {
        return 0;
    }
    Xil_Out32(SHA3_BASEADDR + 140, 0);
    return 1;
}

void reset() {
	uint8_t i;
    Xil_Out32(SHA3_BASEADDR + 136, 0x2);
    for (i = 0; i < 5; i++) {
        Xil_Out32(SHA3_BASEADDR + 140, 0);
    }
}

void load() {
    Xil_Out32(SHA3_BASEADDR + 136, 0x0); // 00
}

void start() {
    Xil_Out32(SHA3_BASEADDR + 136, 0x1); // 01
}

void process_sha3(uint32_t *sha3_data, unsigned char *output) {
	  
	  u32 out, inverted_num;
	  int array_index = 0;
	  int i;
    reset();
    load();
    for (i = 0; i < 18; i++) {
        Xil_Out32(SHA3_BASEADDR + (i * 4), sha3_data[i]);
    }
    start();
    while (1) {
        if (check_done() == 1) {
            break;
        }
    }

    for (i = 72; i < 134; i += 4)
		{
        out = Xil_In32(SHA3_BASEADDR + i);
        inverted_num = reverse_bytes(out);
			  output[array_index + 0] = (unsigned char)((inverted_num >> 24) & 0xFF); // MSB
        output[array_index + 1] = (unsigned char)((inverted_num >> 16) & 0xFF);
        output[array_index + 2] = (unsigned char)((inverted_num >> 8) & 0xFF);
        output[array_index + 3] = (unsigned char)(inverted_num & 0xFF);
        array_index += 4;
    }
		//xil_printf("\r\n");
		//for (i = 0; i < 64; i++)
		//{
	//		xil_printf("%x",output[i]);
	//	}
		//xil_printf("\n\r");
}


// Atomic test function
uint32_t atomic_test(uint32_t *mem, uint32_t val);

/*******************************************************************/

#define TIMER_BASEADDR XPAR_TMRCTR_0_BASEADDR
#define CLOCK_FREQ_HZ 100000000
#define NS_PER_CYCLE (1000000000.0 / CLOCK_FREQ_HZ)

void start_timer(void) {
    XTmrCtr_SetControlStatusReg(TIMER_BASEADDR, 0, 0);
    XTmrCtr_SetLoadReg(TIMER_BASEADDR, 0, 0);
    XTmrCtr_SetControlStatusReg(TIMER_BASEADDR, 0, XTC_CSR_ENABLE_TMR_MASK);
}

u32 read_axi_counter(void) {
    return XTmrCtr_GetTimerCounterReg(TIMER_BASEADDR, 0);
}

void stop_timer(void) {
    XTmrCtr_SetControlStatusReg(TIMER_BASEADDR, 0, 0);
}

unsigned int get_size()
{
	  uint8_t highbyte, lowbyte;
	  uint16_t value=0;
	  highbyte = XUartLite_RecvByte(STDIN_BASEADDRESS);
		XUartLite_SendByte(STDOUT_BASEADDRESS,highbyte);
		lowbyte = XUartLite_RecvByte(STDIN_BASEADDRESS);
		XUartLite_SendByte(STDOUT_BASEADDRESS,lowbyte);
		value  = (highbyte << 8) | lowbyte;
		return value;
}

int main (void)
{
    int     status;
    int     DAPLinkFittedn;
    int     index;
    int     readbackError;
    char    debugStr[256];
    
    // Illegal location
    volatile u32 emptyLoc;
    volatile u32 QSPIbase;
        
    // DTCM test location
    uint32_t dtcmTest;
    
    // BRAM base
    // Specify as volatile to ensure processor reads values back from BRAM
    // and not local storage
    volatile u32 *pBRAMmemory = (u32 *)XPAR_BRAM_0_BASEADDR;

    // CPU ID register
    volatile u32 *pCPUId = (u32 *)0xE000ED00;
    volatile u32 CPUId;
    volatile u32 alias_test;
		char         data_rx;
		int 				 value;
		int start_time, end_time, latency_cycles;
		int i;
	  uint16_t size_buffer;
		uint8_t rx_buffer[72];
		unsigned char output[64];
		uint32_t sha3_data[18];
    // Test data for SPI
    u8 spi_tx_data[8] = {0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
    u8 spi_rx_data[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    
    // Test data for BRAM
    u32 bram_data[8] = {0x01234567, 0x89abcdef, 0xdeadbeef, 0xfeebdaed, 0xa5f03ca5, 0x87654321, 0xfedc0ba9, 0x01020408};

		
		memset(rx_buffer, 0x00, 72);
		while(1)
		{
			data_rx = XUartLite_RecvByte(STDIN_BASEADDRESS);
			if (data_rx == 'a') //size
		  {
			  size_buffer = get_size();
			}
			if (data_rx == 'b') //data
			{
				for (index = 0; index < size_buffer/8; index++)
				{
					rx_buffer[index] = XUartLite_RecvByte(STDIN_BASEADDRESS);
				}
			}
   
			if (data_rx == 'c') //print data structure
			{
				for (index = 0; index < 72; index++)
				{
					XUartLite_SendByte(STDOUT_BASEADDRESS, rx_buffer[index]);
				}
			}
			if (data_rx == 'd') //execute software SHA-3 
			{
				memset(output, 0x00, 64);
				start_time = 0;
				end_time = 0;
				latency_cycles = 0;
				start_timer();
				start_time = read_axi_counter();
				sha3_function32(rx_buffer, size_buffer, 0x06, 576, 1024, 512, output);
				end_time = read_axi_counter();
				latency_cycles = end_time - start_time;
				stop_timer();
			}
			if (data_rx == 'e')  //execute hardware SHA-3
			{
				memset(output, 0x00, 64);
				memset(sha3_data, 0x00, 18);
				//size_buffer = 568;
				start_time = 0;
				end_time = 0;
				latency_cycles = 0;
				start_timer();
				start_time = read_axi_counter();
				xil_printf("\n\r");
				padding(rx_buffer, size_buffer, sha3_data);
				process_sha3(sha3_data, output);
				end_time = read_axi_counter();
				latency_cycles = end_time - start_time;
				stop_timer();
			}
			if (data_rx == 'f')
			{
				for (index=0; index < 64; index++)
				{
					xil_printf("%x ", output[index]);
				}
				xil_printf("\n\r");
			}
			if (data_rx == 'g')
			{
				xil_printf("%d, %d, %d\n\r", start_time, end_time, latency_cycles);
			}
			if (data_rx == 'h')
			{
				memset(rx_buffer, 0x00, sizeof(rx_buffer));
				size_buffer = 0;
				latency_cycles = 0;
				start_time = 0;
				end_time = 0;
			}
			if (data_rx == 'q')
				break;
		}
		
    return 0;
}

