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
 *         Glossy core, header file.
 * \author
 *         Federico Ferrari <ferrari@tik.ee.ethz.ch>
 */

#ifndef GLOSSY_H_
#define GLOSSY_H_

#include "contiki.h"
#include "glossy-arch.h"

PROCESS_NAME(glossy_process);

/* ----------------------- Application interface -------------------- */
/**
 * \defgroup glossy_interface Glossy API
 * @{
 * \file   glossy.h
 * \file   glossy.c
 */

/**
 * \defgroup glossy_main Interface related to flooding
 * @{
 */

/**
 * \brief            Start Glossy and stall all other application tasks.
 *
 * \param data_      A pointer to the flooding data.
 *
 *                   At the initiator, Glossy reads from the given memory
 *                   location data provided by the application.
 *
 *                   At a receiver, Glossy writes to the given memory
 *                   location data for the application.
 * \param data_len_  Length of the flooding data, in bytes.
 * \param initiator_ Not zero if the node is the initiator,
 *                   zero if it is a receiver.
 * \param sync_      Not zero if Glossy must provide time synchronization,
 *                   zero otherwise.
 * \param tx_max_    Maximum number of transmissions (N).
 */
void glossy_start(uint8_t *data_, uint8_t data_len_, uint8_t initiator_,
        uint8_t sync_, uint8_t tx_max_, uint8_t header_,
        rtimer_clock_t t_stop_, rtimer_callback_t cb_,
        struct rtimer *rtimer_, void *ptr_);

/**
 * \brief            Stop Glossy and resume all other application tasks.
 * \returns          Number of times the packet has been received during
 *                   last Glossy phase.
 *                   If it is zero, the packet was not successfully received.
 * \sa               get_rx_cnt
 */
uint8_t glossy_stop(void);

void glossy_reset(void);

/**
 * \brief            Get the last received counter.
 * \returns          Number of times the packet has been received during
 *                   last Glossy phase.
 *                   If it is zero, the packet was not successfully received.
 */
uint8_t get_rx_cnt(void);
uint8_t get_tx_cnt(void);

/**
 * \brief            Get the current Glossy state.
 * \return           Current Glossy state, one of the possible values
 *                   of \link glossy_state \endlink.
 */
uint8_t get_state(void);
uint8_t get_data_len(void);
uint8_t get_header(void);

/**
 * \brief            Get low-frequency time of first packet reception
 *                   during the last Glossy phase.
 * \returns          Low-frequency time of first packet reception
 *                   during the last Glossy phase.
 */
rtimer_clock_t get_t_first_rx_l(void);

/** @} */

/**
 * \defgroup glossy_sync Interface related to time synchronization
 * @{
 */

/**
 * \brief            Get the last relay counter.
 * \returns          Value of the relay counter embedded in the first packet
 *                   received during the last Glossy phase.
 */
uint8_t get_relay_cnt(void);

/**
 * \brief            Get the local estimation of T_slot, in DCO clock ticks.
 * \returns          Local estimation of T_slot.
 */
rtimer_clock_t get_T_slot_h(void);

/**
 * \brief            Get low-frequency synchronization reference time.
 * \returns          Low-frequency reference time
 *                   (i.e., time at which the initiator started the flood).
 */
rtimer_clock_t get_t_ref_l(void);

/**
 * \brief            Provide information about current synchronization status.
 * \returns          Not zero if the synchronization reference time was
 *                   updated during the last Glossy phase, zero otherwise.
 */
uint8_t is_t_ref_l_updated(void);

/**
 * \brief            Set low-frequency synchronization reference time.
 * \param t          Updated reference time.
 *                   Useful to manually update the reference time if a
 *                   packet has not been received.
 */
void set_t_ref_l(rtimer_clock_t t);

/**
 * \brief            Set the current synchronization status.
 * \param updated    Not zero if a node has to be considered synchronized,
 *                   zero otherwise.
 */
void set_t_ref_l_updated(uint8_t updated);

/** @} */

/** @} */

/**
 * \defgroup glossy_internal Glossy internal functions
 * @{
 * \file   glossy.h
 * \file   glossy.c
 */

#endif // GLOSSY_H_
