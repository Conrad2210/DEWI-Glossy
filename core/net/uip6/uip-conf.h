#ifndef __UIP_CONF_H__
#define __UIP_CONF_H__

#include <inttypes.h>

/**
 * Statistics datatype
 *
 * This typedef defines the dataype used for keeping statistics in
 * uIP.
 *
 * \hideinitializer
 */
typedef unsigned short uip_stats_t;


///< Maximum number of TCP connections.
#define UIP_CONF_MAX_CONNECTIONS 10
///< Maximum number of listening TCP ports.
#define UIP_CONF_MAX_LISTENPORTS 10
///< uIP buffer size.
#define UIP_CONF_BUFFER_SIZE     800
///< CPU byte order.
#define UIP_CONF_BYTE_ORDER      LITTLE_ENDIAN
///< Logging on or off
#define UIP_CONF_LOGGING         0
///< UDP support on or off
#define UIP_CONF_UDP             0
///< TCP support on or off
#define UIP_CONF_TCP             1
///< UDP checksums on or off
#define UIP_CONF_UDP_CHECKSUMS   1
///< uIP statistics on or off
#define UIP_CONF_STATISTICS      1
///< enable uIPv6
#define UIP_CONF_IPV6            1
///< The link layer is 802.15.4
#define UIP_CONF_LL_802154       1

#define UIP_CONF_LLH_LEN         0

#endif /* __UIP_CONF_H__ */

/** @} */
/** @} */
