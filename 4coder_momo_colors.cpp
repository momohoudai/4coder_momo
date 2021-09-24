

function b32
Momo_Colors_IsArgbValid(ARGB_Color color)
{
    return color != 0xFF990099;
}

internal void
Momo_Colors_Tick(Application_Links *app, Frame_Info frame_info)
{
    Momo_Colors_SyntaxOptions opts = momo_syntax_opts[momo_active_syntax_opt_idx];
    for(int i = 0; i < sizeof(Momo_Colors_SyntaxFlagType)*8; i += 1)
    {
        f32 delta = ((f32)!!(opts.flags & (1<<i)) - momo_syntax_flag_transitions[i]) * frame_info.animation_dt * 8.f;
        momo_syntax_flag_transitions[i] += delta;
        if(fabsf(delta) > 0.001f)
        {
            animate_in_n_milliseconds(app, 0);
        }
    }
}

CUSTOM_COMMAND_SIG(momo_switch_syntax_option)
CUSTOM_DOC("Switches the syntax highlighting mode.")
{
    momo_active_syntax_opt_idx = (momo_active_syntax_opt_idx + 1) % ArrayCount(momo_syntax_opts);
}

internal String8
momo_syntax_option_string(void)
{
    return momo_syntax_opts[momo_active_syntax_opt_idx].name;
}

typedef u32 Momo_Colors_FlagType;
enum 
{
    Momo_Colors_FlagType_Macro = (1<<0),
};

struct Momo_Colors_ColorCtx
{
    Token token;
    Buffer_ID buffer;
    Momo_Colors_FlagType flags;
    keybinding_mode mode;
};

internal Momo_Colors_ColorCtx
Momo_Colors_GetColorCtxFromToken(Token token, Buffer_ID buffer)
{
    Momo_Colors_ColorCtx ctx = {0};
    ctx.token = token;
    ctx.buffer = buffer;
    return ctx;
}

internal Momo_Colors_ColorCtx
Momo_Colors_GetColorCtxFromCursor(Momo_Colors_FlagType flags, keybinding_mode mode)
{
    Momo_Colors_ColorCtx ctx = {0};
    ctx.flags = flags;
    ctx.mode = mode;
    return ctx;
}

static ARGB_Color
Momo_Colors_ARGBFromID(Color_Table table, Managed_ID id, int subindex)
{
    ARGB_Color result = 0;
    FColor color = fcolor_id(id);
    if (color.a_byte == 0){
        if (color.id != 0){
            result = finalize_color(table, color.id, subindex);
        }
    }
    else{
        result = color.argb;
    }
    return(result);
}

static ARGB_Color
Momo_Colors_ARGBFromID(Color_Table table, Managed_ID id)
{
    return Momo_Colors_ARGBFromID(table, id, 0);
}

