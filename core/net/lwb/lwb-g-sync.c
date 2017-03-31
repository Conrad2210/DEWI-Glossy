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
 * Following implementation is based on the implementation used in
 * http://dl.acm.org/citation.cfm?id=2426658
 *
 */

#include <string.h>

#include "contiki.h"
#include "dev/leds.h"

#include "glossy.h"

#include "lwb-common.h"
#include "lwb-macros.h"
#include "lwb-g-sync.h"
#include "lwb-g-rr.h"
#include "lwb-sched-compressor.h"

extern lwb_context_t lwb_context;
extern uint16_t node_id;

/// @brief We use the same proto thread for both host and source since only one mode is active at a time.
static struct pt pt_lwb_g_sync;

PROCESS_NAME(lwb_main_process);

void lwb_save_ctrl_energest() {
    // This function should be called before starting Glossy for schedules, stream requests and
    // stream acknowledgements
#if LWB_CONTROL_DC
    lwb_context.en_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
    lwb_context.en_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
#endif // LWB_CONTROL_DC
}
void lwb_update_ctrl_energest() {
    // This function should be called after Glossy finishes for schedules, stream requests and
    // stream acknowledgements
#if LWB_CONTROL_DC
    lwb_context.en_control += (energest_type_time(ENERGEST_TYPE_LISTEN) - lwb_context.en_rx) +
                                    (energest_type_time(ENERGEST_TYPE_TRANSMIT) - lwb_context.en_tx);
#endif // LWB_CONTROL_DC
}

//--------------------------------------------------------------------------------------------------
void lwb_g_sync_init() {
    PT_INIT(&pt_lwb_g_sync);
}

//--------------------------------------------------------------------------------------------------
PT_THREAD(lwb_g_sync_host(struct rtimer *t, lwb_context_t *p_context)) {

    PT_BEGIN(&pt_lwb_g_sync);

    // Compress and copy the schedule to buffer
    memcpy(lwb_context.txrx_buf, &CURRENT_SCHEDULE_INFO(), sizeof(lwb_sched_info_t));


    lwb_context.txrx_buf_len = sizeof(lwb_sched_info_t);
    lwb_context.txrx_buf_len += lwb_sched_compress(&CURRENT_SCHEDULE(),
                                                       lwb_context.txrx_buf + lwb_context.txrx_buf_len,
                                                       LWB_MAX_TXRX_BUF_LEN - lwb_context.txrx_buf_len);

    // Assume the reference time is estimated 1 second ago.
    lwb_context.t_sync_ref = RTIMER_TIME(t) - RTIMER_SECOND;

    while (1) {
        leds_on(LEDS_GREEN);

        lwb_context.t_start = RTIMER_TIME(t);

        CURRENT_SCHEDULE_INFO().t_current_rtimer = RTIMER_NOW();
        //printf("hsot synch: t_current_rtimer: %u\n",CURRENT_SCHEDULE_INFO().t_current_rtimer);
        // Disseminate schedule
        // At this point, we expect the compressed scheduled to be in p_context->txrx_buf and
        // p_context->txrx_buf_len is set accordingly.
        glossy_start(lwb_context.txrx_buf,
                     lwb_context.txrx_buf_len,
                     GLOSSY_INITIATOR,
                     GLOSSY_SYNC,
                     N_SYNC,
                     LWB_PKT_TYPE_SCHED,
                     RTIMER_TIME(t) + T_SYNC_ON,
                     (rtimer_callback_t)lwb_g_sync_host,
                     &lwb_context.rt,
                     &lwb_context);
        PT_YIELD(&pt_lwb_g_sync);

        leds_off(LEDS_GREEN);
        glossy_stop();

        if (GLOSSY_IS_SYNCED() && RTIMER_CLOCK_LT(lwb_context.t_start, GLOSSY_T_REF)) {
            // Glossy is time synchronized
            /// @todo find why RTIMER_CLOCK_LT is used in here.
            // we set the reference time taken from Glossy

            lwb_context.t_sync_ref = GLOSSY_T_REF;
            LWB_STATS_SYNC(n_synced)++;

        } else {
            // Glossy is not time synchronized (e.g. when no nodes are around)
            // So, we artificially update reference time by using schedule creation time of previous
            // schedule.
            // Why "t_diff % 2" is used is explained under lwb_estimate_skew().
            uint16_t t_diff = CURRENT_SCHEDULE_INFO().time - OLD_SCHEDULE_INFO().time;
            lwb_context.t_sync_ref = lwb_context.t_sync_ref + (RTIMER_SECOND * (t_diff % 2));
            set_t_ref_l(lwb_context.t_sync_ref);

            LWB_STATS_SYNC(n_sync_missed)++;
        }

        lwb_g_rr_host(t, p_context);

        PT_YIELD(&pt_lwb_g_sync);

    }

    PT_END(&pt_lwb_g_sync);
    return PT_ENDED;
}

