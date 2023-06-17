/*****************************************************************************
 * libvlc-module.c: Options for the core (libvlc itself) module
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_plugin.h>
#include <vlc_cpu.h>
#include <vlc_playlist.h>
#include <vlc_internal.h>

#include <limits.h>
#include "common/core/modules/modules.h"

#define MODULE_NAME     core
#define MODULE_STRING   "core"

//#define Nothing here, this is just to prevent update-po from being stupid
#include "vlc_actions.h"
#include "vlc_meta.h"
#include <vlc_aout.h>

static const char *const ppsz_snap_formats[] = { "png", "jpg", "tiff" };

/*****************************************************************************
 * Configuration options for the core module. Each module will also separatly
 * define its own configuration options.
 * Look into configuration.h if you need to know more about the following
 * macros.
 *****************************************************************************/

/*****************************************************************************
 * Intf
 ****************************************************************************/

// DEPRECATED
#define INTF_CAT_LONGTEXT N_( \
    "These options allow you to configure the interfaces used by VLC. " \
    "You can select the main interface, additional " \
    "interface modules, and define various related options." )

#define INTF_TEXT N_("Interface module")
#define INTF_LONGTEXT N_( \
    "This is the main interface used by VLC. " \
    "The default behavior is to automatically select the best module " \
    "available.")

#define EXTRAINTF_TEXT N_("Extra interface modules")
#define EXTRAINTF_LONGTEXT N_( \
    "You can select \"additional interfaces\" for VLC. " \
    "They will be launched in the background in addition to the default " \
    "interface. Use a colon separated list of interface modules. (common " \
    "values are \"rc\" (remote control), \"http\", \"gestures\" ...)")

#define CONTROL_TEXT N_("Control interfaces")
#define CONTROL_LONGTEXT N_( \
    "You can select control interfaces for VLC.")

#define VERBOSE_TEXT N_("Verbosity (0,1,2)")
#define VERBOSE_LONGTEXT N_( \
    "This is the verbosity level (0=only errors and " \
    "standard messages, 1=warnings, 2=debug).")

#define OPEN_TEXT N_("Default stream")
#define OPEN_LONGTEXT N_( \
    "This stream will always be opened at VLC startup." )

#define COLOR_TEXT N_("Color messages")
#define COLOR_LONGTEXT N_( \
    "This enables colorization of the messages sent to the console. " \
    "Your terminal needs Linux color support for this to work.")

#define ADVANCED_TEXT N_("Show advanced options")
#define ADVANCED_LONGTEXT N_( \
    "When this is enabled, the preferences and/or interfaces will " \
    "show all available options, including those that most users should " \
    "never touch.")

#define INTERACTION_TEXT N_("Interface interaction")
#define INTERACTION_LONGTEXT N_( \
    "When this is enabled, the interface will show a dialog box each time " \
    "some user input is required." )


/*****************************************************************************
 * Audio
 ****************************************************************************/

// DEPRECATED
#define AOUT_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the audio " \
    "subsystem, and to add audio filters which can be used for " \
    "post processing or visual effects (spectrum analyzer, etc.). " \
    "Enable these filters here, and configure them in the \"audio filters\" " \
    "modules section.")

#define AOUT_TEXT N_("Audio output module")
#define AOUT_LONGTEXT N_( \
    "This is the audio output method used by VLC. " \
    "The default behavior is to automatically select the best method " \
    "available.")

#define ROLE_TEXT N_("Media role")
#define ROLE_LONGTEXT N_("Media (player) role for operating system policy.")

#define AUDIO_TEXT N_("Enable audio")
#define AUDIO_LONGTEXT N_( \
    "You can completely disable the audio output. The audio " \
    "decoding stage will not take place, thus saving some processing power.")
static const char *ppsz_roles[] = {
     "video", "music", "communication", "game",
     "notification", "animation", "production",
     "accessibility", "test",
};
static const char *ppsz_roles_text[] = {
    N_("Video"), N_("Music"), N_("Communication"), N_("Game"),
    N_("Notification"),  N_("Animation"), N_("Production"),
    N_("Accessibility"), N_("Test"),
};

#define GAIN_TEXT N_("Audio gain")
#define GAIN_LONGTEXT N_( \
    "This linear gain will be applied to outputted audio.")

#define VOLUME_STEP_TEXT N_("Audio output volume step")
#define VOLUME_STEP_LONGTEXT N_( \
    "The step size of the volume is adjustable using this option.")
#define AOUT_VOLUME_STEP 12.8

#define VOLUME_SAVE_TEXT N_( "Remember the audio volume" )
#define VOLUME_SAVE_LONGTEXT N_( \
    "The volume can be recorded and automatically restored next time " \
    "VLC is used." )

#define DESYNC_TEXT N_("Audio desynchronization compensation")
#define DESYNC_LONGTEXT N_( \
    "This delays the audio output. The delay must be given in milliseconds. " \
    "This can be handy if you notice a lag between the video and the audio.")

#define AUDIO_RESAMPLER_TEXT N_("Audio resampler")
#define AUDIO_RESAMPLER_LONGTEXT N_( \
    "This selects which plugin to use for audio resampling." )

#define MULTICHA_LONGTEXT N_( \
    "Sets the audio output channels mode that will be used by default " \
    "if your hardware and the audio stream are compatible.")

#define SPDIF_TEXT N_("Force S/PDIF support")
#define SPDIF_LONGTEXT N_( \
    "This option should be used when the audio output can't negotiate S/PDIF support.")

#define FORCE_DOLBY_TEXT N_("Force detection of Dolby Surround")
#define FORCE_DOLBY_LONGTEXT N_( \
    "Use this when you know your stream is (or is not) encoded with Dolby "\
    "Surround but fails to be detected as such. Even if the stream is "\
    "not actually encoded with Dolby Surround, turning on this option might "\
    "enhance your experience, especially when combined with the Headphone "\
    "Channel Mixer." )
static const int pi_force_dolby_values[] = { 0, 1, 2 };
static const char *const ppsz_force_dolby_descriptions[] = {
    N_("Auto"), N_("On"), N_("Off") };

#define STEREO_MODE_TEXT N_("Stereo audio output mode")
static const int pi_stereo_mode_values[] = { AOUT_VAR_CHAN_UNSET,
    AOUT_VAR_CHAN_STEREO, AOUT_VAR_CHAN_RSTEREO,
    AOUT_VAR_CHAN_LEFT, AOUT_VAR_CHAN_RIGHT, AOUT_VAR_CHAN_DOLBYS,
    AOUT_VAR_CHAN_HEADPHONES,
};
static const char *const ppsz_stereo_mode_texts[] = { N_("Unset"),
    N_("Stereo"), N_("Reverse stereo"),
    N_("Left"), N_("Right"), N_("Dolby Surround"),
    N_("Headphones"),
};

#define AUDIO_FILTER_TEXT N_("Audio filters")
#define AUDIO_FILTER_LONGTEXT N_( \
    "This adds audio post processing filters, to modify " \
    "the sound rendering." )

#define AUDIO_VISUAL_TEXT N_("Audio visualizations")
#define AUDIO_VISUAL_LONGTEXT N_( \
    "This adds visualization modules (spectrum analyzer, etc.).")


#define AUDIO_REPLAY_GAIN_MODE_TEXT N_( \
    "Replay gain mode" )
#define AUDIO_REPLAY_GAIN_MODE_LONGTEXT N_( \
    "Select the replay gain mode" )
#define AUDIO_REPLAY_GAIN_PREAMP_TEXT N_( \
    "Replay preamp" )
#define AUDIO_REPLAY_GAIN_PREAMP_LONGTEXT N_( \
    "This allows you to change the default target level (89 dB) " \
    "for stream with replay gain information" )
#define AUDIO_REPLAY_GAIN_DEFAULT_TEXT N_( \
    "Default replay gain" )
#define AUDIO_REPLAY_GAIN_DEFAULT_LONGTEXT N_( \
    "This is the gain used for stream without replay gain information" )
#define AUDIO_REPLAY_GAIN_PEAK_PROTECTION_TEXT N_( \
    "Peak protection" )
#define AUDIO_REPLAY_GAIN_PEAK_PROTECTION_LONGTEXT N_( \
    "Protect against sound clipping" )

#define AUDIO_TIME_STRETCH_TEXT N_( \
    "Enable time stretching audio" )
#define AUDIO_TIME_STRETCH_LONGTEXT N_( \
    "This allows playing audio at lower or higher speed without " \
    "affecting the audio pitch" )


static const char *const ppsz_replay_gain_mode[] = {
    "none", "track", "album" };
static const char *const ppsz_replay_gain_mode_text[] = {
    N_("None"), N_("Track"), N_("Album") };

/*****************************************************************************
 * Video
 ****************************************************************************/

// DEPRECATED
#define VOUT_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the video output " \
    "subsystem. You can for example enable video filters (deinterlacing, " \
    "image adjusting, etc.). Enable these filters here and configure " \
    "them in the \"video filters\" modules section. You can also set many " \
    "miscellaneous video options." )

#define VOUT_TEXT N_("Video output module")
#define VOUT_LONGTEXT N_( \
    "This is the the video output method used by VLC. " \
    "The default behavior is to automatically select the best method available.")

#define VIDEO_TEXT N_("Enable video")
#define VIDEO_LONGTEXT N_( \
    "You can completely disable the video output. The video " \
    "decoding stage will not take place, thus saving some processing power.")

#define WIDTH_TEXT N_("Video width")
#define WIDTH_LONGTEXT N_( \
    "You can enforce the video width. By default (-1) VLC will " \
    "adapt to the video characteristics.")

#define HEIGHT_TEXT N_("Video height")
#define HEIGHT_LONGTEXT N_( \
    "You can enforce the video height. By default (-1) VLC will " \
    "adapt to the video characteristics.")

#define VIDEOX_TEXT N_("Video X coordinate")
#define VIDEOX_LONGTEXT N_( \
    "You can enforce the position of the top left corner of the video window "\
    "(X coordinate).")

#define VIDEOY_TEXT N_("Video Y coordinate")
#define VIDEOY_LONGTEXT N_( \
    "You can enforce the position of the top left corner of the video window "\
    "(Y coordinate).")

#define VIDEO_TITLE_TEXT N_("Video title")
#define VIDEO_TITLE_LONGTEXT N_( \
    "Custom title for the video window (in case the video is not embedded in "\
    "the interface).")

#define ALIGN_TEXT N_("Video alignment")
#define ALIGN_LONGTEXT N_( \
    "Enforce the alignment of the video in its window. By default (0) it " \
    "will be centered (0=center, 1=left, 2=right, 4=top, 8=bottom, you can " \
    "also use combinations of these values, like 6=4+2 meaning top-right).")
static const int pi_align_values[] = { 0, 1, 2, 4, 8, 5, 6, 9, 10 };
static const char *const ppsz_align_descriptions[] =
{ N_("Center"), N_("Left"), N_("Right"), N_("Top"), N_("Bottom"),
  N_("Top-Left"), N_("Top-Right"), N_("Bottom-Left"), N_("Bottom-Right") };

#define ZOOM_TEXT N_("Zoom video")
#define ZOOM_LONGTEXT N_( \
    "You can zoom the video by the specified factor.")

#define GRAYSCALE_TEXT N_("Grayscale video output")
#define GRAYSCALE_LONGTEXT N_( \
    "Output video in grayscale. As the color information aren't decoded, " \
    "this can save some processing power." )

#define EMBEDDED_TEXT N_("Embedded video")
#define EMBEDDED_LONGTEXT N_( \
    "Embed the video output in the main interface." )

#define FULLSCREEN_TEXT N_("Fullscreen video output")
#define FULLSCREEN_LONGTEXT N_( \
    "Start video in fullscreen mode" )

#define VIDEO_ON_TOP_TEXT N_("Always on top")
#define VIDEO_ON_TOP_LONGTEXT N_( \
    "Always place the video window on top of other windows." )

