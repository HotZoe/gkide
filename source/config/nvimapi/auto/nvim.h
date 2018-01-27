/// @file config/nvimapi/auto/nvim.h
///
/// Auto generated: UTC 2018-01-27 21:52:25.665392

#ifndef CONFIG_NVIMAPI_AUTO_NVIM_H
#define CONFIG_NVIMAPI_AUTO_NVIM_H

#include <msgpack.h>
#include "plugins/bin/snail/nvimapi.h"

namespace SnailNvimQt {

class NvimConnector;
class MsgpackRequest;

class Nvim: public QObject
{
    Q_OBJECT
public:
    Nvim(NvimConnector *);

protected slots:
    void handleResponse(quint32 id, NvimApiFuncID fun, const QVariant &);
    void handleResponseError(quint32 id, NvimApiFuncID fun, const QVariant &);

signals:
    void error(const QString &errmsg, const QVariant &errObj);
    void neovimNotification(const QByteArray &name, const QVariantList &args);

private:
    NvimConnector *m_c;

public slots:
    
    /// nvim API:
    /// Integer nvim_buf_line_count(Buffer buffer);
    MsgpackRequest *nvim_buf_line_count(int64_t buffer);
    
    /// @deprecated nvim API:
    /// String buffer_get_line(Buffer buffer,
    ///                        Integer index);
    MsgpackRequest *buffer_get_line(int64_t buffer,
                                    int64_t index);
    
    /// @deprecated nvim API:
    /// void buffer_set_line(Buffer buffer,
    ///                      Integer index,
    ///                      String line);
    MsgpackRequest *buffer_set_line(int64_t buffer,
                                    int64_t index,
                                    QByteArray line);
    
    /// @deprecated nvim API:
    /// void buffer_del_line(Buffer buffer,
    ///                      Integer index);
    MsgpackRequest *buffer_del_line(int64_t buffer,
                                    int64_t index);
    
    /// @deprecated nvim API:
    /// ArrayOf(String) buffer_get_line_slice(Buffer buffer,
    ///                                       Integer start,
    ///                                       Integer end,
    ///                                       Boolean include_start,
    ///                                       Boolean include_end);
    MsgpackRequest *buffer_get_line_slice(int64_t buffer,
                                          int64_t start,
                                          int64_t end,
                                          bool include_start,
                                          bool include_end);
    
    /// nvim API:
    /// ArrayOf(String) nvim_buf_get_lines(Buffer buffer,
    ///                                    Integer start,
    ///                                    Integer end,
    ///                                    Boolean strict_indexing);
    MsgpackRequest *nvim_buf_get_lines(int64_t buffer,
                                       int64_t start,
                                       int64_t end,
                                       bool strict_indexing);
    
    /// @deprecated nvim API:
    /// void buffer_set_line_slice(Buffer buffer,
    ///                            Integer start,
    ///                            Integer end,
    ///                            Boolean include_start,
    ///                            Boolean include_end,
    ///                            ArrayOf(String) replacement);
    MsgpackRequest *buffer_set_line_slice(int64_t buffer,
                                          int64_t start,
                                          int64_t end,
                                          bool include_start,
                                          bool include_end,
                                          QList<QByteArray> replacement);
    
    /// nvim API:
    /// void nvim_buf_set_lines(Buffer buffer,
    ///                         Integer start,
    ///                         Integer end,
    ///                         Boolean strict_indexing,
    ///                         ArrayOf(String) replacement);
    MsgpackRequest *nvim_buf_set_lines(int64_t buffer,
                                       int64_t start,
                                       int64_t end,
                                       bool strict_indexing,
                                       QList<QByteArray> replacement);
    
    /// nvim API:
    /// Object nvim_buf_get_var(Buffer buffer,
    ///                         String name);
    MsgpackRequest *nvim_buf_get_var(int64_t buffer,
                                     QByteArray name);
    
    /// nvim API:
    /// Integer nvim_buf_get_changedtick(Buffer buffer);
    MsgpackRequest *nvim_buf_get_changedtick(int64_t buffer);
    
