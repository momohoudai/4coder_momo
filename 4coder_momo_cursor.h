global int global_cursor_count = 1;
global i64 global_cursor_positions[16] = {0};
global i64 global_mark_positions[16] = {0};
global int global_hide_region_boundary = 0;

enum Cursor_Type
{
	CURSOR_NONE,
	CURSOR_INSERT,
	CURSOR_OPEN_RANGE,
	CURSOR_CLOSE_RANGE,
};


function void
Momo_Cursor_Render(Application_Links *app, View_ID view_id, b32 is_active_view,
                           Buffer_ID buffer, Text_Layout_ID text_layout_id,
                           f32 roundness, f32 outline_thickness, Frame_Info frame_info);