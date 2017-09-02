// Auto generated {{date}}
#ifndef SNAIL_LIBS_NVIMCORE_AUTO_NVIM_H
#define SNAIL_LIBS_NVIMCORE_AUTO_NVIM_H

#include <msgpack.h>
#include "snail/libs/nvimcore/function.h"

namespace SnailNvimQt {
class NvimConnector;
class MsgpackRequest;

class Neovim: public QObject
{
    Q_OBJECT
public:
    Neovim(NvimConnector *);

protected slots:
    void handleResponse(quint32 id, Function::FunctionId fun, const QVariant &);
    void handleResponseError(quint32 id, Function::FunctionId fun, const QVariant &);

signals:
    void error(const QString &errmsg, const QVariant &errObj);
    void neovimNotification(const QByteArray &name, const QVariantList &args);

private:
    NvimConnector *m_c;

public slots:
    {

        % for f in functions %
    }
{

    % if f.deprecated() %
        }
    // DEPRECATED
    {
        % endif %
    }
    // {{f.signature()}}
    MsgpackRequest * {{f.name}}({{f.argstring}});
    {
        % endfor %
    }

signals:
    {

        % for f in functions %
    }
void on_{{f.name}}({{f.return_type.native_type}});
    void err_{{f.name}}(const QString &, const QVariant &);
    {
        % endfor %
    }
};

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_AUTO_NVIM_H

