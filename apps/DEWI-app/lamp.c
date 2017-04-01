/*
 * lamp.c
 *
 *  Created on: Apr 1, 2017
 *      Author: user
 */

#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "lwb.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/random.h"

#include "lamp.h"
#include "common.h"

PROCESS(dewi_app_lamp, "dewi_app_lamp");
extern uint16_t node_id;
uint8_t printCounter = 0;
uint8_t lamp_nAck = 0, lamp_rAck = 0, lamp_rxCounter = 0;

uint8_t resultCounter = 0, resultSentCounter = 0;

uint8_t timeslotResult[200] = {0};
uint16_t countResult[200] = {0};
struct etimer resultReplyTimer;

void getResult(uint16_t timeslot, struct resultStruct *res) {
	struct resultStruct *n;
	n = NULL;
	for (n = list_head(lst_results); n != NULL; n = list_item_next(n)) {

		if (n->timeslot == timeslot) {
			break;
		}
	}

	if (n != NULL) {
		res->timeslot = n->timeslot;
		res->counter = n->counter;
	}
}

uint8_t addResult(struct resultStruct *res) {
	struct resultStruct *newRes;
	newRes = NULL;
	uint8_t isNewRes = 1;
	for (newRes = list_head(lst_results); newRes != NULL; newRes =
			list_item_next(newRes)) {

		/* We break out of the loop if the address of the neighbor matches
		 the address of the neighbor from which we received this
		 broadcast message. */
		if (newRes->timeslot == res->timeslot) {

			break;
		}
	}

	if (newRes == NULL) {
		newRes = memb_alloc(&mmb_results);

		/* If we could not allocate a new neighbor entry, we give up. We
		 could have reused an old neighbor entry, but we do not do this
		 for now. */
		if (newRes == NULL) {
			return 0;
		}
	}
	newRes->counter = res->counter;
	newRes->timeslot = res->timeslot;
	list_add(lst_results, newRes);
	return isNewRes;
}

uint16_t getAllResults(uint8_t timeslot[], uint16_t values[]) {
	struct resultStruct *newRes;
	uint8_t counter = 0;

	for (newRes = list_head(lst_results); newRes != NULL; newRes =
			list_item_next(newRes)) {
		timeslot[counter] = newRes->timeslot;
		values[counter] = newRes->counter;
		counter++;
	}
	return counter;
}

void clearResults() {
	while (list_head(lst_results) != NULL) {
		memb_free(&mmb_results, list_head(lst_results));
		list_remove(lst_results, list_head(lst_results));
	}
}

void on_data_lamp(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id,
		int32_t skew) {
	uint8_t frame = (uint8_t) p_data[0];

	switch (frame) {
	case neighupdate:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);
		if (!lamp_nAck) {
			send_stream_mod();
			buffer[0] = hello;
			memset(buffer + 1, 1, 81);
			ui8_buf_len = 82;
			lwb_queue_packet(buffer, ui8_buf_len, 0);
		}
		//void on_schedule_end(void) {
		//	if (node_id == LWB_HOST_ID)
		//		lwb_print_stats();

		break;
	case neighUpdAck:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);
		lamp_nAck = 1;
		break;
	case dataExp:
		//printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
		//		ui16_from_id);
		lamp_rxCounter++;
		uint16_t myTime = RTIMER_NOW();
		uint16_t rxSeq = (uint16_t) p_data[1] | ((uint16_t) (p_data[2]) << 8);
		uint16_t timeCreate = (uint16_t) p_data[3]
				| ((uint16_t) (p_data[4]) << 8);
		uint16_t timeSent = (uint16_t) p_data[5]
				| ((uint16_t) (p_data[6]) << 8);
		uint16_t timeRx = (uint16_t) p_data[7] | ((uint16_t) (p_data[8]) << 8);
		uint16_t lat = (timeSent - timeCreate) + (myTime - timeRx);
		unsigned long avg_rel = lamp_rxCounter * 1e5 / (rxSeq + 1);
		unsigned long latency = (unsigned long) (lat) * 1e6 / RTIMER_SECOND;
//		if (printCounter == 20) {
//			printf(
//					"seq: %u, lat: %u, latency %lu.%03lu ms reliability %3lu.%03lu %%, num Resutsd: %d\n",
//					rxSeq, lat, latency / 1000, latency % 1000, avg_rel / 1000,
//					avg_rel % 1000, list_length(lst_results));
//			printCounter = 0;
//		} else {
//			printCounter++;
//		}
		struct resultStruct Result;
		Result.counter = 0;
		Result.timeslot = 0;
		getResult(latency / 10000, &Result);
		Result.timeslot = latency / 10000;
		Result.counter++;
		addResult(&Result);
		break;
	case resultReq:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);
		resultCounter = getAllResults(&timeslotResult, &countResult);

		PROCESS_CONTEXT_BEGIN(&dewi_app_lamp)
			;
			etimer_set(&resultReplyTimer, CLOCK_SECOND * 0.1);
			PROCESS_CONTEXT_END(&dewi_app_lamp)
		;

		break;
	case reset:
		printCounter = 0;
		lamp_nAck = 0, lamp_rAck = 0, lamp_rxCounter = 0;

		resultCounter = 0, resultSentCounter = 0;
		clearResults();
		break;

	}
}

void on_schedule_end_lamp(void) {
}
lwb_callbacks_t lwb_callbacks_lamp = { on_data_lamp, on_schedule_end_lamp };

PROCESS_THREAD(dewi_app_lamp, ev, data) {
	PROCESS_BEGIN()
		;

		lwb_init(LWB_MODE_SOURCE, &lwb_callbacks_lamp);
		send_stream_mod();

		memb_init(&mmb_results);
		list_init(lst_results);

		while (1) {
			PROCESS_YIELD()
			;
			if (ev == PROCESS_EVENT_TIMER) {
				if (data == &resultReplyTimer) {
					send_stream_mod();
					if (lwb_get_n_my_slots() != 0) {
						send_stream_mod();
						buffer[0] = resultLamp;
						buffer[1] = resultCounter;
						buffer[2] = lamp_rxCounter & 0xFF;
						buffer[3] = (lamp_rxCounter >> 8) & 0xFF;
						uint8_t i = resultSentCounter, i_end = i + 1, k =
								5;

						if(i_end > resultCounter)
							i_end = resultCounter;
						for (; i < i_end; i++) {
							buffer[k] = timeslotResult[i];
							buffer[k + 1] = countResult[i] & 0xff;
							buffer[k + 2] = (countResult[i] >> 8) & 0xff;
							k = k + 3;
							resultSentCounter++;
						}
						if(i_end < resultCounter)
							buffer[3] = 1;
						else
							buffer[3] = 0;
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len, 3);
						printf("resultSentCounter %u < resultCounter %u \n",resultSentCounter,resultCounter);
						if(resultSentCounter < resultCounter)
							etimer_set(&resultReplyTimer, CLOCK_SECOND * 1);
					} else {
						send_stream_mod();
						buffer[0] = 0;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						printf("add message %u\n",
								lwb_queue_packet(buffer, ui8_buf_len, 0));
						etimer_set(&resultReplyTimer, CLOCK_SECOND * 1);
					}
				}
			}
		}

	PROCESS_END();

return PT_ENDED;
}
