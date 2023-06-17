/*****************************************************************************
 * dr_80.c
 * Copyright (C) 2016-2019 IT02 MultiLab
 * $Id: dr_80.c,v 1.7 2003/07/25 20:20:40 fenrir Exp $
 *
 * Authors: Code
 *
 * This library is created by Code;
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

#include "dr_80.h"

/*****************************************************************************
 * dvbpsi_DecodeUserEventDr
 *****************************************************************************/
dvbpsi_user_event_dr_t * dvbpsi_DecodeUserEventDr(dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_user_event_dr_t * p_decoded;
  int i_len1;
  int i_len2;

  /* Check the tag */
  if (!dvbpsi_CanDecodeAsDescriptor(p_descriptor, 0x80) ||
      p_descriptor->i_length < 5 )
    return NULL;

  /* Don't decode twice */
  if (dvbpsi_IsDescriptorDecoded(p_descriptor))
     return p_descriptor->p_decoded;

  /* Check length */
  i_len1 = p_descriptor->p_data[3];
  i_len2 = p_descriptor->p_data[4+i_len1];

  if (p_descriptor->i_length < 5 + i_len1 + i_len2)
    return NULL;

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded = malloc(sizeof(dvbpsi_user_event_dr_t));
  if (!p_decoded)
      return NULL;

  /* Decode data and check the length */
  memcpy( p_decoded->i_iso_639_code, &p_descriptor->p_data[0], 3 );
  p_decoded->i_event_name_length = i_len1;
  if (i_len1 > 0)
      memcpy( p_decoded->i_event_name, &p_descriptor->p_data[3+1], i_len1 );
  p_decoded->i_text_length = i_len2;
  if (i_len2 > 0)
      memcpy( p_decoded->i_text, &p_descriptor->p_data[4+i_len1+1], i_len2 );

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenUserEventDr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenUserEventDr(dvbpsi_user_event_dr_t * p_decoded,
                                             int b_duplicate)
{
	uint8_t i_len1 = p_decoded->i_event_name_length;
	uint8_t i_len2 = p_decoded->i_text_length;

    /* Create the descriptor */

	dvbpsi_descriptor_t * p_descriptor = dvbpsi_NewDescriptor(0x80, 5 + i_len1 + i_len2, NULL );

    if (!p_descriptor)
        return NULL;

    /* Encode data */
    memcpy( &p_descriptor->p_data[0], p_decoded->i_iso_639_code, 3 );


	p_descriptor->p_data[3] = i_len1;
	if (i_len1)
		memcpy( &p_descriptor->p_data[4], p_decoded->i_event_name, i_len1 );
	p_descriptor->p_data[3+1+i_len1] = i_len2;
	if (i_len2)
		memcpy( &p_descriptor->p_data[3+1+i_len1+1], p_decoded->i_text, i_len2 );

    if (b_duplicate)
    {
        /* Duplicate decoded data */
        p_descriptor->p_decoded =
                dvbpsi_DuplicateDecodedDescriptor(p_decoded,
                                                  sizeof(dvbpsi_user_event_dr_t));
    }

    return p_descriptor;
}
