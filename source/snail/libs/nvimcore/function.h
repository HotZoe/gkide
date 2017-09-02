/// @headerfile ""

#ifndef SNAIL_LIBS_NVIMCORE_FUNCTION_H
#define SNAIL_LIBS_NVIMCORE_FUNCTION_H

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
    #include "snail/libs/nvimcore/auto/func_idx.h" // Bring in auto-generated enum
    #endif

    Function();

    Function(const QString &ret, const QString &name,
             QList<QPair<QString,QString>> params, bool can_fail);

    Function(const QString &ret, const QString &name, QList<QString> paramTypes, bool can_fail);
    bool isValid() const;
    bool operator==(const Function &other);
    static Function fromVariant(const QVariant &);
    static QList<QPair<QString,QString>> parseParameters(const QVariantList &obj);


    bool can_fail;        ///< Whether this function call fail without returning
    QString return_type;  ///< Function return type
    QString name;         ///< Function name
    QList<QPair<QString,QString>> parameters; ///< Function parameter types and name

    QString signature() const;

    /// The static list **knowFunctions** holds a list of all the supported
    /// signature. The list is populated at compile time from a code generator.
    const static QList<Function> knownFunctions;
    static FunctionId functionId(const Function &);
private:
    bool m_valid;
};

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_FUNCTION_H
