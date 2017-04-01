/*
 * server.c
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

#include "server.h"
#include "common.h"

PROCESS(dewi_app_server, "dewi_app_lightswitch");
struct etimer neighUpdateTimer, neighUpdateFinishTimer, burstTimer,
		messageTimer, checkSlotsTimer, resultRequestTimer;
struct etimer resetTimer;
uint8_t resetCounter = 0;

extern uint16_t node_id;
uint8_t server_lastMSG = 0;
uint16_t server_NeighboursArra[50] = { 0 };
uint8_t server_neighCounter = 0;
uint8_t server_resultSendCounter = 0;
uint8_t server_resultsReceived = 0;

uint8_t addNeigh(struct neighStruct *res) {
	struct neighStruct *newRes;
	newRes = NULL;
	uint8_t isNewRes = 1;
	for (newRes = list_head(lst_neigh); newRes != NULL;
			newRes = list_item_next(newRes)) {

		/* We break out of the loop if the address of the neighbor matches
		 the address of the neighbor from which we received this
		 broadcast message. */
		if (newRes->id == res->id) {

			break;
		}
	}

	if (newRes == NULL) {
		newRes = memb_alloc(&mmb_neigh);

		/* If we could not allocate a new neighbor entry, we give up. We
		 could have reused an old neighbor entry, but we do not do this
		 for now. */
		if (newRes == NULL) {
			return 0;
		}
	}
	newRes->id = res->id;
	list_add(lst_neigh, newRes);
	return isNewRes;
}

uint16_t getAllNeigh(uint16_t neigh[]) {
	struct neighStruct *newRes;
	uint8_t counter = 0;

	for (newRes = list_head(lst_neigh); newRes != NULL;
			newRes = list_item_next(newRes)) {
		neigh[counter] = newRes->id;
		counter++;
	}
	return counter;
}

void clearNeigh() {
	while (list_head(lst_neigh) != NULL) {
		memb_free(&mmb_neigh, list_head(lst_neigh));
		list_remove(lst_neigh, list_head(lst_neigh));
	}
}

void on_data_server(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id,
		int32_t skew) {
	uint8_t frame = (uint8_t) p_data[0];
	uint16_t txResults = 0;
	uint8_t i = 0;
	uint8_t k = 5;
	switch (frame) {
	case hello:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);
		buffer[0] = neighUpdAck;
		memset(buffer + 1, 1, 81);
		ui8_buf_len = 82;
		lwb_queue_packet(buffer, ui8_buf_len, ui16_from_id);

		struct neighStruct Result;
		Result.id = ui16_from_id;
		addNeigh(&Result);
		PROCESS_CONTEXT_BEGIN(&dewi_app_server)
			;
			etimer_stop(&neighUpdateFinishTimer);
			etimer_set(&neighUpdateFinishTimer, CLOCK_SECOND * 10);
			PROCESS_CONTEXT_END(&dewi_app_server)
		;
		break;
	case finishExp:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);

		server_neighCounter = getAllNeigh(&server_NeighboursArra);
		PROCESS_CONTEXT_BEGIN(&dewi_app_server)
			;
			etimer_set(&resultRequestTimer, CLOCK_SECOND * 1);
			PROCESS_CONTEXT_END(&dewi_app_server)
		;
		break;
	case resultLamp:
		//printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,ui16_from_id);

//		printf("nodeid: %u received frame %u, from: %u moreMSG %u\n", node_id,
//				frame, ui16_from_id, (uint8_t) p_data[3]);

		for (; i < 1; i++) {
			printf("id: %u, timeslot: %u, count: %u\n", ui16_from_id,
					(uint8_t) p_data[k],
					(uint16_t) p_data[k + 1]
							| ((uint16_t) (p_data[k + 2]) << 8));
			k = k + 3;
		}

		if (!(uint8_t) p_data[3]) {
			printf("id: %u, rx: %u\n", ui16_from_id,
					(uint16_t) p_data[2] | ((uint16_t) (p_data[3]) << 8));
			server_resultSendCounter++;
			if (server_resultSendCounter < server_neighCounter) {
				PROCESS_CONTEXT_BEGIN(&dewi_app_server)
					;

					etimer_set(&resultRequestTimer, CLOCK_SECOND * 0.1);
					PROCESS_CONTEXT_END(&dewi_app_server);
			}
		}

		break;
	case resultSwitch:
		txResults = (uint16_t) p_data[1] | ((uint16_t) (p_data[2]) << 8);
		printf("id: 2, tx: %u\n", txResults);
		server_resultSendCounter++;
		if (server_resultSendCounter < server_neighCounter) {
			PROCESS_CONTEXT_BEGIN(&dewi_app_server)
				;

				etimer_set(&resultRequestTimer, CLOCK_SECOND * 0.1);
				PROCESS_CONTEXT_END(&dewi_app_server);
		}
		break;
	}

}
void on_schedule_end_server(void) {
}
lwb_callbacks_t lwb_callbacks_server =
		{ on_data_server, on_schedule_end_server };

PROCESS_THREAD(dewi_app_server, ev, data) {
	PROCESS_BEGIN()
		;

		lwb_init(LWB_MODE_SOURCE, &lwb_callbacks_server);
		printf("start server app\n");

		send_stream_mod();
		memb_init(&mmb_results);
		list_init(lst_results);
		etimer_set(&resetTimer, CLOCK_SECOND);
		while (1) {
			PROCESS_YIELD()
			;
			if (ev == PROCESS_EVENT_TIMER) {
				if (data == &neighUpdateTimer) {
					send_stream_mod();
					if (lwb_get_joining_state() == LWB_JOINING_STATE_JOINED) {
						buffer[0] = neighupdate;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len, 0);
						etimer_set(&neighUpdateTimer, CLOCK_SECOND);
					} else {
						etimer_set(&neighUpdateTimer, CLOCK_SECOND * 5);
					}
				} else if (data == &resetTimer) {
					send_stream_mod();
					if (resetCounter < 30) {
						printf("reset %u\n",resetCounter);
						buffer[0] = reset;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len, 0);
						etimer_set(&resetTimer, CLOCK_SECOND);
						resetCounter++;
					}
					else
					{
						printf("start topology\n");
						etimer_set(&neighUpdateTimer, CLOCK_SECOND * 2);
						etimer_set(&neighUpdateFinishTimer, CLOCK_SECOND * 60);
					}
				} else if (data == &neighUpdateFinishTimer) {
					send_stream_mod();
					printf("topology finished\n");
					etimer_stop(&neighUpdateTimer);
					buffer[0] = startExp;
					memset(buffer + 1, 1, 81);
					ui8_buf_len = 82;
					lwb_queue_packet(buffer, ui8_buf_len, 0);
				} else if (data == &resultRequestTimer) {
					send_stream_mod();
					if (lwb_get_n_my_slots() != 0) {
						buffer[0] = resultReq;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len,
								server_NeighboursArra[server_resultSendCounter]);
					} else {
						buffer[0] = 0;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len, 0);
						etimer_set(&resultRequestTimer, CLOCK_SECOND * 1);
					}

				}
			}
		}

	PROCESS_END();

return PT_ENDED;
}
