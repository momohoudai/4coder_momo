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

#endif

// BOTTOM

