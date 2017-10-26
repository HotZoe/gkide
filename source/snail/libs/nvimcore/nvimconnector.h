/// @file snail/libs/nvimcore/nvimconnector.h

#ifndef SNAIL_LIBS_NVIMCORE_NVIMCONNECTOR_H
#define SNAIL_LIBS_NVIMCORE_NVIMCONNECTOR_H

#include <QObject>
#include <QProcess>
#include <QTextCodec>
#include <QAbstractSocket>
#include "snail/libs/nvimcore/function.h"
#include "snail/libs/nvimcore/auto/nvim.h"

namespace SnailNvimQt {

class MsgpackIODevice;
class NvimConnectorHelper;

class NvimConnector: public QObject
{
    friend class Neovim;
    friend class NvimConnectorHelper;

    Q_OBJECT

    /// True if the Neovim instance is ready
    /// @see neovimObject
    Q_PROPERTY(bool ready READ isReady NOTIFY ready)
    Q_ENUMS(NeovimError)

public:
    enum NeovimError
    {
        NoError=0,
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

    /// Underlying connection used to read Neovim
    enum NeovimConnectionType
    {
        OtherConnection,
        SpawnedConnection,
        HostConnection,
        SocketConnection,
    };

    NvimConnector(QIODevice *s);
    NvimConnector(MsgpackIODevice *s);
    static NvimConnector *spawn(const QStringList &params=QStringList(), const QString &exe="nvim");
    static NvimConnector *connectToSocket(const QString &);
    static NvimConnector *connectToHost(const QString &host, int port);
    static NvimConnector *connectToNeovim(const QString &server=QString());
    static NvimConnector *fromStdinOut();

    bool canReconnect();
    NvimConnector *reconnect();

    NeovimError errorCause();
    QString errorString();

    // FIXME: remove this
    MsgpackRequest *attachUi(int64_t width, int64_t height);
    void detachUi();

    bool isReady();
    Neovim *neovimObject();
    uint64_t channel();
    QString decode(const QByteArray &);
    QByteArray encode(const QString &);
    NeovimConnectionType connectionType();

signals:
    /// Emitted when Neovim is ready
    void ready(void);
    void error(NeovimError err);
    void processExited(int exitCode);

public slots:
    void fatalTimeout(void);

protected:
    void setError(NeovimError err, const QString &msg);
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
    NeovimError m_error;

    Neovim *m_neovimobj;
    quint64 m_channel;

    // Store connection arguments for reconnect()
    NeovimConnectionType m_ctype;
    QStringList m_spawnArgs;
    QString m_spawnExe;
    QString m_connSocket, m_connHost;
    int m_connPort;
    bool m_ready;
};

} // [Namespace] SnailNvimQt

Q_DECLARE_METATYPE(SnailNvimQt::NvimConnector::NeovimError)

#endif // SNAIL_LIBS_NVIMCORE_NVIMCONNECTOR_H
