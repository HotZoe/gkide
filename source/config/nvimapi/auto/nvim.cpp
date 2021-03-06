/// @file config/nvimapi/auto/nvim.cpp
///
/// Auto generated: UTC 2018-02-06 15:02:36.619418

#include "config/nvimapi/auto/nvim.h"

#include "plugins/bin/snail/attributes.h"
#include "plugins/bin/snail/util.h"
#include "plugins/bin/snail/nvimconnector.h"
#include "plugins/bin/snail/msgpackrequest.h"
#include "plugins/bin/snail/msgpackiodevice.h"

namespace SnailNvimQt {

/// Unpack Nvim EXT types: Window, Buffer Tabpage, which are all uint64_t
/// see Nvim:msgpack_rpc_to_
QVariant unpackBuffer(MsgpackIODevice *VATTR_UNUSED_MATCH(dev),
                      const char *in,
                      quint32 size)
{
    msgpack_unpacked result;
    msgpack_unpacked_init(&result);
    msgpack_unpack_return ret = msgpack_unpack_next(&result, in, size, NULL);
    msgpack_unpacked_destroy(&result);

    if(ret != MSGPACK_UNPACK_SUCCESS)
    {
        return QVariant();
    }

    return QVariant((quint64)result.data.via.u64);
}

#define unpackWindow  unpackBuffer
#define unpackTabpage unpackBuffer

Nvim::Nvim(NvimConnector *c) :m_c(c)
{
    // Ext types of msgpack
    m_c->m_dev->registerExtType(0, unpackBuffer);
    m_c->m_dev->registerExtType(1, unpackWindow);
    m_c->m_dev->registerExtType(2, unpackTabpage);

    connect(m_c->m_dev, &MsgpackIODevice::notification,
            this, &Nvim::neovimNotification);
}

// Slots
MsgpackRequest *Nvim::nvim_buf_line_count(int64_t buffer)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_line_count", 1);

    r->setFuncId(kNvimAPI_NVIM_BUF_LINE_COUNT);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    return r;
}

