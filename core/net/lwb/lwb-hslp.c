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

#include "lwb-hslp.h"

///< HSLIP states
enum {
    HSLIP_STATE_TWOPACKETS = 0,
    HSLIP_STATE_OK,
    HSLIP_STATE_ESC,
    HSLIP_STATE_RUBBISH,
    HSLIP_STATE_PROCESSING
};

#ifndef LWB_CONF_HSLP_RX_BUF_LEN
#define LWB_CONF_HSLP_RX_BUF_LEN    256
#endif

static uint8_t ui8_state = HSLIP_STATE_RUBBISH;
static uint8_t ui8arr_rxbuf[LWB_CONF_HSLP_RX_BUF_LEN];
static uint8_t ui8_data_len = 0;

//--------------------------------------------------------------------------------------------------
static uint8_t lwb_hslip_input_byte(uint8_t c) {

    switch(ui8_state) {
        case HSLIP_STATE_PROCESSING:
            return 0;
        case HSLIP_STATE_RUBBISH:
        {
            if(c == HSLIP_END) {
                ui8_state = HSLIP_STATE_OK;
            }
            return 0;
        }
        case HSLIP_STATE_ESC:
        {
            if(c == HSLIP_ESC_END) {
                c = HSLIP_END;
            } else if(c == HSLIP_ESC_ESC) {
                c = HSLIP_ESC;
            } else {
                ui8_state = HSLIP_STATE_RUBBISH;
                ui8_data_len = 0; // remove rubbish
                return 0;
            }
            ui8_state = HSLIP_STATE_OK;
        }
        break;
        case HSLIP_STATE_OK:
        {
            if(c == HSLIP_ESC) {
                ui8_state = HSLIP_STATE_ESC;
                return 0;
            } else if(c == HSLIP_END) {
                if(ui8_data_len > 0) {
                    ui8_state = HSLIP_STATE_PROCESSING;
                    return 1;
                }
                return 0;
            } else {
                // Different character. do nothing in here
            }
        }
        break;
        default:
        break;
    }

    if (ui8_data_len > LWB_CONF_HSLP_RX_BUF_LEN) {
        // buffer overflow. We discard the packet
        ui8_data_len = 0;
        ui8_state = HSLIP_STATE_RUBBISH;
    } else {
        ui8arr_rxbuf[ui8_data_len++] = c;
    }

   return 0;
}

//--------------------------------------------------------------------------------------------------
void lwb_hslp_init() {
    ui8_data_len = 0;
    ui8_state = HSLIP_STATE_RUBBISH;
}

//--------------------------------------------------------------------------------------------------
void lwb_hslp_write_byte(uint8_t c) {
    switch(c) {
        case HSLIP_END:
            lwb_hslp_arch_write_byte(HSLIP_ESC);
            c = HSLIP_ESC_END;
            break;
        case HSLIP_ESC:
            lwb_hslp_arch_write_byte(HSLIP_ESC);
            c = HSLIP_ESC_ESC;
            break;
    }
    lwb_hslp_arch_write_byte(c);
}

//--------------------------------------------------------------------------------------------------
void lwb_hslp_send(uint8_t ui8_pkt_type, uint8_t* p_data, uint8_t ui8_len) {

    uint8_t ui8_i;

    lwb_hslp_arch_write_byte(HSLIP_END);
    lwb_hslp_write_byte(ui8_pkt_type);

    for(ui8_i = 0; ui8_i < ui8_len; ui8_i++) {
        lwb_hslp_write_byte(p_data[ui8_i]);
    }
   
    lwb_hslp_arch_write_byte(HSLIP_END);
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_hslp_read(rtimer_clock_t t_stop) {

    uint8_t ui8_c;
    uint8_t ui8_done = 0;

    ui8_state = HSLIP_STATE_RUBBISH;
    ui8_data_len = 0;

    do {
        // read a byte from the external device in blocking mode until timeout occurs
        while(!(ui8_done = lwb_hslp_arch_read_byte(&ui8_c)) && RTIMER_CLOCK_LT(RTIMER_NOW(), t_stop));

        if (!ui8_done) break;

    } while(!lwb_hslip_input_byte(ui8_c));

    if (!ui8_done) {
        // Oops..! timeout happened
        ui8_data_len = 0;
        return 0;
    }

    return ui8_data_len;
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_hslp_get_packet_type() {
    return ui8arr_rxbuf[0];
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_hslp_get_data_len() {
    return ui8_data_len ? ui8_data_len - 1 : 0;
}

//--------------------------------------------------------------------------------------------------
uint8_t* lwb_hslp_get_data() {
    return &ui8arr_rxbuf[1];
}
