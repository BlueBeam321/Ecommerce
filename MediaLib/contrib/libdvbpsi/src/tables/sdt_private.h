/*****************************************************************************
 * sdt_private.h: private SDT structures
 *----------------------------------------------------------------------------
 * Copyright (C) 2001-2011 VideoLAN
 * $Id: sdt_private.h,v 1.1 2002/12/11 13:04:57 jobi Exp $
 *
 * Authors: Johan Bilien <jobi@via.ecp.fr>
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

#ifndef _DVBPSI_SDT_PRIVATE_H_
#define _DVBPSI_SDT_PRIVATE_H_

/*****************************************************************************
 * dvbpsi_sdt_decoder_t
 *****************************************************************************
 * SDT decoder.
 *****************************************************************************/
typedef struct dvbpsi_sdt_decoder_s
{
    DVBPSI_DECODER_COMMON

    dvbpsi_sdt_callback           pf_sdt_callback;
    void *                        p_cb_data;

    dvbpsi_sdt_t                  current_sdt;
    dvbpsi_sdt_t *                p_building_sdt;

} dvbpsi_sdt_decoder_t;

/*****************************************************************************
 * dvbpsi_sdt_sections_gather
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_sdt_sections_gather(dvbpsi_t *p_dvbpsi,
                                dvbpsi_decoder_t *p_private_decoder,
                                dvbpsi_psi_section_t *p_section);

/*****************************************************************************
 * dvbpsi_sdt_sections_decode
 *****************************************************************************
 * SDT decoder.
 *****************************************************************************/
void dvbpsi_sdt_sections_decode(dvbpsi_sdt_t* p_sdt,
                                dvbpsi_psi_section_t* p_section);

#else
#error "Multiple inclusions of sdt_private.h"
#endif

