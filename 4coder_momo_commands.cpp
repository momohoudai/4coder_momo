#ifndef FCODER_MOMO_COMMANDS
#define FCODER_MOMO_COMMANDS



static b32 global_insert_mode = false;
static b32 global_query_mode = false;

#define QueryModeLock \
if (global_query_mode) {\
    leave_current_input_unhandled(app);\
    return; \
}\
global_query_mode = true; \
Defer{ global_query_mode = false; }

internal void
momo_go_to_definition(Application_Links *app, Momo_Index_Note *note, b32 same_panel)
{
    if(note != 0 && note->file != 0)
    {
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

internal void
momo_jump_to_location(Application_Links *app, View_ID view, Buffer_ID buffer, i64 pos)
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



internal void
momo_push_lister_with_note(Application_Links *app, Arena *arena, Momo_Lister *lister, Momo_Index_Note *note)
{
    if(note && note->file)
    {
        Momo_Index_File *file = note->file;
        Buffer_ID buffer = file->buffer;
        
        Tiny_Jump *jump = push_array(arena, Tiny_Jump, 1);
        jump->buffer = buffer;
        jump->pos = note->range.first;
        
        String_Const_u8 buffer_name = push_buffer_unique_name(app, arena, buffer);
        String_Const_u8 name = push_stringf(arena, "[%.*s] %.*s", string_expand(buffer_name), string_expand(note->string));
        String_Const_u8 sort = S8Lit("");
        switch(note->kind)
        {
            case MOMO_INDEX_NOTE_KIND_TYPE:
            {
                sort = push_stringf(arena, "type [%s] [%s]",
                                    note->flags & MOMO_INDEX_NOTE_FLAG_PROTOTYPE ? "prototype" : "def",
                                    note->flags & MOMO_INDEX_NOTE_FLAG_SUM_TYPE ? "sum" : "product");
            }break;
            
            case MOMO_INDEX_NOTE_KIND_FUNCTION:
            {
                if (note->additional_info.size > 0) {
                    sort = push_stringf(arena, "func [%s] %.*s", 
                                        note->flags & MOMO_INDEX_NOTE_FLAG_PROTOTYPE ? "prototype" : "def",
                                        string_expand(note->additional_info));
                }
                else {
                    sort = push_stringf(arena, "func [%s]", 
                                        note->flags & MOMO_INDEX_NOTE_FLAG_PROTOTYPE ? "prototype" : "def");
                }
                
            }break;
            
            case MOMO_INDEX_NOTE_KIND_MACRO:
            {
                sort = S8Lit("macro");
            }break;
            
            case MOMO_INDEX_NOTE_KIND_CONSTANT:
            {
                sort = S8Lit("constant");
            }break;
            
            case MOMO_INDEX_NOTE_KIND_COMMENT_TAG:
            {
                sort = S8Lit("comment tag");
            }break;
            
            case MOMO_INDEX_NOTE_KIND_COMMENT_TODO:
            {
                sort = S8Lit("TODO");
            }break;
            
            default: break;
        }
        
        momo_lister_add_item(lister, name, sort, jump, 0);
    }
}

// NOTE(Momo): Pred must be of type: b32(Momo_Index_Note*)
template<typename Pred>
function void
momo_list_project_notes_custom(Application_Links* app, String_Const_u8 init_str, b32 same_panel, Pred pred){
    char *query = "Index (Project):";
    
    Scratch_Block scratch(app);
    Momo_Lister_Block lister(app, scratch);
    momo_lister_set_query(lister, query);
    momo_lister_set_key(lister, init_str);
    momo_lister_set_text_field(lister, init_str);
    momo_lister_set_default_handlers(lister);
    
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
                    if (pred(note))
                        momo_push_lister_with_note(app, scratch, lister, note);
                }
            }
        }
    }
    Momo_Index_Unlock();
    
    Momo_Lister_Result l_result = momo_run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        if(!same_panel)
        {
            view = get_next_view_looped_primary_panels(app, view, Access_Always);
        }
        point_stack_push_view_cursor(app, view);
        momo_jump_to_location(app, view, result.buffer, result.pos);
    }
}

function void
momo_list_project_notes(Application_Links* app, String_Const_u8 init_str, b32 same_panel)
{
    momo_list_project_notes_custom(app, init_str, same_panel,   
        [](Momo_Index_Note*) -> b32 { 
            return true; 
    });
}


function i64
momo_boundary_token_and_whitespace(Application_Links *app, Buffer_ID buffer, 
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
    momo_lister_set_query(lister, query);
    momo_lister_set_default_handlers(lister);
    
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
                    momo_push_lister_with_note(app, scratch, lister, note);
                }
            }
        }
    }
    Momo_Index_Unlock();
    
    Momo_Lister_Result l_result = momo_run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        point_stack_push_view_cursor(app, view);
        momo_jump_to_location(app, view, result.buffer, result.pos);
    }
}

CUSTOM_UI_COMMAND_SIG(momo_search_for_definition__current_file)
CUSTOM_DOC("List all definitions in the current file and jump to the one selected by the user.")
{
    char *query = "Index (File):";
    
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    Scratch_Block scratch(app);
    Momo_Lister_Block lister(app, scratch);
    momo_lister_set_query(lister, query);
    momo_lister_set_default_handlers(lister);
    
    Momo_Index_Lock();
    {
        Momo_Index_File *file = Momo_Index_LookupFile(app, buffer);
        if(file != 0)
        {
            for(Momo_Index_Note *note = file->first_note; note; note = note->next_sibling)
            {
                momo_push_lister_with_note(app, scratch, lister, note);
            }
        }
    }
    Momo_Index_Unlock();
    
    Momo_Lister_Result l_result = momo_run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view_id = get_this_ctx_view(app, Access_Always);
        point_stack_push_view_cursor(app, view_id);
        momo_jump_to_location(app, view_id, result.buffer, result.pos);
    }
}


