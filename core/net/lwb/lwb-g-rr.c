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
 * Following implementation is based on the implementation used in
 * http://dl.acm.org/citation.cfm?id=2426658
 *
 */

#include <string.h>

#include "contiki.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/random.h"

#include "glossy.h"

#include "lwb.h"
#include "lwb-common.h"
#include "lwb-macros.h"
#include "lwb-g-sync.h"
#include "lwb-g-rr.h"
#include "lwb-scheduler.h"
#include "lwb-sched-compressor.h"

#if LWB_HSLP
#include "lwb-hslp.h"
#endif // LWB_HSLP

extern uint16_t node_id;

extern lwb_context_t lwb_context;

extern void lwb_save_ctrl_energest();
extern void lwb_update_ctrl_energest();

PROCESS_NAME(lwb_main_process);

/// @brief We use the same proto thread for both host and source since only one mode is active at a time.
static struct pt pt_lwb_g_rr;
/// @brief Iterator for slot index
static uint8_t ui8_slot_idx = 0;
/// @brief Glossy header (Packet type)
static uint8_t ui8_g_header = 0;

/// @brief Buffers for TX and RX
MEMB(mmb_data_buf, data_buf_lst_item_t, LWB_MAX_DATA_BUF_ELEMENTS);
/// @brief Transmit data buffer element list
LIST(lst_tx_buf_queue);
/// @brief Receive data buffer element list
LIST(lst_rx_buf_queue);
/// @brief Number of data packets to be sent over LWB
static uint8_t ui8_tx_buf_q_size = 0;
/// @brief Number of data packets to be delivered to the application layer
static uint8_t ui8_rx_buf_q_size = 0;

/// @brief Memory block allocation space for stream requests
MEMB(mmb_stream_req, stream_req_lst_item_t, LWB_MAX_STREAM_REQ_ELEMENTS);
/// @brief List for stream requests to be sent
LIST(lst_stream_req);
/// @brief Number of stream requests to be sent
static volatile uint8_t ui8_stream_reqs_lst_size = 0;
/// @brief The ID of the next stream request
static volatile uint8_t ui8_stream_id_next = 1;

static volatile uint8_t ui8_n_rounds_to_wait = 0;

static volatile uint8_t ui8_n_trials = 0;

#if LWB_HSLP
static volatile uint8_t ui8_hslp_pkt_type = 0;
static volatile rtimer_clock_t t_hslp_stop = 0;
#endif // LWB_HSLP

//--------------------------------------------------------------------------------------------------
void lwb_g_rr_init() {
	PT_INIT(&pt_lwb_g_rr);

	memb_init(&mmb_data_buf);
	list_init(lst_tx_buf_queue);
	list_init(lst_rx_buf_queue);
	ui8_tx_buf_q_size = 0;
	ui8_rx_buf_q_size = 0;

	memb_init(&mmb_stream_req);
	list_init(lst_stream_req);
	ui8_stream_reqs_lst_size = 0;

#if LWB_HSLP
	lwb_hslp_init();
#endif // LWB_HSLP
}

//--------------------------------------------------------------------------------------------------
static void prepare_data_packet() {

	data_buf_lst_item_t* p_buf_item = list_head(lst_tx_buf_queue);
	uint8_t ui8_n_possible = 0;
	stream_req_lst_item_t* p_req_item;
	uint8_t ui8_c;

	if (p_buf_item
			&& (LWB_MAX_TXRX_BUF_LEN
					> p_buf_item->buf.header.data_len + sizeof(data_header_t))) {
		// We have enough space on the buffer to send data. This is just a sanity check.

		// Set number of packets in the buffer that are ready to be transmitted
		p_buf_item->buf.header.in_queue = ui8_tx_buf_q_size;
		uint16_t type;
		memcpy(&type, p_buf_item->buf.data, 1);
		if (type == 0x03) {
			uint16_t timeNow = RTIMER_NOW();
			uint8_t first8 = timeNow & 0xFF, second8 = (timeNow >> 8) & 0xFF;
			memcpy(p_buf_item->buf.data + 5, &first8, 1);
			memcpy(p_buf_item->buf.data + 6, &second8, 1);
		}
		// First, we copy the data. The data header will be copied later at the end.
		// We may need to update options of the data header if we have stream requests to be sent.
		memcpy(lwb_context.txrx_buf + sizeof(data_header_t),
				p_buf_item->buf.data, p_buf_item->buf.header.data_len);
		lwb_context.txrx_buf_len = sizeof(data_header_t)
				+ p_buf_item->buf.header.data_len;

		if (lwb_context.lwb_mode == LWB_MODE_SOURCE
				&& ui8_stream_reqs_lst_size > 0) {
			// We have some stream requests pending to be sent. Let's try to include them in the data packet
			ui8_n_possible = (LWB_MAX_TXRX_BUF_LEN - lwb_context.txrx_buf_len)
					/ sizeof(lwb_stream_req_t);

			// Iterate through all stream requests and try to include them into one packet.
			// Here, we do not remove any of the stream requests from the list as they may need to be resent
			// in a later time if no acknowledgements are received.
			for (p_req_item = list_head(lst_stream_req), ui8_c = 0;
					p_req_item != NULL && ui8_c < ui8_n_possible; p_req_item =
							p_req_item->next, ui8_c++) {

				memcpy(lwb_context.txrx_buf + lwb_context.txrx_buf_len,
						&p_req_item->req, sizeof(lwb_stream_req_t));

				lwb_context.txrx_buf_len += sizeof(lwb_stream_req_t);
			}

			p_buf_item->buf.header.options = LWB_PKT_TYPE_STREAM_REQ
					| ui8_c << 4;

			// Update the stats
			LWB_STATS_STREAM_REQ_ACK(n_req_tx) += ui8_c;
		}

		// Now we copy the header
		memcpy(lwb_context.txrx_buf, &p_buf_item->buf.header,
				sizeof(data_header_t));

		list_remove(lst_tx_buf_queue, p_buf_item);
		memb_free(&mmb_data_buf, p_buf_item);
		ui8_tx_buf_q_size--;
		LWB_STATS_DATA(n_tx)++;

	} else {
		// We don't have enough space in the buffer
	}

}