internal ARGB_Color
Momo_Colors_GetColor(Application_Links *app, Momo_Colors_ColorCtx ctx)
{
    Color_Table table = active_color_table;
    ARGB_Color default_color = Momo_Colors_ARGBFromID(table, defcolor_text_default);
    ARGB_Color color = default_color;
    f32 t = 1;
    
#define FillFromFlag(flag) do{ t = momo_syntax_flag_transitions[BitOffset(flag)]; }while(0)
    
    //~ NOTE(rjf): Token Color
    if(ctx.token.size != 0)
    {
        Scratch_Block scratch(app);
        
        switch(ctx.token.kind)
        {
            case TokenBaseKind_Identifier:
            {
                String_Const_u8 string = push_buffer_range(app, scratch, ctx.buffer, Ii64(ctx.token.pos, ctx.token.pos + ctx.token.size));
                Momo_Index_Note *note = Momo_Index_LookupNote(string);
                if(note)
                {
                    color = 0xffff0000;
                    switch(note->kind)
                    {
                        case MOMO_INDEX_NOTE_KIND_TYPE:
                        {
                            FillFromFlag(SYNTAX_FLAGS_TYPES);
                            color = Momo_Colors_ARGBFromID(table,
                                                  note->flags & MOMO_INDEX_NOTE_FLAG_SUM_TYPE
                                                  ? fleury_color_index_sum_type
                                                  : fleury_color_index_product_type);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_MACRO:
                        {
                            FillFromFlag(SYNTAX_FLAGS_MACROS);
                            color = Momo_Colors_ARGBFromID(table, fleury_color_index_macro);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_FUNCTION:
                        {
                            FillFromFlag(SYNTAX_FLAGS_FUNCTIONS);
                            color = Momo_Colors_ARGBFromID(table, fleury_color_index_function);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_CONSTANT:
                        {
                            FillFromFlag(SYNTAX_FLAGS_CONSTANTS);
                            color = Momo_Colors_ARGBFromID(table, fleury_color_index_constant);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_DECL:
                        {
                            FillFromFlag(SYNTAX_FLAGS_CONSTANTS);
                            color = Momo_Colors_ARGBFromID(table, fleury_color_index_decl);
                        }break;
                        
                        default: color = 0xffff00ff; break;
                    }
                    
                    if(!Momo_Colors_IsArgbValid(color)) { color = default_color; }
                }
                
            }break;
            
            case TokenBaseKind_Preprocessor:     { FillFromFlag(SYNTAX_FLAGS_PREPROCESSOR); color = Momo_Colors_ARGBFromID(table, defcolor_preproc); } break;
            case TokenBaseKind_Keyword:          { FillFromFlag(SYNTAX_FLAGS_KEYWORDS); color = Momo_Colors_ARGBFromID(table, defcolor_keyword); } break;
            case TokenBaseKind_Comment:          { color = Momo_Colors_ARGBFromID(table, defcolor_comment); } break;
            case TokenBaseKind_LiteralString:    { FillFromFlag(SYNTAX_FLAGS_LITERALS); color = Momo_Colors_ARGBFromID(table, defcolor_str_constant); } break;
            case TokenBaseKind_LiteralInteger:   { FillFromFlag(SYNTAX_FLAGS_LITERALS); color = Momo_Colors_ARGBFromID(table, defcolor_int_constant); } break;
            case TokenBaseKind_LiteralFloat:     { FillFromFlag(SYNTAX_FLAGS_LITERALS); color = Momo_Colors_ARGBFromID(table, defcolor_float_constant); } break;
            case TokenBaseKind_Operator:         { FillFromFlag(SYNTAX_FLAGS_OPERATORS); color = Momo_Colors_ARGBFromID(table, fleury_color_operators); if(!Momo_Colors_IsArgbValid(color)) { color = default_color; } } break;
            
            case TokenBaseKind_ScopeOpen:
            case TokenBaseKind_ScopeClose:
            case TokenBaseKind_ParentheticalOpen:
            case TokenBaseKind_ParentheticalClose:
            case TokenBaseKind_StatementClose:
            {
                color = Momo_Colors_ARGBFromID(table, fleury_color_syntax_crap);
                if(!Momo_Colors_IsArgbValid(color)) { color = default_color; }
                break;
            }
            
            default:
            {
                switch(ctx.token.sub_kind)
                {
                    case TokenCppKind_LiteralTrue:
                    case TokenCppKind_LiteralFalse:
                    {
                        color = Momo_Colors_ARGBFromID(table, defcolor_bool_constant);
                        FillFromFlag(SYNTAX_FLAGS_LITERALS); 
                        break;
                    }
                    case TokenCppKind_LiteralCharacter:
                    case TokenCppKind_LiteralCharacterWide:
                    case TokenCppKind_LiteralCharacterUTF8:
                    case TokenCppKind_LiteralCharacterUTF16:
                    case TokenCppKind_LiteralCharacterUTF32:
                    {
                        color = Momo_Colors_ARGBFromID(table, defcolor_char_constant);
                        FillFromFlag(SYNTAX_FLAGS_LITERALS); 
                        break;
                    }
                    case TokenCppKind_PPIncludeFile:
                    {
                        color = Momo_Colors_ARGBFromID(table, defcolor_include);
                        FillFromFlag(SYNTAX_FLAGS_LITERALS); 
                        break;
                    }
                }
            }break;
            
        }
    }
    
    //~ NOTE(rjf): Cursor Color
    else
    {
        if(ctx.flags & Momo_Colors_FlagType_Macro)
        {
            color = Momo_Colors_ARGBFromID(table, fleury_color_cursor_macro);
        }
        else
        {
            color = Momo_Colors_ARGBFromID(table, defcolor_cursor, ctx.mode);
        }
    }
    
    return color_blend(default_color, t, color);
}

static void
Momo_Colors_SyntaxHighlight(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array)
{
    Color_Table table = active_color_table;
    Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    ARGB_Color comment_tag_color = Momo_Colors_ARGBFromID(table, fleury_color_index_comment_tag, 0);
    
    for(;;)
    {
        Token *token = token_it_read(&it);
        if(!token || token->pos >= visible_range.one_past_last)
        {
            break;
        }
        ARGB_Color argb = Momo_Colors_GetColor(app, Momo_Colors_GetColorCtxFromToken(*token, buffer));
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        
        // NOTE(rjf): Substrings from comments
        if(Momo_Colors_IsArgbValid(comment_tag_color))
        {
            if(token->kind == TokenBaseKind_Comment)
            {
                Scratch_Block scratch(app);
                String_Const_u8 string = push_buffer_range(app, scratch, buffer, Ii64(token->pos, token->pos + token->size));
                for(u64 i = 0; i < string.size; i += 1)
                {
                    if(string.str[i] == '@')
                    {
                        u64 j = i+1;
                        for(; j < string.size; j += 1)
                        {
                            if(character_is_whitespace(string.str[j]))
                            {
                                break;
                            }
                        }
                        paint_text_color(app, text_layout_id, Ii64(token->pos + (i64)i, token->pos + (i64)j), comment_tag_color);
                    }
                }
            }
        }
        
        if(!token_it_inc_all(&it))
        {
            break;
        }
    }
    
    Momo_Language *lang = Momo_Language_GetFromBuffer(app, buffer);
    if(lang != 0 && lang->Highlight != 0)
    {
        lang->Highlight(app, text_layout_id, array, table);
    }
    
}
