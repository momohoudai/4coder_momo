/*
4coder_auto_indent.cpp - Commands for automatic indentation.
*/

// TOP

internal Batch_Edit*
_momo_make_batch_from_indentations(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, i64 *indentations, Indent_Flag flags, i32 tab_width){
    i64 *shifted_indentations = indentations - lines.first;
    
    Batch_Edit *batch_first = 0;
    Batch_Edit *batch_last = 0;
    
    for (i64 line_number = lines.first;
         line_number <= lines.max;
         ++line_number){
        i64 line_start_pos = get_line_start_pos(app, buffer, line_number);
        Indent_Info indent_info = get_indent_info_line_number_and_start(app, buffer, line_number, line_start_pos, tab_width);
        
        i64 correct_indentation = shifted_indentations[line_number];
        if (indent_info.is_blank && HasFlag(flags, Indent_ClearLine)){
            correct_indentation = 0;
        }
        if (correct_indentation <= -1){
            correct_indentation = indent_info.indent_pos;
        }
        
        if (correct_indentation != indent_info.indent_pos){
            u64 str_size = 0;
            u8 *str = 0;
            if (HasFlag(flags, Indent_UseTab)){
                i64 tab_count = correct_indentation/tab_width;
                i64 indent = tab_count*tab_width;
                i64 space_count = correct_indentation - indent;
                str_size = tab_count + space_count;
                str = push_array(arena, u8, str_size);
                block_fill_u8(str, tab_count, '\t');
                block_fill_u8(str + tab_count, space_count, ' ');
            }
            else{
                str_size = correct_indentation;
                str = push_array(arena, u8, str_size);
                block_fill_u8(str, str_size, ' ');
            }
            
            Batch_Edit *batch = push_array(arena, Batch_Edit, 1);
            sll_queue_push(batch_first, batch_last, batch);
            batch->edit.text = SCu8(str, str_size);
            batch->edit.range = Ii64(line_start_pos, indent_info.first_char_pos);
        }
    }
    
    return(batch_first);
}

internal void
_momo_set_line_indents(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, i64 *indentations, Indent_Flag flags, i32 tab_width){
    Batch_Edit *batch = _momo_make_batch_from_indentations(app, arena, buffer, lines, indentations, flags, tab_width);
    if (batch != 0){
        buffer_batch_edit(app, buffer, batch);
    }
}


internal Momo_Indent_Nest*
_momo_new_indent_nest(Arena *arena, Momo_Indent_NestAlloc *alloc){
    Momo_Indent_Nest *new_nest = alloc->free_nest;
    if (new_nest == 0){
        new_nest = push_array(arena, Momo_Indent_Nest, 1);
    }
    else{
        sll_stack_pop(alloc->free_nest);
    }
    return(new_nest);
}

internal void
_momo_free_indent_nest(Momo_Indent_NestAlloc *alloc, Momo_Indent_Nest *nest){
    sll_stack_push(alloc->free_nest, nest);
}

internal b32
_momo_is_unfinished_statement(Token *token, Momo_Indent_Nest *current_nest){
    b32 result = false;
    if (current_nest != 0 && current_nest->kind == TokenBaseKind_ScopeOpen){
        result = true;
        switch (token->kind){
            case TokenBaseKind_ScopeOpen:
            case TokenBaseKind_ScopeClose:
            case TokenBaseKind_StatementClose:
            {
                result = false;
            }break;
        }
        if (HasFlag(token->flags, TokenBaseFlag_PreprocessorBody)){
            result = false;
        }
    }
    return(result);
}

internal void
_momo_update_line_cache(Application_Links *app, Buffer_ID buffer, i32 tab_width, Momo_Indent_LineCache *line_cache){
    if (line_cache->line_number_for_cached_indent != line_cache->where_token_starts){
        ProfileScope(app, "get indent info");
        line_cache->line_number_for_cached_indent = line_cache->where_token_starts;
        line_cache->start_pos = get_line_start_pos(app, buffer, line_cache->where_token_starts);
        Range_i64 range = Ii64(line_cache->start_pos, line_cache->one_past_last_pos);
        line_cache->indent_info = get_indent_info_range(app, buffer, range, tab_width);
    }
}



