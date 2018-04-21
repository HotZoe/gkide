/// @file plugins/bin/snail/nvimconnector.h

#ifndef PLUGIN_SNAIL_NVIMCONNECTOR_H
#define PLUGIN_SNAIL_NVIMCONNECTOR_H

#include <QObject>
#include <QProcess>
#include <QTextCodec>
#include <QLocalSocket>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QNetworkInterface>

#include "config/nvimapi/auto/nvim.h"
#include "plugins/bin/snail/nvimapi.h"
#include "plugins/bin/snail/version.h"

typedef quint32 ipv4_addr_t;

class ipv6_addr_t
{
public:
    inline quint8 &operator [](int index)
    {
        return data[index];
    }

    inline quint8 operator [](int index) const
    {
        return data[index];
    }

    inline ipv6_addr_t operator=(Q_IPV6ADDR addr)
    {
        data[0] = addr[0];
        data[1] = addr[1];
        data[2] = addr[2];
        data[3] = addr[3];
        data[4] = addr[4];
        data[5] = addr[5];
        return *this;
    }

    inline bool operator==(Q_IPV6ADDR addr) const
    {
        return data[0] == addr[0]
               && data[1] == addr[1]
               && data[2] == addr[2]
               && data[3] == addr[3]
               && data[4] == addr[4]
               && data[5] == addr[5];
    }

    quint8 data[16];
};

namespace SnailNvimQt {

class MsgpackIODevice;
class NvimConnectorHelper;

/// Connection to a Nvim instance
class NvimConnector: public QObject
{
    friend class Nvim;
    friend class NvimConnectorHelper;

    Q_OBJECT

    /// True if the Nvim instance is ready
    /// @see neovimObject
    Q_PROPERTY(bool ready READ isReady NOTIFY ready)

public:
    enum NvimError
    {
        NoError = 0,
        NoMetadata,
        MetadataDescriptorError,
        UnexpectedMsg,
        APIMisMatch,
        NoSuchMethod,
        FailedToStart,
        Crashed,
        SocketError,
        MsgpackError,
        RuntimeMsgpackError,
    };
    //Q_ENUM(NvimError)

    /// Underlying connection used to read nvim
    enum NeovimConnectionType
    {
        OtherConnection,
        SpawnedConnection,
        HostConnection,
        SocketConnection,
    };

    NvimConnector(QIODevice *s);
    NvimConnector(MsgpackIODevice *s);

    static NvimConnector *connectToSocket(const QString &path);
    static NvimConnector *connectToHost(const QString &host, int port);

    static NvimConnector *connectToNvimInstance(const QString &server=QString());
    static NvimConnector *startEmbedNvim(const QStringList &args,
                                         const QString &exe);

    bool canReconnect(void);
    NvimConnector *reconnect(void);

    NvimError errorCause(void);
    QString errorString(void);

    // FIXME: remove this
    MsgpackRequest *attachUi(int64_t width, int64_t height);
    void detachUi(void);

    bool isReady(void);
    Nvim *neovimObject(void);
    uint64_t channel(void);
    QString decode(const QByteArray &);
    QByteArray encode(const QString &);
    NeovimConnectionType connectionType();

    NvimVersion *getNvimVersionObj(void);

signals:
    /// Emitted when nvim is ready
    void ready(void);

    /// This signal is emitted when an error occurs.
    /// Use errorString() to get an error message.
    void error(NvimError err);

    /// If the Nvim process was started using startEmbedNvim()
    /// this signal is emitted when the process exits.
    void processExited(int exitCode);

protected:
    static bool isLocalHost(const QString &addr);

protected:
    void clearError(void);
    void socketError(const QString &msg);
    void setError(NvimError err, const QString &msg);

public slots:
    void fatalTimeout(void);

protected slots:
    void discoverMetadata(void);
    void processError(QProcess::ProcessError);
    void tcpSocketError(QAbstractSocket::SocketError err);
    void unixSocketError(QLocalSocket::LocalSocketError err);
    void msgpackError(void);

private:
    MsgpackIODevice *m_dev;
    NvimConnectorHelper *m_helper;
    QString m_errorString;
    NvimError m_error;

    Nvim *m_nvimObj;
    NvimVersion *m_nvimVer;
    quint64 m_channel;

    // Store connection arguments for reconnect()
    NeovimConnectionType m_ctype;
    QStringList m_spawnArgs; ///< nvim executable arguments
    QString m_spawnExe; ///< nvim executable program
    QString m_connSocket;
    QString m_connHost;
    int m_connPort;
    bool m_ready;
};

} // namespace::SnailNvimQt

Q_DECLARE_METATYPE(SnailNvimQt::NvimConnector::NvimError)

#endif // PLUGIN_SNAIL_NVIMCONNECTOR_H
