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

#ifndef __LWB_COMMON_H__
#define __LWB_COMMON_H__

/// @file lwb-common.h
/// @brief Common header file

#include <stdint.h>

#include "contiki.h"

#ifdef LWB_CUSTOM_CONF_H
#include LWB_CUSTOM_CONF_H
#endif

#include "lwb-default-conf.h"

#define LWB_MAX_SLOTS_UNIT_TIME ((uint8_t)((RTIMER_SECOND - T_SYNC_ON - T_HSLP_SCHED)/(T_GAP + T_RR_ON)) - 1)

/// @brief LWB mode.
typedef enum {
    LWB_MODE_HOST = 1,  ///< LWB in host mode.
    LWB_MODE_SOURCE     ///< LWB in source mode.
} lwb_mode_t;

/// @brief Stream request types
typedef enum {
    LWB_STREAM_TYPE_ADD = 1,     ///< Stream add request.
    LWB_STREAM_TYPE_DEL,         ///< Stream delete request.
    LWB_STREAM_TYPE_MOD          ///< Stream repeat request. @note Not sure
} stream_req_types_t;

/// @brief LWB packet types
typedef enum {
    LWB_PKT_TYPE_NO_DATA = 0,   ///< No Type
    LWB_PKT_TYPE_STREAM_REQ,    ///< Stream requests
    LWB_PKT_TYPE_STREAM_ACK,    ///< Stream acknowledgements
    LWB_PKT_TYPE_DATA,          ///< Data packets
    LWB_PKT_TYPE_SCHED          ///< Schedule packets
} pkt_types_t;

/// @brief Synchronization states
typedef enum {
    LWB_SYNC_STATE_BOOTSTRAP = 0,      ///< Initial state. Not synchronized
    LWB_SYNC_STATE_QUASI_SYNCED,       ///< Received schedule when in BOOTSTRAP state. Clock drift is not estimated.
    LWB_SYNC_STATE_SYNCED,             ///< Received schedule and clock drift is estimated.
    LWB_SYNC_STATE_UNSYNCED_1,         ///< Missed schedule one time.
    LWB_SYNC_STATE_UNSYNCED_2,         ///< Missed schedule two times.
    LWB_SYNC_STATE_UNSYNCED_3,         ///< Missed schedule three times.
} lwb_sync_states_t;

typedef enum {
    LWB_JOINING_STATE_NOT_JOINED = 0,
    LWB_JOINING_STATE_JOINING,
    LWB_JOINING_STATE_PARTLY_JOINED,
    LWB_JOINING_STATE_JOINED
} lwb_joining_state_t;


typedef enum {
    LWB_POLL_FLAGS_DATA = 0,
    LWB_POLL_FLAGS_SCHED_END,
    LWB_POLL_FLAGS_SLOT_END
} lwb_poll_flags_t;

/// @brief Header to be used when sending data
typedef struct __attribute__ ((__packed__)) data_header {
    uint16_t from_id;  ///< From node ID
    uint16_t to_id;    ///< To node ID
    uint8_t  data_len;  ///< Data options
    uint8_t  in_queue;  ///< Number of packets in queue that are ready to be sent
    uint8_t  options;   ///< Data options
} data_header_t;


/// @brief This header is used when sending stream requests as separate messages.
typedef struct __attribute__ ((__packed__)) lwb_stream_req_header {
    uint16_t    from_id;      ///< ID of the node that send the packet
    uint8_t     n_reqs;         ///< Number of stream requests
} lwb_stream_req_header_t;


/// @brief Structure for stream requests
typedef struct __attribute__ ((__packed__)) lwb_stream_req {
    uint16_t ipi;           ///< Inter-packet interval in seconds.
    uint16_t time_info;     ///< Starting time of the stream.
    uint8_t  req_type;       ///< Request type. Least significant 2 bits represent stream request type.
                                 ///  Most significant 6 bits represent request ID.
                                 ///  @see stream_req_types_t and GET_STREAM_ID and SET_STREAM_ID
} lwb_stream_req_t;

/// @brief Structure for the header of a schedule
typedef struct __attribute__ ((__packed__)) lwb_sched_info {
    uint16_t host_id;       ///< ID of the host.
    uint16_t time;          ///< Least significant 16 bits of the current time at the host.
    uint8_t  n_slots;       ///< Number of slots in the round.
                            ///  Most significant 2 bits represent free slots and least significant 6 bits represent data slots.
    uint8_t  round_period;  ///< Round period (duration between beginning of two rounds) in seconds
    uint16_t t_current_rtimer;
} lwb_sched_info_t;

