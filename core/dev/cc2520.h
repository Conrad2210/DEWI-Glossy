/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         CC2520 driver header file
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef __CC2520_H__
#define __CC2520_H__

#include "contiki.h"
#include "dev/cc2520_const.h"

#include "contiki-conf.h"

int cc2520_init(void);

#define CC2520_MAX_PACKET_LEN      127

int cc2520_set_channel(uint8_t channel);
int cc2520_get_channel(void);

extern signed char cc2520_last_rssi;
extern uint8_t cc2520_last_correlation;



/**
 * \param power Between 1 and 31.
 */
void cc2520_set_txpower(uint8_t power);
int cc2520_get_txpower(void);
#define CC2520_TXPOWER_MAX  31
#define CC2520_TXPOWER_MIN   0

#define HIGH_PRIORITY 1
#define LOW_PRIORITY 0

/**
 * Interrupt function, called from the simple-cc2520-arch driver.
 *
 */
int cc2520_interrupt(void);

/* XXX hack: these will be made as Chameleon packet attributes */
extern rtimer_clock_t cc2520_time_of_arrival,
  cc2520_time_of_departure;
extern int cc2520_authority_level_of_sender;

int cc2520_on(void);
int cc2520_off(void);

void cc2520_set_cca_threshold(int value);

/************************************************************************/
/* Additional SPI Macros for the CC2520 */
/************************************************************************/
/* Send a strobe to the CC2520 */
#define CC2520_STROBE(s)                                \
  do {                                                  \
    CC2520_SPI_ENABLE();                                \
    SPI_WRITE_FAST(s);                                       \
    CC2520_SPI_DISABLE();                               \
  } while (0)


#define CC2520_INS_MEMCP_COMMON(instr, pri, count, src, dest)   \
  do{    \
    CC2520_SPI_ENABLE();                                                        \
    SPI_WRITE_FAST(instr | pri);                                                \
    SPI_WRITE_FAST(count);                                                      \
    SPI_WRITE_FAST(((uint8_t)(src >> 8) << 4) | (uint8_t)(dest >> 8));          \
    SPI_WRITE_FAST((uint8_t)(src & 0xFF));                                      \
    SPI_WRITE_FAST((uint8_t)(dest & 0xFF));                                     \
    CC2520_SPI_DISABLE();                                                       \
  } while(0)

#define CC2520_MEMCP(pri, count, src, dest)     CC2520_INS_MEMCP_COMMON(CC2520_INS_MEMCP, pri, count, src, dest)

/* copies from RX FIFO buffer to TX FIFO buffer "count" no of bytes */
//#define CC2520_MEMCP_RX_TX_FIFO(count)          CC2520_MEMCP(HIGH_PRIORITY, count, CC2520RAM_RXFIFO, CC2520RAM_TXFIFO)

#define CC2520_TXBUFCP(pri, addr, count)                \
do{   \
    CC2520_SPI_ENABLE();                               \
    SPI_WRITE_FAST(CC2520_INS_TXBUFCP | pri);          \
    SPI_WRITE_FAST(count);                             \
    SPI_WRITE_FAST((uint8_t)(addr >> 8));              \
    SPI_WRITE_FAST((uint8_t)(addr & 0xFF));            \
    CC2520_SPI_DISABLE();                              \
} while(0)

#define CC2520_MEMCP_RX_TX_FIFO(count)          CC2520_TXBUFCP(HIGH_PRIORITY, CC2520RAM_RXFIFO, count)

/* increments with 2^c byte word in CC2520 ram address */
#define CC2520_INC(c, addr)        \
  do{     \
    CC2520_SPI_ENABLE();                                \
    SPI_WRITE_FAST(CC2520_INS_INC | HIGH_PRIORITY);     \
    SPI_WRITE_FAST((c << 4) | (uint8_t)(addr >> 8));    \
    SPI_WRITE_FAST(((uint8_t)addr & 0xFF));             \
    CC2520_SPI_DISABLE();                               \
  } while(0)

#define CC2520_INC_RELAY_CNT(addr)              CC2520_INC(0, (CC2520RAM_TXFIFO + addr))

