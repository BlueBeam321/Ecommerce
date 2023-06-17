/*
 * api.c
 */

#ifdef __ANDROID__
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <android/log.h>

#include <vlc/vlc.h>
#include <libvlc_internal.h>

#define  LOG_TAG    "LIBSTB"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#define DECL_NAME(CLZ, FUN) Java_##CLZ##_##FUN
#define DECL_FUNC(FUN) DECL_NAME(com_it02_libvlc_MediaPlayer, FUN)

#define TITLE_PREFIX        "com/it02/libvlc/media/Title"
#define PROGRAM_PREFIX      "com/it02/libvlc/media/Program"
#define CHAPTER_PREFIX      "com/it02/libvlc/media/Chapter"
#define TRACK_PREFIX        "com/it02/libvlc/media/Track"
#define EPG_EVENT_PREFIX    "com/it02/libvlc/media/EPGEvent"
#define STATS_PREFIX        "com/it02/libvlc/media/Stats"
#define STREAM_TIME_PREFIX  "com/it02/libvlc/media/StreamTime"

char* _PublicKey = "dzcVdPwNcjgNejgSdzgRdzAOdPkOdP5TcjsQd4gTcjkMczoUe41TejgVdzgVd4AVe4oUcPoOczgMc4wSe4oUe4gQcPoOcjwScP9RdzoUczANcjsUcP9SdjgQejgQdz5OcjwQczsRej5NcPkUc4wTcjAUdj1UcPcMcjgVc4oTc4oNe41OdzgUc4cPczcOczsUdzcTdPkUejsUczsUczwVczsNczwMc4oPdjgQc4wNe4oRdjAOejsTc4kMcPwVcP1VdjwTdjcUcPcOe4gSd4wQczsNdjkRcPsNd4cMczsOd4kPc4gUcjsUdPcSd45NcjsPczwOdjoNdjAScj9TczoMdz9Qe41UcjgScjkPc49ScPkNdP5Te4gSdj1OdPkRej1TczgTejAQdPsQcjgQejETc49Pej5NcjsQejgRdPkMdzAPe4ATdjcTdPsOd4sOdjgPcP1PdzsVejwQd4APe41PdPsVdPwTe4sQczcPc4kOcPgRcj5PcP9Odj1TdPwTcPkNcz1NdzgQcP1NdPoPcPsTdj9Ue4sSc4AMdj9Vc49VdzkUe4oScj5SczcRdPAPczwQej9Mc4ARcz9Qdj1ScPwMdP1VcjAMdP9Rd4gUc4oMdjAQejsNdzkUe4gMcj9NdzARcP5TdPAUcjsOejcRdjcMdP9Od4cRdPAOc45VdjwQd4sOcjcMdjwUdj1SejoOd4cOdzoTcj5NcjkQdj5QdPARcj9OdjANcz5Tc4kRe41Nc41RdPAPc4oUdz9Td45Uc4kRc4cNejsPd4cOej1VdjwMdz1Qc4wTdjATe45T";

typedef struct java_title_entry_t {
    jclass clazz;
    jfieldID id;
    jfieldID name;
} java_title_entry_t;

typedef struct java_program_entry_t {
    jclass clazz;
    jfieldID id;
    jfieldID name;
} java_program_entry_t;

typedef struct java_chapter_entry_t {
    jclass clazz;
    jfieldID id;
    jfieldID name;
    //jfieldID start;
    //jfieldID duration;
} java_chapter_entry_t;

typedef struct java_track_entry_t {
    jclass clazz;
    jfieldID id;
    jfieldID name;
} java_track_entry_t;

typedef struct java_epg_entry_t {
    jclass clazz;
    jfieldID start;
    jfieldID duration;
    jfieldID style;
    jfieldID title;
    jfieldID content;
} java_epg_entry_t;

typedef struct java_stats_entry_t {
    jclass clazz;
    /* Input */
    jfieldID inputReadBytes;
    jfieldID inputBitrate;

    /* Demux */
    jfieldID demuxReadBytes;
    jfieldID demuxBitrate;
    jfieldID demuxCorrupted;
    jfieldID demuxDiscontinuity;

    /* Decoders */
    jfieldID decodedVideo;
    jfieldID decodedAudio;

    /* Video Output */
    jfieldID displayedPictures;
    jfieldID lostPictures;

    /* Audio Output */
    jfieldID playedAudioBuffers;
    jfieldID lostAudioBuffers;
} java_stats_entry_t;


