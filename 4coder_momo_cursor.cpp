
#ifndef FCODER_MOMO_CURSOR_H
#define FCODER_MOMO_CURSOR_H

//~ NOTE(rjf): Cursor rendering
internal void
_momo_render_cursor_symbol(Application_Links *app, Rect_f32 rect,
                            f32 roundness, f32 thickness,
                            ARGB_Color color, Cursor_Type type)
{
    f32 line_height = rect.y1 - rect.y0;
    f32 bracket_width = 0.5f*line_height;
    
    if(type == CURSOR_OPEN_RANGE)
    {
        Rect_f32 start_top, start_side, start_bottom;
        
		Vec2_f32 start_p = {rect.x0, rect.y0};
        
        start_top.x0 = start_p.x + thickness;
        start_top.x1 = start_p.x + bracket_width;
        start_top.y0 = start_p.y;
        start_top.y1 = start_p.y + thickness;
        
        start_bottom.x0 = start_top.x0;
        start_bottom.x1 = start_top.x1;
        start_bottom.y1 = start_p.y + line_height;
        start_bottom.y0 = start_bottom.y1 - thickness;
        
        start_side.x0 = start_p.x;
        start_side.x1 = start_p.x + thickness;
        start_side.y0 = start_top.y0;
        start_side.y1 = start_bottom.y1;
        
        draw_rectangle(app, start_top, roundness, color);
        draw_rectangle(app, start_side, roundness, color);
        
        // draw_rectangle(app, start_bottom, start_color);
    }
    else if(type == CURSOR_CLOSE_RANGE)
	{
		Rect_f32 end_top, end_side, end_bottom;
        
		Vec2_f32 end_p = {rect.x0, rect.y0};
        
		end_top.x0 = end_p.x;
		end_top.x1 = end_p.x - bracket_width;
		end_top.y0 = end_p.y;
		end_top.y1 = end_p.y + thickness;
		
		end_side.x1 = end_p.x;
		end_side.x0 = end_p.x + thickness;
		end_side.y0 = end_p.y;
		end_side.y1 = end_p.y + line_height;
        
		end_bottom.x0 = end_top.x0;
		end_bottom.x1 = end_top.x1;
		end_bottom.y1 = end_p.y + line_height;
		end_bottom.y0 = end_bottom.y1 - thickness;
		
		draw_rectangle(app, end_bottom, roundness, color);
		draw_rectangle(app, end_side, roundness, color);
	}
	else if(type == CURSOR_INSERT)
	{
		Rect_f32 side;
		side.x0 = rect.x0;
		side.x1 = rect.x0 + thickness;
		side.y0 = rect.y0;
		side.y1 = rect.y1;
        
		draw_rectangle(app, side, roundness, color);
	}
}

internal void
_momo_do_cursor_interpolation(Application_Links *app, Frame_Info frame_info,
                         Rect_f32 *rect, Rect_f32 *last_rect, Rect_f32 target)
{
    *last_rect = *rect;
    
    f32 x_change = target.x0 - rect->x0;
    f32 y_change = target.y0 - rect->y0;
    
    f32 cursor_size_x = (target.x1 - target.x0);
    f32 cursor_size_y = (target.y1 - target.y0) * (1 + fabsf(y_change) / 60.f);
    
    b32 should_animate_cursor = !global_battery_saver && !def_get_config_b32(vars_save_string_lit("f4_disable_cursor_trails"));
    if(should_animate_cursor)
    {
        if(fabs(x_change) > 1.f || fabs(y_change) > 1.f)
        {
            animate_in_n_milliseconds(app, 0);
        }
    }
    else
    {
        *rect = *last_rect = target;
        cursor_size_y = target.y1 - target.y0;
    }
    
    if(should_animate_cursor)
    {
        rect->x0 += (x_change) * frame_info.animation_dt * 30.f;
        rect->y0 += (y_change) * frame_info.animation_dt * 30.f;
        rect->x1 = rect->x0 + cursor_size_x;
        rect->y1 = rect->y0 + cursor_size_y;
    }
    
    if(target.y0 > last_rect->y0)
    {
        if(rect->y0 < last_rect->y0)
        {
            rect->y0 = last_rect->y0;
        }
    }
    else
    {
        if(rect->y1 > last_rect->y1)
        {
            rect->y1 = last_rect->y1;
        }
    }
    
}

