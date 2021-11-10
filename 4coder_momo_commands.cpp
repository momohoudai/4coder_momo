#ifndef FCODER_MOMO_COMMANDS
#define FCODER_MOMO_COMMANDS






//~ Commands
CUSTOM_COMMAND_SIG(momo_interactive_open_or_new)
CUSTOM_DOC("Interactively open a file out of the file system.") 
{
    for (;;){
        Scratch_Block scratch(app);
        View_ID view = get_this_ctx_view(app, Access_Always);
        File_Name_Result result = Momo_Lister_CreateToGetFilenameFromUser(app, scratch, "Open:", view);
        if (result.canceled) break;
        
        String_Const_u8 file_name = result.file_name_activated;
        if (file_name.size == 0){
            file_name = result.file_name_in_text_field;
        }
        if (file_name.size == 0) break;
        
        String_Const_u8 path = result.path_in_text_field;
        String_Const_u8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s",
                                                         string_expand(path), string_expand(file_name));
        
        if (result.is_folder){
            set_hot_directory(app, full_file_name);
            continue;
        }
        
        if (character_is_slash(file_name.str[file_name.size - 1])){
            File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
            if (HasFlag(attribs.flags, FileAttribute_IsDirectory)){
                set_hot_directory(app, full_file_name);
                continue;
			}
			if (string_looks_like_drive_letter(file_name)){
				set_hot_directory(app, file_name);
				continue;
			}
            if (query_create_folder(app, file_name)){
                set_hot_directory(app, full_file_name);
                continue;
            }
            break;
        }
        
        Buffer_ID buffer = create_buffer(app, full_file_name, 0);
        if (buffer != 0){
            view_set_buffer(app, view, buffer, 0);
        }
        break;
    }
}


CUSTOM_COMMAND_SIG(momo_indent_whole_file)
CUSTOM_DOC("Audo-indents the entire current buffer.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 buffer_size = buffer_get_size(app, buffer);
    Momo_Indent_IndentBuffer(app, buffer, Ii64(0, buffer_size));
}

CUSTOM_COMMAND_SIG(momo_indent_line_at_cursor)
CUSTOM_DOC("Auto-indents the line on which the cursor sits.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Momo_Indent_IndentBuffer(app, buffer, Ii64(pos));
    move_past_lead_whitespace(app, view, buffer);
}

CUSTOM_COMMAND_SIG(momo_indent_range)
CUSTOM_DOC("Auto-indents the range between the cursor and the mark.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_view_range(app, view);
    Momo_Indent_IndentBuffer(app, buffer, range);
    move_past_lead_whitespace(app, view, buffer);
}



CUSTOM_COMMAND_SIG(momo_write_text_and_indent)
CUSTOM_DOC("Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.")
{
    if (!global_insert_mode) {
        return;
    }
    ProfileScope(app, "write and auto indent");
    User_Input in = get_current_input(app);
    String_Const_u8 insert = to_writable(&in);
    Momo_Indent_WriteTextAndIndent(app, insert);
}




CUSTOM_COMMAND_SIG(momo_toggle_compilation_expand)
CUSTOM_DOC("Expand the compilation window.")
{
    Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    if(global_compilation_view_expanded ^= 1)
    {
        view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*32.f));
    }
    else
    {
        view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*4.f));
    }
}



CUSTOM_COMMAND_SIG(momo_home)
CUSTOM_DOC("Goes to the beginning of the line.")
{
    seek_pos_of_visual_line(app, Side_Min);
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target.pixel_shift.x = 0;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
}

CUSTOM_UI_COMMAND_SIG(momo_search_for_definition__project_wide)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    char *query = "Index (Project):";
    
    Scratch_Block scratch(app);
    Momo_Lister_Block lister(app, scratch);
    Momo_Lister_SetQuery(lister, query);
    Momo_Lister_SetDefaultHandlers(lister);
    
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
                    Momo_Lister_PushNote(app, scratch, lister, note);
                }
            }
        }
    }
    Momo_Index_Unlock();
    
    Momo_Lister_Result l_result = Momo_Lister_Run(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        point_stack_push_view_cursor(app, view);
        Momo_JumpToLocation(app, view, result.buffer, result.pos);
    }
}


