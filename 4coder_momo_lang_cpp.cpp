
internal void
_momo_parse_cpp_macro(Momo_Index_ParseCtx *ctx)
{
  Token *name = 0;
  if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
  {
    String_Const_u8 name_str = Momo_Index_StringFromToken(ctx, name);
    
    Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str,
                        MOMO_INDEX_NOTE_KIND_MACRO, 0, Ii64(name));
    Momo_Index_SkipSoftTokens(ctx, 1);
  }
}

internal b32
_momo_parse_cpp_skippable_content(Momo_Index_ParseCtx *ctx)
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
      _momo_parse_cpp_macro(ctx);
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
    else if(Momo_Index_ParsePattern(ctx, "%t", "<"))
    {
      nest += 1;
      body_found = 1;
    }
    else if(Momo_Index_ParsePattern(ctx, "%t", ">"))
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

function void
_momo_parse_cpp_struct(Momo_Index_ParseCtx *ctx)
{
  Token *name = 0;
  b32 need_end_name = 0;
  
  if(!Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
  {
    need_end_name = 1;
  }

  // Parse until we reach open brace and close brace
  b32 found_open_brace = 0;
  b32 found_close_brace = 0;
  b32 found_early_semicolon = 0;
  for(;!ctx->done;) {
    if(Momo_Index_ParsePattern(ctx, "%t", "{")) {
      found_open_brace = 1;
      break;
    }
    if(Momo_Index_ParsePattern(ctx, "%t", ";")) {
      found_early_semicolon = 1;
      break;
    }
    Momo_Index_ParseCtx_Inc(ctx, MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE);
  }

  if (found_early_semicolon && !need_end_name) {
    // prototype found
    return;
  }


  // Look for corresponding close brace
  u32 brace_set_count = 1;
  for(;!ctx->done;) {
    if(Momo_Index_ParsePattern(ctx, "%t", "{")) {
      ++brace_set_count;
    }

    else if(Momo_Index_ParsePattern(ctx, "%t", "}")) {
      --brace_set_count;
      if (brace_set_count == 0)  {
        found_close_brace = 1;
        break;
      }
    }
    else {
      Momo_Index_ParseCtx_Inc(ctx, MOMO_INDEX_TOKEN_SKIP_FLAG_WHITESPACE);
    }
  }


  // Confirm that this is a definition and do the remaining.
  if (found_open_brace && found_close_brace) {
    b32 valid = 0;
    if(need_end_name) {
      if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
      {
        valid = 1;
      }
    }
    else {
      valid = 1;
    }

    if(valid)
    {
      // struct have same key and display
      String_Const_u8 name_str = Momo_Index_StringFromToken(ctx, name);
      Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str, 
                          MOMO_INDEX_NOTE_KIND_TYPE, 0, Ii64(name));
    }
  }

}


function b32
_momo_parse_cpp_function_body(Momo_Index_ParseCtx *ctx, b32 *prototype_ptr)
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
      _momo_parse_cpp_skippable_content(ctx);
    }
  }
  
  *prototype_ptr = prototype;
  
  return valid;
}

function void
_momo_make_cpp_enum_note(Momo_Index_ParseCtx * ctx, Token* constant, Token* parent_name = 0) {
  if(parent_name) {
    u8 buffer[512];
    String_u8 display_str = Su8(buffer, 0, ArrayCount(buffer));            
    String_Const_u8 constant_str = Momo_Index_StringFromToken(ctx, constant);
    String_Const_u8 parent_str = Momo_Index_StringFromToken(ctx, parent_name);

    string_append(&display_str, parent_str);
    string_append(&display_str, string_u8_litexpr("::"));
    string_append(&display_str, constant_str);

    Momo_Index_MakeNote(ctx->app, ctx->file, constant_str, display_str.string, MOMO_INDEX_NOTE_KIND_CONSTANT, 0, Ii64(constant));
  }
  else {
    String_Const_u8 constant_str = Momo_Index_StringFromToken(ctx, constant);
    Momo_Index_MakeNote(ctx->app, ctx->file, constant_str, constant_str, MOMO_INDEX_NOTE_KIND_CONSTANT, 0, Ii64(constant));
  }

}