//--------------------------------------------------------------------------------------------------
static void prepare_stream_acks() {

	lwb_context.txrx_buf[0] = lwb_context.n_stream_acks;
	memcpy(lwb_context.txrx_buf + 1, lwb_context.ui16arr_stream_akcs,
			2 * lwb_context.n_stream_acks);
	lwb_context.txrx_buf_len = 1 + 2 * lwb_context.n_stream_acks;

	LWB_STATS_STREAM_REQ_ACK(n_ack_tx) += lwb_context.n_stream_acks;

	lwb_context.n_stream_acks = 0;
}

//--------------------------------------------------------------------------------------------------
static void process_stream_acks() {

	uint8_t ui8_n_acks = lwb_context.txrx_buf[0]; // first byte is the number of acknowledgements
	uint16_t ui16_ack_node_id;
	stream_req_lst_item_t* p_req_item;

	// Just validate the stream acknowledgements data
	if (lwb_context.txrx_buf_len < 1 + (ui8_n_acks * 2)) {
		return;
	}

	LWB_STATS_STREAM_REQ_ACK(n_ack_rx) += ui8_n_acks;

	// iterate through all stream acknowledgements. There are acknowledgements for us, we remove
	// stored stream requests from the front of the list.
	for (; ui8_n_acks > 0; ui8_n_acks--) {

		ui16_ack_node_id = (uint16_t) lwb_context.txrx_buf[ui8_n_acks * 2] << 8
				| (uint16_t) lwb_context.txrx_buf[ui8_n_acks * 2 - 1];

		if (ui16_ack_node_id == node_id
				&& (p_req_item = list_head(lst_stream_req))) {

			list_remove(lst_stream_req, p_req_item);
			memb_free(&mmb_stream_req, p_req_item);
			ui8_stream_reqs_lst_size--;
		}
	}

	if (ui8_stream_reqs_lst_size == 0) {
		/// @todo Hooray..! we are joined
		lwb_context.joining_state = LWB_JOINING_STATE_JOINED;
	}
}

//--------------------------------------------------------------------------------------------------
static void process_stream_reqs() {

	lwb_stream_req_header_t header;
	lwb_stream_req_t stream_req;
	uint8_t ui8_i;

	if (lwb_context.txrx_buf_len < sizeof(lwb_stream_req_header_t)) {
		return;
	}

	memcpy(&header, lwb_context.txrx_buf, sizeof(lwb_stream_req_header_t));

	if (lwb_context.txrx_buf_len
			< (sizeof(lwb_stream_req_header_t)
					+ (header.n_reqs * sizeof(lwb_stream_req_t)))) {
		return;
	}

	for (ui8_i = 0; ui8_i < lwb_context.n_stream_acks; ui8_i++) {
		if (header.from_id == lwb_context.ui16arr_stream_akcs[ui8_i]) {
			// We already processed a stream request from this node in this round
			return;
		}
	}

	if (lwb_context.n_stream_acks >= LWB_SCHED_MAX_SLOTS) {
		// Maximum space for stream acks exceeded.
		// We just ignore the stream requests.
		return;
	}

	// Add the node id for sending acknowledgements.
	lwb_context.ui16arr_stream_akcs[lwb_context.n_stream_acks++] =
			header.from_id;

	for (ui8_i = 0; ui8_i < header.n_reqs; ui8_i++) {
		memcpy(&stream_req,
				lwb_context.txrx_buf + sizeof(lwb_stream_req_header_t)
						+ ui8_i * sizeof(lwb_stream_req_t),
				sizeof(lwb_stream_req_t));
		lwb_sched_process_stream_req(header.from_id, &stream_req);
	}

	LWB_STATS_STREAM_REQ_ACK(n_req_rx) += ui8_i;
}

