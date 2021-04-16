#ifndef FCODER_FLEURY_MOMO
#define FCODER_FLEURY_MOMO


// TODO(Momo):
// - #if 0 and #endif block insert and removal
static b32 global_insert_mode = false;



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

CUSTOM_COMMAND_SIG(page_up_half)
CUSTOM_DOC("Page up halfway")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    f32 page_jump = get_page_jump(app, view) * 0.5f;
    move_vertical_pixels(app, -page_jump);
}

CUSTOM_COMMAND_SIG(page_down_half)
CUSTOM_DOC("Page down halfway")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    f32 page_jump = get_page_jump(app, view) * 0.5f;
    move_vertical_pixels(app, page_jump);
}

CUSTOM_COMMAND_SIG(momo_switch_to_edit_mode)
CUSTOM_DOC("EDIT MODE ACTIVATED")
{
    switch_to_keybinding_0(app);
    global_insert_mode = false;
}

CUSTOM_COMMAND_SIG(momo_switch_to_insert_mode)
CUSTOM_DOC("INSERT MODE ACTIVATED")
{
    switch_to_keybinding_1(app);
    global_insert_mode = true;
}

CUSTOM_COMMAND_SIG(w)
CUSTOM_DOC("Alias for saving")
{
    save(app);
}

CUSTOM_COMMAND_SIG(wa)
CUSTOM_DOC("Alias for saving all dirty buffers")
{
    save_all_dirty_buffers(app);
}

CUSTOM_COMMAND_SIG(E)
CUSTOM_DOC("Alias for opening files")
{
    interactive_open_or_new(app);
}

CUSTOM_COMMAND_SIG(q)
CUSTOM_DOC("Alias for closing panels. If no panels are left, exit")
{
    View_ID old_view = get_active_view(app, Access_ReadVisible);
    close_panel(app);
    View_ID cur_view = get_active_view(app, Access_ReadVisible);
    if (old_view == cur_view) {
        exit_4coder(app);
    }
}


CUSTOM_COMMAND_SIG(momo_write_text_input)
CUSTOM_DOC("Inserts whatever text was used to trigger this command.")
{
    if (global_insert_mode) {
        write_text_input(app);
    }
}

CUSTOM_COMMAND_SIG(momo_write_text_and_auto_indent)
CUSTOM_DOC("Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.")
{
    if (global_insert_mode) {
        write_text_and_auto_indent(app);
    }
}

CUSTOM_COMMAND_SIG(momo_append_line_and_enter_insert)
CUSTOM_DOC("Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.")
{
    seek_end_of_textual_line(app);
    momo_write_text_and_auto_indent_internal(app, string_u8_litexpr("\n"));
    momo_switch_to_insert_mode(app);
}

CUSTOM_COMMAND_SIG(momo_cut_line)
CUSTOM_DOC("Cut the text in the range from the cursor to the mark onto the clipboard.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 range = get_line_pos_range(app, buffer, line);
    range.end += 1;
    i32 size = (i32)buffer_get_size(app, buffer);
    range.end = clamp_top(range.end, size);
    if (range_size(range) == 0 ||
        buffer_get_char(app, buffer, range.end - 1) != '\n') {
        range.start -= 1;
        range.first = clamp_bot(0, range.first);
    }
    if (clipboard_post_buffer_range(app, 0, buffer, range)) {
        buffer_replace_range(app, buffer, range, string_u8_empty);
    }
}

CUSTOM_COMMAND_SIG(momo_teleport_to_line_and_center)
CUSTOM_DOC("Teleport to line number and center view")
{
    goto_line(app);
    center_view(app);
}

CUSTOM_COMMAND_SIG(momo_write_todo)
CUSTOM_DOC("Write todo and enter insert mode")
{
    write_todo(app);
    momo_switch_to_insert_mode(app);
}

CUSTOM_COMMAND_SIG(momo_write_note)
CUSTOM_DOC("Write note and enter insert mode")
{
    write_note(app);
    momo_switch_to_insert_mode(app);
}

CUSTOM_COMMAND_SIG(momo_open_panel_hsplit)
CUSTOM_DOC("Hsplit panel but don't go")
{
    View_ID view = get_active_view(app, Access_Always);
    View_ID new_view = open_view(app, view, ViewSplit_Bottom);
    new_view_settings(app, new_view);
}

CUSTOM_COMMAND_SIG(momo_open_panel_vsplit)
CUSTOM_DOC("Vsplit panel but don't go")
{
    View_ID view = get_active_view(app, Access_Always);
    View_ID new_view = open_view(app, view, ViewSplit_Right);
    new_view_settings(app, new_view);

}



