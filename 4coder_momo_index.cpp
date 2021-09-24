#ifndef FCODER_MOMO_INDEX_CPP
#define FCODER_MOMO_INDEX_CPP


global Momo_Index_State momo_index = {};

internal void
Momo_Index_Initialize(void)
{
    momo_index.mutex = system_mutex_make();
    momo_index.arena = make_arena_system(KB(16));
}

internal void
Momo_Index_Lock(void)
{
    system_mutex_acquire(momo_index.mutex);
}

internal void
Momo_Index_Unlock(void)
{
    system_mutex_release(momo_index.mutex);
}

internal u64
_Momo_Index_FileHash(Application_Links *app, Buffer_ID id)
{
    Scratch_Block scratch(app);
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, id);
    return table_hash_u8(unique_name.str, unique_name.size);
}

internal Momo_Index_File *
_Momo_Index_LookupFile(Application_Links *app, u64 hash, Buffer_ID buffer)
{
    Momo_Index_File *result = 0;
    u64 slot = hash % ArrayCount(momo_index.file_table);
    for(Momo_Index_File *file = momo_index.file_table[slot]; file; file = file->hash_next)
    {
        if(file->buffer == buffer)
        {
            result = file;
            break;
        }
    }
    return result;
}

internal Momo_Index_File *
Momo_Index_LookupFile(Application_Links *app, Buffer_ID buffer)
{
    return _Momo_Index_LookupFile(app, _Momo_Index_FileHash(app, buffer), buffer);
}

internal Momo_Index_File *
Momo_Index_LookupOrMakeFile(Application_Links *app, Buffer_ID buffer)
{
    Momo_Index_File *result = 0;
    u64 hash = _Momo_Index_FileHash(app, buffer);
    u64 slot = hash % ArrayCount(momo_index.file_table);
    
    // NOTE(rjf): Lookup case.
    {
        result = _Momo_Index_LookupFile(app, hash, buffer);
        if(result)
        {
            goto end;
        }
    }
    
    // NOTE(rjf): Make case.
    {
        if(momo_index.free_file)
        {
            result = momo_index.free_file;
            momo_index.free_file = momo_index.free_file->hash_next;
            memset(result, 0, sizeof(*result));
        }
        else
        {
            result = push_array_zero(&momo_index.arena, Momo_Index_File, 1);
        }
        
        if(result != 0)
        {
            result->hash_next = momo_index.file_table[slot];
            momo_index.file_table[slot] = result;
            result->buffer = buffer;
            result->arena = make_arena_system(KB(16));
        }
    }
    
    end:;
    return result;
}

internal void
Momo_Index_EraseFile(Application_Links *app, Buffer_ID id)
{
    u64 hash = _Momo_Index_FileHash(app, id);
    Momo_Index_File *file = _Momo_Index_LookupFile(app, hash, id);
    if(file)
    {
        u64 slot = hash % ArrayCount(momo_index.file_table);
        {
            Momo_Index_File *prev = 0;
            for(Momo_Index_File *hash_file = momo_index.file_table[slot]; hash_file; prev = hash_file, hash_file = hash_file->hash_next)
            {
                if(file == hash_file)
                {
                    if(prev)
                    {
                        prev->hash_next = file->hash_next;
                    }
                    else
                    {
                        momo_index.file_table[slot] = file->hash_next;
                    }
                    break;
                }
            }
        }
        file->hash_next = momo_index.free_file;
        momo_index.free_file = file;
    }
}