typedef struct java_stream_time_entry_t {
    jclass clazz;
    jfieldID year;
    jfieldID month;
    jfieldID day;
    jfieldID hour;
    jfieldID minute;
    jfieldID second;
} java_stream_time_entry_t;

static libvlc_instance_t* _vlc = NULL;
static libvlc_media_player_t* _player = NULL; // Media Player Object
static libvlc_media_t* _media = NULL; // Parsed Media
static jobject _window = NULL;
static java_title_entry_t _title_entry;
static java_program_entry_t _program_entry;
static java_chapter_entry_t _chapter_entry;
static java_track_entry_t _track_entry;
static java_epg_entry_t _epg_entry;
static java_stats_entry_t _stats_entry;
static java_stream_time_entry_t _stream_time_entry;
static char _machine_id[256] = {0};

static int GetUnicodeLength(uint16_t *p)
{
    int len = 0;
    while (*p++) len++;
    return len;
}
/*
 * JNI function to init player library.
 */
JNIEXPORT void JNICALL DECL_FUNC(nativeInit)(JNIEnv* env, jobject clazz)
{
    jclass clz;

    LOGI("nativeInit");
    _vlc = libvlc_new();
    if (!_vlc)
        LOGI("libvlc_new: Failed!!!");

    /* Initialize Title */
    memset(&_track_entry, 0, sizeof(java_title_entry_t));
    clz = (*env)->FindClass(env, TITLE_PREFIX);
    _title_entry.clazz = (*env)->NewGlobalRef(env, clz);
    _title_entry.id = (*env)->GetFieldID(env, _title_entry.clazz, "id", "I");
    _title_entry.name = (*env)->GetFieldID(env, _title_entry.clazz, "name", "Ljava/lang/String;");

    /* Initialize Program */
    memset(&_program_entry, 0, sizeof(java_program_entry_t));
    clz = (*env)->FindClass(env, PROGRAM_PREFIX);
    _program_entry.clazz = (*env)->NewGlobalRef(env, clz);
    _program_entry.id = (*env)->GetFieldID(env, _program_entry.clazz, "id", "I");
    _program_entry.name = (*env)->GetFieldID(env, _program_entry.clazz, "name", "Ljava/lang/String;");

    /* Initialize Chapter */
    memset(&_chapter_entry, 0, sizeof(java_chapter_entry_t));
    clz = (*env)->FindClass(env, CHAPTER_PREFIX);
    _chapter_entry.clazz = (*env)->NewGlobalRef(env, clz);
    _chapter_entry.id = (*env)->GetFieldID(env, _chapter_entry.clazz, "id", "I");
    _chapter_entry.name = (*env)->GetFieldID(env, _chapter_entry.clazz, "name", "Ljava/lang/String;");
    //_chapter_entry.start = (*env)->GetFieldID(env, _chapter_entry.clazz, "start", "J");
    //_chapter_entry.duration = (*env)->GetFieldID(env, _chapter_entry.clazz, "duration", "J");

    /* Initialize Track */
    memset(&_track_entry, 0, sizeof(java_track_entry_t));
    clz = (*env)->FindClass(env, TRACK_PREFIX);
    _track_entry.clazz = (*env)->NewGlobalRef(env, clz);
    _track_entry.id = (*env)->GetFieldID(env, _track_entry.clazz, "id", "I");
    _track_entry.name = (*env)->GetFieldID(env, _track_entry.clazz, "name", "Ljava/lang/String;");

    /* Initialize EPG Event */
    memset(&_epg_entry, 0, sizeof(java_epg_entry_t));
    clz = (*env)->FindClass(env, EPG_EVENT_PREFIX);
    _epg_entry.clazz = (*env)->NewGlobalRef(env, clz);
    _epg_entry.start = (*env)->GetFieldID(env, _epg_entry.clazz, "start", "Ljava/lang/String;");
    _epg_entry.duration = (*env)->GetFieldID(env, _epg_entry.clazz, "duration", "Ljava/lang/String;");
    _epg_entry.style  = (*env)->GetFieldID(env, _epg_entry.clazz, "style", "Ljava/lang/String;");
    _epg_entry.title = (*env)->GetFieldID(env, _epg_entry.clazz, "title", "Ljava/lang/String;");
    _epg_entry.content = (*env)->GetFieldID(env, _epg_entry.clazz, "content", "Ljava/lang/String;");

    /* Initialize Stats */
    memset(&_stats_entry, 0, sizeof(java_stats_entry_t));
    clz = (*env)->FindClass(env, STATS_PREFIX);
    _stats_entry.clazz = (*env)->NewGlobalRef(env, clz);
    _stats_entry.inputReadBytes = (*env)->GetFieldID(env, _stats_entry.clazz, "inputReadBytes", "I");
    _stats_entry.inputBitrate = (*env)->GetFieldID(env, _stats_entry.clazz, "inputBitrate", "F");
    _stats_entry.demuxReadBytes  = (*env)->GetFieldID(env, _stats_entry.clazz, "demuxReadBytes", "I");
    _stats_entry.demuxBitrate = (*env)->GetFieldID(env, _stats_entry.clazz, "demuxBitrate", "F");
    _stats_entry.demuxCorrupted = (*env)->GetFieldID(env, _stats_entry.clazz, "demuxCorrupted", "I");
    _stats_entry.demuxDiscontinuity = (*env)->GetFieldID(env, _stats_entry.clazz, "demuxDiscontinuity", "I");
    _stats_entry.decodedVideo = (*env)->GetFieldID(env, _stats_entry.clazz, "decodedVideo", "I");
    _stats_entry.decodedAudio = (*env)->GetFieldID(env, _stats_entry.clazz, "decodedAudio", "I");
    _stats_entry.displayedPictures = (*env)->GetFieldID(env, _stats_entry.clazz, "displayedPictures", "I");
    _stats_entry.lostPictures = (*env)->GetFieldID(env, _stats_entry.clazz, "lostPictures", "I");
    _stats_entry.playedAudioBuffers = (*env)->GetFieldID(env, _stats_entry.clazz, "playedAudioBuffers", "I");
    _stats_entry.lostAudioBuffers = (*env)->GetFieldID(env, _stats_entry.clazz, "lostAudioBuffers", "I");

    /* Initialize Stream Time */
    memset(&_stream_time_entry, 0, sizeof(java_stream_time_entry_t));
    clz = (*env)->FindClass(env, STREAM_TIME_PREFIX);
    _stream_time_entry.clazz = (*env)->NewGlobalRef(env, clz);
    _stream_time_entry.year = (*env)->GetFieldID(env, _stream_time_entry.clazz, "year", "I");
    _stream_time_entry.month = (*env)->GetFieldID(env, _stream_time_entry.clazz, "month", "I");
    _stream_time_entry.day = (*env)->GetFieldID(env, _stream_time_entry.clazz, "day", "I");
    _stream_time_entry.hour = (*env)->GetFieldID(env, _stream_time_entry.clazz, "hour", "I");
    _stream_time_entry.minute = (*env)->GetFieldID(env, _stream_time_entry.clazz, "minute", "I");
    _stream_time_entry.second = (*env)->GetFieldID(env, _stream_time_entry.clazz, "second", "I");
}

