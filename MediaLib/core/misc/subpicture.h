/*****************************************************************************
 * subpicture.h: Private subpicture definitions
 *****************************************************************************/

struct subpicture_region_private_t {
    video_format_t fmt;
    picture_t      *p_picture;
};

subpicture_region_private_t *subpicture_region_private_New(video_format_t *);
void subpicture_region_private_Delete(subpicture_region_private_t *);