internal i64* 
_momo_get_indentations_to_shift_lines_right(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, Indent_Flag flags, i32 tab_width, i32 indent_width) {
    i64 count = lines.max - lines.min + 1;
    i64 *indentations = push_array(arena, i64, count);

    // Figure out how many indentations each line has and increase it by 4
    for (i64 line_number = lines.first; line_number <= lines.max; ++line_number)
    {    
        i64 line_start_pos = get_line_start_pos(app, buffer, count);
        Indent_Info indent_info = get_indent_info_line_number_and_start(app, buffer, line_number, line_start_pos, tab_width);
        indentations[line_number - lines.first] = indent_info.indent_pos + 1;
    }
    return indentations;
}



internal i64* 
_momo_get_indentations_to_shift_lines_left(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, Indent_Flag flags, i32 tab_width, i32 indent_width) {
    // TODO
    
}

static void
momo_indent_lines_right(Application_Links *app, Buffer_ID buffer, Range_i64 view_range, Indent_Flag flags) {
    Scratch_Block scratch(app);
    i32 indent_width = (i32)def_get_config_u64(app, vars_save_string_lit("indent_width"));
    i32 tab_width = (i32)def_get_config_u64(app, vars_save_string_lit("default_tab_width"));
    tab_width = clamp_bot(1, tab_width);
    AddFlag(flags, Indent_FullTokens);
    b32 indent_with_tabs = def_get_config_b32(vars_save_string_lit("indent_with_tabs"));
    if (indent_with_tabs){
        AddFlag(flags, Indent_UseTab);
    }

    Range_i64 line_numbers = get_line_range_from_pos_range(app, buffer, view_range);
    i64* indentations = _momo_get_indentations_to_shift_lines_right(app, scratch, buffer, line_numbers, flags, tab_width, indent_width);
    _momo_set_line_indents(app, scratch, buffer, line_numbers, indentations, flags, tab_width);
}