    /// nvim API:
    /// ArrayOf(Dictionary) nvim_buf_get_keymap(Buffer buffer,
    ///                                         String mode);
    MsgpackRequest *nvim_buf_get_keymap(int64_t buffer,
                                        QByteArray mode);
    
    /// nvim API:
    /// void nvim_buf_set_var(Buffer buffer,
    ///                       String name,
    ///                       Object value);
    MsgpackRequest *nvim_buf_set_var(int64_t buffer,
                                     QByteArray name,
                                     QVariant value);
    
    /// nvim API:
    /// void nvim_buf_del_var(Buffer buffer,
    ///                       String name);
    MsgpackRequest *nvim_buf_del_var(int64_t buffer,
                                     QByteArray name);
    
    /// @deprecated nvim API:
    /// Object buffer_set_var(Buffer buffer,
    ///                       String name,
    ///                       Object value);
    MsgpackRequest *buffer_set_var(int64_t buffer,
                                   QByteArray name,
                                   QVariant value);
    
    /// @deprecated nvim API:
    /// Object buffer_del_var(Buffer buffer,
    ///                       String name);
    MsgpackRequest *buffer_del_var(int64_t buffer,
                                   QByteArray name);
    
    /// nvim API:
    /// Object nvim_buf_get_option(Buffer buffer,
    ///                            String name);
    MsgpackRequest *nvim_buf_get_option(int64_t buffer,
                                        QByteArray name);
    
    /// nvim API:
    /// void nvim_buf_set_option(Buffer buffer,
    ///                          String name,
    ///                          Object value);
    MsgpackRequest *nvim_buf_set_option(int64_t buffer,
                                        QByteArray name,
                                        QVariant value);
    
    /// @deprecated nvim API:
    /// Integer nvim_buf_get_number(Buffer buffer);
    MsgpackRequest *nvim_buf_get_number(int64_t buffer);
    
    /// nvim API:
    /// String nvim_buf_get_name(Buffer buffer);
    MsgpackRequest *nvim_buf_get_name(int64_t buffer);
    
    /// nvim API:
    /// void nvim_buf_set_name(Buffer buffer,
    ///                        String name);
    MsgpackRequest *nvim_buf_set_name(int64_t buffer,
                                      QByteArray name);
    
    /// nvim API:
    /// Boolean nvim_buf_is_valid(Buffer buffer);
    MsgpackRequest *nvim_buf_is_valid(int64_t buffer);
    
    /// @deprecated nvim API:
    /// void buffer_insert(Buffer buffer,
    ///                    Integer lnum,
    ///                    ArrayOf(String) lines);
    MsgpackRequest *buffer_insert(int64_t buffer,
                                  int64_t lnum,
                                  QList<QByteArray> lines);
    
    /// nvim API:
    /// ArrayOf(Integer,
    ///                                    2) nvim_buf_get_mark(Buffer buffer,
    ///                                    String name);
    MsgpackRequest *nvim_buf_get_mark(int64_t buffer,
                                      QByteArray name);
    
    /// nvim API:
    /// Integer nvim_buf_add_highlight(Buffer buffer,
    ///                                Integer src_id,
    ///                                String hl_group,
    ///                                Integer line,
    ///                                Integer col_start,
    ///                                Integer col_end);
    MsgpackRequest *nvim_buf_add_highlight(int64_t buffer,
                                           int64_t src_id,
                                           QByteArray hl_group,
                                           int64_t line,
                                           int64_t col_start,
                                           int64_t col_end);
    
    /// nvim API:
    /// void nvim_buf_clear_highlight(Buffer buffer,
    ///                               Integer src_id,
    ///                               Integer line_start,
    ///                               Integer line_end);
    MsgpackRequest *nvim_buf_clear_highlight(int64_t buffer,
                                             int64_t src_id,
                                             int64_t line_start,
                                             int64_t line_end);
    
    /// nvim API:
    /// ArrayOf(Window) nvim_tabpage_list_wins(Tabpage tabpage);
    MsgpackRequest *nvim_tabpage_list_wins(int64_t tabpage);
    
