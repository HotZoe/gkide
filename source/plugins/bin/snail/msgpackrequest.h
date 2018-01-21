/// @file plugins/bin/snail/msgpackrequest.h

#ifndef PLUGIN_SNAIL_MSGPACKREQUEST_H
#define PLUGIN_SNAIL_MSGPACKREQUEST_H

#include <QObject>
#include <QTimer>
#include "plugins/bin/snail/nvimapi.h"

namespace SnailNvimQt {

class MsgpackIODevice;

/// A MsgpackRequest represents an ongoing API call
class MsgpackRequest: public QObject
{
    Q_OBJECT
public:
    MsgpackRequest(quint32 id, MsgpackIODevice *dev, QObject *parent=0);
    void setFunction(NvimApiFuncID);
    NvimApiFuncID function(void);
    void setTimeout(int msec);

signals:
    /// The request has finished
    void finished(quint32 msgid,
                  NvimApiFuncID fun,
                  const QVariant &resp);
    /// The request has error
    void error(quint32 msgid,
               NvimApiFuncID fun,
               const QVariant &err);
    /// The request timeout
    void timeout(quint32 id);

protected slots:
    void requestTimeout(void);

public:
    const quint32 id; ///< msgpack request identifier

private:
    MsgpackIODevice *m_dev;
    NvimApiFuncID m_function;
    QTimer m_timer;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_MSGPACKREQUEST_H
