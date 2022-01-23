//~ NOTE(rjf): Error annotations

static void
momo_render_error_annotations(Application_Links *app, Buffer_ID buffer,
                              Text_Layout_ID text_layout_id,
                              Buffer_ID jump_buffer)
{ 
    if(def_get_config_b32(vars_save_string_lit("f4_disable_error_annotations")))
    {
        return;
    }
    
    ProfileScope(app, "[Fleury] Error Annotations");
    
    Heap *heap = &global_heap;
    Scratch_Block scratch(app);
    
    Locked_Jump_State jump_state = {};
    {
        ProfileScope(app, "[Fleury] Error Annotations (Get Locked Jump State)");
        jump_state = get_locked_jump_state(app, heap);
    }
    
    Face_ID face = global_small_code_face;
    Face_Metrics metrics = get_face_metrics(app, face);
    
    if(jump_buffer != 0 && jump_state.view != 0)
    {
        Managed_Scope buffer_scopes[2];
        {
            ProfileScope(app, "[Fleury] Error Annotations (Buffer Get Managed Scope)");
            buffer_scopes[0] = buffer_get_managed_scope(app, jump_buffer);
            buffer_scopes[1] = buffer_get_managed_scope(app, buffer);
        }
        
        Managed_Scope comp_scope = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Get Managed Scope)");
            comp_scope = get_managed_scope_with_multiple_dependencies(app, buffer_scopes, ArrayCount(buffer_scopes));
        }
        
        Managed_Object *buffer_markers_object = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Scope Attachment)");
            buffer_markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
        }
        
        // NOTE(rjf): Get buffer markers (locations where jumps point at).
        i32 buffer_marker_count = 0;
        Marker *buffer_markers = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Load Managed Object Data)");
            buffer_marker_count = managed_object_get_item_count(app, *buffer_markers_object);
            buffer_markers = push_array(scratch, Marker, buffer_marker_count);
            managed_object_load_data(app, *buffer_markers_object, 0, buffer_marker_count, buffer_markers);
        }
    }
}