//--------------------------------------------------------------------------------------------------
static void prepare_stream_reqs() {

	uint8_t ui8_n_possible = (LWB_MAX_TXRX_BUF_LEN
			- sizeof(lwb_stream_req_header_t)) / sizeof(lwb_stream_req_t);
	lwb_stream_req_header_t header;
	stream_req_lst_item_t* p_req_item;
	uint8_t ui8_c;

	lwb_context.txrx_buf_len = sizeof(lwb_stream_req_header_t);

	// Iterate through all stream requests and try to include them into one packet.
	// Here, we do not remove any of the stream requests from the list as they may need to be resent
	// in a later time if no acknowledgements are received.
	for (p_req_item = list_head(lst_stream_req), ui8_c = 0;
			p_req_item != NULL && ui8_c < ui8_n_possible; p_req_item =
					p_req_item->next, ui8_c++) {

		memcpy(lwb_context.txrx_buf + lwb_context.txrx_buf_len,
				&p_req_item->req, sizeof(lwb_stream_req_t));

		lwb_context.txrx_buf_len += sizeof(lwb_stream_req_t);
	}

	// Update the stats
	LWB_STATS_STREAM_REQ_ACK(n_req_tx) += ui8_c;

	header.from_id = node_id;
	header.n_reqs = ui8_c;
	memcpy(lwb_context.txrx_buf, &header, sizeof(lwb_stream_req_header_t));
}

//--------------------------------------------------------------------------------------------------
static void process_data_packet(uint8_t slot_idx) {

	// Just a sanity check
	if (lwb_context.txrx_buf_len < sizeof(data_header_t)) {
		return;
	}

	data_buf_lst_item_t* p_item = memb_alloc(&mmb_data_buf);

	if (!p_item) {
		LWB_STATS_DATA(n_rx_nospace)++;
		return;
	}

	LWB_STATS_DATA(n_rx)++;

	memcpy(&p_item->buf.header, lwb_context.txrx_buf, sizeof(data_header_t));

	if (p_item->buf.header.to_id != node_id && p_item->buf.header.to_id != 0) {
		// We drop this packet
		memb_free(&mmb_data_buf, p_item);
		LWB_STATS_DATA(n_rx_dropped)++;
		return;
	}

	// Copy the data into the buffer and add to the queue
	memcpy(p_item->buf.data, lwb_context.txrx_buf + sizeof(data_header_t),
			p_item->buf.header.data_len);
	uint16_t type;
	memcpy(&type, p_item->buf.data, 1);
	if (type == 0x03) {
		uint16_t timeRx = GLOSSY_T_REF + T_RR_ON * slot_idx;
		uint8_t first8 = timeRx & 0xFF, second8 = (timeRx >> 8) & 0xFF;
		memcpy(p_item->buf.data + 7, &first8, 1);
		memcpy(p_item->buf.data + 8, &second8, 1);
	}
		list_add(lst_rx_buf_queue, p_item);
		ui8_rx_buf_q_size++;

	// Poll the LWB main process to deliver data to APP layer
	LWB_SET_POLL_FLAG(LWB_POLL_FLAGS_DATA);
	process_poll(&lwb_main_process);

	// This part is only for the host node
	if (lwb_context.lwb_mode == LWB_MODE_HOST) {

		lwb_stream_req_t stream_req;

		lwb_sched_update_qlen(slot_idx, p_item->buf.header.in_queue);

		if ((p_item->buf.header.options & 0x0f) == LWB_PKT_TYPE_STREAM_REQ) {
			// There are some stream requests in the data packet
			uint8_t ui8_n_reqs = p_item->buf.header.options >> 4; // extract number of requests
			uint8_t ui8_i = 0;

			for (; ui8_i < ui8_n_reqs; ui8_i++) {

				memcpy(&stream_req,
						lwb_context.txrx_buf + sizeof(data_header_t)
								+ p_item->buf.header.data_len
								+ ui8_i * sizeof(lwb_stream_req_t),
						sizeof(lwb_stream_req_t));
				lwb_sched_process_stream_req(p_item->buf.header.from_id,
						&stream_req);
			}
		}
	}
}

#if LWB_HSLP
//--------------------------------------------------------------------------------------------------
inline uint8_t lwb_g_rr_hslp_queue_app_data(uint8_t* p_data, uint8_t ui8_len) {

	// Just a sanity check
	if (ui8_len < sizeof(data_header_t)) {
		return 0;
	}

	data_buf_lst_item_t* p_item = memb_alloc(&mmb_data_buf);

	if(!p_item) {
		return 0;
	}

	if (LWB_MAX_TXRX_BUF_LEN > ui8_len) {

		memcpy(&p_item->buf.header, p_data, sizeof(data_header_t));

		p_item->buf.header.from_id = node_id; // always replace from ID
		p_item->buf.header.options = 0;// No options are used
		memcpy(p_item->buf.data, p_data + sizeof(data_header_t), p_item->buf.header.data_len);
		list_add(lst_tx_buf_queue, p_item);
		ui8_tx_buf_q_size++;
		return 1;
	} else {
		memb_free(&mmb_data_buf, p_item);
	}

	LWB_STATS_DATA(ui16_n_tx_nospace)++;

	return 0;
}

