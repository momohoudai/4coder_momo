

static b32
momo_is_argb_valid(ARGB_Color color)
{
    return color != 0xFF990099;
}

static ARGB_Color
momo_argb_from_id(Color_Table table, Managed_ID id, int subindex)
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
momo_argb_from_id(Color_Table table, Managed_ID id)
{
    return momo_argb_from_id(table, id, 0);
}

internal ARGB_Color
_momo_get_token_color(Application_Links *app, Token* token, Buffer_ID buffer, Token* next_token) {
    Color_Table table = active_color_table;
    ARGB_Color default_color = momo_argb_from_id(table, defcolor_text_default);
    ARGB_Color color = default_color;
    f32 t = 1;

    if (token && token->size != 0) {
        Scratch_Block scratch(app);
        
        String_Const_u8 string = push_buffer_range(app, scratch, buffer, Ii64(token->pos, token->pos + token->size));
                
        switch(token->kind)
        {
            case TokenBaseKind_Identifier:
            {
                Momo_Index_Note *note = Momo_Index_LookupNote(string);
                if(note)
                {
                    color = default_color;
                    switch(note->kind)
                    {
                        case MOMO_INDEX_NOTE_KIND_TYPE:
                        {
                            color = momo_argb_from_id(table,
                                                  note->flags & MOMO_INDEX_NOTE_FLAG_SUM_TYPE
                                                  ? fleury_color_index_sum_type
                                                  : fleury_color_index_product_type);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_MACRO:
                        {
                            color = momo_argb_from_id(table, fleury_color_index_macro);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_FUNCTION:
                        {
                            // We only attempt to highlight a function when it 'looks' like a function
                            // (i.e. opening braces after the token)
                            color = momo_argb_from_id(table, fleury_color_index_function);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_CONSTANT:
                        {
                            color = momo_argb_from_id(table, fleury_color_index_constant);
                        }break;
                        
                        case MOMO_INDEX_NOTE_KIND_DECL:
                        {
                            color = momo_argb_from_id(table, fleury_color_index_decl);
                        }break;
                        
                        default: {
                            color = default_color; break;
                        }
                    }
                    
                    if(!momo_is_argb_valid(color)) { color = default_color; }
                }
                
            }break;
            
            case TokenBaseKind_Preprocessor:     { color = momo_argb_from_id(table, defcolor_preproc); } break;
            case TokenBaseKind_Keyword:          { color = momo_argb_from_id(table, defcolor_keyword); } break;
            case TokenBaseKind_Comment:          { color = momo_argb_from_id(table, defcolor_comment); } break;
            case TokenBaseKind_LiteralString:    { color = momo_argb_from_id(table, defcolor_str_constant); } break;
            case TokenBaseKind_LiteralInteger:   { color = momo_argb_from_id(table, defcolor_int_constant); } break;
            case TokenBaseKind_LiteralFloat:     { color = momo_argb_from_id(table, defcolor_float_constant); } break;
            case TokenBaseKind_Operator:         { color = momo_argb_from_id(table, fleury_color_operators); if(!momo_is_argb_valid(color)) { color = default_color; } } break;
            
            case TokenBaseKind_ScopeOpen:
            case TokenBaseKind_ScopeClose:
            case TokenBaseKind_ParentheticalOpen:
            case TokenBaseKind_ParentheticalClose:
            case TokenBaseKind_StatementClose:
            {
                color = momo_argb_from_id(table, fleury_color_syntax_crap);
                if(!momo_is_argb_valid(color)) { color = default_color; }
                break;
            }
            
            default:
            {
                switch(token->sub_kind)
                {
                    case TokenCppKind_LiteralTrue:
                    case TokenCppKind_LiteralFalse:
                    {
                        color = momo_argb_from_id(table, defcolor_bool_constant);
                        break;
                    }
                    case TokenCppKind_LiteralCharacter:
                    case TokenCppKind_LiteralCharacterWide:
                    case TokenCppKind_LiteralCharacterUTF8:
                    case TokenCppKind_LiteralCharacterUTF16:
                    case TokenCppKind_LiteralCharacterUTF32:
                    {
                        color = momo_argb_from_id(table, defcolor_char_constant);
                        break;
                    }
                    case TokenCppKind_PPIncludeFile:
                    {
                        color = momo_argb_from_id(table, defcolor_include);
                        break;
                    }
                }
            }break;
            
        }
    }

    return color_blend(default_color, t, color);
}


internal ARGB_Color
momo_get_cursor_color(Application_Links *app, Momo_KeybindingMode mode, b32 is_recording_mode) {
    // TODO: more control over possible colors?
    Color_Table table = active_color_table;
    ARGB_Color cursor_color = momo_argb_from_id(table, defcolor_cursor, mode);
    ARGB_Color color_to_blend = cursor_color;
    
    if (is_recording_mode) {
        color_to_blend = momo_argb_from_id(table, fleury_color_cursor_macro);
    }
    
    return color_blend(cursor_color, 0.5f, color_to_blend);
}



static void
momo_highlight_syntax(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array)
{
    Color_Table table = active_color_table;
    Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    ARGB_Color comment_tag_color = momo_argb_from_id(table, fleury_color_index_comment_tag, 0);
    
    for(;;)
    {
        Token *token = token_it_read(&it);
        if(!token || token->pos >= visible_range.one_past_last)
        {
            break;
        }
        Token_Iterator_Array next_it = it;
        token_it_inc_all(&next_it);



        Token* next_token = token_it_read(&next_it);
        ARGB_Color argb = _momo_get_token_color(app, token, buffer, next_token);
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        
        // NOTE(rjf): Substrings from comments
        if(momo_is_argb_valid(comment_tag_color))
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
    
    Momo_Language *lang = momo_get_language_from_buffer(app, buffer);
    if(lang != 0 && lang->Highlight != 0)
    {
        lang->Highlight(app, text_layout_id, array, table);
    }
    
}