MsgpackRequest *Nvim::nvim_get_current_line()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_current_line", 0);

    r->setFuncId(kNvimAPI_NVIM_GET_CURRENT_LINE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_set_current_line(QByteArray line)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_set_current_line", 1);

    r->setFuncId(kNvimAPI_NVIM_SET_CURRENT_LINE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(line);
    return r;
}

MsgpackRequest *Nvim::nvim_del_current_line()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_del_current_line", 0);

    r->setFuncId(kNvimAPI_NVIM_DEL_CURRENT_LINE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_lines(int64_t buffer,
                                         int64_t start,
                                         int64_t end,
                                         bool strict_indexing)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_lines", 4);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_LINES);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(start);
    m_c->m_dev->send(end);
    m_c->m_dev->send(strict_indexing);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_set_lines(int64_t buffer,
                                         int64_t start,
                                         int64_t end,
                                         bool strict_indexing,
                                         QList<QByteArray> replacement)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_set_lines", 5);

    r->setFuncId(kNvimAPI_NVIM_BUF_SET_LINES);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(start);
    m_c->m_dev->send(end);
    m_c->m_dev->send(strict_indexing);
    m_c->m_dev->sendArrayOf(replacement);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_var(int64_t buffer,
                                       QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_var", 2);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_changedtick(int64_t buffer)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_changedtick", 1);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_CHANGEDTICK);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_keymap(int64_t buffer,
                                          QByteArray mode)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_keymap", 2);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_KEYMAP);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(mode);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_set_var(int64_t buffer,
                                       QByteArray name,
                                       QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_set_var", 3);

    r->setFuncId(kNvimAPI_NVIM_BUF_SET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_del_var(int64_t buffer,
                                       QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_del_var", 2);

    r->setFuncId(kNvimAPI_NVIM_BUF_DEL_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_option(int64_t buffer,
                                          QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_option", 2);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_OPTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_set_option(int64_t buffer,
                                          QByteArray name,
                                          QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_set_option", 3);

    r->setFuncId(kNvimAPI_NVIM_BUF_SET_OPTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_number(int64_t buffer)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_number", 1);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_NUMBER);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_name(int64_t buffer)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_name", 1);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_NAME);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_set_name(int64_t buffer,
                                        QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_set_name", 2);

    r->setFuncId(kNvimAPI_NVIM_BUF_SET_NAME);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_is_valid(int64_t buffer)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_is_valid", 1);

    r->setFuncId(kNvimAPI_NVIM_BUF_IS_VALID);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_get_mark(int64_t buffer,
                                        QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_get_mark", 2);

    r->setFuncId(kNvimAPI_NVIM_BUF_GET_MARK);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_add_highlight(int64_t buffer,
                                             int64_t src_id,
                                             QByteArray hl_group,
                                             int64_t line,
                                             int64_t col_start,
                                             int64_t col_end)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_add_highlight", 6);

    r->setFuncId(kNvimAPI_NVIM_BUF_ADD_HIGHLIGHT);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(src_id);
    m_c->m_dev->send(hl_group);
    m_c->m_dev->send(line);
    m_c->m_dev->send(col_start);
    m_c->m_dev->send(col_end);
    return r;
}

MsgpackRequest *Nvim::nvim_buf_clear_highlight(int64_t buffer,
                                               int64_t src_id,
                                               int64_t line_start,
                                               int64_t line_end)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_buf_clear_highlight", 4);

    r->setFuncId(kNvimAPI_NVIM_BUF_CLEAR_HIGHLIGHT);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    m_c->m_dev->send(src_id);
    m_c->m_dev->send(line_start);
    m_c->m_dev->send(line_end);
    return r;
}

MsgpackRequest *Nvim::nvim_tabpage_list_wins(int64_t tabpage)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_tabpage_list_wins", 1);

    r->setFuncId(kNvimAPI_NVIM_TABPAGE_LIST_WINS);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    return r;
}

MsgpackRequest *Nvim::nvim_tabpage_get_var(int64_t tabpage,
                                           QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_tabpage_get_var", 2);

    r->setFuncId(kNvimAPI_NVIM_TABPAGE_GET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_tabpage_set_var(int64_t tabpage,
                                           QByteArray name,
                                           QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_tabpage_set_var", 3);

    r->setFuncId(kNvimAPI_NVIM_TABPAGE_SET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_tabpage_del_var(int64_t tabpage,
                                           QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_tabpage_del_var", 2);

    r->setFuncId(kNvimAPI_NVIM_TABPAGE_DEL_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_tabpage_get_win(int64_t tabpage)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_tabpage_get_win", 1);

    r->setFuncId(kNvimAPI_NVIM_TABPAGE_GET_WIN);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    return r;
}

MsgpackRequest *Nvim::nvim_tabpage_get_number(int64_t tabpage)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_tabpage_get_number", 1);

    r->setFuncId(kNvimAPI_NVIM_TABPAGE_GET_NUMBER);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    return r;
}

MsgpackRequest *Nvim::nvim_tabpage_is_valid(int64_t tabpage)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_tabpage_is_valid", 1);

    r->setFuncId(kNvimAPI_NVIM_TABPAGE_IS_VALID);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    return r;
}

MsgpackRequest *Nvim::nvim_ui_attach(int64_t width,
                                     int64_t height,
                                     QVariantMap options)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_ui_attach", 3);

    r->setFuncId(kNvimAPI_NVIM_UI_ATTACH);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(width);
    m_c->m_dev->send(height);
    m_c->m_dev->send(options);
    return r;
}

MsgpackRequest *Nvim::nvim_ui_detach()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_ui_detach", 0);

    r->setFuncId(kNvimAPI_NVIM_UI_DETACH);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_ui_try_resize(int64_t width,
                                         int64_t height)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_ui_try_resize", 2);

    r->setFuncId(kNvimAPI_NVIM_UI_TRY_RESIZE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(width);
    m_c->m_dev->send(height);
    return r;
}

MsgpackRequest *Nvim::nvim_ui_set_option(QByteArray name,
                                         QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_ui_set_option", 2);

    r->setFuncId(kNvimAPI_NVIM_UI_SET_OPTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_command(QByteArray command)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_command", 1);

    r->setFuncId(kNvimAPI_NVIM_COMMAND);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(command);
    return r;
}

MsgpackRequest *Nvim::nvim_feedkeys(QByteArray keys,
                                    QByteArray mode,
                                    bool escape_csi)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_feedkeys", 3);

    r->setFuncId(kNvimAPI_NVIM_FEEDKEYS);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(keys);
    m_c->m_dev->send(mode);
    m_c->m_dev->send(escape_csi);
    return r;
}

MsgpackRequest *Nvim::nvim_input(QByteArray keys)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_input", 1);

    r->setFuncId(kNvimAPI_NVIM_INPUT);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(keys);
    return r;
}

MsgpackRequest *Nvim::nvim_replace_termcodes(QByteArray str,
                                             bool from_part,
                                             bool do_lt,
                                             bool special)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_replace_termcodes", 4);

    r->setFuncId(kNvimAPI_NVIM_REPLACE_TERMCODES);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(str);
    m_c->m_dev->send(from_part);
    m_c->m_dev->send(do_lt);
    m_c->m_dev->send(special);
    return r;
}

MsgpackRequest *Nvim::nvim_command_output(QByteArray str)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_command_output", 1);

    r->setFuncId(kNvimAPI_NVIM_COMMAND_OUTPUT);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(str);
    return r;
}

MsgpackRequest *Nvim::nvim_eval(QByteArray expr)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_eval", 1);

    r->setFuncId(kNvimAPI_NVIM_EVAL);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(expr);
    return r;
}

MsgpackRequest *Nvim::nvim_call_function(QByteArray fname,
                                         QVariantList args)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_call_function", 2);

    r->setFuncId(kNvimAPI_NVIM_CALL_FUNCTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(fname);
    m_c->m_dev->send(args);
    return r;
}

MsgpackRequest *Nvim::nvim_execute_lua(QByteArray code,
                                       QVariantList args)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_execute_lua", 2);

    r->setFuncId(kNvimAPI_NVIM_EXECUTE_LUA);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(code);
    m_c->m_dev->send(args);
    return r;
}

