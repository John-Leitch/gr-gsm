/* -*- c++ -*- */

#ifndef INCLUDED_GSM_PACKED_BURST_H
#define INCLUDED_GSM_PACKED_BURST_H

struct packed_burst_header {
    uint16_t arfcn;		/* ARFCN (frequency) */
    uint32_t frame_number;	/* GSM Frame Number (FN) */
    uint8_t type : 4;	/* Type of burst/channel, see above */
    uint8_t timeslot : 4;	/* timeslot (0..7 on Um) */
};

#include "packed_burst.generated.h"


struct packed_burst {
    packed_burst_header hdr;
    uint8_t length;
    uint8_t *buffer; 
};

#endif /* INCLUDED_GSM_BURST_FILE_SINK_H */

