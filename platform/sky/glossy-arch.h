/*
 * Copyright (c) 2011, ETH Zurich.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Federico Ferrari <ferrari@tik.ee.ethz.ch>
 *
 */

/**
 * \file
 *         Glossy architecture specific, header file.
 * \author
 *         Federico Ferrari <ferrari@tik.ee.ethz.ch>
 */

#ifndef GLOSSY_ARCH_H_
#define GLOSSY_ARCH_H_

#include "contiki.h"
#include "dev/watchdog.h"
#include "dev/cc2420_const.h"
#include "dev/leds.h"
#include "dev/spi.h"
#include <stdio.h>
#include <legacymsp430.h>
#include <stdlib.h>

/**
 * If not zero, nodes print additional debug information (disabled by default).
 */
#define GLOSSY_DEBUG 0
/**
 * Size of the window used to average estimations of slot lengths.
 */
#define GLOSSY_SYNC_WINDOW            64
/**
 * Initiator timeout, in number of Glossy slots.
 * When the timeout expires, if the initiator has not received any packet
 * after its first transmission it transmits again.
 */
#define GLOSSY_INITIATOR_TIMEOUT      3

/**
 * Ratio between the frequencies of the DCO and the low-frequency clocks
 */
#if COOJA
#define CLOCK_PHI                     (4194304uL / RTIMER_SECOND)
#else
#define CLOCK_PHI                     (F_CPU / RTIMER_SECOND)
#endif /* COOJA */

#define GLOSSY_HEADER                 0xa0
#define GLOSSY_HEADER_MASK            0xf0
#define GLOSSY_HEADER_LEN             sizeof(uint8_t)
#define GLOSSY_RELAY_CNT_LEN          sizeof(uint8_t)
#define GLOSSY_IS_ON()                (get_state() != GLOSSY_STATE_OFF)
#define FOOTER_LEN                    2
#define FOOTER1_CRC_OK                0x80
#define FOOTER1_CORRELATION           0x7f

#define GLOSSY_LEN_FIELD              packet[0]
#define GLOSSY_HEADER_FIELD           packet[1]
#define GLOSSY_DATA_FIELD             packet[2]
#define GLOSSY_RELAY_CNT_FIELD        packet[packet_len_tmp - FOOTER_LEN]
#define GLOSSY_RSSI_FIELD             packet[packet_len_tmp - 1]
#define GLOSSY_CRC_FIELD              packet[packet_len_tmp]

enum {
	GLOSSY_INITIATOR = 1, GLOSSY_RECEIVER = 0
};

enum {
	GLOSSY_SYNC = 1, GLOSSY_NO_SYNC = 0
};
/**
 * List of possible Glossy states.
 */
enum glossy_state {
	GLOSSY_STATE_OFF,          /**< Glossy is not executing */
	GLOSSY_STATE_WAITING,      /**< Glossy is waiting for a packet being flooded */
	GLOSSY_STATE_RECEIVING,    /**< Glossy is receiving a packet */
	GLOSSY_STATE_RECEIVED,     /**< Glossy has just finished receiving a packet */
	GLOSSY_STATE_TRANSMITTING, /**< Glossy is transmitting a packet */
	GLOSSY_STATE_TRANSMITTED,  /**< Glossy has just finished transmitting a packet */
	GLOSSY_STATE_ABORTED       /**< Glossy has just aborted a packet reception */
};
#if GLOSSY_DEBUG
unsigned int high_T_irq, rx_timeout, bad_length, bad_header, bad_crc;
#endif /* GLOSSY_DEBUG */

/* ------------------------------ Timeouts -------------------------- */
/**
 * \defgroup glossy_timeouts Timeouts
 * @{
 */

inline void glossy_schedule_rx_timeout(void);
inline void glossy_stop_rx_timeout(void);
inline void glossy_schedule_initiator_timeout(void);
inline void glossy_stop_initiator_timeout(void);

/** @} */

/* ----------------------- Interrupt functions ---------------------- */
/**
 * \defgroup glossy_interrupts Interrupt functions
 * @{
 */

