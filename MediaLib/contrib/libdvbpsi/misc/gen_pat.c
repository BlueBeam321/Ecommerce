/*****************************************************************************
 * gen_pat.c: PAT generator
 *----------------------------------------------------------------------------
 * Copyright (C) 2001-2011 VideoLAN
 * $Id: gen_pat.c,v 1.3 2002/10/07 14:15:14 sam Exp $
 *
 * Authors: Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *----------------------------------------------------------------------------
 *
 *****************************************************************************/


#include "config.h"

#include <stdio.h>
#include <stdbool.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

/* the libdvbpsi distribution defines DVBPSI_DIST */
#ifdef DVBPSI_DIST
#include "../src/dvbpsi.h"
#include "../src/psi.h"
#include "../src/tables/pat.h"
#else
#include <dvbpsi/dvbpsi.h>
#include <dvbpsi/psi.h>
#include <dvbpsi/pat.h>
#endif

/*****************************************************************************
 * writePSI
 *****************************************************************************/
static void writePSI(uint8_t* p_packet, dvbpsi_psi_section_t* p_section)
{
  p_packet[0] = 0x47;

  while(p_section)
  {
    size_t i_bytes_written = 0;
    uint8_t* p_pos_in_ts;
    uint8_t* p_byte = p_section->p_data;
    uint8_t* p_end =   p_section->p_payload_end
                     + (p_section->b_syntax_indicator ? 4 : 0);

    p_packet[1] |= 0x40;
    p_packet[3] = (p_packet[3] & 0x0f) | 0x10;

    p_packet[4] = 0x00; /* pointer_field */
    p_pos_in_ts = p_packet + 5;

    while((p_pos_in_ts < p_packet + 188) && (p_byte < p_end))
      *(p_pos_in_ts++) = *(p_byte++);
    while(p_pos_in_ts < p_packet + 188)
      *(p_pos_in_ts++) = 0xff;
    i_bytes_written = fwrite(p_packet, 1, 188, stdout);
    if(i_bytes_written == 0)
    {
      fprintf(stderr,"eof detected ... aborting\n");
      return;
    }
    p_packet[3] = (p_packet[3] + 1) & 0x0f;

    while(p_byte < p_end)
    {
      p_packet[1] &= 0xbf;
      p_packet[3] = (p_packet[3] & 0x0f) | 0x10;

      p_pos_in_ts = p_packet + 4;

      while((p_pos_in_ts < p_packet + 188) && (p_byte < p_end))
        *(p_pos_in_ts++) = *(p_byte++);
      while(p_pos_in_ts < p_packet + 188)
        *(p_pos_in_ts++) = 0xff;
      i_bytes_written = fwrite(p_packet, 1, 188, stdout);
      if(i_bytes_written == 0)
      {
        fprintf(stderr,"eof detected ... aborting\n");
        return;
      }
      p_packet[3] = (p_packet[3] + 1) & 0x0f;
    }

    p_section = p_section->p_next;
  }
}

static void message(dvbpsi_t *handle, const dvbpsi_msg_level_t level, const char* msg)
{
    switch(level)
    {
        case DVBPSI_MSG_ERROR: fprintf(stderr, "Error: "); break;
        case DVBPSI_MSG_WARN:  fprintf(stderr, "Warning: "); break;
        case DVBPSI_MSG_DEBUG: fprintf(stderr, "Debug: "); break;
        default: /* do nothing */
            return;
    }
    fprintf(stderr, "%s\n", msg);
}

/*****************************************************************************
 * main
 *****************************************************************************/
int main(int i_argc, char* pa_argv[])
{
  uint8_t packet[188];
  dvbpsi_pat_t pat;
  dvbpsi_psi_section_t* p_section1, * p_section2;
  dvbpsi_psi_section_t* p_section3, * p_section4;
  dvbpsi_psi_section_t* p_section5, * p_section6;
  int i;

  dvbpsi_t *p_dvbpsi = dvbpsi_new(&message, DVBPSI_MSG_DEBUG);
  if (p_dvbpsi == NULL)
      return 1;

  /* PAT generation */
  dvbpsi_pat_init(&pat, 1, 0, 0);
  dvbpsi_pat_program_add(&pat, 0, 0x12);
  dvbpsi_pat_program_add(&pat, 1, 0x42);
  dvbpsi_pat_program_add(&pat, 2, 0x21);
  dvbpsi_pat_program_add(&pat, 3, 0x24);
  for(i = 4; i < 43; i++)
    dvbpsi_pat_program_add(&pat, i, i);

  p_section1 = dvbpsi_pat_sections_generate(p_dvbpsi, &pat, 4);
  pat.b_current_next = 1;
  p_section2 = dvbpsi_pat_sections_generate(p_dvbpsi, &pat, 8);

  pat.i_version = 1;

  pat.b_current_next = 0;
  p_section3 = dvbpsi_pat_sections_generate(p_dvbpsi, &pat, 16);
  pat.b_current_next = 1;
  p_section4 = dvbpsi_pat_sections_generate(p_dvbpsi, &pat, 300);

  pat.i_version = 2;

  pat.b_current_next = 0;
  p_section5 = dvbpsi_pat_sections_generate(p_dvbpsi, &pat, 16);
  pat.b_current_next = 1;
  p_section6 = dvbpsi_pat_sections_generate(p_dvbpsi, &pat, 16);

  /* TS packets generation */
  packet[0] = 0x47;
  packet[1] = packet[2] = 0x00;
  packet[3] = 0x00;

  writePSI(packet, p_section1);
  writePSI(packet, p_section2);
  writePSI(packet, p_section3);
  writePSI(packet, p_section4);
  writePSI(packet, p_section5);
  writePSI(packet, p_section6);

  dvbpsi_DeletePSISections(p_section1);
  dvbpsi_DeletePSISections(p_section2);
  dvbpsi_DeletePSISections(p_section3);
  dvbpsi_DeletePSISections(p_section4);
  dvbpsi_DeletePSISections(p_section5);
  dvbpsi_DeletePSISections(p_section6);

  dvbpsi_pat_empty(&pat);

  dvbpsi_delete(p_dvbpsi);
  return 0;
}