    /// nvim API:
    /// Object nvim_tabpage_get_var(Tabpage tabpage,
    ///                             String name);
    MsgpackRequest *nvim_tabpage_get_var(int64_t tabpage,
                                         QByteArray name);
    
    /// nvim API:
    /// void nvim_tabpage_set_var(Tabpage tabpage,
    ///                           String name,
    ///                           Object value);
    MsgpackRequest *nvim_tabpage_set_var(int64_t tabpage,
                                         QByteArray name,
                                         QVariant value);
    
    /// nvim API:
    /// void nvim_tabpage_del_var(Tabpage tabpage,
    ///                           String name);
    MsgpackRequest *nvim_tabpage_del_var(int64_t tabpage,
                                         QByteArray name);
    
    /// @deprecated nvim API:
    /// Object tabpage_set_var(Tabpage tabpage,
    ///                        String name,
    ///                        Object value);
    MsgpackRequest *tabpage_set_var(int64_t tabpage,
                                    QByteArray name,
                                    QVariant value);
    
    /// @deprecated nvim API:
    /// Object tabpage_del_var(Tabpage tabpage,
    ///                        String name);
    MsgpackRequest *tabpage_del_var(int64_t tabpage,
                                    QByteArray name);
    
    /// nvim API:
    /// Window nvim_tabpage_get_win(Tabpage tabpage);
    MsgpackRequest *nvim_tabpage_get_win(int64_t tabpage);
    
    /// nvim API:
    /// Integer nvim_tabpage_get_number(Tabpage tabpage);
    MsgpackRequest *nvim_tabpage_get_number(int64_t tabpage);
    
    /// nvim API:
    /// Boolean nvim_tabpage_is_valid(Tabpage tabpage);
    MsgpackRequest *nvim_tabpage_is_valid(int64_t tabpage);
    
    /// nvim API:
    /// void nvim_ui_attach(Integer width,
    ///                     Integer height,
    ///                     Dictionary options);
    MsgpackRequest *nvim_ui_attach(int64_t width,
                                   int64_t height,
                                   QVariantMap options);
    
    /// @deprecated nvim API:
    /// void ui_attach(Integer width,
    ///                Integer height,
    ///                Boolean enable_rgb);
    MsgpackRequest *ui_attach(int64_t width,
                              int64_t height,
                              bool enable_rgb);
    
    /// nvim API:
    /// void nvim_ui_detach);
    MsgpackRequest *nvim_ui_detach();
    
    /// nvim API:
    /// void nvim_ui_try_resize(Integer width,
    ///                         Integer height);
    MsgpackRequest *nvim_ui_try_resize(int64_t width,
                                       int64_t height);
    
    /// nvim API:
    /// void nvim_ui_set_option(String name,
    ///                         Object value);
    MsgpackRequest *nvim_ui_set_option(QByteArray name,
                                       QVariant value);
    
    /// nvim API:
    /// void nvim_command(String command);
    MsgpackRequest *nvim_command(QByteArray command);
    
    /// nvim API:
    /// void nvim_feedkeys(String keys,
    ///                    String mode,
    ///                    Boolean escape_csi);
    MsgpackRequest *nvim_feedkeys(QByteArray keys,
                                  QByteArray mode,
                                  bool escape_csi);
    
    /// nvim API:
    /// Integer nvim_input(String keys);
    MsgpackRequest *nvim_input(QByteArray keys);
    
    /// nvim API:
    /// String nvim_replace_termcodes(String str,
    ///                               Boolean from_part,
    ///                               Boolean do_lt,
    ///                               Boolean special);
    MsgpackRequest *nvim_replace_termcodes(QByteArray str,
                                           bool from_part,
                                           bool do_lt,
                                           bool special);
    
    /// nvim API:
    /// String nvim_command_output(String str);
    MsgpackRequest *nvim_command_output(QByteArray str);
    
    /// nvim API:
    /// Object nvim_eval(String expr);
    MsgpackRequest *nvim_eval(QByteArray expr);
    
    /// nvim API:
    /// Object nvim_call_function(String fname,
    ///                           Array args);
    MsgpackRequest *nvim_call_function(QByteArray fname,
                                       QVariantList args);
    
