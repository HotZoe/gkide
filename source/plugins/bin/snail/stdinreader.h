/// @file plugins/bin/snail/stdinreader.h

#ifndef PLUGIN_SNAIL_STDINNOTIFIER_H
#define PLUGIN_SNAIL_STDINNOTIFIER_H

#include <QThread>
#include <QFile>

namespace SnailNvimQt {

class StdinReader: public QThread
{
    Q_OBJECT
public:
    StdinReader(qint64 maxSize, QObject *parent=0);
    virtual void run();
signals:
    void dataAvailable(const QByteArray &data);

private:
    QFile m_in;
    qint64 m_maxSize;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_STDINNOTIFIER_H
