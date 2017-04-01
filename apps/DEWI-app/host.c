/*
 * host.c


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

#include "common.h"
#include "host.h"
PROCESS(dewi_app_host, "dewi_app_host");


void on_schedule_end_host(void){

}
void on_data_host(uint8_t *p_data, uint8_t ui8_len, uint16_t ui16_from_id,
		int32_t skew){

}
lwb_callbacks_t lwb_callbacks_host = { on_data_host, on_schedule_end_host };



PROCESS_THREAD(dewi_app_host, ev, data) {
PROCESS_BEGIN()
	;

	lwb_init(LWB_MODE_HOST, &lwb_callbacks_host);

PROCESS_END();

return PT_ENDED;
}
