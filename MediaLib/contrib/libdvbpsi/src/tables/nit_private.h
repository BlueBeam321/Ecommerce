/*****************************************************************************
 * nit_private.h: private NIT structures
 *----------------------------------------------------------------------------
 * Copyright (C) 2001-2011 VideoLAN
 * $Id$
 *
 * Authors: Johann Hanne
 *          heavily based on pmt.c which was written by
 *          Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 *          Jean-Paul Saman <jpsaman@videolan.org>
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

#ifndef _DVBPSI_NIT_PRIVATE_H_
#define _DVBPSI_NIT_PRIVATE_H_

/*****************************************************************************
 * dvbpsi_nit_decoder_t
 *****************************************************************************
 * NIT decoder.
 *****************************************************************************/
typedef struct dvbpsi_nit_decoder_s
{
    DVBPSI_DECODER_COMMON

    dvbpsi_nit_callback           pf_nit_callback;
    void *                        p_cb_data;

    dvbpsi_nit_t                  current_nit;
    dvbpsi_nit_t *                p_building_nit;

    uint16_t                      i_network_id;

} dvbpsi_nit_decoder_t;

/*****************************************************************************
 * dvbpsi_nit_sections_gather
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_nit_sections_gather(dvbpsi_t* p_dvbpsi,
                              dvbpsi_decoder_t* p_private_decoder,
                              dvbpsi_psi_section_t* p_section);

/*****************************************************************************
 * dvbpsi_nit_sections_decode
 *****************************************************************************
 * NIT decoder.
 *****************************************************************************/
void dvbpsi_nit_sections_decode(dvbpsi_nit_t* p_nit,
                               dvbpsi_psi_section_t* p_section);

#else
#error "Multiple inclusions of nit_private.h"
#endif

