CUSTOM_ID(colors, fleury_color_syntax_crap);
CUSTOM_ID(colors, fleury_color_operators);
CUSTOM_ID(colors, fleury_color_inactive_pane_overlay);
CUSTOM_ID(colors, fleury_color_inactive_pane_background);
CUSTOM_ID(colors, fleury_color_file_progress_bar);
CUSTOM_ID(colors, fleury_color_brace_highlight);
CUSTOM_ID(colors, fleury_color_brace_line);
CUSTOM_ID(colors, fleury_color_brace_annotation);
CUSTOM_ID(colors, fleury_color_index_sum_type);
CUSTOM_ID(colors, fleury_color_index_product_type);
CUSTOM_ID(colors, fleury_color_index_function);
CUSTOM_ID(colors, fleury_color_index_macro);
CUSTOM_ID(colors, fleury_color_index_constant);
CUSTOM_ID(colors, fleury_color_index_comment_tag);
CUSTOM_ID(colors, fleury_color_index_decl);
CUSTOM_ID(colors, fleury_color_cursor_macro);
CUSTOM_ID(colors, fleury_color_cursor_power_mode);
CUSTOM_ID(colors, fleury_color_cursor_inactive);
CUSTOM_ID(colors, fleury_color_plot_cycle);
CUSTOM_ID(colors, fleury_color_token_highlight);
CUSTOM_ID(colors, fleury_color_token_minor_highlight);
CUSTOM_ID(colors, fleury_color_comment_user_name);
CUSTOM_ID(colors, fleury_color_lego_grab);
CUSTOM_ID(colors, fleury_color_lego_splat);
CUSTOM_ID(colors, fleury_color_error_annotation);

static ARGB_Color argb_from_id(Color_Table table, Managed_ID id, int subindex);
static ARGB_Color argb_from_id(Color_Table table, Managed_ID id);
typedef u32 Syntax_Flags;
enum
{
    SYNTAX_FLAGS_FUNCTIONS    = (1<<0),
    SYNTAX_FLAGS_MACROS       = (1<<1),
    SYNTAX_FLAGS_TYPES        = (1<<2),
    SYNTAX_FLAGS_OPERATORS    = (1<<3),
    SYNTAX_FLAGS_CONSTANTS    = (1<<4),
    SYNTAX_FLAGS_LITERALS     = (1<<5),
    SYNTAX_FLAGS_PREPROCESSOR = (1<<6),
    SYNTAX_FLAGS_KEYWORDS     = (1<<7),
    SYNTAX_FLAGS_HIGHLIGHT_ALL = (1<<15),
};
#define Momo_SyntaxFlag_All 0xffffffff

struct Syntax_Options
{
    String8 name;
    Syntax_Flags flags;
};

global f32 momo_syntax_flag_transitions[32] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};
global Syntax_Options momo_syntax_opts[] =
{
    { S8Lit("All"),            Momo_SyntaxFlag_All },
    { S8Lit("None"),           0 },
    { S8Lit("Functions Only"), SYNTAX_FLAGS_FUNCTIONS },
    { S8Lit("Macros Only"),    SYNTAX_FLAGS_MACROS },
    { S8Lit("Function-Likes Only"), SYNTAX_FLAGS_FUNCTIONS | SYNTAX_FLAGS_MACROS },
    { S8Lit("Types Only"),     SYNTAX_FLAGS_TYPES },
    { S8Lit("Externals Only"), SYNTAX_FLAGS_FUNCTIONS | SYNTAX_FLAGS_MACROS | SYNTAX_FLAGS_TYPES | SYNTAX_FLAGS_CONSTANTS },
};
global i32 momo_active_syntax_opt_idx = 0;