/*****************************************************************************
 * dr_4c.h
 * Copyright (C) 2013, VideoLAN Association
 *
 * Authors: Jean-Paul Saman <jpsaman@videolan.org>
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

/*!
 * \file <dr_4c.h>
 * \author Jean-Paul Saman <jpsaman@videolan.org>
 * \brief Application interface for the time shifted service
 * descriptor decoder and generator.
 *
 * Application interface for the DVB time shifted service descriptor
 * descriptor decoder and generator. This descriptor's definition can be found in
 * ETSI EN 300 468 section 6.2.45.
 */

#ifndef DR_4C_H_
#define DR_4C_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * dvbpsi_tshifted_service_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_tshifted_service_dr_t
 * \brief "time shifted service" descriptor structure.
 *
 * This structure is used to store a decoded "time shifted service"
 * descriptor. (ETSI EN 300 468 section 6.2.45).
 */
/*!
 * \typedef struct dvbpsi_tshifted_service_dr_s dvbpsi_tshifted_service_dr_t
 * \brief dvbpsi_tshifted_service_dr_t type definition.
 */
/*!
 * \struct dvbpsi_tshifted_service_dr_s
 * \brief struct dvbpsi_tshifted_service_dr_s @see dvbpsi_tshifted_service_dr_t
 */
typedef struct dvbpsi_tshifted_service_dr_s
{
  uint16_t       i_ref_service_id;         /*!< reference service id */
} dvbpsi_tshifted_service_dr_t;

/*****************************************************************************
 * dvbpsi_DecodeTimeShiftedServiceDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_tshifted_service_dr_t *dvbpsi_DecodeTimeShiftedServiceDr(
                                        dvbpsi_descriptor_t *p_descriptor)
 * \brief "time shifted service" descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "time shifted service" descriptor structure
 * which contains the decoded data.
 */
dvbpsi_tshifted_service_dr_t* dvbpsi_DecodeTimeShiftedServiceDr(dvbpsi_descriptor_t *p_descriptor);

/*****************************************************************************
 * dvbpsi_GenTimeShiftedServiceDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t *dvbpsi_GenTimeShiftedServiceDr(dvbpsi_tshifted_service_dr_t *p_decoded,
                                                           bool b_duplicate);
 * \brief "time shifted service" descriptor generator.
 * \param p_decoded pointer to a decoded "time shifted service" descriptor structure
 * \param b_duplicate if true then duplicate the p_decoded structure into
 * the descriptor
 * \return a pointer to a new descriptor structure which contains encoded data.
 */
dvbpsi_descriptor_t *dvbpsi_GenTimeShiftedServiceDr(dvbpsi_tshifted_service_dr_t *p_decoded,
                                                    bool b_duplicate);

#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_4c.h"
#endif
