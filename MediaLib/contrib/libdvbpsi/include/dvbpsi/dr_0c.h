/*****************************************************************************
 * dr_0c.h
 * Copyright (C) 2001-2010 VideoLAN
 * $Id: dr_0c.h,v 1.2 2002/05/10 23:50:36 bozo Exp $
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

/*!
 * \file <dr_0c.h>
 * \author Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 * \brief Application interface for the MPEG 2 "multiplex buffer utilization"
 * descriptor decoder and generator.
 *
 * Application interface for the MPEG 2 "multiplex buffer utilization"
 * descriptor decoder and generator. This descriptor's definition can be found
 * in ISO/IEC 13818-1 section 2.6.22.
 */

#ifndef _DVBPSI_DR_0C_H_
#define _DVBPSI_DR_0C_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_mx_buff_utilization_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_mx_buff_utilization_dr_s
 * \brief "multiplex buffer utilization" descriptor structure.
 *
 * This structure is used to store a decoded "multiplex buffer utilization"
 * descriptor. (ISO/IEC 13818-1 section 2.6.22).
 */
/*!
 * \typedef struct dvbpsi_mx_buff_utilization_dr_s dvbpsi_mx_buff_utilization_dr_t
 * \brief dvbpsi_systemclock_dr_t type definition.
 */
typedef struct dvbpsi_mx_buff_utilization_dr_s
{
  bool          b_mdv_valid;            /*!< mdv_valid_flag */
  uint16_t      i_mx_delay_variation;   /*!< multiplex_delay_variation */
  uint8_t       i_mx_strategy;          /*!< multiplex_strategy */

} dvbpsi_mx_buff_utilization_dr_t;


/*****************************************************************************
 * dvbpsi_DecodeMxBuffUtilizationDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_mx_buff_utilization_dr_t * dvbpsi_DecodeMxBuffUtilizationDr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief "multiplex buffer utilization" descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "multiplex buffer utilization" descriptor
 * structure which contains the decoded data.
 */
dvbpsi_mx_buff_utilization_dr_t* dvbpsi_DecodeMxBuffUtilizationDr(
                                        dvbpsi_descriptor_t * p_descriptor);


/*****************************************************************************
 * dvbpsi_GenMxBuffUtilizationDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t * dvbpsi_GenMxBuffUtilizationDr(
                dvbpsi_mx_buff_utilization_dr_t * p_decoded, bool b_duplicate)
 * \brief "multiplex buffer utilization" descriptor generator.
 * \param p_decoded pointer to a decoded "system clock" descriptor
 * structure
 * \param b_duplicate if true then duplicate the p_decoded structure into
 * the descriptor
 * \return a pointer to a new descriptor structure which contains encoded data.
 */
dvbpsi_descriptor_t * dvbpsi_GenMxBuffUtilizationDr(
                                dvbpsi_mx_buff_utilization_dr_t * p_decoded,
                                bool b_duplicate);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_0c.h"
#endif