#define WALLPAPER_TEXT N_("Enable wallpaper mode")
#define WALLPAPER_LONGTEXT N_( \
    "The wallpaper mode allows you to display the video as the desktop " \
    "background." )

#define VIDEO_TITLE_SHOW_TEXT N_("Show media title on video")
#define VIDEO_TITLE_SHOW_LONGTEXT N_( \
    "Display the title of the video on top of the movie.")

#define VIDEO_TITLE_TIMEOUT_TEXT N_("Show video title for x milliseconds")
#define VIDEO_TITLE_TIMEOUT_LONGTEXT N_( \
    "Show the video title for n milliseconds, default is 5000 ms (5 sec.)")

#define VIDEO_TITLE_POSITION_TEXT N_("Position of video title")
#define VIDEO_TITLE_POSITION_LONGTEXT N_( \
    "Place on video where to display the title (default bottom center).")

#define MOUSE_HIDE_TIMEOUT_TEXT N_("Hide cursor and fullscreen " \
                                   "controller after x milliseconds")
#define MOUSE_HIDE_TIMEOUT_LONGTEXT N_( \
    "Hide mouse cursor and fullscreen controller after " \
    "n milliseconds.")

#define DEINTERLACE_TEXT N_("Deinterlace")
#define DEINTERLACE_LONGTEXT N_(\
    "Deinterlace")
static const int pi_deinterlace[] = {
    0, -1, 1
};
static const char * const  ppsz_deinterlace_text[] = {
    "Off", "Automatic", "On"
};

#define DEINTERLACE_MODE_TEXT N_("Deinterlace mode")
#define DEINTERLACE_MODE_LONGTEXT N_( \
    "Deinterlace method to use for video processing.")
static const char * const ppsz_deinterlace_mode[] = {
    "auto", "discard", "blend", "mean", "bob",
    "linear", "x", "yadif", "yadif2x", "phosphor",
    "ivtc"
};
static const char * const ppsz_deinterlace_mode_text[] = {
    N_("Auto"), N_("Discard"), N_("Blend"), N_("Mean"), N_("Bob"),
    N_("Linear"), "X", "Yadif", "Yadif (2x)", N_("Phosphor"),
    N_("Film NTSC (IVTC)")
};

static const int pi_pos_values[] = { 0, 1, 2, 4, 8, 5, 6, 9, 10 };
static const char *const ppsz_pos_descriptions[] =
{ N_("Center"), N_("Left"), N_("Right"), N_("Top"), N_("Bottom"),
  N_("Top-Left"), N_("Top-Right"), N_("Bottom-Left"), N_("Bottom-Right") };

#define SS_TEXT N_("Disable screensaver")
#define SS_LONGTEXT N_("Disable the screensaver during video playback." )

#define VIDEO_DECO_TEXT N_("Window decorations")
#define VIDEO_DECO_LONGTEXT N_( \
    "VLC can avoid creating window caption, frames, etc... around the video" \
    ", giving a \"minimal\" window.")

#define VIDEO_SPLITTER_TEXT N_("Video splitter module")
#define VIDEO_SPLITTER_LONGTEXT N_( \
    "This adds video splitters like clone or wall" )

#define VIDEO_FILTER_TEXT N_("Video filter module")
#define VIDEO_FILTER_LONGTEXT N_( \
    "This adds post-processing filters to enhance the " \
    "picture quality, for instance deinterlacing, or distort " \
    "the video.")

#define SNAP_PATH_TEXT N_("Video snapshot directory (or filename)")
#define SNAP_PATH_LONGTEXT N_( \
    "Directory where the video snapshots will be stored.")

#define SNAP_PREFIX_TEXT N_("Video snapshot file prefix")
#define SNAP_PREFIX_LONGTEXT N_( \
    "Video snapshot file prefix" )

#define SNAP_FORMAT_TEXT N_("Video snapshot format")
#define SNAP_FORMAT_LONGTEXT N_( \
    "Image format which will be used to store the video snapshots" )

#define SNAP_PREVIEW_TEXT N_("Display video snapshot preview")
#define SNAP_PREVIEW_LONGTEXT N_( \
    "Display the snapshot preview in the screen's top-left corner.")

#define SNAP_SEQUENTIAL_TEXT N_("Use sequential numbers instead of timestamps")
#define SNAP_SEQUENTIAL_LONGTEXT N_( \
    "Use sequential numbers instead of timestamps for snapshot numbering")

#define SNAP_WIDTH_TEXT N_("Video snapshot width")
#define SNAP_WIDTH_LONGTEXT N_( \
    "You can enforce the width of the video snapshot. By default " \
    "it will keep the original width (-1). Using 0 will scale the width " \
    "to keep the aspect ratio." )

#define SNAP_HEIGHT_TEXT N_("Video snapshot height")
#define SNAP_HEIGHT_LONGTEXT N_( \
    "You can enforce the height of the video snapshot. By default " \
    "it will keep the original height (-1). Using 0 will scale the height " \
    "to keep the aspect ratio." )

#define CROP_TEXT N_("Video cropping")
#define CROP_LONGTEXT N_( \
    "This forces the cropping of the source video. " \
    "Accepted formats are x:y (4:3, 16:9, etc.) expressing the global image " \
    "aspect.")

#define ASPECT_RATIO_TEXT N_("Source aspect ratio")
#define ASPECT_RATIO_LONGTEXT N_( \
    "This forces the source aspect ratio. For instance, some DVDs claim " \
    "to be 16:9 while they are actually 4:3. This can also be used as a " \
    "hint for VLC when a movie does not have aspect ratio information. " \
    "Accepted formats are x:y (4:3, 16:9, etc.) expressing the global image " \
    "aspect, or a float value (1.25, 1.3333, etc.) expressing pixel " \
    "squareness.")

#define AUTOSCALE_TEXT N_("Video Auto Scaling")
#define AUTOSCALE_LONGTEXT N_( \
    "Let the video scale to fit a given window or fullscreen.")

#define SCALEFACTOR_TEXT N_("Video scaling factor")
#define SCALEFACTOR_LONGTEXT N_( \
    "Scaling factor used when Auto Scaling is disabled.\n" \
    "Default value is 1.0 (original video size).")

#define HDTV_FIX_TEXT N_("Fix HDTV height")
#define HDTV_FIX_LONGTEXT N_( \
    "This allows proper handling of HDTV-1080 video format " \
    "even if broken encoder incorrectly sets height to 1088 lines. " \
    "You should only disable this option if your video has a " \
    "non-standard format requiring all 1088 lines.")

#define MASPECT_RATIO_TEXT N_("Monitor pixel aspect ratio")
#define MASPECT_RATIO_LONGTEXT N_( \
    "This forces the monitor aspect ratio. Most monitors have square " \
    "pixels (1:1). If you have a 16:9 screen, you might need to change this " \
    "to 4:3 in order to keep proportions.")

#define SKIP_FRAMES_TEXT N_("Skip frames")
#define SKIP_FRAMES_LONGTEXT N_( \
    "Enables framedropping on MPEG2 stream. Framedropping " \
    "occurs when your computer is not powerful enough" )

#define DROP_LATE_FRAMES_TEXT N_("Drop late frames")
#define DROP_LATE_FRAMES_LONGTEXT N_( \
    "This drops frames that are late (arrive to the video output after " \
    "their intended display date)." )

#define QUIET_SYNCHRO_TEXT N_("Quiet synchro")
#define QUIET_SYNCHRO_LONGTEXT N_( \
    "This avoids flooding the message log with debug output from the " \
    "video output synchronization mechanism.")

#define KEYBOARD_EVENTS_TEXT N_("Key press events")
#define KEYBOARD_EVENTS_LONGTEXT N_( \
    "This enables VLC hotkeys from the (non-embedded) video window." )

#define MOUSE_EVENTS_TEXT N_("Mouse events")
#define MOUSE_EVENTS_LONGTEXT N_( \
    "This enables handling of mouse clicks on the video." )

/*****************************************************************************
 * Input
 ****************************************************************************/

// Deprecated
#define INPUT_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the input " \
    "subsystem, such as the DVD or VCD device, the network interface " \
    "settings or the subtitle channel.")

#define CACHING_TEXT N_("File caching (ms)")
#define CACHING_LONGTEXT N_( \
    "Caching value for local files, in milliseconds." )

#define CAPTURE_CACHING_TEXT N_("Live capture caching (ms)")
#define CAPTURE_CACHING_LONGTEXT N_( \
    "Caching value for cameras and microphones, in milliseconds." )

#define DISC_CACHING_TEXT N_("Disc caching (ms)")
#define DISC_CACHING_LONGTEXT N_( \
    "Caching value for optical media, in milliseconds." )

#define NETWORK_CACHING_TEXT N_("Network caching (ms)")
#define NETWORK_CACHING_LONGTEXT N_( \
    "Caching value for network resources, in milliseconds." )

#define CR_AVERAGE_TEXT N_("Clock reference average counter")
#define CR_AVERAGE_LONGTEXT N_( \
    "When using the PVR input (or a very irregular source), you should " \
    "set this to 10000.")

#define CLOCK_SYNCHRO_TEXT N_("Clock synchronisation")
#define CLOCK_SYNCHRO_LONGTEXT N_( \
    "It is possible to disable the input clock synchronisation for " \
    "real-time sources. Use this if you experience jerky playback of " \
    "network streams.")

#define CLOCK_JITTER_TEXT N_("Clock jitter")
#define CLOCK_JITTER_LONGTEXT N_( \
    "This defines the maximum input delay jitter that the synchronization " \
    "algorithms should try to compensate (in milliseconds)." )

#define NETSYNC_TEXT N_("Network synchronisation" )
#define NETSYNC_LONGTEXT N_( "This allows you to remotely " \
        "synchronise clocks for server and client. The detailed settings " \
        "are available in Advanced / Network Sync." )

static const int pi_clock_values[] = { -1, 0, 1 };
static const char *const ppsz_clock_descriptions[] =
{ N_("Default"), N_("Disable"), N_("Enable") };

#define MTU_TEXT N_("MTU of the network interface")
#define MTU_LONGTEXT N_( \
    "This is the maximum application-layer packet size that can be " \
    "transmitted over the network (in bytes).")
/* Should be less than 1500 - 8[ppp] - 40[ip6] - 8[udp] in any case. */
#define MTU_DEFAULT 1400

#define TTL_TEXT N_("Hop limit (TTL)")
#define TTL_LONGTEXT N_( \
    "This is the hop limit (also known as \"Time-To-Live\" or TTL) of " \
    "the multicast packets sent by the stream output (-1 = use operating " \
    "system built-in default).")

#define MIFACE_TEXT N_("Multicast output interface")
#define MIFACE_LONGTEXT N_( \
    "Default multicast interface. This overrides the routing table.")

#define DSCP_TEXT N_("DiffServ Code Point")
#define DSCP_LONGTEXT N_("Differentiated Services Code Point " \
    "for outgoing UDP streams (or IPv4 Type Of Service, " \
    "or IPv6 Traffic Class). This is used for network Quality of Service.")

#define INPUT_PROGRAM_TEXT N_("Program")
#define INPUT_PROGRAM_LONGTEXT N_( \
    "Choose the program to select by giving its Service ID. " \
    "Only use this option if you want to read a multi-program stream " \
    "(like DVB streams for example)." )

#define INPUT_PROGRAMS_TEXT N_("Programs")
#define INPUT_PROGRAMS_LONGTEXT N_( \
    "Choose the programs to select by giving a comma-separated list of " \
    "Service IDs (SIDs). " \
    "Only use this option if you want to read a multi-program stream " \
    "(like DVB streams for example)." )

/// \todo Document how to find it
#define INPUT_AUDIOTRACK_TEXT N_("Audio track")
#define INPUT_AUDIOTRACK_LONGTEXT N_( \
    "Stream number of the audio track to use " \
    "(from 0 to n).")

