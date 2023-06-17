/*****************************************************************************
 * fs.c: file system access plugin
 *****************************************************************************/

#include <vlc_common.h>
#include <vlc_fixups.h>
#include <vlc_threads.h>
#include "fs.h"
#include <vlc_plugin.h>

#define MODULE_NAME     access_file
#define MODULE_STRING   "access_file"

vlc_module_begin ()
    set_description( N_("File input") )
    set_shortname( N_("File") )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_ACCESS )
    add_obsolete_string( "file-cat" )
    set_capability( "access", 50 )
    add_shortcut( "file", "fd", "stream" )
    set_callbacks( FileOpen, FileClose )

    add_submodule()
    set_section( N_("Directory" ), NULL )
    set_capability( "access", 55 )
#ifndef HAVE_FDOPENDIR
    add_shortcut( "file", "directory", "dir" )
#else
    add_shortcut( "directory", "dir" )
#endif
    set_callbacks( DirOpen, DirClose )

    add_bool("list-special-files", false, N_("List special files"),
             N_("Include devices and pipes when listing directories"), true)
    add_obsolete_string("directory-sort") /* since 3.0.0 */
vlc_module_end ()