//--------------------------------------------------------------------------------------------------
inline uint8_t lwb_g_rr_hslp_copy_schedule(uint8_t* p_data, uint8_t ui8_len) {

	// Just a sanity check
	if (ui8_len < sizeof(lwb_sched_info_t)) {
		return 0;
	}

	// First copy the schedule information
	memcpy(&CURRENT_SCHEDULE_INFO(), p_data, sizeof(lwb_sched_info_t));

	uint8_t ui8_data_slots = GET_N_DATA_SLOTS(CURRENT_SCHEDULE_INFO().n_slots);

	// Validate the size of the received schedule
	if (ui8_len != (sizeof(lwb_sched_info_t) + (2 * ui8_data_slots))) {
		return 0;
	}

	// Just a validation check to avoid buffer overflows
	if (ui8_data_slots > LWB_SCHED_MAX_SLOTS) {
		return 0;
	}

	// Everything seem to be fine. So we copy the schedule slots
	memcpy(CURRENT_SCHEDULE().slots, p_data + sizeof(lwb_sched_info_t), ui8_data_slots * 2);

	return 1;
}

//--------------------------------------------------------------------------------------------------
inline void lwb_g_rr_hslp_send_app_data() {
	// We dump received data to the external device
	LWB_HSLP_SET_PKT_MAIN_TYPE(LHSLP_PKT_TYPE_LWB_DATA, ui8_hslp_pkt_type);
	LWB_HSLP_SET_PKT_SUB_TYPE(LHSLP_PKT_SUB_TYPE_APP_DATA, ui8_hslp_pkt_type);
	lwb_hslp_send(ui8_hslp_pkt_type, lwb_context.txrx_buf, lwb_context.txrx_buf_len);
}

//--------------------------------------------------------------------------------------------------
inline void lwb_g_rr_hslp_receive_app_data() {

	// Calculate the time when HSLP operation should be finished
	t_hslp_stop = RTIMER_NOW() + T_HSLP_APP_DATA;
	// Send a request to get application data from the external device
	LWB_HSLP_SET_PKT_MAIN_TYPE(LHSLP_PKT_TYPE_LWB_DATA_REQ, ui8_hslp_pkt_type);
	LWB_HSLP_SET_PKT_SUB_TYPE(LHSLP_PKT_SUB_TYPE_APP_DATA, ui8_hslp_pkt_type);
	lwb_hslp_send(ui8_hslp_pkt_type, lwb_context.txrx_buf, 0);

	// Read a packet from external device if available
	if (lwb_hslp_read(t_hslp_stop)) {

		ui8_hslp_pkt_type = lwb_hslp_get_packet_type();

		if ((LWB_HSLP_GET_PKT_MAIN_TYPE(ui8_hslp_pkt_type) == LHSLP_PKT_TYPE_LWB_DATA) &&
				(LWB_HSLP_GET_PKT_SUB_TYPE(ui8_hslp_pkt_type) == LHSLP_PKT_SUB_TYPE_APP_DATA)) {
			// We have an application data packet from external device
			lwb_g_rr_hslp_queue_app_data(lwb_hslp_get_data(), lwb_hslp_get_data_len());

		} else {
			// Some other HSLP packet
		}
	} else {
		// No data read or timeout
	}
}

//--------------------------------------------------------------------------------------------------
inline void lwb_g_rr_hslp_send_receive_schedule() {

	// Calculate the time when HSLP operation should be finished
	t_hslp_stop = RTIMER_NOW() + T_HSLP_SCHED;
	// We send this to the external device
	LWB_HSLP_SET_PKT_MAIN_TYPE(LHSLP_PKT_TYPE_LWB_DATA, ui8_hslp_pkt_type);
	LWB_HSLP_SET_PKT_SUB_TYPE(LHSLP_PKT_SUB_TYPE_SCHED, ui8_hslp_pkt_type);
	lwb_hslp_send(ui8_hslp_pkt_type, (uint8_t*)&CURRENT_SCHEDULE(),
			sizeof(lwb_sched_info_t) + (N_CURRENT_DATA_SLOTS() * 2));
	// Send a request for the new schedule
	LWB_HSLP_SET_PKT_MAIN_TYPE(LHSLP_PKT_TYPE_LWB_DATA_REQ, ui8_hslp_pkt_type);
	LWB_HSLP_SET_PKT_SUB_TYPE(LHSLP_PKT_SUB_TYPE_SCHED, ui8_hslp_pkt_type);
	lwb_hslp_send(ui8_hslp_pkt_type, (uint8_t*)&CURRENT_SCHEDULE_INFO(), 0);

	// Read a packet from external device if available
	if (lwb_hslp_read(t_hslp_stop)) {
		// We have read HSLP packet successfully
		ui8_hslp_pkt_type = lwb_hslp_get_packet_type();

		if ((LWB_HSLP_GET_PKT_MAIN_TYPE(ui8_hslp_pkt_type) == LHSLP_PKT_TYPE_LWB_DATA) &&
				(LWB_HSLP_GET_PKT_SUB_TYPE(ui8_hslp_pkt_type) == LHSLP_PKT_SUB_TYPE_SCHED)) {
			// We have a schedule packet from external device.
			if (!lwb_g_rr_hslp_copy_schedule(lwb_hslp_get_data(), lwb_hslp_get_data_len())) {
				// Oops..! error in copying the schedule. So we copy the old schedule to current.
				memcpy(&CURRENT_SCHEDULE(), &OLD_SCHEDULE(), sizeof(lwb_schedule_t));
			} else {
				// We successfully received the schedule from external device.
				// So do nothing in here.
			}

		} else {
			// Some other HSLP packet. So we copy the old schedule to current.
			memcpy(&CURRENT_SCHEDULE(), &OLD_SCHEDULE(), sizeof(lwb_schedule_t));
		}
	} else {
		///< @todo Add stats about schedule timeouts
		// No data read or timeout. So we copy the old schedule to current.
		memcpy(&CURRENT_SCHEDULE(), &OLD_SCHEDULE(), sizeof(lwb_schedule_t));
	}
}