/*
 * JNI function to deinitialize player library.
 */
JNIEXPORT void JNICALL DECL_FUNC(nativeDeinit)(JNIEnv* env, jobject clazz)
{
    LOGI("nativeDeinit");
    if (_vlc)
    {
        libvlc_release(_vlc);
        _vlc = NULL;
    }
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetVideoOutput)(JNIEnv* env, jobject clazz, jobject jwindow)
{
    LOGI("nativeSetVideoOutput");
    _window = (*env)->NewGlobalRef(env, jwindow);
}

JNIEXPORT jint JNICALL DECL_FUNC(nativeOpen)(JNIEnv* env, jobject clazz, jstring url)
{
    LOGI("nativeOpen");
    char* szUrl = (*env)->GetStringUTFChars(env, url, NULL);
    if (_vlc)
    {
        libvlc_media_t* media;
        if (strncmp(szUrl, "rtsp://", 7) && strncmp(szUrl, "rtp://", 6) && strncmp(szUrl, "vod://", 6))
            media = libvlc_media_new_path(_vlc, szUrl);
        else
            media = libvlc_media_new_location(_vlc, szUrl);
#if CRYPT_ENABLE
        libvlc_media_add_option(media, "TWGRAFSH4PH04A9SYA2812TSEGRAZR529ZUFQZ9Q5XCWVLGV6LKBZGUBVBCC35HLF9GV9CDRV0VUC");
#else
        libvlc_media_add_option(media, ":codec=mediacodec_it02_jni,iomx,all");
#endif
        _player = libvlc_media_player_new_from_media(media);
        libvlc_media_release(media);

        if (_window)
            libvlc_media_player_set_android_context(_player, _window);
        LOGI("nativeOpen success!!!");
        return 0;
    }

    LOGI("nativeOpen failed!!!");
    return -1;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeClose)(JNIEnv* env, jobject clazz)
{
    LOGI("nativeClose");
    if (_player)
    {
        libvlc_media_player_stop(_player);
        libvlc_media_player_release(_player);
        _player = NULL;
    }
}

