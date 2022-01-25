// TOP

#ifndef MOMO_AUTO_INDENT_H
#define MOMO_AUTO_INDENT_H

typedef u32 Momo_Indent_Flag;
enum{
    Momo_Indent_Flag_ClearLine = 0x1,
    Momo_Indent_Flag_UseTab    = 0x2,
    Momo_Indent_FlagFullTokens = 0x4,
};

struct Momo_Indent_Nest{
    Momo_Indent_Nest *next;
    Token_Base_Kind kind;
    i64 indent;
};

struct Momo_Indent_NestAlloc{
    Momo_Indent_Nest *free_nest;
};

struct Momo_Indent_LineCache {
    i64 where_token_starts;
    i64 line_number_for_cached_indent;
    i64 start_pos;
    i64 one_past_last_pos;
    Indent_Info indent_info;
};


static void momo_indent_lines_right(Application_Links *app, Buffer_ID buffer, Range_i64 view_range, Indent_Flag flags);
static void momo_write_text_and_indent(Application_Links* app, String_Const_u8 insert);
static b32 momo_indent_buffer(Application_Links *app, Buffer_ID buffer, Range_i64 pos, Indent_Flag flags, i32 tab_width, i32 indent_width);
static b32 momo_indent_buffer(Application_Links *app, Buffer_ID buffer, Range_i64 pos, Indent_Flag flags);
static b32 momo_indent_buffer(Application_Links *app, Buffer_ID buffer, Range_i64 pos);
#endif

// BOTTOM