CUSTOM_COMMAND_SIG(snipe_forward_whitespace_and_token_boundary)
CUSTOM_DOC("Delete a single, whole token on or to the right of the cursor and post it to the clipboard.")
{
    Scratch_Block scratch(app);
    current_view_snipe_delete(app, Scan_Forward,
        push_boundary_list(scratch, F4_Boundary_TokenAndWhitespace));

}

CUSTOM_COMMAND_SIG(momo_change_mode)
CUSTOM_DOC("Change mode")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = string_u8_litexpr("Change mode\n");
        start_query_bar(app, &bar, 1);

        User_Input in = {};
        for (;;) {
            in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
            if (in.abort || match_key_code(&in, KeyCode_Escape)) {
                break;
            }

            else if (match_key_code(&in, KeyCode_I)) {
                in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
                if (in.abort || match_key_code(&in, KeyCode_Escape)) {
                    break;
                }

                // ciw
                if (match_key_code(&in, KeyCode_W)) {
                    snipe_forward_whitespace_or_token_boundary(app);
                    momo_switch_to_insert_mode(app);
                    break;
                }
            }


            else if (match_key_code(&in, KeyCode_A)) {
                in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
                if (in.abort || match_key_code(&in, KeyCode_Escape)) {
                    break;
                }

                // caw
                if (match_key_code(&in, KeyCode_W)) {
                    snipe_forward_whitespace_and_token_boundary(app);
                    momo_switch_to_insert_mode(app);
                    break;
                }
            }

        }
    }
}

CUSTOM_COMMAND_SIG(momo_toggle_recording)
CUSTOM_DOC("Toggle recording")
{
    if (global_keyboard_macro_is_recording) {
        keyboard_macro_finish_recording(app);
    }
    else {
        keyboard_macro_start_recording(app);
    }
}

CUSTOM_COMMAND_SIG(momo_window_manip_mode)
CUSTOM_DOC("Window manipulation mode")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = string_u8_litexpr("Window action mode!\n");
        start_query_bar(app, &bar, 1);

        User_Input in = {};
        for (;;) {
            in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
            if (in.abort || match_key_code(&in, KeyCode_Escape)) {
                break;
            }

            if (match_key_code(&in, KeyCode_V)) {
                momo_open_panel_vsplit(app);
                break;
            }
            else if (match_key_code(&in, KeyCode_S)) {
                momo_open_panel_hsplit(app);
                break;
            }
            else if (match_key_code(&in, KeyCode_H)) {
                change_active_panel_backwards(app);
                break;
            }
            else if (match_key_code(&in, KeyCode_L)) {
                change_active_panel(app);
                break;
            }
            else if (match_key_code(&in, KeyCode_J)) {
                change_to_build_panel(app);
                break;
            }
            else if (match_key_code(&in, KeyCode_K)) {
                f4_toggle_compilation_expand(app);
                break;
            }


        }
    }
}

