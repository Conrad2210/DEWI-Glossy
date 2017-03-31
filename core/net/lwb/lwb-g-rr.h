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

#ifndef __LWB_G_RR_H__
#define __LWB_G_RR_H__

/// @file lwb-g-rr.h

#include "contiki.h"
#include "lwb-common.h"

PT_THREAD(lwb_g_rr_host(struct rtimer *t, lwb_context_t *p_context));

PT_THREAD(lwb_g_rr_source(struct rtimer *t, lwb_context_t *p_context));

/// @brief Initialize g_rr
void lwb_g_rr_init();

/// @brief This function is used to queue application data to be sent over LWB
/// @param p_data A pointer to the data
/// @param ui8_len The length of the data
/// @param ui16_to_node_id The ID of the destination node
/// @return Non-zero if queuing is successful.
uint8_t lwb_g_rr_queue_packet(uint8_t* p_data, uint8_t ui8_len, uint16_t ui16_to_node_id);

/// @brief This function to be called to deliver data to application layer.
void lwb_g_rr_data_output();

/// @brief Request from LWB host to add a stream for the node
uint8_t lwb_g_rr_stream_add(uint16_t ui16_ipi, uint16_t ui16_time_offset);

/// @brief Request from LWB host to delete a stream for the node
void lwb_g_rr_stream_del(uint8_t ui8_id);

/// @brief Request from LWB host to modify a stream request.
void lwb_g_rr_stream_mod(uint8_t ui8_id, uint16_t ui16_ipi);

#endif // __LWB_G_RR_H__
