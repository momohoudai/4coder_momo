
internal void
Momo_Go_Parse_Macro(Momo_Index_ParseCtx *ctx)
{
    Token *name = 0;
    if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
    {
        Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, name),
                          MOMO_INDEX_NOTE_KIND_MACRO, 0, Ii64(name));
        Momo_Index_SkipSoftTokens(ctx, 1);
    }
}



internal b32
Momo_Go_Parse_Skippable_Content(Momo_Index_ParseCtx *ctx)
{
    b32 body_found = 0;
    int nest = 0;
    
    for(;!ctx->done;)
    {
        Token *name = 0;
        
        if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Comment, &name))
        {
            Momo_Index_ParseComment(ctx, name);
        }
        else if(Momo_Index_ParsePattern(ctx, "%b", TokenCppKind_PPDefine, &name))
        {
            Momo_Go_Parse_Macro(ctx);
        }
        else if(Momo_Index_ParsePattern(ctx, "%t", "{"))
        {
            nest += 1;
            body_found = 1;
        }
        else if(Momo_Index_ParsePattern(ctx, "%t", "}"))
        {
            nest -= 1;
            if(nest == 0)
            {
                break;
            }
        }
        else if(body_found == 0)
        {
            break;
        }
        else
        {
            Momo_Index_ParseCtx_Inc(ctx, MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE);
        }
    }
    return body_found;
}



function b32
Momo_Go_Parse_FunctionBody(Momo_Index_ParseCtx *ctx)
{
    b32 valid = 0;
    b32 prototype = 0;
    
    for(;!ctx->done;)
    {
        Token *token = token_it_read(&ctx->it);
        if(token == 0) { break; }
        if(token->sub_kind == TokenCppKind_Semicolon)
        {
            valid = 1;
            prototype = 1;
            break;
        }
        else if(token->sub_kind == TokenCppKind_ParenCl)
        {
        }
        else if(token->kind == TokenBaseKind_ScopeOpen)
        {
            valid = 1;
            break;
        }
        Momo_Index_ParseCtx_Inc(ctx, 0);
    }
    
    if(valid)
    {
        if(prototype == 0)
        {
            Momo_Go_Parse_Skippable_Content(ctx);
        }
    }
    
    return valid;
}

// Go structs can contain functions, so we need to parse that within itself as well...
function void
Momo_Go_Parse_StructBody(Momo_Index_ParseCtx *ctx)
{
    Token *base_type = 0;
    Token *name = 0;
    Token* func_name = 0;
    b32 valid = 0;
    
    // must have name
    if(!Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name)) {
        return;
    }

    String_Const_u8 t = Momo_Index_StringFromToken(ctx, name);
   
    b32 found_open_brace = 0;
    if(Momo_Index_ParsePattern(ctx, "%t", ":")) {
        for(;!ctx->done;) {
            if(Momo_Index_ParsePattern(ctx, "%t", "{")) {
                found_open_brace = 1;
                break;
            }
            else {
                Momo_Index_ParseCtx_Inc(ctx, 0);
            }
        }
    }

    Momo_Index_Note *parent_struct = Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, name),
                            MOMO_INDEX_NOTE_KIND_TYPE, 0, Ii64(name));

    // followed by open prases
    if(found_open_brace || Momo_Index_ParsePattern(ctx, "%t", "{")) {
        for(;!ctx->done;)
        {   
            // here, we look for functions
            if( Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                        TokenBaseKind_Identifier, &base_type,
                                        TokenBaseKind_Identifier, &func_name,
                                        "(")||
                Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                        TokenBaseKind_Keyword, &base_type,
                                        TokenBaseKind_Identifier, &func_name,
                                        "("))
            {
                // ignore access modifiers. There are probably more things to filter? But we'll see
                String_Const_u8 base_type_str = Momo_Index_StringFromToken(ctx, base_type);
                b32 constructor = 0;
                if (string_match(base_type_str, string_u8_litexpr("public")) || 
                    string_match(base_type_str, string_u8_litexpr("private")) || 
                    string_match(base_type_str, string_u8_litexpr("protected")))
                {
                    // probably a constructor?
                    constructor = 1;
                }

                if (!constructor) {
                    if(Momo_Go_Parse_FunctionBody(ctx)) {
                        Momo_Index_MakeNote(ctx->app, ctx->file, parent_struct, Momo_Index_StringFromToken(ctx, func_name),
                                        MOMO_INDEX_NOTE_KIND_FUNCTION, 0, Ii64(func_name));
                    }
                }
                
            }
            else if(Momo_Index_ParsePattern(ctx, "%t", "}"))
            {
                valid = 1;
                break;
            }        
            else
            {
                Momo_Index_ParseCtx_Inc(ctx, 0);
            }
        }
    }
    
    if(!valid) {
        // Invalidate the note?
        //Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, name),
        //                    MOMO_INDEX_NOTE_KIND_TYPE, 0, Ii64(name));
    }
}

