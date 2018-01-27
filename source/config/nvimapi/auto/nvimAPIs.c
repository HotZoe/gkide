/// nvim API functions, indexed by SnailNvimQt::NvimApiFuncID
///
/// @note also see NvimApiFuncID
///
/// Auto generated: UTC 2018-01-27 21:52:25.660385
const QList<NvimApiFunc> NvimApiFunc::nvimAPIs = QList<NvimApiFunc>()
<< NvimApiFunc("Integer", // return type
               "nvim_buf_line_count", // function name
	           QList<QString>()
                   << QString("Buffer"),
	           false)
<< NvimApiFunc("String", // return type
               "buffer_get_line", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer"),
	           false)
<< NvimApiFunc("void", // return type
               "buffer_set_line", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "buffer_del_line", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer"),
	           false)
<< NvimApiFunc("ArrayOf(String)", // return type
               "buffer_get_line_slice", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Boolean")
                   << QString("Boolean"),
	           false)
<< NvimApiFunc("ArrayOf(String)", // return type
               "nvim_buf_get_lines", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Boolean"),
	           false)
<< NvimApiFunc("void", // return type
               "buffer_set_line_slice", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Boolean")
                   << QString("Boolean")
                   << QString("ArrayOf(String)"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_buf_set_lines", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Boolean")
                   << QString("ArrayOf(String)"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_buf_get_var", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_buf_get_changedtick", // function name
	           QList<QString>()
                   << QString("Buffer"),
	           false)
<< NvimApiFunc("ArrayOf(Dictionary)", // return type
               "nvim_buf_get_keymap", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_buf_set_var", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_buf_del_var", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "buffer_set_var", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("Object", // return type
               "buffer_del_var", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_buf_get_option", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_buf_set_option", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_buf_get_number", // function name
	           QList<QString>()
                   << QString("Buffer"),
	           false)
<< NvimApiFunc("String", // return type
               "nvim_buf_get_name", // function name
	           QList<QString>()
                   << QString("Buffer"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_buf_set_name", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String"),
	           false)
<< NvimApiFunc("Boolean", // return type
               "nvim_buf_is_valid", // function name
	           QList<QString>()
                   << QString("Buffer"),
	           false)
<< NvimApiFunc("void", // return type
               "buffer_insert", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("ArrayOf(String)"),
	           false)
<< NvimApiFunc("ArrayOf(Integer, 2)", // return type
               "nvim_buf_get_mark", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("String"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_buf_add_highlight", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("String")
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Integer"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_buf_clear_highlight", // function name
	           QList<QString>()
                   << QString("Buffer")
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Integer"),
	           false)
<< NvimApiFunc("ArrayOf(Window)", // return type
               "nvim_tabpage_list_wins", // function name
	           QList<QString>()
                   << QString("Tabpage"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_tabpage_get_var", // function name
	           QList<QString>()
                   << QString("Tabpage")
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_tabpage_set_var", // function name
	           QList<QString>()
                   << QString("Tabpage")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_tabpage_del_var", // function name
	           QList<QString>()
                   << QString("Tabpage")
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "tabpage_set_var", // function name
	           QList<QString>()
                   << QString("Tabpage")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("Object", // return type
               "tabpage_del_var", // function name
	           QList<QString>()
                   << QString("Tabpage")
                   << QString("String"),
	           false)
<< NvimApiFunc("Window", // return type
               "nvim_tabpage_get_win", // function name
	           QList<QString>()
                   << QString("Tabpage"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_tabpage_get_number", // function name
	           QList<QString>()
                   << QString("Tabpage"),
	           false)
<< NvimApiFunc("Boolean", // return type
               "nvim_tabpage_is_valid", // function name
	           QList<QString>()
                   << QString("Tabpage"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_ui_attach", // function name
	           QList<QString>()
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Dictionary"),
	           false)
<< NvimApiFunc("void", // return type
               "ui_attach", // function name
	           QList<QString>()
                   << QString("Integer")
                   << QString("Integer")
                   << QString("Boolean"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_ui_detach", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_ui_try_resize", // function name
	           QList<QString>()
                   << QString("Integer")
                   << QString("Integer"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_ui_set_option", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_command", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_feedkeys", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("String")
                   << QString("Boolean"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_input", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("String", // return type
               "nvim_replace_termcodes", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("Boolean")
                   << QString("Boolean")
                   << QString("Boolean"),
	           false)
<< NvimApiFunc("String", // return type
               "nvim_command_output", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_eval", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_call_function", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("Array"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_execute_lua", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("Array"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_strwidth", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("ArrayOf(String)", // return type
               "nvim_list_runtime_paths", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_set_current_dir", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("String", // return type
               "nvim_get_current_line", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_set_current_line", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_del_current_line", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_get_var", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_set_var", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_del_var", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "vim_set_var", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("Object", // return type
               "vim_del_var", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_get_vvar", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_get_option", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_set_option", // function name
	           QList<QString>()
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_out_write", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_errmsg_write", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_errmsg_writeln", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("ArrayOf(Buffer)", // return type
               "nvim_list_bufs", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("Buffer", // return type
               "nvim_get_current_buf", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_set_current_buf", // function name
	           QList<QString>()
                   << QString("Buffer"),
	           false)
<< NvimApiFunc("ArrayOf(Window)", // return type
               "nvim_list_wins", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("Window", // return type
               "nvim_get_current_win", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_set_current_win", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("ArrayOf(Tabpage)", // return type
               "nvim_list_tabpages", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("Tabpage", // return type
               "nvim_get_current_tabpage", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_set_current_tabpage", // function name
	           QList<QString>()
                   << QString("Tabpage"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_subscribe", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_unsubscribe", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_get_color_by_name", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Dictionary", // return type
               "nvim_get_color_map", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("Dictionary", // return type
               "nvim_get_mode", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("ArrayOf(Dictionary)", // return type
               "nvim_get_keymap", // function name
	           QList<QString>()
                   << QString("String"),
	           false)
<< NvimApiFunc("Array", // return type
               "nvim_get_api_info", // function name
	           QList<QString>(),
	           false)
<< NvimApiFunc("Array", // return type
               "nvim_call_atomic", // function name
	           QList<QString>()
                   << QString("Array"),
	           false)
<< NvimApiFunc("Buffer", // return type
               "nvim_win_get_buf", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("ArrayOf(Integer, 2)", // return type
               "nvim_win_get_cursor", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_win_set_cursor", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("ArrayOf(Integer, 2)"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_win_get_height", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_win_set_height", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("Integer"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_win_get_width", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_win_set_width", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("Integer"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_win_get_var", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_win_set_var", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_win_del_var", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "window_set_var", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("Object", // return type
               "window_del_var", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("String"),
	           false)
<< NvimApiFunc("Object", // return type
               "nvim_win_get_option", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("String"),
	           false)
<< NvimApiFunc("void", // return type
               "nvim_win_set_option", // function name
	           QList<QString>()
                   << QString("Window")
                   << QString("String")
                   << QString("Object"),
	           false)
<< NvimApiFunc("ArrayOf(Integer, 2)", // return type
               "nvim_win_get_position", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("Tabpage", // return type
               "nvim_win_get_tabpage", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("Integer", // return type
               "nvim_win_get_number", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
<< NvimApiFunc("Boolean", // return type
               "nvim_win_is_valid", // function name
	           QList<QString>()
                   << QString("Window"),
	           false)
;