internal void
_Momo_Index_FreeNoteTree(Momo_Index_Note *note)
{
    for(Momo_Index_Note *child = note->first_child; child; child = child->next_sibling)
    {
        _Momo_Index_FreeNoteTree(child);
    }
    
    Momo_Index_Note *prev = note->prev;
    Momo_Index_Note *next = note->next;
    Momo_Index_Note *hash_prev = note->hash_prev;
    Momo_Index_Note *hash_next = note->hash_next;
    
    u64 hash = note->hash;
    u64 slot = hash % ArrayCount(momo_index.note_table);
    
    if(prev)
    {
        prev->next = next;
    }
    if(next)
    {
        next->prev = prev;
    }
    
    if(prev == 0)
    {
        if(next)
        {
            next->hash_prev = hash_prev;
            next->hash_next = hash_next;
            if(hash_prev)
            {
                hash_prev->hash_next = next;
            }
            if(hash_next)
            {
                hash_next->hash_prev = next;
            }
        }
        else
        {
            if(hash_prev)
            {
                hash_prev->hash_next = hash_next;
            }
            if(hash_next)
            {
                hash_next->hash_prev = hash_prev;
            }
        }
        
        if(hash_prev == 0)
        {
            momo_index.note_table[slot] = next ? next : hash_next;
        }
    }
}

internal void
Momo_Index_ClearFile(Momo_Index_File *file)
{
    if(file)
    {
        file->generation += 1;
        for(Momo_Index_Note *note = file->first_note;
            note; note = note->next_sibling)
        {
            _Momo_Index_FreeNoteTree(note);
        }
        linalloc_clear(&file->arena);
        file->first_note = file->last_note = 0;
    }
}




internal Momo_Index_Note *
Momo_Index_LookupNote(String_Const_u8 string)
{
    Momo_Index_Note *result = 0;
    u64 hash = table_hash_u8(string.str, string.size);
    u64 slot = hash % ArrayCount(momo_index.note_table);
    for(Momo_Index_Note *note = momo_index.note_table[slot]; note; note = note->hash_next)
    {
        if(note->hash == hash)
        {
            if(string_match(string, note->string))
            {
                result = note;
                break;
            }
        }
    }
    return result;
}

internal Momo_Index_Note *
Momo_Index_LookupNote(String_Const_u8 string, Momo_Index_Note *parent)
{
    Momo_Index_Note *result = 0;
    u64 hash = table_hash_u8(string.str, string.size);
    u64 slot = hash % ArrayCount(momo_index.note_table);
    for(Momo_Index_Note *note = momo_index.note_table[slot]; note; note = note->hash_next)
    {
        if(note->hash == hash && note->parent == parent)
        {
            if(string_match(string, note->string))
            {
                result = note;
                break;
            }
        }
    }
    return result;
}

#if 0
internal Momo_Index_Note *
Momo_Index_LookupNote(String_Const_u8 string)
{
    return Momo_Index_LookupNote(string, 0);
}
#endif

internal Momo_Index_Note *
Momo_Index_MakeNote(Application_Links *app,
                  Momo_Index_File *file,
                  Momo_Index_Note *parent,
                  String_Const_u8 string,
                  Momo_Index_Note_Kind note_kind,
                  Momo_Index_Note_Flags note_flags,
                  Range_i64 range)
{
    ProfileScope(app, "[f] MakeNote");
    
    Momo_Index_Note *result = 0;
    if(file)
    {
        u64 hash = table_hash_u8(string.str, string.size);
        u64 slot = hash % ArrayCount(momo_index.note_table);
        
        if(momo_index.free_note)
        {
            result = momo_index.free_note;
            momo_index.free_note = momo_index.free_note->hash_next;
            memset(result, 0, sizeof(*result));
        }
        else
        {
            result = push_array_zero(&momo_index.arena, Momo_Index_Note, 1);
        }
        
        // NOTE(rjf): Push to duplicate chain.
        {
            Momo_Index_Note *list_head = Momo_Index_LookupNote(string);
            Momo_Index_Note *list_tail = list_head;
            for(Momo_Index_Note *note = list_tail; note; list_tail = note, note = note->next);
            if(list_tail != 0)
            {
                list_tail->next = result;
                result->prev = list_tail;
                result->hash_next = 0;
                result->hash_prev = 0;
            }
            else
            {
                result->hash_next = momo_index.note_table[slot];
                if(momo_index.note_table[slot])
                {
                    momo_index.note_table[slot]->hash_prev = result;
                }
                momo_index.note_table[slot] = result;
                result->hash_prev = 0;
                result->prev = 0;
            }
        }
        result->next = 0;
        
        // NOTE(rjf): Push to tree.
        {
            result->parent = parent;
            if(parent)
            {
                result->prev_sibling = parent->last_child;
                result->next_sibling = 0;
                if(parent->last_child == 0)
                {
                    parent->first_child = parent->last_child = result;
                }
                else
                {
                    parent->last_child->next_sibling = result;
                    parent->last_child = parent->last_child->next_sibling;
                }
            }
            else
            {
                result->prev_sibling = file->last_note;
                result->next_sibling = 0;
                if(file->last_note == 0)
                {
                    file->first_note = file->last_note = result;
                }
                else
                {
                    file->last_note->next_sibling = result;
                    file->last_note = file->last_note->next_sibling;
                }
            }
        }
        
        // NOTE(rjf): Fill out data.
        {
            result->hash = hash;
            result->string = push_string_copy(&file->arena, string);
            result->kind = note_kind;
            result->flags = note_flags;
            result->range = range;
            result->file = file;
            result->file_generation = file->generation;
        }
    }
    return result;
}