function Custom_Command_Function*
Momo_GetCommandFromUser(Application_Links *app, String_Const_u8 query, i32 *command_ids, i32 command_id_count, Command_Lister_Status_Rule *status_rule){
    if (command_ids == 0){
        command_id_count = command_one_past_last_id;
    }
    
    Scratch_Block scratch(app);
    Momo_Lister_Block lister(app, scratch);
    Momo_Lister_SetQuery(lister, query);
    Momo_Lister_SetDefaultHandlers(lister);
    
    for (i32 i = 0; i < command_id_count; i += 1){
        i32 j = i;
        if (command_ids != 0){
            j = command_ids[i];
        }
        j = clamp(0, j, command_one_past_last_id);
        
        Custom_Command_Function *proc = fcoder_metacmd_table[j].proc;
        String_Const_u8 status = {};
        switch (status_rule->mode){
            case CommandLister_Descriptions:
            {
                status = SCu8(fcoder_metacmd_table[j].description);
            }break;
            case CommandLister_Bindings:
            {
                Command_Trigger_List triggers = map_get_triggers_recursive(scratch, status_rule->mapping, status_rule->map_id, proc);
                
                List_String_Const_u8 list = {};
                for (Command_Trigger *node = triggers.first;
                     node != 0;
                     node = node->next){
                    command_trigger_stringize(scratch, &list, node);
                    if (node->next != 0){
                        string_list_push(scratch, &list, string_u8_litexpr(" "));
                    }
                }
                
                status = string_list_flatten(scratch, list);
            }break;
        }
        
        Momo_Lister_AddItem(lister, SCu8(fcoder_metacmd_table[j].name), status,
                        (void*)proc, 0);
    }
    
    Momo_Lister_Result l_result = Momo_Lister_Run(app, lister);
    
    Custom_Command_Function *result = 0;
    if (!l_result.canceled){
        result = (Custom_Command_Function*)l_result.user_data;
    }
    return(result);
}

function Custom_Command_Function*
Momo_GetCommandFromUser(Application_Links *app, String_Const_u8 query, Command_Lister_Status_Rule *status_rule){
    return(Momo_GetCommandFromUser(app, query, 0, 0, status_rule));
}

function Custom_Command_Function*
Momo_GetCommandFromUser(Application_Links *app, char *query,
                      i32 *command_ids, i32 command_id_count, Command_Lister_Status_Rule *status_rule){
    return(Momo_GetCommandFromUser(app, SCu8(query), command_ids, command_id_count, status_rule));
}

function Custom_Command_Function*
Momo_GetCommandFromUser(Application_Links *app, char *query, Command_Lister_Status_Rule *status_rule){
    return(Momo_GetCommandFromUser(app, SCu8(query), 0, 0, status_rule));
}