#define INPUT_SUBTRACK_TEXT N_("Subtitle track")
#define INPUT_SUBTRACK_LONGTEXT N_( \
    "Stream number of the subtitle track to use " \
    "(from 0 to n).")

#define INPUT_AUDIOTRACK_LANG_TEXT N_("Audio language")
#define INPUT_AUDIOTRACK_LANG_LONGTEXT N_( \
    "Language of the audio track you want to use " \
    "(comma separated, two or three letter country code, you may use 'none' to avoid a fallback to another language).")

#define INPUT_SUBTRACK_LANG_TEXT N_("Subtitle language")
#define INPUT_SUBTRACK_LANG_LONGTEXT N_( \
    "Language of the subtitle track you want to use " \
    "(comma separated, two or three letters country code, you may use 'any' as a fallback).")

#define INPUT_MENUTRACK_LANG_TEXT N_("Menu language")
#define INPUT_MENUTRACK_LANG_LONGTEXT N_( \
    "Language of the menus you want to use with DVD/BluRay " \
    "(comma separated, two or three letters country code, you may use 'any' as a fallback).")

/// \todo Document how to find it
#define INPUT_AUDIOTRACK_ID_TEXT N_("Audio track ID")
#define INPUT_AUDIOTRACK_ID_LONGTEXT N_( \
    "Stream ID of the audio track to use.")

#define INPUT_SUBTRACK_ID_TEXT N_("Subtitle track ID")
#define INPUT_SUBTRACK_ID_LONGTEXT N_( \
    "Stream ID of the subtitle track to use.")

#define INPUT_CAPTIONS_TEXT N_(N_("Preferred Closed Captions decoder"))
static const int pi_captions[] = { 608, 708 };
static const char *const ppsz_captions[] = { "EIA/CEA 608", "CEA 708" };

#define INPUT_PREFERREDRESOLUTION_TEXT N_("Preferred video resolution")
#define INPUT_PREFERREDRESOLUTION_LONGTEXT N_( \
    "When several video formats are available, select one whose " \
    "resolution is closest to (but not higher than) this setting, " \
    "in number of lines. Use this option if you don't have enough CPU " \
    "power or network bandwidth to play higher resolutions.")
static const int pi_prefres[] = { -1, 1080, 720, 576, 360, 240 };
static const char *const ppsz_prefres[] = {
    N_("Best available"), N_("Full HD (1080p)"), N_("HD (720p)"),
    N_("Standard Definition (576 or 480 lines)"),
    N_("Low Definition (360 lines)"),
    N_("Very Low Definition (240 lines)"),
};

#define INPUT_REPEAT_TEXT N_("Input repetitions")
#define INPUT_REPEAT_LONGTEXT N_( \
    "Number of time the same input will be repeated")

#define START_TIME_TEXT N_("Start time")
#define START_TIME_LONGTEXT N_( \
    "The stream will start at this position (in seconds)." )

#define STOP_TIME_TEXT N_("Stop time")
#define STOP_TIME_LONGTEXT N_( \
    "The stream will stop at this position (in seconds)." )

#define RUN_TIME_TEXT N_("Run time")
#define RUN_TIME_LONGTEXT N_( \
    "The stream will run this duration (in seconds)." )

#define INPUT_FAST_SEEK_TEXT N_("Fast seek")
#define INPUT_FAST_SEEK_LONGTEXT N_( \
    "Favor speed over precision while seeking" )

#define INPUT_RATE_TEXT N_("Playback speed")
#define INPUT_RATE_LONGTEXT N_( \
    "This defines the playback speed (nominal speed is 1.0)." )

#define INPUT_LIST_TEXT N_("Input list")
#define INPUT_LIST_LONGTEXT N_( \
    "You can give a comma-separated list " \
    "of inputs that will be concatenated together after the normal one.")

#define INPUT_SLAVE_TEXT N_("Input slave (experimental)")
#define INPUT_SLAVE_LONGTEXT N_( \
    "This allows you to play from several inputs at " \
    "the same time. This feature is experimental, not all formats " \
    "are supported. Use a '#' separated list of inputs.")

#define BOOKMARKS_TEXT N_("Bookmarks list for a stream")
#define BOOKMARKS_LONGTEXT N_( \
    "You can manually give a list of bookmarks for a stream in " \
    "the form \"{name=bookmark-name,time=optional-time-offset," \
    "bytes=optional-byte-offset},{...}\"")

#define INPUT_RECORD_PATH_TEXT N_("Record directory")
#define INPUT_RECORD_PATH_LONGTEXT N_( \
    "Directory where the records will be stored" )

#define INPUT_RECORD_NATIVE_TEXT N_("Prefer native stream recording")
#define INPUT_RECORD_NATIVE_LONGTEXT N_( \
    "When possible, the input stream will be recorded instead of using " \
    "the stream output module" )

#define INPUT_TIMESHIFT_PATH_TEXT N_("Timeshift directory")
#define INPUT_TIMESHIFT_PATH_LONGTEXT N_( \
    "Directory used to store the timeshift temporary files." )

#define INPUT_TIMESHIFT_GRANULARITY_TEXT N_("Timeshift granularity")
#define INPUT_TIMESHIFT_GRANULARITY_LONGTEXT N_( \
    "This is the maximum size in bytes of the temporary files " \
    "that will be used to store the timeshifted streams." )

#define INPUT_TITLE_FORMAT_TEXT N_( "Change title according to current media" )
#define INPUT_TITLE_FORMAT_LONGTEXT N_( "This option allows you to set the title according to what's being played<br>"  \
    "$a: Artist<br>$b: Album<br>$c: Copyright<br>$t: Title<br>$g: Genre<br>"  \
    "$n: Track num<br>$p: Now playing<br>$A: Date<br>$D: Duration<br>"  \
    "$Z: \"Now playing\" (Fall back on Title - Artist)" )

#define INPUT_LUA_TEXT N_( "Disable all lua plugins" )

// DEPRECATED
#define SUB_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the subpictures " \
    "subsystem. You can for example enable subpictures sources (logo, etc.). " \
    "Enable these filters here and configure them in the " \
    "\"subsources filters\" modules section. You can also set many " \
    "miscellaneous subpictures options." )

#define SUB_MARGIN_TEXT N_("Force subtitle position")
#define SUB_MARGIN_LONGTEXT N_( \
    "You can use this option to place the subtitles under the movie, " \
    "instead of over the movie. Try several positions.")

#define SUB_TEXT_SCALE_TEXT N_("Subtitles text scaling factor")
#define SUB_TEXT_SCALE_LONGTEXT N_("Changes the subtitles size where possible")

#define SPU_TEXT N_("Enable sub-pictures")
#define SPU_LONGTEXT N_( \
    "You can completely disable the sub-picture processing.")

#define OSD_TEXT N_("On Screen Display")
#define OSD_LONGTEXT N_( \
    "VLC can display messages on the video. This is called OSD (On Screen " \
    "Display).")

#define TEXTRENDERER_TEXT N_("Text rendering module")
#define TEXTRENDERER_LONGTEXT N_( \
    "VLC normally uses Freetype for rendering, but this allows you to use svg for instance.")

#define SUB_SOURCE_TEXT N_("Subpictures source module")
#define SUB_SOURCE_LONGTEXT N_( \
    "This adds so-called \"subpicture sources\". These filters overlay " \
    "some images or text over the video (like a logo, arbitrary text, ...)." )

#define SUB_FILTER_TEXT N_("Subpictures filter module")
#define SUB_FILTER_LONGTEXT N_( \
    "This adds so-called \"subpicture filters\". These filter subpictures " \
    "created by subtitle decoders or other subpictures sources." )

#define SUB_AUTO_TEXT N_("Autodetect subtitle files")
#define SUB_AUTO_LONGTEXT N_( \
    "Automatically detect a subtitle file, if no subtitle filename is " \
    "specified (based on the filename of the movie).")

#define SUB_FUZZY_TEXT N_("Subtitle autodetection fuzziness")
#define SUB_FUZZY_LONGTEXT N_( \
    "This determines how fuzzy subtitle and movie filename matching " \
    "will be. Options are:\n" \
    "0 = no subtitles autodetected\n" \
    "1 = any subtitle file\n" \
    "2 = any subtitle file containing the movie name\n" \
    "3 = subtitle file matching the movie name with additional chars\n" \
    "4 = subtitle file matching the movie name exactly")

#define SUB_PATH_TEXT N_("Subtitle autodetection paths")
#define SUB_PATH_LONGTEXT N_( \
    "Look for a subtitle file in those paths too, if your subtitle " \
    "file was not found in the current directory.")

#define SUB_FILE_TEXT N_("Use subtitle file")
#define SUB_FILE_LONGTEXT N_( \
    "Load this subtitle file. To be used when autodetect cannot detect " \
    "your subtitle file.")

/* DVD and VCD devices */
#define DVD_DEV_TEXT N_("DVD device")
#define VCD_DEV_TEXT N_("VCD device")
#define CDAUDIO_DEV_TEXT N_("Audio CD device")

#if defined( _WIN32 ) || defined( __OS2__ )
# define DVD_DEV_LONGTEXT N_( \
    "This is the default DVD drive (or file) to use. Don't forget the colon " \
    "after the drive letter (e.g. D:)")
# define VCD_DEV_LONGTEXT N_( \
    "This is the default VCD drive (or file) to use. Don't forget the colon " \
    "after the drive letter (e.g. D:)")
# define CDAUDIO_DEV_LONGTEXT N_( \
    "This is the default Audio CD drive (or file) to use. Don't forget the " \
    "colon after the drive letter (e.g. D:)")
# define DVD_DEVICE     NULL
# define VCD_DEVICE     "D:"

#else
# define DVD_DEV_LONGTEXT N_( \
    "This is the default DVD device to use.")
# define VCD_DEV_LONGTEXT N_( \
    "This is the default VCD device to use." )
# define CDAUDIO_DEV_LONGTEXT N_( \
    "This is the default Audio CD device to use." )

# if defined(__OpenBSD__)
#  define DVD_DEVICE     "/dev/cd0c"
#  define VCD_DEVICE     "/dev/cd0c"
# elif defined(__linux__)
#  define DVD_DEVICE     "/dev/sr0"
#  define VCD_DEVICE     "/dev/sr0"
# else
#  define DVD_DEVICE     "/dev/dvd"
#  define VCD_DEVICE     "/dev/cdrom"
# endif
#endif

#define TIMEOUT_TEXT N_("TCP connection timeout")
#define TIMEOUT_LONGTEXT N_( \
    "Default TCP connection timeout (in milliseconds)." )

#define HTTP_HOST_TEXT N_( "HTTP server address" )
#define HOST_LONGTEXT N_( \
    "By default, the server will listen on any local IP address. " \
    "Specify an IP address (e.g. ::1 or 127.0.0.1) or a host name " \
    "(e.g. localhost) to  them to a specific network interface." )

#define RTSP_HOST_TEXT N_( "RTSP server address" )
#define RTSP_HOST_LONGTEXT N_( \
    "This defines the address the RTSP server will listen on, along " \
    "with the base path of the RTSP VOD media. Syntax is address/path. " \
    "By default, the server will listen on any local IP address. " \
    "Specify an IP address (e.g. ::1 or 127.0.0.1) or a host name " \
    "(e.g. localhost) to  them to a specific network interface." )

#define HTTP_PORT_TEXT N_( "HTTP server port" )
#define HTTP_PORT_LONGTEXT N_( \
    "The HTTP server will listen on this TCP port. " \
    "The standard HTTP port number is 80. " \
    "However allocation of port numbers below 1025 is usually restricted " \
    "by the operating system." )

#define HTTPS_PORT_TEXT N_( "HTTPS server port" )
#define HTTPS_PORT_LONGTEXT N_( \
    "The HTTPS server will listen on this TCP port. " \
    "The standard HTTPS port number is 443. " \
    "However allocation of port numbers below 1025 is usually restricted " \
    "by the operating system." )