internal i64*
_momo_get_indentations_for_golang(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, Indent_Flag flags, i32 tab_width, i32 indent_width){
    ProfileScope(app, "get indentation array golang");
    i64 count = lines.max - lines.min + 1;
    i64 *indentations = push_array(arena, i64, count);
    i64 *shifted_indentations = indentations - lines.first;
    block_fill_u64(indentations, sizeof(*indentations)*count, (u64)(-1));
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Token_Array *tokens = &token_array;
    
    i64 anchor_line = clamp_bot(1, lines.first - 1);
    Token *anchor_token = find_anchor_token(app, buffer, tokens, anchor_line);
    if (anchor_token != 0 &&
        anchor_token >= tokens->tokens &&
        anchor_token < tokens->tokens + tokens->count){
        i64 line = get_line_number_from_pos(app, buffer, anchor_token->pos);
        line = clamp_top(line, lines.first);
        
        Token_Iterator_Array token_it = token_iterator(0, tokens, anchor_token);
        
        Scratch_Block scratch(app, arena);
        Momo_Indent_Nest *nest = 0;
        Momo_Indent_NestAlloc nest_alloc = {};
        
        i64 line_last_indented = line - 1;
        i64 last_indent = 0;
        i64 actual_indent = 0;
        b32 in_unfinished_statement = false;
        
        Momo_Indent_LineCache line_cache = {};
        
        for (;;){
            Token *token = token_it_read(&token_it);
            
            String_Const_u8 token_string;
            token_string = push_buffer_range(app, scratch, buffer, Ii64(token));

            if (line_cache.where_token_starts == 0 ||
                token->pos >= line_cache.one_past_last_pos){
                ProfileScope(app, "get line number");
                line_cache.where_token_starts = get_line_number_from_pos(app, buffer, token->pos);
                line_cache.one_past_last_pos = get_line_end_pos(app, buffer, line_cache.where_token_starts);
            }
            
            i64 current_indent = 0;
            if (nest != 0){
                current_indent = nest->indent;
            }
            i64 this_indent = current_indent;
            i64 following_indent = current_indent;
            
            b32 shift_by_actual_indent = false;
            b32 ignore_unfinished_statement = false;
            if (HasFlag(token->flags, TokenBaseFlag_PreprocessorBody)){
                this_indent = 0;
            }
            else{
                switch (token->kind){
                    case TokenBaseKind_ScopeOpen:
                    {
                        Momo_Indent_Nest *new_nest = _momo_new_indent_nest(arena, &nest_alloc);
                        sll_stack_push(nest, new_nest);
                        nest->kind = TokenBaseKind_ScopeOpen;
                        nest->indent = current_indent + indent_width;
                        following_indent = nest->indent;
                        ignore_unfinished_statement = true;
                    }break;
                    
                    case TokenBaseKind_ScopeClose:
                    {
                        for (;nest != 0 && nest->kind != TokenBaseKind_ScopeOpen;){
                            Momo_Indent_Nest *n = nest;
                            sll_stack_pop(nest);
                            _momo_free_indent_nest(&nest_alloc, n);
                        }
                        if (nest != 0 && nest->kind == TokenBaseKind_ScopeOpen){
                            Momo_Indent_Nest *n = nest;
                            sll_stack_pop(nest);
                            _momo_free_indent_nest(&nest_alloc, n);
                        }
                        this_indent = 0;
                        if (nest != 0){
                            this_indent = nest->indent;
                        }
                        following_indent = this_indent;
                        ignore_unfinished_statement = true;
                    }break;
                    
                    case TokenBaseKind_ParentheticalOpen:
                    {
                        Momo_Indent_Nest *new_nest = _momo_new_indent_nest(arena, &nest_alloc);
                        sll_stack_push(nest, new_nest);
                        nest->kind = TokenBaseKind_ParentheticalOpen;
                        _momo_update_line_cache(app, buffer, tab_width, &line_cache);
                        nest->indent = (token->pos - line_cache.indent_info.first_char_pos) + 1;
                        following_indent = nest->indent;
                        shift_by_actual_indent = true;
                        ignore_unfinished_statement = true;
                    }break;
                    
                    case TokenBaseKind_ParentheticalClose:
                    {
                        if (nest != 0 && nest->kind == TokenBaseKind_ParentheticalOpen){
                            Momo_Indent_Nest *n = nest;
                            sll_stack_pop(nest);
                            _momo_free_indent_nest(&nest_alloc, n);
                        }
                        following_indent = 0;
                        if (nest != 0){
                            following_indent = nest->indent;
                        }
                        //ignore_unfinished_statement = true;
                    }break;
                }
                
                if (token->sub_kind == TokenCppKind_BlockComment ||
                    token->sub_kind == TokenCppKind_LiteralStringRaw){
                    ignore_unfinished_statement = true;
                }
                
                if (in_unfinished_statement && !ignore_unfinished_statement){
                    //this_indent += indent_width;
                }
            }
            
#define EMIT(N) \
Stmnt(if (lines.first <= line_it){shifted_indentations[line_it]=N;} \
if (line_it == lines.end){goto finished;} \
actual_indent = N; )
            
            i64 line_it = line_last_indented;
            if (lines.first <= line_cache.where_token_starts){
                for (;line_it < line_cache.where_token_starts;){
                    line_it += 1;
                    if (line_it == line_cache.where_token_starts){
                        EMIT(this_indent);
                    }
                    else{
                        EMIT(last_indent);
                    }
                }
            }
            else{
                actual_indent = this_indent;
                line_it = line_cache.where_token_starts;
            }
            
            i64 line_where_token_ends = get_line_number_from_pos(app, buffer, token->pos + token->size);
            if (lines.first <= line_where_token_ends){
                _momo_update_line_cache(app, buffer, tab_width, &line_cache);
                i64 line_where_token_starts_shift = this_indent - line_cache.indent_info.indent_pos;
                for (;line_it < line_where_token_ends;){
                    line_it += 1;
                    i64 line_it_start_pos = get_line_start_pos(app, buffer, line_it);
                    Indent_Info line_it_indent_info = get_indent_info_line_number_and_start(app, buffer, line_it, line_it_start_pos, tab_width);
                    i64 new_indent = line_it_indent_info.indent_pos + line_where_token_starts_shift;
                    new_indent = clamp_bot(0, new_indent);
                    EMIT(new_indent);
                }
            }
            else{
                line_it = line_where_token_ends;
            }
#undef EMIT
            
            if (shift_by_actual_indent){
                nest->indent += actual_indent;
                following_indent += actual_indent;
            }
            
            if (token->kind != TokenBaseKind_Comment){
                in_unfinished_statement = _momo_is_unfinished_statement(token, nest);
                if (in_unfinished_statement) {
                    //following_indent += indent_width;
                }
            }
            
            last_indent = following_indent;
            line_last_indented = line_it;
            
            if (!token_it_inc_non_whitespace(&token_it)){
                break;
            }
        }
    }
    
    finished:;
    return(indentations);
}