static void
momo_render_cursor(Application_Links *app, View_ID view_id, b32 is_active_view,
                           Buffer_ID buffer, Text_Layout_ID text_layout_id,
                           f32 roundness, f32 outline_thickness, Frame_Info frame_info)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 clip = draw_set_clip(app, view_rect);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    b32 has_highlight_range = draw_highlight_range(app, view_id, buffer, text_layout_id, roundness);
    
    ARGB_Color cursor_color = momo_get_cursor_color(app, global_keybinding_mode, global_keyboard_macro_is_recording);
    ARGB_Color mark_color = cursor_color;
    ARGB_Color inactive_cursor_color = momo_argb_from_id(active_color_table, fleury_color_cursor_inactive, 0);
    
    if(!momo_is_argb_valid(inactive_cursor_color))
    {
        inactive_cursor_color = cursor_color;
    }
    
    if(is_active_view == 0)
    {
        cursor_color = inactive_cursor_color;
        mark_color = inactive_cursor_color;
    }
	
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 mark_pos = view_get_mark_pos(app, view_id);

    if(!has_highlight_range)
    {
        
		Cursor_Type cursor_type = CURSOR_NONE;
		Cursor_Type mark_type = CURSOR_NONE;
		if(cursor_pos <= mark_pos)
		{
			cursor_type = CURSOR_OPEN_RANGE;
			mark_type = CURSOR_CLOSE_RANGE;
		}
		else
		{
			cursor_type = CURSOR_CLOSE_RANGE;
			mark_type = CURSOR_OPEN_RANGE;
		}
		
		if(global_hide_region_boundary)
		{
			cursor_type = CURSOR_INSERT;
			mark_type = CURSOR_NONE;
		}
        
        Rect_f32 target_cursor = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
        Rect_f32 target_mark = text_layout_character_on_screen(app, text_layout_id, mark_pos);
		
        // NOTE(rjf): Draw cursor.
        {
            if(is_active_view)
            {
                
                if(cursor_pos < visible_range.start || cursor_pos > visible_range.end)
                {
                    f32 width = target_cursor.x1 - target_cursor.x0;
                    target_cursor.x0 = view_rect.x0;
                    target_cursor.x1 = target_cursor.x0 + width;
                }
                
                _momo_do_cursor_interpolation(app, frame_info, &global_cursor_rect,
                                         &global_last_cursor_rect, target_cursor);
                
                
                if(mark_pos > visible_range.end)
                {
                    target_mark.x0 = 0;
                    target_mark.y0 = view_rect.y1;
                    target_mark.y1 = view_rect.y1;
                }
                
                if(mark_pos < visible_range.start || mark_pos > visible_range.end)
                {
                    f32 width = target_mark.x1 - target_mark.x0;
                    target_mark.x0 = view_rect.x0;
                    target_mark.x1 = target_mark.x0 + width;
                }
                
                _momo_do_cursor_interpolation(app, frame_info, &global_mark_rect, &global_last_mark_rect,
                                         target_mark);
            }
            
            // NOTE(rjf): Draw main cursor.
            {
                _momo_render_cursor_symbol(app, global_cursor_rect, roundness, 2.f, cursor_color, cursor_type);
				_momo_render_cursor_symbol(app, target_cursor, roundness, 2.f, cursor_color, cursor_type);
            }
            
            // NOTE(rjf): GLOW IT UP
            for(int glow = 0; glow < 20; ++glow)
            {
                f32 alpha = 0.1f - (glow*0.015f);
                if(alpha > 0)
                {
                    Rect_f32 glow_rect = target_cursor;
                    glow_rect.x0 -= glow;
                    glow_rect.y0 -= glow;
                    glow_rect.x1 += glow;
                    glow_rect.y1 += glow;
                    _momo_render_cursor_symbol(app, glow_rect, roundness + glow*0.7f, 2.f,
                                                fcolor_resolve(fcolor_change_alpha(fcolor_argb(cursor_color), alpha)), cursor_type);
                }
                else
                {
                    break;
                }
            }
            
        }
        
        // paint_text_color_pos(app, text_layout_id, cursor_pos,
        // fcolor_id(defcolor_at_cursor));
        _momo_render_cursor_symbol(app, global_mark_rect, roundness, 2.f,
                                    fcolor_resolve(fcolor_change_alpha(fcolor_argb(mark_color), 0.5f)), mark_type);
		_momo_render_cursor_symbol(app, target_mark, roundness, 2.f,
                                    fcolor_resolve(fcolor_change_alpha(fcolor_argb(mark_color), 0.75f)), mark_type);
        
    }
    
    draw_set_clip(app, clip);
}


static void
momo_hook_highlight_cursor_mark_range(Application_Links *app, View_ID view_id)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 clip = draw_set_clip(app, view_rect);
    
    f32 lower_bound_y;
    f32 upper_bound_y;
    
    if(global_last_cursor_rect.y0 < global_last_mark_rect.y0)
    {
        lower_bound_y = global_last_cursor_rect.y0;
        upper_bound_y = global_last_mark_rect.y1;
    }
    else
    {
        lower_bound_y = global_last_mark_rect.y0;
        upper_bound_y = global_last_cursor_rect.y1;
    }
    
    draw_rectangle(app, Rf32(view_rect.x0, lower_bound_y, view_rect.x0 + 4, upper_bound_y), 3.f,
                   fcolor_resolve(fcolor_change_alpha(fcolor_id(defcolor_comment), 0.5f)));
    draw_set_clip(app, clip);
}


#endif