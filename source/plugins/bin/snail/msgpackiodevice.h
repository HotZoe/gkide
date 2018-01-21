/// @file plugins/bin/snail/msgpackiodevice.h

#ifndef PLUGIN_SNAIL_MSGPACKIODEVICE_H
#define PLUGIN_SNAIL_MSGPACKIODEVICE_H

#include <QIODevice>
#include <QHash>
#include <msgpack.h>

namespace SnailNvimQt {

class MsgpackRequest;
class MsgpackRequestHandler;

/// A msgpack-rpc channel build on top of QIODevice
class MsgpackIODevice: public QObject
{
    Q_OBJECT
    Q_ENUMS(MsgpackError)
    Q_PROPERTY(MsgpackError error
               READ     errorCause
               NOTIFY   error)
    Q_PROPERTY(QByteArray encoding
               READ     encoding
               WRITE    setEncoding)
public:
    enum MsgpackError
    {
        NoError             = 0,
        InvalidDevice       = 1,
        InvalidMsgpack      = 2,
        UnsupportedEncoding = 3
    };
    enum MsgpackMsgType
    {
        msgRequest      = 0,
        msgResponse     = 1,
        msgNotification = 2,
        msgUnsupported  = 3,
    };

    MsgpackIODevice(QIODevice *, QObject *parent=0);
    ~MsgpackIODevice();

    static MsgpackIODevice *fromStdinOut(QObject *parent=0);

    bool isOpen(void);
    QString errorString(void) const;
    MsgpackError errorCause(void) const
    {
        return m_error;
    }

    QByteArray encoding(void) const;
    bool setEncoding(const QByteArray &);

    quint32 msgId(void);
    MsgpackRequest *startRequestUnchecked(const QString &method,
                                          quint32 argcount);

    void send(int64_t);
    void send(const QVariant &);
    void send(const QByteArray &);
    void send(bool);
    void send(const QList<QByteArray> &list);

    template <class T>
    void sendArrayOf(const QList<T> &list)
    {
        msgpack_pack_array(&m_pk, list.size());

        foreach(const T &elem, list)
        {
            send(elem);
        }
    }

    QByteArray encode(const QString &);
    QString decode(const QByteArray &);
    bool checkVariant(const QVariant &);

    bool sendResponse(uint64_t msgid, const QVariant &err, const QVariant &res);
    bool sendNotification(const QByteArray &method, const QVariantList &params);

    void setRequestHandler(MsgpackRequestHandler *);

    /// Typedef for msgpack-to-Qvariant decoder @see registerExtType
    typedef QVariant(*msgpackExtDecoder)(MsgpackIODevice *,
                                         const char *data,
                                         quint32 size);
    void registerExtType(int8_t type, msgpackExtDecoder);

    QList<quint32> pendingRequests(void) const;

signals:
    void error(MsgpackError);
    /// A notification with the given name and arguments was received
    void notification(const QByteArray &name, const QVariantList &args);

protected:
    void sendError(const msgpack_object &req, const QString &msg);
    void sendError(uint64_t msgid, const QString &msg);
    void dispatch(msgpack_object &obj);
    void dispatchRequest(msgpack_object &obj);
    void dispatchResponse(msgpack_object &obj);
    void dispatchNotification(msgpack_object &obj);

    bool decodeMsgpack(const msgpack_object &in, int64_t &out);
    bool decodeMsgpack(const msgpack_object &in, QVariant &out);
    bool decodeMsgpack(const msgpack_object &in, QByteArray &out);
    bool decodeMsgpack(const msgpack_object &in, bool &out);
    bool decodeMsgpack(const msgpack_object &in, QList<QByteArray> &out);
    bool decodeMsgpack(const msgpack_object &in, QList<int64_t> &out);
    bool decodeMsgpack(const msgpack_object &in, QPoint &out);

protected slots:
    void setError(MsgpackError err, const QString &msg);

    void dataAvailable(void);
    void dataAvailableStdin(const QByteArray &);
    void dataAvailableFd(int fd);

    void requestTimeout(quint32 id);

private:
    static int msgpack_write_to_stdout(void *data,
                                       const char *buf,
                                       unsigned long int len);
    static int msgpack_write_to_dev(void *data,
                                    const char *buf,
                                    unsigned long int len);

    quint32 m_reqid;
    QIODevice *m_dev;

    /// conversion between text encodingslocale->unicode
    QTextCodec *m_encoding;

    msgpack_packer m_pk;    ///< msgpack packer
    msgpack_unpacker m_uk;  ///< msgpack unpacker

    QHash<quint32, MsgpackRequest *> m_requests;
    MsgpackRequestHandler *m_reqHandler;
    QHash<int8_t, msgpackExtDecoder> m_extTypes;

    QString m_errorString;
    MsgpackError m_error;
};

class MsgpackRequestHandler
{
public:
    virtual void handleRequest(MsgpackIODevice *,
                               quint32 msgid,
                               const QByteArray &,
                               const QVariantList &)=0;
};

} // namespace::SnailNvimQt

Q_DECLARE_METATYPE(SnailNvimQt::MsgpackIODevice::MsgpackError)

#endif // PLUGIN_SNAIL_MSGPACKIODEVICE_H
