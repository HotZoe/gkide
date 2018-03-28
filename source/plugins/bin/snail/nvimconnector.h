/// @file plugins/bin/snail/nvimconnector.h

#ifndef PLUGIN_SNAIL_NVIMCONNECTOR_H
#define PLUGIN_SNAIL_NVIMCONNECTOR_H

#include <QObject>
#include <QProcess>
#include <QTextCodec>
#include <QAbstractSocket>

#include "config/nvimapi/auto/nvim.h"
#include "plugins/bin/snail/nvimapi.h"
#include "plugins/bin/snail/version.h"

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
    Q_ENUM(NvimError)

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
    NvimVersion *m_nvimVer;
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