/* Write to a register in the CC2520                         */
/* Note: the SPI_WRITE(0) seems to be needed for getting the */
/* write reg working on the Z1 / MSP430X platform            */
#define CC2520_WRITE_REG(adr,data)                                      \
  do {                                                                  \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE_FAST(CC2520_INS_MEMWR | ((adr>>8)&0xFF));                 \
    SPI_WRITE_FAST(adr & 0xff);                                         \
    SPI_WRITE_FAST((uint8_t) data);                                     \
    SPI_WAITFORTx_ENDED();                                              \
    CC2520_SPI_DISABLE();                                               \
  } while(0)


/* Read a register in the CC2520 */
#define CC2520_READ_REG(adr,data)                                       \
  do {                                                                  \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE((CC2520_INS_MEMRD | ((adr>>8)&0xFF)));                    \
    SPI_WRITE_READ((adr & 0xFF), data);                                            \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

#define CC2520_READ_FIFO_BYTE(data)                                     \
  do {                                                                  \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE_READ(CC2520_INS_RXBUF, data);                                            \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

#define CC2520_READ_FIFO_BYTE_FAST(data)                                     \
  do {                                                                  \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE_READ_FAST(CC2520_INS_RXBUF, data);                        \
    CC2520_SPI_DISABLE();                                               \
  } while(0)


#define CC2520_READ_FIFO_BUF(buffer,count)                              \
  do {                                                                  \
    uint8_t i;                                                          \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE(CC2520_INS_RXBUF);                                        \
    for(i = 0; i < (count); i++) {                                      \
      SPI_READ(((uint8_t *)(buffer))[i]);                               \
    }                                                                   \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

#define CC2520_READ_FIFO_BUF_FAST(buffer,count)                              \
  do {                                                                  \
    uint8_t i;                                                          \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE_FAST(CC2520_INS_RXBUF);      \
    for(i = 0; i < (count); i++) {                                      \
      SPI_READ_FAST(((uint8_t *)(buffer))[i]);                               \
    }                                                                   \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

#define CC2520_WRITE_FIFO_BUF(buffer,count)                             \
  do {                                                                  \
    uint8_t i;                                                          \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE(CC2520_INS_TXBUF);                                   \
    for(i = 0; i < (count); i++) {                                      \
      SPI_WRITE(((uint8_t *)(buffer))[i]);                         \
    }                                                                   \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

#define CC2520_WRITE_FIFO_BUF_FAST(buffer,count)                             \
  do {                                                                  \
    uint8_t i;                                                          \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE_FAST(CC2520_INS_TXBUF);                                   \
    for(i = 0; i < (count); i++) {                                      \
      SPI_WRITE_FAST(((uint8_t *)(buffer))[i]);                         \
    }                                                                   \
    SPI_WAITFORTx_ENDED();                                                \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

/* Write to RAM in the CC2520 */
#define CC2520_WRITE_RAM(buffer,adr,count)                              \
  do {                                                                  \
    uint8_t i;                                                          \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE_FAST(CC2520_INS_MEMWR | (((adr)>>8) & 0xFF));             \
    SPI_WRITE_FAST(((adr) & 0xFF));                                     \
    for(i = 0; i < (count); i++) {                                      \
      SPI_WRITE_FAST(((uint8_t*)(buffer))[i]);                          \
    }                                                                   \
    SPI_WAITFORTx_ENDED();                                              \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

/* Read from RAM in the CC2520 */
#define CC2520_READ_RAM(buffer,adr,count)                               \
  do {                                                                  \
    uint8_t i;                                                          \
    CC2520_SPI_ENABLE();                                                \
    SPI_WRITE(CC2520_INS_MEMRD | (((adr)>>8) & 0xFF));                  \
    SPI_WRITE(((adr) & 0xFF));                                          \
    for(i = 0; i < (count); i++) {                                      \
      SPI_READ(((uint8_t*)(buffer))[i]);                                \
    }                                                                   \
    CC2520_SPI_DISABLE();                                               \
  } while(0)

/* Read status of the CC2520 */
#define CC2520_GET_STATUS(s)                                            \
  do {                                                                  \
    CC2520_SPI_ENABLE();                                                \
    SPI_READ_STATUS(s);                                                  \
    CC2520_SPI_DISABLE();                                               \
  } while (0)

#endif /* __CC2520_H__ */
