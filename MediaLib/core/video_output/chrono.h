/*****************************************************************************
 * chrono.h: vout chrono
 *****************************************************************************/

#ifndef LIBVLC_VOUT_CHRONO_H
#define LIBVLC_VOUT_CHRONO_H

#include <assert.h>

typedef struct {
    int     shift;
    mtime_t avg;
    mtime_t avg_initial;

    int     shift_var;
    mtime_t var;

    mtime_t start;
} vout_chrono_t;

static inline void vout_chrono_Init(vout_chrono_t *chrono, int shift, mtime_t avg_initial)
{
    chrono->shift       = shift;
    chrono->avg_initial =
    chrono->avg         = avg_initial;

    chrono->shift_var   = shift+1;
    chrono->var         = avg_initial / 2;

    chrono->start = VLC_TS_INVALID;
}
static inline void vout_chrono_Clean(vout_chrono_t *chrono)
{
    VLC_UNUSED(chrono);
}
static inline void vout_chrono_Start(vout_chrono_t *chrono)
{
    chrono->start = mdate();
}
static inline mtime_t vout_chrono_GetHigh(vout_chrono_t *chrono)
{
    return chrono->avg + 2 * chrono->var;
}
static inline mtime_t vout_chrono_GetLow(vout_chrono_t *chrono)
{
    return __MAX(chrono->avg - 2 * chrono->var, 0);
}

static inline void vout_chrono_Stop(vout_chrono_t *chrono)
{
    assert(chrono->start != VLC_TS_INVALID);

    /* */
    const mtime_t duration = mdate() - chrono->start;
    const mtime_t var = llabs( duration - chrono->avg );

    /* Update average only if the current point is 'valid' */
    if( duration < vout_chrono_GetHigh( chrono ) )
        chrono->avg = (((1 << chrono->shift) - 1) * chrono->avg + duration) >> chrono->shift;
    /* Always update the variance */
    chrono->var = (((1 << chrono->shift_var) - 1) * chrono->var + var) >> chrono->shift_var;

    /* For assert */
    chrono->start = VLC_TS_INVALID;
}
static inline void vout_chrono_Reset(vout_chrono_t *chrono)
{
    vout_chrono_t ch = *chrono;
    vout_chrono_Clean(chrono);
    vout_chrono_Init(chrono, ch.shift, ch.avg_initial);
}

#endif
