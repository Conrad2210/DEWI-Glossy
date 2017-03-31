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

#include "lwb-sched-compressor.h"


#if TEST_LWB_SCHED_COMPRESSOR
#include <stdio.h>
#endif

#include <string.h>


#define COMP_BUFFER_LEN 128
static uint8_t ui8arr_comp_buf[COMP_BUFFER_LEN]; ///< Compression buffer

//--------------------------------------------------------------------------------------------------
static inline uint8_t get_n_bits(uint16_t a) {
    uint8_t i;
    for (i = 15; i > 0; i--) {
        if (a & (1 << i)) {
            return i + 1;
        }
    }
    return i + 1;
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_sched_compress(lwb_schedule_t *p_sched, uint8_t* p_buf, uint8_t ui8_buf_len) {

    uint8_t ui8_i = 0;
    uint8_t ui8_max_bits = 0;
    uint8_t ui8_n_bits = 0;
    uint32_t ui32_tmp = 0;
    uint16_t ui16_bit_start = 0;
    uint8_t ui8_req_len = 0;

    // Finding number of maximum bits required
    for (ui8_i = 0; ui8_i < GET_N_DATA_SLOTS(p_sched->sched_info.n_slots); ui8_i++) {

        ui8_n_bits = get_n_bits(p_sched->slots[ui8_i]);
        if (ui8_n_bits > ui8_max_bits) {
          ui8_max_bits = ui8_n_bits;
        }
    }

    // Calculating required buffer length. 1 byte is additionally required to store bit length
    ui8_req_len = (ui8_max_bits * GET_N_DATA_SLOTS(p_sched->sched_info.n_slots) / 8) +
                  (((ui8_max_bits * GET_N_DATA_SLOTS(p_sched->sched_info.n_slots)) % 8) ? 1 : 0);

    if (ui8_buf_len < ui8_req_len + 1) {
        // Not enough space in destination buffer
        return 0;
    }

    if (COMP_BUFFER_LEN < ((ui8_max_bits * GET_N_DATA_SLOTS(p_sched->sched_info.n_slots) / 8) + 3)) {
        // compression buffer is not enough
        return 0;
    }

    // reset the buffer
    memset(ui8arr_comp_buf, 0, COMP_BUFFER_LEN);

    // Compressing
    for (ui8_i = 0; ui8_i < GET_N_DATA_SLOTS(p_sched->sched_info.n_slots); ui8_i++) {

        ui16_bit_start = (uint16_t)ui8_i * (uint16_t)ui8_max_bits;

        ui32_tmp = (uint32_t)ui8arr_comp_buf[(ui16_bit_start / 8)] |
                   ((uint32_t)ui8arr_comp_buf[(ui16_bit_start / 8) + 1] << 8) |
                   ((uint32_t)ui8arr_comp_buf[(ui16_bit_start / 8) + 2] << 16);

        ui32_tmp |= (uint32_t)p_sched->slots[ui8_i] << (ui16_bit_start % 8);

        ui8arr_comp_buf[(ui16_bit_start / 8)] = (uint8_t)(ui32_tmp & 0xFF);
        ui8arr_comp_buf[(ui16_bit_start / 8) + 1] = (uint8_t)((ui32_tmp >> 8) & 0xFF);
        ui8arr_comp_buf[(ui16_bit_start / 8) + 2] = (uint8_t)((ui32_tmp >> 16) & 0xFF);

    }

    p_buf[0] = ui8_max_bits;
    memcpy(p_buf + 1, ui8arr_comp_buf, ui8_req_len);

    return ui8_req_len + 1;
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_sched_decompress(lwb_schedule_t *p_sched, uint8_t* p_buf, uint8_t ui8_buf_len) {

    uint8_t ui8_i = 0;
    uint8_t ui8_max_bits = 0;
    uint32_t ui32_tmp = 0;
    uint16_t ui16_bit_start = 0;

    if (ui8_buf_len < 1) {
        // Not enough data
        return 0;
    }

    ui8_max_bits = p_buf[0];

    if (COMP_BUFFER_LEN < ((ui8_max_bits * GET_N_DATA_SLOTS(p_sched->sched_info.n_slots) / 8) + 3)) {
        // Not enough space in compression buffer
        return 0;
    }

    // reset the compression buffer
    memset(ui8arr_comp_buf, 0, COMP_BUFFER_LEN);
    // copy compressed data to compression buffer
    memcpy(ui8arr_comp_buf, p_buf + 1, ui8_buf_len - 1);

    for (ui8_i = 0; ui8_i < GET_N_DATA_SLOTS(p_sched->sched_info.n_slots); ui8_i++) {

        ui16_bit_start = (uint16_t)ui8_i * (uint16_t)ui8_max_bits;

        ui32_tmp = (uint32_t)ui8arr_comp_buf[ui16_bit_start / 8] |
                   ((uint32_t)ui8arr_comp_buf[ui16_bit_start / 8 + 1] << 8) |
                   ((uint32_t)ui8arr_comp_buf[ui16_bit_start / 8 + 2] << 16);

        p_sched->slots[ui8_i] = (ui32_tmp >> (ui16_bit_start % 8)) & ((1 << ui8_max_bits) - 1);
    }

    return GET_N_DATA_SLOTS(p_sched->sched_info.n_slots);
}

//--------------------------------------------------------------------------------------------------

#if TEST_LWB_SCHED_COMPRESSOR

void print_comp_sched(uint8_t* p_buf, uint8_t ui8_buf_len) {

    if (ui8_buf_len < 1){
        return;
    }

    printf("%02X | ", p_buf[0]);
    p_buf++;
    ui8_buf_len--;

    uint8_t i = 0;
    for (; i < ui8_buf_len; i++) {
        printf("%02X ", p_buf[i]);
    }

    printf("\n");
}


void print_sched(lwb_schedule_t *p_sched) {

    printf("S %03u | ", p_sched->sched_info.n_slots);

    uint8_t i = 0;
    for (; i < p_sched->sched_info.n_slots; i++) {
        printf("%04X ", p_sched->slots[i]);
    }

    printf("\n");
}

int main(int argc, char *argv[]) {

    char buffer[128];
    uint8_t status = 0;

    lwb_schedule_t sched;
    sched.sched_info.host_id = 1;
    sched.sched_info.time = 1;
    sched.sched_info.round_period = 1;
    sched.sched_info.n_slots = 10;

    sched.slots[0] = 0x0001;
    sched.slots[1] = 0x0002;
    sched.slots[2] = 0x0001;
    sched.slots[3] = 0x00FA;
    sched.slots[4] = 0x0003;

    sched.slots[5] = 0x00F1;
    sched.slots[6] = 0x0001;
    sched.slots[7] = 0x0002;
    sched.slots[8] = 0x0003;
    sched.slots[9] = 0x000F;


    print_sched(&sched);

    status = lwb_sched_compress(&sched, buffer, 127);

    printf("compression status : %u\n", status);

    print_comp_sched(buffer, status);

    memset(sched.slots, 0, LWB_SCHED_MAX_SLOTS * 2);

    status = lwb_sched_decompress(&sched, buffer, status);

    printf("decompression status : %u\n", status);

    print_sched(&sched);

    return 0;
}

#endif

