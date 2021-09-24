
static b32 global_insert_mode = false;
static b32 global_query_lock = false;

#define QueryLock \
if (global_query_lock) { \
    momo_leave_event_unhandled(app); \
    return; \
} \
global_query_lock = true; \
Defer { global_query_lock = false; }; 


function void
Momo_SeekStringForward(Application_Links* app, Buffer_ID buffer, i64 fallback_pos, i64 pos, i64 end, String_Const_u8 needle, i64* result) {
    if (end == 0) {
        end = (i32)buffer_get_size(app, buffer);
    }
    String_Match match = {};
    match.range.first = pos;
    for (;;) {
        match = buffer_seek_string(app, buffer, needle, Scan_Forward, (i32)match.range.first);
        if (HasFlag(match.flags, StringMatch_CaseSensitive) ||
            match.buffer != buffer || match.range.first >= end) break;
    }
    if (match.range.first < end && match.buffer == buffer) {
        *result = match.range.first;
    }
    else {
        // Instead of returning end of buffer size, return pos
        //*result = buffer_get_size(app, buffer);
        *result = fallback_pos;
    }
}

function void
Momo_SeekStringBackward(Application_Links* app, Buffer_ID buffer, i64 fallback_pos, i64 pos, i64 min, String_Const_u8 needle, i64* result) {
    String_Match match = {};
    match.range.first = pos;
    for (;;) {
        match = buffer_seek_string(app, buffer, needle, Scan_Backward, (i32)match.range.first);
        if (HasFlag(match.flags, StringMatch_CaseSensitive) ||
            match.buffer != buffer || match.range.first < min) break;
    }
    if (match.range.first >= min && match.buffer == buffer) {
        *result = match.range.first;
    }
    else {
        *result = fallback_pos;
    }
}


function i64
Momo_BoundaryWord(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
    i64 result = 0;
    if (direction == Scan_Forward) {
        result = Min(buffer_seek_character_class_change_0_1(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos),
            buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos));
        result = Min(result, buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, direction, pos));
    }
    else {
        result = Max(buffer_seek_character_class_change_0_1(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos),
            buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos));
        result = Max(result, buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, direction, pos));
    }

    return result;
}




function void
Momo_EnterNumberMode(Application_Links* app, String_Const_u8 init_str) {
    QueryLock;

    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);

    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar find = {};
        u8 number_buffer[1024];
        find.prompt = string_u8_litexpr("Number: ");
        find.string = SCu8(number_buffer, (u64)0);
        find.string_capacity = sizeof(number_buffer);
        if (!start_query_bar(app, &find, 0)) {
            return;
        }



        // If the current input is a number, we can just append.
        if (init_str.size > 0)
        {
            Scratch_Block scratch(app);
            User_Input in = get_current_input(app);
            init_str = string_replace(scratch, init_str,
                                        string_u8_litexpr("\n"),
                                        string_u8_litexpr(""));
            init_str = string_replace(scratch, init_str,
                                        string_u8_litexpr("\t"),
                                        string_u8_litexpr(""));
            if (string_is_integer(init_str, 10)){
                String_u8 string = Su8(find.string.str, find.string.size, find.string_capacity);
                string_append(&string, init_str);
                find.string.size = string.string.size;
            }
            

        }


        for (;;) {
            User_Input in = get_next_input(app, EventPropertyGroup_Any, 
                                EventProperty_Escape|EventProperty_MouseButton);
            if (in.abort) {
                break;
            }

            // for all other cases, attempt to insert
            Scratch_Block scratch(app);
            b32 good_insert = false;
            String_Const_u8 insert_string = to_writable(&in);
            if (insert_string.str != 0 && insert_string.size > 0){
                insert_string = string_replace(scratch, insert_string,
                                            string_u8_litexpr("\n"),
                                            string_u8_litexpr(""));
                insert_string = string_replace(scratch, insert_string,
                                                        string_u8_litexpr("\t"),
                                                        string_u8_litexpr(""));
                if (string_is_integer(insert_string, 10)){
                    good_insert = true;
                }
            }

            // Shift-G will go to line
            if (match_key_code(&in, KeyCode_G)) {
                Input_Modifier_Set* mods = &in.event.key.modifiers;
                if (has_modifier(mods, KeyCode_Shift)) {
                    i32 line_number = (i32)string_to_integer(find.string, 10);
                    View_ID view = get_active_view(app, Access_ReadVisible);
                    view_set_cursor_and_preferred_x(app, view, seek_line_col(line_number, 0));
                    break;
                }
                
            }

            if (in.event.kind == InputEventKind_KeyStroke &&
                (in.event.key.code == KeyCode_Return || in.event.key.code == KeyCode_Tab))
            {
                break;
            }
            else if (in.event.kind == InputEventKind_KeyStroke &&
                    in.event.key.code == KeyCode_Backspace){
                find.string = backspace_utf8(find.string);
            }
            else if (good_insert){
                String_u8 string = Su8(find.string.str, find.string.size, find.string_capacity);
                string_append(&string, insert_string);
                find.string.size = string.string.size;
            }
            else{
                leave_current_input_unhandled(app);
            }
        }
    }
}

