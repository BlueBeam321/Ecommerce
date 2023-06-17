/*****************************************************************************
 * virtual_segment.hpp : virtual segment implementation in the MKV demuxer
 *****************************************************************************/

#ifndef VLC_MKV_VIRTUAL_SEGMENT_HPP_
#define VLC_MKV_VIRTUAL_SEGMENT_HPP_

#include "mkv.hpp"

#include "matroska_segment.hpp"
#include "chapters.hpp"

/* virtual classes don't own anything but virtual elements so they shouldn't have to delete anything */

class virtual_chapter_c
{
public:
    virtual_chapter_c( matroska_segment_c &seg, chapter_item_c *p_chap, int64_t start, int64_t stop, std::vector<virtual_chapter_c *> & sub_chaps ):
        segment(seg), p_chapter(p_chap),
        i_mk_virtual_start_time(start), i_mk_virtual_stop_time(stop),
        sub_vchapters(sub_chaps)
    {}
    ~virtual_chapter_c();

    static virtual_chapter_c * CreateVirtualChapter( chapter_item_c * p_chap,
                                                     matroska_segment_c & main_segment,
                                                     std::vector<matroska_segment_c*> & segments,
                                                     int64_t & usertime_offset, bool b_ordered );

    virtual_chapter_c* getSubChapterbyTimecode( int64_t time );
    bool Leave( );
    bool EnterAndLeave( virtual_chapter_c *p_leaving_vchapter, bool b_enter = true );
    virtual_chapter_c * FindChapter( int64_t i_find_uid );
    int PublishChapters( input_title_t & title, int & i_user_chapters, int i_level, bool allow_no_name );

    virtual_chapter_c * BrowseCodecPrivate( unsigned int codec_id,
                                            bool (*match)( const chapter_codec_cmds_c &data,
                                                           const void *p_cookie,
                                                           size_t i_cookie_size ),
                                            const void *p_cookie,
                                            size_t i_cookie_size );
    bool Enter( bool b_do_subs );
    bool Leave( bool b_do_subs );

    static bool CompareTimecode( const virtual_chapter_c * itemA, const virtual_chapter_c * itemB )
    {
        return ( itemA->i_mk_virtual_start_time < itemB->i_mk_virtual_start_time );
    }

    bool ContainsTimestamp( mtime_t i_pts );

    matroska_segment_c  &segment;
    chapter_item_c      *p_chapter;
    mtime_t             i_mk_virtual_start_time;
    mtime_t             i_mk_virtual_stop_time;
    int                 i_seekpoint_num;
    std::vector<virtual_chapter_c *> sub_vchapters;
};

class virtual_edition_c
{
public:
    virtual_edition_c( chapter_edition_c * p_edition, matroska_segment_c & main_segment, std::vector<matroska_segment_c*> & opened_segments );
    ~virtual_edition_c();
    std::vector<virtual_chapter_c*> vchapters;

    virtual_chapter_c* getChapterbyTimecode( int64_t time );
    std::string GetMainName();
    int PublishChapters( input_title_t & title, int & i_user_chapters, int i_level );
    virtual_chapter_c * BrowseCodecPrivate( unsigned int codec_id,
                                            bool (*match)( const chapter_codec_cmds_c &data,
                                                           const void *p_cookie,
                                                           size_t i_cookie_size ),
                                             const void *p_cookie, size_t i_cookie_size );

    bool                b_ordered;
    mtime_t             i_duration;
    chapter_edition_c   *p_edition;
    int                 i_seekpoint_num;

private:
    void retimeChapters();
    void retimeSubChapters(virtual_chapter_c * p_vchap);
};

// class holding hard-linked segment together in the playback order
class virtual_segment_c
{
public:
    virtual_segment_c( matroska_segment_c & segment, std::vector<matroska_segment_c*> & opened_segments );
    ~virtual_segment_c();
    std::vector<virtual_edition_c*> veditions;
    std::vector<virtual_edition_c*>::size_type i_current_edition;
    virtual_chapter_c               *p_current_vchapter;
    bool                            b_current_vchapter_entered;
    int                             i_sys_title;


    inline virtual_edition_c * CurrentEdition()
    {
        if( i_current_edition < veditions.size() )
            return veditions[i_current_edition];
        return NULL;
    }

    virtual_chapter_c * CurrentChapter() const
    {
        return p_current_vchapter;
    }

    matroska_segment_c * CurrentSegment() const
    {
        if ( !p_current_vchapter )
            return NULL;
        return &p_current_vchapter->segment;
    }

    inline int64_t Duration()
    {
        return veditions[i_current_edition]->i_duration / 1000;
    }

    inline std::vector<virtual_edition_c*>* Editions() { return &veditions; }

    virtual_chapter_c *BrowseCodecPrivate( unsigned int codec_id,
                                           bool (*match)( const chapter_codec_cmds_c &data,
                                                          const void *p_cookie,
                                                          size_t i_cookie_size ),
                                           const void *p_cookie,
                                           size_t i_cookie_size );

    virtual_chapter_c * FindChapter( int64_t i_find_uid );

    bool UpdateCurrentToChapter( demux_t & demux );
    bool Seek( demux_t & demuxer, mtime_t i_mk_date, virtual_chapter_c *p_vchapter, bool b_precise = true );
private:
    void KeepTrackSelection( matroska_segment_c & old, matroska_segment_c & next );
};

#endif
