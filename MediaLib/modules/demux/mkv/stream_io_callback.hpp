/*****************************************************************************
 * stream_io_callback.hpp : matroska demuxer
 *****************************************************************************/
#include "mkv.hpp"

/*****************************************************************************
 * Stream managment
 *****************************************************************************/
class vlc_stream_io_callback: public IOCallback
{
  private:
    stream_t       *s;
    bool           mb_eof;
    bool           b_owner;

  public:
    vlc_stream_io_callback( stream_t *, bool owner );

    virtual ~vlc_stream_io_callback()
    {
        if( b_owner )
            vlc_stream_Delete( s );
    }

    bool IsEOF() const { return mb_eof; }

    virtual uint32   read            ( void *p_buffer, size_t i_size);
    virtual void     setFilePointer  ( int64_t i_offset, seek_mode mode = seek_beginning );
    virtual size_t   write           ( const void *p_buffer, size_t i_size);
    virtual uint64   getFilePointer  ( void );
    virtual void     close           ( void ) { return; }
    uint64           toRead          ( void );
};