function void
_momo_parse_cpp_enum_body(Momo_Index_ParseCtx *ctx, Token* parent_name = 0)
{
  if(Momo_Index_ParsePattern(ctx, "%t", "{"))
  {
    for(;!ctx->done;)
    {
      Token *constant = 0;
      if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Comment, &constant))
      {
        Momo_Index_ParseComment(ctx, constant);
      }
      else if(Momo_Index_ParsePattern(ctx, "%b", TokenCppKind_PPDefine, &constant))
      {
        _momo_parse_cpp_macro(ctx);
      }
      else if(Momo_Index_ParsePattern(ctx, "%k%t", TokenBaseKind_Identifier, &constant, ","))
      {
        _momo_make_cpp_enum_note(ctx, constant, parent_name);              
      }
      else if(Momo_Index_ParsePattern(ctx, "%k%t", TokenBaseKind_Identifier, &constant, "="))
      {
        _momo_make_cpp_enum_note(ctx, constant, parent_name);             

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
        _momo_make_cpp_enum_note(ctx, constant, parent_name);              
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

internal MOMO_LANGUAGE_INDEXFILE(momo_index_cpp_file)
{
  int scope_nest = 0;
  int template_nest = 0;
  for(b32 handled = 0; !ctx->done;)
  {
    handled = 0;
    
    Token *name = 0;
    Token *base_type = 0;
    Token *scope_name = 0;
    
    
    
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

    else if(Momo_Index_ParsePattern(ctx, "%t", "<"))
    {
      handled = 1;
      template_nest += 1;
    }
    else if(Momo_Index_ParsePattern(ctx, "%t", ">"))
    {
      handled = 1;
      template_nest -= 1;
      if(template_nest < 0)
      {
        template_nest = 0;
      }
    }
    
    //~ NOTE(rjf): Macros
    else if(Momo_Index_ParsePattern(ctx, "%b", TokenCppKind_PPDefine, &name))
    {
      handled = 1;
      _momo_parse_cpp_macro(ctx);
    }
    
    
    //~ NOTE(rjf): Structs
    else if(scope_nest == 0 && 
            Momo_Index_ParsePattern(ctx, "%t", "struct"))
    {
      handled = 1;
      _momo_parse_cpp_struct(ctx);
    }
    else if(scope_nest == 0 && 
            Momo_Index_ParsePattern(ctx, "%t%t", "typedef", "struct"))
    {
      handled = 1;
      _momo_parse_cpp_struct(ctx);
      
      // What is this for?
#if 0
      if (Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
      {
        Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, name),
                            MOMO_INDEX_NOTE_KIND_TYPE, MOMO_INDEX_NOTE_FLAG_PRODUCT_TYPE, Ii64(name));
      }
#endif
    }
    
    //~ NOTE(Momo): Class
    else if(scope_nest == 0 && 
            Momo_Index_ParsePattern(ctx, "%t", "class"))
    {
      handled = 1;
      _momo_parse_cpp_struct(ctx);
    }
    
    //~ NOTE(rjf): Unions
    else if(scope_nest == 0 && 
            Momo_Index_ParsePattern(ctx, "%t", "union"))
    {
      handled = 1;
      _momo_parse_cpp_struct(ctx);
    }
    else if (scope_nest == 0 && 
             Momo_Index_ParsePattern(ctx, "%t%t", "typedef", "union"))
    {
      handled = 1;
      _momo_parse_cpp_struct(ctx);
      
      // What is this for?
#if 0
      if (Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
      {
        Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, name),
                            MOMO_INDEX_NOTE_KIND_TYPE, MOMO_INDEX_NOTE_FLAG_SUM_TYPE, Ii64(name));
      }
#endif
    }
    
    //~ NOTE(rjf): Typedef'd Enums
    else if(scope_nest == 0 && 
            Momo_Index_ParsePattern(ctx, "%t%t%k", "typedef", "enum", TokenBaseKind_Identifier, &name) ||
            Momo_Index_ParsePattern(ctx, "%t%t", "typedef", "enum"))
    {
      handled = 1;
      b32 prototype = 0;
      b32 possible_name_at_end = name == 0;
      if(Momo_Index_ParsePattern(ctx, "%t", ";"))
      {
        prototype = 1;
      }
      if(prototype == 0)
      {
        _momo_parse_cpp_enum_body(ctx);
      }
      if(possible_name_at_end)
      {
        if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
        {}
      }
      if(name != 0 && !prototype)
      {
        String_Const_u8 name_str = Momo_Index_StringFromToken(ctx, name);
        Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str, 
                            MOMO_INDEX_NOTE_KIND_TYPE, 0, Ii64(name));
      }
    }
   
    else if (scope_nest == 0 &&
             Momo_Index_ParsePattern(ctx, "%t%t%k", "enum", "struct", TokenBaseKind_Identifier, &name))
    {
      handled = 1;
      b32 prototype = 0;
      if(Momo_Index_ParsePattern(ctx, "%t", ";"))
      {
        prototype = 1;
      }
      if(prototype == 0)
      {
        _momo_parse_cpp_enum_body(ctx, name);
      }
      if(name != 0 && !prototype)
      {
        String_Const_u8 name_str = Momo_Index_StringFromToken(ctx, name);  
        Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str,
                            MOMO_INDEX_NOTE_KIND_TYPE, 0, Ii64(name));
      }
    }
    //~ NOTE(rjf): Enums
    else if(scope_nest == 0 &&
            Momo_Index_ParsePattern(ctx, "%t%k", "enum", TokenBaseKind_Identifier, &name) ||
            Momo_Index_ParsePattern(ctx, "%t", "enum"))
    {
      handled = 1;
      b32 prototype = 0;
      if(Momo_Index_ParsePattern(ctx, "%t", ";"))
      {
        prototype = 1;
      }
      if(prototype == 0)
      {
        _momo_parse_cpp_enum_body(ctx);
      }
      if(name != 0 && !prototype)
      {
        String_Const_u8 name_str = Momo_Index_StringFromToken(ctx, name);  
        Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str,
                            MOMO_INDEX_NOTE_KIND_TYPE, 0, Ii64(name));
      }
    }
    
