/**
 * \file
 *
 * \brief USART Synchronous example for SAM.
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/**
 *  \mainpage USART Synchronous Mode Example
 *
 *  \par Purpose
 *
 *  This example demonstrates the Synchronous  mode provided by the USART
 *  peripherals on SAM.
 *
 *  \par Requirements
 *
 * This example can be used on  SAMV71-Xplained-Ultra/SAME70-Xplained boards and requires 2 boards to
 * be connected directly through populated USART pins.
 * \copydoc usart_sync_example_pin_defs
 *
 *  \par Description
 *
 *  This application gives an example of how to use USART in synchronous mode.
 *  Synchronous operations provide a high speed transfer capability. The
 *  transfer under this mode needs a pair of master and slave, which is
 *  determined by which one offers the clock source.
 *
 *  The example initializes USART as master by default. To enable the
 *  communication between each side of the connection, the user should change
 *  the mode of another side to slave through user interface. If well configured,
 *  transfer could be started by typing 'r' and  'w' from terminal application.
 *  This example also leaves the interface to select the clock freq.
 *
 *  The meaning of each input character could be found in items of the main menu.
 *
 * \par Usage
 *
 *  -# Build the program and download it into the two evaluation boards.
 *  -# Connect a serial cable to the UART port for each evaluation kit.
 *  -# On the computer, open and configure a terminal application (e.g.,
 *     HyperTerminal on Microsoft Windows) with these settings:
 *        - 115200 bauds
 *        - 8 data bits
 *        - No parity
 *        - 1 stop bit
 *        - No flow control
 *  -# Start the application. The following traces shall appear on the terminal:
 *     \code
	     -- USART Synchronous Mode Example --
	     -- xxxxxx-xx
	     -- Compiled: xxx xx xxxx xx:xx:xx --
	     -- Menu Choices for this example --
	     -- [0-3]:Select clock freq of master --
	     -- i: Display configuration info
	     -- w: Write data block .--
	     -- r: Read data block.--
	     -- s: Switch between master and slave mode.--
	     -- m: Display this menu again.--
	     --USART in MASTER mode--

\endcode
 *  -# The main menu will guide the user to configure the device and conduct
 *     operations.
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include <string.h>
#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"
#include "conf_example.h"

/** size of the receive buffer used by the PDC, in bytes.*/
#define BUFFER_SIZE         2000

/** USART synchronous master. */
#define SYNC_MASTER         1
/** USART synchronous slave. */
#define SYNC_SLAVE          0

/** USART is reading. */
#define STATE_READ          0

/** USART is writing. */
#define STATE_WRITE         1

#define FREQ_OPTIONS_NUM    4

/** All interrupt mask. */
#define ALL_INTERRUPT_MASK  0xffffffff

#define STRING_EOL    "\r"
#define STRING_HEADER "--USART Synchronous Mode Example --\r\n" \
		"-- "BOARD_NAME" --\r\n" \
		"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

/** Transmit buffer. */
char tran_buff[BUFFER_SIZE] = "DESCRIPTION of this example: \r\n \
 **************************************************************************\r\n\
 *  This application gives an example of how to use USART in synchronous mode.\r\n\
 *  Synchronous operations provide a high speed transfer capability. The\r\n\
 *  transfer under this mode needs a pair of master and slave, which is\r\n\
 *  determined by which one offers the clock source.\r\n\
 *  \r\n\
 *  The example initialized USART as master by default. To enable the \r\n\
 *  communication between each side of the connection. The user should change\r\n\
 *  the mode of another side to slave through user interface. If well configured,\r\n\
 *  transfer could be started by typing 'r' and  'w' from terminal application.\r\n\
 *  This example also leaves the interface to select the clock freq.\r\n\
 *  \r\n\
 *  The meaning of each input character could be found in items of the main menu.\r\n\
 *  \r\n\
 **************************************************************************\r\n\
 END of DESCRIPTION \r\n\
 ";

/** Buffer for receiving. */
uint8_t g_c_recv_buff[BUFFER_SIZE] = { 0 };

/** Reception is done. */
volatile uint32_t g_ul_recv_done = false;
/** Sending is done. */
volatile uint32_t g_ul_sent_done = false;

/** Mode for usart, 0 means usart as slave and 1 for
 master. */
