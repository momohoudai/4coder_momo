function f32
Momo_Lister_GetTextFileHeight(f32 line_height){
    return(line_height);
}

function f32
Momo_Lister_GetBlockHeight(f32 line_height){
    return(line_height*2);
}

function Rect_f32_Pair
Momo_Lister_GetTopLevelLayout(Rect_f32 rect, f32 text_field_height){
    return(rect_split_top_bottom(rect, text_field_height));
}

////////////////////////////////

function Momo_Lister*
Momo_Lister_GetFromView(Application_Links *app, View_ID view){
    Managed_Scope scope = view_get_managed_scope(app, view);
    Momo_Lister **ptr = scope_attachment(app, scope, view_lister_loc, Momo_Lister*);
    Momo_Lister *result = 0;
    if (ptr != 0){
        result = *ptr;
    }
    return(result);
}

function Momo_Lister*
Momo_Lister_SetToView(Application_Links *app, View_ID view, Momo_Lister *lister){
    Managed_Scope scope = view_get_managed_scope(app, view);
    Momo_Lister **ptr = scope_attachment(app, scope, view_lister_loc, Momo_Lister*);
    Momo_Lister *result = 0;
    if (ptr != 0){
        result = *ptr;
        *ptr = lister;
    }
    return(result);
}

function void
Momo_Lister_SetMap(Momo_Lister *lister, Mapping *mapping, Command_Map *map){
    lister->mapping = mapping;
    lister->map = map;
}

function void
Momo_Lister_SetMap(Momo_Lister *lister, Mapping *mapping, Command_Map_ID map){
    lister->mapping = mapping;
    lister->map = mapping_get_map(mapping, map);
}

function Momo_Lister_Prev_Current
Momo_Lister_Begin(Application_Links *app, Arena *arena){
    Momo_Lister_Prev_Current result = {};
    Momo_Lister *lister = push_array_zero(arena, Momo_Lister, 1);
    lister->arena = arena;
    lister->query = Su8(lister->query_space, 0, sizeof(lister->query_space));
    lister->text_field = Su8(lister->text_field_space, 0, sizeof(lister->text_field_space));
    lister->key_string = Su8(lister->key_string_space, 0, sizeof(lister->key_string_space));
    View_ID view = get_this_ctx_view(app, Access_Always);
    result.prev = Momo_Lister_SetToView(app, view, lister);
    result.current = lister;
    lister->restore_all_point = begin_temp(lister->arena);
    View_Context ctx = view_current_context(app, view);
    Momo_Lister_SetMap(lister, ctx.mapping, ctx.map_id);
    return(result);
}

Momo_Lister_Block::Momo_Lister_Block(Application_Links *a, Arena *arena){
    Momo_Lister_Prev_Current new_lister = Momo_Lister_Begin(a, arena);
    this->app = a;
    this->lister = new_lister;
}

Momo_Lister_Block::~Momo_Lister_Block(){
    View_ID view = get_this_ctx_view(app, Access_Always);
    Momo_Lister_SetToView(this->app, view, this->lister.prev);
}

Momo_Lister_Block::operator Momo_Lister *(){
    return(this->lister.current);
}

function void
Momo_Lister_SetString(String_Const_u8 string, String_u8 *target){
    target->size = 0;
    string_append(target, string);
}
function void
Momo_Lister_AppendString(String_Const_u8 string, String_u8 *target){
    string_append(target, string);
}

function void
Momo_Lister_SetQuery(Momo_Lister *lister, String_Const_u8 string){
    Momo_Lister_SetString(string, &lister->query);
}
function void
Momo_Lister_SetQuery(Momo_Lister *lister, char *string){
    Momo_Lister_SetString(SCu8(string), &lister->query);
}
function void
Momo_Lister_SetTextField(Momo_Lister *lister, String_Const_u8 string){
    Momo_Lister_SetString(string, &lister->text_field);
}
function void
Momo_Lister_SetTextField(Momo_Lister *lister, char *string){
    Momo_Lister_SetString(SCu8(string), &lister->text_field);
}
function void
Momo_Lister_SetKey(Momo_Lister *lister, String_Const_u8 string){
    Momo_Lister_SetString(string, &lister->key_string);
}
function void
Momo_Lister_SetKey(Momo_Lister *lister, char *string){
    Momo_Lister_SetString(SCu8(string), &lister->key_string);
}

function void
Momo_Lister_AppendQuery(Momo_Lister *lister, String_Const_u8 string){
    Momo_Lister_AppendString(string, &lister->query);
}
function void
Momo_Lister_AppendQuery(Momo_Lister *lister, char *string){
    Momo_Lister_AppendString(SCu8(string), &lister->query);
}
function void
Momo_Lister_AppendTextField(Momo_Lister *lister, String_Const_u8 string){
    Momo_Lister_AppendString(string, &lister->text_field);
}
function void
Momo_Lister_AppendTextField(Momo_Lister *lister, char *string){
    Momo_Lister_AppendString(SCu8(string), &lister->text_field);
}
function void
Momo_Lister_AppendKey(Momo_Lister *lister, String_Const_u8 string){
    Momo_Lister_AppendString(string, &lister->key_string);
}
function void
Momo_Lister_AppendKey(Momo_Lister *lister, char *string){
    Momo_Lister_AppendString(SCu8(string), &lister->key_string);
}