function void
Momo_Go_Parse_EnumBody(Momo_Index_ParseCtx *ctx)
{
    if(Momo_Index_ParsePattern(ctx, "%t", "{"))
    {
        for(;!ctx->done;)
        {
            Token *constant = 0;
            if(Momo_Index_ParsePattern(ctx, "%k%t", TokenBaseKind_Identifier, &constant, ","))
            {
                Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, constant), MOMO_INDEX_NOTE_KIND_CONSTANT, 0, Ii64(constant));
            }
            else if(Momo_Index_ParsePattern(ctx, "%k%t", TokenBaseKind_Identifier, &constant, "="))
            {
                Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, constant), MOMO_INDEX_NOTE_KIND_CONSTANT, 0, Ii64(constant));
                
                for(;!ctx->done;)
                {
                    Token *token = token_it_read(&ctx->it);
                    if(token->kind == TokenBaseKind_StatementClose)
                    {
                        Momo_Index_ParseCtx_Inc(ctx, 0);
                        break;
                    }
                    else if(token->kind == TokenBaseKind_ScopeClose ||
                            token->kind == TokenBaseKind_ScopeOpen)
                    {
                        break;
                    }
                    Momo_Index_ParseCtx_Inc(ctx, 0);
                }
            }
            else if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &constant))
            {
                Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, constant), MOMO_INDEX_NOTE_KIND_CONSTANT, 0, Ii64(constant));
            }
            else if(Momo_Index_ParsePattern(ctx, "%t", "}"))
            {
                break;
            }
            else
            {
                Momo_Index_ParseCtx_Inc(ctx, 0);
            }
        }
    }
}

internal MOMO_LANGUAGE_INDEXFILE(Momo_Go_Index_File)
{
    int scope_nest = 0;
    for(b32 handled = 0; !ctx->done;)
    {
        handled = 0;
        
        Token *name = 0;
        //Momo_Index_Note *containing_struct = 0;
        //Momo_Index_Note *note = 0;
        
        if(0){}
        
        //~ NOTE(rjf): Extern "C" scope changes (ignore) ((dude C++ syntax is so fucked up))
        // NOTE(rjf): CORRECTION: Text files in general are so fucked up, fuck all of this
        // parsing bullshit
        else if(Momo_Index_ParsePattern(ctx, "%t%t%t", "extern", "C", "{"))
        {
            handled = 1;
        }
        
        //~ NOTE(rjf): Scope Nest Changes
        else if(Momo_Index_ParsePattern(ctx, "%t", "{"))
        {
            handled = 1;
            scope_nest += 1;
        }
        else if(Momo_Index_ParsePattern(ctx, "%t", "}"))
        {
            handled = 1;
            scope_nest -= 1;
            if(scope_nest < 0)
            {
                scope_nest = 0;
            }
        }
        
        //~ NOTE(rjf): Structs and classes
        else if(Momo_Index_ParsePattern(ctx, "%t", "struct") || 
                Momo_Index_ParsePattern(ctx, "%t", "class"))
        {
            handled = 1;      
            Momo_Go_Parse_StructBody(ctx);
                    
        }
        


        // NOTE(Momo): Maybe we don't care about enums?
        // Because enums in C# is like variables
        else if(Momo_Index_ParsePattern(ctx, "%t%k", "enum", TokenBaseKind_Identifier, &name))
        {
            handled = 1;
            //Momo_Go_Parse_EnumBody(ctx);
            Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, name),
                                  MOMO_INDEX_NOTE_KIND_TYPE, 0, Ii64(name));
        }


        //~ NOTE(rjf): Comment Tags
        else if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Comment, &name))
        {
            handled = 1;
            Momo_Index_ParseComment(ctx, name);
        }
        
        //~ NOTE(rjf): Macros
        else if(Momo_Index_ParsePattern(ctx, "%b", TokenCppKind_PPDefine, &name))
        {
            handled = 1;
            Momo_Go_Parse_Macro(ctx);
        }
        
        
        if(handled == 0)
        {
            Momo_Index_ParseCtx_Inc(ctx, 0);
        }
    }
}

internal MOMO_LANGUAGE_POSCONTEXT(Momo_Go_PosContext)
{
    int count = 0;
    Momo_Language_PosContextData *first = 0;
    Momo_Language_PosContextData *last = 0;
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
    
    // NOTE(rjf): Search for left parentheses (function call or macro invocation).
    {
        int paren_nest = 0;
        int arg_idx = 0;
        for(int i = 0; count < 4; i += 1)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(paren_nest == 0 &&
                   token->sub_kind == TokenCppKind_ParenOp &&
                   token_it_dec_non_whitespace(&it))
                {
                    Token *name = token_it_read(&it);
                    if(name && name->kind == TokenBaseKind_Identifier)
                    {
                        Momo_Language_PosContext_PushDataCall(arena, &first, &last, push_buffer_range(app, arena, buffer, Ii64(name)), arg_idx);
                        count += 1;
                        arg_idx = 0;
                    }
                }
                else if(token->sub_kind == TokenCppKind_ParenOp)
                {
                    paren_nest -= 1;
                }
                else if(token->sub_kind == TokenCppKind_ParenCl && i > 0)
                {
                    paren_nest += 1;
                }
                else if(token->sub_kind == TokenCppKind_Comma && i > 0 && paren_nest == 0)
                {
                    arg_idx += 1;
                }
            }
            else { break; }
            if(!token_it_dec_non_whitespace(&it))
            {
                break;
            }
        }
    }
    
    // NOTE(rjf): Search for *.* pattern, or *->* pattern (accessing a type)
    {
#if 0
        Token *last_query_candidate = 0;
        for(;;)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(token->kind == TokenBaseKind_Identifier)
                {
                    last_query_candidate = token;
                }
                else if((token->sub_kind == TokenCppKind_Dot ||
                         token->sub_kind == TokenCppKind_Arrow) &&
                        token_it_dec_non_whitespace(&it))
                {
                    Token *decl_name = token_it_read(&it);
                    if(decl_name && decl_name->kind == TokenBaseKind_Identifier)
                    {
                        
                    }
                }
            }
            else { break; }
            if(!token_it_dec_non_whitespace(&it))
            {
                break;
            }
        }
#endif
    }
    
    return first;
}

internal MOMO_LANGUAGE_HIGHLIGHT(Momo_Go_Highlight)
{
}
