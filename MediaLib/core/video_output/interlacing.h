/*****************************************************************************
 * interlacing.h: Interlacing related helpers
 *****************************************************************************/

#ifndef LIBVLC_VOUT_INTERLACING_H
#define LIBVLC_VOUT_INTERLACING_H

void vout_InitInterlacingSupport(vout_thread_t *, bool is_interlaced);
void vout_ReinitInterlacingSupport(vout_thread_t *);
void vout_SetInterlacingState(vout_thread_t *, bool is_interlaced);

#endif