uint8_t g_uc_transfer_mode = SYNC_MASTER;

/** State of reading or writing. */
uint8_t g_uc_state = STATE_WRITE;

/** Clock freq. */
uint32_t g_ul_freq[FREQ_OPTIONS_NUM] =
		{ 1000000UL, 4000000UL, 10000000UL, 16000000UL };

/** Present frequency index in list g_ul_freq[]. */
uint8_t g_uc_freq_idx = 0;

/** Pointer to receive buffer base. */
uint8_t *p_revdata = &g_c_recv_buff[0];
/** count number for received data. */
uint32_t g_ulcount = 0;
/**
 * \brief USART IRQ handler.
 *
 * Interrupt handler for USART. After reception is done, set g_ul_recv_done to true,
 * and if transmission is done, set g_ul_sent_done to true.
 *
 */
void USART0_Handler(void)
{
	uint32_t ul_status;
	uint8_t uc_char;

	/* Read USART Status. */
	ul_status = usart_get_status(BOARD_USART);

	if(ul_status & (US_IER_TXRDY | US_IER_TXEMPTY)) {
		usart_disable_interrupt(BOARD_USART, (US_IER_TXRDY | US_IER_TXEMPTY));
	}

	/* Receive register is full. */
	if((g_uc_state == STATE_READ) && (usart_read(BOARD_USART, (uint32_t *)&uc_char) == 0)) {
		*p_revdata++ = uc_char;
		g_ulcount++;
		if(g_ulcount >= BUFFER_SIZE) {
			usart_disable_interrupt(BOARD_USART, US_IER_RXRDY);
			g_ul_recv_done = true;
		}
	}

}

/**
 *  \brief Configure UART for debug message output.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
#ifdef CONF_UART_CHAR_LENGTH
		.charlength = CONF_UART_CHAR_LENGTH,
#endif
		.paritytype = CONF_UART_PARITY,
#ifdef CONF_UART_STOP_BITS
		.stopbits = CONF_UART_STOP_BITS,
#endif
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

/**
 * \brief Configure USART in synchronous mode.
 *
 * \param ul_ismaster  1 for master, 0 for slave.
 * \param ul_baudrate  Baudrate for synchronous communication.
 *
 */
static void configure_usart(uint32_t ul_ismaster, uint32_t ul_baudrate)
{
	sam_usart_opt_t usart_console_settings = {
		0,
		US_MR_CHRL_8_BIT,
		US_MR_PAR_NO,
		US_MR_NBSTOP_1_BIT,
		US_MR_CHMODE_NORMAL,
		/* This field is only used in IrDA mode. */
		0
	};

	usart_console_settings.baudrate = ul_baudrate;

	/* Enable the peripheral clock in the PMC. */
	sysclk_enable_peripheral_clock(BOARD_ID_USART);


	/* Configure USART in SYNC. master or slave mode. */
	if (ul_ismaster) {
		usart_init_sync_master(BOARD_USART, &usart_console_settings, sysclk_get_peripheral_hz());
	} else {
		usart_init_sync_slave(BOARD_USART, &usart_console_settings);
	}

	/* Disable all the interrupts. */
	usart_disable_interrupt(BOARD_USART, ALL_INTERRUPT_MASK);

	/* Enable TX & RX function. */
	usart_enable_tx(BOARD_USART);
	usart_enable_rx(BOARD_USART);

	/* Configure and enable interrupt of USART. */
	NVIC_EnableIRQ(USART_IRQn);
}

/**
 * \brief Display main menu.
 */
static void display_main_menu(void)
{
	puts("-- Menu Choices for this example --\r\n"
			"-- [0-3]:Select clock freq of master --\r\n"
			"-- i: Display configuration info --\r\n"
			"-- w: Write data block .--\r\n"
			"-- r: Read data block.--\r\n"
			"-- s: Switch between master and slave mode.--\r\n"
			"-- m: Display this menu again.--\r");
}

/**
* \brief transmit data in synchronous mode.
*
* \param *p_buff  data to be transmitted
* \param ulsize size of all data.
*
*/
static uint8_t transmit_mode_sync(uint8_t *p_buff, uint32_t ulsize)
{
	Assert(p_buff);

	while(ulsize > 0) {
		if(0 == usart_write(BOARD_USART, *p_buff)){
			usart_enable_interrupt(BOARD_USART, US_IER_TXRDY | US_IER_TXEMPTY);
			ulsize--;
			p_buff++;
		}
	}

	while(!usart_is_tx_empty(BOARD_USART)) {
				;  /*waiting for transmit over*/
	}

	g_ul_sent_done = true;
	return 0;
}

