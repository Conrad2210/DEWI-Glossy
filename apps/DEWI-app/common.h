/*
 * common.h
 *
 *  Created on: Apr 1, 2017
 *      Author: user
 */

#ifndef COMMON_H_
#define COMMON_H_



#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "lwb.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/random.h"



//PROCESS(dewi_app_server, "dewi_app_server");


#define LWB_HOST_ID         1
#define LWB_LIGHTSWITCH    	2
#define LWB_SERVER    		3

enum frameType {
	hello = 0x01,
	neighupdate = 0x02,
	dataExp = 0x03,
	resultLamp = 0x04,
	neighUpdAck = 0x05,
	startExp = 0x06,
	finishExp = 0x07,
	resultAck = 0x08,
	resultReq = 0x09,
	reset = 0x0a,
	resultSwitch = 0x0b
};


typedef struct resultStruct {
	struct resultStruct* next;
	uint8_t timeslot;
	uint16_t counter;
} resultStruct_t;
typedef struct neighStruct {
	struct neighStruct* next;
	uint8_t id;
} neighStruct_t;

/// @brief Result buffer element list
MEMB(mmb_results, resultStruct_t, 200);
/// @brief Result buffer element list
LIST(lst_results);

MEMB(mmb_neigh, neighStruct_t, 60);
/// @brief Result buffer element list
LIST(lst_neigh);

static uint8_t buffer[100];
static uint8_t ui8_buf_len = 0;


inline void send_stream_mod();


#endif /* COMMON_H_ */
