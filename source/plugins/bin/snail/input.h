/// @file plugins/bin/snail/input.h

#ifndef SNAIL_LIBS_NVIMCORE_INPUT_H
#define SNAIL_LIBS_NVIMCORE_INPUT_H

#include <QHash>
#include <QString>
#include <QEvent>
#include <QPoint>

namespace SnailNvimQt {

class InputConv
{
public:
    InputConv();

    QString convertKey(const QString &text, int key, Qt::KeyboardModifiers mod);
    QString modPrefix(Qt::KeyboardModifiers mod);

    QHash<int, QString> specialKeys;
    QHash<QString, QString> replaceKeys;
    QString convertMouse(Qt::MouseButton bt, QEvent::Type type, Qt::KeyboardModifiers mod,
                         QPoint pos, short clicksCount);

protected:
    // define our own key and modifier modifier constants to abstract OS-specific issues
#ifdef Q_OS_MAC
    const Qt::KeyboardModifiers ControlModifier = Qt::MetaModifier;
    const Qt::KeyboardModifiers CmdModifier = Qt::ControlModifier;
    const Qt::KeyboardModifiers MetaModifier  = Qt::AltModifier;
    const Qt::Key Key_Control = Qt::Key_Meta;
    const Qt::Key Key_Cmd = Qt::Key_Control;
#else
    const Qt::KeyboardModifiers ControlModifier = Qt::ControlModifier;
    #ifdef Q_OS_UNIX
    const Qt::KeyboardModifiers CmdModifier = Qt::MetaModifier;
    const Qt::Key Key_Cmd = Qt::Key_Meta;;
    #else
    const Qt::KeyboardModifiers CmdModifier = (Qt::KeyboardModifiers)0;
    const Qt::Key Key_Cmd = (Qt::Key)0;
    #endif
    const Qt::KeyboardModifiers MetaModifier  = Qt::MetaModifier;
    const Qt::Key Key_Control = Qt::Key_Control;
#endif
    const Qt::KeyboardModifiers ShiftModifier = Qt::ShiftModifier;
    const Qt::KeyboardModifiers AltModifier   = Qt::AltModifier;
    const Qt::KeyboardModifiers NoModifier    = Qt::NoModifier;
    const Qt::Key Key_Alt = Qt::Key_Alt;
};

extern InputConv Input;

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_INPUT_H