inline void glossy_begin_rx(void);
inline void glossy_end_rx(void);
inline void glossy_begin_tx(void);
inline void glossy_end_tx(void);

/** @} */

/**
 * \defgroup glossy_capture Timer capture of clock ticks
 * @{
 */

/* -------------------------- Clock Capture ------------------------- */
/**
 * \brief Capture next low-frequency clock tick and DCO clock value at that instant.
 * \param t_cap_h variable for storing value of DCO clock value
 * \param t_cap_l variable for storing value of low-frequency clock value
 */
#define CAPTURE_NEXT_CLOCK_TICK(t_cap_h, t_cap_l) do {\
		/* Enable capture mode for timers B6 and A2 (ACLK) */\
		TBCCTL6 = CCIS0 | CM_POS | CAP | SCS; \
		TACCTL2 = CCIS0 | CM_POS | CAP | SCS; \
		/* Wait until both timers capture the next clock tick */\
		while (!((TBCCTL6 & CCIFG) && (TACCTL2 & CCIFG))); \
		/* Store the capture timer values */\
		t_cap_h = TBCCR6; \
		t_cap_l = TACCR2; \
		/* Disable capture mode */\
		TBCCTL6 = 0; \
		TACCTL2 = 0; \
} while (0)

/** @} */

/* -------------------------------- SFD ----------------------------- */

/**
 * \defgroup glossy_sfd Management of SFD interrupts
 * @{
 */

/**
 * \brief Capture instants of SFD events on timer B1
 * \param edge Edge used for capture.
 *
 */
#define SFD_CAP_INIT(edge) do {\
	P4SEL |= BV(SFD);\
	TBCCTL1 = edge | CAP | SCS;\
} while (0)

/**
 * \brief Enable generation of interrupts due to SFD events
 */
#define ENABLE_SFD_INT()		do { TBCCTL1 |= CCIE; } while (0)

/**
 * \brief Disable generation of interrupts due to SFD events
 */
#define DISABLE_SFD_INT()		do { TBCCTL1 &= ~CCIE; } while (0)

/**
 * \brief Clear interrupt flag due to SFD events
 */
#define CLEAR_SFD_INT()			do { TBCCTL1 &= ~CCIFG; } while (0)

/**
 * \brief Check if generation of interrupts due to SFD events is enabled
 */
#define IS_ENABLED_SFD_INT()    !!(TBCCTL1 & CCIE)

/** @} */

#define SET_PIN(a,b)          do { P##a##OUT |=  BV(b); } while (0)
#define UNSET_PIN(a,b)        do { P##a##OUT &= ~BV(b); } while (0)
#define TOGGLE_PIN(a,b)       do { P##a##OUT ^=  BV(b); } while (0)
#define INIT_PIN_IN(a,b)      do { P##a##SEL &= ~BV(b); P##a##DIR &= ~BV(b); } while (0)
#define INIT_PIN_OUT(a,b)     do { P##a##SEL &= ~BV(b); P##a##DIR |=  BV(b); } while (0)
#define PIN_IS_SET(a,b)       (    P##a##IN  &   BV(b))

// UserINT (P2.7)
#define SET_PIN_USERINT      SET_PIN(2,7)
#define UNSET_PIN_USERINT    UNSET_PIN(2,7)
#define TOGGLE_PIN_USERINT   TOGGLE_PIN(2,7)
#define INIT_PIN_USERINT_IN  INIT_PIN_IN(2,7)
#define INIT_PIN_USERINT_OUT INIT_PIN_OUT(2,7)
#define PIN_USERINT_IS_SET   PIN_IS_SET(2,7)

// GIO2 (P2.3)
#define SET_PIN_GIO2         SET_PIN(2,3)
#define UNSET_PIN_GIO2       UNSET_PIN(2,3)
#define TOGGLE_PIN_GIO2      TOGGLE_PIN(2,3)
#define INIT_PIN_GIO2_IN     INIT_PIN_IN(2,3)
#define INIT_PIN_GIO2_OUT    INIT_PIN_OUT(2,3)
#define PIN_GIO2_IS_SET      PIN_IS_SET(2,3)

