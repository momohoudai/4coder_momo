#ifndef __4CODER_MOMO_BRACES_H__
#define __4CODER_MOMO_BRACES_H__

function void Momo_Brace_RenderHightlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 pos, ARGB_Color *colors, i32 color_count);
function void Momo_Brace_RenderCloseBraceAnnotation(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 pos);
function void Momo_Brace_RenderLines(Application_Links *app, Buffer_ID buffer, View_ID view, Text_Layout_ID text_layout_id, i64 pos);

#endif