JNIEXPORT int JNICALL DECL_FUNC(nativePlay)(JNIEnv* env, jobject clazz)
{
    LOGI("nativePlay");
    if (_player)
    {
        libvlc_state_t state = libvlc_media_player_get_state(_player);
        if (state != libvlc_Playing)
            libvlc_media_player_play(_player);

        do {
            state = libvlc_media_player_get_state(_player);
        } while (state != libvlc_Playing && state != libvlc_Error && state != libvlc_Ended);

        if (state != libvlc_Error)
            return 0;
    }

    return -1;
}

JNIEXPORT void JNICALL DECL_FUNC(nativePause)(JNIEnv* env, jobject clazz)
{
    LOGI("nativePause");
    if (_player)
    {
        libvlc_state_t state = libvlc_media_player_get_state(_player);
        if (state != libvlc_Paused)
            libvlc_media_player_pause(_player);

        do {
            state = libvlc_media_player_get_state(_player);
        } while (state != libvlc_Paused && state != libvlc_Error && state != libvlc_Ended);
    }
}

JNIEXPORT jboolean JNICALL DECL_FUNC(nativeIsPlaying)(JNIEnv* env, jobject clazz)
{
    if (_player)
        return libvlc_media_player_is_playing(_player);
    return 0;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetTime)(JNIEnv* env, jobject clazz, int64_t timeMs)
{
    if (_player)
        libvlc_media_player_set_time(_player, timeMs);
}


JNIEXPORT void JNICALL DECL_FUNC(nativeSetPosition)(JNIEnv* env, jobject clazz, float fPos)
{
    if (_player)
        libvlc_media_player_set_position(_player, fPos);
}

JNIEXPORT int64_t JNICALL DECL_FUNC(nativeGetCurrentTime)(JNIEnv* env, jobject clazz)
{
    if (_player)
        return libvlc_media_player_get_time(_player);
    return 0;
}

JNIEXPORT int64_t JNICALL DECL_FUNC(nativeGetTotalTime)(JNIEnv* env, jobject clazz)
{
    if (_player)
        return libvlc_media_player_get_length(_player);
    return 0;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetVolume)(JNIEnv* env, jobject clazz, int volume)
{
    if (_player)
        libvlc_audio_set_volume(_player, volume);
}

JNIEXPORT jobjectArray JNICALL DECL_FUNC(nativeGetTitles)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        libvlc_title_description_t **pp_titles;
        int i_titles = libvlc_media_player_get_full_title_descriptions(_player, &pp_titles);
        if (i_titles > 0)
        {
            jobjectArray titleArray;
            jobject objEntry;
            jstring jtitleName;

            titleArray = (*env)->NewObjectArray(env, i_titles, _title_entry.clazz, NULL);
            for (int i = 0; i < i_titles; i++)
            {
                objEntry = (*env)->AllocObject(env, _title_entry.clazz);
                jtitleName = (*env)->NewStringUTF(env, pp_titles[i]->psz_name);

                (*env)->SetIntField(env, objEntry, _title_entry.id, i);
                (*env)->SetObjectField(env, objEntry, _title_entry.name, jtitleName);

                (*env)->SetObjectArrayElement(env, titleArray, i, objEntry);
                (*env)->DeleteLocalRef(env, jtitleName);
                (*env)->DeleteLocalRef(env, objEntry);
            }
            libvlc_title_descriptions_release(pp_titles, i_titles);

            return titleArray;
        }
    }

    return NULL;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetTitle)(JNIEnv* env, jobject clazz, int titleId)
{
    if (_player)
        libvlc_media_player_set_title(_player, titleId);
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetVodTitleCount)(JNIEnv* env, jobject clazz, int count)
{
    if (_player)
        libvlc_media_player_set_vod_title_count(_player, count);
}