internal i64*
_momo_get_indentations_for_default(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, Indent_Flag flags, i32 tab_width, i32 indent_width){
    ProfileScope(app, "get indentation array");
    i64 count = lines.max - lines.min + 1;
    i64 *indentations = push_array(arena, i64, count);
    i64 *shifted_indentations = indentations - lines.first;
    block_fill_u64(indentations, sizeof(*indentations)*count, (u64)(-1));
    

    
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Token_Array *tokens = &token_array;
    
    i64 anchor_line = clamp_bot(1, lines.first - 1);
    Token *anchor_token = find_anchor_token(app, buffer, tokens, anchor_line);
    if (anchor_token != 0 &&
        anchor_token >= tokens->tokens &&
        anchor_token < tokens->tokens + tokens->count){
        i64 line = get_line_number_from_pos(app, buffer, anchor_token->pos);
        line = clamp_top(line, lines.first);
        
        Token_Iterator_Array token_it = token_iterator(0, tokens, anchor_token);
        
        Scratch_Block scratch(app, arena);
        Momo_Indent_Nest *nest = 0;
        Momo_Indent_NestAlloc nest_alloc = {};
        
        i64 line_last_indented = line - 1;
        i64 last_indent = 0;
        i64 actual_indent = 0;
        b32 in_unfinished_statement = false;
        
        Momo_Indent_LineCache line_cache = {};
        
        for (;;){
            Token *token = token_it_read(&token_it);
            
            if (line_cache.where_token_starts == 0 ||
                token->pos >= line_cache.one_past_last_pos){
                ProfileScope(app, "get line number");
                line_cache.where_token_starts = get_line_number_from_pos(app, buffer, token->pos);
                line_cache.one_past_last_pos = get_line_end_pos(app, buffer, line_cache.where_token_starts);
            }
            
            i64 current_indent = 0;
            if (nest != 0){
                current_indent = nest->indent;
            }
            i64 this_indent = current_indent;
            i64 following_indent = current_indent;
            
            b32 shift_by_actual_indent = false;
            b32 ignore_unfinished_statement = false;
            if (HasFlag(token->flags, TokenBaseFlag_PreprocessorBody)){
                this_indent = 0;
            }
            else{
                switch (token->kind){
                    case TokenBaseKind_ScopeOpen:
                    {
                        Momo_Indent_Nest *new_nest = _momo_new_indent_nest(arena, &nest_alloc);
                        sll_stack_push(nest, new_nest);
                        nest->kind = TokenBaseKind_ScopeOpen;
                        nest->indent = current_indent + indent_width;
                        following_indent = nest->indent;
                        ignore_unfinished_statement = true;
                    }break;
                    
                    case TokenBaseKind_ScopeClose:
                    {
                        for (;nest != 0 && nest->kind != TokenBaseKind_ScopeOpen;){
                            Momo_Indent_Nest *n = nest;
                            sll_stack_pop(nest);
                            _momo_free_indent_nest(&nest_alloc, n);
                        }
                        if (nest != 0 && nest->kind == TokenBaseKind_ScopeOpen){
                            Momo_Indent_Nest *n = nest;
                            sll_stack_pop(nest);
                            _momo_free_indent_nest(&nest_alloc, n);
                        }
                        this_indent = 0;
                        if (nest != 0){
                            this_indent = nest->indent;
                        }
                        following_indent = this_indent;
                        ignore_unfinished_statement = true;
                    }break;
                    
                    case TokenBaseKind_ParentheticalOpen:
                    {
                        Momo_Indent_Nest *new_nest = _momo_new_indent_nest(arena, &nest_alloc);
                        sll_stack_push(nest, new_nest);
                        nest->kind = TokenBaseKind_ParentheticalOpen;
                        _momo_update_line_cache(app, buffer, tab_width, &line_cache);
                        nest->indent = (token->pos - line_cache.indent_info.first_char_pos) + 1;
                        following_indent = nest->indent;
                        shift_by_actual_indent = true;
                        ignore_unfinished_statement = true;
                    }break;
                    
                    case TokenBaseKind_ParentheticalClose:
                    {
                        if (nest != 0 && nest->kind == TokenBaseKind_ParentheticalOpen){
                            Momo_Indent_Nest *n = nest;
                            sll_stack_pop(nest);
                            _momo_free_indent_nest(&nest_alloc, n);
                        }
                        following_indent = 0;
                        if (nest != 0){
                            following_indent = nest->indent;
                        }
                        //ignore_unfinished_statement = true;
                    }break;
                }
                
                if (token->sub_kind == TokenCppKind_BlockComment ||
                    token->sub_kind == TokenCppKind_LiteralStringRaw){
                    ignore_unfinished_statement = true;
                }
                
                if (in_unfinished_statement && !ignore_unfinished_statement){
                    this_indent += indent_width;
                }
            }
            
#define EMIT(N) \
Stmnt(if (lines.first <= line_it){shifted_indentations[line_it]=N;} \
if (line_it == lines.end){goto finished;} \
actual_indent = N; )
            
            i64 line_it = line_last_indented;
            if (lines.first <= line_cache.where_token_starts){
                for (;line_it < line_cache.where_token_starts;){
                    line_it += 1;
                    if (line_it == line_cache.where_token_starts){
                        EMIT(this_indent);
                    }
                    else{
                        EMIT(last_indent);
                    }
                }
            }
            else{
                actual_indent = this_indent;
                line_it = line_cache.where_token_starts;
            }
            
            i64 line_where_token_ends = get_line_number_from_pos(app, buffer, token->pos + token->size);
            if (lines.first <= line_where_token_ends){
                _momo_update_line_cache(app, buffer, tab_width, &line_cache);
                i64 line_where_token_starts_shift = this_indent - line_cache.indent_info.indent_pos;
                for (;line_it < line_where_token_ends;){
                    line_it += 1;
                    i64 line_it_start_pos = get_line_start_pos(app, buffer, line_it);
                    Indent_Info line_it_indent_info = get_indent_info_line_number_and_start(app, buffer, line_it, line_it_start_pos, tab_width);
                    i64 new_indent = line_it_indent_info.indent_pos + line_where_token_starts_shift;
                    new_indent = clamp_bot(0, new_indent);
                    EMIT(new_indent);
                }
            }
            else{
                line_it = line_where_token_ends;
            }
#undef EMIT
            
            if (shift_by_actual_indent){
                nest->indent += actual_indent;
                following_indent += actual_indent;
            }
            
            if (token->kind != TokenBaseKind_Comment){
                in_unfinished_statement = _momo_is_unfinished_statement(token, nest);
                if (in_unfinished_statement){
                    following_indent += indent_width;
                }
            }
            
            last_indent = following_indent;
            line_last_indented = line_it;
            
            if (!token_it_inc_non_whitespace(&token_it)){
                break;
            }
        }
    }
    
    finished:;
    return(indentations);
}


