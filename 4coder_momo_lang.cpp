/* date = January 29th 2021 9:37 pm */

#ifndef FCODER_MOMO_LANG_LIST_H
#define FCODER_MOMO_LANG_LIST_H

// NOTE(rjf): Include language files here.
#include "generated/4coder_fleury_lexer_jai.h"
#include "generated/4coder_fleury_lexer_jai.cpp"
#include "4coder_fleury_lang_cpp.cpp"
#include "4coder_fleury_lang_jai.cpp"
#include "4coder_fleury_lang_metadesk.cpp"

// NOTE(rjf): @momo_register_languages Register languages.
function void
Momo_RegisterLanguages(void)
{
    // NOTE(rjf): C/C++
    {
        String_Const_u8 extensions[] =
        {
            S8Lit("cpp"), S8Lit("cc"), S8Lit("c"), S8Lit("cxx"),
            S8Lit("C"), S8Lit("h"), S8Lit("hpp"),
        };
        for(int i = 0; i < ArrayCount(extensions); i += 1)
        {
            Momo_RegisterLanguage(extensions[i],
                                Momo_CPP_IndexFile,
                                lex_full_input_cpp_init,
                                lex_full_input_cpp_breaks,
                                Momo_CPP_PosContext,
                                Momo_CPP_Highlight,
                                Lex_State_Cpp);
        }
    }
    
    // NOTE(rjf): Jai
    {
        Momo_RegisterLanguage(S8Lit("jai"),
                            Momo_Jai_IndexFile,
                            lex_full_input_jai_init,
                            lex_full_input_jai_breaks,
                            Momo_Jai_PosContext,
                            Momo_Jai_Highlight,
                            Lex_State_Jai);
    }
}

#endif 