#define RTSP_PORT_TEXT N_( "RTSP server port" )
#define RTSP_PORT_LONGTEXT N_( \
    "The RTSP server will listen on this TCP port. " \
    "The standard RTSP port number is 554. " \
    "However allocation of port numbers below 1025 is usually restricted " \
    "by the operating system." )

#define HTTP_CERT_TEXT N_("HTTP/TLS server certificate")
#define CERT_LONGTEXT N_( \
   "This X.509 certicate file (PEM format) is used for server-side TLS. " \
   "On OS X, the string is used as a label to search the certificate in the keychain." )

#define HTTP_KEY_TEXT N_("HTTP/TLS server private key")
#define KEY_LONGTEXT N_( \
   "This private key file (PEM format) is used for server-side TLS.")

#define PROXY_TEXT N_("HTTP proxy")
#define PROXY_LONGTEXT N_( \
    "HTTP proxy to be used It must be of the form " \
    "http://[user@]myproxy.mydomain:myport/ ; " \
    "if empty, the http_proxy environment variable will be tried." )

#define PROXY_PASS_TEXT N_("HTTP proxy password")
#define PROXY_PASS_LONGTEXT N_( \
    "If your HTTP proxy requires a password, set it here." )

#define SOCKS_SERVER_TEXT N_("SOCKS server")
#define SOCKS_SERVER_LONGTEXT N_( \
    "SOCKS proxy server to use. This must be of the form " \
    "address:port. It will be used for all TCP connections" )

#define SOCKS_USER_TEXT N_("SOCKS user name")
#define SOCKS_USER_LONGTEXT N_( \
    "User name to be used for connection to the SOCKS proxy." )

#define SOCKS_PASS_TEXT N_("SOCKS password")
#define SOCKS_PASS_LONGTEXT N_( \
    "Password to be used for connection to the SOCKS proxy." )

#define META_TITLE_TEXT N_("Title metadata")
#define META_TITLE_LONGTEXT N_( \
     "Allows you to specify a \"title\" metadata for an input.")

#define META_AUTHOR_TEXT N_("Author metadata")
#define META_AUTHOR_LONGTEXT N_( \
     "Allows you to specify an \"author\" metadata for an input.")

#define META_ARTIST_TEXT N_("Artist metadata")
#define META_ARTIST_LONGTEXT N_( \
     "Allows you to specify an \"artist\" metadata for an input.")

#define META_GENRE_TEXT N_("Genre metadata")
#define META_GENRE_LONGTEXT N_( \
     "Allows you to specify a \"genre\" metadata for an input.")

#define META_CPYR_TEXT N_("Copyright metadata")
#define META_CPYR_LONGTEXT N_( \
     "Allows you to specify a \"copyright\" metadata for an input.")

#define META_DESCR_TEXT N_("Description metadata")
#define META_DESCR_LONGTEXT N_( \
     "Allows you to specify a \"description\" metadata for an input.")

#define META_DATE_TEXT N_("Date metadata")
#define META_DATE_LONGTEXT N_( \
     "Allows you to specify a \"date\" metadata for an input.")

#define META_URL_TEXT N_("URL metadata")
#define META_URL_LONGTEXT N_( \
     "Allows you to specify a \"url\" metadata for an input.")

// DEPRECATED
#define CODEC_CAT_LONGTEXT N_( \
    "This option can be used to alter the way VLC selects " \
    "its codecs (decompression methods). Only advanced users should " \
    "alter this option as it can break playback of all your streams." )

#define CODEC_TEXT N_("Preferred decoders list")
#define CODEC_LONGTEXT N_( \
    "List of codecs that VLC will use in " \
    "priority. For instance, 'dummy,a52' will try the dummy and a52 codecs " \
    "before trying the other ones. Only advanced users should " \
    "alter this option as it can break playback of all your streams." )

#define ENCODER_TEXT N_("Preferred encoders list")
#define ENCODER_LONGTEXT N_( \
    "This allows you to select a list of encoders that VLC will use in " \
    "priority.")

/*****************************************************************************
 * Sout
 ****************************************************************************/

// DEPRECATED
#define SOUT_CAT_LONGTEXT N_( \
    "These options allow you to set default global options for the " \
    "stream output subsystem." )

#define SOUT_TEXT N_("Default stream output chain")
#define SOUT_LONGTEXT N_( \
    "You can enter here a default stream output chain. Refer to "\
    "the documentation to learn how to build such chains. " \
    "Warning: this chain will be enabled for all streams." )

#define SOUT_ALL_TEXT N_("Enable streaming of all ES")
#define SOUT_ALL_LONGTEXT N_( \
    "Stream all elementary streams (video, audio and subtitles)")

#define SOUT_DISPLAY_TEXT N_("Display while streaming")
#define SOUT_DISPLAY_LONGTEXT N_( \
    "Play locally the stream while streaming it.")

#define SOUT_VIDEO_TEXT N_("Enable video stream output")
#define SOUT_VIDEO_LONGTEXT N_( \
    "Choose whether the video stream should be redirected to " \
    "the stream output facility when this last one is enabled.")

#define SOUT_AUDIO_TEXT N_("Enable audio stream output")
#define SOUT_AUDIO_LONGTEXT N_( \
    "Choose whether the audio stream should be redirected to " \
    "the stream output facility when this last one is enabled.")

#define SOUT_SPU_TEXT N_("Enable SPU stream output")
#define SOUT_SPU_LONGTEXT N_( \
    "Choose whether the SPU streams should be redirected to " \
    "the stream output facility when this last one is enabled.")

#define SOUT_KEEP_TEXT N_("Keep stream output open" )
#define SOUT_KEEP_LONGTEXT N_( \
    "This allows you to keep an unique stream output instance across " \
    "multiple playlist item (automatically insert the gather stream output " \
    "if not specified)" )

#define SOUT_MUX_CACHING_TEXT N_("Stream output muxer caching (ms)")
#define SOUT_MUX_CACHING_LONGTEXT N_( \
    "This allow you to configure the initial caching amount for stream output " \
    "muxer. This value should be set in milliseconds." )

#define PACKETIZER_TEXT N_("Preferred packetizer list")
#define PACKETIZER_LONGTEXT N_( \
    "This allows you to select the order in which VLC will choose its " \
    "packetizers."  )

#define MUX_TEXT N_("Mux module")
#define MUX_LONGTEXT N_( \
    "This is a legacy entry to let you configure mux modules")

#define ACCESS_OUTPUT_TEXT N_("Access output module")
#define ACCESS_OUTPUT_LONGTEXT N_( \
    "This is a legacy entry to let you configure access output modules")

#define ANN_SAPCTRL_LONGTEXT N_( \
    "If this option is enabled, the flow on " \
    "the SAP multicast address will be controlled. This is needed if you " \
    "want to make announcements on the MBone." )

#define ANN_SAPINTV_TEXT N_("SAP announcement interval")
#define ANN_SAPINTV_LONGTEXT N_( \
    "When the SAP flow control is disabled, " \
    "this lets you set the fixed interval between SAP announcements." )

/*****************************************************************************
 * Advanced
 ****************************************************************************/

// DEPRECATED
#define MISC_CAT_LONGTEXT N_( \
    "These options allow you to select default modules. Leave these " \
    "alone unless you really know what you are doing." )

#define ACCESS_TEXT N_("Access module")
#define ACCESS_LONGTEXT N_( \
    "This allows you to force an access module. You can use it if " \
    "the correct access is not automatically detected. You should not "\
    "set this as a global option unless you really know what you are doing." )

#define STREAM_FILTER_TEXT N_("Stream filter module")
#define STREAM_FILTER_LONGTEXT N_( \
    "Stream filters are used to modify the stream that is being read." )

#define DEMUX_FILTER_TEXT N_("Demux filter module")
#define DEMUX_FILTER_LONGTEXT N_( \
    "Demux filters are used to modify/control the stream that is being read." )

#define DEMUX_TEXT N_("Demux module")
#define DEMUX_LONGTEXT N_( \
    "Demultiplexers are used to separate the \"elementary\" streams " \
    "(like audio and video streams). You can use it if " \
    "the correct demuxer is not automatically detected. You should not "\
    "set this as a global option unless you really know what you are doing." )

#define VOD_SERVER_TEXT N_("VoD server module")
#define VOD_SERVER_LONGTEXT N_( \
    "You can select which VoD server module you want to use. Set this " \
    "to 'vod_rtsp' to switch back to the old, legacy module." )

#define RT_OFFSET_TEXT N_("Adjust VLC priority")
#define RT_OFFSET_LONGTEXT N_( \
    "This option adds an offset (positive or negative) to VLC default " \
    "priorities. You can use it to tune VLC priority against other " \
    "programs, or against other VLC instances.")

#define USE_STREAM_IMMEDIATE_LONGTEXT N_( \
     "This option is useful if you want to lower the latency when " \
     "reading a stream")

#define VLM_CONF_TEXT N_("VLM configuration file")
#define VLM_CONF_LONGTEXT N_( \
    "Read a VLM configuration file as soon as VLM is started." )

#define PLUGINS_CACHE_TEXT N_("Use a plugins cache")
#define PLUGINS_CACHE_LONGTEXT N_( \
    "Use a plugins cache which will greatly improve the startup time of VLC.")

#define PLUGINS_SCAN_TEXT N_("Scan for new plugins")
#define PLUGINS_SCAN_LONGTEXT N_( \
    "Scan plugin directories for new plugins at startup. " \
    "This increases the startup time of VLC.")

#define KEYSTORE_TEXT N_("Preferred keystore list")
#define KEYSTORE_LONGTEXT N_( \
    "List of keystores that VLC will use in priority." )

#define STATS_TEXT N_("Locally collect statistics")
#define STATS_LONGTEXT N_( \
     "Collect miscellaneous local statistics about the playing media.")

#define DAEMON_TEXT N_("Run as daemon process")
#define DAEMON_LONGTEXT N_( \
     "Runs VLC as a background daemon process.")

#define PIDFILE_TEXT N_("Write process id to file")
#define PIDFILE_LONGTEXT N_( \
       "Writes process id into specified file.")

#define ONEINSTANCE_TEXT N_("Allow only one running instance")
#define ONEINSTANCE_LONGTEXT N_( \
    "Allowing only one running instance of VLC can sometimes be useful, " \
    "for example if you associated VLC with some media types and you " \
    "don't want a new instance of VLC to be opened each time you " \
    "open a file in your file manager. This option will allow you " \
    "to play the file with the already running instance or enqueue it.")

#define STARTEDFROMFILE_TEXT N_("VLC is started from file association")
#define STARTEDFROMFILE_LONGTEXT N_( \
    "Tell VLC that it is being launched due to a file association in the OS" )

#define ONEINSTANCEWHENSTARTEDFROMFILE_TEXT N_( \
    "Use only one instance when started from file manager")

#define PLAYLISTENQUEUE_TEXT N_( \
    "Enqueue items into playlist in one instance mode")
#define PLAYLISTENQUEUE_LONGTEXT N_( \
    "When using the one instance only option, enqueue items to playlist " \
    "and keep playing current item.")

#define DBUS_TEXT N_("Expose media player via D-Bus")
#define DBUS_LONGTEXT N_("Allow other applications to control VLC " \
    "using the D-Bus MPRIS protocol.")

/*****************************************************************************
 * Playlist
 ****************************************************************************/

// DEPRECATED
#define PLAYLIST_CAT_LONGTEXT N_( \
     "These options define the behavior of the playlist. Some " \
     "of them can be overridden in the playlist dialog box." )

#define PREPARSE_TEXT N_( "Automatically preparse items")
#define PREPARSE_LONGTEXT N_( \
    "Automatically preparse items added to the playlist " \
    "(to retrieve some metadata)." )

