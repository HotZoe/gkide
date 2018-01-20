// Auto generated 2017-07-12 22:09:26.826170
const QList<NvimApiFunc> NvimApiFunc::nvimAPIs = QList<NvimApiFunc>()
<< NvimApiFunc("Integer", "nvim_buf_line_count",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("String", "buffer_get_line",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("void", "buffer_set_line",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "buffer_del_line",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("ArrayOf(String)", "buffer_get_line_slice",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Boolean")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("ArrayOf(String)", "nvim_buf_get_lines",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("void", "buffer_set_line_slice",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Boolean")
			<< QString("Boolean")
			<< QString("ArrayOf(String)")
		, false)
<< NvimApiFunc("void", "nvim_buf_set_lines",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Boolean")
			<< QString("ArrayOf(String)")
		, false)
<< NvimApiFunc("Object", "nvim_buf_get_var",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Integer", "nvim_buf_get_changedtick",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("ArrayOf(Dictionary)", "nvim_buf_get_keymap",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_buf_set_var",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("void", "nvim_buf_del_var",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "buffer_set_var",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("Object", "buffer_del_var",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "nvim_buf_get_option",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_buf_set_option",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("Integer", "nvim_buf_get_number",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("String", "nvim_buf_get_name",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("void", "nvim_buf_set_name",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Boolean", "nvim_buf_is_valid",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("void", "buffer_insert",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("ArrayOf(String)")
		, false)
<< NvimApiFunc("ArrayOf(Integer, 2)", "nvim_buf_get_mark",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Integer", "nvim_buf_add_highlight",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("String")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("void", "nvim_buf_clear_highlight",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("ArrayOf(Window)", "nvim_tabpage_list_wins",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("Object", "nvim_tabpage_get_var",
	QList<QString>()
			<< QString("Tabpage")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_tabpage_set_var",
	QList<QString>()
			<< QString("Tabpage")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("void", "nvim_tabpage_del_var",
	QList<QString>()
			<< QString("Tabpage")
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "tabpage_set_var",
	QList<QString>()
			<< QString("Tabpage")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("Object", "tabpage_del_var",
	QList<QString>()
			<< QString("Tabpage")
			<< QString("String")
		, false)
<< NvimApiFunc("Window", "nvim_tabpage_get_win",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("Integer", "nvim_tabpage_get_number",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("Boolean", "nvim_tabpage_is_valid",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("void", "nvim_ui_attach",
	QList<QString>()
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Dictionary")
		, false)
<< NvimApiFunc("void", "ui_attach",
	QList<QString>()
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("void", "nvim_ui_detach",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "nvim_ui_try_resize",
	QList<QString>()
			<< QString("Integer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("void", "nvim_ui_set_option",
	QList<QString>()
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("void", "nvim_command",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_feedkeys",
	QList<QString>()
			<< QString("String")
			<< QString("String")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("Integer", "nvim_input",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("String", "nvim_replace_termcodes",
	QList<QString>()
			<< QString("String")
			<< QString("Boolean")
			<< QString("Boolean")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("String", "nvim_command_output",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "nvim_eval",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "nvim_call_function",
	QList<QString>()
			<< QString("String")
			<< QString("Array")
		, false)
<< NvimApiFunc("Object", "nvim_execute_lua",
	QList<QString>()
			<< QString("String")
			<< QString("Array")
		, false)
<< NvimApiFunc("Integer", "nvim_strwidth",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("ArrayOf(String)", "nvim_list_runtime_paths",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "nvim_set_current_dir",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("String", "nvim_get_current_line",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "nvim_set_current_line",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_del_current_line",
	QList<QString>()
		, false)
<< NvimApiFunc("Object", "nvim_get_var",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_set_var",
	QList<QString>()
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("void", "nvim_del_var",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "vim_set_var",
	QList<QString>()
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("Object", "vim_del_var",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "nvim_get_vvar",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "nvim_get_option",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_set_option",
	QList<QString>()
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("void", "nvim_out_write",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_err_write",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_err_writeln",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("ArrayOf(Buffer)", "nvim_list_bufs",
	QList<QString>()
		, false)
<< NvimApiFunc("Buffer", "nvim_get_current_buf",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "nvim_set_current_buf",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("ArrayOf(Window)", "nvim_list_wins",
	QList<QString>()
		, false)
<< NvimApiFunc("Window", "nvim_get_current_win",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "nvim_set_current_win",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("ArrayOf(Tabpage)", "nvim_list_tabpages",
	QList<QString>()
		, false)
<< NvimApiFunc("Tabpage", "nvim_get_current_tabpage",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "nvim_set_current_tabpage",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("void", "nvim_subscribe",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_unsubscribe",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Integer", "nvim_get_color_by_name",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Dictionary", "nvim_get_color_map",
	QList<QString>()
		, false)
<< NvimApiFunc("Dictionary", "nvim_get_mode",
	QList<QString>()
		, false)
<< NvimApiFunc("ArrayOf(Dictionary)", "nvim_get_keymap",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Array", "nvim_get_api_info",
	QList<QString>()
		, false)
<< NvimApiFunc("Array", "nvim_call_atomic",
	QList<QString>()
			<< QString("Array")
		, false)
<< NvimApiFunc("Buffer", "nvim_win_get_buf",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("ArrayOf(Integer, 2)", "nvim_win_get_cursor",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("void", "nvim_win_set_cursor",
	QList<QString>()
			<< QString("Window")
			<< QString("ArrayOf(Integer, 2)")
		, false)
<< NvimApiFunc("Integer", "nvim_win_get_height",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("void", "nvim_win_set_height",
	QList<QString>()
			<< QString("Window")
			<< QString("Integer")
		, false)
<< NvimApiFunc("Integer", "nvim_win_get_width",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("void", "nvim_win_set_width",
	QList<QString>()
			<< QString("Window")
			<< QString("Integer")
		, false)
<< NvimApiFunc("Object", "nvim_win_get_var",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_win_set_var",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("void", "nvim_win_del_var",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "window_set_var",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("Object", "window_del_var",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "nvim_win_get_option",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "nvim_win_set_option",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("ArrayOf(Integer, 2)", "nvim_win_get_position",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("Tabpage", "nvim_win_get_tabpage",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("Integer", "nvim_win_get_number",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("Boolean", "nvim_win_is_valid",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("Integer", "buffer_line_count",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("ArrayOf(String)", "buffer_get_lines",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("void", "buffer_set_lines",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Boolean")
			<< QString("ArrayOf(String)")
		, false)
<< NvimApiFunc("Object", "buffer_get_var",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "buffer_get_option",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "buffer_set_option",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("Integer", "buffer_get_number",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("String", "buffer_get_name",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("void", "buffer_set_name",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Boolean", "buffer_is_valid",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("ArrayOf(Integer, 2)", "buffer_get_mark",
	QList<QString>()
			<< QString("Buffer")
			<< QString("String")
		, false)
<< NvimApiFunc("Integer", "buffer_add_highlight",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("String")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("void", "buffer_clear_highlight",
	QList<QString>()
			<< QString("Buffer")
			<< QString("Integer")
			<< QString("Integer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("ArrayOf(Window)", "tabpage_get_windows",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("Object", "tabpage_get_var",
	QList<QString>()
			<< QString("Tabpage")
			<< QString("String")
		, false)
<< NvimApiFunc("Window", "tabpage_get_window",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("Boolean", "tabpage_is_valid",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("void", "ui_detach",
	QList<QString>()
		, false)
<< NvimApiFunc("Object", "ui_try_resize",
	QList<QString>()
			<< QString("Integer")
			<< QString("Integer")
		, false)
<< NvimApiFunc("void", "vim_command",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "vim_feedkeys",
	QList<QString>()
			<< QString("String")
			<< QString("String")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("Integer", "vim_input",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("String", "vim_replace_termcodes",
	QList<QString>()
			<< QString("String")
			<< QString("Boolean")
			<< QString("Boolean")
			<< QString("Boolean")
		, false)
<< NvimApiFunc("String", "vim_command_output",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "vim_eval",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "vim_call_function",
	QList<QString>()
			<< QString("String")
			<< QString("Array")
		, false)
<< NvimApiFunc("Integer", "vim_strwidth",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("ArrayOf(String)", "vim_list_runtime_paths",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "vim_change_directory",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("String", "vim_get_current_line",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "vim_set_current_line",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "vim_del_current_line",
	QList<QString>()
		, false)
<< NvimApiFunc("Object", "vim_get_var",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "vim_get_vvar",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "vim_get_option",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "vim_set_option",
	QList<QString>()
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("void", "vim_out_write",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "vim_err_write",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "vim_report_error",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("ArrayOf(Buffer)", "vim_get_buffers",
	QList<QString>()
		, false)
<< NvimApiFunc("Buffer", "vim_get_current_buffer",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "vim_set_current_buffer",
	QList<QString>()
			<< QString("Buffer")
		, false)
<< NvimApiFunc("ArrayOf(Window)", "vim_get_windows",
	QList<QString>()
		, false)
<< NvimApiFunc("Window", "vim_get_current_window",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "vim_set_current_window",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("ArrayOf(Tabpage)", "vim_get_tabpages",
	QList<QString>()
		, false)
<< NvimApiFunc("Tabpage", "vim_get_current_tabpage",
	QList<QString>()
		, false)
<< NvimApiFunc("void", "vim_set_current_tabpage",
	QList<QString>()
			<< QString("Tabpage")
		, false)
<< NvimApiFunc("void", "vim_subscribe",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("void", "vim_unsubscribe",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Integer", "vim_name_to_color",
	QList<QString>()
			<< QString("String")
		, false)
<< NvimApiFunc("Dictionary", "vim_get_color_map",
	QList<QString>()
		, false)
<< NvimApiFunc("Buffer", "window_get_buffer",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("ArrayOf(Integer, 2)", "window_get_cursor",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("void", "window_set_cursor",
	QList<QString>()
			<< QString("Window")
			<< QString("ArrayOf(Integer, 2)")
		, false)
<< NvimApiFunc("Integer", "window_get_height",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("void", "window_set_height",
	QList<QString>()
			<< QString("Window")
			<< QString("Integer")
		, false)
<< NvimApiFunc("Integer", "window_get_width",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("void", "window_set_width",
	QList<QString>()
			<< QString("Window")
			<< QString("Integer")
		, false)
<< NvimApiFunc("Object", "window_get_var",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
		, false)
<< NvimApiFunc("Object", "window_get_option",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
		, false)
<< NvimApiFunc("void", "window_set_option",
	QList<QString>()
			<< QString("Window")
			<< QString("String")
			<< QString("Object")
		, false)
<< NvimApiFunc("ArrayOf(Integer, 2)", "window_get_position",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("Tabpage", "window_get_tabpage",
	QList<QString>()
			<< QString("Window")
		, false)
<< NvimApiFunc("Boolean", "window_is_valid",
	QList<QString>()
			<< QString("Window")
		, false)
	;
