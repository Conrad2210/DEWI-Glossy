#include <stdio.h>
#include "contiki-conf.h"
#include "dev/uart1.h"

#include "lwb-hslp.h"

#if LWB_HSLP
static uint8_t ui8_dbg_frm_start = 0;
static uint8_t ui8_pkt_type = 0;
#endif // LWB_HSLP

int putchar(int c)
{
#if LWB_HSLP

    if(!ui8_dbg_frm_start) {
        // Start of debug printf
        ui8_dbg_frm_start = 1;
        lwb_hslp_write_start();

        LWB_HSLP_SET_PKT_MAIN_TYPE(LHSLP_PKT_TYPE_DBG_DATA, ui8_pkt_type);
        LWB_HSLP_SET_PKT_SUB_TYPE(LHSLP_PKT_SUB_TYPE_DBG_DATA, ui8_pkt_type);
        lwb_hslp_write_byte(ui8_pkt_type);
    }

    lwb_hslp_write_byte(c);

    if (c == '\n') {
        lwb_hslp_write_end();
        ui8_dbg_frm_start = 0;
    }

#else
    uart1_writeb((char)c);
#endif
    return c;
}



