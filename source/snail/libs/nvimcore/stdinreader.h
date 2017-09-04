/// @file snail/libs/nvimcore/stdinreader.h

#ifndef SNAIL_LIBS_NVIMCORE_STDINNOTIFIER_H
#define SNAIL_LIBS_NVIMCORE_STDINNOTIFIER_H

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

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_STDINNOTIFIER_H