CUSTOM_UI_COMMAND_SIG(momo_command_lister)
CUSTOM_DOC("Opens an interactive list of all registered commands.")
{
    View_ID view = get_this_ctx_view(app, Access_Always);
    if (view != 0){
        Command_Lister_Status_Rule rule = {};
        Buffer_ID buffer = view_get_buffer(app, view, Access_Visible);
        Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer);
        Command_Map_ID *map_id_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
        if (map_id_ptr != 0){
            rule = command_lister_status_bindings(&framework_mapping, *map_id_ptr);
        }
        else{
            rule = command_lister_status_descriptions();
        }
        Custom_Command_Function *func = Momo_GetCommandFromUser(app, "Command:", &rule);
        if (func != 0){
            view_enqueue_command_function(app, view, func);
        }
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
    momo_indent_whole_file(app);
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
    momo_interactive_open_or_new(app);
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


CUSTOM_COMMAND_SIG(momo_append_line_and_enter_insert)
CUSTOM_DOC("Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.")
{
    seek_end_of_textual_line(app);
    //Momo_Indent_WriteTextAndIndent(app, string_u8_litexpr("\n"));

    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 pos = {};
    pos.min = view_get_cursor_pos(app, view);
    write_text(app, string_u8_litexpr("\n"));
    pos.max = view_get_cursor_pos(app, view);
    Momo_Indent_IndentBuffer(app, buffer, pos, 0);
    move_past_lead_whitespace(app, view, buffer);

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

// NOTE(Momo): Yeah this is pretty dumb, but hey
CUSTOM_COMMAND_SIG(momo_number_mode_1) 
CUSTOM_DOC("Number Mode 1") { Momo_EnterNumberMode(app, string_u8_litexpr("1")); }
CUSTOM_COMMAND_SIG(momo_number_mode_2) 
CUSTOM_DOC("Number Mode 2") { Momo_EnterNumberMode(app, string_u8_litexpr("2")); }
CUSTOM_COMMAND_SIG(momo_number_mode_3) 
CUSTOM_DOC("Number Mode 3") { Momo_EnterNumberMode(app, string_u8_litexpr("3")); }
CUSTOM_COMMAND_SIG(momo_number_mode_4) 
CUSTOM_DOC("Number Mode 4") { Momo_EnterNumberMode(app, string_u8_litexpr("4")); }
CUSTOM_COMMAND_SIG(momo_number_mode_5) 
CUSTOM_DOC("Number Mode 5") { Momo_EnterNumberMode(app, string_u8_litexpr("5")); }
CUSTOM_COMMAND_SIG(momo_number_mode_6) 
CUSTOM_DOC("Number Mode 6") { Momo_EnterNumberMode(app, string_u8_litexpr("6")); }
CUSTOM_COMMAND_SIG(momo_number_mode_7) 
CUSTOM_DOC("Number Mode 7") { Momo_EnterNumberMode(app, string_u8_litexpr("7")); }
CUSTOM_COMMAND_SIG(momo_number_mode_8) 
CUSTOM_DOC("Number Mode 8") { Momo_EnterNumberMode(app, string_u8_litexpr("8")); }
CUSTOM_COMMAND_SIG(momo_number_mode_9) 
CUSTOM_DOC("Number Mode 9") { Momo_EnterNumberMode(app, string_u8_litexpr("9")); }


CUSTOM_UI_COMMAND_SIG(momo_search_for_definition_under_cursor_project_wide)
CUSTOM_DOC("List all definitions in the index and enter the token under the cursor")
{
    Scratch_Block scratch(app);
    String_Const_u8 string = push_token_or_word_under_active_cursor(app, scratch);
    Momo_Index_Note* note = Momo_Index_LookupNote(string);
    note = Momo_Index_FindFirstNonPrototypeNote(note);
    Momo_Index_GoToDefinitionInNote(app, note, 1);  
}

CUSTOM_UI_COMMAND_SIG(momo_search_for_definition_under_cursor_project_wide_other_panel)
CUSTOM_DOC("List all definitions in the index and enter the token under the cursor")
{
    Scratch_Block scratch(app);
    String_Const_u8 string = push_token_or_word_under_active_cursor(app, scratch);
    Momo_Index_Note* note = Momo_Index_LookupNote(string);
    note = Momo_Index_FindFirstNonPrototypeNote(note);
    Momo_Index_GoToDefinitionInNote(app, note, 0);  
}



CUSTOM_UI_COMMAND_SIG(momo_list_notes_project_wide)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    String_Const_u8 str = {};
    Momo_Lister_CreateWithProjectNotes(app, str, 1);
}

CUSTOM_UI_COMMAND_SIG(momo_list_notes_project_wide_other_panel)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    String_Const_u8 str = {};
    Momo_Lister_CreateWithProjectNotes(app, str, 0);
}

