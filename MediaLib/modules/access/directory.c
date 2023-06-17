/*****************************************************************************
 * directory.c: expands a directory (directory: access_browser plug-in)
 *****************************************************************************/

#include <limits.h>
#include <sys/stat.h>

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include "fs.h"
#include <vlc_access.h>
#include <vlc_input_item.h>

#include <vlc_fs.h>
#include <vlc_url.h>

struct access_sys_t
{
    char *base_uri;
    bool need_separator;
    DIR *dir;
};

/*****************************************************************************
 * DirInit: Init the directory access with a directory stream
 *****************************************************************************/
int DirInit (stream_t *access, DIR *dir)
{
    access_sys_t *sys = vlc_obj_malloc(VLC_OBJECT(access), sizeof (*sys));
    if (unlikely(sys == NULL))
        goto error;

    if (!strcmp(access->psz_name, "fd"))
    {
        if (unlikely(asprintf(&sys->base_uri, "fd://%s",
                              access->psz_location) == -1))
            sys->base_uri = NULL;
    }
    else
        sys->base_uri = vlc_path2uri(access->psz_filepath, "file");
    if (unlikely(sys->base_uri == NULL))
        goto error;

    char last_char = sys->base_uri[strlen(sys->base_uri) - 1];
    sys->need_separator =
#ifdef _WIN32
        last_char != '\\' &&
#endif
        last_char != '/';
    sys->dir = dir;

    access->p_sys = sys;
    access->pf_readdir = DirRead;
    access->pf_control = access_vaDirectoryControlHelper;
    return VLC_SUCCESS;

error:
    closedir(dir);
    return VLC_ENOMEM;
}

/*****************************************************************************
 * DirOpen: Open the directory access
 *****************************************************************************/
int DirOpen (vlc_object_t *obj)
{
    stream_t *access = (stream_t *)obj;

    if (access->psz_filepath == NULL)
        return VLC_EGENERIC;

    DIR *dir = vlc_opendir(access->psz_filepath);
    if (dir == NULL)
        return VLC_EGENERIC;

    return DirInit(access, dir);
}

/*****************************************************************************
 * Close: close the target
 *****************************************************************************/
void DirClose(vlc_object_t *obj)
{
    stream_t *access = (stream_t *)obj;
    access_sys_t *sys = access->p_sys;

    free(sys->base_uri);
    closedir(sys->dir);
}

int DirRead (stream_t *access, input_item_node_t *node)
{
    access_sys_t *sys = access->p_sys;
    const char *entry;
    int ret = VLC_SUCCESS;

    bool special_files = var_InheritBool(access, "list-special-files");

    struct vlc_readdir_helper rdh;
    vlc_readdir_helper_init(&rdh, access, node);

    while (ret == VLC_SUCCESS && (entry = vlc_readdir(sys->dir)) != NULL)
    {
        struct stat st;
        int type;

#ifdef HAVE_OPENAT
        if (fstatat(dirfd(sys->dir), entry, &st, 0))
            continue;
#else
        char *path;

        if (asprintf(&path, "%s"DIR_SEP"%s", access->psz_filepath, entry) == -1
            || (type = vlc_stat(path, &st), free(path), type))
            continue;
#endif
        switch (st.st_mode & S_IFMT)
        {
            case S_IFBLK:
                if (!special_files)
                    continue;
                type = ITEM_TYPE_DISC;
                break;
            case S_IFCHR:
                if (!special_files)
                    continue;
                type = ITEM_TYPE_CARD;
                break;
            case S_IFIFO:
                if (!special_files)
                    continue;
                type = ITEM_TYPE_STREAM;
                break;
            case S_IFREG:
                type = ITEM_TYPE_FILE;
                break;
            case S_IFDIR:
                type = ITEM_TYPE_DIRECTORY;
                break;
            /* S_IFLNK cannot occur while following symbolic links */
            /* S_IFSOCK cannot be opened with open()/openat() */
            default:
                continue; /* ignore */
        }

        /* Create an input item for the current entry */
        char *encoded = vlc_uri_encode(entry);
        if (unlikely(encoded == NULL))
        {
            ret = VLC_ENOMEM;
            break;
        }

        char *uri;
        if (unlikely(asprintf(&uri, "%s%s%s", sys->base_uri, sys->need_separator ? "/" : "", encoded) == -1))
            uri = NULL;
        free(encoded);
        if (unlikely(uri == NULL))
        {
            ret = VLC_ENOMEM;
            break;
        }
        ret = vlc_readdir_helper_additem(&rdh, uri, NULL, entry, type,
                                         ITEM_NET_UNKNOWN);
        free(uri);
    }

    vlc_readdir_helper_finish(&rdh, ret == VLC_SUCCESS);

    return ret;
}
