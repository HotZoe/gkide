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

namespace SnailNvimQt {

/// Representation of a Nvim API function signature
class Function
{
    Q_ENUMS(FunctionId)
public:

    #ifndef NEOVIMQT_NO_AUTO
    // Bring in auto-generated enum
    #include "config/nvimapi/auto/func_idx.h"
    #endif

    static Function fromVariant(const QVariant &);
    static QList<QPair<QString,QString>> parseParameters(const QVariantList &obj);

    /// The static list **knowFunctions** holds a list of all the supported
    /// signature. The list is populated at compile time from a code generator.
    static const QList<Function> knownFunctions;
    static FunctionId functionId(const Function &);

    Function();
    Function(const QString &ret, const QString &name,
             QList<QPair<QString,QString>> params, bool can_fail);
    Function(const QString &ret, const QString &name,
             QList<QString> paramTypes, bool can_fail);

    bool isValid() const;
    bool operator==(const Function &other);

    /// API function return type
    QString return_type;
    /// API function name
    QString name;
    /// API function argument type and name
    QList<QPair<QString,QString>> parameters;

    /// Whether this function call fail without returning
    bool can_fail;

    QString signature() const;

private:
    bool m_valid;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_FUNCTION_H