internal void
_Momo_Index_Parse(Application_Links *app, Momo_Index_File *file, String_Const_u8 string, Token_Array tokens, Momo_Language *language)
{
    Momo_Index_ParseCtx ctx =
    {
        false,
        app,
        file,
        string,
        tokens,
        token_iterator_pos(0, &ctx.tokens, 0),
    };
    if(language != 0)
    {
        language->IndexFile(&ctx);
    }
}

internal void
Momo_Index_ParseFile(Application_Links *app, Momo_Index_File *file, String_Const_u8 string, Token_Array tokens)
{
    Momo_Index_Lock();
    struct Momo_Language *lang = Momo_Language_GetFromBuffer(app, file->buffer);
    _Momo_Index_Parse(app, file, string, tokens, lang);
    Momo_Index_Unlock();
}

internal String_Const_u8
Momo_Index_StringFromToken(Momo_Index_ParseCtx *ctx, Token *token)
{
    Range_i64 token_range = Ii64(token->pos, token->pos+token->size);
    String_Const_u8 string = string_substring(ctx->string, token_range);
    return string;
}

internal b32
Momo_Index_ParseCtx_Inc(Momo_Index_ParseCtx *ctx, Momo_Index_Token_Skip_Flags flags)
{
    if(flags & MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE)
    {
        ctx->done = !token_it_inc_non_whitespace(&ctx->it);
    }
    else
    {
        ctx->done = !token_it_inc_all(&ctx->it);
    }
    return ctx->done;
}

internal b32
Momo_Index_RequireToken(Momo_Index_ParseCtx *ctx, String_Const_u8 string, Momo_Index_Token_Skip_Flags flags)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        String_Const_u8 token_string =
            string_substring(ctx->string, Ii64(token->pos, token->pos+token->size));
        if(string_match(string, token_string))
        {
            result = 1;
        }
    }
    else
    {
        ctx->done = 1;
    }
    if(result)
    {
        Momo_Index_ParseCtx_Inc(ctx, flags);
    }
    return result;
}

internal b32
Momo_Index_RequireTokenKind(Momo_Index_ParseCtx *ctx, Token_Base_Kind kind, Token **token_out, Momo_Index_Token_Skip_Flags flags)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->kind == kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }if(result)
    {
        Momo_Index_ParseCtx_Inc(ctx, flags);
    }
    return result;
}

internal b32
Momo_Index_RequireTokenSubKind(Momo_Index_ParseCtx *ctx, int sub_kind, Token **token_out, Momo_Index_Token_Skip_Flags flags)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->sub_kind == sub_kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }if(result)
    {
        Momo_Index_ParseCtx_Inc(ctx, flags);
    }
    return result;
}

internal b32
Momo_Index_PeekToken(Momo_Index_ParseCtx *ctx, String_Const_u8 string)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        String_Const_u8 token_string =
            string_substring(ctx->string, Ii64(token->pos, token->pos+token->size));
        if(string_match(string, token_string))
        {
            result = 1;
        }
    }
    else
    {
        ctx->done = 1;
    }
    return result;
}

