/*****************************************************************************
 * vlc_objects.h: vlc_object_t definition and manipulation methods
 *****************************************************************************/

/**
 * \defgroup vlc_object VLC objects
 * @{
 * \file
 * Common VLC object defintions
 */

/* Object flags */
#define OBJECT_FLAGS_QUIET       0x0002
#define OBJECT_FLAGS_NOINTERACT  0x0004
#define OBJECT_FLAGS_INSECURE    0x1000 /* VLC 3.0 only, will be removed */

/*****************************************************************************
 * The vlc_object_t type. Yes, it's that simple :-)
 *****************************************************************************/
/** The main vlc_object_t structure */
struct vlc_object_t
{
    VLC_COMMON_MEMBERS
};

/*****************************************************************************
 * Prototypes
 *****************************************************************************/
VLC_API void *vlc_object_create( vlc_object_t *, size_t ) VLC_MALLOC VLC_USED;
VLC_API vlc_object_t *vlc_object_find_name( vlc_object_t *, const char * ) VLC_USED VLC_DEPRECATED;
VLC_API void * vlc_object_hold( vlc_object_t * );
VLC_API void vlc_object_release( vlc_object_t * );
VLC_API vlc_list_t *vlc_list_children( vlc_object_t * ) VLC_USED;
VLC_API void vlc_list_release( vlc_list_t * );
VLC_API char *vlc_object_get_name( const vlc_object_t * ) VLC_USED;
#define vlc_object_get_name(o) vlc_object_get_name(VLC_OBJECT(o))

#define vlc_object_create(a,b) vlc_object_create( VLC_OBJECT(a), b )

#define vlc_object_find_name(a,b) \
    vlc_object_find_name( VLC_OBJECT(a),b)

#define vlc_object_hold(a) \
    vlc_object_hold( VLC_OBJECT(a) )

#define vlc_object_release(a) \
    vlc_object_release( VLC_OBJECT(a) )

#define vlc_list_children(a) \
    vlc_list_children( VLC_OBJECT(a) )

VLC_API VLC_MALLOC void *vlc_obj_malloc(vlc_object_t *, size_t);
VLC_API VLC_MALLOC void *vlc_obj_calloc(vlc_object_t *, size_t, size_t);
VLC_API void vlc_obj_free(vlc_object_t *, void *);

/**
* Creates a VLC object.
*
* Note that because the object name pointer must remain valid, potentially
* even after the destruction of the object (through the message queues), this
* function CANNOT be exported to plugins as is. In this case, the old
* vlc_object_create() must be used instead.
*
* @param p_this an existing VLC object
* @param i_size byte size of the object structure
* @param psz_type object type name
* @return the created object, or NULL.
*/
VLC_API void* vlc_custom_create(vlc_object_t *p_this, size_t i_size, const char *psz_type);
#define vlc_custom_create(o, s, n)      vlc_custom_create(VLC_OBJECT(o), s, n)

/**
* Assign a name to an object for vlc_object_find_name().
*/
VLC_API int vlc_object_set_name(vlc_object_t *, const char *);
#define vlc_object_set_name(o, n)       vlc_object_set_name(VLC_OBJECT(o), n)

/* Types */
typedef void(*vlc_destructor_t)(struct vlc_object_t *);
VLC_API void vlc_object_set_destructor(vlc_object_t *, vlc_destructor_t);
#define vlc_object_set_destructor(a, b) \
        vlc_object_set_destructor(VLC_OBJECT(a), b)

/**
* Allocates an object resource.
*
* @param size storage size in bytes of the resource data
* @param release callback to release the resource
*
* @return a pointer to the (uninitialized) storage space, or NULL on error
*/
void *vlc_objres_new(size_t size, void(*release)(void *));

/**
* Pushes an object resource on the object resources stack.
*
* @param obj object to allocate the resource for
* @param data resource base address (as returned by vlc_objres_new())
*/
void vlc_objres_push(vlc_object_t *obj, void *data);

/**
* Releases all resources of an object.
*
* All resources added with vlc_objres_add() are released in reverse order.
* The resource list is reset to empty.
*
* @param obj object whose resources to release
*/
void vlc_objres_clear(vlc_object_t *obj);

/**
* Releases one object resource explicitly.
*
* If a resource associated with an object needs to be released explicitly
* earlier than normal, call this function. This is relatively slow and should
* be avoided.
*
* @param obj object whose resource to release
* @param data private data for the comparison function
* @param match comparison function to match the targeted resource
*/
void vlc_objres_remove(vlc_object_t *obj, void *data,
    bool(*match)(void *, void *));

/** @} */
