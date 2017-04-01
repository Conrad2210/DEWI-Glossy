/*
 * common.c
 *
 *  Created on: Apr 1, 2017
 *      Author: user
 */

#include "common.h"
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

