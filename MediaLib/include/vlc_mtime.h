/*****************************************************************************
 * vlc_mtime.h: high resolution time management functions
 *****************************************************************************/

#ifndef __VLC_MTIME_H
# define __VLC_MTIME_H 1

/*****************************************************************************
 * LAST_MDATE: date which will never happen
 *****************************************************************************
 * This date can be used as a 'never' date, to mark missing events in a function
 * supposed to return a date, such as nothing to display in a function
 * returning the date of the first image to be displayed. It can be used in
 * comparaison with other values: all existing dates will be earlier.
 *****************************************************************************/
#define LAST_MDATE ((mtime_t)((uint64_t)(-1)/2))

/*****************************************************************************
 * MSTRTIME_MAX_SIZE: maximum possible size of mstrtime
 *****************************************************************************
 * This values is the maximal possible size of the string returned by the
 * mstrtime() function, including '-' and the final '\0'. It should be used to
 * allocate the buffer.
 *****************************************************************************/
#define MSTRTIME_MAX_SIZE 22

/*****************************************************************************
 * Prototypes
 *****************************************************************************/
VLC_API char * secstotimestr( char *psz_buffer, int32_t secs );

/*****************************************************************************
 * date_t: date incrementation without long-term rounding errors
 *****************************************************************************/
struct date_t
{
    mtime_t  date;
    uint32_t i_divider_num;
    uint32_t i_divider_den;
    uint32_t i_remainder;
};

VLC_API void date_Init( date_t *, uint32_t, uint32_t );
VLC_API void date_Change( date_t *, uint32_t, uint32_t );
VLC_API void date_Set( date_t *, mtime_t );
VLC_API mtime_t date_Get( const date_t * );
VLC_API void date_Move( date_t *, mtime_t );
VLC_API mtime_t date_Increment( date_t *, uint32_t );
VLC_API mtime_t date_Decrement( date_t *, uint32_t );
VLC_API uint64_t NTPtime64( void );
#endif /* !__VLC_MTIME_ */
