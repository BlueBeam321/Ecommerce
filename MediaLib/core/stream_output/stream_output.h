/*****************************************************************************
 * stream_output.h : internal stream output
 ***************************************************************************/

#ifndef VLC_SRC_STREAMOUT_H
# define VLC_SRC_STREAMOUT_H 1

# include <vlc_sout.h>
# include <vlc_network.h>

/****************************************************************************
 * sout_packetizer_input_t: p_sout <-> p_packetizer
 ****************************************************************************/
struct sout_packetizer_input_t
{
    sout_instance_t     *p_sout;

    sout_stream_id_sys_t    *id;
};

sout_instance_t *sout_NewInstance( vlc_object_t *, const char * );
#define sout_NewInstance(a,b) sout_NewInstance(VLC_OBJECT(a),b)
void sout_DeleteInstance( sout_instance_t * );

sout_packetizer_input_t *sout_InputNew( sout_instance_t *, const es_format_t * );
int sout_InputDelete( sout_packetizer_input_t * );
int sout_InputSendBuffer( sout_packetizer_input_t *, block_t* );
bool sout_InputIsEmpty(sout_packetizer_input_t *);
void sout_InputFlush( sout_packetizer_input_t * );

#endif