/**
 * \brief Dump buffer to uart.
 *
 */
static void dump_info(char *p_buf, uint32_t ul_size)
{
	uint32_t ul_i = 0;

	while ((ul_i < ul_size) && (p_buf[ul_i] != 0)) {
		printf("%c", p_buf[ul_i++]);
	}
}

/**
 * \brief Application entry point.
 *
 * Configure USART in synchronous master/slave mode to start a transmission
 * between two boards.
 * \return Unused.
 */
int main(void)
{
	uint8_t uc_char;
	uint8_t *p_data;

	/* Initialize the SAM system. */
	sysclk_init();
	board_init();

	/* Configure UART for debug message output. */
	configure_console();

	/* Output example information. */
	puts(STRING_HEADER);

	/* Display main menu. */
	display_main_menu();

	/* Configure USART. */
	configure_usart(SYNC_MASTER, g_ul_freq[g_uc_freq_idx]);

	g_uc_transfer_mode = SYNC_MASTER;

	g_uc_state = STATE_WRITE;

	puts("-- USART in MASTER mode --\r");

	while (1) {
		uc_char = 0;
		scanf("%c", (char *)&uc_char);
		switch (uc_char) {
		case '0':
		case '1':
		case '2':
		case '3':
			g_uc_freq_idx = uc_char - '0';
			printf("-- The clock freq is: %luHz.\r\n",
				(unsigned long)g_ul_freq[g_uc_freq_idx]);
			configure_usart(SYNC_MASTER, g_ul_freq[g_uc_freq_idx]);
			break;
		case 'i':
		case 'I':
			if (g_uc_transfer_mode == SYNC_MASTER) {
				printf("-- USART is MASTER at %luHz.\r\n",
					(unsigned long)g_ul_freq[g_uc_freq_idx]);
			} else {
				puts("-- USART is SLAVE \r");
			}
			break;
		case 's':
		case 'S':
			if (g_uc_transfer_mode == SYNC_MASTER) {
				g_uc_transfer_mode = SYNC_SLAVE;
				configure_usart(SYNC_SLAVE, g_ul_freq[g_uc_freq_idx]);
				puts("-- USART in SLAVE mode --\r");
			} else {
				if (g_uc_transfer_mode == SYNC_SLAVE) {
					g_uc_transfer_mode = SYNC_MASTER;
					configure_usart(SYNC_MASTER, g_ul_freq[g_uc_freq_idx]);
					puts("-- USART in MASTER mode --\r");
				}
			}
			break;
		case 'w':
		case 'W':
			g_uc_state = STATE_WRITE;
			p_data = (uint8_t *)&tran_buff[0];
			transmit_mode_sync(p_data, BUFFER_SIZE);

			while (!g_ul_sent_done) {
			}
			if (g_ul_sent_done) {
				printf("-- %s sent done --\r\n",
						g_uc_transfer_mode ? "MASTER" :
						"SLAVE");
			}
			break;
		case 'r':
		case 'R':
			g_uc_state = STATE_READ;
			g_ulcount = 0;
			p_revdata = &g_c_recv_buff[0];
			if (g_uc_transfer_mode == SYNC_MASTER) {
				puts("----USART MASTER Read----\r");
			} else {
				puts("----USART SLAVE Read----\r");
			}

			usart_enable_interrupt(BOARD_USART, US_IER_RXRDY);

			while (!g_ul_recv_done) {
			}
			if (g_ul_recv_done) {
				if (strncmp((char*)g_c_recv_buff, tran_buff, BUFFER_SIZE)) {
					puts(" -F-: Failed!\r");
				} else {
					/* successfully received */
					dump_info((char*)g_c_recv_buff, BUFFER_SIZE);
				}
				puts("----END of read----\r");
				memset(g_c_recv_buff, 0, sizeof(g_c_recv_buff));
				g_ul_recv_done = false;
			}
			break;
		case 'm':
		case 'M':
			display_main_menu();
			break;
		default:
			break;
		}
	}
}
