/*****************************************************************************
 * picture_fifo.c : picture fifo functions
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_picture_fifo.h>

#include <assert.h>

/*****************************************************************************
 *
 *****************************************************************************/
struct picture_fifo_t {
    vlc_mutex_t lock;
    picture_t   *first;
    picture_t   **last_ptr;
};

static void PictureFifoReset(picture_fifo_t *fifo)
{
    fifo->first    = NULL;
    fifo->last_ptr = &fifo->first;
}
static void PictureFifoPush(picture_fifo_t *fifo, picture_t *picture)
{
    assert(!picture->p_next);
    *fifo->last_ptr = picture;
    fifo->last_ptr  = &picture->p_next;
}
static picture_t *PictureFifoPop(picture_fifo_t *fifo)
{
    picture_t *picture = fifo->first;

    if (picture) {
        fifo->first = picture->p_next;
        if (!fifo->first)
            fifo->last_ptr = &fifo->first;
        picture->p_next = NULL;
    }
    return picture;
}

picture_fifo_t *picture_fifo_New(void)
{
    picture_fifo_t *fifo = malloc(sizeof(*fifo));
    if (!fifo)
        return NULL;

    vlc_mutex_init(&fifo->lock);
    PictureFifoReset(fifo);
    return fifo;
}

void picture_fifo_Push(picture_fifo_t *fifo, picture_t *picture)
{
    vlc_mutex_lock(&fifo->lock);
    PictureFifoPush(fifo, picture);
    vlc_mutex_unlock(&fifo->lock);
}
picture_t *picture_fifo_Pop(picture_fifo_t *fifo)
{
    vlc_mutex_lock(&fifo->lock);
    picture_t *picture = PictureFifoPop(fifo);
    vlc_mutex_unlock(&fifo->lock);

    return picture;
}
picture_t *picture_fifo_Peek(picture_fifo_t *fifo)
{
    vlc_mutex_lock(&fifo->lock);
    picture_t *picture = fifo->first;
    if (picture)
        picture_Hold(picture);
    vlc_mutex_unlock(&fifo->lock);

    return picture;
}
void picture_fifo_Flush(picture_fifo_t *fifo, mtime_t date, bool flush_before)
{
    picture_t *picture;

    vlc_mutex_lock(&fifo->lock);

    picture = fifo->first;
    PictureFifoReset(fifo);

    picture_fifo_t tmp;
    PictureFifoReset(&tmp);

    while (picture) {
        picture_t *next = picture->p_next;

        picture->p_next = NULL;
        if (( flush_before && picture->date <= date) ||
            (!flush_before && picture->date >= date))
            PictureFifoPush(&tmp, picture);
        else
            PictureFifoPush(fifo, picture);
        picture = next;
    }
    vlc_mutex_unlock(&fifo->lock);

    while ((picture = PictureFifoPop(&tmp)) != NULL)
        picture_Release(picture);
}
void picture_fifo_OffsetDate(picture_fifo_t *fifo, mtime_t delta)
{
    vlc_mutex_lock(&fifo->lock);
    for (picture_t *picture = fifo->first; picture != NULL;) {
        picture->date += delta;
        picture = picture->p_next;
    }
    vlc_mutex_unlock(&fifo->lock);
}
void picture_fifo_Delete(picture_fifo_t *fifo)
{
    picture_fifo_Flush(fifo, INT64_MAX, true);
    vlc_mutex_destroy(&fifo->lock);
    free(fifo);
}