JNIEXPORT jobjectArray JNICALL DECL_FUNC(nativeGetPrograms)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        int programCount = libvlc_media_player_get_program_count(_player);
        if (programCount > 0)
        {
            int id;
            char name[256];
            jobjectArray programArray;
            jobject objEntry;
            jstring jprogramName;

            programArray = (*env)->NewObjectArray(env, programCount, _program_entry.clazz, NULL);
            for (int i = 0; i < programCount; i++)
            {
                memset(name, 0, 256);
                libvlc_media_player_get_program_info(_player, i, &id, name);

                objEntry = (*env)->AllocObject(env, _program_entry.clazz);
                jprogramName = (*env)->NewStringUTF(env, name);
                //jprogramName = (*env)->NewString(env, name, GetUnicodeLength(name));

                (*env)->SetIntField(env, objEntry, _program_entry.id, id);
                (*env)->SetObjectField(env, objEntry, _program_entry.name, jprogramName);

                (*env)->SetObjectArrayElement(env, programArray, i, objEntry);
                (*env)->DeleteLocalRef(env, jprogramName);
                (*env)->DeleteLocalRef(env, objEntry);
            }

            return programArray;
        }
    }

    return NULL;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetProgram)(JNIEnv* env, jobject clazz, int programId)
{
    if (_player)
        libvlc_media_player_change_program(_player, programId);
}

JNIEXPORT jobjectArray JNICALL DECL_FUNC(nativeGetChapters)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        libvlc_chapter_description_t** pp_chapters;
        int i_chapters = libvlc_media_player_get_full_chapter_descriptions(_player, -1, &pp_chapters);
        if (i_chapters > 0)
        {
            jobjectArray chapterArray;
            jobject objEntry;
            jstring jchapterName;

            chapterArray = (*env)->NewObjectArray(env, i_chapters, _chapter_entry.clazz, NULL);
            for (int i = 0; i < i_chapters; i++)
            {
                objEntry = (*env)->AllocObject(env, _chapter_entry.clazz);
                jchapterName = (*env)->NewStringUTF(env, pp_chapters[i]->psz_name);

                (*env)->SetIntField(env, objEntry, _chapter_entry.id, i);
                (*env)->SetObjectField(env, objEntry, _chapter_entry.name, jchapterName);
                //(*env)->SetLongField(env, objEntry, _chapter_entry.start, pp_chapters[i]->i_time_offset);
                //(*env)->SetLongField(env, objEntry, _chapter_entry.duration, pp_chapters[i]->i_duration);

                (*env)->SetObjectArrayElement(env, chapterArray, i, objEntry);
                (*env)->DeleteLocalRef(env, jchapterName);
                (*env)->DeleteLocalRef(env, objEntry);
            }
            libvlc_chapter_descriptions_release(pp_chapters, i_chapters);

            return chapterArray;
        }
    }

    return NULL;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetChapter)(JNIEnv* env, jobject clazz, int chapterId)
{
    if (_player)
        libvlc_media_player_set_chapter(_player, chapterId);
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetVideoTrack)(JNIEnv* env, jobject clazz, int trackId)
{
    if (_player)
        libvlc_video_set_track(_player, trackId);
}

JNIEXPORT jobjectArray JNICALL DECL_FUNC(nativeGetAudioTracks)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        libvlc_track_description_t* p_tracks = libvlc_audio_get_track_description(_player);
        if (p_tracks)
        {
            jobjectArray trackArray;
            jobject objEntry;
            jstring jtrackName;

            int trackCount = 0, i = 0;
            libvlc_track_description_t* p_track = p_tracks;
            while (p_track)
            {
                trackCount++;
                p_track = p_track->p_next;
            }

            trackArray = (*env)->NewObjectArray(env, trackCount, _track_entry.clazz, NULL);
            p_track = p_tracks;
            while (p_track)
            {
                objEntry = (*env)->AllocObject(env, _track_entry.clazz);
                jtrackName = (*env)->NewStringUTF(env, p_track->psz_name);

                (*env)->SetIntField(env, objEntry, _track_entry.id, p_track->i_id);
                (*env)->SetObjectField(env, objEntry, _track_entry.name, jtrackName);

                (*env)->SetObjectArrayElement(env, trackArray, i, objEntry);
                (*env)->DeleteLocalRef(env, jtrackName);
                (*env)->DeleteLocalRef(env, objEntry);
                i++;
                p_track = p_track->p_next;
            }
            libvlc_track_description_list_release(p_tracks);

            return trackArray;
        }
    }

    return NULL;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetAudioTrack)(JNIEnv* env, jobject clazz, int trackId)
{
    if (_player)
        libvlc_audio_set_track(_player, trackId);
}

