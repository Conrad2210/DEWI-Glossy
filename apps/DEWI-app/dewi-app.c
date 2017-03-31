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
#include "dev/leds.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/random.h"

#include "lwb-print.h"

PROCESS(dewi_app_start, "dewi_app_start");
PROCESS(dewi_app_host, "dewi_app_host");
PROCESS(dewi_app_server, "dewi_app_server");
PROCESS(dewi_app_lightswitch, "dewi_app_lightswitch");
PROCESS(dewi_app_lamp, "dewi_app_lamp");
AUTOSTART_PROCESSES(&dewi_app_start);

#define LWB_HOST_ID         1
#define LWB_LIGHTSWITCH    	2
#define LWB_SERVER    		3

struct etimer neighUpdateTimer, neighUpdateFinishTimer, burstTimer,
		messageTimer;
extern uint16_t node_id, msgCount = 0, burstCount = 0,rxCounter=0;
uint8_t nAck = 0, rAck = 0, firstBurst = 0;
uint16_t seqNo = 0;
uint8_t printCounter = 0;
enum frameType {
	hello = 0x01,
	neighupdate = 0x02,
	dataExp = 0x03,
	result = 0x04,
	neighUpdAck = 0x05,
	startExp = 0x06,
	finishExp = 0x07,
	resultAck = 0x08
};

typedef struct resultStruct {
	struct resultStruct* next;
	uint16_t timeslot;
	uint16_t counter;
} resultStruct_t;
typedef struct neighStruct {
	struct neighStruct* next;
	uint8_t id;
} neighStruct_t;

/// @brief Result buffer element list
MEMB(mmb_results, resultStruct_t, 50);
/// @brief Result buffer element list
LIST(lst_results);

MEMB(mmb_neigh, neighStruct_t, 50);
/// @brief Result buffer element list
LIST(lst_neigh);

static uint8_t buffer[100];
static uint8_t ui8_buf_len = 0;

inline void send_stream_mod() {

	if (((lwb_get_joining_state() == LWB_JOINING_STATE_NOT_JOINED)
			|| (lwb_get_joining_state() == LWB_JOINING_STATE_JOINED))
			&& (lwb_get_n_my_slots() == 0)) {
		lwb_request_stream_mod(1, 1);
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
	uint8_t frame = (uint8_t) p_data[0];

	switch (node_id) {
	case LWB_HOST_ID:
		break;
	case LWB_SERVER:
		switch (frame) {
		case hello:
			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
					ui16_from_id);
			buffer[0] = neighUpdAck;
			memset(buffer + 1, 1, 81);
			ui8_buf_len = 82;
			lwb_queue_packet(buffer, ui8_buf_len, ui16_from_id);

			PROCESS_CONTEXT_BEGIN(&dewi_app_server)
				;
				etimer_stop(&neighUpdateFinishTimer);
				etimer_set(&neighUpdateFinishTimer, CLOCK_SECOND * 10);
				PROCESS_CONTEXT_END(&dewi_app_server)
			;
			break;

		}
		break;
	case finishExp:
		//printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
		//		ui16_from_id);
		break;
	case LWB_LIGHTSWITCH:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);
		switch (frame) {
		case startExp:
			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
					ui16_from_id);
			PROCESS_CONTEXT_BEGIN(&dewi_app_lightswitch)
				;
				etimer_set(&burstTimer, CLOCK_SECOND);
				PROCESS_CONTEXT_END(&dewi_app_lightswitch)
			;
			break;

		}
		break;
		break;
	default:
		switch (frame) {
		case neighupdate:
			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
					ui16_from_id);
			if (!nAck) {
				buffer[0] = hello;
				memset(buffer + 1, 1, 81);
				ui8_buf_len = 82;
				lwb_queue_packet(buffer, ui8_buf_len, 0);
			}
			break;
		case neighUpdAck:
			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
					ui16_from_id);
			nAck = 1;
			break;
		case dataExp:
			//printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
			//		ui16_from_id);
			rxCounter++;
			uint16_t myTime = RTIMER_NOW();
			uint16_t rxSeq = (uint16_t) p_data[1] | ((uint16_t) (p_data[2]) << 8);
			uint16_t timeCreate = (uint16_t) p_data[3] | ((uint16_t) (p_data[4]) << 8);
			uint16_t timeSent = (uint16_t) p_data[5] | ((uint16_t) (p_data[6]) << 8);
			uint16_t timeRx = (uint16_t) p_data[7] | ((uint16_t) (p_data[8]) << 8);
			uint16_t lat = (timeSent - timeCreate) + (myTime - timeRx);
			unsigned long avg_rel = rxCounter * 1e5 / (rxSeq + 1);
			unsigned long latency = (unsigned long)(lat) * 1e6 / RTIMER_SECOND;
			if(printCounter == 10){
				printf("seq: %u, lat: %u, latency %lu.%03lu ms reliability %3lu.%03lu %%\n",rxSeq,lat,latency / 1000, latency % 1000,avg_rel / 1000, avg_rel % 1000);
				printCounter = 0;
			}else
			{
				printCounter++;
			}
			break;
		}
		break;
	}

}

//--------------------------------------------------------------------------------------------------
void on_schedule_end(void) {
	if (node_id == LWB_HOST_ID)
		lwb_print_stats();
}