    /// nvim API:
    /// Object nvim_execute_lua(String code,
    ///                         Array args);
    MsgpackRequest *nvim_execute_lua(QByteArray code,
                                     QVariantList args);
    
    /// nvim API:
    /// Integer nvim_strwidth(String str);
    MsgpackRequest *nvim_strwidth(QByteArray str);
    
    /// nvim API:
    /// ArrayOf(String) nvim_list_runtime_paths);
    MsgpackRequest *nvim_list_runtime_paths();
    
    /// nvim API:
    /// void nvim_set_current_dir(String dir);
    MsgpackRequest *nvim_set_current_dir(QByteArray dir);
    
    /// nvim API:
    /// String nvim_get_current_line);
    MsgpackRequest *nvim_get_current_line();
    
    /// nvim API:
    /// void nvim_set_current_line(String line);
    MsgpackRequest *nvim_set_current_line(QByteArray line);
    
    /// nvim API:
    /// void nvim_del_current_line);
    MsgpackRequest *nvim_del_current_line();
    
    /// nvim API:
    /// Object nvim_get_var(String name);
    MsgpackRequest *nvim_get_var(QByteArray name);
    
    /// nvim API:
    /// void nvim_set_var(String name,
    ///                   Object value);
    MsgpackRequest *nvim_set_var(QByteArray name,
                                 QVariant value);
    
    /// nvim API:
    /// void nvim_del_var(String name);
    MsgpackRequest *nvim_del_var(QByteArray name);
    
    /// @deprecated nvim API:
    /// Object vim_set_var(String name,
    ///                    Object value);
    MsgpackRequest *vim_set_var(QByteArray name,
                                QVariant value);
    
    /// @deprecated nvim API:
    /// Object vim_del_var(String name);
    MsgpackRequest *vim_del_var(QByteArray name);
    
    /// nvim API:
    /// Object nvim_get_vvar(String name);
    MsgpackRequest *nvim_get_vvar(QByteArray name);
    
    /// nvim API:
    /// Object nvim_get_option(String name);
    MsgpackRequest *nvim_get_option(QByteArray name);
    
    /// nvim API:
    /// void nvim_set_option(String name,
    ///                      Object value);
    MsgpackRequest *nvim_set_option(QByteArray name,
                                    QVariant value);
    
    /// nvim API:
    /// void nvim_out_write(String str);
    MsgpackRequest *nvim_out_write(QByteArray str);
    
    /// nvim API:
    /// void nvim_errmsg_write(String str);
    MsgpackRequest *nvim_errmsg_write(QByteArray str);
    
    /// nvim API:
    /// void nvim_errmsg_writeln(String str);
    MsgpackRequest *nvim_errmsg_writeln(QByteArray str);
    
    /// nvim API:
    /// ArrayOf(Buffer) nvim_list_bufs);
    MsgpackRequest *nvim_list_bufs();
    
    /// nvim API:
    /// Buffer nvim_get_current_buf);
    MsgpackRequest *nvim_get_current_buf();
    
    /// nvim API:
    /// void nvim_set_current_buf(Buffer buffer);
    MsgpackRequest *nvim_set_current_buf(int64_t buffer);
    
    /// nvim API:
    /// ArrayOf(Window) nvim_list_wins);
    MsgpackRequest *nvim_list_wins();
    
    /// nvim API:
    /// Window nvim_get_current_win);
    MsgpackRequest *nvim_get_current_win();
    
    /// nvim API:
    /// void nvim_set_current_win(Window window);
    MsgpackRequest *nvim_set_current_win(int64_t window);
    
    /// nvim API:
    /// ArrayOf(Tabpage) nvim_list_tabpages);
    MsgpackRequest *nvim_list_tabpages();
    
    /// nvim API:
    /// Tabpage nvim_get_current_tabpage);
    MsgpackRequest *nvim_get_current_tabpage();
    
    /// nvim API:
    /// void nvim_set_current_tabpage(Tabpage tabpage);
    MsgpackRequest *nvim_set_current_tabpage(int64_t tabpage);
    