JNIEXPORT jobjectArray JNICALL DECL_FUNC(nativeGetSubtitleTracks)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        libvlc_track_description_t* p_tracks = libvlc_video_get_spu_description(_player);
        if (p_tracks)
        {
            jobjectArray trackArray;
            jobject objEntry;
            jstring jtrackName;

            int trackCount = 0, i = 0;
            libvlc_track_description_t* p_track = p_tracks;
            while (p_track)
            {
                trackCount++;
                p_track = p_track->p_next;
            }

            trackArray = (*env)->NewObjectArray(env, trackCount, _track_entry.clazz, NULL);
            p_track = p_tracks;
            while (p_track)
            {
                objEntry = (*env)->AllocObject(env, _track_entry.clazz);
                jtrackName = (*env)->NewStringUTF(env, p_track->psz_name);

                (*env)->SetIntField(env, objEntry, _track_entry.id, p_track->i_id);
                (*env)->SetObjectField(env, objEntry, _track_entry.name, jtrackName);

                (*env)->SetObjectArrayElement(env, trackArray, i, objEntry);
                (*env)->DeleteLocalRef(env, jtrackName);
                (*env)->DeleteLocalRef(env, objEntry);
                i++;
                p_track = p_track->p_next;
            }
            libvlc_track_description_list_release(p_tracks);

            return trackArray;
        }
    }

    return NULL;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetSubtitleTrack)(JNIEnv* env, jobject clazz, int mainTrackId, int subTrackId)
{
    if (_player)
    {
        libvlc_video_set_spu(_player, mainTrackId, false);
        libvlc_video_set_spu(_player, subTrackId, true);
    }
}

JNIEXPORT jobjectArray JNICALL DECL_FUNC(nativeGetEPGEvents)(JNIEnv* env, jobject clazz, int programId)
{
    if (_player)
    {
        int epg_count = libvlc_media_player_get_epg_count(_player, programId);
        if (epg_count > 0)
        {
            jobjectArray epgEventArray;
            jobject objEntry;
            char start[64], duration[64];
            jchar style[256], title[256], content[256];
            jstring jstart, jduration, jstyle, jtitle, jcontent;

            //if (epg_count > 30)
            //    epg_count = 30;

            epgEventArray = (*env)->NewObjectArray(env, epg_count, _epg_entry.clazz, NULL);
            for (int i = 0; i < epg_count; i++)
            {
                memset(style, 0, 256 * sizeof(jchar));
                memset(title, 0, 256 * sizeof(jchar));
                memset(content, 0, 256 * sizeof(jchar));
                libvlc_media_player_get_epg_item(_player, programId, i, start, duration, style, title, content);

                objEntry = (*env)->AllocObject(env, _epg_entry.clazz);

                jstart = (*env)->NewStringUTF(env, start);
                jduration = (*env)->NewStringUTF(env, duration);
                jstyle = (*env)->NewStringUTF(env, style);
                jtitle = (*env)->NewStringUTF(env, title);
                jcontent = (*env)->NewStringUTF(env, content);

                (*env)->SetObjectField(env, objEntry, _epg_entry.start, jstart);
                (*env)->SetObjectField(env, objEntry, _epg_entry.duration, jduration);
                (*env)->SetObjectField(env, objEntry, _epg_entry.style, jstyle);
                (*env)->SetObjectField(env, objEntry, _epg_entry.title, jtitle);
                (*env)->SetObjectField(env, objEntry, _epg_entry.content, jcontent);

                (*env)->SetObjectArrayElement(env, epgEventArray, i, objEntry);
                (*env)->DeleteLocalRef(env, jstart);
                (*env)->DeleteLocalRef(env, jduration);
                (*env)->DeleteLocalRef(env, jstyle);
                (*env)->DeleteLocalRef(env, jtitle);
                (*env)->DeleteLocalRef(env, jcontent);
                (*env)->DeleteLocalRef(env, objEntry);
            }

            return epgEventArray;
        }
    }

    return NULL;
}