//--------------------------------------------------------------------------------------------------
static inline uint8_t lwb_get_n_my_slots() {
    uint8_t ui8_slots = 0;
    uint8_t i;

    for (i = 0; i < N_CURRENT_DATA_SLOTS(); i++) {
        if (node_id == CURRENT_SCHEDULE().slots[i]) {
            ui8_slots++;
        }
    }

    return ui8_slots;
}

//--------------------------------------------------------------------------------------------------
static inline void lwb_estimate_skew() {

    uint16_t t_diff = CURRENT_SCHEDULE_INFO().time - OLD_SCHEDULE_INFO().time;
    // The maximum time that can be measured with rtimer (32 kHz) is 2.048 seconds before integer wrap-round.
    // This is due to 16-bit counters are used in Glossy.
    // Therefore, if t_diff is a multiple of 2, then we do not need to use it for skew calculation.
    int16_t skew_tmp = lwb_context.t_sync_ref - (lwb_context.t_last_sync_ref + (uint16_t)RTIMER_SECOND * (t_diff % 2));

    if (skew_tmp < 500 && t_diff != 0) {

        /// @todo Why 64 is used in here.
        ///       I think 64 is used to make skew_tmp large. The skew_tmp is calculated in rtimer
        ///       clock ticks. If it is smaller than t_diff, clock skew per unit time becomes zero.
        ///       Therefore, skew_tmp is multiplied by 64 to make it larger.
        lwb_context.skew = (int32_t)(64 * (int32_t)skew_tmp) / (int32_t)t_diff;
        /// @todo "dirty hack" to temporary fix a Glossy bug that rarely occurs in disconnected networks
        OLD_SCHEDULE_INFO().time = CURRENT_SCHEDULE_INFO().time;
        lwb_context.t_last_sync_ref = GLOSSY_T_REF;

    } else {

        lwb_context.sync_state = LWB_SYNC_STATE_BOOTSTRAP;
        set_t_ref_l_updated(0);
    }
}

