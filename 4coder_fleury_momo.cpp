#ifndef FCODER_FLEURY_MOMO
#define FCODER_FLEURY_MOMO

static b32 global_insert_mode = false;

function void
momo_write_text_and_auto_indent_internal(Application_Links* app, String_Const_u8 insert) {
    ProfileScope(app, "write and auto indent");
    if (insert.str != 0 && insert.size > 0){
        b32 do_auto_indent = false;
        for (u64 i = 0; !do_auto_indent && i < insert.size; i += 1){
            switch (insert.str[i]){
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
        if (do_auto_indent){
            View_ID view = get_active_view(app, Access_ReadWriteVisible);
            Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
            Range_i64 pos = {};
            pos.min = view_get_cursor_pos(app, view);
            write_text(app, insert);
            pos.max= view_get_cursor_pos(app, view);
            auto_indent_buffer(app, buffer, pos, 0);
            move_past_lead_whitespace(app, view, buffer);
        }
        else{
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
    } else {
        result = Max(buffer_seek_character_class_change_0_1(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos),
                     buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_alpha_numeric_underscore_utf8, direction, pos));
        result = Max(result, buffer_seek_character_class_change_1_0(app, buffer, &character_predicate_whitespace, direction, pos));
    }
    
    return result;
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
CUSTOM_DOC("Alias for exiting")
{
    exit_4coder(app);
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
        buffer_get_char(app, buffer, range.end - 1) != '\n'){
        range.start -= 1;
        range.first = clamp_bot(0, range.first);
    }
    if (clipboard_post_buffer_range(app, 0, buffer, range)){
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
#endif