#if 0
    //~ NOTE(rjf): Typedef'ed functions
    else if(scope_nest == 0 &&
            (Momo_Index_ParsePattern(ctx, "%t%k%t%t%o%k%t%t",
                                     "typedef",
                                     TokenBaseKind_Keyword, &base_type,
                                     "(", "*",
                                     TokenBaseKind_Identifier, &name,
                                     ")", "(") ||
             Momo_Index_ParsePattern(ctx, "%t%k%t%t%o%k%t%t",
                                     "typedef",
                                     TokenBaseKind_Identifier, &base_type,
                                     "(", "*",
                                     TokenBaseKind_Identifier, &name,
                                     ")", "(")))
    {
      handled = 1;
      int is_valid = 0;
      if(name != 0)
      {
        while(!ctx->done) {
          Token* token = token_it_read(&ctx->it);
          if (token == 0) {
            break;
          }
          if (token->sub_kind == TokenCppKind_Semicolon) {
            if (is_valid) {
              is_valid = 2;
            }
            // This is a prototype, so we ignore.
            break;
          }
          else if (token->sub_kind == TokenCppKind_ParenCl) {
            if(is_valid == 0) {
              is_valid = 1;
            }
          }
          
          Momo_Index_ParseCtx_Inc(ctx, 0);    
          
        }
        
        if (is_valid == 2) {
          Momo_Index_Note_Flags note_flags = 0;
          String_Const_u8 name_str =  Momo_Index_StringFromToken(ctx, name);
          Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str,
                              MOMO_INDEX_NOTE_KIND_TYPE, note_flags, Ii64(name));
        }
      }
    }
