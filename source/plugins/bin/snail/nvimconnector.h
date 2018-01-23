/// @file plugins/bin/snail/nvimconnector.h

#ifndef PLUGIN_SNAIL_NVIMCONNECTOR_H
#define PLUGIN_SNAIL_NVIMCONNECTOR_H

#include <QObject>
#include <QProcess>
#include <QTextCodec>
#include <QAbstractSocket>
#include "config/nvimapi/auto/nvim.h"
#include "plugins/bin/snail/nvimapi.h"

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
    Q_ENUMS(NvimError)

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

    static NvimConnector *spawn(const QStringList &params=QStringList(),
                                const QString &exe="nvim");

    static NvimConnector *connectToStdInOut(void);
    static NvimConnector *connectToSocket(const QString &);
    static NvimConnector *connectToHost(const QString &host, int port);
    static NvimConnector *connectToNvim(const QString &server=QString());

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

signals:
    /// Emitted when nvim is ready
    void ready(void);
    void error(NvimError err);
    void processExited(int exitCode);

public slots:
    void fatalTimeout(void);

protected:
    void setError(NvimError err, const QString &msg);
    void clearError(void);

protected slots:
    void discoverMetadata(void);
    void processError(QProcess::ProcessError);
    void socketError(void);
    void msgpackError(void);

private:
    MsgpackIODevice *m_dev;
    NvimConnectorHelper *m_helper;
    QString m_errorString;
    NvimError m_error;

    Nvim *m_nvimObj;
    quint64 m_channel;

    // Store connection arguments for reconnect()
    NeovimConnectionType m_ctype;
    QStringList m_spawnArgs; ///< nvim executable arguments
    QString m_spawnExe;      ///< nvim executable program
    QString m_connSocket;
    QString m_connHost;
    int m_connPort;
    bool m_ready;
};

} // namespace::SnailNvimQt

Q_DECLARE_METATYPE(SnailNvimQt::NvimConnector::NvimError)

#endif // PLUGIN_SNAIL_NVIMCONNECTOR_H
