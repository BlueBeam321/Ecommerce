
/*****************************************************************************
 * mkv.cpp : matroska demuxer
 *****************************************************************************/
#ifndef VLC_MKV_EBML_PARSER_HPP_
#define VLC_MKV_EBML_PARSER_HPP_

#include "mkv.hpp"

/*****************************************************************************
 * Ebml Stream parser
 *****************************************************************************/
class EbmlParser
{
  public:
    EbmlParser( EbmlStream *es, EbmlElement *el_start, demux_t *p_demux );
    ~EbmlParser( void );

    void reconstruct( EbmlStream*, EbmlElement*, demux_t*);

    void Up( void );
    void Down( void );
    void Reset( demux_t *p_demux );
    EbmlElement *Get( bool allow_overshoot = true );
    void        Keep( void );
    void        Unkeep( void );

    int  GetLevel( void ) const;

    /* Is the provided element presents in our upper elements */
    bool IsTopPresent( EbmlElement * ) const;

  private:
    static const int M_EL_MAXSIZE = 10;

    demux_t     *p_demux;
    EbmlStream  *m_es;
    int          mi_level;
    EbmlElement *m_el[M_EL_MAXSIZE];

    EbmlElement *m_got;

    int          mi_user_level;
    bool         mb_keep;
    /* Allow dummy/unknown EBML elements */
    bool         mb_dummy;
};

/* This class works around a bug in KaxBlockVirtual implementation */
class KaxBlockVirtualWorkaround : public KaxBlockVirtual
{
public:
    void Fix()
    {
        if( GetBuffer() == DataBlock )
            SetBuffer( NULL, 0 );
    }
};

#endif
