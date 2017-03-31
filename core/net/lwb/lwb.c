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

#include <string.h>
#include "contiki.h"
#include "glossy.h"
#include "lwb-common.h"
#include "lwb-macros.h"
#include "lwb.h"
#include "lwb-g-sync.h"
#include "lwb-g-rr.h"
#include "lwb-scheduler.h"

#ifndef INIT_PERIOD
#define INIT_PERIOD 1
#endif

PROCESS(lwb_main_process, "lwb main");

lwb_context_t lwb_context;

//--------------------------------------------------------------------------------------------------
uint8_t lwb_init(uint8_t ui8_mode, lwb_callbacks_t *p_callbacks) {

    memset(&lwb_context, 0, sizeof(lwb_context_t));

    /// TODO Initialize lwb_context
    lwb_context.lwb_mode = ui8_mode;
    lwb_context.p_callbacks = p_callbacks;

    // compute initial schedule.
    if (ui8_mode == LWB_MODE_HOST) {
        lwb_sched_init();
        lwb_sched_compute_schedule(&lwb_context.current_sched);
    }

    lwb_g_sync_init();
    lwb_g_rr_init();

    process_start(&lwb_main_process, NULL);
    process_start(&glossy_process, NULL);

    if (ui8_mode == LWB_MODE_HOST) {
        // host mode
        SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * (uint32_t)RTIMER_SECOND * 2, lwb_g_sync_host);
    } else {
        // source mode
        lwb_context.joining_state = LWB_JOINING_STATE_NOT_JOINED;
        SCHEDULE_L(RTIMER_NOW() - 10, INIT_PERIOD * RTIMER_SECOND, lwb_g_sync_source);
    }


    return 1;
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_queue_packet(uint8_t* p_data, uint8_t ui8_len, uint16_t ui16_to_node_id) {
    return lwb_g_rr_queue_packet(p_data, ui8_len, ui16_to_node_id);
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_request_stream_add(uint16_t ui16_ipi, uint16_t ui16_time_offset) {
    return lwb_g_rr_stream_add(ui16_ipi, ui16_time_offset);
}

//--------------------------------------------------------------------------------------------------
void lwb_request_stream_del(uint8_t ui8_id) {
    lwb_g_rr_stream_del(ui8_id);
}

//--------------------------------------------------------------------------------------------------
void lwb_request_stream_mod(uint8_t ui8_id, uint16_t ui16_ipi) {
    lwb_g_rr_stream_mod(ui8_id, ui16_ipi);
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_get_n_my_slots() {
    return lwb_context.n_my_slots;
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_get_joining_state() {
    return lwb_context.joining_state;
}

//--------------------------------------------------------------------------------------------------
PROCESS_THREAD(lwb_main_process, ev, data)
{
    PROCESS_BEGIN();

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

        if (LWB_IS_SET_POLL_FLAG(LWB_POLL_FLAGS_DATA)) {

            lwb_g_rr_data_output();
            LWB_UNSET_POLL_FLAG(LWB_POLL_FLAGS_DATA);
        }

        if (LWB_IS_SET_POLL_FLAG(LWB_POLL_FLAGS_SCHED_END) && lwb_context.p_callbacks &&
                                                          lwb_context.p_callbacks->p_on_sched_end) {

            lwb_context.p_callbacks->p_on_sched_end();
            LWB_UNSET_POLL_FLAG(LWB_POLL_FLAGS_SCHED_END);
        }
    }

    PROCESS_END();

    return PT_ENDED;
}

uint32_t lwb_get_host_time() {
    return lwb_context.time;
}

int32_t lwb_get_skew(){
	return lwb_context.skew;
}
