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
#include "packed_burst_file_source_impl.h"
#include "stdio.h"

namespace gr
{
    namespace gsm
    {
        const int8_t packed_burst_file_source_impl::d_dummy_burst[] = {0, 0, 0,
                                                                       1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0,
                                                                       0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0,
                                                                       0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
                                                                       0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0,
                                                                       0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0,
                                                                       0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0,
                                                                       0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1,
                                                                       1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1,
                                                                       0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,
                                                                       0, 0, 0};
        packed_burst_file_source::sptr
        packed_burst_file_source::make(const std::string &filename)
        {
            return gnuradio::get_initial_sptr(new packed_burst_file_source_impl(filename));
        }

        /*
         * The private constructor
         */
        packed_burst_file_source_impl::packed_burst_file_source_impl(const std::string &filename)
            : gr::block("packed_burst_file_source",
                        gr::io_signature::make(0, 0, 0),
                        gr::io_signature::make(0, 0, 0)),
              d_input_file(filename.c_str(), std::ifstream::binary),
              d_finished(false)
        {
            message_port_register_out(pmt::mp("out"));
        }

        /*
         * Our virtual destructor.
         */
        packed_burst_file_source_impl::~packed_burst_file_source_impl()
        {
            if (d_finished == false)
            {
                d_finished = true;
            }
        }

        bool packed_burst_file_source_impl::start()
        {
            d_finished = false;
            d_thread = boost::shared_ptr<gr::thread::thread>(new gr::thread::thread(boost::bind(&packed_burst_file_source_impl::run, this)));
            return block::start();
        }

        bool packed_burst_file_source_impl::stop()
        {
            d_finished = true;
            d_thread->interrupt();
            d_thread->join();
            return block::stop();
        }

        bool packed_burst_file_source_impl::finished()
        {
            return d_finished;
        }

        void packed_burst_file_source_impl::run()
        {
            packed_burst_header ph;
            uint8_t buf[sizeof(gsmtap_hdr) + BURST_SIZE];
            uint8_t *burst = buf + sizeof(gsmtap_hdr);
            struct gsmtap_hdr *tap_header = (struct gsmtap_hdr *)buf;
            tap_header->version = GSMTAP_VERSION;
            tap_header->hdr_len = sizeof(gsmtap_hdr) / 4;
            tap_header->type = GSMTAP_TYPE_UM_BURST;
            char *burstBuffer;
            while (d_input_file.read((char *)&ph, sizeof(packed_burst_header)))
            {
                tap_header->sub_type = ph.type;
                tap_header->frame_number = htobe32(ph.frame_number);
                tap_header->timeslot = ph.timeslot;
                tap_header->arfcn = ph.arfcn;

                // printf(
                //     "Header size: %d, ARFCN: %d, frame_number: %d, type: %d, ts: %d\n",
                //     sizeof(packed_burst_header),
                //     ph.arfcn,
                //     ph.frame_number,
                //     ph.type,
                //     ph.timeslot);
                if (ph.type == GSMTAP_BURST_DUMMY)
                {
                    memcpy(burst, packed_burst_file_source_impl::d_dummy_burst, sizeof(packed_burst_file_source_impl::d_dummy_burst));
                    // continue;
                }
                else
                {
                    uint8_t burst_len;
                    d_input_file.read((char *)&burst_len, 1);

                    if (d_input_file.bad())
                    {
                        break;
                    }

                    // printf("Burst length: %d\n", burst_len);
                    burstBuffer = (char *)malloc(burst_len);
                    d_input_file.read(burstBuffer, burst_len);
                    
                    if (d_input_file.bad())
                    {
                        break;
                    }

                    int j = 0;
                    for (int i = 0; i < burst_len; i++)
                    {
                        uint8_t octet = burstBuffer[i];
                        burst[j++] = octet & 1;
                        burst[j++] = (octet >> 1) & 1;
                        burst[j++] = (octet >> 2) & 1;
                        burst[j++] = (octet >> 3) & 1;
                        burst[j++] = (octet >> 4) & 1;
                        burst[j++] = (octet >> 5) & 1;
                        burst[j++] = (octet >> 6) & 1;
                        burst[j++] = (octet >> 7) & 1;
                    }
                }

                pmt::pmt_t pdu_header = pmt::make_dict();
                pmt::pmt_t blob = pmt::make_blob(buf, sizeof(gsmtap_hdr) + BURST_SIZE);
                pmt::pmt_t msg = pmt::cons(pdu_header, blob);
                message_port_pub(pmt::mp("out"), msg);
                if (burstBuffer != 0)
                {
                    free(burstBuffer);
                    burstBuffer = 0;
                }
                // break;
            }

            // std::filebuf* pbuf = d_input_file.rdbuf();
            // while (!d_finished)
            // {
            //     pmt::pmt_t burst = pmt::deserialize(*pbuf);
            //     if (pmt::is_eof_object(burst)) {
            //         break;
            //     }
            //     message_port_pub(pmt::mp("out"), burst);
            // }
            d_input_file.close();
            post(pmt::mp("system"), pmt::cons(pmt::mp("done"), pmt::from_long(1)));
        }
    } /* namespace gsm */
} /* namespace gr */
