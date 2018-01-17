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

class Function
{
    Q_ENUMS(FunctionId)
public:

    #ifndef NEOVIMQT_NO_AUTO
    // Bring in auto-generated enum
    #include "config/nvimapi/auto/func_idx.h"
    #endif

    Function();

    Function(const QString &ret, const QString &name,
             QList<QPair<QString,QString>> params, bool can_fail);

    Function(const QString &ret, const QString &name,
             QList<QString> paramTypes, bool can_fail);
    bool isValid() const;
    bool operator==(const Function &other);
    static Function fromVariant(const QVariant &);
    static QList<QPair<QString,QString>> parseParameters(const QVariantList &obj);


    bool can_fail;       ///< Whether this function call fail without returning
    QString return_type; ///< Function return type
    QString name;        ///< Function name
    ///< Function parameter types and name
    QList<QPair<QString,QString>> parameters;

    QString signature() const;

    /// The static list **knowFunctions** holds a list of all the supported
    /// signature. The list is populated at compile time from a code generator.
    const static QList<Function> knownFunctions;
    static FunctionId functionId(const Function &);
private:
    bool m_valid;
};

} // Namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_FUNCTION_H