    /// nvim API:
    /// void nvim_subscribe(String event);
    MsgpackRequest *nvim_subscribe(QByteArray event);
    
    /// nvim API:
    /// void nvim_unsubscribe(String event);
    MsgpackRequest *nvim_unsubscribe(QByteArray event);
    
    /// nvim API:
    /// Integer nvim_get_color_by_name(String name);
    MsgpackRequest *nvim_get_color_by_name(QByteArray name);
    
    /// nvim API:
    /// Dictionary nvim_get_color_map);
    MsgpackRequest *nvim_get_color_map();
    
    /// nvim API:
    /// Dictionary nvim_get_mode);
    MsgpackRequest *nvim_get_mode();
    
    /// nvim API:
    /// ArrayOf(Dictionary) nvim_get_keymap(String mode);
    MsgpackRequest *nvim_get_keymap(QByteArray mode);
    
    /// nvim API:
    /// Array nvim_get_api_info);
    MsgpackRequest *nvim_get_api_info();
    
    /// nvim API:
    /// Array nvim_call_atomic(Array calls);
    MsgpackRequest *nvim_call_atomic(QVariantList calls);
    
    /// nvim API:
    /// Buffer nvim_win_get_buf(Window window);
    MsgpackRequest *nvim_win_get_buf(int64_t window);
    
    /// nvim API:
    /// ArrayOf(Integer,
    ///                                      2) nvim_win_get_cursor(Window window);
    MsgpackRequest *nvim_win_get_cursor(int64_t window);
    
    /// nvim API:
    /// void nvim_win_set_cursor(Window window,
    ///                          ArrayOf(Integer,
    ///                          2) pos);
    MsgpackRequest *nvim_win_set_cursor(int64_t window,
                                        QPoint pos);
    
    /// nvim API:
    /// Integer nvim_win_get_height(Window window);
    MsgpackRequest *nvim_win_get_height(int64_t window);
    
    /// nvim API:
    /// void nvim_win_set_height(Window window,
    ///                          Integer height);
    MsgpackRequest *nvim_win_set_height(int64_t window,
                                        int64_t height);
    
    /// nvim API:
    /// Integer nvim_win_get_width(Window window);
    MsgpackRequest *nvim_win_get_width(int64_t window);
    
    /// nvim API:
    /// void nvim_win_set_width(Window window,
    ///                         Integer width);
    MsgpackRequest *nvim_win_set_width(int64_t window,
                                       int64_t width);
    
    /// nvim API:
    /// Object nvim_win_get_var(Window window,
    ///                         String name);
    MsgpackRequest *nvim_win_get_var(int64_t window,
                                     QByteArray name);
    
    /// nvim API:
    /// void nvim_win_set_var(Window window,
    ///                       String name,
    ///                       Object value);
    MsgpackRequest *nvim_win_set_var(int64_t window,
                                     QByteArray name,
                                     QVariant value);
    
    /// nvim API:
    /// void nvim_win_del_var(Window window,
    ///                       String name);
    MsgpackRequest *nvim_win_del_var(int64_t window,
                                     QByteArray name);
    
    /// @deprecated nvim API:
    /// Object window_set_var(Window window,
    ///                       String name,
    ///                       Object value);
    MsgpackRequest *window_set_var(int64_t window,
                                   QByteArray name,
                                   QVariant value);
    
    /// @deprecated nvim API:
    /// Object window_del_var(Window window,
    ///                       String name);
    MsgpackRequest *window_del_var(int64_t window,
                                   QByteArray name);
    
    /// nvim API:
    /// Object nvim_win_get_option(Window window,
    ///                            String name);
    MsgpackRequest *nvim_win_get_option(int64_t window,
                                        QByteArray name);
    
    /// nvim API:
    /// void nvim_win_set_option(Window window,
    ///                          String name,
    ///                          Object value);
    MsgpackRequest *nvim_win_set_option(int64_t window,
                                        QByteArray name,
                                        QVariant value);
    
    /// nvim API:
    /// ArrayOf(Integer,
    ///                                        2) nvim_win_get_position(Window window);
    MsgpackRequest *nvim_win_get_position(int64_t window);
    
