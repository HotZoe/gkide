/// @file plugins/bin/snail/stdinreader.cpp

#include <QDebug>

#include "plugins/bin/snail/stdinreader.h"

#ifdef _WIN32
    #include <io.h>
    #include <fcntl.h>
#else
    #include <unistd.h>
#endif

namespace SnailNvimQt {

/// @class SnailNvimQt::StdinReader
///
/// A background thread to read data from Stdin
///
/// For Unix systems a better alternative to this is to use QSocketNotifier,
/// with a little work it produces the same effect with none of the overhead.

/// Read from stdin in a background thread with
/// @arg maxSize is the read buffer size
StdinReader::StdinReader(qint64 maxSize, QObject *parent)
    :QThread(parent), m_maxSize(maxSize)
{
#ifdef _WIN32
    setmode(0, _O_BINARY);
#endif

    if(!m_in.open(0, QIODevice::ReadOnly | QIODevice::Unbuffered))
    {
        qWarning("Unable to open stdin for reading");
    }
}

void StdinReader::run()
{
    char *buf = new char[m_maxSize];

    while(true)
    {
        qint64 bytes = read(0, buf, (unsigned int)m_maxSize);

        if(bytes > 0)
        {
            qDebug() << "Reading data from stdin: " << bytes;
            emit dataAvailable(QByteArray(buf, (int)bytes));
        }
    }

    delete buf;
}

/// @fn SnailNvimQt::StdinReader::dataAvailable()
///
/// Emitted when data is read from stdin
///
/// @arg data is no larger than maxSize
/// @see StdinReader::StdinReader

} // namespace::SnailNvimQt