internal i64*
_momo_get_indentations_based_on_extension(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 lines, Indent_Flag flags, i32 tab_width, i32 indent_width){
    String8 file_name = push_buffer_file_name(app, arena, buffer);
    if (file_name.size > 0){
        String8 extension = string_file_extension(file_name);
        if (string_match(extension, S8Lit("go"))) {
            return _momo_get_indentations_for_golang(app, arena, buffer, lines, flags, tab_width, indent_width);
        }
    }

    return _momo_get_indentations_for_default(app, arena, buffer, lines, flags, tab_width, indent_width);
    
}

function b32
momo_indent_buffer(Application_Links *app, Buffer_ID buffer, Range_i64 pos, Indent_Flag flags, i32 tab_width, i32 indent_width){
    ProfileScope(app, "auto indent buffer");
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Token_Array *tokens = &token_array;
    
    b32 result = false;
    if (tokens->tokens != 0){
        result = true;
        
        Scratch_Block scratch(app);
        Range_i64 line_numbers = {};
        if (HasFlag(flags, Indent_FullTokens)){
            i32 safety_counter = 0;
            for (;;){
                Range_i64 expanded = enclose_tokens(app, buffer, pos);
                expanded = enclose_whole_lines(app, buffer, expanded);
                if (expanded == pos){
                    break;
                }
                pos = expanded;
                safety_counter += 1;
                if (safety_counter == 20){
                    pos = buffer_range(app, buffer);
                    break;
                }
            }
        }
        line_numbers = get_line_range_from_pos_range(app, buffer, pos);
        
        i64 *indentations = _momo_get_indentations_based_on_extension(app, scratch, buffer, line_numbers, flags, tab_width, indent_width);
        _momo_set_line_indents(app, scratch, buffer, line_numbers, indentations, flags, tab_width);
    }
    
    return(result);
}