#endif 
    //~ NOTE(rjf): Pure Typedefs
    // TODO(Momo): I don't think we care about sum/product types here
    else if(Momo_Index_ParsePattern(ctx, "%t", "typedef"))
    {
      handled = 1;
      int nest = 0;
      b32 sum_type = 0;
      for(;!ctx->done;)
      {
        if(Momo_Index_ParsePattern(ctx, "%t", "("))
        {
          nest += 1;
        }
        else if(Momo_Index_ParsePattern(ctx, "%t", "("))
        {
          nest -= 1;
        }
        else if(nest == 0 && Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
        {
          String8 name_str = Momo_Index_StringFromToken(ctx, name);
          Momo_Index_Note *namenote = Momo_Index_LookupNote(name_str, 0);
          if(namenote != 0 && namenote->kind == MOMO_INDEX_NOTE_KIND_TYPE &&
             namenote->flags & MOMO_INDEX_NOTE_FLAG_SUM_TYPE)
          {
            sum_type = 1;
          }
        }
        else if(Momo_Index_ParsePattern(ctx, "%t", ";"))
        {
          break;
        }
        else 
        {
          Momo_Index_ParseCtx_Inc(ctx, 0);
        }
      }
      if(name != 0)
      {
        Momo_Index_Note_Flags note_flags = 0;
        if(sum_type)
        {
          note_flags |= MOMO_INDEX_NOTE_FLAG_SUM_TYPE;
        }
        String_Const_u8 name_str =  Momo_Index_StringFromToken(ctx, name);
        Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str,
                            MOMO_INDEX_NOTE_KIND_TYPE, note_flags, Ii64(name));
      }
      
      
    }
    
    
    
    //~ NOTE(rjf): Functions
    else if(scope_nest == 0 &&
            (Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                     TokenBaseKind_Identifier, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     "(") ||
             Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                     TokenBaseKind_Keyword, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     "(")))
      
    {
      handled = 1;
      b32 is_valid = 0;
      
      Range_i64 prototype_range = {};
      prototype_range.min = name->pos;
      
      while(!ctx->done) {
        Token* token = token_it_read(&ctx->it);
        if (token == 0) {
          break;
        }
        if (token->sub_kind == TokenCppKind_Semicolon) {
          // This is a prototype, so we ignore.
          break;
        }
        else if (token->sub_kind == TokenCppKind_ParenCl) {
          prototype_range.max = token->pos+token->size;
        }
        
        else if(token->kind == TokenBaseKind_ScopeOpen)
        {
          is_valid = 1;
          break;
        }
        Momo_Index_ParseCtx_Inc(ctx, 0);    
        
      }
      
      if (is_valid) {
        // For functions, the display is the whole function itself
        u8 buffer[512];
        String_u8 display_str = Su8(buffer, 0, ArrayCount(buffer));
        
        String_Const_u8 key_str = Momo_Index_StringFromToken(ctx, name);  
        string_append(&display_str, Momo_Index_StringFromRange(ctx, prototype_range));
        Momo_Index_MakeNote(ctx->app, ctx->file, key_str, display_str.string,
                            MOMO_INDEX_NOTE_KIND_FUNCTION, 0, Ii64(name));
        
        
      }
    }
    
    
    // ignore C++ operator overloading
    else if (scope_nest == 0 && 
             (Momo_Index_ParsePattern(ctx, "%k%o%k%k%t",
                                      TokenBaseKind_Keyword, &base_type,
                                      TokenBaseKind_Keyword, &name,
                                      TokenBaseKind_Operator, &name,
                                      "(")) ||
             (Momo_Index_ParsePattern(ctx, "%k%o%k%k%t",
                                      TokenBaseKind_Identifier, &base_type,
                                      TokenBaseKind_Keyword, &name,
                                      TokenBaseKind_Operator, &name,
                                      "(")))
    {
      handled = 1;
      while(!ctx->done) {
        Token* token = token_it_read(&ctx->it);
        if (token == 0) {
          break;
        }
        if (token->sub_kind == TokenCppKind_Semicolon) {
          // This is a prototype, so we ignore.
          break;
        }
        else if(token->kind == TokenBaseKind_ScopeOpen)
        {
          break;
        }
        Momo_Index_ParseCtx_Inc(ctx, 0);
      }
    }
    
    //~ NOTE(Momo): C++ Member Functions (outside of the struct/class)
    else if(scope_nest == 0 &&
            (Momo_Index_ParsePattern(ctx, "%k%o%k%t%k%t",
                                     TokenBaseKind_Identifier, &base_type,
                                     TokenBaseKind_Identifier, &scope_name,
                                     "::",
                                     TokenBaseKind_Identifier, &name,
                                     "(") ||
             Momo_Index_ParsePattern(ctx, "%k%o%k%t%k%t",
                                     TokenBaseKind_Keyword, &base_type,
                                     TokenBaseKind_Identifier, &scope_name,
                                     "::",
                                     TokenBaseKind_Identifier, &name,
                                     "(")))
    {
      handled = 1;         
      b32 is_valid = 0;
      Range_i64 prototype_range = {};

  
      while(!ctx->done) {
        Token* token = token_it_read(&ctx->it);
        if (token == 0) {
          break;
        }
        if (token->sub_kind == TokenCppKind_Semicolon) {
          // This is a prototype, so we ignore.
          break;
        }
        else if (token->sub_kind == TokenCppKind_ParenCl) {
          prototype_range.max = token->pos+token->size;
        }
        
        else if(token->kind == TokenBaseKind_ScopeOpen)
        {
          is_valid = 1;
          break;
        }
        Momo_Index_ParseCtx_Inc(ctx, 0);
      }
      if (is_valid && scope_name) {
        prototype_range.min = scope_name->pos;
        
        // For functions, the display is the whole function itself
        u8 buffer[512];
        String_u8 display_str = Su8(buffer, 0, ArrayCount(buffer));
        
        String_Const_u8 key_str = Momo_Index_StringFromToken(ctx, name);  
        string_append(&display_str, Momo_Index_StringFromRange(ctx, prototype_range));
        Momo_Index_MakeNote(ctx->app, ctx->file, key_str, display_str.string,
                            MOMO_INDEX_NOTE_KIND_FUNCTION, 0, Ii64(name));
        
        
      }
      
    }
    
    //~ NOTE(rjf): Declarations
    else if(scope_nest == 0 && template_nest == 0 &&
            (Momo_Index_ParsePattern(ctx, "%k%o%k%o%t",
                                     TokenBaseKind_Identifier, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     ";") ||
             Momo_Index_ParsePattern(ctx, "%k%o%k%o%t",
                                     TokenBaseKind_Keyword, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     ";") ||
             Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                     TokenBaseKind_Identifier, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     "=") ||
             Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                     TokenBaseKind_Keyword, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     "=") ||       
             Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                     TokenBaseKind_Keyword, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     "[") ||
             Momo_Index_ParsePattern(ctx, "%k%o%k%t",
                                     TokenBaseKind_Identifier, &base_type,
                                     TokenBaseKind_Identifier, &name,
                                     "[")))
                                  
    {
      handled = 1;
      String_Const_u8 name_str = Momo_Index_StringFromToken(ctx, name);
      Momo_Index_MakeNote(ctx->app, ctx->file, name_str, name_str, MOMO_INDEX_NOTE_KIND_DECL, 0, Ii64(name));
    }
    
    //~ NOTE(rjf): Macro Functions
    // TODO(Momo): Should get this to work. Could actually be useful!
