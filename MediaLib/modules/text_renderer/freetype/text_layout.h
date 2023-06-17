/*****************************************************************************
 * text_layout.h : Text shaping and layout
 *****************************************************************************/

/** \ingroup freetype
 * @{
 * \file
 * Text shaping and layout
 */

#include "freetype2.h"

typedef struct
{
    FT_BitmapGlyph p_glyph;
    FT_BitmapGlyph p_outline;
    FT_BitmapGlyph p_shadow;
    FT_BBox        bbox;
    const text_style_t *p_style;
    int            i_line_offset;       /* underline/strikethrough offset */
    int            i_line_thickness;    /* underline/strikethrough thickness */
    bool           b_in_karaoke;
} line_character_t;

typedef struct line_desc_t line_desc_t;
struct line_desc_t
{
    line_desc_t      *p_next;

    int              i_width;
    int              i_height;
    int              i_base_line;
    int              i_character_count;
    int              i_first_visible_char_index;
    int              i_last_visible_char_index;
    line_character_t *p_character;
    FT_BBox          bbox;
};

void FreeLines( line_desc_t *p_lines );
line_desc_t *NewLine( int i_count );

/**
 * Layout the text with shaping, bidirectional support, and font fallback if available.
 *
 * \param p_filter the FreeType module object [IN]
 * \param psz_text array of size \p i_len containing character codepoints [IN]
 * \param pp_styles array of size \p i_len containing character styles [IN]
 * \param pi_k_dates array of size \p i_len containing karaoke timestamps for characters [IN]
 * \param i_len length of the arrays \p psz_text, \p pp_styles, and \p pi_k_dates [IN]
 * \param b_grid true for grid-mode text [IN]
 * \param b_balance true for balanced wrapped lines [IN]
 * \param i_max_width maximum available width to layout text [IN]
 * \param i_max_height maximum available height to layout text [IN]
 * \param pp_lines the list of line_desc_t's with rendered glyphs [OUT]
 * \param p_bbox the bounding box of all the lines [OUT]
 * \param pi_max_face_height maximum line height [OUT]
 */
int LayoutText( filter_t *p_filter,
                const uni_char_t *psz_text, text_style_t **pp_styles,
                uint32_t *pi_k_dates, int i_len, bool b_grid, bool b_balance,
                unsigned i_max_width, unsigned i_max_height,
                line_desc_t **pp_lines, FT_BBox *p_bbox, int *pi_max_face_height );
