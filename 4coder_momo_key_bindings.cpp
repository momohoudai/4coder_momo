
#ifndef FCODER_MOMO_KEY_BINDINGS_H
#define FCODER_MOMO_KEY_BINDINGS_H


function Implicit_Map_Result
Momo_KeyBinding_ImplicitMap(Application_Links *app, String_ID lang, String_ID mode, Input_Event *event)
{

	Implicit_Map_Result result = {};
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    
   	Command_Map_ID orig_id = default_get_map_id(app, view);
	Command_Map_ID map_id = orig_id;

	for (u32 index = 0; 
		index < ArrayCount(global_command_maps[Momo_KeyBindingMode_Default]);
		++index)
	{
		if (global_command_maps[Momo_KeyBindingMode_Default][index] == map_id) {
			map_id = global_command_maps[global_keybinding_mode][index];
			break;
		}
	}


	Command_Binding binding = map_get_binding_recursive(&framework_mapping, map_id, event);
	if(!binding.custom)
	{
		binding = map_get_binding_recursive(&framework_mapping, orig_id, event);
	}
    
    // TODO(allen): map_id <-> map name?
    result.map = 0;
    result.command = binding.custom;
    

    return(result);
}


//~ NOTE(rjf): Bindings
function void
Momo_KeyBindings_Init(Mapping *mapping)
{
    String_ID global_map_id = vars_save_string_lit("keys_global");
    String_ID file_map_id = vars_save_string_lit("keys_file");
    String_ID code_map_id = vars_save_string_lit("keys_code");
        
	implicit_map_function = Momo_KeyBinding_ImplicitMap;
	
	MappingScope();
    SelectMapping(mapping);
    
	// 'Core' keys binding
    SelectMap(global_map_id);
    BindCore(momo_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
    
    SelectMap(file_map_id);
    ParentMap(global_map_id);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);
    
    SelectMap(code_map_id);
    ParentMap(file_map_id);
   
    
	global_command_maps[Momo_KeyBindingMode_Default][Momo_CommandMapType_Global] = global_map_id;
	global_command_maps[Momo_KeyBindingMode_Default][Momo_CommandMapType_File] = file_map_id;
	global_command_maps[Momo_KeyBindingMode_Default][Momo_CommandMapType_Code] = code_map_id;


	// 'Normal' mode bindings
	String_ID global_normal_map_id = vars_save_string_lit("keys_normal_global");
	String_ID file_normal_map_id = vars_save_string_lit("keys_normal_file");
    String_ID code_normal_map_id = vars_save_string_lit("keys_normal_code");
	SelectMap(global_normal_map_id);
	ParentMap(global_map_id);
   
    SelectMap(file_normal_map_id);
	ParentMap(global_normal_map_id);
    
	SelectMap(code_normal_map_id);
	ParentMap(file_normal_map_id);


    global_command_maps[Momo_KeyBindingMode_Normal][Momo_CommandMapType_Global] = global_normal_map_id;
	global_command_maps[Momo_KeyBindingMode_Normal][Momo_CommandMapType_File] = file_normal_map_id;
	global_command_maps[Momo_KeyBindingMode_Normal][Momo_CommandMapType_Code] = code_normal_map_id;


	// 'Insert' mode bindings
	String_ID global_insert_map_id = vars_save_string_lit("keys_insert_global");
	String_ID file_insert_map_id = vars_save_string_lit("keys_insert_file");
    String_ID code_insert_map_id = vars_save_string_lit("keys_insert_code");
	SelectMap(global_insert_map_id);
	ParentMap(global_map_id);
    
	SelectMap(file_insert_map_id);
	ParentMap(global_insert_map_id);
	BindTextInput(momo_write_text_input);

    SelectMap(code_insert_map_id);
	ParentMap(file_insert_map_id);
	BindTextInput(momo_write_text_and_indent);

    global_command_maps[Momo_KeyBindingMode_Insert][Momo_CommandMapType_Global] = global_insert_map_id;
	global_command_maps[Momo_KeyBindingMode_Insert][Momo_CommandMapType_File] = file_insert_map_id;
	global_command_maps[Momo_KeyBindingMode_Insert][Momo_CommandMapType_Code] = code_insert_map_id;

	// Starting default bindings
	global_keybinding_mode = Momo_KeyBindingMode_Normal;
}

#endif