#define PREPARSE_TIMEOUT_TEXT N_( "Preparsing timeout" )
#define PREPARSE_TIMEOUT_LONGTEXT N_( \
    "Maximum time allowed to preparse an item, in milliseconds" )

#define METADATA_NETWORK_TEXT N_( "Allow metadata network access" )

static const char *const psz_recursive_list[] = {
    "none", "collapse", "expand" };
static const char *const psz_recursive_list_text[] = {
    N_("None"), N_("Collapse"), N_("Expand") };

#define RECURSIVE_TEXT N_("Subdirectory behavior")
#define RECURSIVE_LONGTEXT N_( \
        "Select whether subdirectories must be expanded.\n" \
        "none: subdirectories do not appear in the playlist.\n" \
        "collapse: subdirectories appear but are expanded on first play.\n" \
        "expand: all subdirectories are expanded.\n" )

#define IGNORE_TEXT N_("Ignored extensions")
#define IGNORE_LONGTEXT N_( \
        "Files with these extensions will not be added to playlist when " \
        "opening a directory.\n" \
        "This is useful if you add directories that contain playlist files " \
        "for instance. Use a comma-separated list of extensions." )

#define SHOW_HIDDENFILES_TEXT N_("Show hidden files")
#define SHOW_HIDDENFILES_LONGTEXT N_( \
        "Ignore files starting with '.'" )

#define SD_TEXT N_( "Services discovery modules")
#define SD_LONGTEXT N_( \
     "Specifies the services discovery modules to preload, separated by " \
     "colons. Typical value is \"sap\"." )

#define RANDOM_TEXT N_("Play files randomly forever")
#define RANDOM_LONGTEXT N_( \
    "VLC will randomly play files in the playlist until interrupted.")

#define LOOP_TEXT N_("Repeat all")
#define LOOP_LONGTEXT N_( \
    "VLC will keep playing the playlist indefinitely." )

#define REPEAT_TEXT N_("Repeat current item")
#define REPEAT_LONGTEXT N_( \
    "VLC will keep playing the current playlist item." )

#define PAS_TEXT N_("Play and stop")
#define PAS_LONGTEXT N_( \
    "Stop the playlist after each played playlist item." )

#define PAE_TEXT N_("Play and exit")
#define PAE_LONGTEXT N_( \
    "Exit if there are no more items in the playlist." )

#define PAP_TEXT N_("Play and pause")
#define PAP_LONGTEXT N_( \
    "Pause each item in the playlist on the last frame." )

#define SP_TEXT N_("Start paused")
#define SP_LONGTEXT N_( \
    "Pause each item in the playlist on the first frame." )

#define AUTOSTART_TEXT N_( "Auto start" )
#define AUTOSTART_LONGTEXT N_( "Automatically start playing the playlist " \
                "content once it's loaded." )

#define CORK_TEXT N_("Pause on audio communication")
#define CORK_LONGTEXT N_( \
    "If pending audio communication is detected, playback will be paused " \
    "automatically." )

#define PLTREE_TEXT N_("Display playlist tree")
#define PLTREE_LONGTEXT N_( \
    "The playlist can use a tree to categorize some items, like the " \
    "contents of a directory." )


/*****************************************************************************
 * Hotkeys
 ****************************************************************************/

// DEPRECATED
#define HOTKEY_CAT_LONGTEXT N_( "These settings are the global VLC key " \
    "bindings, known as \"hotkeys\"." )

#define MOUSE_Y_WHEEL_MODE_TEXT N_("Mouse wheel vertical axis control")
#define MOUSE_Y_WHEEL_MODE_LONGTEXT N_( \
   "The mouse wheel vertical (up/down) axis can control volume, " \
   "position or be ignored.")
#define MOUSE_X_WHEEL_MODE_TEXT N_("Mouse wheel horizontal axis control")
#define MOUSE_X_WHEEL_MODE_LONGTEXT N_( \
   "The mouse wheel horizontal (left/right) axis can control volume, " \
   "position or be ignored.")

#define JIEXTRASHORT_TEXT N_("Very short jump length")
#define JIEXTRASHORT_LONGTEXT N_("Very short jump length, in seconds.")
#define JISHORT_TEXT N_("Short jump length")
#define JISHORT_LONGTEXT N_("Short jump length, in seconds.")
#define JIMEDIUM_TEXT N_("Medium jump length")
#define JIMEDIUM_LONGTEXT N_("Medium jump length, in seconds.")
#define JILONG_TEXT N_("Long jump length")
#define JILONG_LONGTEXT N_("Long jump length, in seconds.")

#define AUDI_DEVICE_CYCLE_KEY_TEXT N_("Cycle through audio devices")
#define AUDI_DEVICE_CYCLE_KEY_LONGTEXT N_("Cycle through available audio devices")

vlc_module_begin ()
/* Audio options */
    set_category( CAT_AUDIO )
    set_subcategory( SUBCAT_AUDIO_GENERAL )
    add_category_hint( N_("Audio"), AOUT_CAT_LONGTEXT , false )

    add_bool( "audio", 1, AUDIO_TEXT, AUDIO_LONGTEXT, false )
        change_safe ()
    add_float( "gain", 1., GAIN_TEXT, GAIN_LONGTEXT, true )
        change_float_range( 0., 8. )
    add_obsolete_integer( "volume" ) /* since 2.1.0 */
    add_float( "volume-step", AOUT_VOLUME_STEP, VOLUME_STEP_TEXT,
                 VOLUME_STEP_LONGTEXT, true )
        change_float_range( 1., AOUT_VOLUME_DEFAULT )
    add_bool( "volume-save", true, VOLUME_SAVE_TEXT, VOLUME_SAVE_TEXT, true )
    add_obsolete_integer( "aout-rate" ) /* since 2.0.0 */
    add_obsolete_bool( "hq-resampling" ) /* since 1.1.8 */
    add_bool( "spdif", false, SPDIF_TEXT, SPDIF_LONGTEXT, true )
    add_integer( "force-dolby-surround", 0, FORCE_DOLBY_TEXT,
                 FORCE_DOLBY_LONGTEXT, false )
        change_integer_list( pi_force_dolby_values, ppsz_force_dolby_descriptions )
    add_integer( "stereo-mode", 0, STEREO_MODE_TEXT, STEREO_MODE_TEXT, true )
        change_integer_list( pi_stereo_mode_values, ppsz_stereo_mode_texts )
    add_integer( "audio-desync", 0, DESYNC_TEXT,
                 DESYNC_LONGTEXT, true )
        change_safe ()

    /* FIXME TODO create a subcat replay gain ? */
    add_string( "audio-replay-gain-mode", ppsz_replay_gain_mode[0], AUDIO_REPLAY_GAIN_MODE_TEXT,
                AUDIO_REPLAY_GAIN_MODE_LONGTEXT, false )
        change_string_list( ppsz_replay_gain_mode, ppsz_replay_gain_mode_text )
    add_float( "audio-replay-gain-preamp", 0.0,
               AUDIO_REPLAY_GAIN_PREAMP_TEXT, AUDIO_REPLAY_GAIN_PREAMP_LONGTEXT, false )
    add_float( "audio-replay-gain-default", -7.0,
               AUDIO_REPLAY_GAIN_DEFAULT_TEXT, AUDIO_REPLAY_GAIN_DEFAULT_LONGTEXT, false )
    add_bool( "audio-replay-gain-peak-protection", true,
              AUDIO_REPLAY_GAIN_PEAK_PROTECTION_TEXT, AUDIO_REPLAY_GAIN_PEAK_PROTECTION_LONGTEXT, true )

    add_bool( "audio-time-stretch", true,
              AUDIO_TIME_STRETCH_TEXT, AUDIO_TIME_STRETCH_LONGTEXT, false )

    set_subcategory( SUBCAT_AUDIO_AOUT )
    add_module( "aout", "audio output", NULL, AOUT_TEXT, AOUT_LONGTEXT,
                true )
        change_short('A')
    add_string( "role", "video", ROLE_TEXT, ROLE_LONGTEXT, true )
        change_string_list( ppsz_roles, ppsz_roles_text )

    set_subcategory( SUBCAT_AUDIO_AFILTER )
    add_module_list( "audio-filter", "audio filter", NULL,
                     AUDIO_FILTER_TEXT, AUDIO_FILTER_LONGTEXT, false )
    set_subcategory( SUBCAT_AUDIO_VISUAL )
    add_module( "audio-visual", "visualization", "none", AUDIO_VISUAL_TEXT,
                AUDIO_VISUAL_LONGTEXT, false )

    set_subcategory( SUBCAT_AUDIO_RESAMPLER )
    add_module( "audio-resampler", "audio resampler", NULL,
                AUDIO_RESAMPLER_TEXT, AUDIO_RESAMPLER_LONGTEXT, true )