function void
Momo_Lister_SetHandlers(Momo_Lister *lister, Momo_Lister_Handlers *handlers){
    block_copy_struct(&lister->handlers, handlers);
}

function void
Momo_Lister_ZeroScroll(Momo_Lister *lister){
    block_zero_struct(&lister->scroll);
}

function void
Momo_Lister_Render(Application_Links *app, Frame_Info frame_info, View_ID view){
    Scratch_Block scratch(app);
    
    Momo_Lister *lister = Momo_Lister_GetFromView(app, view);
    if (lister == 0){
        return;
    }
    
    Rect_f32 region = draw_background_and_margin(app, view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Face_ID face_id = get_face_id(app, 0);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 line_height = metrics.line_height;
    f32 block_height = Momo_Lister_GetBlockHeight(line_height);
    f32 text_field_height = Momo_Lister_GetTextFileHeight(line_height);
    
    // NOTE(allen): file bar
    // TODO(allen): What's going on with 'showing_file_bar'? I found it like this.
    b64 showing_file_bar = false;
    b32 hide_file_bar_in_ui = def_get_config_b32(vars_save_string_lit("hide_file_bar_in_ui"));
    if (view_get_setting(app, view, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar && !hide_file_bar_in_ui){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        draw_file_bar(app, view, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Mouse_State mouse = get_mouse_state(app);
    Vec2_f32 m_p = V2f32(mouse.p);
    
    lister->visible_count = (i32)((rect_height(region)/block_height)) - 3;
    lister->visible_count = clamp_bot(1, lister->visible_count);
    
    Rect_f32 text_field_rect = {};
    Rect_f32 list_rect = {};
    {
        Rect_f32_Pair pair = Momo_Lister_GetTopLevelLayout(region, text_field_height);
        text_field_rect = pair.min;
        list_rect = pair.max;
    }
    
    {
        Vec2_f32 p = V2f32(text_field_rect.x0 + 3.f, text_field_rect.y0);
        Fancy_Line text_field = {};
        push_fancy_string(scratch, &text_field, fcolor_id(defcolor_pop1),
                          lister->query.string);
        push_fancy_stringf(scratch, &text_field, " ");
        p = draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
        
        // TODO(allen): This is a bit of a hack. Maybe an upgrade to fancy to focus
        // more on being good at this and less on overriding everything 10 ways to sunday
        // would be good.
        block_zero_struct(&text_field);
        push_fancy_string(scratch, &text_field, fcolor_id(defcolor_text_default),
                          lister->text_field.string);
        f32 width = get_fancy_line_width(app, face_id, &text_field);
        f32 cap_width = text_field_rect.x1 - p.x - 6.f;
        if (cap_width < width){
            Rect_f32 prect = draw_set_clip(app, Rf32(p.x, text_field_rect.y0, p.x + cap_width, text_field_rect.y1));
            p.x += cap_width - width;
            draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
            draw_set_clip(app, prect);
        }
        else{
            draw_fancy_line(app, face_id, fcolor_zero(), &text_field, p);
        }
    }
    
    
    Range_f32 x = rect_range_x(list_rect);
    draw_set_clip(app, list_rect);
    
    // NOTE(allen): auto scroll to the item if the flag is set.
    f32 scroll_y = lister->scroll.position.y;
    
    if (lister->set_vertical_focus_to_item){
        lister->set_vertical_focus_to_item = false;
        Range_f32 item_y = If32_size(lister->item_index*block_height, block_height);
        f32 view_h = rect_height(list_rect);
        Range_f32 view_y = If32_size(scroll_y, view_h);
        if (view_y.min > item_y.min || item_y.max > view_y.max){
            f32 item_center = (item_y.min + item_y.max)*0.5f;
            f32 view_center = (view_y.min + view_y.max)*0.4f;
            f32 margin = view_h*.3f;
            margin = clamp_top(margin, block_height*3.f);
            if (item_center < view_center){
                lister->scroll.target.y = item_y.min - margin;
            }
            else{
                f32 target_bot = item_y.max + margin;
                lister->scroll.target.y = target_bot - view_h;
            }
        }
    }
    
    // NOTE(allen): clamp scroll target and position; smooth scroll rule
    i32 count = lister->filtered.count;
    Range_f32 scroll_range = If32(0.f, clamp_bot(0.f, count*block_height - block_height));
    lister->scroll.target.y = clamp_range(scroll_range, lister->scroll.target.y);
    lister->scroll.target.x = 0.f;
    
    Vec2_f32_Delta_Result delta = delta_apply(app, view,
                                              frame_info.animation_dt, lister->scroll);
    lister->scroll.position = delta.p;
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    lister->scroll.position.y = clamp_range(scroll_range, lister->scroll.position.y);
    lister->scroll.position.x = 0.f;
    
    scroll_y = lister->scroll.position.y;
    f32 y_pos = list_rect.y0 - scroll_y;
    
    i32 first_index = (i32)(scroll_y/block_height);
    y_pos += first_index*block_height;
    
    for (i32 i = first_index; i < count; i += 1){
        Momo_Lister_Node *node = lister->filtered.node_ptrs[i];
        
        Range_f32 y = If32(y_pos, y_pos + block_height);
        y_pos = y.max;
        
        Rect_f32 item_rect = Rf32(x, y);
        Rect_f32 item_inner = rect_inner(item_rect, 3.f);
        
        b32 hovered = rect_contains_point(item_rect, m_p);
        UI_Highlight_Level highlight = UIHighlight_None;
        if (node == lister->highlighted_node){
            highlight = UIHighlight_Active;
        }
        else if (node->user_data == lister->hot_user_data){
            if (hovered){
                highlight = UIHighlight_Active;
            }
            else{
                highlight = UIHighlight_Hover;
            }
        }
        else if (hovered){
            highlight = UIHighlight_Hover;
        }
        
        u64 lister_roundness_100 = def_get_config_u64(app, vars_save_string_lit("lister_roundness"));
        f32 roundness = block_height*lister_roundness_100*0.01f;
        draw_rectangle_fcolor(app, item_rect, roundness, get_item_margin_color(highlight));
        draw_rectangle_fcolor(app, item_inner, roundness, get_item_margin_color(highlight, 1));
        
        Fancy_Line line = {};
        push_fancy_string(scratch, &line, fcolor_id(defcolor_text_default), node->string);
        push_fancy_stringf(scratch, &line, " ");
        push_fancy_string(scratch, &line, fcolor_id(defcolor_pop2), node->status);
        
        Vec2_f32 p = item_inner.p0 + V2f32(3.f, (block_height - line_height)*0.5f);
        draw_fancy_line(app, face_id, fcolor_zero(), &line, p);
    }
    
    draw_set_clip(app, prev_clip);
}

function void*
Momo_Lister_GetUserData(Momo_Lister *lister, i32 index){
    void *result = 0;
    if (0 <= index && index < lister->options.count){
        i32 counter = 0;
        for (Momo_Lister_Node *node = lister->options.first;
             node != 0;
             node = node->next, counter += 1){
            if (counter == index){
                result = node->user_data;
                break;
            }
        }
    }
    return(result);
}

function Momo_Lister_Filtered
Momo_Lister_GetFiltered(Arena *arena, Momo_Lister *lister){
    i32 node_count = lister->options.count;
    
    Momo_Lister_Filtered filtered = {};
    filtered.exact_matches.node_ptrs = push_array(arena, Momo_Lister_Node*, 1);
    filtered.before_extension_matches.node_ptrs = push_array(arena, Momo_Lister_Node*, node_count);
    filtered.substring_matches.node_ptrs = push_array(arena, Momo_Lister_Node*, node_count);
    
    Temp_Memory_Block temp(arena);
    
    String_Const_u8 key = lister->key_string.string;
    key = push_string_copy(arena, key);
    string_mod_replace_character(key, '_', '*');
    string_mod_replace_character(key, ' ', '*');
    
    List_String_Const_u8 absolutes = {};
    string_list_push(arena, &absolutes, string_u8_litexpr(""));
    List_String_Const_u8 splits = string_split(arena, key, (u8*)"*", 1);
    b32 has_wildcard = (splits.node_count > 1);
    string_list_push(&absolutes, &splits);
    string_list_push(arena, &absolutes, string_u8_litexpr(""));
    
    for (Momo_Lister_Node *node = lister->options.first;
         node != 0;
         node = node->next){
        String_Const_u8 node_string = node->string;
        if (key.size == 0 || string_wildcard_match_insensitive(absolutes, node_string)){
            if (string_match_insensitive(node_string, key) && filtered.exact_matches.count == 0){
                filtered.exact_matches.node_ptrs[filtered.exact_matches.count++] = node;
            }
            else if (key.size > 0 &&
                     !has_wildcard &&
                     string_match_insensitive(string_prefix(node_string, key.size), key) &&
                     node->string.size > key.size &&
                     node->string.str[key.size] == '.'){
                filtered.before_extension_matches.node_ptrs[filtered.before_extension_matches.count++] = node;
            }
            else{
                filtered.substring_matches.node_ptrs[filtered.substring_matches.count++] = node;
            }
        }
    }
    
    return(filtered);
}

function void
Momo_Lister_UpdateSelectionValues(Momo_Lister *lister){
    lister->raw_item_index = -1;
    lister->highlighted_node = 0;
    i32 count = lister->filtered.count;
    for (i32 i = 0; i < count; i += 1){
        Momo_Lister_Node *node = lister->filtered.node_ptrs[i];
        if (lister->item_index == i){
            lister->highlighted_node = node;
            lister->raw_item_index = node->raw_index;
        }
    }
}

function void
Momo_Lister_UpdateFilteredList(Application_Links *app, Momo_Lister *lister){
    Arena *arena = lister->arena;
    Scratch_Block scratch(app, arena);
    
    Momo_Lister_Filtered filtered = Momo_Lister_GetFiltered(scratch, lister);
    
    Momo_Lister_Node_Ptr_Array node_ptr_arrays[] = {
        filtered.exact_matches,
        filtered.before_extension_matches,
        filtered.substring_matches,
    };
    
    end_temp(lister->filter_restore_point);
    
    i32 total_count = 0;
    for (i32 array_index = 0; array_index < ArrayCount(node_ptr_arrays); array_index += 1){
        Momo_Lister_Node_Ptr_Array node_ptr_array = node_ptr_arrays[array_index];
        total_count += node_ptr_array.count;
    }
    
    Momo_Lister_Node **node_ptrs = push_array(arena, Momo_Lister_Node*, total_count);
    lister->filtered.node_ptrs = node_ptrs;
    lister->filtered.count = total_count;
    i32 counter = 0;
    for (i32 array_index = 0; array_index < ArrayCount(node_ptr_arrays); array_index += 1){
        Momo_Lister_Node_Ptr_Array node_ptr_array = node_ptr_arrays[array_index];
        for (i32 node_index = 0; node_index < node_ptr_array.count; node_index += 1){
            Momo_Lister_Node *node = node_ptr_array.node_ptrs[node_index];
            node_ptrs[counter] = node;
            counter += 1;
        }
    }
    
    Momo_Lister_UpdateSelectionValues(lister);
}

function void
Momo_Lister_CallRefreshHandler(Application_Links *app, Momo_Lister *lister){
    if (lister->handlers.refresh != 0){
        lister->handlers.refresh(app, lister);
        lister->filter_restore_point = begin_temp(lister->arena);
        Momo_Lister_UpdateFilteredList(app, lister);
    }
}

function void
Momo_Lister_Activate(Application_Links *app, Momo_Lister *lister, void *user_data, b32 mouse){
    lister->out.activated_by_click = mouse;
    lister->out.text_field = lister->text_field.string;
    lister->out.user_data = user_data;
}

function void*
Momo_Lister_UserDataAtP(Application_Links *app, View_ID view, Momo_Lister *lister, Vec2_f32 m_p){
    Rect_f32 region = view_get_screen_rect(app, view);
    // TODO(allen): eliminate this. bad bad bad bad :(
    region = rect_inner(region, 3.f);
    
    Face_ID face_id = get_face_id(app, 0);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 line_height = metrics.line_height;
    f32 block_height = Momo_Lister_GetBlockHeight(line_height);
    f32 text_field_height = Momo_Lister_GetTextFileHeight(line_height);
    
    b64 showing_file_bar = false;
    b32 hide_file_bar_in_ui = def_get_config_b32(vars_save_string_lit("hide_file_bar_in_ui"));
    if (view_get_setting(app, view, ViewSetting_ShowFileBar, &showing_file_bar) &&
        showing_file_bar && hide_file_bar_in_ui){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        region = pair.max;
    }
    
    Rect_f32_Pair pair = Momo_Lister_GetTopLevelLayout(region, text_field_height);
    Rect_f32 list_rect = pair.max;
    
    void *result = 0;
    if (rect_contains_point(list_rect, m_p)){
        f32 y = m_p.y - list_rect.y0 + lister->scroll.position.y;
        i32 index = (i32)(y/block_height);
        if (0 <= index && index < lister->filtered.count){
            Momo_Lister_Node *node = lister->filtered.node_ptrs[index];
            result = node->user_data;
        }
    }
    
    return(result);
}

function Momo_Lister_Result
Momo_Lister_Run(Application_Links *app, Momo_Lister *lister){
    lister->filter_restore_point = begin_temp(lister->arena);
    Momo_Lister_UpdateFilteredList(app, lister);
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    View_Context ctx = view_current_context(app, view);
    ctx.render_caller = Momo_Lister_Render;
    ctx.hides_buffer = true;
    View_Context_Block ctx_block(app, view, &ctx);
    
    b32 is_done = false;
    while (!is_done){
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort){
            block_zero_struct(&lister->out);
            lister->out.canceled = true;
            break;
        }
        
        Lister_Activation_Code result = ListerActivation_Continue;
        b32 handled = true;
        switch (in.event.kind){
            case InputEventKind_TextInsert:
            {
                if (lister->handlers.write_character != 0){
                    result = lister->handlers.write_character(app);
                }
            }break;
        
            case InputEventKind_KeyStroke:
            {
                switch (in.event.key.code){
                    case KeyCode_Return:
                    case KeyCode_Tab:
                    {
                        void *user_data = 0;
                        if (0 <= lister->raw_item_index &&
                            lister->raw_item_index < lister->options.count){
                            user_data = Momo_Lister_GetUserData(lister, lister->raw_item_index);
                        }
                        Momo_Lister_Activate(app, lister, user_data, false);
                        result = ListerActivation_Finished;
                    }break;
                    
                    case KeyCode_Backspace:
                    {
                        if (lister->handlers.backspace != 0){
                            lister->handlers.backspace(app);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_Up:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister, -1);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    
                    }break;
                    
                    case KeyCode_Down:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister, 1);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                        
                    }break;
                    
                    case KeyCode_PageUp:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister,
                                                      -lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_PageDown:
                    {
                        if (lister->handlers.navigate != 0){
                            lister->handlers.navigate(app, view, lister,
                                                      lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    default:
                    {
                        if (lister->handlers.key_stroke != 0){
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButton:
            {
                switch (in.event.mouse.code){
                    case MouseCode_Left:
                    {
                        Vec2_f32 p = V2f32(in.event.mouse.p);
                        void *clicked = Momo_Lister_UserDataAtP(app, view, lister, p);
                        lister->hot_user_data = clicked;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButtonRelease:
            {
                switch (in.event.mouse.code){
                    case MouseCode_Left:
                    {
                        if (lister->hot_user_data != 0){
                            Vec2_f32 p = V2f32(in.event.mouse.p);
                            void *clicked = Momo_Lister_UserDataAtP(app, view, lister, p);
                            if (lister->hot_user_data == clicked){
                                Momo_Lister_Activate(app, lister, clicked, true);
                                result = Momo_ListerActivation_Finished;
                            }
                        }
                        lister->hot_user_data = 0;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseWheel:
            {
                Mouse_State mouse = get_mouse_state(app);
                lister->scroll.target.y += mouse.wheel;
                Momo_Lister_UpdateFilteredList(app, lister);
            }break;
            
            case InputEventKind_MouseMove:
            {
                Momo_Lister_UpdateFilteredList(app, lister);
            }break;
            
            case InputEventKind_Core:
            {
                switch (in.event.core.code){
                    case CoreCode_Animate:
                    {
                        Momo_Lister_UpdateFilteredList(app, lister);
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            default:
            {
                handled = false;
            }break;
        }
        
        if (result == Momo_ListerActivation_Finished){
            break;
        }
        
        if (!handled){
            leave_current_input_unhandled(app);
        }
    }
    
    return(lister->out);
}

function Momo_Lister_Prealloced_String
Momo_Lister_PreAlloced(String_Const_u8 string){
    Momo_Lister_Prealloced_String result = {};
    result.string = string;
    return(result);
}

function void
Momo_Lister_BeginNewItemSet(Application_Links *app, Momo_Lister *lister){
    end_temp(lister->restore_all_point);
    block_zero_struct(&lister->options);
    block_zero_struct(&lister->filtered);
}

function void*
Momo_Lister_AddItem(Momo_Lister *lister, Momo_Lister_Prealloced_String string, Momo_Lister_Prealloced_String status, void *user_data, u64 extra_space){
    void *base_memory = push_array(lister->arena, u8, sizeof(Momo_Lister_Node) + extra_space);
    Momo_Lister_Node *node = (Momo_Lister_Node*)base_memory;
    node->string = string.string;
    node->status = status.string;
    node->user_data = user_data;
    node->raw_index = lister->options.count;
    zdll_push_back(lister->options.first, lister->options.last, node);
    lister->options.count += 1;
    void *result = (node + 1);
    return(result);
}

function void*
Momo_Lister_AddItem(Momo_Lister *lister, Momo_Lister_Prealloced_String string, String_Const_u8 status,
                void *user_data, u64  extra_space){
    return(Momo_Lister_AddItem(lister, string, Momo_Lister_PreAlloced(push_string_copy(lister->arena, status)), user_data, extra_space));
}

function void*
Momo_Lister_AddItem(Momo_Lister *lister, String_Const_u8 string, Momo_Lister_Prealloced_String status, void *user_data, u64 extra_space){
    return(Momo_Lister_AddItem(lister, Momo_Lister_PreAlloced(push_string_copy(lister->arena, string)), status, user_data, extra_space));
}

function void*
Momo_Lister_AddItem(Momo_Lister *lister, String_Const_u8 string, String_Const_u8 status, void *user_data, u64 extra_space){
    return(Momo_Lister_AddItem(lister,
                           Momo_Lister_PreAlloced(push_string_copy(lister->arena, string)),
                           Momo_Lister_PreAlloced(push_string_copy(lister->arena, status)),
                           user_data, extra_space));
}

function void*
Momo_Lister_AddItem(Momo_Lister *lister, String_Const_u8 string, String_Const_u8 status, String_Const_u8 info, void *user_data, u64 extra_space){
    return(Momo_Lister_AddItem(lister,
                           Momo_Lister_PreAlloced(push_string_copy(lister->arena, string)),
                           Momo_Lister_PreAlloced(push_string_copy(lister->arena, status)),
                           user_data, extra_space));
}

function Lister_Activation_Code
Momo_Lister__WriteStringDefault(Application_Links *app){
    Lister_Activation_Code result = ListerActivation_Continue;
    View_ID view = get_active_view(app, Access_Always);
    Momo_Lister *lister = Momo_Lister_GetFromView(app, view);
    if (lister != 0){
        User_Input in = get_current_input(app);
        String_Const_u8 string = to_writable(&in);
        if (string.str != 0 && string.size > 0){
            Momo_Lister_AppendTextField(lister, string);
            Momo_Lister_AppendKey(lister, string);
            lister->item_index = 0;
            Momo_Lister_ZeroScroll(lister);
            Momo_Lister_UpdateFilteredList(app, lister);
        }
    }
    return(result);
}

function void
Momo_Lister__BackspaceTextFieldDefault(Application_Links *app){
    View_ID view = get_active_view(app, Access_Always);
    Momo_Lister *lister = Momo_Lister_GetFromView(app, view);
    if (lister != 0){
        lister->text_field.string = backspace_utf8(lister->text_field.string);
        lister->key_string.string = backspace_utf8(lister->key_string.string);
        lister->item_index = 0;
        Momo_Lister_ZeroScroll(lister);
        Momo_Lister_UpdateFilteredList(app, lister);
    }
}

function void
Momo_Lister__NavigateDefault(Application_Links *app, View_ID view, Momo_Lister *lister, i32 delta){
    i32 new_index = lister->item_index + delta;
    if (new_index < 0 && lister->item_index == 0){
        lister->item_index = lister->filtered.count - 1;
    }
    else if (new_index >= lister->filtered.count &&
             lister->item_index == lister->filtered.count - 1){
        lister->item_index = 0;
    }
    else{
        lister->item_index = clamp(0, new_index, lister->filtered.count - 1);
    }
    lister->set_vertical_focus_to_item = true;
    Momo_Lister_UpdateSelectionValues(lister);
}

function Momo_Lister_Handlers
Momo_Lister_GetDefaultHandlers(void){
    Momo_Lister_Handlers handlers = {};
    handlers.write_character = Momo_Lister__WriteStringDefault;
    handlers.backspace       = Momo_Lister__BackspaceTextFieldDefault;
    handlers.navigate        = Momo_Lister__NavigateDefault;
    return(handlers);
}

function void
Momo_Lister_SetDefaultHandlers(Momo_Lister *lister){
    Momo_Lister_Handlers handlers = Momo_Lister_GetDefaultHandlers();
    Momo_Lister_SetHandlers(lister, &handlers);
}

////////////////////////////////

function Momo_Lister_Result
Momo_Lister_RunWithRefreshHandler(Application_Links *app, Arena *arena, String_Const_u8 query, Momo_Lister_Handlers handlers){
    Momo_Lister_Result result = {};
    if (handlers.refresh != 0){
        Momo_Lister_Block lister(app, arena);
        Momo_Lister_SetQuery(lister, query);
        Momo_Lister_SetHandlers(lister, &handlers);
        handlers.refresh(app, lister);
        result = Momo_Lister_Run(app, lister);
    }
    else{
#define M "ERROR: No refresh handler specified for lister (query_string = \"%.*s\")\n"
        String_Const_u8 str = push_u8_stringf(arena, M, string_expand(query));
#undef M
        print_message(app, str);
        result.canceled = true;
    }
    return(result);
}

function Momo_Lister_Result
Momo_Lister_RunWithRefreshHandler(Application_Links *app, String_Const_u8 query,  Momo_Lister_Handlers handlers){
    Scratch_Block scratch(app);
    return(Momo_Lister_RunWithRefreshHandler(app, scratch, query, handlers));
}

function Momo_Lister_Result
Momo_Lister_RunWithRefreshHandler(Application_Links *app, Arena *arena, char *query, Momo_Lister_Handlers handlers){
    return(Momo_Lister_RunWithRefreshHandler(app, arena, SCu8(query), handlers));
}

function Momo_Lister_Result
Momo_Lister_RunWithRefreshHandler(Application_Links *app, char *query, Momo_Lister_Handlers handlers){
    return(Momo_Lister_RunWithRefreshHandler(app, SCu8(query), handlers));
}

////////////////////////////////

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, String_Const_u8 string, String_Const_u8 status, Key_Code code, u64 user_data){
    Momo_Lister_Choice *choice = push_array(arena, Momo_Lister_Choice, 1);
    sll_queue_push(list->first, list->last, choice);
    choice->string = string;
    choice->status = status;
    choice->key_code = code;
    choice->user_data = user_data;
}

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, char *string, String_Const_u8 status, Key_Code code, u64 user_data){
    Momo_Lister_Choose(arena, list, SCu8(string), status, code, (u64)PtrAsInt(user_data));
}

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, String_Const_u8 string, char *status, Key_Code code, u64 user_data){
    Momo_Lister_Choose(arena, list, string, SCu8(status), code, (u64)PtrAsInt(user_data));
}

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, char *string, char *status, Key_Code code, u64 user_data){
    Momo_Lister_Choose(arena, list, SCu8(string), SCu8(status), code,
                  (u64)PtrAsInt(user_data));
}

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, String_Const_u8 string, String_Const_u8 status, Key_Code code, void *user_data){
    Momo_Lister_Choose(arena, list, string, status, code, (u64)PtrAsInt(user_data));
}

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, char *string, String_Const_u8 status, Key_Code code, void *user_data){
    Momo_Lister_Choose(arena, list, string, status, code, (u64)PtrAsInt(user_data));
}

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, String_Const_u8 string, char *status, Key_Code code, void *user_data){
    Momo_Lister_Choose(arena, list, string, status, code, (u64)PtrAsInt(user_data));
}

function void
Momo_Lister_Choose(Arena *arena, Momo_Lister_Choice_List *list, char *string, char *status, Key_Code code, void *user_data){
    Momo_Lister_Choose(arena, list, string, status, code, (u64)PtrAsInt(user_data));
}

function Lister_Activation_Code
Momo_Lister__KeyStrokeChoiceList(Application_Links *app){
    Lister_Activation_Code result = ListerActivation_Continue;
    View_ID view = get_active_view(app, Access_Always);
    Momo_Lister *lister = Momo_Lister_GetFromView(app, view);
    if (lister != 0){
        User_Input in = get_current_input(app);
        if (in.event.kind == InputEventKind_KeyStroke){
            void *user_data = 0;
            b32 did_shortcut_key = false;
            for (Momo_Lister_Node *node = lister->options.first;
                 node != 0;
                 node = node->next){
                Key_Code *key_code = (Key_Code*)(node + 1);
                if (*key_code == in.event.key.code){
                    user_data = node->user_data;
                    did_shortcut_key = true;
                    break;
                }
            }
            if (did_shortcut_key){
                Momo_Lister_Activate(app, lister, user_data, false);
                result = Momo_ListerActivation_Finished;
            }
        }
    }
    return(result);
}

function Momo_Lister_Choice*
Momo_Lister_GetChoiceFromUser(Application_Links *app, String_Const_u8 query,
                     Momo_Lister_Choice_List list){
    Scratch_Block scratch(app);
    Momo_Lister_Block lister(app, scratch);
    for (Momo_Lister_Choice *choice = list.first;
         choice != 0;
         choice = choice->next){
        u64 code_size = sizeof(choice->key_code);
        void *extra = Momo_Lister_AddItem(lister, choice->string, choice->status,
                                      choice, code_size);
        block_copy(extra, &choice->key_code, code_size);
    }
    Momo_Lister_SetQuery(lister, query);
    Momo_Lister_Handlers handlers = {};
    handlers.navigate        = Momo_Lister__NavigateDefault;
    handlers.key_stroke      = Momo_Lister__KeyStrokeChoiceList;
    Momo_Lister_SetHandlers(lister, &handlers);
    
    Momo_Lister_Result l_result = Momo_Lister_Run(app, lister);
    Momo_Lister_Choice *result = 0;
    if (!l_result.canceled){
        result = (Momo_Lister_Choice*)l_result.user_data;
    }
    return(result);
}

function Momo_Lister_Choice*
Momo_Lister_GetChoiceFromUser(Application_Links *app, char *query, Momo_Lister_Choice_List list){
    return(Momo_Lister_GetChoiceFromUser(app, SCu8(query), list));
}

function void
Momo_Lister_PushDirectoryFiles(Application_Links *app, Momo_Lister *lister){
    Scratch_Block scratch(app, lister->arena);
    
    Temp_Memory temp = begin_temp(lister->arena);
    String_Const_u8 hot = push_hot_directory(app, lister->arena);
    if (!character_is_slash(string_get_character(hot, hot.size - 1))){
        hot = push_u8_stringf(lister->arena, "%.*s/", string_expand(hot));
    }
    Momo_Lister_SetTextField(lister, hot);
    Momo_Lister_SetKey(lister, string_front_of_path(hot));
    
    File_List file_list = system_get_file_list(scratch, hot);
    end_temp(temp);
    
    File_Info **one_past_last = file_list.infos + file_list.count;
    
    Momo_Lister_BeginNewItemSet(app, lister);
    
    hot = push_hot_directory(app, lister->arena);
    push_align(lister->arena, 8);
    if (hot.str != 0){
        String_Const_u8 empty_string = string_u8_litexpr("");
        Momo_Lister_Prealloced_String empty_string_prealloced = Momo_Lister_PreAlloced(empty_string);
        for (File_Info **info = file_list.infos;
             info < one_past_last;
             info += 1){
            if (!HasFlag((**info).attributes.flags, FileAttribute_IsDirectory)) continue;
            String_Const_u8 file_name = push_u8_stringf(lister->arena, "%.*s/",
                                                        string_expand((**info).file_name));
            Momo_Lister_AddItem(lister, Momo_Lister_PreAlloced(file_name), empty_string_prealloced, file_name.str, 0);
        }
        
        for (File_Info **info = file_list.infos;
             info < one_past_last;
             info += 1){
            if (HasFlag((**info).attributes.flags, FileAttribute_IsDirectory)) continue;
            String_Const_u8 file_name = push_string_copy(lister->arena, (**info).file_name);
            char *is_loaded = "";
            char *status_flag = "";
            
            Buffer_ID buffer = {};
            {
                Temp_Memory path_temp = begin_temp(lister->arena);
                List_String_Const_u8 list = {};
                string_list_push(lister->arena, &list, hot);
                string_list_push_overlap(lister->arena, &list, '/', (**info).file_name);
                String_Const_u8 full_file_path = string_list_flatten(lister->arena, list);
                buffer = get_buffer_by_file_name(app, full_file_path, Access_Always);
                end_temp(path_temp);
            }
            
            if (buffer != 0){
                is_loaded = "LOADED";
                Dirty_State dirty = buffer_get_dirty_state(app, buffer);
                switch (dirty){
                    case DirtyState_UnsavedChanges:  status_flag = " *"; break;
                    case DirtyState_UnloadedChanges: status_flag = " !"; break;
                    case DirtyState_UnsavedChangesAndUnloadedChanges: status_flag = " *!"; break;
                }
            }
            String_Const_u8 status = push_u8_stringf(lister->arena, "%s%s", is_loaded, status_flag);
            Momo_Lister_AddItem(lister, Momo_Lister_PreAlloced(file_name), Momo_Lister_PreAlloced(status), file_name.str, 0);
        }
    }
}


internal void
Momo_Lister_PushNote(Application_Links *app, Arena *arena, Momo_Lister *lister, Momo_Index_Note *note)
{
    if(note && note->file)
    {
        Momo_Index_File *file = note->file;
        Buffer_ID buffer = file->buffer;
        
        Tiny_Jump *jump = push_array(arena, Tiny_Jump, 1);
        jump->buffer = buffer;
        jump->pos = note->range.first;
        
        //String_Const_u8 buffer_name = push_buffer_unique_name(app, arena, buffer);
        String_Const_u8 display_name = {};
        display_name = push_stringf(arena, "%.*s", string_expand(note->display));
        String_Const_u8 sort = S8Lit("");
        switch(note->kind)
        {
            case MOMO_INDEX_NOTE_KIND_TYPE:
            {
                sort = push_stringf(arena, "type");
            }break;            
            case MOMO_INDEX_NOTE_KIND_FUNCTION:
            {
                sort = push_stringf(arena, "func"); 
            }break;
            
            case MOMO_INDEX_NOTE_KIND_MACRO:
            {
                sort = S8Lit("macro");
            }break;
            
            case MOMO_INDEX_NOTE_KIND_CONSTANT:
            {
                sort = S8Lit("const");
            }break;
            
            case MOMO_INDEX_NOTE_KIND_COMMENT_TAG:
            {
                sort = S8Lit("Tag");
            }break;
            
            case MOMO_INDEX_NOTE_KIND_COMMENT_TODO:
            {
                sort = S8Lit("TODO");
            }break;
            
            default: break;
        }
        
        Momo_Lister_AddItem(lister, display_name, sort, jump, 0);
    }
}


template<typename Pred>
function void 
Momo_Lister_PushNoteWithChildren(Application_Links* app, Arena* arena, Momo_Lister* lister, Momo_Index_Note* parent, Pred pred) {
    if (!parent) {
        return;
    }
    if (pred(parent)) {
        Momo_Lister_PushNote(app, arena, lister, parent);
    }    
    for(Momo_Index_Note *child = parent->first_child; child; child = child->next_sibling) {
        Momo_Lister_PushNoteWithChildren(app, arena, lister, child, pred);
    }
}

///~ Lister creation
function File_Name_Result
Momo_Lister_CreateToGetFilenameFromUser(Application_Links *app, Arena *arena, String_Const_u8 query, View_ID view){
  
    Momo_Lister_Handlers handlers = Momo_Lister_GetDefaultHandlers();
    handlers.refresh = Momo_Lister_PushDirectoryFiles;
    handlers.write_character = lister__write_character__file_path;
    handlers.backspace = lister__backspace_text_field__file_path;

    Momo_Lister_Result l_result = Momo_Lister_RunWithRefreshHandler(app, arena, query, handlers);
    
    File_Name_Result result = {};
    result.canceled = l_result.canceled;
    if (!l_result.canceled){
        result.clicked = l_result.activated_by_click;
        if (l_result.user_data != 0){
            String_Const_u8 name = SCu8((u8*)l_result.user_data);
            result.file_name_activated = name;
            result.is_folder = character_is_slash(string_get_character(name, name.size - 1));
        }
        result.file_name_in_text_field = string_front_of_path(l_result.text_field);
        
        String_Const_u8 path = {};
        if (l_result.user_data == 0 && result.file_name_in_text_field.size == 0 && l_result.text_field.size > 0){
            result.file_name_in_text_field = string_front_folder_of_path(l_result.text_field);
            path = string_remove_front_folder_of_path(l_result.text_field);
        }
        else{
            path = string_remove_front_of_path(l_result.text_field);
        }
        if (character_is_slash(string_get_character(path, path.size - 1))){
            path = string_chop(path, 1);
        }
        result.path_in_text_field = path;
    }
    
    return(result);
}

function File_Name_Result
Momo_Lister_CreateToGetFilenameFromUser(Application_Links *app, Arena *arena, char *query, View_ID view){
    return(Momo_Lister_CreateToGetFilenameFromUser(app, arena, SCu8(query), view));
}



// NOTE(Momo): Pred must be of type: b32(Momo_Index_Note*)
template<typename Pred>
function void
Momo_Lister_CreateWithProjectNotes(Application_Links* app, String_Const_u8 init_str, b32 same_panel, Pred pred){
    char *query = "Search: ";
    
    Scratch_Block scratch(app);
    Momo_Lister_Block lister(app, scratch);
    Momo_Lister_SetQuery(lister, query);
    Momo_Lister_SetKey(lister, init_str);
    Momo_Lister_SetTextField(lister, init_str);
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
                    Momo_Lister_PushNoteWithChildren(app, scratch, lister, note, pred);                   
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
        if(!same_panel)
        {
            view = get_next_view_looped_primary_panels(app, view, Access_Always);
        }
        point_stack_push_view_cursor(app, view);
        momo_jump_to_location(app, view, result.buffer, result.pos);
    }
}

function void
Momo_Lister_CreateWithProjectNotes(Application_Links* app, String_Const_u8 init_str, b32 same_panel)
{
    Momo_Lister_CreateWithProjectNotes(app, init_str, same_panel,   
        [](Momo_Index_Note*) -> b32 { 
            return true; 
    });
}