internal void
Momo_JumpToLocation(Application_Links *app, View_ID view, Buffer_ID buffer, i64 pos)
{
    // NOTE(rjf): This function was ripped from 4coder's jump_to_location. It was copied
    // and modified so that jumping to a location didn't cause a selection in notepad-like
    // mode.
    
    view_set_active(app, view);
    Buffer_Seek seek = seek_pos(pos);
    set_view_to_location(app, view, buffer, seek);
    
    if (auto_center_after_jumps)
    {
        center_view(app);
    }
    view_set_cursor(app, view, seek);
    view_set_mark(app, view, seek);
}



function i64
Momo_BoundaryTokenAndWhiteSpace(Application_Links *app, Buffer_ID buffer, 
                               Side side, Scan_Direction direction, i64 pos)
{
    i64 result = boundary_non_whitespace(app, buffer, side, direction, pos);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if (tokens.tokens != 0){
        switch (direction){
            case Scan_Forward:
            {
                i64 buffer_size = buffer_get_size(app, buffer);
                result = buffer_size;
                if(tokens.count > 0)
                {
                    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
                    Token *token = token_it_read(&it);
                    
                    if(token == 0)
                    {
                        break;
                    }
                    
                    // NOTE(rjf): Comments/Strings
                    if(token->kind == TokenBaseKind_Comment ||
                       token->kind == TokenBaseKind_LiteralString)
                    {
                        result = boundary_non_whitespace(app, buffer, side, direction, pos);
                        break;
                    }
                    
                    // NOTE(rjf): All other cases.
                    else
                    {
                        if (token->kind == TokenBaseKind_Whitespace)
                        {
                            // token_it_inc_non_whitespace(&it);
                            // token = token_it_read(&it);
                        }
                        
                        if (side == Side_Max){
                            result = token->pos + token->size;
                            
                            token_it_inc_all(&it);
                            Token *ws = token_it_read(&it);
                            if(ws != 0 && ws->kind == TokenBaseKind_Whitespace &&
                               get_line_number_from_pos(app, buffer, ws->pos + ws->size) ==
                               get_line_number_from_pos(app, buffer, token->pos))
                            {
                                result = ws->pos + ws->size;
                            }
                        }
                        else{
                            if (token->pos <= pos){
                                token_it_inc_non_whitespace(&it);
                                token = token_it_read(&it);
                            }
                            if (token != 0){
                                result = token->pos;
                            }
                        }
                    }
                    
                }
            }break;
            
            case Scan_Backward:
            {
                result = 0;
                if (tokens.count > 0){
                    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
                    Token *token = token_it_read(&it);
                    
                    Token_Iterator_Array it2 = it;
                    token_it_dec_non_whitespace(&it2);
                    Token *token2 = token_it_read(&it2);
                    
                    // NOTE(rjf): Comments/Strings
                    if(token->kind == TokenBaseKind_Comment ||
                       token->kind == TokenBaseKind_LiteralString ||
                       (token2 && 
                        token2->kind == TokenBaseKind_Comment ||
                        token2->kind == TokenBaseKind_LiteralString))
                    {
                        result = boundary_non_whitespace(app, buffer, side, direction, pos);
                        break;
                    }
                    
                    if (token->kind == TokenBaseKind_Whitespace){
                        token_it_dec_non_whitespace(&it);
                        token = token_it_read(&it);
                    }
                    if (token != 0){
                        if (side == Side_Min){
                            if (token->pos >= pos){
                                token_it_dec_non_whitespace(&it);
                                token = token_it_read(&it);
                            }
                            result = token->pos;
                        }
                        else{
                            if (token->pos + token->size >= pos){
                                token_it_dec_non_whitespace(&it);
                                token = token_it_read(&it);
                            }
                            result = token->pos + token->size;
                        }
                    }
                }
            }break;
        }
    }
    return(result);
}