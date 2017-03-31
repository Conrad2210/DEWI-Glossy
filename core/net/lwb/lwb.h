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

#ifndef __LWB_H__
#define __LWB_H__

/// @file lwb.h
/// @brief Application header file for LWB

#include "lwb-common.h"

/// @brief Initialize LWB.
/// @param ui8_mode The mode of LWB. @see lwb_mode_t
/// @param p_callbacks A pointer to callback functions
/// @return Non-zero if initialization is successful.
uint8_t lwb_init(uint8_t ui8_mode, lwb_callbacks_t *p_callbacks);

/// @brief Queue packet to be sent over LWB.
/// @param p_data A pointer to the data buffer.
/// @param ui8_len The length of data.
/// @param ui16_to_node_id The ID of the destination node
/// @return Non-zero if queuing is successful.
uint8_t lwb_queue_packet(uint8_t* p_data, uint8_t ui8_len, uint16_t ui16_to_node_id);

/// @brief Get the time of LWB in seconds from when host is started.
uint32_t lwb_get_host_time();

int32_t lwb_get_skew();

/// @brief Request from LWB host to add a stream for the node.
/// @param ipi Inter-packet interval in seconds.
/// @param ui16_time_offset The time offset when the slot should be allocated.
/// @return The ID of the stream
uint8_t lwb_request_stream_add(uint16_t ui16_ipi, uint16_t ui16_time_offset);

/// @brief Request from LWB host to delete a stream for the node
/// @param ui8_id The stream ID
void lwb_request_stream_del(uint8_t ui8_id);

/// @brief Request from LWB host to modify a stream request.
/// @param ui8_id The ID of the stream to be modified.
/// @param ipi New inter-packet interval
void lwb_request_stream_mod(uint8_t ui8_id, uint16_t ui16_ipi);

/// @brief Return number of slots belong to the node
/// @return Number of slots
uint8_t lwb_get_n_my_slots();

/// @brief Get LWB joining state
/// @return one of the states from @link lwb_joining_state_t
uint8_t lwb_get_joining_state();

#endif // __LWB_H__