    /// nvim API:
    /// Tabpage nvim_win_get_tabpage(Window window);
    MsgpackRequest *nvim_win_get_tabpage(int64_t window);
    
    /// nvim API:
    /// Integer nvim_win_get_number(Window window);
    MsgpackRequest *nvim_win_get_number(int64_t window);
    
    /// nvim API:
    /// Boolean nvim_win_is_valid(Window window);
    MsgpackRequest *nvim_win_is_valid(int64_t window);

signals:
    void on_nvim_buf_line_count(int64_t);
    void err_nvim_buf_line_count(const QString &, const QVariant &);

    void on_buffer_get_line(QByteArray);
    void err_buffer_get_line(const QString &, const QVariant &);

    void on_buffer_set_line(void);
    void err_buffer_set_line(const QString &, const QVariant &);

    void on_buffer_del_line(void);
    void err_buffer_del_line(const QString &, const QVariant &);

    void on_buffer_get_line_slice(QList<QByteArray>);
    void err_buffer_get_line_slice(const QString &, const QVariant &);

    void on_nvim_buf_get_lines(QList<QByteArray>);
    void err_nvim_buf_get_lines(const QString &, const QVariant &);

    void on_buffer_set_line_slice(void);
    void err_buffer_set_line_slice(const QString &, const QVariant &);

    void on_nvim_buf_set_lines(void);
    void err_nvim_buf_set_lines(const QString &, const QVariant &);

    void on_nvim_buf_get_var(QVariant);
    void err_nvim_buf_get_var(const QString &, const QVariant &);

    void on_nvim_buf_get_changedtick(int64_t);
    void err_nvim_buf_get_changedtick(const QString &, const QVariant &);

    void on_nvim_buf_get_keymap(QList<QVariantMap>);
    void err_nvim_buf_get_keymap(const QString &, const QVariant &);

    void on_nvim_buf_set_var(void);
    void err_nvim_buf_set_var(const QString &, const QVariant &);

    void on_nvim_buf_del_var(void);
    void err_nvim_buf_del_var(const QString &, const QVariant &);

    void on_buffer_set_var(QVariant);
    void err_buffer_set_var(const QString &, const QVariant &);

    void on_buffer_del_var(QVariant);
    void err_buffer_del_var(const QString &, const QVariant &);

    void on_nvim_buf_get_option(QVariant);
    void err_nvim_buf_get_option(const QString &, const QVariant &);

    void on_nvim_buf_set_option(void);
    void err_nvim_buf_set_option(const QString &, const QVariant &);

    void on_nvim_buf_get_number(int64_t);
    void err_nvim_buf_get_number(const QString &, const QVariant &);

    void on_nvim_buf_get_name(QByteArray);
    void err_nvim_buf_get_name(const QString &, const QVariant &);

    void on_nvim_buf_set_name(void);
    void err_nvim_buf_set_name(const QString &, const QVariant &);

    void on_nvim_buf_is_valid(bool);
    void err_nvim_buf_is_valid(const QString &, const QVariant &);

    void on_buffer_insert(void);
    void err_buffer_insert(const QString &, const QVariant &);

    void on_nvim_buf_get_mark(QPoint);
    void err_nvim_buf_get_mark(const QString &, const QVariant &);

    void on_nvim_buf_add_highlight(int64_t);
    void err_nvim_buf_add_highlight(const QString &, const QVariant &);

    void on_nvim_buf_clear_highlight(void);
    void err_nvim_buf_clear_highlight(const QString &, const QVariant &);

    void on_nvim_tabpage_list_wins(QList<int64_t>);
    void err_nvim_tabpage_list_wins(const QString &, const QVariant &);

    void on_nvim_tabpage_get_var(QVariant);
    void err_nvim_tabpage_get_var(const QString &, const QVariant &);

    void on_nvim_tabpage_set_var(void);
    void err_nvim_tabpage_set_var(const QString &, const QVariant &);

    void on_nvim_tabpage_del_var(void);
    void err_nvim_tabpage_del_var(const QString &, const QVariant &);

