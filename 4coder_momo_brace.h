#ifndef __4CODER_MOMO_BRACES_H__
#define __4CODER_MOMO_BRACES_H__

static void momo_render_brace_highlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 pos, ARGB_Color *colors, i32 color_count);
static void momo_render_close_brace_annotation(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 pos);
static void momo_render_brace_lines(Application_Links *app, Buffer_ID buffer, View_ID view, Text_Layout_ID text_layout_id, i64 pos);

#endif