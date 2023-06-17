/*****************************************************************************
 * omxil.c: Video decoder module making use of OpenMAX IL components.
 *****************************************************************************/

#include "OMX_Core.h"

OMX_ERRORTYPE (*pf_init) (void);
OMX_ERRORTYPE (*pf_deinit) (void);
OMX_ERRORTYPE (*pf_get_handle) (OMX_HANDLETYPE *, OMX_STRING,
                                OMX_PTR, OMX_CALLBACKTYPE *);
OMX_ERRORTYPE (*pf_free_handle) (OMX_HANDLETYPE);
OMX_ERRORTYPE (*pf_component_enum)(OMX_STRING, OMX_U32, OMX_U32);
OMX_ERRORTYPE (*pf_get_roles_of_component)(OMX_STRING, OMX_U32 *, OMX_U8 **);

/* Extra IOMX android functions. Can be NULL if we don't link with libiomx */
OMX_ERRORTYPE (*pf_enable_graphic_buffers)(OMX_HANDLETYPE, OMX_U32, OMX_BOOL);
OMX_ERRORTYPE (*pf_get_graphic_buffer_usage)(OMX_HANDLETYPE, OMX_U32, OMX_U32*);
OMX_ERRORTYPE (*pf_get_hal_format) (const char *, int *);

int InitOmxCore(vlc_object_t *p_this);
void DeinitOmxCore(void);

#define MAX_COMPONENTS_LIST_SIZE 32
int CreateComponentsList(vlc_object_t *p_this, const char *psz_role,
                         char ppsz_components[MAX_COMPONENTS_LIST_SIZE][OMX_MAX_STRINGNAME_SIZE]);

