/*
 * switch.c
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

#include "switch.h"
#include "common.h"

PROCESS(dewi_app_lightswitch, "dewi_app_lightswitch");
struct etimer neighUpdateTimer, neighUpdateFinishTimer, burstTimer,
		messageTimer, expFinishTimer, resultRequestTimer;
struct etimer resultReplyTimer;

extern uint16_t node_id;
uint16_t msgCount = 0, burstCount = 0;
uint8_t nAck = 0, rAck = 0, firstBurst = 0;
uint16_t seqNo = 0;

uint8_t lastMSG;

void on_data_switch(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id,
		int32_t skew) {
	uint8_t frame = (uint8_t) p_data[0];

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

	case neighupdate:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);
		if (!nAck) {
			send_stream_mod();
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
	case resultReq:
		printf("nodeid: %u received frame %u, from: %u\n", node_id, frame,
				ui16_from_id);
		PROCESS_CONTEXT_BEGIN(&dewi_app_lightswitch)
			;
			etimer_set(&resultReplyTimer, CLOCK_SECOND * 0.1);
			PROCESS_CONTEXT_END(&dewi_app_lightswitch)
		;

		break;
	case reset:
		msgCount = 0, burstCount = 0;
		nAck = 0, rAck = 0, firstBurst = 0;
		seqNo = 0;
		break;
	}

}
void on_schedule_end_switch(void) {
}
lwb_callbacks_t lwb_callbacks_switch =
		{ on_data_switch, on_schedule_end_switch };

void sendMSG(uint8_t type) {
	buffer[0] = type;
	memset(buffer + 1, 1, 81);
	ui8_buf_len = 82;
	lwb_queue_packet(buffer, ui8_buf_len, 0);
}

PROCESS_THREAD(dewi_app_lightswitch, ev, data) {
	PROCESS_BEGIN()
		;
		printf("start dewi_app_lightswitch\n");
		lwb_init(LWB_MODE_SOURCE, &lwb_callbacks_switch);

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
						if (burstCount < 1) {
							uint8_t randomnum = 1 + (uint8_t) rand() % 9;
							msgCount = 0;
							etimer_set(&burstTimer, CLOCK_SECOND * randomnum);
							burstCount++;
						} else {

							etimer_set(&expFinishTimer, CLOCK_SECOND * 10);

						}
					}
				} else if (data == &expFinishTimer) {
					send_stream_mod();
					printf("experiment finsihes\n");
					if (lwb_get_n_my_slots() != 0) {
						buffer[0] = finishExp;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len,0);
					} else {
						buffer[0] = 0;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len, 0);
						etimer_set(&expFinishTimer, CLOCK_SECOND * 1);
					}

				}else if (data == &resultReplyTimer) {
					send_stream_mod();
					printf("experiment finsihes\n");
					if (lwb_get_n_my_slots() != 0) {
						buffer[0] = resultSwitch;
						buffer[1] = seqNo & 0xFF;
						buffer[2] = (seqNo >> 8) & 0xFF;
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len, 3);
					} else {
						buffer[0] = 0;
						memset(buffer + 1, 1, 81);
						ui8_buf_len = 82;
						lwb_queue_packet(buffer, ui8_buf_len, 0);
						etimer_set(&resultReplyTimer, CLOCK_SECOND * 1);
					}

				}
			}

		}
	PROCESS_END();

return PT_ENDED;
}
