// NOTE(Momo): Collection of unorganized stuff


#define AnonVarSub(x) zawarudo_ryojianon##x
#define AnonVar(x) AnonVarSub(x)
#define Defer auto AnonVar(__COUNTER__) = zawarudo::defer_dummy{} + [&]()


static b32 global_insert_mode = false;
static b32 global_query_mode = false;

#define QueryModeLock \
if (global_query_mode) {\
    leave_current_input_unhandled(app);\
    return; \
}\
global_query_mode = true; \
Defer{ global_query_mode = false; }


function void
momo_write_text_and_auto_indent_internal(Application_Links* app, String_Const_u8 insert) {
    ProfileScope(app, "write and auto indent");
    if (insert.str != 0 && insert.size > 0) {
        b32 do_auto_indent = false;
        for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1) {
            switch (insert.str[i]) {
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
        if (do_auto_indent) {
            View_ID view = get_active_view(app, Access_ReadWriteVisible);
            Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
            Range_i64 pos = {};
            pos.min = view_get_cursor_pos(app, view);
            write_text(app, insert);
            pos.max = view_get_cursor_pos(app, view);
            auto_indent_buffer(app, buffer, pos, 0);
            move_past_lead_whitespace(app, view, buffer);
        }
        else {
            write_text(app, insert);
        }
    }
}

function i64
momo_boundary_word(Application_Links* app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos) {
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
momo_seek_string_forward(Application_Links* app, Buffer_ID buffer, i64 fallback_pos, i64 pos, i64 end, String_Const_u8 needle, i64* result) {
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
momo_seek_string_backward(Application_Links* app, Buffer_ID buffer, i64 fallback_pos, i64 pos, i64 min, String_Const_u8 needle, i64* result) {
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


function void
momo_number_mode(Application_Links* app, String_Const_u8 init_str) {
    QueryModeLock;

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


// NOTE(Momo): list project definitions
// TODO(Momo): make it smarter? If there is only one definition, just go to that one.
function void
momo_list_project_definitions(Application_Links* app, String_Const_u8 init_str, b32 same_panel){
    char *query = "Index (Project):";
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);
    lister_set_key(lister, init_str);
    lister_set_text_field(lister, init_str);
    lister_set_default_handlers(lister);
    
    Momo_Index_Lock();
    {
        for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
             buffer != 0; buffer = get_buffer_next(app, buffer, Access_Always))
        {
            Momo_Index_File *file = Momo_Index_LookupFile(app, buffer);
            if(file != 0)
            {
                for(Momo_Index_Note *note = file->first_note; note; note = note->next_sibling)
                {
                    _Momo_PushListerOptionForNote(app, scratch, lister, note);
                }
            }
        }
    }
    Momo_Index_Unlock();
    
    Lister_Result l_result = run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        if (!same_panel) {
            view = get_next_view_looped_primary_panels(app, view, Access_Always);
        }
        point_stack_push_view_cursor(app, view);
        Momo_JumpToLocation(app, view, result.buffer, result.pos);
        
     
    }
}