JNIEXPORT void JNICALL DECL_FUNC(nativeSetVideoAspect)(JNIEnv* env, jobject clazz, int aspect)
{
    if (_player)
    {
        if (aspect == 0) // Auto
            libvlc_video_set_aspect_ratio(_player, "");
        else if (aspect == 1) // 16:9
            libvlc_video_set_aspect_ratio(_player, "16:9");
        else if (aspect == 2) // 4:3
            libvlc_video_set_aspect_ratio(_player, "4:3");
        else if (aspect == 3) // 1:1
            libvlc_video_set_aspect_ratio(_player, "1:1");
    }
}

JNIEXPORT jobject JNICALL DECL_FUNC(nativeGetStats)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        jobject objEntry;
        libvlc_media_t* media = libvlc_media_player_get_media(_player);
        libvlc_media_stats_t stats;
        libvlc_media_get_stats(media, &stats);
        libvlc_media_release(media);

        objEntry = (*env)->AllocObject(env, _stats_entry.clazz);
        (*env)->SetIntField(env, objEntry, _stats_entry.inputReadBytes, stats.i_read_bytes);
        (*env)->SetFloatField(env, objEntry, _stats_entry.inputBitrate, stats.f_input_bitrate);
        (*env)->SetIntField(env, objEntry, _stats_entry.demuxReadBytes, stats.i_demux_read_bytes);
        (*env)->SetFloatField(env, objEntry, _stats_entry.demuxBitrate, stats.f_demux_bitrate);
        (*env)->SetIntField(env, objEntry, _stats_entry.demuxCorrupted, stats.i_demux_corrupted);
        (*env)->SetIntField(env, objEntry, _stats_entry.demuxDiscontinuity, stats.i_demux_discontinuity);
        (*env)->SetIntField(env, objEntry, _stats_entry.decodedVideo, stats.i_decoded_video);
        (*env)->SetIntField(env, objEntry, _stats_entry.decodedAudio, stats.i_decoded_audio);
        (*env)->SetIntField(env, objEntry, _stats_entry.displayedPictures, stats.i_displayed_pictures);
        (*env)->SetIntField(env, objEntry, _stats_entry.lostPictures, stats.i_lost_pictures);
        (*env)->SetIntField(env, objEntry, _stats_entry.playedAudioBuffers, stats.i_played_abuffers);
        (*env)->SetIntField(env, objEntry, _stats_entry.lostAudioBuffers, stats.i_lost_abuffers);
        //(*env)->DeleteLocalRef(env, objEntry);

        return objEntry;
    }

    return NULL;
}

JNIEXPORT jint JNICALL DECL_FUNC(nativeGetVideoDecoded)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        libvlc_media_t* media = libvlc_media_player_get_media(_player);
        libvlc_media_stats_t stats;
        libvlc_media_get_stats(media, &stats);
        libvlc_media_release(media);

        return stats.i_decoded_video;
    }
    return 0;
}

JNIEXPORT jint JNICALL DECL_FUNC(nativeGetAudioDecoded)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        libvlc_media_t* media = libvlc_media_player_get_media(_player);
        libvlc_media_stats_t stats;
        libvlc_media_get_stats(media, &stats);
        libvlc_media_release(media);

        return stats.i_decoded_audio;
    }
    return 0;
}


JNIEXPORT void JNICALL DECL_FUNC(nativeSetScramble)(JNIEnv* env, jobject clazz, jboolean b_scramble)
{
    if (_player)
        libvlc_media_player_set_scramble(_player, b_scramble);
}


JNIEXPORT jstring JNICALL DECL_FUNC(nativeGetMachineID)(JNIEnv* env, jobject clazz)
{
    char *serial;
    char temp[256];
    char *imei;

    jboolean copy;

    jclass build_class = (*env)->FindClass(env, "android/os/Build");
    jfieldID serial_id = (*env)->GetStaticFieldID(env, build_class, "SERIAL", "Ljava/lang/String;");
    jstring serial_obj = (jstring)(*env)->GetStaticObjectField(env, build_class, serial_id);

    serial = (*env)->GetStringUTFChars(env, serial_obj, &copy);
    libvlc_media_player_get_machine_id(0, serial, _machine_id);
    return (*env)->NewStringUTF(env, _machine_id);
}