CUSTOM_COMMAND_SIG(page_up_half)
CUSTOM_DOC("Page up halfway")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    f32 page_jump = get_page_jump(app, view) * 0.5f;
    move_vertical_pixels(app, -page_jump);
}

CUSTOM_COMMAND_SIG(momo_interactive_open_or_new)
CUSTOM_DOC("Momo interactive open or new")
{
    QueryModeLock;
    interactive_open_or_new(app);
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

// NOTE(Momo): Yeah this is pretty dumb, but hey
CUSTOM_COMMAND_SIG(momo_number_mode_1) 
CUSTOM_DOC("Number Mode 1") { momo_number_mode(app, string_u8_litexpr("1")); }
CUSTOM_COMMAND_SIG(momo_number_mode_2) 
CUSTOM_DOC("Number Mode 2") { momo_number_mode(app, string_u8_litexpr("2")); }
CUSTOM_COMMAND_SIG(momo_number_mode_3) 
CUSTOM_DOC("Number Mode 3") { momo_number_mode(app, string_u8_litexpr("3")); }
CUSTOM_COMMAND_SIG(momo_number_mode_4) 
CUSTOM_DOC("Number Mode 4") { momo_number_mode(app, string_u8_litexpr("4")); }
CUSTOM_COMMAND_SIG(momo_number_mode_5) 
CUSTOM_DOC("Number Mode 5") { momo_number_mode(app, string_u8_litexpr("5")); }
CUSTOM_COMMAND_SIG(momo_number_mode_6) 
CUSTOM_DOC("Number Mode 6") { momo_number_mode(app, string_u8_litexpr("6")); }
CUSTOM_COMMAND_SIG(momo_number_mode_7) 
CUSTOM_DOC("Number Mode 7") { momo_number_mode(app, string_u8_litexpr("7")); }
CUSTOM_COMMAND_SIG(momo_number_mode_8) 
CUSTOM_DOC("Number Mode 8") { momo_number_mode(app, string_u8_litexpr("8")); }
CUSTOM_COMMAND_SIG(momo_number_mode_9) 
CUSTOM_DOC("Number Mode 9") { momo_number_mode(app, string_u8_litexpr("9")); }



CUSTOM_UI_COMMAND_SIG(momo_search_for_definition_under_cursor_project_wide)
CUSTOM_DOC("List all definitions in the index and enter the token under the cursor")
{
    Scratch_Block scratch(app);
    String_Const_u8 string = push_token_or_word_under_active_cursor(app, scratch);
    Momo_Index_Note* note = Momo_Index_LookupNote(string);
    if (note != 0) {
        if (note->next == 0) {
            // if there is only one exact match, just gooo
            momo_go_to_definition(app, note, 1);
        }
        else {
            // case for multiple notes
            auto ignore_pred = [string](Momo_Index_Note* note) -> b32 {
                return string_match(note->string, string);
            };
            momo_list_project_notes_custom(app, string, 1, ignore_pred);    
        }
    }
}

CUSTOM_UI_COMMAND_SIG(momo_search_for_definition_under_cursor_project_wide_other_panel)
CUSTOM_DOC("List all definitions in the index and enter the token under the cursor")
{
    Scratch_Block scratch(app);
    String_Const_u8 string = push_token_or_word_under_active_cursor(app, scratch);
    Momo_Index_Note* note = Momo_Index_LookupNote(string);
    if (note != 0) {
        if (note->next == 0) {
            // if there is only one exact match, just gooo
            momo_go_to_definition(app, note, 0);
        }
        else {
            // case for multiple notes
            auto ignore_pred = [string](Momo_Index_Note* note) -> b32 {
                return string_match(note->string, string);
            };
            momo_list_project_notes_custom(app, string, 0, ignore_pred);    
        }
    }
        
    
}



CUSTOM_UI_COMMAND_SIG(momo_search_for_definition_project_wide)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    String_Const_u8 str = {};
    momo_list_project_notes(app, str, 1);
}

CUSTOM_COMMAND_SIG(snipe_forward_whitespace_and_token_boundary)
CUSTOM_DOC("Delete a single, whole token on or to the right of the cursor and post it to the clipboard.")
{
    Scratch_Block scratch(app);
    current_view_snipe_delete(app, Scan_Forward, push_boundary_list(scratch, momo_boundary_token_and_whitespace));

}

CUSTOM_COMMAND_SIG(momo_change_mode)
CUSTOM_DOC("Change mode")
{
    QueryModeLock;

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
    QueryModeLock;

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
    QueryModeLock;

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
                momo_search_for_definition_project_wide(app);
                break;
            }

        }
    }
}

CUSTOM_COMMAND_SIG(momo_z_mode)
CUSTOM_DOC("Z mode")
{
    QueryModeLock;

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
    QueryModeLock;
    
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

CUSTOM_COMMAND_SIG(momo_query_replace)
CUSTOM_DOC("Queries the user for two strings, and incrementally replaces every occurence of the first string with the second string.")
{
    QueryModeLock;

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
    QueryModeLock;
    
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
    current_view_boundary_delete(app, Scan_Backward, push_boundary_list(scratch, momo_boundary_token_and_whitespace));
}

CUSTOM_COMMAND_SIG(momo_delete_token_boundary)
CUSTOM_DOC("Deletes right to a token boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Forward, push_boundary_list(scratch, momo_boundary_token_and_whitespace));
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



#endif