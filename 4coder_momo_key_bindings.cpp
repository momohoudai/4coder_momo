
#ifndef FCODER_MOMO_KEY_BINDINGS_H
#define FCODER_MOMO_KEY_BINDINGS_H

// TODO(Momo): We should really refactor this. 
// I'm not entirely sure if we need the "momo_set_default_bindings" function etc.
// Consider moving the commands to 4coder_momo_commands too
//



CUSTOM_COMMAND_SIG(switch_to_keybinding_0)
CUSTOM_DOC("Switch the keybindings to mode 0.")
{
	GlobalKeybindingMode = KeyBindingMode_0;
}

CUSTOM_COMMAND_SIG(switch_to_keybinding_1)
CUSTOM_DOC("Switch the keybindings to mode 1.")
{
	GlobalKeybindingMode = KeyBindingMode_1;
}

CUSTOM_COMMAND_SIG(switch_to_keybinding_2)
CUSTOM_DOC("Switch the keybindings to mode 2.")
{
	GlobalKeybindingMode = KeyBindingMode_2;
}

CUSTOM_COMMAND_SIG(switch_to_keybinding_3)
CUSTOM_DOC("Switch the keybindings to mode 3.")
{
	GlobalKeybindingMode = KeyBindingMode_3;
}

function Implicit_Map_Result
momo_implicit_map(Application_Links *app, String_ID lang, String_ID mode, Input_Event *event)
{
    Implicit_Map_Result result = {};
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    
	Command_Map_ID orig_id = default_get_map_id(app, view);
    Command_Map_ID map_id = orig_id;
	if(GlobalKeybindingMode == KeyBindingMode_1)
	{
		for(int PairIndex = 0;
			PairIndex < ArrayCount(GlobalCommandMapReroute);
			++PairIndex)
		{
			if(GlobalCommandMapReroute[PairIndex].From == map_id)
			{
				map_id = GlobalCommandMapReroute[PairIndex].To;
				break;
			}
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
momo_set_essential_bindings(Mapping *mapping)
{
    String_ID global_map_id = vars_save_string_lit("keys_global");
    String_ID file_map_id = vars_save_string_lit("keys_file");
    String_ID code_map_id = vars_save_string_lit("keys_code");
    
	String_ID global_command_map_id = vars_save_string_lit("keys_global_1");
	String_ID file_command_map_id = vars_save_string_lit("keys_file_1");
    String_ID code_command_map_id = vars_save_string_lit("keys_code_1");
    
	implicit_map_function = momo_implicit_map;
	
	MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_map_id);
    BindCore(momo_startup, CoreCode_Startup);
    BindCore(default_try_exit, CoreCode_TryExit);
    Bind(exit_4coder,          KeyCode_F4, KeyCode_Alt);
    BindMouseWheel(mouse_wheel_scroll);
    BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
    
    SelectMap(file_map_id);
    ParentMap(global_map_id);
    BindTextInput(momo_write_text_input);
    BindMouse(click_set_cursor_and_mark, MouseCode_Left);
    BindMouseRelease(click_set_cursor, MouseCode_Left);
    BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
    BindMouseMove(click_set_cursor_if_lbutton);
    
    SelectMap(code_map_id);
    ParentMap(file_map_id);
    BindTextInput(momo_write_text_and_auto_indent);
    
    SelectMap(global_command_map_id);
	ParentMap(global_map_id);
	GlobalCommandMapReroute[0].From = global_map_id;
	GlobalCommandMapReroute[0].To = global_command_map_id;
	
    SelectMap(file_command_map_id);
	ParentMap(global_command_map_id);
	GlobalCommandMapReroute[1].From = file_map_id;
	GlobalCommandMapReroute[1].To = file_command_map_id;
	
    SelectMap(code_command_map_id);
	ParentMap(file_command_map_id);
	GlobalCommandMapReroute[2].From = code_map_id;
	GlobalCommandMapReroute[2].To = code_command_map_id;
    
}

#endif