/* Video options */
    set_category( CAT_VIDEO )
    set_subcategory( SUBCAT_VIDEO_GENERAL )
    add_category_hint( N_("Video"), VOUT_CAT_LONGTEXT , false )

    add_bool( "video", 1, VIDEO_TEXT, VIDEO_LONGTEXT, true )
        change_safe ()
    add_bool( "grayscale", 0, GRAYSCALE_TEXT,
              GRAYSCALE_LONGTEXT, true )
    add_bool( "fullscreen", false, FULLSCREEN_TEXT, FULLSCREEN_LONGTEXT, false )
        change_short('f')
        change_safe ()
    add_bool( "embedded-video", 1, EMBEDDED_TEXT, EMBEDDED_LONGTEXT,
              true )
    add_bool( "xlib", true, "", "", true )
        change_private ()
    add_bool( "drop-late-frames", 1, DROP_LATE_FRAMES_TEXT,
              DROP_LATE_FRAMES_LONGTEXT, true )
    /* Used in vout_synchro */
    add_bool( "skip-frames", 1, SKIP_FRAMES_TEXT,
              SKIP_FRAMES_LONGTEXT, true )
    add_bool( "quiet-synchro", 0, QUIET_SYNCHRO_TEXT,
              QUIET_SYNCHRO_LONGTEXT, true )
    add_obsolete_integer( "vout-event" ) /* deprecated since 1.1.0 */
    add_obsolete_integer( "x11-event" ) /* renamed since 1.0.0 */
    add_obsolete_bool( "overlay" ) /* renamed since 3.0.0 */
    add_bool( "video-on-top", 0, VIDEO_ON_TOP_TEXT,
              VIDEO_ON_TOP_LONGTEXT, false )
    add_bool( "video-wallpaper", false, WALLPAPER_TEXT,
              WALLPAPER_LONGTEXT, false )
    add_bool( "disable-screensaver", true, SS_TEXT, SS_LONGTEXT,
              true )

    add_bool( "video-title-show", 1, VIDEO_TITLE_SHOW_TEXT,
              VIDEO_TITLE_SHOW_LONGTEXT, false )
        change_safe()
    add_integer( "video-title-timeout", 5000, VIDEO_TITLE_TIMEOUT_TEXT,
                 VIDEO_TITLE_TIMEOUT_LONGTEXT, false )
        change_safe()
    add_integer( "video-title-position", 8, VIDEO_TITLE_POSITION_TEXT,
                 VIDEO_TITLE_POSITION_LONGTEXT, false )
        change_safe()
        change_integer_list( pi_pos_values, ppsz_pos_descriptions )
    // autohide after 1 second
    add_integer( "mouse-hide-timeout", 1000, MOUSE_HIDE_TIMEOUT_TEXT,
                 MOUSE_HIDE_TIMEOUT_LONGTEXT, false )
    set_section( N_("Snapshot") , NULL )
    add_directory( "snapshot-path", NULL, SNAP_PATH_TEXT,
                   SNAP_PATH_LONGTEXT, false )
    add_string( "snapshot-prefix", "vlcsnap-", SNAP_PREFIX_TEXT,
                   SNAP_PREFIX_LONGTEXT, false )
    add_string( "snapshot-format", "png", SNAP_FORMAT_TEXT,
                   SNAP_FORMAT_LONGTEXT, false )
        change_string_list( ppsz_snap_formats, ppsz_snap_formats )
    add_bool( "snapshot-preview", true, SNAP_PREVIEW_TEXT,
              SNAP_PREVIEW_LONGTEXT, false )
    add_bool( "snapshot-sequential", false, SNAP_SEQUENTIAL_TEXT,
              SNAP_SEQUENTIAL_LONGTEXT, false )
    add_integer( "snapshot-width", -1, SNAP_WIDTH_TEXT,
                 SNAP_WIDTH_LONGTEXT, true )
    add_integer( "snapshot-height", -1, SNAP_HEIGHT_TEXT,
                 SNAP_HEIGHT_LONGTEXT, true )

    set_section( N_("Window properties" ), NULL )
    add_integer( "width", -1, WIDTH_TEXT, WIDTH_LONGTEXT, true )
        change_safe ()
    add_integer( "height", -1, HEIGHT_TEXT, HEIGHT_LONGTEXT, true )
        change_safe ()
    add_integer( "video-x", 0, VIDEOX_TEXT, VIDEOX_LONGTEXT, true )
        change_safe ()
    add_integer( "video-y", 0, VIDEOY_TEXT, VIDEOY_LONGTEXT, true )
        change_safe ()
    add_string( "crop", NULL, CROP_TEXT, CROP_LONGTEXT, false )
        change_safe ()
    add_string( "aspect-ratio", NULL,
                ASPECT_RATIO_TEXT, ASPECT_RATIO_LONGTEXT, false )
        change_safe ()
    add_bool( "autoscale", true, AUTOSCALE_TEXT, AUTOSCALE_LONGTEXT, false )
        change_safe ()
    add_obsolete_float( "scale" ) /* since 3.0.0 */
    add_string( "monitor-par", NULL,
                MASPECT_RATIO_TEXT, MASPECT_RATIO_LONGTEXT, true )
    add_bool( "hdtv-fix", 1, HDTV_FIX_TEXT, HDTV_FIX_LONGTEXT, true )
    add_bool( "video-deco", 1, VIDEO_DECO_TEXT,
              VIDEO_DECO_LONGTEXT, true )
    add_string( "video-title", NULL, VIDEO_TITLE_TEXT,
                 VIDEO_TITLE_LONGTEXT, true )
    add_integer( "align", 0, ALIGN_TEXT, ALIGN_LONGTEXT, true )
        change_integer_list( pi_align_values, ppsz_align_descriptions )
    add_float( "zoom", 1., ZOOM_TEXT, ZOOM_LONGTEXT, true )
        change_safe()
    add_integer( "deinterlace", -1,
                 DEINTERLACE_TEXT, DEINTERLACE_LONGTEXT, false )
        change_integer_list( pi_deinterlace, ppsz_deinterlace_text )
        change_safe()
    add_string( "deinterlace-mode", "auto",
                DEINTERLACE_MODE_TEXT, DEINTERLACE_MODE_LONGTEXT, false )
        change_string_list( ppsz_deinterlace_mode, ppsz_deinterlace_mode_text )
        change_safe()

    set_subcategory( SUBCAT_VIDEO_VOUT )
    add_module( "vout", "vout display", NULL, VOUT_TEXT, VOUT_LONGTEXT, true )
        change_short('V')

    set_subcategory( SUBCAT_VIDEO_VFILTER )
    add_module_list( "video-filter", "video filter", NULL,
                     VIDEO_FILTER_TEXT, VIDEO_FILTER_LONGTEXT, false )

    set_subcategory( SUBCAT_VIDEO_SPLITTER )
    add_module_list( "video-splitter", "video splitter", NULL,
                     VIDEO_SPLITTER_TEXT, VIDEO_SPLITTER_LONGTEXT, false )
    add_obsolete_string( "vout-filter" ) /* since 2.0.0 */
#if 0
    add_string( "pixel-ratio", "1", PIXEL_RATIO_TEXT, PIXEL_RATIO_TEXT )
#endif

/* Subpictures options */
    set_subcategory( SUBCAT_VIDEO_SUBPIC )
    set_section( N_("On Screen Display") , NULL )
    add_category_hint( N_("Subpictures"), SUB_CAT_LONGTEXT , false )

    add_bool( "spu", 1, SPU_TEXT, SPU_LONGTEXT, false )
        change_safe ()
    add_bool( "osd", 1, OSD_TEXT, OSD_LONGTEXT, false )
    add_module( "text-renderer", "text renderer", NULL, TEXTRENDERER_TEXT,
                TEXTRENDERER_LONGTEXT, true )

    set_section( N_("Subtitles") , NULL )
    add_loadfile( "sub-file", NULL, SUB_FILE_TEXT,
                  SUB_FILE_LONGTEXT, false )
        change_safe()
    add_bool( "sub-autodetect-file", true,
                 SUB_AUTO_TEXT, SUB_AUTO_LONGTEXT, false )
    add_integer( "sub-autodetect-fuzzy", 3,
                 SUB_FUZZY_TEXT, SUB_FUZZY_LONGTEXT, true )
#if defined( _WIN32 ) || defined( __OS2__ )
#   define SUB_PATH ".\\subtitles, .\\subs"
#else
#   define SUB_PATH "./Subtitles, ./subtitles, ./Subs, ./subs"
#endif
    add_string( "sub-autodetect-path", SUB_PATH,
                 SUB_PATH_TEXT, SUB_PATH_LONGTEXT, true )
    add_integer( "sub-margin", 0, SUB_MARGIN_TEXT,
                 SUB_MARGIN_LONGTEXT, true )
    add_integer_with_range( "sub-text-scale", 100, 10, 500,
               SUB_TEXT_SCALE_TEXT, SUB_TEXT_SCALE_LONGTEXT, false )
        change_volatile  ()
    set_section( N_( "Overlays" ) , NULL )
    add_module_list( "sub-source", "sub source", NULL,
                     SUB_SOURCE_TEXT, SUB_SOURCE_LONGTEXT, false )
    add_module_list( "sub-filter", "sub filter", NULL,
                     SUB_FILTER_TEXT, SUB_FILTER_LONGTEXT, false )

/* Input options */
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_GENERAL )

    set_section( N_( "Track settings" ), NULL )
    add_integer( "program", 0,
                 INPUT_PROGRAM_TEXT, INPUT_PROGRAM_LONGTEXT, true )
        change_safe ()
    add_string( "programs", "",
                INPUT_PROGRAMS_TEXT, INPUT_PROGRAMS_LONGTEXT, true )
        change_safe ()
    add_integer( "audio-track", -1,
                 INPUT_AUDIOTRACK_TEXT, INPUT_AUDIOTRACK_LONGTEXT, true )
        change_safe ()
    add_integer( "sub-track", -1,
                 INPUT_SUBTRACK_TEXT, INPUT_SUBTRACK_LONGTEXT, true )
        change_safe ()
    add_string( "audio-language", "",
                 INPUT_AUDIOTRACK_LANG_TEXT, INPUT_AUDIOTRACK_LANG_LONGTEXT,
                  false )
        change_safe ()
    add_string( "sub-language", "",
                 INPUT_SUBTRACK_LANG_TEXT, INPUT_SUBTRACK_LANG_LONGTEXT,
                  false )
        change_safe ()
    add_string( "menu-language", "",
                 INPUT_MENUTRACK_LANG_TEXT, INPUT_MENUTRACK_LANG_LONGTEXT,
                  false )
        change_safe ()
    add_integer( "audio-track-id", -1, INPUT_AUDIOTRACK_ID_TEXT,
                 INPUT_AUDIOTRACK_ID_LONGTEXT, true )
        change_safe ()
    add_integer( "sub-track-id", -1,
                 INPUT_SUBTRACK_ID_TEXT, INPUT_SUBTRACK_ID_LONGTEXT, true )
        change_safe ()
    add_integer( "captions", 608,
                 INPUT_CAPTIONS_TEXT, INPUT_CAPTIONS_TEXT, true )
        change_integer_list( pi_captions, ppsz_captions )
        change_safe ()
    add_integer( "preferred-resolution", -1, INPUT_PREFERREDRESOLUTION_TEXT,
                 INPUT_PREFERREDRESOLUTION_LONGTEXT, false )
        change_safe ()
        change_integer_list( pi_prefres, ppsz_prefres )

    set_section( N_( "Playback control" ) , NULL)
    add_integer( "input-repeat", 0,
                 INPUT_REPEAT_TEXT, INPUT_REPEAT_LONGTEXT, false )
        change_integer_range( 0, 65535 )
        change_safe ()
    add_float( "start-time", 0,
               START_TIME_TEXT, START_TIME_LONGTEXT, true )
        change_safe ()
    add_float( "stop-time", 0,
               STOP_TIME_TEXT, STOP_TIME_LONGTEXT, true )
        change_safe ()
    add_float( "run-time", 0,
               RUN_TIME_TEXT, RUN_TIME_LONGTEXT, true )
        change_safe ()
    add_bool( "input-fast-seek", false,
              INPUT_FAST_SEEK_TEXT, INPUT_FAST_SEEK_LONGTEXT, false )
        change_safe ()
    add_float( "rate", 1.,
               INPUT_RATE_TEXT, INPUT_RATE_LONGTEXT, false )

    add_string( "input-list", NULL,
                 INPUT_LIST_TEXT, INPUT_LIST_LONGTEXT, true )
    add_string( "input-slave", NULL,
                 INPUT_SLAVE_TEXT, INPUT_SLAVE_LONGTEXT, true )

    add_string( "bookmarks", NULL,
                 BOOKMARKS_TEXT, BOOKMARKS_LONGTEXT, true )
        change_safe ()

    set_section( N_( "Default devices") , NULL )

    add_loadfile( "dvd", DVD_DEVICE, DVD_DEV_TEXT, DVD_DEV_LONGTEXT,
                  false )
    add_loadfile( "vcd", VCD_DEVICE, VCD_DEV_TEXT, VCD_DEV_LONGTEXT,
                  false )

    set_section( N_( "Network settings" ), NULL )

    add_integer( "mtu", MTU_DEFAULT, MTU_TEXT, MTU_LONGTEXT, true )
    add_obsolete_bool( "ipv6" ) /* since 2.0.0 */
    add_obsolete_bool( "ipv4" ) /* since 2.0.0 */
    add_integer( "ipv4-timeout", 5 * 1000, TIMEOUT_TEXT,
                 TIMEOUT_LONGTEXT, true )
        change_integer_range( 0, INT_MAX )

    add_string( "http-host", NULL, HTTP_HOST_TEXT, HOST_LONGTEXT, true )
    add_integer( "http-port", 8080, HTTP_PORT_TEXT, HTTP_PORT_LONGTEXT, true )
        change_integer_range( 1, 65535 )
    add_integer( "https-port", 8443, HTTPS_PORT_TEXT, HTTPS_PORT_LONGTEXT, true )
        change_integer_range( 1, 65535 )
    add_string( "rtsp-host", NULL, RTSP_HOST_TEXT, RTSP_HOST_LONGTEXT, true )
    add_integer( "rtsp-port", 554, RTSP_PORT_TEXT, RTSP_PORT_LONGTEXT, true )
        change_integer_range( 1, 65535 )
    add_loadfile( "http-cert", NULL, HTTP_CERT_TEXT, CERT_LONGTEXT, true )
    add_obsolete_string( "sout-http-cert" ) /* since 2.0.0 */
    add_loadfile( "http-key", NULL, HTTP_KEY_TEXT, KEY_LONGTEXT, true )
    add_obsolete_string( "sout-http-key" ) /* since 2.0.0 */
    add_obsolete_string( "http-ca" ) /* since 3.0.0 */
    add_obsolete_string( "sout-http-ca" ) /* since 2.0.0 */
    add_obsolete_string( "http-crl" ) /* since 3.0.0 */
    add_obsolete_string( "sout-http-crl" ) /* since 2.0.0 */

