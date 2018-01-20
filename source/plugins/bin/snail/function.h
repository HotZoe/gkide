/// @file plugins/bin/snail/function.h

#ifndef PLUGIN_SNAIL_FUNCTION_H
#define PLUGIN_SNAIL_FUNCTION_H

#include <QtGlobal>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDebug>
#include <QStringList>
#include <QPoint>

#include "plugins/bin/snail/snail.h"

namespace SnailNvimQt {

/// Representation of a Nvim API function signature
class NvimApiFunc
{
    Q_ENUMS(NvimApiFuncID)
public:

    #ifndef NEOVIMQT_NO_AUTO
    // Bring in auto-generated enum
    #include "config/nvimapi/auto/func_idx.h"
    #endif

    static NvimApiFunc fromVariant(const QVariant &);
    static QList<FuncArg> parseArgs(const QVariantList &obj);

    /// A list of all the supported nvim API signature.
    /// The list is populated at compile time from a code generator.
    static const QList<NvimApiFunc> nvimAPIs;
    static NvimApiFuncID nvimApiID(const NvimApiFunc &);

    NvimApiFunc();
    NvimApiFunc(const QString &ret, const QString &name,
                QList<FuncArg> params, bool can_fail);
    NvimApiFunc(const QString &ret, const QString &name,
                QList<QString> paramTypes, bool can_fail);

    bool isValid() const;
    bool operator==(const NvimApiFunc &other);

    /// API function return type
    QString m_func_type;
    /// API function name
    QString name;
    /// API function argument type and name
    QList<FuncArg> parameters;

    /// Whether this function call fail without returning
    bool can_fail;

    QString signature() const;

private:
    bool m_valid;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_FUNCTION_H
