/// @file plugins/bin/snail/util.h

#ifndef PLUGIN_SNAIL_UTIL_H
#define PLUGIN_SNAIL_UTIL_H

#include <QDebug>
#include <msgpack.h>
#include "plugins/bin/snail/function.h"

QDebug operator<<(QDebug dbg, const msgpack_object &);
QDebug operator<<(QDebug dbg, const SnailNvimQt::Function &f);

namespace SnailNvimQt {

/// Methods used to turn QVariant into native types,
/// used by neovim.cpp to decode QVariants
template <class T>
bool decode(const QVariant &in, QList<T> &out)
{
    out.clear();

    if((QMetaType::Type)in.type() != QMetaType::QVariantList)
    {
        qWarning() << "Attempting to decode as QList<...> when type is"
                   << in.type() << in;
        return true;
    }

    foreach(const QVariant val, in.toList())
    {
        if(!val.canConvert<T>())
        {
            return false;
        }
    }

    foreach(const QVariant val, in.toList())
    {
        out.append(val.value<T>());
    }

    return false;
}
bool decode(const QVariant &in, QVariant &out);
template <class T>
bool decode(const QVariant &in, T &out)
{
    if(!in.canConvert<T>())
    {
        return true;
    }

    out = in.value<T>();
    return false;
}

/// Return false if the variant is an integer with
/// value (0), all other values return true
inline bool variant_not_zero(const QVariant &v)
{
    bool ok=false;
    int int_val = v.toInt(&ok);
    return !ok || int_val != 0;
}

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_UTIL_H
