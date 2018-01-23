/// @file plugins/bin/snail/nvimconnector.cpp

#include <QtGlobal>
#include <QMetaMethod>
#include <QLocalSocket>
#include <QTcpSocket>
#include "plugins/bin/snail/nvimconnector.h"
#include "plugins/bin/snail/msgpackrequest.h"
#include "plugins/bin/snail/nvimconnectorhelper.h"
#include "plugins/bin/snail/msgpackiodevice.h"

namespace SnailNvimQt {

/// Create a new Nvim API connection from an open IO device
NvimConnector::NvimConnector(QIODevice *dev)
    :NvimConnector(new MsgpackIODevice(dev))
{
    /* nothing */
}

NvimConnector::NvimConnector(MsgpackIODevice *dev)
    : QObject(), m_dev(dev), m_helper(NULL), m_error(NoError),
      m_nvimObj(NULL), m_channel(0), m_ctype(OtherConnection),
      m_ready(false)
{
    m_helper = new NvimConnectorHelper(this);
    qRegisterMetaType<NvimError>("NvimError");
    connect(m_dev, &MsgpackIODevice::error,
            this, &NvimConnector::msgpackError);

    if(!m_dev->isOpen())
    {
        return;
    }

    discoverMetadata();
}

/// Sets latest error code and message for this connector
void NvimConnector::setError(NvimError err, const QString &msg)
{
    m_ready = false;

    if(m_error == NoError && err != NoError)
    {
        m_error = err;
        m_errorString = msg;
        qWarning() << "Nvim fatal error" << m_errorString;
        emit error(m_error);
    }
    else
    {
        // Only the first error is raised
        qDebug() << "(Ignored) Nvim fatal error" << msg;
    }
}

/// Reset error state
void NvimConnector::clearError(void)
{
    m_error = NoError;
    m_errorString = "";
}

/// Called when an error takes place
NvimConnector::NvimError NvimConnector::errorCause(void)
{
    return m_error;
}

/// An human readable error message for the last error
QString NvimConnector::errorString(void)
{
    return m_errorString;
}

/// Inform Nvim we are a GUI with the given width/height and want
/// to receive UI events. With/Height are expressed in cells.
///
/// @warning This method might be moved to class Nvim
MsgpackRequest *NvimConnector::attachUi(int64_t width, int64_t height)
{
    // FIXME: this should be in class Nvim
    MsgpackRequest *r = m_dev->startRequestUnchecked("ui_attach", 3);
    connect(r, &MsgpackRequest::timeout,
            this, &NvimConnector::fatalTimeout);
    r->setTimeout(5000);
    m_dev->send(width);
    m_dev->send(height);
    m_dev->send(true);
    return r;
}

/// Stop receiving UI updates
///
/// @warning This method might be moved to class Nvim
void NvimConnector::detachUi(void)
{
    // FIXME: this should be in class Nvim
    m_dev->startRequestUnchecked("ui_detach", 0);
}

/// Returns the channel id used by Nvim to identify this connection
uint64_t NvimConnector::channel(void)
{
    return m_channel;
}

/// Request API information from nvim
void NvimConnector::discoverMetadata(void)
{
    MsgpackRequest *r = m_dev->startRequestUnchecked("vim_get_api_info", 0);
    connect(r, &MsgpackRequest::finished,
            m_helper, &NvimConnectorHelper::handleMetadata);
    connect(r, &MsgpackRequest::error,
            m_helper, &NvimConnectorHelper::handleMetadataError);
    connect(r, &MsgpackRequest::timeout,
            this, &NvimConnector::fatalTimeout);
    r->setTimeout(5000); // 5s
}

/// True if the Nvim instance is ready
///
/// @see ready
bool NvimConnector::isReady(void)
{
    return m_ready;
}

/// Decode a byte array as a string according to 'encoding'
QString NvimConnector::decode(const QByteArray &in)
{
    return m_dev->decode(in);
}

/// Encode a string into the appropriate encoding for this Nvim instance
///
/// see :h 'encoding'
QByteArray NvimConnector::encode(const QString &in)
{
    return m_dev->encode(in);
}

/// Get main NeovimObject
///
/// @warning Do not call this before NvimConnector::ready as been signaled
/// @see NvimConnector::isReady
Nvim *NvimConnector::neovimObject(void)
{
    if(!m_nvimObj)
    {
        m_nvimObj = new Nvim(this);
    }

    return m_nvimObj;
}

/// Launch an embedded nvim process
///
/// @param params   the nvim executable arguments
/// @param exe      the nvim executable program
///
/// @see processExited
NvimConnector *NvimConnector::spawn(const QStringList &params,
                                    const QString &exe)
{
    QStringList args;

    if(params.indexOf("--") == -1)
    {
        args.append(params);
        args << "--embed" << "--headless";
    }
    else
    {
        // nvim accepts a -- argument after
        // which only filenames are passed
        int idx = params.indexOf("--");
        args.append(params.mid(0, idx));
        args << "--embed" << "--headless";
        args.append(params.mid(idx));
    }

    // nvim process
    QProcess *p = new QProcess();
    // snail nvim connector
    NvimConnector *c = new NvimConnector(p);

    c->m_ctype = SpawnedConnection;
    c->m_spawnArgs = params;
    c->m_spawnExe = exe;

    // Qt 4.X old syntax
    connect(p, SIGNAL(error(QProcess::ProcessError)),
            c, SLOT(processError(QProcess::ProcessError)));

    connect(p, SIGNAL(finished(int, QProcess::ExitStatus)),
            c, SIGNAL(processExited(int)));

    // Qt 5.X new syntax
    connect(p, &QProcess::started,
            c, &NvimConnector::discoverMetadata);

    p->start(exe, args);
    return c;
}

/// Connect to Nvim using a local UNIX socket.
///
/// This method also works in Windows, using named pipes.
///
/// @see QLocalSocket
NvimConnector *NvimConnector::connectToSocket(const QString &path)
{
    QLocalSocket *s = new QLocalSocket();
    NvimConnector *c = new NvimConnector(s);
    c->m_ctype = SocketConnection;
    c->m_connSocket = path;
    connect(s, SIGNAL(error(QLocalSocket::LocalSocketError)),
            c, SLOT(socketError()));
    connect(s, &QLocalSocket::connected,
            c, &NvimConnector::discoverMetadata);
    s->connectToServer(path);
    return c;
}

/// Connect to a Nvim through a TCP connection
///
/// @param host is a valid hostname or IP address
/// @param port is the TCP port
///
NvimConnector *NvimConnector::connectToHost(const QString &host, int port)
{
    QTcpSocket *s = new QTcpSocket();
    NvimConnector *c = new NvimConnector(s);
    c->m_ctype = HostConnection;
    c->m_connHost = host;
    c->m_connPort = port;
    connect(s, SIGNAL(error(QAbstractSocket::SocketError)),
            c, SLOT(socketError()));
    connect(s, &QAbstractSocket::connected,
            c, &NvimConnector::discoverMetadata);
    s->connectToHost(host, (quint16)port);
    return c;
}

/// Connect to a running instance of Nvim (if available).
///
/// This method gets the Nvim endpoint from the NVIM_LISTEN_ADDRESS
/// environment variable, if it is not available a new Nvim instance
/// is spawned().
///
/// @see spawn()
NvimConnector *NvimConnector::connectToNvim(const QString &server)
{
    QString addr = server;

    if(addr.isEmpty())
    {
        addr = QString::fromLocal8Bit(qgetenv("NVIM_LISTEN_ADDRESS"));
    }

    if(addr.isEmpty())
    {
        return spawn();
    }

    int colon_pos = addr.lastIndexOf(':');

    if(colon_pos != -1 && colon_pos != 0 && addr[colon_pos-1] != ':')
    {
        bool ok;
        int port = addr.mid(colon_pos+1).toInt(&ok);

        if(ok)
        {
            QString host = addr.mid(0, colon_pos);
            return connectToHost(host, port);
        }
    }

    return connectToSocket(addr);
}

NvimConnector *NvimConnector::connectToStdInOut(void)
{
    return new NvimConnector(MsgpackIODevice::connectToStdInOut());
}

/// Called when running embedded Nvim to
/// report an error with the Nvim process
void NvimConnector::processError(QProcess::ProcessError err)
{
    switch(err)
    {
        case QProcess::FailedToStart:
            setError(FailedToStart, m_dev->errorString());
            break;

        case QProcess::Crashed:
            setError(Crashed, "The nvim process has crashed");
            break;

        default:
            // In practice we should be able to catch
            // other types of errors from the QIODevice
            qDebug() << "nvim process error " << m_dev->errorString();
    }
}

/// Handle errors from QLocalSocket or QTcpSocket
void NvimConnector::socketError(void)
{
    setError(SocketError, m_dev->errorString());
}

/// Handle errors in MsgpackIODevice
void NvimConnector::msgpackError(void)
{
    setError(MsgpackError, m_dev->errorString());
}

/// Raise a fatal error for a Nvim timeout
///
/// Sometimes Nvim takes too long to respond to some requests, or maybe
/// the channel is stuck. In such cases it is preferable to raise and error,
/// internally this is what discoverMetadata does if Nvim does not reply.
void NvimConnector::fatalTimeout(void)
{
    setError(RuntimeMsgpackError, "Nvim is taking too long to respond");
}

/// True if NvimConnector::reconnect can be called to reconnect with Nvim.
/// This is true unless you built the NvimConnector ctor directly instead
/// of using on of the static methods.
bool NvimConnector::canReconnect(void)
{
    return m_ctype != OtherConnection;
}

/// @see NvimConnector::NeovimConnectionType
NvimConnector::NeovimConnectionType NvimConnector::connectionType()
{
    return m_ctype;
}

/// Create a new connection using the same parameters as the current one.
///
/// This is the equivalent of creating a new object with spawn(),
/// connectToHost(), or connectToSocket()
///
/// If canReconnect() returns false, this function will return NULL.
NvimConnector *NvimConnector::reconnect(void)
{
    switch(m_ctype)
    {
        case SpawnedConnection:
            return NvimConnector::spawn(m_spawnArgs, m_spawnExe);

        case HostConnection:
            return NvimConnector::connectToHost(m_connHost, m_connPort);

        case SocketConnection:
            return NvimConnector::connectToSocket(m_connSocket);

        default:
            return NULL;
    }

    return NULL; // NOT-REACHED
}

/// @fn SnailNvimQt::NvimConnector::error(NvimError)
///
/// This signal is emitted when an error occurs. Use NvimConnector::errorString
/// to get an error message.

/// @fn SnailNvimQt::NvimConnector::processExited(int exitStatus)
///
/// If the Nvim process was started using SnailNvimQt::NvimConnector::spawn
/// this signal is emitted when the process exits.

} // namespace::SnailNvimQt

#include "moc_nvimconnector.cpp"