//--------------------------------------------------------------------------------------------------
inline void lwb_g_rr_hslp_send_stream_reqs() {
	// HSLP is enabled. We dump received stream requests over the serial
	LWB_HSLP_SET_PKT_MAIN_TYPE(LHSLP_PKT_TYPE_LWB_DATA, ui8_hslp_pkt_type);
	LWB_HSLP_SET_PKT_SUB_TYPE(LHSLP_PKT_SUB_TYPE_STREAM_REQ, ui8_hslp_pkt_type);
	lwb_hslp_send(ui8_hslp_pkt_type, lwb_context.txrx_buf, lwb_context.txrx_buf_len);
}

#endif // LWB_HSLP

//--------------------------------------------------------------------------------------------------
PT_THREAD(lwb_g_rr_host(struct rtimer *t, lwb_context_t *p_context)) {
	PT_BEGIN(&pt_lwb_g_rr)
		;

		while (1) {
			leds_on(LEDS_GREEN);

			for (ui8_slot_idx = 0; ui8_slot_idx < N_CURRENT_DATA_SLOTS();
					ui8_slot_idx++) {

#if LWB_HSLP
				lwb_g_rr_hslp_receive_app_data();
#endif // LWB_HSLP

				// schedule Glossy for the next slot
				SCHEDULE(lwb_context.t_sync_ref,
						T_SYNC_ON + T_S_R_GAP + (ui8_slot_idx * (T_RR_ON + T_GAP)),
						lwb_g_rr_host);
				PT_YIELD(&pt_lwb_g_rr);

				if (CURRENT_SCHEDULE().slots[ui8_slot_idx] == 0) {
					// We have stream acknowledgement(s) to be sent.
					prepare_stream_acks();
					glossy_start(lwb_context.txrx_buf, lwb_context.txrx_buf_len,
							GLOSSY_INITIATOR, GLOSSY_NO_SYNC,
							N_RR, LWB_PKT_TYPE_STREAM_ACK,
							lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
									+ (ui8_slot_idx * (T_RR_ON + T_GAP))
									+ T_RR_ON,
							(rtimer_callback_t) lwb_g_rr_host, &lwb_context.rt,
							&lwb_context);

					PT_YIELD(&pt_lwb_g_rr);

					glossy_stop();

				} else if (CURRENT_SCHEDULE().slots[ui8_slot_idx] == node_id) {
					// This is my slot (allocated for the host)

					if (ui8_tx_buf_q_size != 0) {
						// We have something to send
						prepare_data_packet();
						glossy_start(lwb_context.txrx_buf,
								lwb_context.txrx_buf_len, GLOSSY_INITIATOR,
								GLOSSY_NO_SYNC,
								N_RR, LWB_PKT_TYPE_DATA,
								lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
										+ (ui8_slot_idx * (T_RR_ON + T_GAP))
										+ T_RR_ON,
								(rtimer_callback_t) lwb_g_rr_host,
								&lwb_context.rt, &lwb_context);

						PT_YIELD(&pt_lwb_g_rr);
						glossy_stop();

					} else {
						// Ohh...we have nothing to send even though this is our slot. We just stay silent.
					}

				} else {
					// The slot belongs to some other node
					glossy_start(lwb_context.txrx_buf, 0, GLOSSY_RECEIVER,
							GLOSSY_NO_SYNC,
							N_RR, 0,
							lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
									+ (ui8_slot_idx * (T_RR_ON + T_GAP))
									+ T_RR_ON,
							(rtimer_callback_t) lwb_g_rr_host, &lwb_context.rt,
							&lwb_context);

					PT_YIELD(&pt_lwb_g_rr);

					if (glossy_stop()) {
						// RX count is greater than zero i.e. Glossy received a packet
						ui8_g_header = get_header();
						lwb_context.txrx_buf_len = get_data_len();

						if (ui8_g_header == LWB_PKT_TYPE_DATA) {
							// Data packet
							lwb_sched_update_data_slot_usage(ui8_slot_idx, 1);
#if LWB_HSLP
							lwb_g_rr_hslp_send_app_data();
#else
							process_data_packet(ui8_slot_idx);
#endif // LWB_HSLP

						} else {
							// Unknown packets
							/// @todo add stats about unknown packets.
						}

					} else {
						// we haven't received Glossy flooding.
						/// @todo Add stats about not received Glossy flooding.
						lwb_sched_update_data_slot_usage(ui8_slot_idx, 0);
					}
				}
			}

			// We've iterated through all the data/ack slots. Now it is time for contention slots

			for (;
					ui8_slot_idx
							< N_CURRENT_DATA_SLOTS() + N_CURRENT_FREE_SLOTS();
					ui8_slot_idx++) {

				// schedule Glossy for the next slot.
				SCHEDULE(lwb_context.t_sync_ref,
						T_SYNC_ON + T_S_R_GAP + (ui8_slot_idx * (T_RR_ON + T_GAP)),
						lwb_g_rr_host);
				PT_YIELD(&pt_lwb_g_rr);

				glossy_start(lwb_context.txrx_buf, 0, GLOSSY_RECEIVER,
						GLOSSY_NO_SYNC,
						N_RR, 0,
						lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
								+ (ui8_slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON,
						(rtimer_callback_t) lwb_g_rr_host, &lwb_context.rt,
						&lwb_context);

				PT_YIELD(&pt_lwb_g_rr);

				if (glossy_stop()) {
					// RX count is greater than zero i.e. Glossy received a packet
					ui8_g_header = get_header();
					lwb_context.txrx_buf_len = get_data_len();

					if (ui8_g_header == LWB_PKT_TYPE_STREAM_REQ) {
						// Stream requests
#if LWB_HSLP
						lwb_g_rr_hslp_send_stream_reqs();
#endif
						// We feed stream request information regardless of external device is used to
						// compute schedule
						process_stream_reqs();
					} else {
						// Unknown packets
						/// @todo add stats about unknown packets.
					}

				} else {
					// we haven't received Glossy flooding.
					/// @todo Add stats about not received Glossy flooding.
				}

			}

			// Copy current schedule to old schedule
			memcpy(&OLD_SCHEDULE(), &CURRENT_SCHEDULE(),
					sizeof(lwb_schedule_t));

#if LWB_HSLP
			lwb_g_rr_hslp_send_receive_schedule();
#else
			// Compute new schedule
			lwb_sched_compute_schedule(&CURRENT_SCHEDULE());
#endif // LWB_HSLP

			// Increase the time by round period
			lwb_context.time += OLD_SCHEDULE_INFO().round_period;

			// We ignore what scheduler tells about host id and time
			CURRENT_SCHEDULE_INFO().host_id = node_id;
			CURRENT_SCHEDULE_INFO().time = UI32_GET_LOW(lwb_context.time);

			// Compress and copy the schedule to buffer
			memcpy(lwb_context.txrx_buf, &CURRENT_SCHEDULE_INFO(),
					sizeof(lwb_sched_info_t));
			lwb_context.txrx_buf_len = sizeof(lwb_sched_info_t);
			lwb_context.txrx_buf_len += lwb_sched_compress(&CURRENT_SCHEDULE(),
					lwb_context.txrx_buf + lwb_context.txrx_buf_len,
					LWB_MAX_TXRX_BUF_LEN - lwb_context.txrx_buf_len);

			// Schedule next Glossy synchronization based on old period.
			if (OLD_SCHEDULE_INFO().round_period == 1) {
				SCHEDULE(lwb_context.t_sync_ref,
						RTIMER_SECOND + (lwb_context.skew / (int32_t)64) - lwb_context.t_sync_guard,
						lwb_g_sync_host);
			} else {
				// Round period is not 1 second. Therefore, we use rtimer_set_long()
				SCHEDULE_L(lwb_context.t_sync_ref,
						(OLD_SCHEDULE_INFO().round_period * (uint32_t)RTIMER_SECOND) + ((int32_t)OLD_SCHEDULE_INFO().round_period * lwb_context.skew / (int32_t)64) - lwb_context.t_sync_guard,
						lwb_g_sync_host);
			}

			leds_off(LEDS_GREEN);

			LWB_SET_POLL_FLAG(LWB_POLL_FLAGS_SCHED_END);
			process_poll(&lwb_main_process);

			PT_YIELD(&pt_lwb_g_rr);

		}

	PT_END(&pt_lwb_g_rr);
return PT_ENDED;
}

//--------------------------------------------------------------------------------------------------
PT_THREAD(lwb_g_rr_source(struct rtimer *t, lwb_context_t *p_context)) {
PT_BEGIN(&pt_lwb_g_rr)
	;

// starting time of a slot = T_REF + T_S_R_GAP + (slot_idx * (T_RR_ON + T_GAP))
// end time of a slot      = T_REF + T_SYNC_ON + T_S_R_GAP + (slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON

	while (1) {
		leds_on(LEDS_GREEN);

		for (ui8_slot_idx = 0; ui8_slot_idx < N_CURRENT_DATA_SLOTS();
				ui8_slot_idx++) {

			// schedule Glossy for the next slot
			SCHEDULE(lwb_context.t_sync_ref,
					T_SYNC_ON + T_S_R_GAP + (ui8_slot_idx * (T_RR_ON + T_GAP)),
					lwb_g_rr_source);
			PT_YIELD(&pt_lwb_g_rr);

			if (CURRENT_SCHEDULE().slots[ui8_slot_idx] == node_id) {
				// This is our slot. Now we can become the initiator.

				if (ui8_tx_buf_q_size != 0) {
					// We have something to send
					prepare_data_packet();

					glossy_start(lwb_context.txrx_buf, lwb_context.txrx_buf_len,
							GLOSSY_INITIATOR, GLOSSY_NO_SYNC,
							N_RR, LWB_PKT_TYPE_DATA,
							lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
									+ (ui8_slot_idx * (T_RR_ON + T_GAP))
									+ T_RR_ON,
							(rtimer_callback_t) lwb_g_rr_source,
							&lwb_context.rt, &lwb_context);

					PT_YIELD(&pt_lwb_g_rr);
					glossy_stop();

				} else {
					// Ohh...we have nothing to send even though this is our slot. We just stay silent.
				}

			} else {
				// This is not our time slot. So we become receiver to receive Glossy flooding.
				lwb_save_ctrl_energest();
				glossy_start(lwb_context.txrx_buf, 0, GLOSSY_RECEIVER,
						GLOSSY_NO_SYNC,
						N_RR, 0,
						lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
								+ (ui8_slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON,
						(rtimer_callback_t) lwb_g_rr_source, &lwb_context.rt,
						&lwb_context);

				PT_YIELD(&pt_lwb_g_rr);

				if (glossy_stop()) {
					// RX count is greater than zero i.e. Glossy received a packet
					ui8_g_header = get_header();
					lwb_context.txrx_buf_len = get_data_len();

					if (ui8_g_header == LWB_PKT_TYPE_STREAM_ACK) {
						// Stream ACKs
						// Here, we are updating energet values only for stream acks.
						// It is fine since we always save energest values before glossy starts.
						lwb_update_ctrl_energest();
						process_stream_acks();
					} else if (ui8_g_header == LWB_PKT_TYPE_DATA) {
						// Data packet
						process_data_packet(ui8_slot_idx);
					} else {
						// Unknown packets
						/// @todo add stats about unknown packets.
					}

				} else {
					// we haven't received Glossy flooding.
					/// @todo Add stats about not received Glossy flooding.
				}
			}

		}

		// We've iterated through all the data/ack slots. Now it is time for contention slots

		for (; ui8_slot_idx < N_CURRENT_DATA_SLOTS() + N_CURRENT_FREE_SLOTS();
				ui8_slot_idx++) {

			// schedule Glossy for the next slot.
			SCHEDULE(lwb_context.t_sync_ref,
					T_SYNC_ON + T_S_R_GAP + (ui8_slot_idx * (T_RR_ON + T_GAP)),
					lwb_g_rr_source);
			PT_YIELD(&pt_lwb_g_rr);

			if (ui8_stream_reqs_lst_size > 0) {
				if (ui8_n_rounds_to_wait == 0) {
					// We have some stream requests to be sent
					prepare_stream_reqs();
					lwb_save_ctrl_energest();
					glossy_start(lwb_context.txrx_buf, lwb_context.txrx_buf_len,
							GLOSSY_INITIATOR, GLOSSY_NO_SYNC,
							N_RR, LWB_PKT_TYPE_STREAM_REQ,
							lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
									+ (ui8_slot_idx * (T_RR_ON + T_GAP))
									+ T_RR_ON,
							(rtimer_callback_t) lwb_g_rr_source,
							&lwb_context.rt, &lwb_context);

					PT_YIELD(&pt_lwb_g_rr);

					glossy_stop();
					lwb_update_ctrl_energest();
					ui8_n_trials++;
					// Calculate the number of trials to wait before trying next
					ui8_n_rounds_to_wait = (uint8_t) random_rand()
							% (1 << (ui8_n_trials % 4));
				} else {
					// We have to wait. So, we just participate to the flooding
					lwb_save_ctrl_energest();
					glossy_start(lwb_context.txrx_buf, 0, GLOSSY_RECEIVER,
							GLOSSY_NO_SYNC,
							N_RR, 0,
							lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
									+ (ui8_slot_idx * (T_RR_ON + T_GAP))
									+ T_RR_ON,
							(rtimer_callback_t) lwb_g_rr_source,
							&lwb_context.rt, &lwb_context);

					PT_YIELD(&pt_lwb_g_rr);
					glossy_stop();
					lwb_update_ctrl_energest();
					ui8_n_rounds_to_wait--;
				}

			} else {
				// We just participate to the flooding
				lwb_save_ctrl_energest();
				glossy_start(lwb_context.txrx_buf, 0, GLOSSY_RECEIVER,
						GLOSSY_NO_SYNC,
						N_RR, 0,
						lwb_context.t_sync_ref + T_SYNC_ON + T_S_R_GAP
								+ (ui8_slot_idx * (T_RR_ON + T_GAP)) + T_RR_ON,
						(rtimer_callback_t) lwb_g_rr_source, &lwb_context.rt,
						&lwb_context);

				PT_YIELD(&pt_lwb_g_rr);
				glossy_stop();
				lwb_update_ctrl_energest();
			}

		}

		// Copy current schedule to old schedule
		memcpy(&OLD_SCHEDULE(), &CURRENT_SCHEDULE(), sizeof(lwb_schedule_t));

		// Now, we have iterated through all the data and free slots.
		// We have to schedule for receiving next round's schedule.

		if (OLD_SCHEDULE_INFO().round_period == 1) {
			SCHEDULE(lwb_context.t_sync_ref,
					(uint32_t)RTIMER_SECOND + (lwb_context.skew / (int32_t)64) - lwb_context.t_sync_guard,
					lwb_g_sync_source);
		} else {
			// Round period is not 1 second. Therefore, we use rtimer_set_long()
			SCHEDULE_L(lwb_context.t_sync_ref,
					(OLD_SCHEDULE_INFO().round_period * (uint32_t)RTIMER_SECOND) + ((int32_t)OLD_SCHEDULE_INFO().round_period * lwb_context.skew / (int32_t)64) - lwb_context.t_sync_guard,
					lwb_g_sync_source);
		}

		leds_off(LEDS_GREEN);

		LWB_SET_POLL_FLAG(LWB_POLL_FLAGS_SCHED_END);
		process_poll(&lwb_main_process);

		PT_YIELD(&pt_lwb_g_rr);
	}

PT_END(&pt_lwb_g_rr);
return PT_ENDED;
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_g_rr_queue_packet(uint8_t* p_data, uint8_t ui8_len,
uint16_t ui16_to_node_id) {

data_buf_lst_item_t* p_item = memb_alloc(&mmb_data_buf);

if (!p_item) {
return 0;
}

if (LWB_MAX_TXRX_BUF_LEN > ui8_len + sizeof(data_header_t)) {

p_item->buf.header.from_id = node_id;
p_item->buf.header.to_id = ui16_to_node_id;
p_item->buf.header.data_len = ui8_len;
p_item->buf.header.options = 0;
memcpy(p_item->buf.data, p_data, ui8_len);
list_add(lst_tx_buf_queue, p_item);
ui8_tx_buf_q_size++;
return 1;
} else {
memb_free(&mmb_data_buf, p_item);
}

LWB_STATS_DATA(n_tx_nospace)++;

return 0;
}

//--------------------------------------------------------------------------------------------------
void lwb_g_rr_data_output() {

data_buf_lst_item_t* p_item = NULL;
while ((p_item = list_head(lst_rx_buf_queue))) {

if (lwb_context.p_callbacks && lwb_context.p_callbacks->p_on_data) {

	lwb_context.p_callbacks->p_on_data(p_item->buf.data,
			p_item->buf.header.data_len, p_item->buf.header.from_id,
			lwb_context.skew);
}

list_remove(lst_rx_buf_queue, p_item);
memb_free(&mmb_data_buf, p_item);
ui8_rx_buf_q_size--;
}
}

//--------------------------------------------------------------------------------------------------
uint8_t lwb_g_rr_stream_add(uint16_t ui16_ipi, uint16_t ui16_time_offset) {

stream_req_lst_item_t* p_req_item = memb_alloc(&mmb_stream_req);

if (!p_req_item) {
return 0;
}

p_req_item->req.ipi = ui16_ipi;
p_req_item->req.time_info = ui16_time_offset;
SET_STREAM_TYPE(p_req_item->req.req_type, LWB_STREAM_TYPE_ADD);
SET_STREAM_ID(p_req_item->req.req_type, ui8_stream_id_next);
list_add(lst_stream_req, p_req_item);
ui8_stream_reqs_lst_size++;

		// Set the joining state
switch (lwb_context.joining_state) {
case LWB_JOINING_STATE_NOT_JOINED:
case LWB_JOINING_STATE_JOINING:
lwb_context.joining_state = LWB_JOINING_STATE_JOINING;
break;
case LWB_JOINING_STATE_JOINED:
case LWB_JOINING_STATE_PARTLY_JOINED:
lwb_context.joining_state = LWB_JOINING_STATE_PARTLY_JOINED;
break;
}

return ui8_stream_id_next++;
}

//--------------------------------------------------------------------------------------------------
void lwb_g_rr_stream_del(uint8_t ui8_id) {

stream_req_lst_item_t* p_req_item = memb_alloc(&mmb_stream_req);

if (!p_req_item) {
return;
}

p_req_item->req.ipi = 0;
p_req_item->req.time_info = 0;
SET_STREAM_TYPE(p_req_item->req.req_type, LWB_STREAM_TYPE_DEL);
SET_STREAM_ID(p_req_item->req.req_type, ui8_id);
list_add(lst_stream_req, p_req_item);
ui8_stream_reqs_lst_size++;

		// we don't care about the joining state in here.
}

//--------------------------------------------------------------------------------------------------
void lwb_g_rr_stream_mod(uint8_t ui8_id, uint16_t ui16_ipi) {
stream_req_lst_item_t* p_req_item = memb_alloc(&mmb_stream_req);

if (!p_req_item) {
return;
}

p_req_item->req.ipi = ui16_ipi;
p_req_item->req.time_info = 0;
SET_STREAM_TYPE(p_req_item->req.req_type, LWB_STREAM_TYPE_MOD);
SET_STREAM_ID(p_req_item->req.req_type, ui8_id);
list_add(lst_stream_req, p_req_item);
ui8_stream_reqs_lst_size++;

		// Set the joining state
switch (lwb_context.joining_state) {
case LWB_JOINING_STATE_NOT_JOINED:
case LWB_JOINING_STATE_JOINING:
lwb_context.joining_state = LWB_JOINING_STATE_JOINING;
break;
case LWB_JOINING_STATE_JOINED:
case LWB_JOINING_STATE_PARTLY_JOINED:
lwb_context.joining_state = LWB_JOINING_STATE_PARTLY_JOINED;
break;
}
}