    void on_tabpage_set_var(QVariant);
    void err_tabpage_set_var(const QString &, const QVariant &);

    void on_tabpage_del_var(QVariant);
    void err_tabpage_del_var(const QString &, const QVariant &);

    void on_nvim_tabpage_get_win(int64_t);
    void err_nvim_tabpage_get_win(const QString &, const QVariant &);

    void on_nvim_tabpage_get_number(int64_t);
    void err_nvim_tabpage_get_number(const QString &, const QVariant &);

    void on_nvim_tabpage_is_valid(bool);
    void err_nvim_tabpage_is_valid(const QString &, const QVariant &);

    void on_nvim_ui_attach(void);
    void err_nvim_ui_attach(const QString &, const QVariant &);

    void on_ui_attach(void);
    void err_ui_attach(const QString &, const QVariant &);

    void on_nvim_ui_detach(void);
    void err_nvim_ui_detach(const QString &, const QVariant &);

    void on_nvim_ui_try_resize(void);
    void err_nvim_ui_try_resize(const QString &, const QVariant &);

    void on_nvim_ui_set_option(void);
    void err_nvim_ui_set_option(const QString &, const QVariant &);

    void on_nvim_command(void);
    void err_nvim_command(const QString &, const QVariant &);

    void on_nvim_feedkeys(void);
    void err_nvim_feedkeys(const QString &, const QVariant &);

    void on_nvim_input(int64_t);
    void err_nvim_input(const QString &, const QVariant &);

    void on_nvim_replace_termcodes(QByteArray);
    void err_nvim_replace_termcodes(const QString &, const QVariant &);

    void on_nvim_command_output(QByteArray);
    void err_nvim_command_output(const QString &, const QVariant &);

    void on_nvim_eval(QVariant);
    void err_nvim_eval(const QString &, const QVariant &);

    void on_nvim_call_function(QVariant);
    void err_nvim_call_function(const QString &, const QVariant &);

    void on_nvim_execute_lua(QVariant);
    void err_nvim_execute_lua(const QString &, const QVariant &);

    void on_nvim_strwidth(int64_t);
    void err_nvim_strwidth(const QString &, const QVariant &);

    void on_nvim_list_runtime_paths(QList<QByteArray>);
    void err_nvim_list_runtime_paths(const QString &, const QVariant &);

    void on_nvim_set_current_dir(void);
    void err_nvim_set_current_dir(const QString &, const QVariant &);

    void on_nvim_get_current_line(QByteArray);
    void err_nvim_get_current_line(const QString &, const QVariant &);

    void on_nvim_set_current_line(void);
    void err_nvim_set_current_line(const QString &, const QVariant &);

    void on_nvim_del_current_line(void);
    void err_nvim_del_current_line(const QString &, const QVariant &);

    void on_nvim_get_var(QVariant);
    void err_nvim_get_var(const QString &, const QVariant &);

    void on_nvim_set_var(void);
    void err_nvim_set_var(const QString &, const QVariant &);

    void on_nvim_del_var(void);
    void err_nvim_del_var(const QString &, const QVariant &);

    void on_vim_set_var(QVariant);
    void err_vim_set_var(const QString &, const QVariant &);

    void on_vim_del_var(QVariant);
    void err_vim_del_var(const QString &, const QVariant &);

    void on_nvim_get_vvar(QVariant);
    void err_nvim_get_vvar(const QString &, const QVariant &);

    void on_nvim_get_option(QVariant);
    void err_nvim_get_option(const QString &, const QVariant &);

    void on_nvim_set_option(void);
    void err_nvim_set_option(const QString &, const QVariant &);

    void on_nvim_out_write(void);
    void err_nvim_out_write(const QString &, const QVariant &);

    void on_nvim_errmsg_write(void);
    void err_nvim_errmsg_write(const QString &, const QVariant &);

    void on_nvim_errmsg_writeln(void);
    void err_nvim_errmsg_writeln(const QString &, const QVariant &);

    void on_nvim_list_bufs(QList<int64_t>);
    void err_nvim_list_bufs(const QString &, const QVariant &);

