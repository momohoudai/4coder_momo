struct Momo_Index_ParseCtx;

//~ NOTE(rjf): Indexes an entire file of a language and adds stuff to the code index.
#define MOMO_LANGUAGE_INDEXFILE(name) void name(Momo_Index_ParseCtx *ctx)
typedef MOMO_LANGUAGE_INDEXFILE(Momo_Language_IndexFile);

//~ NOTE(rjf): Initializes lexer state.
#define MOMO_LANGUAGE_LEXINIT(name) void name(void *state_ptr, String_Const_u8 contents)
typedef MOMO_LANGUAGE_LEXINIT(Momo_Language_LexInit);

//~ NOTE(rjf): Lexes an entire file to produce tokens for the language (with breaks).
#define MOMO_LANGUAGE_LEXFULLINPUT(name) b32 name(Arena *arena, Token_List *list, void *state_ptr, u64 max)
typedef MOMO_LANGUAGE_LEXFULLINPUT(Momo_Language_LexFullInput);

//~ NOTE(rjf): Figures out some language-specific contextual information given some
// position in a buffer. For example, what type I am accessing, what function I am
// calling which parameter am I accessing, etc.

struct Momo_Index_Note;
struct Momo_Language_PosContextData
{
    Momo_Language_PosContextData *next;
    Momo_Index_Note *relevant_note;
    Token *query_token;
    int argument_index;
};

#define MOMO_LANGUAGE_POSCONTEXT(name) Momo_Language_PosContextData * name(Application_Links *app, Arena *arena, Buffer_ID buffer, i64 pos)
typedef MOMO_LANGUAGE_POSCONTEXT(Momo_Language_PosContext);

//~ NOTE(rjf): Does language-specific coloring for a buffer, for special-case syntax
// highlighting and stuff. Most syntax highlighting is handled through token-base-kinds
// and index lookups, but sometimes it's useful to be able to override colors by
// language-specific token sub-kinds and flags, so that's what this hook is for.
#define MOMO_LANGUAGE_HIGHLIGHT(name) void name(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array, Color_Table color_table)
typedef MOMO_LANGUAGE_HIGHLIGHT(Momo_Language_Highlight);

//~

struct Momo_Language
{
    Momo_Language *next;
    u64 hash;
    String_Const_u8 name;
    u64 lex_state_size;
    Momo_Language_IndexFile            *IndexFile;
    Momo_Language_LexInit              *LexInit;
    Momo_Language_LexFullInput         *LexFullInput;
    Momo_Language_PosContext           *PosContext;
    Momo_Language_Highlight            *Highlight;
};

struct Momo_Language_State
{
    b32 initialized;
    Arena arena;
    Momo_Language *language_table[4096];
};

global Momo_Language_State momo_langs = {};