/*****************************************************************************
 * avcodec.h: decoder and encoder using libavcodec
 *****************************************************************************/

#ifndef AVCOMMON_COMPAT_H
#define AVCOMMON_COMPAT_H 1

#define AVPROVIDER(lib) ("ffmpeg")

#include <libavcodec/avcodec.h>

/* LIBAVCODEC_VERSION_CHECK checks for the right version of libav and FFmpeg
 * a is the major version
 * b and c the minor and micro versions of libav
 * d and e the minor and micro versions of FFmpeg */
#define LIBAVCODEC_VERSION_CHECK( a, b, c, d, e ) \
    ( (LIBAVCODEC_VERSION_MICRO <  100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT( a, b, c ) ) || \
      (LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT( a, d, e ) ) )

#ifndef AV_CODEC_FLAG_OUTPUT_CORRUPT
# define AV_CODEC_FLAG_OUTPUT_CORRUPT CODEC_FLAG_OUTPUT_CORRUPT
#endif
#ifndef AV_CODEC_FLAG_GRAY
# define AV_CODEC_FLAG_GRAY CODEC_FLAG_GRAY
#endif
#ifndef AV_CODEC_FLAG_DR1
# define AV_CODEC_FLAG_DR1 CODEC_FLAG_DR1
#endif
#ifndef AV_CODEC_FLAG_DELAY
# define AV_CODEC_FLAG_DELAY CODEC_FLAG_DELAY
#endif
#ifndef AV_CODEC_FLAG2_FAST
# define AV_CODEC_FLAG2_FAST CODEC_FLAG2_FAST
#endif
#ifndef FF_INPUT_BUFFER_PADDING_SIZE
# define FF_INPUT_BUFFER_PADDING_SIZE AV_INPUT_BUFFER_PADDING_SIZE
#endif
#ifndef AV_CODEC_FLAG_INTERLACED_DCT
# define AV_CODEC_FLAG_INTERLACED_DCT CODEC_FLAG_INTERLACED_DCT
#endif
#ifndef AV_CODEC_FLAG_INTERLACED_ME
# define AV_CODEC_FLAG_INTERLACED_ME CODEC_FLAG_INTERLACED_ME
#endif
#ifndef AV_CODEC_FLAG_GLOBAL_HEADER
# define AV_CODEC_FLAG_GLOBAL_HEADER CODEC_FLAG_GLOBAL_HEADER
#endif
#ifndef AV_CODEC_FLAG_LOW_DELAY
# define AV_CODEC_FLAG_LOW_DELAY CODEC_FLAG_LOW_DELAY
#endif
#ifndef AV_CODEC_CAP_SMALL_LAST_FRAME
# define AV_CODEC_CAP_SMALL_LAST_FRAME CODEC_CAP_SMALL_LAST_FRAME
#endif
#ifndef AV_INPUT_BUFFER_MIN_SIZE
# define AV_INPUT_BUFFER_MIN_SIZE FF_MIN_BUFFER_SIZE
#endif
#ifndef  FF_MAX_B_FRAMES
# define  FF_MAX_B_FRAMES 16 // FIXME: remove this
#endif

# include <libavutil/avutil.h>

/* LIBAVUTIL_VERSION_CHECK checks for the right version of libav and FFmpeg
 * a is the major version
 * b and c the minor and micro versions of libav
 * d and e the minor and micro versions of FFmpeg */
#define LIBAVUTIL_VERSION_CHECK( a, b, c, d, e ) \
    ( (LIBAVUTIL_VERSION_MICRO <  100 && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT( a, b, c ) ) || \
      (LIBAVUTIL_VERSION_MICRO >= 100 && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT( a, d, e ) ) )

#if !LIBAVUTIL_VERSION_CHECK( 52, 11, 0, 32, 100 )
#   define AV_PIX_FMT_FLAG_HWACCEL  PIX_FMT_HWACCEL
#endif


#if LIBAVUTIL_VERSION_MAJOR >= 55
# define FF_API_AUDIOCONVERT 1
#endif

/* libavutil/pixfmt.h */
#ifndef PixelFormat
# define PixelFormat AVPixelFormat
#endif

# include <libavformat/avformat.h>

#define LIBAVFORMAT_VERSION_CHECK( a, b, c, d, e ) \
    ( (LIBAVFORMAT_VERSION_MICRO <  100 && LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT( a, b, c ) ) || \
      (LIBAVFORMAT_VERSION_MICRO >= 100 && LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT( a, d, e ) ) )

#endif
