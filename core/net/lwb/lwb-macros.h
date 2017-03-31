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

#ifndef __LWB_MACROS_H___
#define __LWB_MACROS_H___

/// @file lwb-macros.h
/// @brief LWB context specific macros

#include "contiki.h"

#include "lwb-common.h"

#define N_CURRENT_DATA_SLOTS()          GET_N_DATA_SLOTS(lwb_context.current_sched.sched_info.n_slots)
#define N_CURRENT_FREE_SLOTS()          GET_N_FREE_SLOTS(lwb_context.current_sched.sched_info.n_slots)
#define CURRENT_SCHEDULE()              (lwb_context.current_sched)
#define CURRENT_SCHEDULE_INFO()         (lwb_context.current_sched.sched_info)

#define OLD_SCHEDULE()                  (lwb_context.old_sched)
#define OLD_SCHEDULE_INFO()             (lwb_context.old_sched.sched_info)

#define LWB_STATS_SYNC(statitem)        (lwb_context.sync_stats.statitem)
#define LWB_STATS_DATA(statitem)        (lwb_context.data_stats.statitem)
#define LWB_STATS_STREAM_REQ_ACK(statitem)  (lwb_context.stream_req_ack_stats.statitem)
#define LWB_STATS_SCHED(statitem)       (lwb_context.sched_stats.statitem)

#define LWB_SET_POLL_FLAG(flag)         (lwb_context.poll_flags |= 1 << flag)
#define LWB_UNSET_POLL_FLAG(flag)       (lwb_context.poll_flags &= ~(1 << flag))
#define LWB_IS_SET_POLL_FLAG(flag)      (lwb_context.poll_flags && (1 << flag))

/// @addtogroup rtimer scheduling
///             macro for rtimer based scheduling
/// @{
#define SCHEDULE(ref, offset, cb)   rtimer_set(&lwb_context.rt, ref + offset, 1, (rtimer_callback_t)cb, &lwb_context)
#define SCHEDULE_L(ref, offset, cb) rtimer_set_long(&lwb_context.rt, ref, offset, (rtimer_callback_t)cb, &lwb_context)
///  @}

#endif // __LWB_MACROS_H___
