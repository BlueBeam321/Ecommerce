/*****************************************************************************
 * minibox.h: minimal mp4 box iterator
 *****************************************************************************/

typedef struct
{
    const uint8_t *p_buffer;
    size_t i_buffer;
    const uint8_t *p_payload;
    size_t i_payload;
    vlc_fourcc_t i_type;
} mp4_box_iterator_t;

static void mp4_box_iterator_Init( mp4_box_iterator_t *p_it,
                                   const uint8_t *p_data, size_t i_data )
{
    p_it->p_buffer = p_data;
    p_it->i_buffer = i_data;
}

static bool mp4_box_iterator_Next( mp4_box_iterator_t *p_it )
{
    while( p_it->i_buffer > 8 )
    {
        const uint8_t *p = p_it->p_buffer;
        const size_t i_size = GetDWBE( p );
        p_it->i_type = VLC_FOURCC(p[4], p[5], p[6], p[7]);
        if( i_size >= 8 && i_size <= p_it->i_buffer )
        {
            p_it->p_payload = &p_it->p_buffer[8];
            p_it->i_payload = i_size - 8;
            /* update for next run */
            p_it->p_buffer += i_size;
            p_it->i_buffer -= i_size;
            return true;
        }
        else break;
    }
    return false;
}