lwb_callbacks_t lwb_callbacks = { on_data, on_schedule_end };

//--------------------------------------------------------------------------------------------------
PROCESS_THREAD(dewi_app_start, ev, data) {
	PROCESS_BEGIN()
		;

		if (node_id == LWB_HOST_ID) {
			process_start(&dewi_app_host, NULL);
		} else if (node_id == LWB_LIGHTSWITCH) {
			process_start(&dewi_app_lightswitch, NULL);
		} else if (node_id == LWB_SERVER) {
			process_start(&dewi_app_server, NULL);
		} else {
			process_start(&dewi_app_lamp, NULL);
		}
	PROCESS_END();

return PT_ENDED;
}

PROCESS_THREAD(dewi_app_host, ev, data) {
PROCESS_BEGIN()
	;

	lwb_init(LWB_MODE_HOST, &lwb_callbacks);

PROCESS_END();

return PT_ENDED;
}

PROCESS_THREAD(dewi_app_server, ev, data) {
PROCESS_BEGIN()
;

lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);
printf("start server app\n");

send_stream_mod();
memb_init(&mmb_results);
list_init(lst_results);
etimer_set(&neighUpdateTimer, CLOCK_SECOND * 2);
etimer_set(&neighUpdateFinishTimer, CLOCK_SECOND * 60);
while (1) {
	PROCESS_YIELD()
	;
	if (ev == PROCESS_EVENT_TIMER) {
		if (data == &neighUpdateTimer) {
			if (lwb_get_joining_state() == LWB_JOINING_STATE_JOINED) {
				buffer[0] = neighupdate;
				memset(buffer + 1, 1, 81);
				ui8_buf_len = 82;
				lwb_queue_packet(buffer, ui8_buf_len, 0);
				etimer_set(&neighUpdateTimer, CLOCK_SECOND);
				//send_stream_mod();
			} else {
				etimer_set(&neighUpdateTimer, CLOCK_SECOND * 5);
			}
		} else if (data == &neighUpdateFinishTimer) {
			printf("topology finished\n");
			etimer_stop(&neighUpdateTimer);
			buffer[0] = startExp;
			memset(buffer + 1, 1, 81);
			ui8_buf_len = 82;
			lwb_queue_packet(buffer, ui8_buf_len, 0);
		}
	}
}

PROCESS_END();

return PT_ENDED;
}
PROCESS_THREAD(dewi_app_lightswitch, ev, data) {
PROCESS_BEGIN()
;

lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);

send_stream_mod();

while (1) {
PROCESS_YIELD()
;
if (ev == PROCESS_EVENT_TIMER) {
	if (data == &burstTimer) {
		printf("data slots %d\n", lwb_get_n_my_slots());
		if (lwb_get_n_my_slots() != 0 || firstBurst == 1) {
			uint16_t timernow = RTIMER_NOW();
			buffer[0] = dataExp;
			buffer[1] = seqNo & 0xFF;
			buffer[2] = (seqNo >> 8) & 0xFF;
			buffer[3] = timernow & 0xFF;
			buffer[4] = (timernow >> 8) & 0xFF;
			memset(buffer + 5, 1, 80);
			ui8_buf_len = 82;
			lwb_queue_packet(buffer, ui8_buf_len, 0);
			etimer_set(&messageTimer, CLOCK_SECOND * 0.1);
			msgCount = msgCount + 1;
			firstBurst = 1;
			seqNo++;
			send_stream_mod();
		} else {
			send_stream_mod();
			buffer[0] = 0;
			memset(buffer + 1, 1, 81);
			ui8_buf_len = 82;
			printf("add message %u\n",
					lwb_queue_packet(buffer, ui8_buf_len, 0));
			etimer_set(&burstTimer, CLOCK_SECOND * 0.1);
		}
	} else if (data == &messageTimer) {

		if (msgCount < 20) {
			uint16_t timernow = RTIMER_NOW();
				buffer[0] = dataExp;
				buffer[1] = seqNo & 0xFF;
				buffer[2] = (seqNo >> 8) & 0xFF;
				buffer[3] = timernow & 0xFF;
				buffer[4] = (timernow >> 8) & 0xFF;
				memset(buffer + 5, 1, 80);
				ui8_buf_len = 82;
				lwb_queue_packet(buffer, ui8_buf_len, 0);
				etimer_set(&messageTimer, CLOCK_SECOND * 0.1);
				msgCount = msgCount + 1;
				firstBurst = 1;
				seqNo++;
				send_stream_mod();
		} else {
			if (burstCount < 50) {
				uint8_t randomnum = 1 + (uint8_t) rand() % 9;
				msgCount = 0;
				etimer_set(&burstTimer, CLOCK_SECOND * randomnum);
				burstCount++;
			} else {
				buffer[0] = finishExp;
				memset(buffer + 1, 1, 81);
				ui8_buf_len = 82;
				lwb_queue_packet(buffer, ui8_buf_len, 0);
				send_stream_mod();

			}
		}
	}
}

}
PROCESS_END();

return PT_ENDED;
}

PROCESS_THREAD(dewi_app_lamp, ev, data) {
PROCESS_BEGIN()
;

lwb_init(LWB_MODE_SOURCE, &lwb_callbacks);
send_stream_mod();

memb_init(&mmb_results);
list_init(lst_results);

PROCESS_END();

return PT_ENDED;
}