JNIEXPORT int JNICALL DECL_FUNC(nativeCheckLicense)(JNIEnv* env, jobject clazz, jstring jlicense)
{
    char *license;
    jboolean copy;
    license = (*env)->GetStringUTFChars(env, jlicense, &copy);
    return libvlc_media_player_set_lincense(0, _PublicKey, license, _machine_id);
}

JNIEXPORT jobject JNICALL DECL_FUNC(nativeGetStreamTime)(JNIEnv* env, jobject clazz)
{
    if (_player)
    {
        jobject objEntry;

        int64_t i_time = libvlc_media_player_get_epg_time(_player);
        if (i_time > 0)
        {
            struct tm *t = localtime(&i_time);
            objEntry = (*env)->AllocObject(env, _stream_time_entry.clazz);
            (*env)->SetIntField(env, objEntry, _stream_time_entry.year, t->tm_year + 1900);
            (*env)->SetIntField(env, objEntry, _stream_time_entry.month, t->tm_mon + 1);
            (*env)->SetIntField(env, objEntry, _stream_time_entry.day, t->tm_mday);
            (*env)->SetIntField(env, objEntry, _stream_time_entry.hour, t->tm_hour);
            (*env)->SetIntField(env, objEntry, _stream_time_entry.minute, t->tm_min);
            (*env)->SetIntField(env, objEntry, _stream_time_entry.second, t->tm_sec);

            return objEntry;
        }
    }

    return NULL;
}

JNIEXPORT int JNICALL DECL_FUNC(nativeProgramParseStart)(JNIEnv* env, jobject clazz, jstring url)
{
    LOGI("nativeProgramParseStart");
    if (_vlc && !_media)
    {
        char* szUrl = (*env)->GetStringUTFChars(env, url, NULL);
        _media = libvlc_media_new_location(_vlc, szUrl);
        if (_media)
            return libvlc_media_program_parser_start(_media);
    }

    return -1;
}

JNIEXPORT int JNICALL DECL_FUNC(nativeProgramParseStop)(JNIEnv* env, jobject clazz)
{
    LOGI("nativeProgramParseStop");
    if (_media)
    {
        libvlc_media_program_parser_stop(_media);
        libvlc_media_release(_media);
        _media = NULL;
    }
    return 0;
}

JNIEXPORT jboolean JNICALL DECL_FUNC(nativeProgramParseIsCompleted)(JNIEnv* env, jobject clazz)
{
    if (_media)
        return libvlc_media_program_parser_is_completed(_media);
    return 0;
}

JNIEXPORT jobjectArray JNICALL DECL_FUNC(nativeProgramParseGet)(JNIEnv* env, jobject clazz)
{
    if (_media)
    {
        int i_programs;
        libvlc_program_t** pp_programs = NULL;
        i_programs = libvlc_media_program_parser_get(_media, &pp_programs);
        if (i_programs > 0)
        {
            int i;
            jobjectArray programArray;
            jobject objEntry;
            jstring jprogramName;
            
            programArray = (*env)->NewObjectArray(env, i_programs, _program_entry.clazz, NULL);
            for (i = 0; i < i_programs; i++)
            {
                objEntry = (*env)->AllocObject(env, _program_entry.clazz);
                //if (!pp_programs[i]->psz_name)
                //    pp_programs[i]->psz_name = strdup("NoName");
                jprogramName = (*env)->NewStringUTF(env, pp_programs[i]->psz_name);
                (*env)->SetIntField(env, objEntry, _program_entry.id, pp_programs[i]->i_id);
                (*env)->SetObjectField(env, objEntry, _program_entry.name, jprogramName);
                (*env)->SetObjectArrayElement(env, programArray, i, objEntry);
                (*env)->DeleteLocalRef(env, jprogramName);
                (*env)->DeleteLocalRef(env, objEntry);
            }
            libvlc_programs_release(pp_programs, i_programs);

            return programArray;
        }
    }

    return NULL;
}

#endif