    void on_nvim_get_current_buf(int64_t);
    void err_nvim_get_current_buf(const QString &, const QVariant &);

    void on_nvim_set_current_buf(void);
    void err_nvim_set_current_buf(const QString &, const QVariant &);

    void on_nvim_list_wins(QList<int64_t>);
    void err_nvim_list_wins(const QString &, const QVariant &);

    void on_nvim_get_current_win(int64_t);
    void err_nvim_get_current_win(const QString &, const QVariant &);

    void on_nvim_set_current_win(void);
    void err_nvim_set_current_win(const QString &, const QVariant &);

    void on_nvim_list_tabpages(QList<int64_t>);
    void err_nvim_list_tabpages(const QString &, const QVariant &);

    void on_nvim_get_current_tabpage(int64_t);
    void err_nvim_get_current_tabpage(const QString &, const QVariant &);

    void on_nvim_set_current_tabpage(void);
    void err_nvim_set_current_tabpage(const QString &, const QVariant &);

    void on_nvim_subscribe(void);
    void err_nvim_subscribe(const QString &, const QVariant &);

    void on_nvim_unsubscribe(void);
    void err_nvim_unsubscribe(const QString &, const QVariant &);

    void on_nvim_get_color_by_name(int64_t);
    void err_nvim_get_color_by_name(const QString &, const QVariant &);

    void on_nvim_get_color_map(QVariantMap);
    void err_nvim_get_color_map(const QString &, const QVariant &);

    void on_nvim_get_mode(QVariantMap);
    void err_nvim_get_mode(const QString &, const QVariant &);

    void on_nvim_get_keymap(QList<QVariantMap>);
    void err_nvim_get_keymap(const QString &, const QVariant &);

    void on_nvim_get_api_info(QVariantList);
    void err_nvim_get_api_info(const QString &, const QVariant &);

    void on_nvim_call_atomic(QVariantList);
    void err_nvim_call_atomic(const QString &, const QVariant &);

    void on_nvim_win_get_buf(int64_t);
    void err_nvim_win_get_buf(const QString &, const QVariant &);

    void on_nvim_win_get_cursor(QPoint);
    void err_nvim_win_get_cursor(const QString &, const QVariant &);

    void on_nvim_win_set_cursor(void);
    void err_nvim_win_set_cursor(const QString &, const QVariant &);

    void on_nvim_win_get_height(int64_t);
    void err_nvim_win_get_height(const QString &, const QVariant &);

    void on_nvim_win_set_height(void);
    void err_nvim_win_set_height(const QString &, const QVariant &);

    void on_nvim_win_get_width(int64_t);
    void err_nvim_win_get_width(const QString &, const QVariant &);

    void on_nvim_win_set_width(void);
    void err_nvim_win_set_width(const QString &, const QVariant &);

    void on_nvim_win_get_var(QVariant);
    void err_nvim_win_get_var(const QString &, const QVariant &);

    void on_nvim_win_set_var(void);
    void err_nvim_win_set_var(const QString &, const QVariant &);

    void on_nvim_win_del_var(void);
    void err_nvim_win_del_var(const QString &, const QVariant &);

    void on_window_set_var(QVariant);
    void err_window_set_var(const QString &, const QVariant &);

    void on_window_del_var(QVariant);
    void err_window_del_var(const QString &, const QVariant &);

    void on_nvim_win_get_option(QVariant);
    void err_nvim_win_get_option(const QString &, const QVariant &);

    void on_nvim_win_set_option(void);
    void err_nvim_win_set_option(const QString &, const QVariant &);

    void on_nvim_win_get_position(QPoint);
    void err_nvim_win_get_position(const QString &, const QVariant &);

    void on_nvim_win_get_tabpage(int64_t);
    void err_nvim_win_get_tabpage(const QString &, const QVariant &);

    void on_nvim_win_get_number(int64_t);
    void err_nvim_win_get_number(const QString &, const QVariant &);

    void on_nvim_win_is_valid(bool);
    void err_nvim_win_is_valid(const QString &, const QVariant &);

};

} // namespace::SnailNvimQt

#endif // CONFIG_NVIMAPI_AUTO_NVIM_H