static b32
momo_indent_buffer(Application_Links *app, Buffer_ID buffer, Range_i64 pos, Indent_Flag flags){
    i32 indent_width = (i32)def_get_config_u64(app, vars_save_string_lit("indent_width"));
    i32 tab_width = (i32)def_get_config_u64(app, vars_save_string_lit("default_tab_width"));
    tab_width = clamp_bot(1, tab_width);
    AddFlag(flags, Indent_FullTokens);
    b32 indent_with_tabs = def_get_config_b32(vars_save_string_lit("indent_with_tabs"));
    if (indent_with_tabs){
        AddFlag(flags, Indent_UseTab);
    }
    return momo_indent_buffer(app, buffer, pos, flags, indent_width, tab_width);
}

static b32
momo_indent_buffer(Application_Links *app, Buffer_ID buffer, Range_i64 pos){
    return momo_indent_buffer(app, buffer, pos, 0);
}


function void
momo_write_text_and_indent(Application_Links* app, String_Const_u8 insert) {
    if (insert.str != 0 && insert.size > 0){
        b32 do_auto_indent = false;
        for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1){
            switch (insert.str[i]){
                case ';': case ':':
                case '{': case '}':
                case '(': case ')':
                case '[': case ']':
                case '#':
                case '\n': case '\t':
                {
                    do_auto_indent = true;
                }break;
            }
        }
        if (do_auto_indent){
            View_ID view = get_active_view(app, Access_ReadWriteVisible);
            Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
            
            Range_i64 pos = {};
            if (view_has_highlighted_range(app, view)){
                pos = get_view_range(app, view);
            }
            else{
                pos.min = pos.max = view_get_cursor_pos(app, view);
            }
            
            write_text_input(app);
            
            i64 end_pos = view_get_cursor_pos(app, view);
            pos.min = Min(pos.min, end_pos);
            pos.max = Max(pos.max, end_pos);
            
            momo_indent_buffer(app, buffer, pos, 0);
            move_past_lead_whitespace(app, view, buffer);
        }
        else{
            write_text_input(app);
        }
    }
}

////////////////////////////////

// BOTTOM