/// @brief LWB schedule
typedef struct __attribute__ ((__packed__)) lwb_schedule {
    lwb_sched_info_t    sched_info;                                 ///< schedule information
    uint16_t            slots[LWB_SCHED_MAX_SLOTS];    ///< slots. The node ID will be stored.
} lwb_schedule_t;

typedef struct lwb_stream_info {
    struct lwb_stream_info *next;
    uint16_t node_id;             ///< Node ID
    uint16_t ipi;                 ///< Inter-packet interval in seconds
    uint16_t last_assigned;
    uint16_t next_ready;
    uint8_t  stream_id;           ///< Stream ID
    uint8_t  n_cons_missed;       ///< Number of consecutive slot misses for the stream
    uint8_t  n_slots_used;        ///< Number of slots used in a round
    uint8_t  n_slots_allocated;   ///< Number of allocated slots for the stream in a round

    uint8_t  avg_max_qlen;
    uint8_t  max_qlen;            ///< Maximum length of the queue at the source node
    uint8_t  max_qlen_tmp;
    uint8_t  qlens[LWB_SCHED_MAX_QLEN_WNIDOW_SIZE];
    uint8_t  n_qlens;

} lwb_stream_info_t;

/// @brief LWB callbacks
typedef struct lwb_callbacks {
    void (*p_on_data)(uint8_t*, uint8_t, uint16_t,int32_t);
    void (*p_on_sched_end)(void);
} lwb_callbacks_t;


/// @brief Scheduler related statistics
typedef struct lwb_sched_stats {
    uint16_t n_added;          ///< Number of streams added so far
    uint16_t n_deleted;        ///< Number of streams deleted so far
    uint16_t n_no_space;       ///< Number of streams that are unable to add due to space unavailability
    uint16_t n_modified;       ///< Number of streams modified
    uint16_t n_duplicates;     ///< Number of duplicated stream requests
    uint16_t n_unused_slots;
} lwb_sched_stats_t;

/// @brief Glossy synchronization related statistics
typedef struct lwb_sync_stats {
    uint16_t n_synced;           ///< Number of instances that the schedule is received
    uint16_t n_sync_missed;      ///< Number of instances that the schedule is not received
} lwb_sync_stats_t;

/// @brief Statistics related to data packets
typedef struct lwb_data_stats {
    uint16_t n_tx_nospace;     ///< Number of instances in which unable to allocate memory for transmitting data
    uint16_t n_rx_nospace;     ///< Number of instances in which unable to allocate memory for receiving data
    uint16_t n_tx;             ///< Number of data packets transmitted
    uint16_t n_rx;             ///< Number of data packets received
    uint16_t n_rx_dropped;     ///< Number of data packets dropped
} lwb_data_stats_t;

/// @brief Stream requests and acknowledgement related statistics
typedef struct lwb_stream_req_ack_stats {
    uint16_t n_req_tx;         ///< Number of stream requests sent
    uint16_t n_req_rx;         ///< Number of stream requests received
    uint16_t n_ack_tx;         ///< Number of stream acknowledgements sent
    uint16_t n_ack_rx;         ///< Number of stream acknowledgements received
} lwb_stream_req_ack_stats_t;