internal b32
Momo_Index_PeekTokenKind(Momo_Index_ParseCtx *ctx, Token_Base_Kind kind, Token **token_out)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->kind == kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }
    return result;
}

internal b32
Momo_Index_PeekTokenSubKind(Momo_Index_ParseCtx *ctx, int sub_kind, Token **token_out)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->sub_kind == sub_kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }
    return result;
}

internal b32
Momo_Index_SeekToken(Momo_Index_ParseCtx *ctx, String_Const_u8 string)
{
    b32 found = 0;
    Token_Iterator_Array it_restore = ctx->it;
    for(;;)
    {
        Token *token = token_it_read(&ctx->it);
        if(token)
        {
            String_Const_u8 token_string = Momo_Index_StringFromToken(ctx, token);
            if(string_match(token_string, string))
            {
                found = 1;
                break;
            }
        }
        else
        {
            break;
        }
        if(!token_it_inc_all(&ctx->it))
        {
            break;
        }
    }
    if(found == 0)
    {
        ctx->it = it_restore;
    }
    return found;
}

internal void
Momo_Index_ParseComment(Momo_Index_ParseCtx *ctx, Token *token)
{
    String_Const_u8 string = Momo_Index_StringFromToken(ctx, token);
    
    for(u64 i = 0; i < string.size; i += 1)
    {
        if(string.str[i] == '@')
        {
            Momo_Index_MakeNote(ctx->app, ctx->file, 0, string_substring(string, Ii64(i, string.size)), MOMO_INDEX_NOTE_KIND_COMMENT_TAG, 0, Ii64(token));
            break;
        }
        else if(i+4 < string.size && string_match(S8Lit("TODO"), string_substring(string, Ii64(i, i + 4))))
        {
            Momo_Index_MakeNote(ctx->app, ctx->file, 0, string_substring(string, Ii64(i, string.size)), MOMO_INDEX_NOTE_KIND_COMMENT_TODO, 0, Ii64(token));
        }
    }
}

internal void
Momo_Index_SkipSoftTokens(Momo_Index_ParseCtx *ctx, b32 preproc)
{
    for(;!ctx->done;)
    {
        Token *token = token_it_read(&ctx->it);
        if(preproc)
        {
            if(!(token->flags & TokenBaseFlag_PreprocessorBody) ||
               token->kind == TokenBaseKind_Preprocessor)
            {
                break;
            }
        }
        else
        {
            if(token->kind == TokenBaseKind_StatementClose ||
               token->kind == TokenBaseKind_ScopeOpen ||
               token->kind == TokenBaseKind_ParentheticalOpen)
            {
                break;
            }
        }
        if(!token_it_inc_non_whitespace(&ctx->it))
        {
            break;
        }
    }
}

internal void
Momo_Index_SkipOpTokens(Momo_Index_ParseCtx *ctx)
{
    int paren_nest = 0;
    for(;!ctx->done;)
    {
        Token *token = token_it_read(&ctx->it);
        if(token->kind == TokenBaseKind_ParentheticalOpen)
        {
            paren_nest += 1;
        }
        else if(token->kind == TokenBaseKind_ParentheticalClose)
        {
            paren_nest -= 1;
            if(paren_nest < 0)
            {
                paren_nest = 0;
            }
        }
        else if(token->kind != TokenBaseKind_Operator && paren_nest == 0)
        {
            break;
        }
        Momo_Index_ParseCtx_Inc(ctx, MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE);
    }
}

