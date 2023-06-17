/*****************************************************************************
 * dr_08.c
 * Copyright (C) 2001-2011 VideoLAN
 * $Id: dr_08.c,v 1.4 2003/07/25 20:20:40 fenrir Exp $
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
 *****************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include "../dvbpsi.h"
#include "../dvbpsi_private.h"
#include "../descriptor.h"

#include "dr_08.h"


/*****************************************************************************
 * dvbpsi_DecodeVWindowDr
 *****************************************************************************/
dvbpsi_vwindow_dr_t * dvbpsi_DecodeVWindowDr(dvbpsi_descriptor_t * p_descriptor)
{
    dvbpsi_vwindow_dr_t * p_decoded;

    /* Check the tag */
    if (!dvbpsi_CanDecodeAsDescriptor(p_descriptor, 0x08))
        return NULL;

    /* Don't decode twice */
    if (dvbpsi_IsDescriptorDecoded(p_descriptor))
        return p_descriptor->p_decoded;

    if (p_descriptor->i_length != 4)
        return NULL;

    /* Allocate memory */
    p_decoded = (dvbpsi_vwindow_dr_t*)malloc(sizeof(dvbpsi_vwindow_dr_t));
    if (!p_decoded)
        return NULL;

    p_decoded->i_horizontal_offset =   ((uint16_t)(p_descriptor->p_data[0]) << 6)
            | ((p_descriptor->p_data[1] & 0xfc) >> 2);
    p_decoded->i_vertical_offset =
            ((uint16_t)(p_descriptor->p_data[1] & 0x03) << 12)
            | ((uint16_t)(p_descriptor->p_data[2]) << 4)
            | ((p_descriptor->p_data[3] & 0xf0) >> 4);
    p_decoded->i_window_priority = p_descriptor->p_data[3] & 0x0f;

    p_descriptor->p_decoded = (void*)p_decoded;

    return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenVWindowDr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenVWindowDr(dvbpsi_vwindow_dr_t * p_decoded,
                                          bool b_duplicate)
{
    /* Create the descriptor */
    dvbpsi_descriptor_t * p_descriptor = dvbpsi_NewDescriptor(0x08, 4, NULL);

    if (!p_descriptor)
        return NULL;

    /* Encode data */
    p_descriptor->p_data[0] = p_decoded->i_horizontal_offset >> 6;
    p_descriptor->p_data[1] =   (((uint8_t)p_decoded->i_horizontal_offset) << 2)
            | (p_decoded->i_vertical_offset >> 12);
    p_descriptor->p_data[2] = p_decoded->i_vertical_offset >> 4;
    p_descriptor->p_data[3] =   (((uint8_t)p_decoded->i_vertical_offset) << 4)
            | (p_decoded->i_window_priority & 0x0f);

    if (b_duplicate)
    {
        /* Duplicate decoded data */
        p_descriptor->p_decoded =
                dvbpsi_DuplicateDecodedDescriptor(p_decoded,
                                                  sizeof(dvbpsi_vwindow_dr_t));
    }

    return p_descriptor;
}

