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

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "lwb.h"

#include "lwb-print.h"

PROCESS(lwb_test_process, "lwb test");
AUTOSTART_PROCESSES(&lwb_test_process);

#define LWB_HOST_ID             1
#define LWB_DATA_ECHO_CLIENT    12
#define LWB_DATA_ECHO_SERVER    2
extern lwb_context_t lwb_context;
extern uint16_t node_id;
uint16_t rxCounter = 0;
uint16_t msgCount = 0, burstCount = 0;

static struct etimer burstTimer, messageTimer;
static uint8_t buffer[100];
static uint8_t ui8_buf_len = 0;
static uint16_t ui16_c = 0;

inline void send_stream_mod() {

	if (((lwb_get_joining_state() == LWB_JOINING_STATE_NOT_JOINED)
			|| (lwb_get_joining_state() == LWB_JOINING_STATE_JOINED))
			&& (lwb_get_n_my_slots() == 0)) {
		lwb_request_stream_mod(node_id, 1);
	} else {
		// It is possible that we are joining or partly joined.
		// This means that we are waiting for an acknowledgment. We do not send more stream add(mod)
		// requests until we receive an acknowledgment.

		// It is also possible that the host has assigned some slots for us.
		// We just use them
	}
}

//--------------------------------------------------------------------------------------------------
void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id,
		int32_t skew) {

	uint16_t ui16_val;
//    if (node_id == LWB_DATA_ECHO_SERVER) {
//
//        ui16_val = (uint16_t)p_data[0] | ((uint16_t)(p_data[1]) << 8);
//        printf("server: on_data from %u len %u, counter %u\n", ui16_from_id, ui8_len, ui16_val);
//        lwb_queue_packet(p_data, ui8_len, LWB_DATA_ECHO_CLIENT);
//        send_stream_mod();
//
//    } else if(node_id == LWB_DATA_ECHO_CLIENT) {
//
//        ui16_val = (uint16_t)p_data[0] | ((uint16_t)(p_data[1]) << 8);
//        printf("client: on_data from %u len %u, counter %u\n", ui16_from_id, ui8_len, ui16_val);
//    }
	if (node_id != 2) {
		rxCounter++;
		uint16_t myTime = RTIMER_NOW();
		ui16_val = (uint16_t) p_data[0] | ((uint16_t) (p_data[1]) << 8);
		uint16_t timeCreate = (uint16_t) p_data[2] | ((uint16_t) (p_data[3]) << 8);
		uint16_t timeSent = (uint16_t) p_data[5] | ((uint16_t) (p_data[6]) << 8);
		uint16_t timeRx = (uint16_t) p_data[7] | ((uint16_t) (p_data[8]) << 8);
		uint16_t lat = (timeSent - timeCreate) + (myTime - timeRx);
		unsigned long latency = (unsigned long)(lat) * 1e6 / RTIMER_SECOND;
		            // Print information about average reliability.
//		printf(" mytime: %u, timeCreate %u,timeSent:%u,timeRx %u\n", myTime,timeCreate,timeSent,timeRx);
		printf("seq: %u, lat: %u, latency %lu.%03lu ms reliability %3lu.%03lu %%\n",ui16_val,lat,latency / 1000, latency % 1000,avg_rel / 1000, avg_rel % 1000);
////
////
////		printf("server: on_data from %u len %u, seq: %u rxCounter; %u\n",
//				ui16_from_id, ui8_len, ui16_val, rxCounter);

	}
}

//--------------------------------------------------------------------------------------------------
void on_schedule_end(void) {
	//lwb_print_stats();
}

lwb_callbacks_t lwb_callbacks = { on_data, on_schedule_end };

//--------------------------------------------------------------------------------------------------
PROCESS_THREAD(lwb_test_process, ev, data) {
	PROCESS_BEGIN()
		;

		if (node_id == LWB_HOST_ID) {

			lwb_init(LWB_MODE_HOST, &lwb_callbacks);

		} else {

			lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);

			if (node_id == 2) {

				send_stream_mod();
				etimer_set(&burstTimer, CLOCK_SECOND * 10);

				while (1) {
					PROCESS_YIELD()
					;
					if (ev == PROCESS_EVENT_TIMER) {
						if (data == &burstTimer) {

							buffer[0] = ui16_c & 0xFF;
							buffer[1] = (ui16_c >> 8) & 0xFF;
							uint16_t timernow = RTIMER_NOW();
							buffer[2] = timernow & 0xFF;
							buffer[3] = (timernow >> 8) & 0xFF;
							memset(buffer + 4, 1, 80);
							ui8_buf_len = 82;
							ui16_c++;
							lwb_queue_packet(buffer, ui8_buf_len, 0);
							etimer_set(&messageTimer, CLOCK_SECOND * 0.1);
							msgCount = msgCount + 1;
							send_stream_mod();
						} else if (data == &messageTimer) {

							if (msgCount < 20) {

								buffer[0] = ui16_c & 0xFF;
								buffer[1] = (ui16_c >> 8) & 0xFF;
								uint16_t timernow = RTIMER_NOW();
								buffer[2] = timernow & 0xFF;
								buffer[3] = (timernow >> 8) & 0xFF;

								memset(buffer + 4, 1, 80);
								ui8_buf_len = 82;
								ui16_c++;
								lwb_queue_packet(buffer, ui8_buf_len, 0);
								etimer_set(&messageTimer, CLOCK_SECOND * 0.1);
								msgCount = msgCount + 1;
								send_stream_mod();
							} else {
								uint8_t randomnum = 1 + (uint8_t) rand() % 9;
								msgCount = 0;
								send_stream_mod();
								etimer_set(&burstTimer,
										CLOCK_SECOND * randomnum);
							}
						}
					}

				}

			} else if (node_id == LWB_DATA_ECHO_SERVER) {

			}
		}

	PROCESS_END();

return PT_ENDED;
}

