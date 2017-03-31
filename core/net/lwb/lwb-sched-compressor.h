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

#ifndef __LWB_SCHED_COMPRESSOR_H__
#define __LWB_SCHED_COMPRESSOR_H__

#define TEST_LWB_SCHED_COMPRESSOR 0

#if TEST_LWB_SCHED_COMPRESSOR

#include <stdint.h>
#define LWB_SCHED_HEADER_LEN    6

#define LWB_SCHED_MAX_SLOTS      50

typedef struct lwb_sched_info {
    uint16_t host_id;
    uint16_t time;
    uint8_t  n_slots;
    uint8_t  round_period;
} lwb_sched_info_t;


typedef struct lwb_schedule {
    lwb_sched_info_t    sched_info;
    uint16_t            slots[LWB_SCHED_MAX_SLOTS];
} lwb_schedule_t;

#else
#include "lwb-common.h"
#endif

/// @brief Compress LWB schedule into byte buffer.
/// @details Schedule information is copied to the buffer from this function.
/// @param p_shed A pointer to the schedule to be compressed
/// @param p_buf A buffer to hold compressed schedule
/// @param ui8_buf_len The size of the buffer
/// @return The size of the compressed schedule. Zero is returned in case of an error
uint8_t lwb_sched_compress(lwb_schedule_t *p_sched, uint8_t* p_buf, uint8_t ui8_buf_len);

/// @brief Decompress LWB schedule from a byte buffer.
/// @details Schedule information has to be already in p_sched.
/// @param p_shed A pointer to the schedule to hold the decompressed schedule.
/// @param p_buf The buffer that holds the compressed schedule.
/// @param ui8_buf_len The size of the buffer compressed schedule
/// @return non-zero if no error occurred.
uint8_t lwb_sched_decompress(lwb_schedule_t *p_sched, uint8_t* p_buf, uint8_t ui8_buf_len);

#endif // __LWB_SHED_COMPRESSOR_H__