CUSTOM_COMMAND_SIG(momo_if0_on)
CUSTOM_DOC("Removes #if 0/#endif")
{
    // check the current line the cursor is in
    Scratch_Block scratch(app);

    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    i64 line = get_line_number_from_pos(app, buffer, pos);
    Range_i64 line_range = get_line_pos_range(app, buffer, line);
    line_range.end += 1;
    i32 size = (i32)buffer_get_size(app, buffer);
    line_range.end = clamp_top(line_range.end, size);
    if (range_size(line_range) == 0 ||
        buffer_get_char(app, buffer, line_range.end - 1) != '\n') {
        line_range.start -= 1;
        line_range.first = clamp_bot(0, line_range.first);
    }
    String_Const_u8 selected_str = push_buffer_range(app, scratch, buffer, line_range);
    selected_str = string_skip_chop_whitespace(selected_str);

    String_Const_u8 selected_str3 = selected_str;
    selected_str3.size = 3;

    String_Const_u8 selected_str6 = selected_str;
    selected_str6.size = 6;

    // TODO(Momo): It doesn't have to be explicitly #if 0, right?
    String_Const_u8 if0_str = string_u8_litexpr("#if");
    String_Const_u8 endif_str = string_u8_litexpr("#endif");
    if (string_match(selected_str3, if0_str)) {
        // Have to -1 here. Otherwise, we might accidentally skip 
        // two #ifs that are side by side.
        i64 search_from = line_range.end - 1;

        u32 if0_count = 1;
        u32 endif_count = 0; // remember to count current line

        for (;;) {
            // find the next #endif or #if0
            i64 endif_pos;
            i64 if0_pos;
         
            momo_seek_string_forward(app, buffer, -1, search_from, 0, if0_str, &if0_pos);
            momo_seek_string_forward(app, buffer, -1, search_from, 0, endif_str, &endif_pos);

            if (if0_pos != -1) {
                if (if0_pos < endif_pos) {
                    search_from = if0_pos;
                    ++if0_count;
                }
            }

            if (endif_pos != -1) {
                if (if0_pos > endif_pos || if0_pos == -1) {
                    search_from = endif_pos;
                    ++endif_count;
                }

                // The question now is whether this #if0 is ours?
                if (if0_count == endif_count) {
                    // replace the #if
                    buffer_replace_range(app, buffer, line_range, string_u8_empty);

                    // replace the #endif
                    i64 endif_line = get_line_number_from_pos(app, buffer, endif_pos);
                    Range_i64 endif_line_range = get_line_pos_range(app, buffer, endif_line);
                    buffer_replace_range(app, buffer, endif_line_range, string_u8_empty);
                    break;
                }

            }
            else {
                // Here, we ran out of #endif
                break;
            }

        }
    }
    else if (string_match(selected_str6, endif_str)) {
        i64 search_from = line_range.start;

        u32 if0_count = 0;
        u32 endif_count = 1; // remember to count current line
        for (;;) {
            // find the next #endif or #if0
            i64 endif_pos;
            i64 if0_pos;
            momo_seek_string_backward(app, buffer, -1, search_from, 0, if0_str, &if0_pos);
            momo_seek_string_backward(app, buffer, -1, search_from, 0, endif_str, &endif_pos);
           
            if (endif_pos != -1) {
                if (if0_pos < endif_pos) {
                    search_from = endif_pos;
                    ++endif_count;
                }
            }

            if (if0_pos != -1) {
                if (if0_pos > endif_pos || endif_pos == -1) {
                    search_from = if0_pos;
                    ++if0_count;
                }

                // The question now is whether this #if0 is ours?
                if (if0_count == endif_count) {
                    // replace the #endif
                    buffer_replace_range(app, buffer, line_range, string_u8_empty);

                    // replace the #if 
                    i64 if0_line = get_line_number_from_pos(app, buffer, if0_pos);
                    Range_i64 if0_line_range = get_line_pos_range(app, buffer, if0_line);
                    buffer_replace_range(app, buffer, if0_line_range, string_u8_empty);
                    break;
                }

            } 
            else {
                // Here, we ran out of #if0
                break;
            }

        }
    }
}


CUSTOM_COMMAND_SIG(momo_goto_mode)
CUSTOM_DOC("Goto mode")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = string_u8_litexpr("Goto mode!\n");
        start_query_bar(app, &bar, 1);

        User_Input in = {};
        for (;;) {
            in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
            if (in.abort || match_key_code(&in, KeyCode_Escape)) {
                break;
            }

            if (match_key_code(&in, KeyCode_D)) {
                Input_Modifier_Set* mods = &in.event.key.modifiers;
                if (has_modifier(mods, KeyCode_Shift)) {
                    f4_go_to_definition_same_panel(app);
                }
                else {
                    f4_go_to_definition(app);
                }
                break;
            }

        }
    }
}


CUSTOM_COMMAND_SIG(momo_query_search)
CUSTOM_DOC("Queries the user a string, and can do reverse and forward search with 'n' and 'N'")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar find = {};
        u8 find_buffer[1024];
        find.prompt = string_u8_litexpr("Search: ");
        find.string = SCu8(find_buffer, (u64)0);
        find.string_capacity = sizeof(find_buffer);

        if (query_user_string(app, &find)) {
            if (find.string.size > 0) {

                Query_Bar bar = {};
                bar.prompt = string_u8_litexpr("Navigate to: next(n), prev(N). Exit (Esc).\n");
                start_query_bar(app, &bar, 1);

                i64 pos = view_get_cursor_pos(app, view);
                //query_replace_parameter(app, find.string, pos, false);
                i64 new_pos = 0;
                seek_string_forward(app, buffer, pos - 1, 0, find.string, &new_pos);
                User_Input in = {};
                for (;;) {
                    Range_i64 match = Ii64(new_pos, new_pos + find.string.size);
                    isearch__update_highlight(app, view, match);

                    in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
                    if (in.abort || match_key_code(&in, KeyCode_Escape)) {
                        break;
                    }

                    if (match_key_code(&in, KeyCode_N)) {
                        Input_Modifier_Set* mods = &in.event.key.modifiers;
                        if (has_modifier(mods, KeyCode_Shift)) {
                            momo_seek_string_backward(app, buffer, match.start, match.start - 1, 0, find.string, &new_pos);
                        }
                        else {
                            momo_seek_string_forward(app, buffer, match.start, match.end, 0, find.string, &new_pos);
                        }
                    }

                }

                view_disable_highlight_range(app, view);
                if (in.abort) {
                    return;
                }
                view_set_cursor_and_preferred_x(app, view, seek_pos(new_pos));
            }
        }
    }
}

#endif