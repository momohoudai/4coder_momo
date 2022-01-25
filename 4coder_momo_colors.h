CUSTOM_ID(colors, fleury_color_syntax_crap);
CUSTOM_ID(colors, fleury_color_operators);
CUSTOM_ID(colors, fleury_color_inactive_pane_overlay);
CUSTOM_ID(colors, fleury_color_inactive_pane_background);
CUSTOM_ID(colors, fleury_color_file_progress_bar);
CUSTOM_ID(colors, momo_color_brace_highlight);
CUSTOM_ID(colors, momo_color_brace_line);
CUSTOM_ID(colors, momo_color_brace_annotation);
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

enum Momo_KeybindingMode;

static ARGB_Color momo_argb_from_id(Color_Table table, Managed_ID id, int subindex);
static ARGB_Color momo_argb_from_id(Color_Table table, Managed_ID id);
static b32 momo_is_argb_valid(ARGB_Color color);
static ARGB_Color momo_get_cursor_color(Application_Links *app, Momo_KeybindingMode mode, b32 is_recording_mode);
static void momo_highlight_syntax(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array);