CUSTOM_COMMAND_SIG(snipe_forward_whitespace_and_token_boundary)
CUSTOM_DOC("Delete a single, whole token on or to the right of the cursor and post it to the clipboard.")
{
    Scratch_Block scratch(app);
    current_view_snipe_delete(app, Scan_Forward, push_boundary_list(scratch, Momo_BoundaryTokenAndWhiteSpace));

}

CUSTOM_COMMAND_SIG(momo_change_mode)
CUSTOM_DOC("Change mode")
{
    QueryLock;

    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = string_u8_litexpr("Change mode\n");
        if (!start_query_bar(app, &bar, 0)) {
            return;
        }

        User_Input in = {};
        for (;;) {
            in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape|EventProperty_MouseButton);
            if (in.abort) {
                return;
            }

            else if (match_key_code(&in, KeyCode_I)) {
                for (;;) {
                    in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape | EventProperty_MouseButton);
                    if (in.abort) {
                        return;
                    }

                    // ciw
                    if (match_key_code(&in, KeyCode_W)) {
                        snipe_forward_whitespace_or_token_boundary(app);
                        momo_switch_to_insert_mode(app);
                        return;
                    }
                }
            }


            else if (match_key_code(&in, KeyCode_A)) {
                for (;;) {
                    in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape | EventProperty_MouseButton);
                    if (in.abort) {
                        return;
                    }

                    // caw
                    if (match_key_code(&in, KeyCode_W)) {
                        snipe_forward_whitespace_and_token_boundary(app);
                        momo_switch_to_insert_mode(app);
                        return;
                    }
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
        start_query_bar(app, &bar, 0);

        User_Input in = {};
        for (;;) {
            in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape|EventProperty_MouseButton);
            if (in.abort) {
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
                momo_toggle_compilation_expand(app);
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
         
            Momo_SeekStringForward(app, buffer, -1, search_from, 0, if0_str, &if0_pos);
            Momo_SeekStringForward(app, buffer, -1, search_from, 0, endif_str, &endif_pos);

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
            Momo_SeekStringBackward(app, buffer, -1, search_from, 0, if0_str, &if0_pos);
            Momo_SeekStringBackward(app, buffer, -1, search_from, 0, endif_str, &endif_pos);
           
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
    QueryLock;

    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = string_u8_litexpr("Goto mode!\n");
        start_query_bar(app, &bar, 0);

        User_Input in = {};
        for (;;) {
            in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape|EventProperty_MouseButton);
            if (in.abort) {
                break;
            }

            if (match_key_code(&in, KeyCode_D)) {
                Input_Modifier_Set* mods = &in.event.key.modifiers;
                if (has_modifier(mods, KeyCode_Shift)) {
                    momo_search_for_definition_under_cursor_project_wide(app);
                }
                else {
                    momo_search_for_definition_under_cursor_project_wide_other_panel(app);
                    
                }
                break;
            }
            else if (match_key_code(&in, KeyCode_G)) {
                Input_Modifier_Set* mods = &in.event.key.modifiers;
                if (has_modifier(mods, KeyCode_Shift)) {
                    momo_list_notes_project_wide(app);
                }
                else {
                    momo_list_notes_project_wide_other_panel(app);
                    
                }
                break;
            }

        }
    }
}


CUSTOM_COMMAND_SIG(momo_reset_view_x)
CUSTOM_DOC("Reset the view to x = 0")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
    Vec2_f32 p = view_relative_xy_of_pos(app, view, cursor.line, pos);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target.pixel_shift.x = 0.f;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
    no_mark_snap_to_cursor(app, view);
}


CUSTOM_COMMAND_SIG(momo_z_mode)
CUSTOM_DOC("Z mode")
{
    QueryLock;


    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (buffer != 0) {
        Query_Bar_Group group(app);
        Query_Bar bar = {};
        bar.prompt = string_u8_litexpr("Z mode!\n");
        start_query_bar(app, &bar, 0);

        User_Input in = {};
        for (;;) {
            in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape|EventProperty_MouseButton);
            if (in.abort) {
                break;
            }

            if (match_key_code(&in, KeyCode_Z)) {
                center_view(app);
                break;
            }

        }
    }
}

