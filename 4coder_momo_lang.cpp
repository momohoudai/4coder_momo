/* date = January 29th 2021 9:37 pm */

internal Momo_Language *
Momo_Language_GetFromString(String_Const_u8 name)
{
    Momo_Language *result = 0;
    if(momo_langs.initialized)
    {
        u64 hash = table_hash_u8(name.str, name.size);
        u64 slot = hash % ArrayCount(momo_langs.language_table);
        for(Momo_Language *l = momo_langs.language_table[slot]; l; l = l->next)
        {
            if(l->hash == hash && string_match(l->name, name))
            {
                result = l;
                break;
            }
        }
    }
    return result;
}

#define Momo_Language_Register(name, IndexFile, LexInit, LexFullInput, PosContext, Highlight, lex_state_type) _Momo_Language_Resgister(name, IndexFile, (Momo_Language_LexInit *)LexInit, (Momo_Language_LexFullInput *)LexFullInput, (Momo_Language_PosContext *)PosContext, (Momo_Language_Highlight *)Highlight, sizeof(lex_state_type))

internal void
_Momo_Language_Resgister(String_Const_u8 name,
                     Momo_Language_IndexFile          *IndexFile,
                     Momo_Language_LexInit            *LexInit,
                     Momo_Language_LexFullInput       *LexFullInput,
                     Momo_Language_PosContext         *PosContext,
                     Momo_Language_Highlight          *Highlight,
                     u64 lex_state_size)
{
    if(momo_langs.initialized == 0)
    {
        momo_langs.initialized = 1;
        momo_langs.arena = make_arena_system(KB(16));
    }
    
    Momo_Language *language = 0;
    u64 hash = table_hash_u8(name.str, name.size);
    u64 slot = hash % ArrayCount(momo_langs.language_table);
    for(Momo_Language *l = momo_langs.language_table[slot]; l; l = l->next)
    {
        if(l->hash == hash && string_match(l->name, name))
        {
            language = l;
            break;
        }
    }
    
    if(language == 0)
    {
        language = push_array(&momo_langs.arena, Momo_Language, 1);
        language->next = momo_langs.language_table[slot];
        momo_langs.language_table[slot] = language;
        language->hash = hash;
        language->name = push_string_copy(&momo_langs.arena, name);
        language->lex_state_size     = lex_state_size;
        language->IndexFile          = IndexFile;
        language->LexInit            = LexInit;
        language->LexFullInput       = LexFullInput;
		language->PosContext         = PosContext;
        language->Highlight          = Highlight;
    }
}

internal Momo_Language *
Momo_Language_GetFromBuffer(Application_Links *app, Buffer_ID buffer)
{
    Momo_Language *language = 0;
    Scratch_Block scratch(app);
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer);
    String_Const_u8 extension = string_file_extension(file_name);
    language = Momo_Language_GetFromString(extension);
    return language;
}

internal void
Momo_Language_PosContext_PushData(Arena *arena,
                                Momo_Language_PosContextData **first_ptr,
                                Momo_Language_PosContextData **last_ptr,
                                Momo_Index_Note *note,
                                Token *query,
                                int arg_index)
{
    Momo_Language_PosContextData *first = *first_ptr;
    Momo_Language_PosContextData *last = *last_ptr;
    Momo_Language_PosContextData *func = push_array_zero(arena, Momo_Language_PosContextData, 1);
    func->relevant_note = note;
    func->query_token = query;
    func->argument_index = arg_index;
    if(last == 0)
    {
        first = last = func;
    }
    else
    {
        last->next = func;
        last = last->next;
    }
    *first_ptr = first;
    *last_ptr = last;
}

internal void
Momo_Language_PosContext_PushDataCall(Arena *arena,
                                     Momo_Language_PosContextData **first_ptr,
                                     Momo_Language_PosContextData **last_ptr,
                                     String_Const_u8 string, int param_idx)
{
    Momo_Language_PosContext_PushData(arena, first_ptr, last_ptr, Momo_Index_LookupNote(string, 0), 0, param_idx);
}

internal void
Momo_Language_PosContext_PushDataDot(Arena *arena,
                                    Momo_Language_PosContextData **first_ptr,
                                    Momo_Language_PosContextData **last_ptr,
                                    String_Const_u8 string, Token *query)
{
    Momo_Language_PosContext_PushData(arena, first_ptr, last_ptr, Momo_Index_LookupNote(string, 0), query, 0);
}

internal Token_List
Momo_Language_LexFullInput_NoBreaks(Application_Links *app, Momo_Language *language, Arena *arena, String_Const_u8 text)
{
    Token_List list = {};
    if(language != 0)
    {
        Scratch_Block scratch(app, arena);
        void *state = push_array_zero(scratch, u8, language->lex_state_size);
		language->LexInit(state, text);
        language->LexFullInput(arena, &list, state, max_u64);
    }
    return list;
}


// NOTE(rjf): Include language files here.
#include "generated/4coder_momo_lexer_cs.h"
#include "generated/4coder_momo_lexer_cs.cpp"
#include "4coder_momo_lang_cpp.cpp"
#include "4coder_momo_lang_cs.cpp"


// NOTE(rjf): @Momo_Language_RegisterAll Register languages.
function void
Momo_Language_RegisterAll(void)
{
    // NOTE(Momo): C/C++
    {
        String_Const_u8 extensions[] =
        {
            S8Lit("cpp"), S8Lit("cc"), S8Lit("c"), S8Lit("cxx"),
            S8Lit("C"), S8Lit("h"), S8Lit("hpp"),
        };
        for(int i = 0; i < ArrayCount(extensions); i += 1)
        {
            Momo_Language_Register(extensions[i],
                                momo_cpp_index_file,
                                lex_full_input_cpp_init,
                                lex_full_input_cpp_breaks,
                                momo_cpp_poscontext,
                                momo_cpp_highlight,
                                Lex_State_Cpp);
        }
    }

    // NOTE(Momo): C#
    {
        Momo_Language_Register(S8Lit("cs"),
                            Momo_CS_Index_File,
                            lex_full_input_cs_init,
                            lex_full_input_cs_breaks,
                            Momo_CS_PosContext,
                            Momo_CS_Highlight,
                            Lex_State_Cs);
    
    }
    
#if 0
    // NOTE(rjf): Jai
    {
        Momo_Language_Register(S8Lit("jai"),
                            Momo_Jai_IndexFile,
                            lex_full_input_jai_init,
                            lex_full_input_jai_breaks,
                            Momo_Jai_PosContext,
                            Momo_Jai_Highlight,
                            Lex_State_Jai);
    }
#endif
}