//--------------------------------------------------------------------------------------------------
static inline void compute_new_sync_state() {

    if (GLOSSY_IS_SYNCED()) {
        // Reference time of Glossy is updated.
        LWB_STATS_SYNC(n_synced)++;

        if ((lwb_context.sync_state == LWB_SYNC_STATE_QUASI_SYNCED ||
             lwb_context.sync_state == LWB_SYNC_STATE_SYNCED) &&
                        (CURRENT_SCHEDULE_INFO().time < OLD_SCHEDULE_INFO().time)) {
            // 16-bit overflow
            UI32_SET_HIGH(lwb_context.time, UI32_GET_HIGH(lwb_context.time) + 1);
        }

        UI32_SET_LOW(lwb_context.time, CURRENT_SCHEDULE_INFO().time);

        switch (lwb_context.sync_state) {
            case LWB_SYNC_STATE_BOOTSTRAP:
            {
                lwb_context.sync_state = LWB_SYNC_STATE_QUASI_SYNCED;

                lwb_context.t_sync_guard = T_GUARD_3;
                lwb_context.t_last_sync_ref = GLOSSY_T_REF;
                lwb_context.t_sync_ref = GLOSSY_T_REF;
            }
            break;
            default:
            {
                // If Glossy's reference time is updated while in any other states,
                // we consider LWB is synchronized.
                lwb_context.sync_state = LWB_SYNC_STATE_SYNCED;

                lwb_context.t_sync_guard = T_GUARD;
                lwb_context.t_sync_ref = GLOSSY_T_REF;
                lwb_estimate_skew();
            }
            break;
        }

    } else {
        // Reference time of Glossy is not updated.

        LWB_STATS_SYNC(n_sync_missed)++;

        switch (lwb_context.sync_state) {
            case LWB_SYNC_STATE_SYNCED:
            {
                lwb_context.sync_state = LWB_SYNC_STATE_UNSYNCED_1;
                lwb_context.t_sync_guard = T_GUARD_1;
            }
            break;
            case LWB_SYNC_STATE_UNSYNCED_1:
            {
                lwb_context.sync_state = LWB_SYNC_STATE_UNSYNCED_2;
                lwb_context.t_sync_guard = T_GUARD_2;
            }
            break;
            case LWB_SYNC_STATE_UNSYNCED_2:
            {
                lwb_context.sync_state = LWB_SYNC_STATE_UNSYNCED_3;
                lwb_context.t_sync_guard = T_GUARD_3;
            }
            break;
            case LWB_SYNC_STATE_UNSYNCED_3:
            case LWB_SYNC_STATE_QUASI_SYNCED:
            {
                // go back to bootstrap
                lwb_context.sync_state = LWB_SYNC_STATE_BOOTSTRAP;
                lwb_context.t_sync_guard = T_GUARD_3;
                lwb_context.skew = 0;
            }
            break;
            default:
                break;
        }

        if (lwb_context.sync_state != LWB_SYNC_STATE_BOOTSTRAP) {
            // We are not bootstrapping. So, we calculate the new schedule information based on the old one.

            // set new time based on old time
            if (OLD_SCHEDULE_INFO().round_period == 1) {
                CURRENT_SCHEDULE_INFO().time = OLD_SCHEDULE_INFO().time + 1;
            } else {
                /// @todo Find why sched_info.round_period - 1 is used
                CURRENT_SCHEDULE_INFO().time = OLD_SCHEDULE_INFO().time + OLD_SCHEDULE_INFO().round_period;
            }

            CURRENT_SCHEDULE_INFO().round_period = OLD_SCHEDULE_INFO().round_period;

            // compute new reference time
            // Why "ui16_t_diff % 2" is used is explained under lwb_estimate_skew().
            uint16_t ui16_t_diff = CURRENT_SCHEDULE_INFO().time - OLD_SCHEDULE_INFO().time;
            uint16_t new_t_ref = lwb_context.t_last_sync_ref +
                                 ((int32_t)ui16_t_diff * lwb_context.skew / (int32_t)64) +
                                 ((uint32_t)RTIMER_SECOND * (ui16_t_diff % 2));
            set_t_ref_l(new_t_ref);
            lwb_context.t_last_sync_ref = new_t_ref;
            lwb_context.t_sync_ref = new_t_ref;

        } else {
            // The new state is bootstrap. We do not calculate things based on old information.
        }

    }
}


//--------------------------------------------------------------------------------------------------

