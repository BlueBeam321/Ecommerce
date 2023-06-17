/*****************************************************************************
 * dr_80.h
  * Copyright (C) 2016-2019 IT02 MultiLab
 * $Id: dr_80.c,v 1.7 2003/07/25 20:20:40 fenrir Exp $
 *
 * Authors: Code
 *
 * This library is created by Code;


 *****************************************************************************/

/*!
 * \file <dr_80.h>
 * \author Laurent Aimar <fenrir@via.ecp.fr>
 * \brief Application interface for the "User event" descriptor
 * decoder and generator.
 *
 * Application interface for the "User event" descriptor
 * decoder and generator. This descriptor's definition can be found in
 * ETSI EN 300 468 section 6.2.35.
 */

#ifndef _DVBPSI_DR_80_H_
#define _DVBPSI_DR_80_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_user_event_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_user_event_dr_s
 * \brief "User event" descriptor structure.
 *
 * This structure is used to store a decoded "User event" descriptor.
 */
/*!
 * \typedef struct dvbpsi_user_event_dr_s dvbpsi_user_event_dr_t
 * \brief dvbpsi_user_event_dr_t type definition.
 */
typedef struct dvbpsi_user_event_dr_s
{
  uint8_t i_iso_639_code[3];    /*!< ISO 639 language code */
  int     i_event_name_length;  /*!< length of event name */
  int     i_text_length;        /*!< text length */
  uint8_t i_event_name[256];    /*!< "User event" name */
  uint8_t i_text[256];          /*!< "User event" text */

} dvbpsi_user_event_dr_t;


/*****************************************************************************
 * dvbpsi_DecodeUserEventDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_user_event_dr_t * dvbpsi_DecodeUserEventDr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief "User event" descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "User event" descriptor structure which
 * contains the decoded data.
 */
dvbpsi_user_event_dr_t* dvbpsi_DecodeUserEventDr(dvbpsi_descriptor_t * p_descriptor);


/*****************************************************************************
 * dvbpsi_GenUserEventDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t * dvbpsi_GenUserEventDr(
                        dvbpsi_user_event_dr_t * p_decoded, int b_duplicate)
 * \brief "User event" descriptor generator.
 * \param p_decoded pointer to a decoded "video stream" descriptor structure
 * \param b_duplicate if 1 then duplicate the p_decoded structure into
 * the descriptor
 * \return a pointer to a new descriptor structure which contains encoded data.
 */
dvbpsi_descriptor_t * dvbpsi_GenUserEventDr(dvbpsi_user_event_dr_t * p_decoded,
                                             int b_duplicate);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_80.h"
#endif

