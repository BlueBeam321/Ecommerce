/*****************************************************************************
 * statistic.h : vout statistic
 *****************************************************************************/

#ifndef LIBVLC_VOUT_STATISTIC_H
# define LIBVLC_VOUT_STATISTIC_H
# include <vlc_atomic.h>

/* NOTE: Both statistics are atomic on their own, so one might be older than
 * the other one. Currently, only one of them is updated at a time, so this
 * is a non-issue. */
typedef struct {
    atomic_uint displayed;
    atomic_uint lost;
} vout_statistic_t;

static inline void vout_statistic_Init(vout_statistic_t *stat)
{
    atomic_init(&stat->displayed, 0);
    atomic_init(&stat->lost, 0);
}

static inline void vout_statistic_Clean(vout_statistic_t *stat)
{
    (void) stat;
}

static inline void vout_statistic_GetReset(vout_statistic_t *stat,
                                           unsigned * displayed,
                                           unsigned * lost)
{
    *displayed = atomic_exchange(&stat->displayed, 0);
    *lost      = atomic_exchange(&stat->lost, 0);
}

static inline void vout_statistic_AddDisplayed(vout_statistic_t *stat,
                                               int displayed)
{
    atomic_fetch_add(&stat->displayed, displayed);
}

static inline void vout_statistic_AddLost(vout_statistic_t *stat, int lost)
{
    atomic_fetch_add(&stat->lost, lost);
}

#endif
