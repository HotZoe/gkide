/// @file plugins/bin/snail/msgpackrequest.h

#ifndef SNAIL_LIBS_NVIMCORE_MSGPACKREQUEST_H
#define SNAIL_LIBS_NVIMCORE_MSGPACKREQUEST_H

#include <QObject>
#include <QTimer>
#include "plugins/bin/snail/function.h"

namespace SnailNvimQt {

class MsgpackIODevice;
class MsgpackRequest: public QObject
{
    Q_OBJECT
public:
    MsgpackRequest(quint32 id, MsgpackIODevice *dev, QObject *parent=0);
    void setFunction(Function::FunctionId);
    Function::FunctionId function();
    void setTimeout(int msec);
    /// The identifier for this Msgpack request
    const quint32 id;
signals:
    void finished(quint32 msgid, Function::FunctionId fun, const QVariant &resp);
    void error(quint32 msgid, Function::FunctionId fun, const QVariant &err);
    void timeout(quint32 id);
protected slots:
    void requestTimeout();

private:
    MsgpackIODevice *m_dev;
    Function::FunctionId m_function;
    QTimer m_timer;
};

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_MSGPACKREQUEST_H