PT_THREAD(lwb_g_sync_source(struct rtimer *t, lwb_context_t *p_context)) {
    PT_BEGIN(&pt_lwb_g_sync);

    lwb_context.sync_state = LWB_SYNC_STATE_BOOTSTRAP;

    while (1) {
        leds_on(LEDS_GREEN);

        if (lwb_context.sync_state == LWB_SYNC_STATE_BOOTSTRAP) {
            // We are in the bootstrap mode.
            // Start Glossy to receive initial schedule.
            lwb_save_ctrl_energest();
            glossy_start(lwb_context.txrx_buf,
                         0,
                         GLOSSY_RECEIVER,
                         GLOSSY_SYNC,
                         N_SYNC,
                         0,
                         RTIMER_NOW() + T_SYNC_ON,
                         (rtimer_callback_t)lwb_g_sync_source,
                         &lwb_context.rt,
                         &lwb_context);
            PT_YIELD(&pt_lwb_g_sync);

            while (!GLOSSY_IS_SYNCED()) {
                // We have to receive the initial schedule in order to proceed.
                // Therefore, we loop until we receive a schedule.
                glossy_stop();
                lwb_update_ctrl_energest();

                //process_poll(&lwb_print);

                // wait 50 millisecond before retrying
                //rtimer_clock_t now = RTIMER_NOW();
                //while (RTIMER_CLOCK_LT(RTIMER_NOW(), now + RTIMER_SECOND / 20));

                lwb_save_ctrl_energest();
                glossy_start(lwb_context.txrx_buf,
                             0,
                             GLOSSY_RECEIVER,
                             GLOSSY_SYNC,
                             N_SYNC,
                             0,
                             RTIMER_NOW() + T_SYNC_ON,
                             (rtimer_callback_t)lwb_g_sync_source,
                             &lwb_context.rt,
                             &lwb_context);
                PT_YIELD(&pt_lwb_g_sync);
            }

        } else {
            // We are not in the bootstrap mode.
            // we try to receive schedule
            lwb_save_ctrl_energest();
            glossy_start(lwb_context.txrx_buf,
                         0,
                         GLOSSY_RECEIVER,
                         GLOSSY_SYNC,
                         N_SYNC,
                         0,
                         RTIMER_TIME(t) + T_SYNC_ON,
                         (rtimer_callback_t)lwb_g_sync_source,
                         &lwb_context.rt,
                         &lwb_context);
            PT_YIELD(&pt_lwb_g_sync);
        }

        glossy_stop();
        lwb_update_ctrl_energest();

        if (get_rx_cnt() && (get_header() != LWB_PKT_TYPE_SCHED)) {
          // We have received Glossy but not with the schedule header.
          // Therefore, we consider this as an unsynchronized situation.
          // We set Glossy's reference time is not updated.
          set_t_ref_l_updated(0);
        }

        lwb_context.txrx_buf_len = get_data_len();
        // We copy only the schedule header.
        memcpy(&CURRENT_SCHEDULE_INFO(), lwb_context.txrx_buf, sizeof(lwb_sched_info_t));
       // printf("GLOSSY_T_REF: %u offset: %u, lat %u \n",GLOSSY_T_REF,currentTime - GLOSSY_T_REF,lat);
       // printf("LWB SYNC: mytime %u, t_offset %d, rtimer host %u, latency %lu.%03lu ms \n", lwb_context.t_offset, CURRENT_SCHEDULE_INFO().t_current_rtimer,latency / 1000, latency % 1000);
        compute_new_sync_state();

        if ((lwb_context.sync_state == LWB_SYNC_STATE_SYNCED || lwb_context.sync_state == LWB_SYNC_STATE_UNSYNCED_1)) {
            // We are synchronized or missed one schedule

            if (lwb_context.sync_state == LWB_SYNC_STATE_SYNCED) {
                // We decompress schedule if the state is synced. Otherwise, we do not need to do
                // it since we copy the old schedule to current schedule
                lwb_sched_decompress(&CURRENT_SCHEDULE(),
                                     lwb_context.txrx_buf + sizeof(lwb_sched_info_t),
                                     lwb_context.txrx_buf_len - sizeof(lwb_sched_info_t));
                lwb_context.n_my_slots = lwb_get_n_my_slots();
            } else {
                // We just missed one schedule. We do not give up even if we miss one schedule.
                // It is reasonable to assume that the next schedule is also to be the same.
                // So, we copy the old schedule to current one.
                CURRENT_SCHEDULE_INFO().host_id = OLD_SCHEDULE_INFO().host_id;
                CURRENT_SCHEDULE_INFO().n_slots = OLD_SCHEDULE_INFO().n_slots;
                memcpy(CURRENT_SCHEDULE().slots,
                       OLD_SCHEDULE().slots,
                       sizeof(OLD_SCHEDULE().slots));
            }

            lwb_g_rr_source(t, p_context);

            PT_YIELD(&pt_lwb_g_sync);

        } else {
            // We are partially synchronized or missed more schedules or in bootstrap state.

            if (lwb_context.sync_state != LWB_SYNC_STATE_BOOTSTRAP) {

                // Copy current schedule information to old one
                memcpy(&OLD_SCHEDULE_INFO(), &CURRENT_SCHEDULE_INFO(), sizeof(lwb_sched_info_t));

                // We are not in bootstrap state. e.g. LWB_QUASI_SYNCED
                if (CURRENT_SCHEDULE_INFO().round_period == 1) {
                    // Round period is 1 second. Therefore, we can use rtimer_set()
                    // This is due to rtimer is 32 kHz and 16-bit counters are used.
                    SCHEDULE(lwb_context.t_sync_ref,
                             RTIMER_SECOND + (lwb_context.skew / (int32_t)64) - lwb_context.t_sync_guard,
                             lwb_g_sync_source);
                } else {
                    // Round period is not 1 second. Therefore, we use rtimer_set_long()
                    SCHEDULE_L(lwb_context.t_sync_ref,
                               (CURRENT_SCHEDULE_INFO().round_period * (uint32_t)RTIMER_SECOND) +
                                                       ((int32_t)CURRENT_SCHEDULE_INFO().round_period * lwb_context.skew / (int32_t)64) -
                                                       lwb_context.t_sync_guard,
                               lwb_g_sync_source);
                }

                LWB_SET_POLL_FLAG(LWB_POLL_FLAGS_SCHED_END);
                process_poll(&lwb_main_process);

                PT_YIELD(&pt_lwb_g_sync);
            } else {
                // We are in the bootstrap state.
                // We have to turn on the radio until a schedule is received.
            }
        }

    }

    PT_END(&pt_lwb_g_sync);
    return PT_ENDED;
}