MsgpackRequest *Nvim::nvim_strwidth(QByteArray str)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_strwidth", 1);

    r->setFuncId(kNvimAPI_NVIM_STRWIDTH);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(str);
    return r;
}

MsgpackRequest *Nvim::nvim_list_runtime_paths()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_list_runtime_paths", 0);

    r->setFuncId(kNvimAPI_NVIM_LIST_RUNTIME_PATHS);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_set_current_dir(QByteArray dir)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_set_current_dir", 1);

    r->setFuncId(kNvimAPI_NVIM_SET_CURRENT_DIR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(dir);
    return r;
}

MsgpackRequest *Nvim::nvim_get_var(QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_var", 1);

    r->setFuncId(kNvimAPI_NVIM_GET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_set_var(QByteArray name,
                                   QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_set_var", 2);

    r->setFuncId(kNvimAPI_NVIM_SET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_del_var(QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_del_var", 1);

    r->setFuncId(kNvimAPI_NVIM_DEL_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_get_vvar(QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_vvar", 1);

    r->setFuncId(kNvimAPI_NVIM_GET_VVAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_get_option(QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_option", 1);

    r->setFuncId(kNvimAPI_NVIM_GET_OPTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_set_option(QByteArray name,
                                      QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_set_option", 2);

    r->setFuncId(kNvimAPI_NVIM_SET_OPTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_out_write(QByteArray str)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_out_write", 1);

    r->setFuncId(kNvimAPI_NVIM_OUT_WRITE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(str);
    return r;
}

MsgpackRequest *Nvim::nvim_errmsg_write(QByteArray str)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_errmsg_write", 1);

    r->setFuncId(kNvimAPI_NVIM_ERRMSG_WRITE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(str);
    return r;
}

MsgpackRequest *Nvim::nvim_errmsg_writeln(QByteArray str)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_errmsg_writeln", 1);

    r->setFuncId(kNvimAPI_NVIM_ERRMSG_WRITELN);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(str);
    return r;
}

MsgpackRequest *Nvim::nvim_list_bufs()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_list_bufs", 0);

    r->setFuncId(kNvimAPI_NVIM_LIST_BUFS);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_get_current_buf()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_current_buf", 0);

    r->setFuncId(kNvimAPI_NVIM_GET_CURRENT_BUF);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_set_current_buf(int64_t buffer)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_set_current_buf", 1);

    r->setFuncId(kNvimAPI_NVIM_SET_CURRENT_BUF);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(buffer);
    return r;
}

MsgpackRequest *Nvim::nvim_list_wins()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_list_wins", 0);

    r->setFuncId(kNvimAPI_NVIM_LIST_WINS);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_get_current_win()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_current_win", 0);

    r->setFuncId(kNvimAPI_NVIM_GET_CURRENT_WIN);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_set_current_win(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_set_current_win", 1);

    r->setFuncId(kNvimAPI_NVIM_SET_CURRENT_WIN);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_list_tabpages()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_list_tabpages", 0);

    r->setFuncId(kNvimAPI_NVIM_LIST_TABPAGES);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_get_current_tabpage()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_current_tabpage", 0);

    r->setFuncId(kNvimAPI_NVIM_GET_CURRENT_TABPAGE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_set_current_tabpage(int64_t tabpage)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_set_current_tabpage", 1);

    r->setFuncId(kNvimAPI_NVIM_SET_CURRENT_TABPAGE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(tabpage);
    return r;
}

MsgpackRequest *Nvim::nvim_subscribe(QByteArray event)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_subscribe", 1);

    r->setFuncId(kNvimAPI_NVIM_SUBSCRIBE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(event);
    return r;
}

MsgpackRequest *Nvim::nvim_unsubscribe(QByteArray event)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_unsubscribe", 1);

    r->setFuncId(kNvimAPI_NVIM_UNSUBSCRIBE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(event);
    return r;
}

MsgpackRequest *Nvim::nvim_get_color_by_name(QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_color_by_name", 1);

    r->setFuncId(kNvimAPI_NVIM_GET_COLOR_BY_NAME);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_get_color_map()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_color_map", 0);

    r->setFuncId(kNvimAPI_NVIM_GET_COLOR_MAP);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_get_mode()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_mode", 0);

    r->setFuncId(kNvimAPI_NVIM_GET_MODE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_get_keymap(QByteArray mode)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_keymap", 1);

    r->setFuncId(kNvimAPI_NVIM_GET_KEYMAP);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(mode);
    return r;
}

MsgpackRequest *Nvim::nvim_get_api_info()
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_get_api_info", 0);

    r->setFuncId(kNvimAPI_NVIM_GET_API_INFO);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    return r;
}

MsgpackRequest *Nvim::nvim_call_atomic(QVariantList calls)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_call_atomic", 1);

    r->setFuncId(kNvimAPI_NVIM_CALL_ATOMIC);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(calls);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_buf(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_buf", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_BUF);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_cursor(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_cursor", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_CURSOR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_win_set_cursor(int64_t window,
                                          QPoint pos)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_set_cursor", 2);

    r->setFuncId(kNvimAPI_NVIM_WIN_SET_CURSOR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(pos);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_height(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_height", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_HEIGHT);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_win_set_height(int64_t window,
                                          int64_t height)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_set_height", 2);

    r->setFuncId(kNvimAPI_NVIM_WIN_SET_HEIGHT);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(height);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_width(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_width", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_WIDTH);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_win_set_width(int64_t window,
                                         int64_t width)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_set_width", 2);

    r->setFuncId(kNvimAPI_NVIM_WIN_SET_WIDTH);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(width);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_var(int64_t window,
                                       QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_var", 2);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_win_set_var(int64_t window,
                                       QByteArray name,
                                       QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_set_var", 3);

    r->setFuncId(kNvimAPI_NVIM_WIN_SET_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_win_del_var(int64_t window,
                                       QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_del_var", 2);

    r->setFuncId(kNvimAPI_NVIM_WIN_DEL_VAR);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_option(int64_t window,
                                          QByteArray name)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_option", 2);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_OPTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(name);
    return r;
}

MsgpackRequest *Nvim::nvim_win_set_option(int64_t window,
                                          QByteArray name,
                                          QVariant value)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_set_option", 3);

    r->setFuncId(kNvimAPI_NVIM_WIN_SET_OPTION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    m_c->m_dev->send(name);
    m_c->m_dev->send(value);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_position(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_position", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_POSITION);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_tabpage(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_tabpage", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_TABPAGE);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_win_get_number(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_get_number", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_GET_NUMBER);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}

MsgpackRequest *Nvim::nvim_win_is_valid(int64_t window)
{
    MsgpackRequest *r =
        m_c->m_dev->startRequestUnchecked("nvim_win_is_valid", 1);

    r->setFuncId(kNvimAPI_NVIM_WIN_IS_VALID);

    connect(r, &MsgpackRequest::finished, this, &Nvim::handleResponse);
    connect(r, &MsgpackRequest::error, this, &Nvim::handleResponseError);

    m_c->m_dev->send(window);
    return r;
}


// Handlers
void Nvim::handleResponseError(quint32 VATTR_UNUSED_MATCH(msgid),
                               NvimApiFuncID fun,
                               const QVariant &res)
{
    // TODO: support Neovim error types Exception/Validation/etc
    QString errMsg;
    const QVariantList asList = res.toList();

    if(asList.size() >= 2)
    {
        if(asList.at(1).canConvert<QByteArray>())
        {
            errMsg = m_c->m_dev->decode(asList.at(1).toByteArray());
        }
        else
        {
            errMsg = tr("Received unsupported Neovim error type");
        }
    }

    switch(fun)
    {
        case kNvimAPI_NVIM_BUF_LINE_COUNT:
            emit err_nvim_buf_line_count(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_CURRENT_LINE:
            emit err_nvim_get_current_line(errMsg, res);
            break;

        case kNvimAPI_NVIM_SET_CURRENT_LINE:
            emit err_nvim_set_current_line(errMsg, res);
            break;

        case kNvimAPI_NVIM_DEL_CURRENT_LINE:
            emit err_nvim_del_current_line(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_LINES:
            emit err_nvim_buf_get_lines(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_SET_LINES:
            emit err_nvim_buf_set_lines(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_VAR:
            emit err_nvim_buf_get_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_CHANGEDTICK:
            emit err_nvim_buf_get_changedtick(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_KEYMAP:
            emit err_nvim_buf_get_keymap(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_SET_VAR:
            emit err_nvim_buf_set_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_DEL_VAR:
            emit err_nvim_buf_del_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_OPTION:
            emit err_nvim_buf_get_option(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_SET_OPTION:
            emit err_nvim_buf_set_option(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_NUMBER:
            emit err_nvim_buf_get_number(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_NAME:
            emit err_nvim_buf_get_name(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_SET_NAME:
            emit err_nvim_buf_set_name(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_IS_VALID:
            emit err_nvim_buf_is_valid(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_GET_MARK:
            emit err_nvim_buf_get_mark(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_ADD_HIGHLIGHT:
            emit err_nvim_buf_add_highlight(errMsg, res);
            break;

        case kNvimAPI_NVIM_BUF_CLEAR_HIGHLIGHT:
            emit err_nvim_buf_clear_highlight(errMsg, res);
            break;

        case kNvimAPI_NVIM_TABPAGE_LIST_WINS:
            emit err_nvim_tabpage_list_wins(errMsg, res);
            break;

        case kNvimAPI_NVIM_TABPAGE_GET_VAR:
            emit err_nvim_tabpage_get_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_TABPAGE_SET_VAR:
            emit err_nvim_tabpage_set_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_TABPAGE_DEL_VAR:
            emit err_nvim_tabpage_del_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_TABPAGE_GET_WIN:
            emit err_nvim_tabpage_get_win(errMsg, res);
            break;

        case kNvimAPI_NVIM_TABPAGE_GET_NUMBER:
            emit err_nvim_tabpage_get_number(errMsg, res);
            break;

        case kNvimAPI_NVIM_TABPAGE_IS_VALID:
            emit err_nvim_tabpage_is_valid(errMsg, res);
            break;

        case kNvimAPI_NVIM_UI_ATTACH:
            emit err_nvim_ui_attach(errMsg, res);
            break;

        case kNvimAPI_NVIM_UI_DETACH:
            emit err_nvim_ui_detach(errMsg, res);
            break;

        case kNvimAPI_NVIM_UI_TRY_RESIZE:
            emit err_nvim_ui_try_resize(errMsg, res);
            break;

        case kNvimAPI_NVIM_UI_SET_OPTION:
            emit err_nvim_ui_set_option(errMsg, res);
            break;

        case kNvimAPI_NVIM_COMMAND:
            emit err_nvim_command(errMsg, res);
            break;

        case kNvimAPI_NVIM_FEEDKEYS:
            emit err_nvim_feedkeys(errMsg, res);
            break;

        case kNvimAPI_NVIM_INPUT:
            emit err_nvim_input(errMsg, res);
            break;

        case kNvimAPI_NVIM_REPLACE_TERMCODES:
            emit err_nvim_replace_termcodes(errMsg, res);
            break;

        case kNvimAPI_NVIM_COMMAND_OUTPUT:
            emit err_nvim_command_output(errMsg, res);
            break;

        case kNvimAPI_NVIM_EVAL:
            emit err_nvim_eval(errMsg, res);
            break;

        case kNvimAPI_NVIM_CALL_FUNCTION:
            emit err_nvim_call_function(errMsg, res);
            break;

        case kNvimAPI_NVIM_EXECUTE_LUA:
            emit err_nvim_execute_lua(errMsg, res);
            break;

        case kNvimAPI_NVIM_STRWIDTH:
            emit err_nvim_strwidth(errMsg, res);
            break;

        case kNvimAPI_NVIM_LIST_RUNTIME_PATHS:
            emit err_nvim_list_runtime_paths(errMsg, res);
            break;

        case kNvimAPI_NVIM_SET_CURRENT_DIR:
            emit err_nvim_set_current_dir(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_VAR:
            emit err_nvim_get_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_SET_VAR:
            emit err_nvim_set_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_DEL_VAR:
            emit err_nvim_del_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_VVAR:
            emit err_nvim_get_vvar(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_OPTION:
            emit err_nvim_get_option(errMsg, res);
            break;

        case kNvimAPI_NVIM_SET_OPTION:
            emit err_nvim_set_option(errMsg, res);
            break;

        case kNvimAPI_NVIM_OUT_WRITE:
            emit err_nvim_out_write(errMsg, res);
            break;

        case kNvimAPI_NVIM_ERRMSG_WRITE:
            emit err_nvim_errmsg_write(errMsg, res);
            break;

        case kNvimAPI_NVIM_ERRMSG_WRITELN:
            emit err_nvim_errmsg_writeln(errMsg, res);
            break;

        case kNvimAPI_NVIM_LIST_BUFS:
            emit err_nvim_list_bufs(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_CURRENT_BUF:
            emit err_nvim_get_current_buf(errMsg, res);
            break;

        case kNvimAPI_NVIM_SET_CURRENT_BUF:
            emit err_nvim_set_current_buf(errMsg, res);
            break;

        case kNvimAPI_NVIM_LIST_WINS:
            emit err_nvim_list_wins(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_CURRENT_WIN:
            emit err_nvim_get_current_win(errMsg, res);
            break;

        case kNvimAPI_NVIM_SET_CURRENT_WIN:
            emit err_nvim_set_current_win(errMsg, res);
            break;

        case kNvimAPI_NVIM_LIST_TABPAGES:
            emit err_nvim_list_tabpages(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_CURRENT_TABPAGE:
            emit err_nvim_get_current_tabpage(errMsg, res);
            break;

        case kNvimAPI_NVIM_SET_CURRENT_TABPAGE:
            emit err_nvim_set_current_tabpage(errMsg, res);
            break;

        case kNvimAPI_NVIM_SUBSCRIBE:
            emit err_nvim_subscribe(errMsg, res);
            break;

        case kNvimAPI_NVIM_UNSUBSCRIBE:
            emit err_nvim_unsubscribe(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_COLOR_BY_NAME:
            emit err_nvim_get_color_by_name(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_COLOR_MAP:
            emit err_nvim_get_color_map(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_MODE:
            emit err_nvim_get_mode(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_KEYMAP:
            emit err_nvim_get_keymap(errMsg, res);
            break;

        case kNvimAPI_NVIM_GET_API_INFO:
            emit err_nvim_get_api_info(errMsg, res);
            break;

        case kNvimAPI_NVIM_CALL_ATOMIC:
            emit err_nvim_call_atomic(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_BUF:
            emit err_nvim_win_get_buf(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_CURSOR:
            emit err_nvim_win_get_cursor(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_SET_CURSOR:
            emit err_nvim_win_set_cursor(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_HEIGHT:
            emit err_nvim_win_get_height(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_SET_HEIGHT:
            emit err_nvim_win_set_height(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_WIDTH:
            emit err_nvim_win_get_width(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_SET_WIDTH:
            emit err_nvim_win_set_width(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_VAR:
            emit err_nvim_win_get_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_SET_VAR:
            emit err_nvim_win_set_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_DEL_VAR:
            emit err_nvim_win_del_var(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_OPTION:
            emit err_nvim_win_get_option(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_SET_OPTION:
            emit err_nvim_win_set_option(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_POSITION:
            emit err_nvim_win_get_position(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_TABPAGE:
            emit err_nvim_win_get_tabpage(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_GET_NUMBER:
            emit err_nvim_win_get_number(errMsg, res);
            break;

        case kNvimAPI_NVIM_WIN_IS_VALID:
            emit err_nvim_win_is_valid(errMsg, res);
            break;
        default:
            m_c->setError(NvimConnector::RuntimeMsgpackError,
                          QString("Received error for function "
                                  "that should not fail: %s").arg(fun));
    }
}

void Nvim::handleResponse(quint32 VATTR_UNUSED_MATCH(msgid),
                          NvimApiFuncID fun,
                          const QVariant &res)
{
    switch(fun)
    {

        case kNvimAPI_NVIM_BUF_LINE_COUNT:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_line_count");
                return;
            }
            else
            {
                emit on_nvim_buf_line_count(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_CURRENT_LINE:
        {
            QByteArray data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_current_line");
                return;
            }
            else
            {
                emit on_nvim_get_current_line(data);
            }
        }
        break;

        case kNvimAPI_NVIM_SET_CURRENT_LINE:
        {
            emit on_nvim_set_current_line();}
        break;

        case kNvimAPI_NVIM_DEL_CURRENT_LINE:
        {
            emit on_nvim_del_current_line();}
        break;

        case kNvimAPI_NVIM_BUF_GET_LINES:
        {
            QList<QByteArray> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_lines");
                return;
            }
            else
            {
                emit on_nvim_buf_get_lines(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_SET_LINES:
        {
            emit on_nvim_buf_set_lines();}
        break;

        case kNvimAPI_NVIM_BUF_GET_VAR:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_var");
                return;
            }
            else
            {
                emit on_nvim_buf_get_var(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_GET_CHANGEDTICK:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_changedtick");
                return;
            }
            else
            {
                emit on_nvim_buf_get_changedtick(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_GET_KEYMAP:
        {
            QList<QVariantMap> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_keymap");
                return;
            }
            else
            {
                emit on_nvim_buf_get_keymap(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_SET_VAR:
        {
            emit on_nvim_buf_set_var();}
        break;

        case kNvimAPI_NVIM_BUF_DEL_VAR:
        {
            emit on_nvim_buf_del_var();}
        break;

        case kNvimAPI_NVIM_BUF_GET_OPTION:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_option");
                return;
            }
            else
            {
                emit on_nvim_buf_get_option(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_SET_OPTION:
        {
            emit on_nvim_buf_set_option();}
        break;

        case kNvimAPI_NVIM_BUF_GET_NUMBER:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_number");
                return;
            }
            else
            {
                emit on_nvim_buf_get_number(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_GET_NAME:
        {
            QByteArray data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_name");
                return;
            }
            else
            {
                emit on_nvim_buf_get_name(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_SET_NAME:
        {
            emit on_nvim_buf_set_name();}
        break;

        case kNvimAPI_NVIM_BUF_IS_VALID:
        {
            bool data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_is_valid");
                return;
            }
            else
            {
                emit on_nvim_buf_is_valid(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_GET_MARK:
        {
            QPoint data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_get_mark");
                return;
            }
            else
            {
                emit on_nvim_buf_get_mark(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_ADD_HIGHLIGHT:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_buf_add_highlight");
                return;
            }
            else
            {
                emit on_nvim_buf_add_highlight(data);
            }
        }
        break;

        case kNvimAPI_NVIM_BUF_CLEAR_HIGHLIGHT:
        {
            emit on_nvim_buf_clear_highlight();}
        break;

        case kNvimAPI_NVIM_TABPAGE_LIST_WINS:
        {
            QList<int64_t> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_tabpage_list_wins");
                return;
            }
            else
            {
                emit on_nvim_tabpage_list_wins(data);
            }
        }
        break;

        case kNvimAPI_NVIM_TABPAGE_GET_VAR:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_tabpage_get_var");
                return;
            }
            else
            {
                emit on_nvim_tabpage_get_var(data);
            }
        }
        break;

        case kNvimAPI_NVIM_TABPAGE_SET_VAR:
        {
            emit on_nvim_tabpage_set_var();}
        break;

        case kNvimAPI_NVIM_TABPAGE_DEL_VAR:
        {
            emit on_nvim_tabpage_del_var();}
        break;

        case kNvimAPI_NVIM_TABPAGE_GET_WIN:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_tabpage_get_win");
                return;
            }
            else
            {
                emit on_nvim_tabpage_get_win(data);
            }
        }
        break;

        case kNvimAPI_NVIM_TABPAGE_GET_NUMBER:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_tabpage_get_number");
                return;
            }
            else
            {
                emit on_nvim_tabpage_get_number(data);
            }
        }
        break;

        case kNvimAPI_NVIM_TABPAGE_IS_VALID:
        {
            bool data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_tabpage_is_valid");
                return;
            }
            else
            {
                emit on_nvim_tabpage_is_valid(data);
            }
        }
        break;

        case kNvimAPI_NVIM_UI_ATTACH:
        {
            emit on_nvim_ui_attach();}
        break;

        case kNvimAPI_NVIM_UI_DETACH:
        {
            emit on_nvim_ui_detach();}
        break;

        case kNvimAPI_NVIM_UI_TRY_RESIZE:
        {
            emit on_nvim_ui_try_resize();}
        break;

        case kNvimAPI_NVIM_UI_SET_OPTION:
        {
            emit on_nvim_ui_set_option();}
        break;

        case kNvimAPI_NVIM_COMMAND:
        {
            emit on_nvim_command();}
        break;

        case kNvimAPI_NVIM_FEEDKEYS:
        {
            emit on_nvim_feedkeys();}
        break;

        case kNvimAPI_NVIM_INPUT:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_input");
                return;
            }
            else
            {
                emit on_nvim_input(data);
            }
        }
        break;

        case kNvimAPI_NVIM_REPLACE_TERMCODES:
        {
            QByteArray data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_replace_termcodes");
                return;
            }
            else
            {
                emit on_nvim_replace_termcodes(data);
            }
        }
        break;

        case kNvimAPI_NVIM_COMMAND_OUTPUT:
        {
            QByteArray data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_command_output");
                return;
            }
            else
            {
                emit on_nvim_command_output(data);
            }
        }
        break;

        case kNvimAPI_NVIM_EVAL:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_eval");
                return;
            }
            else
            {
                emit on_nvim_eval(data);
            }
        }
        break;

        case kNvimAPI_NVIM_CALL_FUNCTION:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_call_function");
                return;
            }
            else
            {
                emit on_nvim_call_function(data);
            }
        }
        break;

        case kNvimAPI_NVIM_EXECUTE_LUA:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_execute_lua");
                return;
            }
            else
            {
                emit on_nvim_execute_lua(data);
            }
        }
        break;

        case kNvimAPI_NVIM_STRWIDTH:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_strwidth");
                return;
            }
            else
            {
                emit on_nvim_strwidth(data);
            }
        }
        break;

        case kNvimAPI_NVIM_LIST_RUNTIME_PATHS:
        {
            QList<QByteArray> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_list_runtime_paths");
                return;
            }
            else
            {
                emit on_nvim_list_runtime_paths(data);
            }
        }
        break;

        case kNvimAPI_NVIM_SET_CURRENT_DIR:
        {
            emit on_nvim_set_current_dir();}
        break;

        case kNvimAPI_NVIM_GET_VAR:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_var");
                return;
            }
            else
            {
                emit on_nvim_get_var(data);
            }
        }
        break;

        case kNvimAPI_NVIM_SET_VAR:
        {
            emit on_nvim_set_var();}
        break;

        case kNvimAPI_NVIM_DEL_VAR:
        {
            emit on_nvim_del_var();}
        break;

        case kNvimAPI_NVIM_GET_VVAR:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_vvar");
                return;
            }
            else
            {
                emit on_nvim_get_vvar(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_OPTION:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_option");
                return;
            }
            else
            {
                emit on_nvim_get_option(data);
            }
        }
        break;

        case kNvimAPI_NVIM_SET_OPTION:
        {
            emit on_nvim_set_option();}
        break;

        case kNvimAPI_NVIM_OUT_WRITE:
        {
            emit on_nvim_out_write();}
        break;

        case kNvimAPI_NVIM_ERRMSG_WRITE:
        {
            emit on_nvim_errmsg_write();}
        break;

        case kNvimAPI_NVIM_ERRMSG_WRITELN:
        {
            emit on_nvim_errmsg_writeln();}
        break;

        case kNvimAPI_NVIM_LIST_BUFS:
        {
            QList<int64_t> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_list_bufs");
                return;
            }
            else
            {
                emit on_nvim_list_bufs(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_CURRENT_BUF:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_current_buf");
                return;
            }
            else
            {
                emit on_nvim_get_current_buf(data);
            }
        }
        break;

        case kNvimAPI_NVIM_SET_CURRENT_BUF:
        {
            emit on_nvim_set_current_buf();}
        break;

        case kNvimAPI_NVIM_LIST_WINS:
        {
            QList<int64_t> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_list_wins");
                return;
            }
            else
            {
                emit on_nvim_list_wins(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_CURRENT_WIN:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_current_win");
                return;
            }
            else
            {
                emit on_nvim_get_current_win(data);
            }
        }
        break;

        case kNvimAPI_NVIM_SET_CURRENT_WIN:
        {
            emit on_nvim_set_current_win();}
        break;

        case kNvimAPI_NVIM_LIST_TABPAGES:
        {
            QList<int64_t> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_list_tabpages");
                return;
            }
            else
            {
                emit on_nvim_list_tabpages(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_CURRENT_TABPAGE:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_current_tabpage");
                return;
            }
            else
            {
                emit on_nvim_get_current_tabpage(data);
            }
        }
        break;

        case kNvimAPI_NVIM_SET_CURRENT_TABPAGE:
        {
            emit on_nvim_set_current_tabpage();}
        break;

        case kNvimAPI_NVIM_SUBSCRIBE:
        {
            emit on_nvim_subscribe();}
        break;

        case kNvimAPI_NVIM_UNSUBSCRIBE:
        {
            emit on_nvim_unsubscribe();}
        break;

        case kNvimAPI_NVIM_GET_COLOR_BY_NAME:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_color_by_name");
                return;
            }
            else
            {
                emit on_nvim_get_color_by_name(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_COLOR_MAP:
        {
            QVariantMap data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_color_map");
                return;
            }
            else
            {
                emit on_nvim_get_color_map(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_MODE:
        {
            QVariantMap data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_mode");
                return;
            }
            else
            {
                emit on_nvim_get_mode(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_KEYMAP:
        {
            QList<QVariantMap> data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_keymap");
                return;
            }
            else
            {
                emit on_nvim_get_keymap(data);
            }
        }
        break;

        case kNvimAPI_NVIM_GET_API_INFO:
        {
            QVariantList data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_get_api_info");
                return;
            }
            else
            {
                emit on_nvim_get_api_info(data);
            }
        }
        break;

        case kNvimAPI_NVIM_CALL_ATOMIC:
        {
            QVariantList data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_call_atomic");
                return;
            }
            else
            {
                emit on_nvim_call_atomic(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_GET_BUF:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_buf");
                return;
            }
            else
            {
                emit on_nvim_win_get_buf(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_GET_CURSOR:
        {
            QPoint data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_cursor");
                return;
            }
            else
            {
                emit on_nvim_win_get_cursor(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_SET_CURSOR:
        {
            emit on_nvim_win_set_cursor();}
        break;

        case kNvimAPI_NVIM_WIN_GET_HEIGHT:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_height");
                return;
            }
            else
            {
                emit on_nvim_win_get_height(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_SET_HEIGHT:
        {
            emit on_nvim_win_set_height();}
        break;

        case kNvimAPI_NVIM_WIN_GET_WIDTH:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_width");
                return;
            }
            else
            {
                emit on_nvim_win_get_width(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_SET_WIDTH:
        {
            emit on_nvim_win_set_width();}
        break;

        case kNvimAPI_NVIM_WIN_GET_VAR:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_var");
                return;
            }
            else
            {
                emit on_nvim_win_get_var(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_SET_VAR:
        {
            emit on_nvim_win_set_var();}
        break;

        case kNvimAPI_NVIM_WIN_DEL_VAR:
        {
            emit on_nvim_win_del_var();}
        break;

        case kNvimAPI_NVIM_WIN_GET_OPTION:
        {
            QVariant data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_option");
                return;
            }
            else
            {
                emit on_nvim_win_get_option(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_SET_OPTION:
        {
            emit on_nvim_win_set_option();}
        break;

        case kNvimAPI_NVIM_WIN_GET_POSITION:
        {
            QPoint data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_position");
                return;
            }
            else
            {
                emit on_nvim_win_get_position(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_GET_TABPAGE:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_tabpage");
                return;
            }
            else
            {
                emit on_nvim_win_get_tabpage(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_GET_NUMBER:
        {
            int64_t data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_get_number");
                return;
            }
            else
            {
                emit on_nvim_win_get_number(data);
            }
        }
        break;

        case kNvimAPI_NVIM_WIN_IS_VALID:
        {
            bool data;

            if(decode(res, data))
            {
                m_c->setError(NvimConnector::RuntimeMsgpackError,
                    "Error unpacking return type for nvim_win_is_valid");
                return;
            }
            else
            {
                emit on_nvim_win_is_valid(data);
            }
        }
        break;

        default:
            qWarning() << "Received unexpected response";
    }
}

} // [Namespace] SnailNvimQt
