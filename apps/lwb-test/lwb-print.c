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

#include "contiki.h"

#include "glossy.h"

#include "lwb-print.h"
#include "lwb-common.h"
#include "lwb-macros.h"

extern lwb_context_t lwb_context;

void lwb_print_stats() {

    if (lwb_context.lwb_mode == LWB_MODE_SOURCE) {
        printf("state %u, ref %u, rtimer next %u, oflows %u, skew %ld, guard %u\n",
               lwb_context.sync_state,
               lwb_context.t_last_sync_ref,
               RTIMER_TIME(&lwb_context.rt),
               lwb_context.rt.overflows_to_go,
               (lwb_context.skew/(int32_t)64),
               lwb_context.t_sync_guard);

    }

    printf("time %u, n_slots [data %u, free %u, my %u], T %u\n",
           CURRENT_SCHEDULE_INFO().time,
           GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots),
           GET_N_FREE_SLOTS(CURRENT_SCHEDULE_INFO().n_slots),
           lwb_context.n_my_slots,
           CURRENT_SCHEDULE_INFO().round_period);

    uint8_t i = 0;

    if (lwb_context.sync_state == LWB_SYNC_STATE_SYNCED ||
        lwb_context.sync_state == LWB_SYNC_STATE_UNSYNCED_1) {
        printf("sched : ");
        for (i = 0; i < GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots); i++) {
            printf("%04X ", CURRENT_SCHEDULE().slots[i]);
        }
        printf("\n");
    }

    if (lwb_context.lwb_mode == LWB_MODE_HOST) {
        printf("stream stats [added %u, dups : %u, no_space %u]\n",
                LWB_STATS_SCHED(n_added),
                LWB_STATS_SCHED(n_duplicates),
                LWB_STATS_SCHED(n_no_space));
    }

}
