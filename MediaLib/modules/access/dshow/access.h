/*****************************************************************************
 * access.h : DirectShow access module for vlc
 *****************************************************************************/

#include <dshow.h>

#include <vector>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

/****************************************************************************
 * Crossbar stuff
 ****************************************************************************/
#define MAX_CROSSBAR_DEPTH 10

struct CrossbarRoute
{
    ComPtr<IAMCrossbar> pXbar;
    LONG        VideoInputIndex;
    LONG        VideoOutputIndex;
    LONG        AudioInputIndex;
    LONG        AudioOutputIndex;
};

void DeleteCrossbarRoutes(struct access_sys_t *);
HRESULT FindCrossbarRoutes(vlc_object_t *, struct access_sys_t *, IPin *, LONG, int = 0);

/****************************************************************************
 * Access descriptor declaration
 ****************************************************************************/
struct access_sys_t
{
    /* These 2 must be left at the beginning */
    vlc_mutex_t lock;
    vlc_cond_t wait;

    IFilterGraph* p_graph;
    ICaptureGraphBuilder2* p_capture_graph_builder2;
    IMediaControl* p_control;

    int i_crossbar_route_depth;
    CrossbarRoute crossbar_routes[MAX_CROSSBAR_DEPTH];

    /* list of elementary streams */
    std::vector<struct dshow_stream_t*> pp_streams;
    int i_current_stream;

    /* misc properties */
    int i_width;
    int i_height;
    int i_chroma;
    mtime_t i_start;
};