#ifdef _WIN32
    add_string( "http-proxy", NULL, PROXY_TEXT, PROXY_LONGTEXT,
                false )
    add_password( "http-proxy-pwd", NULL,
                  PROXY_PASS_TEXT, PROXY_PASS_LONGTEXT, false )
#else
    add_obsolete_string( "http-proxy" )
    add_obsolete_string( "http-proxy-pwd" )

#endif
    add_obsolete_bool( "http-use-IE-proxy" )

    set_section( N_( "Socks proxy") , NULL )
    add_string( "socks", NULL,
                 SOCKS_SERVER_TEXT, SOCKS_SERVER_LONGTEXT, true )
    add_string( "socks-user", NULL,
                 SOCKS_USER_TEXT, SOCKS_USER_LONGTEXT, true )
    add_string( "socks-pwd", NULL,
                 SOCKS_PASS_TEXT, SOCKS_PASS_LONGTEXT, true )


    set_section( N_("Metadata" ) , NULL )
    add_string( "meta-title", NULL, META_TITLE_TEXT,
                META_TITLE_LONGTEXT, true )
        change_safe()
    add_string( "meta-author", NULL, META_AUTHOR_TEXT,
                META_AUTHOR_LONGTEXT, true )
        change_safe()
    add_string( "meta-artist", NULL, META_ARTIST_TEXT,
                META_ARTIST_LONGTEXT, true )
        change_safe()
    add_string( "meta-genre", NULL, META_GENRE_TEXT,
                META_GENRE_LONGTEXT, true )
        change_safe()
    add_string( "meta-copyright", NULL, META_CPYR_TEXT,
                META_CPYR_LONGTEXT, true )
        change_safe()
    add_string( "meta-description", NULL, META_DESCR_TEXT,
                META_DESCR_LONGTEXT, true )
        change_safe()
    add_string( "meta-date", NULL, META_DATE_TEXT,
                META_DATE_LONGTEXT, true )
        change_safe()
    add_string( "meta-url", NULL, META_URL_TEXT,
                META_URL_LONGTEXT, true )
        change_safe()

    set_section( N_( "Advanced" ), NULL )

    add_integer( "file-caching", DEFAULT_PTS_DELAY / 1000,
                 CACHING_TEXT, CACHING_LONGTEXT, true )
        change_integer_range( 0, 60000 )
        change_safe()
    add_obsolete_integer( "vdr-caching" ) /* 2.0.0 */
    add_integer( "live-caching", DEFAULT_PTS_DELAY / 1000,
                 CAPTURE_CACHING_TEXT, CAPTURE_CACHING_LONGTEXT, true )
        change_integer_range( 0, 60000 )
        change_safe()
    add_obsolete_integer( "alsa-caching" ) /* 2.0.0 */
    add_obsolete_integer( "dshow-caching" ) /* 2.0.0 */
    add_obsolete_integer( "dv-caching" ) /* 2.0.0 */
    add_obsolete_integer( "dvb-caching" ) /* 2.0.0 */
    add_obsolete_integer( "eyetv-caching" ) /* 2.0.0 */
    add_obsolete_integer( "jack-input-caching" ) /* 2.0.0 */
    add_obsolete_integer( "linsys-hdsdi-caching" ) /* 2.0.0 */
    add_obsolete_integer( "linsys-sdi-caching" ) /* 2.0.0 */
    add_obsolete_integer( "oss-caching" ) /* 2.0.0 */
    add_obsolete_integer( "screen-caching" ) /* 2.0.0 */
    add_obsolete_integer( "v4l2-caching" ) /* 2.0.0 */
    add_integer( "disc-caching", DEFAULT_PTS_DELAY / 1000,
                 DISC_CACHING_TEXT, DISC_CACHING_LONGTEXT, true )
        change_integer_range( 0, 60000 )
        change_safe()
    add_obsolete_integer( "bd-caching" ) /* 2.0.0 */
    add_obsolete_integer( "bluray-caching" ) /* 2.0.0 */
    add_obsolete_integer( "cdda-caching" ) /* 2.0.0 */
    add_obsolete_integer( "dvdnav-caching" ) /* 2.0.0 */
    add_obsolete_integer( "dvdread-caching" ) /* 2.0.0 */
    add_obsolete_integer( "vcd-caching" ) /* 2.0.0 */
    add_integer( "network-caching", CLOCK_FREQ / 1000,
                 NETWORK_CACHING_TEXT, NETWORK_CACHING_LONGTEXT, true )
        change_integer_range( 0, 60000 )
        change_safe()
    add_obsolete_integer( "ftp-caching" ) /* 2.0.0 */
    add_obsolete_integer( "http-caching" ) /* 2.0.0 */
    add_obsolete_integer( "mms-caching" ) /* 2.0.0 */
    add_obsolete_integer( "realrtsp-caching" ) /* 2.0.0 */
    add_obsolete_integer( "rtp-caching" ) /* 2.0.0 */
    add_obsolete_integer( "rtsp-caching" ) /* 2.0.0 */
    add_obsolete_integer( "sftp-caching" ) /* 2.0.0 */
    add_obsolete_integer( "smb-caching" ) /* 2.0.0 */
    add_obsolete_integer( "tcp-caching" ) /* 2.0.0 */
    add_obsolete_integer( "udp-caching" ) /* 2.0.0 */

    add_integer( "cr-average", 40, CR_AVERAGE_TEXT,
                 CR_AVERAGE_LONGTEXT, true )
    add_integer( "clock-synchro", -1, CLOCK_SYNCHRO_TEXT,
                 CLOCK_SYNCHRO_LONGTEXT, true )
        change_integer_list( pi_clock_values, ppsz_clock_descriptions )
    add_integer( "clock-jitter", 5 * CLOCK_FREQ/1000, CLOCK_JITTER_TEXT,
              CLOCK_JITTER_LONGTEXT, true )
        change_safe()

    add_bool( "network-synchronisation", false, NETSYNC_TEXT,
              NETSYNC_LONGTEXT, true )

    add_directory( "input-record-path", NULL, INPUT_RECORD_PATH_TEXT,
                INPUT_RECORD_PATH_LONGTEXT, true )
    add_bool( "input-record-native", true, INPUT_RECORD_NATIVE_TEXT,
              INPUT_RECORD_NATIVE_LONGTEXT, true )

    add_directory( "input-timeshift-path", NULL, INPUT_TIMESHIFT_PATH_TEXT,
                INPUT_TIMESHIFT_PATH_LONGTEXT, true )
    add_integer( "input-timeshift-granularity", -1, INPUT_TIMESHIFT_GRANULARITY_TEXT,
                 INPUT_TIMESHIFT_GRANULARITY_LONGTEXT, true )

    add_string( "input-title-format", "$Z", INPUT_TITLE_FORMAT_TEXT, INPUT_TITLE_FORMAT_LONGTEXT, false );

    add_bool( "lua", true, INPUT_LUA_TEXT, INPUT_LUA_TEXT, true );

/* Decoder options */
    set_subcategory( SUBCAT_INPUT_VCODEC )
    add_category_hint( N_("Decoders"), CODEC_CAT_LONGTEXT , true )
    add_string( "codec", NULL, CODEC_TEXT,
                CODEC_LONGTEXT, true )
    add_string( "encoder",  NULL, ENCODER_TEXT,
                ENCODER_LONGTEXT, true )

    set_subcategory( SUBCAT_INPUT_ACCESS )
    add_category_hint( N_("Input"), INPUT_CAT_LONGTEXT , false )
    add_module( "access", "access", NULL, ACCESS_TEXT, ACCESS_LONGTEXT, true )

    set_subcategory( SUBCAT_INPUT_DEMUX )
    add_module( "demux", "demux", "any", DEMUX_TEXT, DEMUX_LONGTEXT, true )
    set_subcategory( SUBCAT_INPUT_ACODEC )
    set_subcategory( SUBCAT_INPUT_SCODEC )
    add_obsolete_bool( "prefer-system-codecs" )

    set_subcategory( SUBCAT_INPUT_STREAM_FILTER )
    add_module_list( "stream-filter", "stream_filter", NULL,
                     STREAM_FILTER_TEXT, STREAM_FILTER_LONGTEXT, false )

    add_string( "demux-filter", NULL, DEMUX_FILTER_TEXT, DEMUX_FILTER_LONGTEXT, true )

/* Stream output options */
    set_category( CAT_SOUT )
    set_subcategory( SUBCAT_SOUT_GENERAL )
    add_category_hint( N_("Stream output"), SOUT_CAT_LONGTEXT , true )

    add_string( "sout", NULL, SOUT_TEXT, SOUT_LONGTEXT, true )
    add_bool( "sout-display", false, SOUT_DISPLAY_TEXT,
                                SOUT_DISPLAY_LONGTEXT, true )
    add_bool( "sout-keep", false, SOUT_KEEP_TEXT,
                                SOUT_KEEP_LONGTEXT, true )
    add_bool( "sout-all", true, SOUT_ALL_TEXT,
                                SOUT_ALL_LONGTEXT, true )
    add_bool( "sout-audio", 1, SOUT_AUDIO_TEXT,
                                SOUT_AUDIO_LONGTEXT, true )
    add_bool( "sout-video", 1, SOUT_VIDEO_TEXT,
                                SOUT_VIDEO_LONGTEXT, true )
    add_bool( "sout-spu", 1, SOUT_SPU_TEXT,
                                SOUT_SPU_LONGTEXT, true )
    add_integer( "sout-mux-caching", 1500, SOUT_MUX_CACHING_TEXT,
                                SOUT_MUX_CACHING_LONGTEXT, true )

    set_section( N_("VLM"), NULL )
    add_loadfile( "vlm-conf", NULL, VLM_CONF_TEXT,
                    VLM_CONF_LONGTEXT, true )



    set_subcategory( SUBCAT_SOUT_STREAM )
    add_integer( "sap-interval", 5, ANN_SAPINTV_TEXT,
                               ANN_SAPINTV_LONGTEXT, true )

    set_subcategory( SUBCAT_SOUT_MUX )
    add_module( "mux", "sout mux", NULL, MUX_TEXT, MUX_LONGTEXT, true )
    set_subcategory( SUBCAT_SOUT_ACO )
    add_module( "access_output", "sout access", NULL,
                ACCESS_OUTPUT_TEXT, ACCESS_OUTPUT_LONGTEXT, true )
    add_integer( "ttl", -1, TTL_TEXT, TTL_LONGTEXT, true )
    add_string( "miface", NULL, MIFACE_TEXT, MIFACE_LONGTEXT, true )
    add_obsolete_string( "miface-addr" ) /* since 2.0.0 */
    add_integer( "dscp", 0, DSCP_TEXT, DSCP_LONGTEXT, true )

    set_subcategory( SUBCAT_SOUT_PACKETIZER )
    add_module( "packetizer", "packetizer", NULL,
                PACKETIZER_TEXT, PACKETIZER_LONGTEXT, true )

    set_subcategory( SUBCAT_SOUT_VOD )

/* CPU options */
    set_category( CAT_ADVANCED )
    add_obsolete_bool( "fpu" )
#if defined( __i386__ ) || defined( __x86_64__ )
    add_obsolete_bool( "mmx" ) /* since 2.0.0 */
    add_obsolete_bool( "3dn" ) /* since 2.0.0 */
    add_obsolete_bool( "mmxext" ) /* since 2.0.0 */
    add_obsolete_bool( "sse" ) /* since 2.0.0 */
    add_obsolete_bool( "sse2" ) /* since 2.0.0 */
    add_obsolete_bool( "sse3" ) /* since 2.0.0 */
    add_obsolete_bool( "ssse3" ) /* since 2.0.0 */
    add_obsolete_bool( "sse41" ) /* since 2.0.0 */
    add_obsolete_bool( "sse42" ) /* since 2.0.0 */
