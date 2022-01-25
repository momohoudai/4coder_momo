enum Momo_KeybindingMode
{
	Momo_KeyBindingMode_Default,

	Momo_KeyBindingMode_Normal,
    Momo_KeyBindingMode_Insert,
    //Momo_KeyBindingMode_Visual,	

    Momo_KeyBindingMode_Count
};

enum Momo_CommandMapType
{
	Momo_CommandMapType_Global,
	Momo_CommandMapType_File,
    Momo_CommandMapType_Code,
	
    Momo_CommandMapType_Count
};

static Momo_KeybindingMode global_keybinding_mode = {};
static Command_Map_ID global_command_maps[Momo_KeyBindingMode_Count][Momo_CommandMapType_Count] ={};

function Implicit_Map_Result momo_get_keybinding_implicit_map(Application_Links *app, String_ID lang, String_ID mode, Input_Event *event);
function void momo_init_keybindings(Mapping *mapping);