function b32
Momo_Index_ParsePattern(Momo_Index_ParseCtx *ctx, char *fmt, ...)
{
    b32 parsed = 1;
    
    Momo_Index_ParseCtx ctx_restore = *ctx;
    Momo_Index_Token_Skip_Flags flags = MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE;
    
    va_list args;
    va_start(args, fmt);
    for(int i = 0; fmt[i];)
    {
        if(fmt[i] == '%')
        {
            switch(fmt[i+1])
            {
                case 't':
                {
                    char *cstring = va_arg(args, char *);
                    String8 string = SCu8((u8 *)cstring, cstring_length(cstring));
                    parsed = parsed && Momo_Index_RequireToken(ctx, string, flags);
                }break;
                
                case 'k':
                {
                    Token_Base_Kind kind = va_arg(args, Token_Base_Kind);
                    Token **output_token = va_arg(args, Token **);
                    parsed = parsed && Momo_Index_RequireTokenKind(ctx, kind, output_token, flags);
                }break;
                
                case 'b':
                {
                    i16 kind = va_arg(args, i16);
                    Token **output_token = va_arg(args, Token **);
                    parsed = parsed && Momo_Index_RequireTokenSubKind(ctx, kind, output_token, flags);
                }break;
                
                case 'n':
                {
                    Momo_Index_Note_Kind kind = va_arg(args, Momo_Index_Note_Kind);
                    Momo_Index_Note **output_note = va_arg(args, Momo_Index_Note **);
                    Token *token = 0;
                    parsed = parsed && Momo_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &token, flags);
                    parsed = parsed && !!token;
					if (parsed)
					{
						String8 token_string = Momo_Index_StringFromToken(ctx, token);
						Momo_Index_Note *note = Momo_Index_LookupNote(token_string, 0);
                        b32 kind_match = 0;
                        for(Momo_Index_Note *n = note; n; n = n->next)
                        {
                            if(n->kind == kind)
                            {
                                kind_match = 1;
                                note = n;
                                break;
                            }
                        }
						if (note && kind_match)
						{
							*output_note = note;
							parsed = 1;
						}
						else
						{
							parsed = 0;
						}
					}
                }break;
                
                case 's':
                {
                    Momo_Index_SkipSoftTokens(ctx, 0);
                }break;
                
                case 'o':
                {
                    Momo_Index_SkipOpTokens(ctx);
                }break;
                
                default: break;
            }
			i += 1;
        }
        else
        {
            i += 1;
        }
    }
    
    va_end(args);
    
    if(parsed == 0)
    {
        *ctx = ctx_restore;
    }
    return parsed;
}

internal void
Momo_Index_Tick(Application_Links *app)
{
    Scratch_Block scratch(app);
    for (Buffer_Modified_Node *node = global_buffer_modified_set.first; node != 0;node = node->next)
    {
        Temp_Memory_Block temp(scratch);
        Buffer_ID buffer_id = node->buffer;
        
        String_Const_u8 contents = push_whole_buffer(app, scratch, buffer_id);
        Token_Array tokens = get_token_array_from_buffer(app, buffer_id);
        if(tokens.count == 0) { continue; }
        
        Momo_Index_Lock();
        Momo_Index_File *file = Momo_Index_LookupOrMakeFile(app, buffer_id);
        if(file)
        {
            ProfileScope(app, "[f] reparse");
            Momo_Index_ClearFile(file);
            Momo_Index_ParseFile(app, file, contents, tokens);
        }
        Momo_Index_Unlock();
        buffer_clear_layout_cache(app, buffer_id);
    }
}
internal void
Momo_Index_GoToDefinitionInNote(Application_Links *app, Momo_Index_Note *note, b32 same_panel) { 
    if(note != 0 && note->file != 0) {
        View_ID view = get_active_view(app, Access_Always);
        Rect_f32 region = view_get_buffer_region(app, view);
        f32 view_height = rect_height(region);
        Buffer_ID buffer = note->file->buffer;
        if(!same_panel)
        {
            view = get_next_view_looped_primary_panels(app, view, Access_Always);
        }
        point_stack_push_view_cursor(app, view);
        view_set_buffer(app, view, buffer, 0);
        i64 line_number = get_line_number_from_pos(app, buffer, note->range.min);
        Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
        scroll.position.line_number = line_number;
        scroll.target.line_number = line_number;
        scroll.position.pixel_shift.y = scroll.target.pixel_shift.y = -view_height*0.5f;
        view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
        view_set_cursor(app, view, seek_pos(note->range.min));
        view_set_mark(app, view, seek_pos(note->range.min));
    }
}

#endif 