#endif
#if defined( __powerpc__ ) || defined( __ppc__ ) || defined( __ppc64__ )
    add_obsolete_bool( "altivec" ) /* since 2.0.0 */
#endif

/* Misc options */
    set_subcategory( SUBCAT_ADVANCED_MISC )
    set_section( N_("Special modules"), NULL )
    add_category_hint( N_("Miscellaneous"), MISC_CAT_LONGTEXT, true )
    add_module( "vod-server", "vod server", NULL, VOD_SERVER_TEXT,
                VOD_SERVER_LONGTEXT, true )

    set_section( N_("Plugins" ), NULL )
    add_obsolete_string( "data-path" ) /* since 2.1.0 */
    add_string( "keystore", NULL, KEYSTORE_TEXT,
                KEYSTORE_LONGTEXT, true )

    set_section( N_("Performance options"), NULL )

/* Playlist options */
    set_category( CAT_PLAYLIST )
    set_subcategory( SUBCAT_PLAYLIST_GENERAL )
    add_category_hint( N_("Playlist"), PLAYLIST_CAT_LONGTEXT , false )
    add_bool( "random", 0, RANDOM_TEXT, RANDOM_LONGTEXT, false )
        change_short('Z')
        change_safe()
    add_bool( "loop", 0, LOOP_TEXT, LOOP_LONGTEXT, false )
        change_short('L')
        change_safe()
    add_bool( "repeat", 0, REPEAT_TEXT, REPEAT_LONGTEXT, false )
        change_short('R')
        change_safe()
    add_bool( "play-and-exit", 0, PAE_TEXT, PAE_LONGTEXT, false )
    add_bool( "play-and-stop", 0, PAS_TEXT, PAS_LONGTEXT, false )
        change_safe()
    add_bool( "play-and-pause", 0, PAP_TEXT, PAP_LONGTEXT, true )
        change_safe()
    add_bool( "start-paused", 0, SP_TEXT, SP_LONGTEXT, false )
    add_bool( "playlist-autostart", true,
              AUTOSTART_TEXT, AUTOSTART_LONGTEXT, false )
    add_bool( "playlist-cork", true, CORK_TEXT, CORK_LONGTEXT, false )
#if defined(_WIN32) || defined(HAVE_DBUS) || defined(__OS2__)
    add_bool( "one-instance", 0, ONEINSTANCE_TEXT,
              ONEINSTANCE_LONGTEXT, true )
    add_bool( "started-from-file", 0, STARTEDFROMFILE_TEXT,
              STARTEDFROMFILE_LONGTEXT, true )
        change_volatile ()
    add_bool( "one-instance-when-started-from-file", 1,
              ONEINSTANCEWHENSTARTEDFROMFILE_TEXT,
              ONEINSTANCEWHENSTARTEDFROMFILE_TEXT, true )
    add_bool( "playlist-enqueue", 0, PLAYLISTENQUEUE_TEXT,
              PLAYLISTENQUEUE_LONGTEXT, true )
#endif
#ifdef HAVE_DBUS
    add_bool( "dbus", false, DBUS_TEXT, DBUS_LONGTEXT, true )
#endif
    add_bool( "playlist-tree", 0, PLTREE_TEXT, PLTREE_LONGTEXT, false )

    add_string( "open", "", OPEN_TEXT, OPEN_LONGTEXT, false )

    add_bool( "auto-preparse", true, PREPARSE_TEXT,
              PREPARSE_LONGTEXT, false )

    add_integer( "preparse-timeout", 5000, PREPARSE_TIMEOUT_TEXT,
                 PREPARSE_TIMEOUT_LONGTEXT, false )

    add_obsolete_integer( "album-art" )
    add_bool( "metadata-network-access", false, METADATA_NETWORK_TEXT,
                 METADATA_NETWORK_TEXT, false )

    add_string( "recursive", "collapse" , RECURSIVE_TEXT,
                RECURSIVE_LONGTEXT, false )
        change_string_list( psz_recursive_list, psz_recursive_list_text )
    add_string( "ignore-filetypes", "m3u,db,nfo,ini,jpg,jpeg,ljpg,gif,png,pgm,"
                "pgmyuv,pbm,pam,tga,bmp,pnm,xpm,xcf,pcx,tif,tiff,lbm,sfv,txt,"
                "sub,idx,srt,cue,ssa",
                IGNORE_TEXT, IGNORE_LONGTEXT, false )
    add_bool( "show-hiddenfiles", false,
              SHOW_HIDDENFILES_TEXT, SHOW_HIDDENFILES_LONGTEXT, false )
    add_bool( "extractor-flatten", false,
              "Flatten files listed by extractors (archive)", NULL, true )
        change_volatile()

    set_subcategory( SUBCAT_PLAYLIST_SD )
    add_string( "services-discovery", "", SD_TEXT, SD_LONGTEXT, true )
        change_short('S')

/* Interface options */
    set_category( CAT_INTERFACE )
    set_subcategory( SUBCAT_INTERFACE_GENERAL )
    add_integer( "verbose", 0, VERBOSE_TEXT, VERBOSE_LONGTEXT,
                 false )
        change_short('v')
        change_volatile ()
    add_obsolete_string( "verbose-objects" ) /* since 2.1.0 */
#if !defined(_WIN32) && !defined(__OS2__)
    add_bool( "daemon", 0, DAEMON_TEXT, DAEMON_LONGTEXT, true )
        change_short('d')

    add_string( "pidfile", NULL, PIDFILE_TEXT, PIDFILE_LONGTEXT,
                                       false )
#endif

#if defined (_WIN32) || defined (__APPLE__)
    add_obsolete_string( "language" ) /* since 2.1.0 */
#endif

    add_bool( "color", true, COLOR_TEXT, COLOR_LONGTEXT, true )
        change_volatile ()
    add_bool( "advanced", false, ADVANCED_TEXT, ADVANCED_LONGTEXT,
                    false )
    add_bool( "interact", true, INTERACTION_TEXT,
              INTERACTION_LONGTEXT, false )

    add_bool ( "stats", true, STATS_TEXT, STATS_LONGTEXT, true )

    set_subcategory( SUBCAT_INTERFACE_MAIN )
    add_module_cat( "intf", SUBCAT_INTERFACE_MAIN, NULL, INTF_TEXT,
                INTF_LONGTEXT, false )
        change_short('I')
    add_module_list_cat( "extraintf", SUBCAT_INTERFACE_MAIN, NULL,
                         EXTRAINTF_TEXT, EXTRAINTF_LONGTEXT, false )


    set_subcategory( SUBCAT_INTERFACE_CONTROL )
    add_module_list_cat( "control", SUBCAT_INTERFACE_CONTROL, NULL,
                         CONTROL_TEXT, CONTROL_LONGTEXT, false )

/* Hotkey options*/
    set_subcategory( SUBCAT_INTERFACE_HOTKEYS )
    add_category_hint( N_("Hot keys"), HOTKEY_CAT_LONGTEXT , false )

    set_section ( N_("Jump sizes" ), NULL )
    add_integer( "extrashort-jump-size", 3, JIEXTRASHORT_TEXT,
                                    JIEXTRASHORT_LONGTEXT, false )
    add_integer( "short-jump-size", 10, JISHORT_TEXT,
                                    JISHORT_LONGTEXT, false )
    add_integer( "medium-jump-size", 60, JIMEDIUM_TEXT,
                                    JIMEDIUM_LONGTEXT, false )
    add_integer( "long-jump-size", 300, JILONG_TEXT,
                                    JILONG_LONGTEXT, false )

    set_description( N_("core program") )
vlc_module_end ()

/*****************************************************************************
 * End configuration.
 *****************************************************************************/

#define DECL_PLUGIN(p) \
    int vlc_entry__##p(int (*)(void *, void *, int, ...), void *);

#define FUNC_PLUGIN(p) \
    vlc_entry__##p,

#define ACCESS_PLUGINS(f) \
    f(access_file) \
    f(access_mms) \
    f(access_demux_rtp) \
    f(access_realrtsp) \
    f(access_demux_rtsp) \
    f(access_demux_vod)

#define DEMUX_PLUGINS(f) \
    f(meta_folder) \
    f(demux_asf) \
    f(demux_mp4) \
    f(demux_mkv) \
    f(demux_avi) \
    f(demux_dv) \
    f(demux_mpegps) \
    f(demux_mpegts) \
    f(demux_mpgv) \
    f(demux_audio_es) \
    f(demux_h264) \
    f(demux_text_subtitle) \
    f(demux_vobsub) \
    f(demux_ffmpeg)

#define MUX_PLUGINS(f) \
    f(mux_mpegts)

#define PACKETIZER_PLUGINS(f) \
    f(packetizer_copy) \
    f(packetizer_av1) \
    f(packetizer_a52) \
    f(packetizer_dts) \
    f(packetizer_h264) \
    f(packetizer_hevc) \
    f(packetizer_mlp) \
    f(packetizer_mpeg4video) \
    f(packetizer_mpeg4audio) \
    f(packetizer_mpegvideo) \
    f(packetizer_mpegaudio) \
    f(packetizer_vc1) \
    f(packetizer_dirac) \
    f(packetizer_flac) \
    f(packetizer_ffmpeg)

#define CODEC_PLUGINS(f) \
    f(codec_ffmpeg) \
    f(codec_cc) \
    f(codec_libass) \
    f(codec_spu) \
    f(codec_subs) \
    f(codec_scte27) \
    f(codec_rawvideo) \
    f(codec_araw) \
    f(codec_x264)

#define TEXT_RENDER_PLUGINS(f) \
    f(trender_dummy) \
    f(trender_freetype2)

#define FILTER_PLUGINS(f) \
    f(afilter_format) \
    f(afilter_scaletempo) \
    f(afilter_bandlimited) \
    f(afilter_ugly) \
    f(afilter_a52) \
    f(vchroma_chain) \
    f(vchroma_grey_yuv) \
    f(vchroma_i420_nv12) \
    f(vchroma_i422_i420) \
    f(vchroma_rv32) \
    f(vchroma_ffmpeg) \
    f(vchroma_yuvp) \
    f(vchroma_yuy2_i420) \
    f(vchroma_yuy2_i422) \
    f(vchroma_i420_10_p010) \
    f(vfilter_deinterlace) \
    f(vblend)

#define OUTPUT_PLUGINS(f) \
    f(aout_adummy) \
    f(aout_file) \
    f(vwin_drawable) \
    f(vout_vdummy) \
    f(vout_yuv)

#define SOUT_PLUGINS(f) \
    f(sout_rtp) \
    f(sout_duplicate) \
    f(sout_display) \
    f(sout_transcode)

#if defined(_WIN32)
#define SPECIFIC_PLUGINS(f) \
    f(aout_waveout) \
    f(vout_direct3d) \
    f(vout_directdraw) \
    f(vout_wingdi) \
    f(hwdec_dxva2) \
    f(access_demux_dshow) \
    f(access_demux_screen)
#elif defined(__ANDROID__)
#define SPECIFIC_PLUGINS(f) \
    f(codec_mediacodec) \
    /*f(codec_omxil)*/ \
    f(aout_audiotrack) \
    f(vwin_android) \
    f(vout_android) \
    f(vout_omxil)
#else
#define SPECIFIC_PLUGINS(f)     
#endif

#define PLUGINS(f) \
    ACCESS_PLUGINS(f) \
    DEMUX_PLUGINS(f) \
    MUX_PLUGINS(f) \
    PACKETIZER_PLUGINS(f) \
    CODEC_PLUGINS(f) \
    TEXT_RENDER_PLUGINS(f) \
    FILTER_PLUGINS(f) \
    OUTPUT_PLUGINS(f) \
    SOUT_PLUGINS(f) \
    SPECIFIC_PLUGINS(f)

PLUGINS(DECL_PLUGIN)

vlc_plugin_cb vlc_static_modules[] = { 
        PLUGINS(FUNC_PLUGIN) 
        NULL 
};
