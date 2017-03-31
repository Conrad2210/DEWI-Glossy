/*
 * Copyright (c) 2014, Uppsala University, Sweden.
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
 * Author: Kasun Hewage <kasun.hewage@it.uu.se>
 *
 */

#ifndef __LWB_HSLP_H__
#define __LWB_HSLP_H__

/// @file lwb-hslp.h
/// @brief Header file for LWB Host Serial Line Protocol (LHSLP)

#include <stdint.h>

#include "contiki.h"

#ifdef LWB_CONF_HSLP
#define LWB_HSLP    LWB_CONF_HSLP
#else
#define LWB_HSLP    0
#endif

///< HSLIP character types
#define HSLIP_END       0xC0
#define HSLIP_ESC       0xDB
#define HSLIP_ESC_END   0xDC
#define HSLIP_ESC_ESC   0xDD

/// @brief LHSLP main packet types
typedef enum {
    LHSLP_PKT_TYPE_LWB_DATA = 1,        ///< Indicates LWB specific data
    LHSLP_PKT_TYPE_LWB_DATA_REQ,        ///< Request LWB specific data from external device

    LHSLP_PKT_TYPE_DBG_DATA,            ///< Debugging data such as output from printf
    LHSLP_PKT_TYPE_DBG_DATA_REQ         ///< Request debugging data from external device
} lhslip_main_pkt_types_t;

/// @brief LHSLP packet sub types
typedef enum {
    LHSLP_PKT_SUB_TYPE_APP_DATA = 1,       ///< LWB application data
    LHSLP_PKT_SUB_TYPE_STREAM_REQ,         ///< LWB stream requests
    LHSLP_PKT_SUB_TYPE_STREAM_ACK,         ///< LWB stream acknowledgments
    LHSLP_PKT_SUB_TYPE_SCHED,              ///< LWB schedule

    LHSLP_PKT_SUB_TYPE_DBG_DATA            ///< Debugging data
} lhslip_sub_pkt_types_t;

///< Get main packet type
#define LWB_HSLP_GET_PKT_MAIN_TYPE(pkt_type)   (0xFF & (pkt_type >> 4))
///< Get sub packet type
#define LWB_HSLP_GET_PKT_SUB_TYPE(pkt_type)    (0x0F & pkt_type)
///< Set main packet type
#define LWB_HSLP_SET_PKT_MAIN_TYPE(pkt_type, var)   (var = (pkt_type << 4) | (var & 0x0F))
///< Set sub packet type
#define LWB_HSLP_SET_PKT_SUB_TYPE(pkt_type, var)    (var = ((var >> 4) << 4) | (pkt_type & 0x0F))


///< Write a byte to indicate the start of a HSLIP packet
#define lwb_hslp_write_start()	lwb_hslp_arch_write_byte(HSLIP_END)
///< Write a byte to indicate the end of a HSLIP packet
#define lwb_hslp_write_end()    lwb_hslp_arch_write_byte(HSLIP_END)

/// @brief Initialize LWB Host Serial Line Protocol
void lwb_hslp_init();
/// @brief Write a byte using LWB Host Serial Line Protocol
/// @param c The byte to be written
void lwb_hslp_write_byte(uint8_t c);
/// @brief Write a packet to serial line
/// @param ui8_pkt_type The packet type. @see LWB_HSLP_SET_PKT_MAIN_TYPE and @see LWB_HSLP_SET_PKT_SUB_TYPE
/// @param p_data A pointer to the data to be sent
/// @param ui8_len The length of the packet
void lwb_hslp_send(uint8_t ui8_pkt_type, uint8_t* p_data, uint8_t ui8_len);
/// @brief Read a packet from serial line
/// @details This function is blocked until a packet is read or timeout occurs
/// @param t_stop The time when the function should return
/// @return The length of the packet read. Zero otherwise i.e. in case of timeout
uint8_t lwb_hslp_read(rtimer_clock_t t_stop);
/// @brief Get packet type
/// @details @see LWB_HSLP_GET_PKT_MAIN_TYPE and LWB_HSLP_GET_PKT_SUB_TYPE
/// @return packet type
uint8_t lwb_hslp_get_packet_type();
/// @brief Get the length of data
/// @return The length of data
uint8_t lwb_hslp_get_data_len();
/// @brief Get a pointer to the data
/// @return A pointer to the data
uint8_t* lwb_hslp_get_data();


/// @brief Architecture specific implementation for writing a byte to serial line.
/// @param c THe character to be written
void lwb_hslp_arch_write_byte(uint8_t c);
/// @brief Architecture specific implementation for reading a byte from serial line.
/// @param c A pointer to a variable to store that character to be read
/// @return non-zero if a byte is read from the serial line
uint8_t lwb_hslp_arch_read_byte(uint8_t* c);

#endif // __LWB_HSLP_H__