#if 0
    else if(Momo_Index_ParsePattern(ctx, "%n%t%k",
                                    MOMO_INDEX_NOTE_KIND_MACRO, &note,
                                    "(",
                                    TokenBaseKind_Identifier, &name))
    {
      b32 valid = 0;
      b32 prototype = 0;
      
      for(;!ctx->done;)
      {
        Token *token = token_it_read(&ctx->it);
        if(token == 0) { break; }
        if(token->sub_kind == TokenCppKind_Semicolon)
        {
          prototype = 1;
          valid = 1;
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
        handled = 1;
        Momo_Index_MakeNote(ctx->app, ctx->file, 0, Momo_Index_StringFromToken(ctx, name),
                            MOMO_INDEX_NOTE_KIND_FUNCTION, prototype ? MOMO_INDEX_NOTE_FLAG_PRODUCT_TYPE : 0, Ii64(name));
        _momo_parse_cpp_skippable_content(ctx);
      }
    }
#endif        
    
    //~ NOTE(rjf): Comment Tags
    else if(Momo_Index_ParsePattern(ctx, "%k", TokenBaseKind_Comment, &name))
    {
      handled = 1;
      Momo_Index_ParseComment(ctx, name);
    }
    
    
    
    if(handled == 0)
    {
      Momo_Index_ParseCtx_Inc(ctx, 0);
    }
  }
}

internal MOMO_LANGUAGE_POSCONTEXT(momo_cpp_poscontext)
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
            momo_push_data_to_language_pos_context_call(arena, &first, &last, push_buffer_range(app, arena, buffer, Ii64(name)), arg_idx);
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

internal MOMO_LANGUAGE_HIGHLIGHT(momo_highlight_cpp)
{
}
