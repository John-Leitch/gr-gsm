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
#include "packed_burst.h"
#include "stdio.h"

namespace gr
{
    namespace gsm
    {
        const int8_t packed_burst_file_sink_impl::d_dummy_burst[] = {0, 0, 0,
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

        packed_burst_file_sink::sptr
        packed_burst_file_sink::make(const std::string &filename)
        {
            return gnuradio::get_initial_sptr(new packed_burst_file_sink_impl(filename));
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
            gsmtap_hdr *header = (gsmtap_hdr *)pmt::blob_data(header_plus_burst);

            packed_normal_burst pkd;

            pkd.hdr.arfcn = header->arfcn;
            pkd.hdr.frame_number = header->frame_number;
            pkd.hdr.timeslot = header->timeslot;
            bursts_caught++;
            if (current_frame_number != pkd.hdr.frame_number)
            {
                // if (current_frame_number_count != 8)
                // {
                //     printf("Success: %d %f\n", be32toh(current_frame_number), (float)current_frame_number_count / 8);
                // }

                if (!(frames_caught & 0xff))
                {
                    printf(
                        "Bursts caught: %d, expected: %d, success: %f\n",
                        bursts_caught, bursts_expected, (float)bursts_caught / bursts_expected);
                }

                current_frame_number = pkd.hdr.frame_number;
                current_frame_number_count = 1;
                bursts_expected += 8;
                frames_caught++;

                
            }
            else
            {
                current_frame_number_count++;
            }

            

            // int8_t *burst = (int8_t *)(pmt::blob_data(header_plus_burst)) + sizeof(gsmtap_hdr);
            // int8_t *burst2 = (int8_t *)((int8_t *)header) + sizeof(gsmtap_hdr);
            // printf("%p %p %d\n", burst, burst2, burst == burst2);
            
            int8_t *burst = (int8_t *)((int8_t *)header) + sizeof(gsmtap_hdr);
            size_t burst_len = pmt::blob_length(header_plus_burst) - sizeof(gsmtap_hdr);
            if (burst_len != 148)
            {
                printf("%d\n", burst_len);
            }

            uint32_t *burst32 = (uint32_t*)burst;
            uint64_t *burst64 = (uint64_t*)burst;
            // printf(".");

            if (burst_len == DUMMY_BURST_LEN &&
                //memcmp(burst, d_dummy_burst, DUMMY_BURST_LEN) == 0)
                FAST_BURST_CHECK)
            {
                pkd.hdr.type = GSMTAP_BURST_DUMMY;
                d_output_file.write((char *)&pkd, sizeof(packed_burst_header));
            }
            else
            {
                pkd.hdr.type = GSMTAP_BURST_NORMAL;
                pkd.bit0 = burst[0];
                pkd.bit1 = burst[1];
                pkd.bit2 = burst[2];
                pkd.bit3 = burst[3];
                pkd.bit4 = burst[4];
                pkd.bit5 = burst[5];
                pkd.bit6 = burst[6];
                pkd.bit7 = burst[7];
                pkd.bit8 = burst[8];
                pkd.bit9 = burst[9];
                pkd.bit10 = burst[10];
                pkd.bit11 = burst[11];
                pkd.bit12 = burst[12];
                pkd.bit13 = burst[13];
                pkd.bit14 = burst[14];
                pkd.bit15 = burst[15];
                pkd.bit16 = burst[16];
                pkd.bit17 = burst[17];
                pkd.bit18 = burst[18];
                pkd.bit19 = burst[19];
                pkd.bit20 = burst[20];
                pkd.bit21 = burst[21];
                pkd.bit22 = burst[22];
                pkd.bit23 = burst[23];
                pkd.bit24 = burst[24];
                pkd.bit25 = burst[25];
                pkd.bit26 = burst[26];
                pkd.bit27 = burst[27];
                pkd.bit28 = burst[28];
                pkd.bit29 = burst[29];
                pkd.bit30 = burst[30];
                pkd.bit31 = burst[31];
                pkd.bit32 = burst[32];
                pkd.bit33 = burst[33];
                pkd.bit34 = burst[34];
                pkd.bit35 = burst[35];
                pkd.bit36 = burst[36];
                pkd.bit37 = burst[37];
                pkd.bit38 = burst[38];
                pkd.bit39 = burst[39];
                pkd.bit40 = burst[40];
                pkd.bit41 = burst[41];
                pkd.bit42 = burst[42];
                pkd.bit43 = burst[43];
                pkd.bit44 = burst[44];
                pkd.bit45 = burst[45];
                pkd.bit46 = burst[46];
                pkd.bit47 = burst[47];
                pkd.bit48 = burst[48];
                pkd.bit49 = burst[49];
                pkd.bit50 = burst[50];
                pkd.bit51 = burst[51];
                pkd.bit52 = burst[52];
                pkd.bit53 = burst[53];
                pkd.bit54 = burst[54];
                pkd.bit55 = burst[55];
                pkd.bit56 = burst[56];
                pkd.bit57 = burst[57];
                pkd.bit58 = burst[58];
                pkd.bit59 = burst[59];
                pkd.bit60 = burst[60];
                pkd.bit61 = burst[61];
                pkd.bit62 = burst[62];
                pkd.bit63 = burst[63];
                pkd.bit64 = burst[64];
                pkd.bit65 = burst[65];
                pkd.bit66 = burst[66];
                pkd.bit67 = burst[67];
                pkd.bit68 = burst[68];
                pkd.bit69 = burst[69];
                pkd.bit70 = burst[70];
                pkd.bit71 = burst[71];
                pkd.bit72 = burst[72];
                pkd.bit73 = burst[73];
                pkd.bit74 = burst[74];
                pkd.bit75 = burst[75];
                pkd.bit76 = burst[76];
                pkd.bit77 = burst[77];
                pkd.bit78 = burst[78];
                pkd.bit79 = burst[79];
                pkd.bit80 = burst[80];
                pkd.bit81 = burst[81];
                pkd.bit82 = burst[82];
                pkd.bit83 = burst[83];
                pkd.bit84 = burst[84];
                pkd.bit85 = burst[85];
                pkd.bit86 = burst[86];
                pkd.bit87 = burst[87];
                pkd.bit88 = burst[88];
                pkd.bit89 = burst[89];
                pkd.bit90 = burst[90];
                pkd.bit91 = burst[91];
                pkd.bit92 = burst[92];
                pkd.bit93 = burst[93];
                pkd.bit94 = burst[94];
                pkd.bit95 = burst[95];
                pkd.bit96 = burst[96];
                pkd.bit97 = burst[97];
                pkd.bit98 = burst[98];
                pkd.bit99 = burst[99];
                pkd.bit100 = burst[100];
                pkd.bit101 = burst[101];
                pkd.bit102 = burst[102];
                pkd.bit103 = burst[103];
                pkd.bit104 = burst[104];
                pkd.bit105 = burst[105];
                pkd.bit106 = burst[106];
                pkd.bit107 = burst[107];
                pkd.bit108 = burst[108];
                pkd.bit109 = burst[109];
                pkd.bit110 = burst[110];
                pkd.bit111 = burst[111];
                pkd.bit112 = burst[112];
                pkd.bit113 = burst[113];
                pkd.bit114 = burst[114];
                pkd.bit115 = burst[115];
                pkd.bit116 = burst[116];
                pkd.bit117 = burst[117];
                pkd.bit118 = burst[118];
                pkd.bit119 = burst[119];
                pkd.bit120 = burst[120];
                pkd.bit121 = burst[121];
                pkd.bit122 = burst[122];
                pkd.bit123 = burst[123];
                pkd.bit124 = burst[124];
                pkd.bit125 = burst[125];
                pkd.bit126 = burst[126];
                pkd.bit127 = burst[127];
                pkd.bit128 = burst[128];
                pkd.bit129 = burst[129];
                pkd.bit130 = burst[130];
                pkd.bit131 = burst[131];
                pkd.bit132 = burst[132];
                pkd.bit133 = burst[133];
                pkd.bit134 = burst[134];
                pkd.bit135 = burst[135];
                pkd.bit136 = burst[136];
                pkd.bit137 = burst[137];
                pkd.bit138 = burst[138];
                pkd.bit139 = burst[139];
                pkd.bit140 = burst[140];
                pkd.bit141 = burst[141];
                pkd.bit142 = burst[142];
                pkd.bit143 = burst[143];
                pkd.bit144 = burst[144];
                pkd.bit145 = burst[145];
                pkd.bit146 = burst[146];
                pkd.bit147 = burst[147];

                d_output_file.write((char *)&pkd, sizeof(packed_normal_burst));
            }

            // std::string s = pmt::serialize_str(msg);
            // const char *serialized = s.data();
            // d_output_file.write(serialized, s.length());
        }
    } /* namespace gsm */
} /* namespace gr */
