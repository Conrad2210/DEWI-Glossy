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
#include "common.h"
#include "switch.h"
#include "host.h"
#include "server.h"
#include "lamp.h"



PROCESS(dewi_app_start, "dewi_app_start");
AUTOSTART_PROCESSES(&dewi_app_start);
extern uint16_t node_id;

//--------------------------------------------------------------------------------------------------
//void on_data(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id,
//		int32_t skew) {
////	uint8_t frame = (uint8_t) p_data[0];
////
////	switch (node_id) {
////	case LWB_HOST_ID:
////		break;
////	case LWB_SERVER:
////		switch (frame) {
////		case hello:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////					ui16_from_id);
////			buffer[0] = neighUpdAck;
////			memset(buffer + 1, 1, 81);
////			ui8_buf_len = 82;
////			lwb_queue_packet(buffer, ui8_buf_len, ui16_from_id);
////
////			struct neighStruct Result;
////			Result.id = ui16_from_id;
////			addNeigh(&Result);
////			PROCESS_CONTEXT_BEGIN(&dewi_app_server)
////				;
////				etimer_stop(&neighUpdateFinishTimer);
////				etimer_set(&neighUpdateFinishTimer, CLOCK_SECOND * 10);
////				PROCESS_CONTEXT_END(&dewi_app_server)
////			;
////			break;
////		case finishExp:
////			PROCESS_CONTEXT_BEGIN(&dewi_app_server)
////				;
////				etimer_set(&resultRequestTimer, CLOCK_SECOND * 1);
////				PROCESS_CONTEXT_END(&dewi_app_server)
////			;
////			break;
////		case resultLamp:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////				ui16_from_id);
////			break;
////		case resultSwitch:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////								ui16_from_id);
////			break;
////		}
////		break;
////
////	case LWB_LIGHTSWITCH:
////		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////				ui16_from_id);
////		switch (frame) {
////		case startExp:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////					ui16_from_id);
////			PROCESS_CONTEXT_BEGIN(&dewi_app_lightswitch)
////				;
////				etimer_set(&burstTimer, CLOCK_SECOND);
////				PROCESS_CONTEXT_END(&dewi_app_lightswitch)
////			;
////			break;
////
////		case neighupdate:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////					ui16_from_id);
////			if (!nAck) {
////				buffer[0] = hello;
////				memset(buffer + 1, 1, 81);
////				ui8_buf_len = 82;
////				lwb_queue_packet(buffer, ui8_buf_len, 0);
////			}
////			break;
////		case neighUpdAck:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////					ui16_from_id);
////			nAck = 1;
////			break;
////		case resultReq:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////				ui16_from_id);
////			break;
////		}
////		break;
////	default:
////		switch (frame) {
////		case neighupdate:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////					ui16_from_id);
////			if (!nAck) {
////				buffer[0] = hello;
////				memset(buffer + 1, 1, 81);
////				ui8_buf_len = 82;
////				lwb_queue_packet(buffer, ui8_buf_len, 0);
////			}
////			//void on_schedule_end(void) {
//	////	if (node_id == LWB_HOST_ID)
//	////		lwb_print_stats();
//	//}break;
////		case neighUpdAck:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////					ui16_from_id);
////			nAck = 1;
////			break;
////		case dataExp:
////			//printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////			//		ui16_from_id);
////			rxCounter++;
////			uint16_t myTime = RTIMER_NOW();
////			uint16_t rxSeq = (uint16_t) p_data[1]
////					| ((uint16_t) (p_data[2]) << 8);
////			uint16_t timeCreate = (uint16_t) p_data[3]
////					| ((uint16_t) (p_data[4]) << 8);
////			uint16_t timeSent = (uint16_t) p_data[5]
////					| ((uint16_t) (p_data[6]) << 8);
////			uint16_t timeRx = (uint16_t) p_data[7]
////					| ((uint16_t) (p_data[8]) << 8);
////			uint16_t lat = (timeSent - timeCreate) + (myTime - timeRx);
////			unsigned long avg_rel = rxCounter * 1e5 / (rxSeq + 1);
////			unsigned long latency = (unsigned long) (lat) * 1e6 / RTIMER_SECOND;
////			if (printCounter == 10) {
////				printf(
////						"seq: %u, lat: %u, latency %lu.%03lu ms reliability %3lu.%03lu %%, num Resutsd: %d\n",
////						rxSeq, lat, latency / 1000, latency % 1000,
////						avg_rel / 1000, avg_rel % 1000,
////						list_length(lst_results));
////				printCounter = 0;
////			} else {
////				printCounter++;
////			}
////			struct resultStruct Result;
////			Result.counter = 0;
////			Result.timeslot = 0;
////			getResult(latency / 10000, &Result);
////			Result.timeslot = latency / 10000;
////			Result.counter++;
////			addResult(&Result);
////			break;
////		case resultReq:
////			printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
////				ui16_from_id);
////			break;
////		}
////		break;
////	}
//
//}

//--------------------------------------------------------------------------------------------------
//void on_schedule_end(void) {
////	if (node_id == LWB_HOST_ID)
////		lwb_print_stats();
//}





//void sendMSG(uint8_t type) {
//	buffer[0] = type;
//	memset(buffer + 1, 1, 81);
//	ui8_buf_len = 82;
//	lwb_queue_packet(buffer, ui8_buf_len, 0);
//}

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