CUSTOM_COMMAND_SIG(momo_query_search)
CUSTOM_DOC("Queries the user a string, and can do reverse and forward search with 'n' and 'N'")
{
    QueryLock;

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
                start_query_bar(app, &bar, 0);

                i64 pos = view_get_cursor_pos(app, view);
                //query_replace_parameter(app, find.string, pos, false);
                i64 new_pos = 0;
                seek_string_forward(app, buffer, pos - 1, 0, find.string, &new_pos);
                User_Input in = {};
                for (;;) {
                    Range_i64 match = Ii64(new_pos, new_pos + find.string.size);
                    isearch__update_highlight(app, view, match);

                    in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape|EventProperty_MouseButton);
                    if (in.abort) {
                        break;
                    }

                    if (match_key_code(&in, KeyCode_N)) {
                        Input_Modifier_Set* mods = &in.event.key.modifiers;
                        if (has_modifier(mods, KeyCode_Shift)) {
                            Momo_SeekStringBackward(app, buffer, match.start, match.start - 1, 0, find.string, &new_pos);
                        }
                        else {
                            Momo_SeekStringForward(app, buffer, match.start, match.end, 0, find.string, &new_pos);
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

CUSTOM_COMMAND_SIG(momo_query_replace)
CUSTOM_DOC("Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.")
{
    QueryLock;

    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if (buffer != 0){
        Query_Bar_Group group(app);
        Query_Bar replace = {};
        u8 replace_space[1024];
        replace.prompt = string_u8_litexpr("Replace: ");
        replace.string = SCu8(replace_space, (u64)0);
        replace.string_capacity = sizeof(replace_space);
        if (query_user_string(app, &replace)){
            if (replace.string.size > 0){
                i64 pos = view_get_cursor_pos(app, view);
                query_replace_parameter(app, replace.string, pos, false);
            }
        }
    }
}

CUSTOM_COMMAND_SIG(momo_replace_in_range)
CUSTOM_DOC("Queries the user for a needle and string. Replaces all occurences of needle with string in the range between cursor and the mark in the active buffer.")
{
    QueryLock;

    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_view_range(app, view);
    replace_in_range_query_user(app, buffer, range);
}

CUSTOM_COMMAND_SIG(momo_leave_event_unhandled)
CUSTOM_DOC("when bound to keystroke, ensures the event falls through to text insertion")
{
    leave_current_input_unhandled(app);
}

CUSTOM_COMMAND_SIG(momo_backspace_token_boundary)
CUSTOM_DOC("Deletes left to a token boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Backward, push_boundary_list(scratch, Momo_BoundaryTokenAndWhiteSpace));
}

CUSTOM_COMMAND_SIG(momo_delete_token_boundary)
CUSTOM_DOC("Deletes right to a token boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Forward, push_boundary_list(scratch, Momo_BoundaryTokenAndWhiteSpace));
}

CUSTOM_COMMAND_SIG(momo_backspace_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Deletes left to a alphanumeric or camel boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Backward, push_boundary_list(scratch,
                                                                        boundary_alpha_numeric,
                                                                        boundary_alpha_numeric_camel));
}

CUSTOM_COMMAND_SIG(momo_delete_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Deletes right to an alphanumeric or camel boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Forward, push_boundary_list(scratch,
                                                                       boundary_alpha_numeric,
                                                                       boundary_alpha_numeric_camel));
}

CUSTOM_COMMAND_SIG(momo_insert_tab)
CUSTOM_DOC("Inserts tab at cursor")
{
    write_text(app, string_u8_litexpr("\t"));
}

CUSTOM_COMMAND_SIG(momo_indent_lines_right)
CUSTOM_DOC("Shift lines to the left by indent width")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Range_i64 range = get_view_range(app, view);
    Momo_Indent_IndentLinesRight(app, buffer, range, 0);

}


#endif