// ADC0 (P6.0)
#define SET_PIN_ADC0         SET_PIN(6,0)
#define UNSET_PIN_ADC0       UNSET_PIN(6,0)
#define TOGGLE_PIN_ADC0      TOGGLE_PIN(6,0)
#define INIT_PIN_ADC0_IN     INIT_PIN_IN(6,0)
#define INIT_PIN_ADC0_OUT    INIT_PIN_OUT(6,0)
#define PIN_ADC0_IS_SET      PIN_IS_SET(6,0)

// ADC1 (P6.1)
#define SET_PIN_ADC1         SET_PIN(6,1)
#define UNSET_PIN_ADC1       UNSET_PIN(6,1)
#define TOGGLE_PIN_ADC1      TOGGLE_PIN(6,1)
#define INIT_PIN_ADC1_IN     INIT_PIN_IN(6,1)
#define INIT_PIN_ADC1_OUT    INIT_PIN_OUT(6,1)
#define PIN_ADC1_IS_SET      PIN_IS_SET(6,1)

// ADC2 (P6.2) -> LED3
#define SET_PIN_ADC2         SET_PIN(6,2)
#define UNSET_PIN_ADC2       UNSET_PIN(6,2)
#define TOGGLE_PIN_ADC2      TOGGLE_PIN(6,2)
#define INIT_PIN_ADC2_IN     INIT_PIN_IN(6,2)
#define INIT_PIN_ADC2_OUT    INIT_PIN_OUT(6,2)
#define PIN_ADC2_IS_SET      PIN_IS_SET(6,2)

// ADC6 (P6.6) -> LED2
#define SET_PIN_ADC6         SET_PIN(6,6)
#define UNSET_PIN_ADC6       UNSET_PIN(6,6)
#define TOGGLE_PIN_ADC6      TOGGLE_PIN(6,6)
#define INIT_PIN_ADC6_IN     INIT_PIN_IN(6,6)
#define INIT_PIN_ADC6_OUT    INIT_PIN_OUT(6,6)
#define PIN_ADC6_IS_SET      PIN_IS_SET(6,6)

// ADC7 (P6.7) -> LED1
#define SET_PIN_ADC7         SET_PIN(6,7)
#define UNSET_PIN_ADC7       UNSET_PIN(6,7)
#define TOGGLE_PIN_ADC7      TOGGLE_PIN(6,7)
#define INIT_PIN_ADC7_IN     INIT_PIN_IN(6,7)
#define INIT_PIN_ADC7_OUT    INIT_PIN_OUT(6,7)
#define PIN_ADC7_IS_SET      PIN_IS_SET(6,7)

#define SET_PIN_LEDS_RED            UNSET_PIN(5, 4)
#define UNSET_PIN_LEDS_RED          SET_PIN(5, 4)
#define TOGGLE_PIN_LEDS_RED         TOGGLE_PIN(5, 4)
#define INIT_PIN_LEDS_RED_OUT       INIT_PIN_OUT(5, 4)
#define PIN_LEDS_RED_IS_SET         PIN_IS_SET(5,4)

#define SET_PIN_LEDS_GREEN            UNSET_PIN(5, 5)
#define UNSET_PIN_LEDS_GREEN          SET_PIN(5, 5)
#define TOGGLE_PIN_LEDS_GREEN         TOGGLE_PIN(5, 6)
#define INIT_PIN_LEDS_GREEN_OUT       INIT_PIN_OUT(5, 5)
#define PIN_LEDS_GREEN_IS_SET         PIN_IS_SET(5, 5)

#define SET_PIN_LEDS_BLUE            UNSET_PIN(5, 6)
#define UNSET_PIN_LEDS_BLUE          SET_PIN(5, 6)
#define TOGGLE_PIN_LEDS_BLUE         TOGGLE_PIN(5, 6)
#define INIT_PIN_LEDS_BLUE_OUT       INIT_PIN_OUT(5, 6)
#define PIN_LEDS_BLUE_IS_SET         PIN_IS_SET(5, 6)

#endif /* GLOSSY_ARCH_H_ */

/** @} */
