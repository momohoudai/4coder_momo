
enum Momo_Index_Note_Kind
{
    MOMO_INDEX_NOTE_KIND_NULL,
    MOMO_INDEX_NOTE_KIND_SCOPE,
    MOMO_INDEX_NOTE_KIND_TYPE,
    MOMO_INDEX_NOTE_KIND_CONSTANT,
    MOMO_INDEX_NOTE_KIND_FUNCTION,
    MOMO_INDEX_NOTE_KIND_DECL,
    MOMO_INDEX_NOTE_KIND_MACRO,
    MOMO_INDEX_NOTE_KIND_COMMENT_TAG,
    MOMO_INDEX_NOTE_KIND_COMMENT_TODO,

    MOMO_INDEX_NOTE_KIND_COUNT
};

typedef u32 Momo_Index_Note_Flags;
enum
{
    MOMO_INDEX_NOTE_FLAG_PROTOTYPE   = (1<<0),
    MOMO_INDEX_NOTE_FLAG_PRODUCT_TYPE = (1<<1),
    MOMO_INDEX_NOTE_FLAG_SUM_TYPE     = (1<<2),
};

struct Momo_Index_File;

struct Momo_Index_Note
{
    Momo_Index_Note *next;
    Momo_Index_Note *prev;
    Momo_Index_Note *hash_next;
	Momo_Index_Note *hash_prev;
    Momo_Index_Note *parent;
    Momo_Index_Note *next_sibling;
    Momo_Index_Note *prev_sibling;
    Momo_Index_Note *first_child;
    Momo_Index_Note *last_child;
    
    u64 hash;
    String_Const_u8 additional_info;
    String_Const_u8 string;
    Momo_Index_Note_Kind kind;
    Momo_Index_Note_Flags flags;
    Range_i64 range;
    Momo_Index_File *file;
	int file_generation;
};

struct Momo_Index_File
{
    Momo_Index_File *hash_next;
    Arena arena;
    Buffer_ID buffer;
    Momo_Index_Note *first_note;
    Momo_Index_Note *last_note;
	int generation;
};

struct Momo_Index_State
{
    System_Mutex mutex;
    Arena arena;
    Momo_Index_Note *note_table[16384];
	Momo_Index_Note* free_note;
    Momo_Index_File *file_table[16384];
    Momo_Index_File *free_file;
};

struct Momo_Index_ParseCtx
{
    b32 done;
    Application_Links *app;
    Momo_Index_File *file;
    String_Const_u8 string;
    Token_Array tokens;
    Token_Iterator_Array it;
};

typedef u32 Momo_Index_Token_Skip_Flags;
enum
{
    MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE = (1<<0),
};

internal void Momo_Index_Initialize(void);
internal void Momo_Index_Lock(void);
internal void Momo_Index_Unlock(void);
internal Momo_Index_File *Momo_Index_LookupFile(Application_Links *app, Buffer_ID buffer);
internal Momo_Index_File *Momo_Index_LookupOrMakeFile(Application_Links *app, Buffer_ID buffer);
internal void Momo_Index_EraseFile(Application_Links *app, Buffer_ID id);
internal void Momo_Index_ClearFile(Momo_Index_File *file);
internal Momo_Index_Note *Momo_Index_LookupNote(String_Const_u8 string, Momo_Index_Note *parent);
internal Momo_Index_Note *Momo_Index_LookupNote(String_Const_u8 string);
internal Momo_Index_Note *Momo_Index_MakeNote(Application_Links *app,
                                          Momo_Index_File *file,
                                          Momo_Index_Note *parent,
                                          String_Const_u8 string,
                                          Momo_Index_Note_Kind note_kind,
                                          Momo_Index_Note_Flags note_flags,
                                          Range_i64 range,
                                          String_Const_u8 additional_info = {});
internal void Momo_Index_ParseFile(Application_Links *app, Momo_Index_File *file, String_Const_u8 string, Token_Array tokens);
internal b32 Momo_Index_ParseCtx_Inc(Momo_Index_ParseCtx *ctx, Momo_Index_Token_Skip_Flags flags);
#define Momo_Index_ParseCtx_IncWs(ctx) Momo_Index_ParseCtx_Inc(ctx, MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE)
internal b32 Momo_Index_RequireToken(Momo_Index_ParseCtx *ctx, String_Const_u8 string, Momo_Index_Token_Skip_Flags flags);
internal b32 Momo_Index_RequireTokenKind(Momo_Index_ParseCtx *ctx, Token_Base_Kind kind, Token **token_out, Momo_Index_Token_Skip_Flags flags);
internal b32 Momo_Index_RequireTokenSubKind(Momo_Index_ParseCtx *ctx, int sub_kind, Token **token_out, Momo_Index_Token_Skip_Flags flags);
internal b32 Momo_Index_PeekToken(Momo_Index_ParseCtx *ctx, String_Const_u8 string);
internal b32 Momo_Index_PeekTokenKind(Momo_Index_ParseCtx *ctx, Token_Base_Kind kind, Token **token_out);
internal b32 Momo_Index_PeekTokenSubKind(Momo_Index_ParseCtx *ctx, int sub_kind, Token **token_out);
internal b32 Momo_Index_SeekToken(Momo_Index_ParseCtx *ctx, String_Const_u8 string);
internal void Momo_Index_ParseComment(Momo_Index_ParseCtx *ctx, Token *token);
internal void Momo_Index_SkipSoftTokens(Momo_Index_ParseCtx *ctx, b32 preproc);
internal void Momo_Index_SkipOpTokens(Momo_Index_ParseCtx *ctx);
internal String_Const_u8 Momo_Index_StringFromToken(Momo_Index_ParseCtx* ctx, Token* token);
internal void Momo_Index_Tick(Application_Links *app);

// Format:
// %t -> token,         requires char * specifying token string
// %k -> token kind,    requires Token_Base_Kind and Token ** for output token
// %b -> token subkind, requires token subkind and Token ** for output token
// %n -> note,          requires Momo_Index_Note_Kind and Momo_Index_Note ** for output note
// %s -> soft group,    requires no arguments
// %o -> operator group,requires no arguments
function b32 Momo_Index_ParsePattern(Momo_Index_ParseCtx *ctx, char *fmt, ...);