/// @brief LWB context
typedef struct lwb_context {
    // common
    lwb_schedule_t  current_sched;                              ///< Current schedule.
    lwb_schedule_t  old_sched;                                  ///< Old schedule.
    uint32_t        time;                                       ///< Global time in seconds
    int32_t         skew;                                       ///< Clock skew per unit time in rtimer ticks.
    uint16_t        t_sync_guard;                               ///< Guard time that Glossy should be started earlier for receiving schedule.
    uint16_t        t_sync_ref;                                 ///< Reference time of the host when schedule is received in rtimer ticks.
    uint16_t        t_last_sync_ref;                            ///< Reference time of the host when last schedule is received in rtimer ticks.
    uint16_t        t_start;                                    ///< Start time of the first schedule transmission in a round.
    int			t_offset;									///< Clock offset to host node
    lwb_callbacks_t *p_callbacks;                               ///< Call back functions.
    uint8_t         lwb_mode;                                   ///< Mode of LWB @see lwb_mode_t
    struct rtimer   rt;                                         ///< the real-time timer used to start glossy phases
    uint8_t         txrx_buf[LWB_MAX_TXRX_BUF_LEN];             ///< TX/RX buffer
    uint8_t         txrx_buf_len;                               ///< The length of the data in TX/RX buffer
    uint8_t         poll_flags;                                 ///< Flags that indicate why LWB main process is polled.
    uint8_t         n_my_slots;                                 ///< Number of slots allocated for the node

    // source node
    uint8_t         sync_state;                                 ///< Synchronization state
    uint8_t         joining_state;                              ///< Joining state

    // host
    uint16_t        ui16arr_stream_akcs[LWB_SCHED_MAX_SLOTS];   ///< IDs of the nodes which stream acknowledgements to be sent
    uint8_t         n_stream_acks;                              ///< Number of stream acknowledgements

    // stats
    lwb_sync_stats_t            sync_stats;
    lwb_data_stats_t            data_stats;
    lwb_stream_req_ack_stats_t  stream_req_ack_stats;
    lwb_sched_stats_t           sched_stats;

    // for radio duty cycle of control packets
    uint32_t en_control;
    uint32_t en_rx;
    uint32_t en_tx;


} lwb_context_t;


/// @brief Structure for data buffer element.
typedef struct data_buf {
    data_header_t   header;
    uint8_t         data[LWB_MAX_TXRX_BUF_LEN];
} data_buf_t;

/// @brief Structure for data buffer element.
typedef struct data_buf_lst_item {
    struct data_buf* next;
    data_buf_t        buf;
} data_buf_lst_item_t;


typedef struct stream_req_lst_item {
    struct stream_req_lst_item* next;
    lwb_stream_req_t            req;
} stream_req_lst_item_t;

/// @defgroup Stream and schedule related macros
/// @{


/// @brief Get the ID of the stream. This is to be used with lwb_stream_req#req_type.
#define GET_STREAM_ID(req_opt)                  (req_opt >> 2)
/// @brief Set the ID of the stream. This is to be used to set lwb_stream_req#req_type .
#define SET_STREAM_ID(req_opt, id)              (req_opt = (id << 2) | (req_opt & 0x03))

#define SET_STREAM_TYPE(req_opt, type)          (req_opt = ((req_opt >> 2) << 2) | (type & 0x03))

#define GET_STREAM_TYPE(req_opt)                (req_opt & 0x03)

/// @brief Get number of free slots in a schedule. This is to be used with lwb_sched_header#n_slots.
#define GET_N_FREE_SLOTS(a)                     (a >> 6)
/// @brief Set number of free slots in a schedule. This is to be used to set lwb_sched_header#n_slots.
#define SET_N_FREE_SLOTS(slots, free_slots)     (slots = (slots & 0x3f)  | (free_slots << 6))
/// @brief Get number of data slots in a schedule. This is to be used with lwb_sched_header#n_slots.
#define GET_N_DATA_SLOTS(a)                     (a & 0x3f)
/// @brief Set number of data slots in a schedule. This is to be used to set lwb_sched_header#n_slots.
#define SET_N_DATA_SLOTS(slots, data_slots)     (slots = (slots & 0xc0) | (data_slots & 0x3f))

/// @}

/// @addtogroup UI32 Macros
///           Unsign 32-bit integer lated macros to get/set low/high segments.
/// @{
#define UI32_GET_LOW(var)             (uint16_t)(var & 0xffff)
#define UI32_GET_HIGH(var)            (uint16_t)(var >> 16)
#define UI32_SET_LOW(var, val)   (var = ((uint32_t)UI32_GET_HIGH(var) << 16) | (uint32_t)((val) & 0xffff))
#define UI32_SET_HIGH(var, val)  (var = ((uint32_t)(val) << 16) | (uint32_t)UI32_GET_LOW(var))

/// @}

/// @addtogroup glossy macros
/// @{
/// @brief Reference time of Glossy.
#define GLOSSY_T_REF                (get_t_ref_l())
/// @brief Check if glossy's reference time is updated.
#define GLOSSY_IS_SYNCED()          (is_t_ref_l_updated())
/// @brief Set glossy's reference time not updated.
#define GLOSSY_SET_UNSYNCED()       (set_t_ref_l_updated(0))
/// @}

#endif // __LWB_COMMON_H__
