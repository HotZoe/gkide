/// @file plugins/bin/snail/nvimapi.h

#ifndef PLUGIN_SNAIL_NVIMAPI_H
#define PLUGIN_SNAIL_NVIMAPI_H

#include <QtGlobal>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QDebug>
#include <QStringList>
#include <QPoint>

#include "plugins/bin/snail/snail.h"

namespace SnailNvimQt {

#include "config/nvimapi/auto/NvimApiFuncID.c"

/// Representation of a Nvim API function signature
class NvimApiFunc
{
    Q_ENUMS(NvimApiFuncID)
public:

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


    QString m_func_type;        ///< API function return type
    QString m_func_name;        ///< API function name
    QList<FuncArg> m_func_args; ///< API function arguments type and name list

    /// Whether this function call fail without returning
    bool m_can_fail;

    QString signature() const;

private:
    bool m_valid;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_NVIMAPI_H
