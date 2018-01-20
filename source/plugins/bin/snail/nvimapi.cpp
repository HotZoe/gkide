/// @file plugins/bin/snail/nvimapi.cpp

#include <QMetaMethod>
#include <QStringList>
#include "plugins/bin/snail/nvimapi.h"

namespace SnailNvimQt {

/// @enum NvimApiFunc::NvimApiFuncID
///
/// Nvim API function identifiers, the list
/// SnailNvimQt::NvimApiFunc::nvimAPIs is indexed with this enum.

#ifndef NEOVIMQT_NO_AUTO
    #include "config/nvimapi/auto/func_sig.hpp"
#endif

/// Construct invalid function
NvimApiFunc::NvimApiFunc()
    :can_fail(false), m_valid(false)
{
    /* nothing */
}

/// Construct new function with the given
/// return type, name, parameters and error
NvimApiFunc::NvimApiFunc(const QString &ret,
                         const QString &name,
                         QList<FuncArg> params,
                         bool can_fail)
    :m_valid(true)
{
    this->m_func_type = ret;
    this->name = name;
    this->parameters = params;
    this->can_fail = can_fail;
}

/// Construct new function with the given
/// return type, name, parameters and error
NvimApiFunc::NvimApiFunc(const QString &ret,
                   const QString &name,
                   QList<QString> paramTypes,
                   bool can_fail)
    :m_valid(true)
{
    this->m_func_type = ret;
    this->name = name;

    foreach(QString type, paramTypes)
    {
        this->parameters.append(FuncArg(type, ""));
    }

    this->can_fail = can_fail;
}

/// Returns true if this function has all the necessary attributes
bool NvimApiFunc::isValid() const
{
    return m_valid;
}

/// Two functions are considered identical if their names
/// argument and return types, and error status are identical
bool NvimApiFunc::operator==(const NvimApiFunc &other)
{
    if(this->name != other.name)
    {
        return false;
    }

    if(this->m_func_type != other.m_func_type)
    {
        return false;
    }

    if(this->parameters.size() != other.parameters.size())
    {
        return false;
    }

    for(int i=0; i<this->parameters.size(); i++)
    {
        if(this->parameters.at(i).first != other.parameters.at(i).first)
        {
            return false;
        }
    }

    return true;
}

NvimApiFunc NvimApiFunc::fromVariant(const QVariant &fun)
{
    NvimApiFunc f;

    if(!fun.canConvert<QVariantMap>())
    {
        qDebug() << "Found unexpected data type when unpacking function" << fun;
        return f;
    }

    const QVariantMap &m = fun.toMap();
    QMapIterator<QString,QVariant> it(m);

    while(it.hasNext())
    {
        it.next();

        if(it.key() == "return_type")
        {
            if(!it.value().canConvert<QByteArray>())
            {
                qDebug() << "Found unexpected data type when unpacking function"
                         << fun;
                return f;
            }

            f.m_func_type = QString::fromUtf8(it.value().toByteArray());
        }
        else if(it.key() == "name")
        {
            if(!it.value().canConvert<QByteArray>())
            {
                qDebug() << "Found unexpected data type when unpacking function"
                         << fun;
                return f;
            }

            f.name = QString::fromUtf8(it.value().toByteArray());
        }
        else if(it.key() == "can_fail")
        {
            if(!it.value().canConvert<bool>())
            {
                qDebug() << "Found unexpected data type when unpacking function"
                         << fun;
                return f;
            }

            f.can_fail = it.value().toBool();
        }
        else if(it.key() == "parameters")
        {
            if(!it.value().canConvert<QVariantList>())
            {
                qDebug() << "Found unexpected data type when unpacking function"
                         << fun;
                return f;
            }

            f.parameters = parseArgs(it.value().toList());
        }
        else if(it.key() == "id")
        {
            // Deprecated
        }
        else if(it.key() == "receives_channel_id")
        {
            // Internal
        }
        else if(it.key() == "impl_name")
        {
            // Internal
        }
        else if(it.key() == "method")
        {
            // Internal
        }
        else if(it.key() == "noeval")
        {
            // API only function
        }
        else if(it.key() == "deferred" || it.key() == "async")
        {
            // Internal, "deferred" renamed "async" in neovim/ccdeb91
        }
        else if(it.key() == "deprecated_since" || it.key() == "since")
        {
            // Creation/Deprecation
        }
        else
        {
            qDebug() << "Unsupported function attribute"
                     << it.key()
                     << it.value();
        }
    }

    f.m_valid = true;
    return f;
}

/// Retrieve parameter types from a list of function parameters
/// in the metadata object. Basically retrieves the even numbered
/// elements of the array (types), for example:
/// [Type0 name0 Type1 name1 ... ] -> [Type0 Type1 ...]
QList<FuncArg> NvimApiFunc::parseArgs(const QVariantList &obj)
{
    QList<FuncArg> res;
    QList<FuncArg> fail;

    foreach(const QVariant &val, obj)
    {
        const QVariantList &params = val.toList();

        if(params.size() % 2 != 0)
        {
            return fail;
        }

        for(int j=0; j<params.size(); j+=2)
        {
            if(!params.at(j).canConvert<QByteArray>()
               || !params.at(j+1).canConvert<QByteArray>())
            {
                return fail;
            }

            FuncArg arg(QString::fromUtf8(params.at(j).toByteArray()),
                        QString::fromUtf8(params.at(j+1).toByteArray()));
            res.append(arg);
        }
    }

    return res;
}

QString NvimApiFunc::signature() const
{
    QStringList sigparams;

    foreach(const FuncArg p, parameters)
    {
        sigparams.append(QString("%1 %2").arg(p.first).arg(p.second));
    }

    QString notes;

    if(can_fail)
    {
        notes += " !fail";
    }

    return QString("%1 %2(%3)%4")
           .arg(m_func_type)
           .arg(name)
           .arg(sigparams.join(", ")).arg(notes);
}

/// return the NvimApiFuncID or kNvimAPI_NULL if the function is uknown
NvimApiFunc::NvimApiFuncID NvimApiFunc::nvimApiID(const NvimApiFunc &f)
{
    if(!f.isValid())
    {
        return NvimApiFunc::kNvimAPI_NULL;
    }

    int index = NvimApiFunc::nvimAPIs.indexOf(f);

    if(index != -1)
    {
        return NvimApiFunc::NvimApiFuncID(index);
    }

    qDebug() << "Unknown Nvim function" << f.signature();
    return NvimApiFunc::kNvimAPI_NULL;
}

} // namespace::SnailNvimQt

