/* -*- c++ -*- */
/* @file
 * @author (C) 2015 by Roman Khassraf <rkhassraf@gmail.com>
 * @section LICENSE
 *
 * Gr-gsm is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gr-gsm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gr-gsm; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include <grgsm/gsmtap.h>
#include "packed_burst_file_sink_impl.h"
#include "stdio.h"

namespace gr {
  namespace gsm {
      const int8_t packed_burst_file_sink_impl::d_dummy_burst[] = {0,0,0,
            1,1,1,1,1,0,1,1,0,1,1,1,0,1,1,0,
            0,0,0,0,1,0,1,0,0,1,0,0,1,1,1,0,
            0,0,0,0,1,0,0,1,0,0,0,1,0,0,0,0,
            0,0,0,1,1,1,1,1,0,0,0,1,1,1,0,0,
            0,1,0,1,1,1,0,0,0,1,0,1,1,1,0,0,
            0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,0,
            0,0,1,1,0,0,1,1,0,0,1,1,1,0,0,1,
            1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,1,
            0,0,1,0,1,1,1,1,1,0,1,0,1,0,
            0,0,0 };

    packed_burst_file_sink::sptr
    packed_burst_file_sink::make(const std::string &filename)
    {
      return gnuradio::get_initial_sptr
        (new packed_burst_file_sink_impl(filename));
    }

    /*
     * The private constructor
     */
    packed_burst_file_sink_impl::packed_burst_file_sink_impl(const std::string &filename)
      : gr::block("packed_burst_file_sink",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 0, 0)),
              d_output_file(filename.c_str(), std::ofstream::binary)
    {
        message_port_register_in(pmt::mp("in"));
        set_msg_handler(pmt::mp("in"), boost::bind(&packed_burst_file_sink_impl::process_burst, this, boost::placeholders::_1));
    }

    /*
     * Our virtual destructor.
     */
    packed_burst_file_sink_impl::~packed_burst_file_sink_impl()
    {
        if (d_output_file.is_open())
        {
            d_output_file.close();
        }
    }

    

    void packed_burst_file_sink_impl::process_burst(pmt::pmt_t msg)
    {
        pmt::pmt_t header_plus_burst = pmt::cdr(msg);
        gsmtap_hdr * header = (gsmtap_hdr *)pmt::blob_data(header_plus_burst);
        
        packed_burst pkd;
        
        pkd.hdr.arfcn = header->arfcn;
        pkd.hdr.frame_number = be32toh(header->frame_number);
        pkd.hdr.timeslot = header->timeslot;

        int8_t * burst = (int8_t *)(pmt::blob_data(header_plus_burst))+sizeof(gsmtap_hdr);
        size_t burst_len=pmt::blob_length(header_plus_burst)-sizeof(gsmtap_hdr);

        if (burst_len == DUMMY_BURST_LEN &&
            memcmp(burst, d_dummy_burst, DUMMY_BURST_LEN) == 0)
        {
            pkd.hdr.type = GSMTAP_BURST_DUMMY;
            d_output_file.write((char*)&pkd, sizeof(packed_burst_header));
        }
        else
        {
            pkd.hdr.type = GSMTAP_BURST_NORMAL;
            pkd.length = (burst_len + (8 - 1)) / 8;
            pkd.buffer = (uint8_t*)malloc(pkd.length);

            if (pkd.buffer == 0)
            {
                return;
            }

            int j = 0;

            for (int i = 0; i < burst_len; i++)
            {
                int bit = i & 0x7;

                if (bit == 0x0)
                {
                    pkd.buffer[j] = 0x0;
                }

                if (burst[i] == 1)
                {
                    pkd.buffer[j] |= (1 << bit);
                }

                if (bit == 0x7)
                {
                    j++;
                }
            }

            d_output_file.write((char*)&pkd, sizeof(packed_burst_header) + sizeof(uint8_t));
            d_output_file.write((char*)pkd.buffer, pkd.length);
            free(pkd.buffer);
        }

        
        // std::string s = pmt::serialize_str(msg);
        // const char *serialized = s.data();
        // d_output_file.write(serialized, s.length());
    }
  } /* namespace gsm */
} /* namespace gr */

