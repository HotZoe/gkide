/// @file plugins/bin/snail/msgpackiodevice.cpp

#include <errno.h> // for errno
#include <string.h> // for strerror()

#include <QAbstractSocket>
#include <QTextCodec>
#include <QSocketNotifier>

#include "plugins/bin/snail/util.h"
#include "plugins/bin/snail/msgpackrequest.h"
#include "plugins/bin/snail/msgpackiodevice.h"

// msgpack unpack buffer to extend size
#define UNPACK_BUFFER_EXTEND_SIZE   (8*1024)

namespace SnailNvimQt {

MsgpackIODevice::MsgpackIODevice(QIODevice *dev, QObject *parent)
    : QObject(parent), m_reqid(0), m_dev(dev),
      m_encoding(NULL), m_reqHandler(NULL), m_error(NoError)
{
    //qRegisterMetaType<MsgpackError>("MsgpackError");
    //qRegisterMetaType<NvimApiFuncID>("NvimApiFuncID");

    // for msgpack 1.x
    // must be destroyed by msgpack_unpacker_destroy(msgpack_unpacker*)
    msgpack_unpacker_init(&m_uk, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);

    if(m_dev)
    {
        // for msgpack 1.x
        // use packer as value (not pointer),
        // don't need to call destroy function
        msgpack_packer_write write_func =
            (msgpack_packer_write)MsgpackIODevice::msgpack_write_to_dev;
        msgpack_packer_init(&m_pk, this, write_func);

        m_dev->setParent(this);

        connect(m_dev, &QAbstractSocket::readyRead,
                this, &MsgpackIODevice::dataAvailable);

        if(!m_dev->isSequential())
        {
            setError(InvalidDevice,
                     QString("IO device needs to be sequential"));
            return;
        }
    }
}

MsgpackIODevice::~MsgpackIODevice()
{
    //msgpack_packer_destroy(&m_pk);
    msgpack_unpacker_destroy(&m_uk);
}

/// is IODevice opened and check errors
bool MsgpackIODevice::isOpen(void)
{
    if(m_error == InvalidDevice)
    {
        return false;
    }
    else if(m_dev)
    {
        // for QIODevice
        return m_dev->isOpen();
    }
    else
    {
        // for stdin/stdout
        return true;
    }
}

/// Get the name of the text encoding, used by the
/// MsgpackIODevice::encode and MsgpackIODevice::decode methods
///
/// @see setEncoding
QByteArray MsgpackIODevice::encoding(void) const
{
    if(m_encoding)
    {
        // pointer to subclass
        return m_encoding->name();
    }

    return QByteArray();
}

/// Set encoding for strings in this RPC,
/// if it not set we fall back to UTF-8
bool MsgpackIODevice::setEncoding(const QByteArray &name)
{
    m_encoding = QTextCodec::codecForName(name);

    if(!m_encoding)
    {
        setError(UnsupportedEncoding,
                 QString("Unsupported encoding: %1")
                 .arg(QString::fromLatin1(name)));

        return false;
    }

    return true;
}

int MsgpackIODevice::msgpack_write_to_dev(void *data,
                                          const char *buf,
                                          size_t len)
{
    MsgpackIODevice *c = static_cast<MsgpackIODevice *>(data);
    qint64 bytes = c->m_dev->write(buf, len);

    if(bytes == -1)
    {
        c->setError(InvalidDevice, QString("Error writing to device"));
    }

    return (int)bytes;
}

/// Process incoming data from the underlying device
///
/// @see QIODevice()
void MsgpackIODevice::dataAvailable(void)
{
    qint64 read = 1;

    while(read > 0)
    {
        // unpacker has not enough buffer
        if(msgpack_unpacker_buffer_capacity(&m_uk) == 0)
        {
            if(!msgpack_unpacker_reserve_buffer(&m_uk, UNPACK_BUFFER_EXTEND_SIZE))
            {
                qFatal("Could not allocate memory in unpack buffer");
                return;
            }
        }

        // read data from device to unpacker buffer
        read = m_dev->read(msgpack_unpacker_buffer(&m_uk),
                           msgpack_unpacker_buffer_capacity(&m_uk));

        if(read > 0)
        {
            msgpack_unpacker_buffer_consumed(&m_uk, read);

            msgpack_unpacked obj;
            msgpack_unpacked_init(&obj);

            while(msgpack_unpacker_next(&m_uk, &obj))
            {
                dispatch(obj.data);
            }
        }
    }
}

/// Send error response for the given request message with error-message
void MsgpackIODevice::sendError(const msgpack_object &req, const QString &msg)
{
    // nvim msgpack rpc calls are
    // [type(int), msgid(int), method(int), args(array)]
    if(req.via.array.ptr[0].via.u64 != msgRequest)
    {
        qFatal("Errors can only be send as replies "
               "to nvim for snail message request");
    }

    sendError(req.via.array.ptr[1].via.u64, msg);
}

/// nvim response error, given back a error response to nvim
void MsgpackIODevice::sendError(uint64_t msgid, const QString &msg)
{
    // [type(1), msgid, error, result(nil)]
    msgpack_pack_array(&m_pk, 4);
    msgpack_pack_int(&m_pk, msgResponse);
    msgpack_pack_int(&m_pk, (int)msgid);
    QByteArray utf8 = msg.toUtf8();
    msgpack_pack_bin(&m_pk, utf8.size());
    msgpack_pack_bin_body(&m_pk, utf8.constData(), utf8.size());
    msgpack_pack_nil(&m_pk);
}

/// Do some sanity checks and forward message to the proper handler
void MsgpackIODevice::dispatch(msgpack_object &req)
{
    // nvim msgpack rpc calls are:
    // 0            1            2             3
    // [msgType(0), msgID(uint), method(uint), args(array)]  => request
    // [msgType(1), msgID(uint), error(?),     result(?)]    => response
    // [msgType(2), method,      params]                     => notification
    if(req.type != MSGPACK_OBJECT_ARRAY)
    {
        qDebug() << "Received Invalid msgpack: not an array";
        return;
    }

    if(req.via.array.size < 3 || req.via.array.size > 4)
    {
        qDebug() << "Received Invalid msgpack: msgLen MUST be 3 or 4";
        return;
    }

    if(req.via.array.ptr[0].type != MSGPACK_OBJECT_POSITIVE_INTEGER)
    {
        qDebug() << "Received Invalid msgpack: msgType MUST be an integer";
        return;
    }

    // MsgpackMsgType
    uint64_t msgType = req.via.array.ptr[0].via.u64;
    // internal rpc message ID
    uint64_t msgID = req.via.array.ptr[1].type;

    switch(msgType)
    {
        case msgRequest:
        {
            if(MSGPACK_OBJECT_POSITIVE_INTEGER != msgID)
            {
                qDebug() << "Invalid Request: "
                            "msgID must be a positive integer";
                sendError(req, "msgID must be a positive integer");
                return;
            }

            if(req.via.array.ptr[2].type != MSGPACK_OBJECT_BIN
               && req.via.array.ptr[2].type != MSGPACK_OBJECT_STR)
            {
                qDebug() << "Invalid Request: "
                            "msgMethod MUST be a String";
                sendError(req, "msgMethod must be a positive integer or String");
                return;
            }

            if(req.via.array.ptr[3].type != MSGPACK_OBJECT_ARRAY)
            {
                qDebug() << "Invalid Request: "
                            "msgArguments MUST be an array";
                sendError(req, "msgArguments must be an array");
                return;
            }

            dispatchRequest(req);
            break;
        }
        case msgResponse:
        {
            if(MSGPACK_OBJECT_POSITIVE_INTEGER != msgID)
            {
                qDebug() << "Invalid Response: "
                            "msgID MUST be a positive integer";
                return;
            }

            dispatchResponse(req);
            break;
        }
        case msgNotification:
        {
            dispatchNotification(req);
            break;
        }
        default:
        {
            qDebug() << "Invalid msgType: " << msgType;
        }
    }
}

/// Handle nvim request message
void MsgpackIODevice::dispatchRequest(msgpack_object &req)
{
    // 0            1           2            3
    // [msgType(0), msgID(uint), method(uint), args(array)]
    uint64_t msgid = req.via.array.ptr[1].via.u64;
    QByteArray errmsg("Unknown method");
    QVariant params;
    QByteArray method;

    if(!m_reqHandler)
    {
        goto err;
    }

    if(decodeMsgpack(req.via.array.ptr[2], method))
    {
        qDebug() << "Found unexpected method in request: " << req;
        goto err;
    }

    if(decodeMsgpack(req.via.array.ptr[3], params))
    {
        qDebug() << "Found unexpected parameters in request: " << req;
        goto err;
    }

    m_reqHandler->handleRequest(this, (quint32)msgid, method, params.toList());
    return;

err:
    // Send error reply [type(1), msgid, error, NIL]
    msgpack_pack_array(&m_pk, 4);
    msgpack_pack_int(&m_pk, msgResponse);
    msgpack_pack_int(&m_pk, (int)msgid);
    msgpack_pack_bin(&m_pk, errmsg.size());
    msgpack_pack_bin_body(&m_pk, errmsg.constData(), errmsg.size());
    msgpack_pack_nil(&m_pk);
}

/// Assign a handler for Msgpack-RPC requests
void MsgpackIODevice::setRequestHandler(MsgpackRequestHandler *h)
{
    m_reqHandler = h;
}

/// Send back a response [type(1), msgid(uint), error(...), result(...)]
///
/// @param msgid    message ID of nvim<->snail
/// @param err      error message
/// @param res      result message
bool MsgpackIODevice::sendResponse(uint64_t msgid,
                                   const QVariant &err,
                                   const QVariant &res)
{
    if(!checkVariant(err) || !checkVariant(res))
    {
        sendError(msgid, tr("Internal server error, "
                            "could not serialize response"));
        return false;
    }

    msgpack_pack_array(&m_pk, 4);
    msgpack_pack_int(&m_pk, msgResponse);
    msgpack_pack_int(&m_pk, (int)msgid);
    send(err);
    send(res);
    return true;
}

/// Send [type(2), method, params]
///
/// @return false if the params could not be serialized
bool MsgpackIODevice::sendNotification(const QByteArray &method,
                                       const QVariantList &params)
{
    if(!checkVariant(params))
    {
        return false;
    }

    msgpack_pack_array(&m_pk, 3);
    msgpack_pack_int(&m_pk, msgNotification);
    send(method);
    send(params);
    return true;
}

/// Handle nvim response messages
void MsgpackIODevice::dispatchResponse(msgpack_object &obj)
{
    //  0             1           2           3
    // [msgType(1), msgid(uint), error(?), result(?)]
    uint64_t msgid = obj.via.array.ptr[1].via.u64;

    if(!m_requests.contains((unsigned int)msgid))
    {
        qWarning() << "Unknown Response msgID: " << msgid;
        return;
    }

    MsgpackRequest *req = m_requests.take((unsigned int)msgid);

    if(obj.via.array.ptr[2].type != MSGPACK_OBJECT_NIL)
    {
        // nvim response error
        QVariant val;

        if(decodeMsgpack(obj.via.array.ptr[2], val))
        {
            qWarning() << "Error decoding response error-object";
            goto err;
        }

        emit req->error(req->m_msgid, req->funcId(), val);
    }
    else
    {
        // nvim response ok
        QVariant val;

        if(decodeMsgpack(obj.via.array.ptr[3], val))
        {
            qWarning() << "Error decoding response object";
            goto err;
        }

        emit req->finished(req->m_msgid, req->funcId(), val);
    }

err:
    req->deleteLater();
}

/// Return list of pending request msgIDs
QList<quint32> MsgpackIODevice::pendingRequests(void) const
{
    return m_requests.keys();
}

/// Handle nvim notification messages
void MsgpackIODevice::dispatchNotification(msgpack_object &nt)
{
    //  0           1       2
    // [msgType(2), method, params]
    QByteArray methodName;

    if(decodeMsgpack(nt.via.array.ptr[1], methodName))
    {
        qDebug() << "Received Invalid notification: event MUST be a String";
        return;
    }

    QVariant val;

    if(decodeMsgpack(nt.via.array.ptr[2], val)
       || (QMetaType::Type)val.type() != QMetaType::QVariantList)
    {
        qDebug() << "Unable to unpack notification parameters" << nt;
        return;
    }

    emit notification(methodName, val.toList());
}

/// Sets latest error code and message for this connector
void MsgpackIODevice::setError(MsgpackError err, const QString &msg)
{
    m_error = err;
    m_errorString = msg;
    qWarning() << "MsgpackIO fatal error: " << m_errorString;
    emit error(m_error);
}

/// Return error string for the current error
QString MsgpackIODevice::errorString(void) const
{
    if(m_error)
    {
        return m_errorString;
    }
    else if(m_dev)
    {
        return m_dev->errorString();
    }
    else
    {
        return QString();
    }
}

/// Start an RPC request
///
/// Use send() to pass on each of the call parameters
///
/// @param method    nvim api function name
/// @param argcount  nvim api function arguments count
///
/// @return a MsgpackRequest object.
/// You can connect to its finished() SIGNAL to handle the response
MsgpackRequest *MsgpackIODevice::startRequestUnchecked(const QString &method,
                                                       quint32 argcount)
{
    quint32 msgid = msgId();
    // [type(0), msgid, method, args]
    msgpack_pack_array(&m_pk, 4);
    msgpack_pack_int(&m_pk, msgRequest);
    msgpack_pack_int(&m_pk, msgid);
    const QByteArray &utf8 = method.toUtf8();
    msgpack_pack_bin(&m_pk, utf8.size());
    msgpack_pack_bin_body(&m_pk, utf8.constData(), utf8.size());
    msgpack_pack_array(&m_pk, argcount);
    MsgpackRequest *r = new MsgpackRequest(msgid, this);
    connect(r, &MsgpackRequest::timeout,
            this, &MsgpackIODevice::requestTimeout);
    m_requests.insert(msgid, r);
    return r;
}

/// Request timed out, discard it
void MsgpackIODevice::requestTimeout(quint32 id)
{
    if(m_requests.contains(id))
    {
        MsgpackRequest *r = m_requests.take(id);
        r->deleteLater();
        qWarning() << "Request(" << id << ") timed out: " << r->funcId();
    }
}

/// Returns a new msgid that can be used for a msg
quint32 MsgpackIODevice::msgId(void)
{
    return this->m_reqid++;
}

void MsgpackIODevice::send(int64_t i)
{
    msgpack_pack_int64(&m_pk, i);
}
bool MsgpackIODevice::decodeMsgpack(const msgpack_object &in, int64_t &out)
{
    if(in.type != MSGPACK_OBJECT_POSITIVE_INTEGER)
    {
        qWarning() << "Attempting to decode as int64_t when type is"
                   << in.type << in;
        out = -1;
        return true;
    }

    out = in.via.i64;
    return false;
}

/// Serialise a value into the msgpack stream
void MsgpackIODevice::send(const QByteArray &bin)
{
    msgpack_pack_bin(&m_pk, bin.size());
    msgpack_pack_bin_body(&m_pk, bin.constData(), bin.size());
}

bool MsgpackIODevice::decodeMsgpack(const msgpack_object &in, QByteArray &out)
{
    if(in.type == MSGPACK_OBJECT_BIN)
    {
        out = QByteArray(in.via.bin.ptr, in.via.bin.size);
    }
    else if(in.type == MSGPACK_OBJECT_STR)
    {
        out = QByteArray(in.via.str.ptr, in.via.str.size);
    }
    else
    {
        qWarning() << "Attempting to decode as QByteArray when type is"
                   << in.type << in;
        out = QByteArray();
        return true;
    }

    return false;
}

/// Serialise a valud into the msgpack stream
void MsgpackIODevice::send(bool b)
{
    if(b)
    {
        msgpack_pack_true(&m_pk);
    }
    else
    {
        msgpack_pack_false(&m_pk);
    }
}

bool MsgpackIODevice::decodeMsgpack(const msgpack_object &in, bool &out)
{
    if(in.type != MSGPACK_OBJECT_BOOLEAN)
    {
        qWarning() << "Attempting to decode as bool when type is"
                   << in.type << in;
        out = false;
        return true;
    }

    out = in.via.boolean;
    return false;
}

void MsgpackIODevice::send(const QList<QByteArray> &list)
{
    msgpack_pack_array(&m_pk, list.size());

    foreach(const QByteArray &elem, list)
    {
        send(elem);
    }
}

bool MsgpackIODevice::decodeMsgpack(const msgpack_object &in,
                                    QList<QByteArray> &out)
{
    out.clear();

    if(in.type != MSGPACK_OBJECT_ARRAY)
    {
        qWarning() << "Attempting to decode as QList<QByteArray> when type is"
                   << in.type << in;
        return true;
    }

    for(uint64_t i=0; i<in.via.array.size; i++)
    {
        QByteArray val;

        if(decodeMsgpack(in.via.array.ptr[i], val))
        {
            out.clear();
            return true;
        }

        out.append(val);
    }

    return false;
}

bool MsgpackIODevice::decodeMsgpack(const msgpack_object &in,
                                    QList<int64_t> &out)
{
    out.clear();

    if(in.type != MSGPACK_OBJECT_ARRAY)
    {
        qWarning() << "Attempting to decode as QList<int64_t> when type is"
                   << in.type << in;
        return true;
    }

    for(uint64_t i=0; i<in.via.array.size; i++)
    {
        int64_t val;

        if(decodeMsgpack(in.via.array.ptr[i], val))
        {
            out.clear();
            return true;
        }

        out.append(val);
    }

    return false;
}

/// We use QVariants for RPC functions that use the
/// *Object* type do not use this in any other conditions
bool MsgpackIODevice::decodeMsgpack(const msgpack_object &in, QVariant &out)
{
    switch(in.type)
    {
        case MSGPACK_OBJECT_NIL:
        {
            out = QVariant();
            break;
        }
        case MSGPACK_OBJECT_BOOLEAN:
        {
            out = in.via.boolean;
            break;
        }
        case MSGPACK_OBJECT_NEGATIVE_INTEGER:
        {
            out = QVariant((qint64)in.via.i64);
            break;
        }
        case MSGPACK_OBJECT_POSITIVE_INTEGER:
        {
            out = QVariant((quint64)in.via.u64);
            break;
        }
        case MSGPACK_OBJECT_FLOAT:
        {
            out = in.via.f64;
            break;
        }
        case MSGPACK_OBJECT_STR:
        case MSGPACK_OBJECT_BIN:
        {
            QByteArray val;

            if(decodeMsgpack(in, val))
            {
                qWarning() << "Error unpacking ByteArray as QVariant";
                out = QVariant();
                return true;
            }

            out = val;
            break;
        }
        case MSGPACK_OBJECT_ARRAY:
        {
            // either a QVariantList or a QStringList
            QVariantList ls;

            for(uint64_t i=0; i<in.via.array.size; i++)
            {
                QVariant v;
                bool failed = decodeMsgpack(in.via.array.ptr[i], v);

                if(failed)
                {
                    qWarning() << "Error unpacking Map as QVariantList";
                    out = QVariant();
                    return true;
                }

                ls.append(v);
            }

            out = ls;
            break;
        }
        case MSGPACK_OBJECT_MAP:
        {
            QVariantMap m;

            for(uint64_t i=0; i<in.via.map.size; i++)
            {
                QByteArray key;

                if(decodeMsgpack(in.via.map.ptr[i].key, key))
                {
                    qWarning() << "Error decoding Object(Map) key";
                    out = QVariant();
                    return true;
                }

                QVariant val;

                if(decodeMsgpack(in.via.map.ptr[i].val, val))
                {
                    qWarning() << "Error decoding Object(Map) value";
                    out = QVariant();
                    return true;
                }

                m.insert(key, val);
            }

            out = m;
            break;
        }
        case MSGPACK_OBJECT_EXT:
        {
            if(m_extTypes.contains(in.via.ext.type))
            {
                out = m_extTypes.value(in.via.ext.type)(this,
                                                        in.via.ext.ptr,
                                                        in.via.ext.size);

                if(!out.isValid())
                {
                    qWarning() << "EXT unpacking failed" << in.via.ext.type;
                    return true;
                }
            }
            else
            {
                out = QVariant();
                qWarning() << "Unsupported EXT type found in Object"
                           << in.via.ext.type;
            }

            break;
        }
        default:
        {
            out = QVariant();
            qWarning() << "Unsupported type found in Object" << in.type << in;
            return true;
        }
    }

    return false;
}

/// Register a function to decode Msgpack EXT types
///
/// @see msgpackExtDecoder
void MsgpackIODevice::registerExtType(int8_t type, msgpackExtDecoder fun)
{
    m_extTypes.insert(type, fun);
}

/// Serialise a value into the msgpack stream
///
/// We use QVariants for RPC functions that use the *Object* type.
/// Do not use this in any other conditions
void MsgpackIODevice::send(const QVariant &var)
{
    if(!checkVariant(var))
    {
        msgpack_pack_nil(&m_pk);
        qWarning() << "Trying to pack unsupported variant type"
                   << var.type() << "packing Nil instead";
        return;
    }

    switch((QMetaType::Type)var.type())
    {
        case QMetaType::Void:
        case QMetaType::UnknownType:
            msgpack_pack_nil(&m_pk);
            break;

        case QMetaType::Bool:
            send(var.toBool());
            break;

        case QMetaType::Int:
            msgpack_pack_int(&m_pk, var.toInt());
            break;

        case QMetaType::UInt:
            msgpack_pack_unsigned_int(&m_pk, var.toUInt());
            break;

        case QMetaType::Long:
            msgpack_pack_long_long(&m_pk, var.toLongLong());
            break;

        case QMetaType::LongLong:
            msgpack_pack_long_long(&m_pk, var.toLongLong());
            break;

        case QMetaType::ULong:
            msgpack_pack_unsigned_long_long(&m_pk, var.toULongLong());
            break;

        case QMetaType::ULongLong:
            msgpack_pack_unsigned_long_long(&m_pk, var.toULongLong());
            break;

        case QMetaType::Float:
            msgpack_pack_float(&m_pk, var.toFloat());
            break;

        case QMetaType::Double:
            msgpack_pack_double(&m_pk, var.toDouble());
            break;

        case QMetaType::QByteArray:
            send(var.toByteArray());
            break;

        case QMetaType::QVariantList:
            msgpack_pack_array(&m_pk, var.toList().size());

            foreach(const QVariant &elem, var.toList())
            {
                send(elem);
            }

            break;

        case QMetaType::QVariantMap:
        {
            const QVariantMap &m = var.toMap();
            msgpack_pack_map(&m_pk, m.size());
            QMapIterator<QString,QVariant> it(m);

            while(it.hasNext())
            {
                it.next();
                send(it.key());
                send(it.value());
            }
        }
        break;

        case QMetaType::QPoint:
            // As an array [row, col]
            msgpack_pack_array(&m_pk, 2);
            msgpack_pack_int64(&m_pk, var.toPoint().y());
            msgpack_pack_int64(&m_pk, var.toPoint().x());
            break;

        case QMetaType::QString:
            send(encode(var.toString()));
            break;

        default:
            msgpack_pack_nil(&m_pk);
            qWarning() << "There is a BUG in the QVariant serializer"
                       << var.type();
    }
}

bool MsgpackIODevice::decodeMsgpack(const msgpack_object &in, QPoint &out)
{
    if(in.type != MSGPACK_OBJECT_ARRAY || in.via.array.size != 2)
    {
        goto fail;
    }

    int64_t col, row;

    if(decodeMsgpack(in.via.array.ptr[0], row))
    {
        goto fail;
    }

    if(decodeMsgpack(in.via.array.ptr[1], col))
    {
        goto fail;
    }

    // QPoint is (x,y)  neovim Position is (row, col)
    out = QPoint((int)col, (int)row);
    return false;

fail:
    qWarning() << "Attempting to decode as QPoint failed" << in.type << in;
    out =  QPoint();
    return true;
}

/// Convert string to the proper encoding
/// @see MsgpackIODevice::setEncoding
QByteArray MsgpackIODevice::encode(const QString &str)
{
    if(m_encoding)
    {
        return m_encoding->fromUnicode(str);
    }
    else
    {
        qWarning() << "Encoding String into MsgpackIODevice "
                      "without an encoding (defaulting to utf8)";
        return str.toUtf8();
    }
}

/// Decode byte array as string, from Nvim's encoding
QString MsgpackIODevice::decode(const QByteArray &data)
{
    if(m_encoding)
    {
        return m_encoding->toUnicode(data);
    }
    else
    {
        qWarning() << "Decoding String from MsgpackIODevice "
                      "without an encoding (defaulting to utf8)";
        return QString::fromUtf8(data);
    }
}

/// QVariant can encapsulate some data types that
/// cannot be serialised at as msgpack data..
///
/// Ideally we would detect such errors early on and avoid sending those
/// objects entirely, this function checks if QVariant objects are admissible
///
/// Returns true if the QVariant can be serialized
bool MsgpackIODevice::checkVariant(const QVariant &var)
{
    switch((QMetaType::Type)var.type())
    {
        case QMetaType::UnknownType:
            break;

        case QMetaType::Bool:
            break;

        case QMetaType::Int:
            break;

        case QMetaType::UInt:
            break;

        case QMetaType::Long:
            break;

        case QMetaType::LongLong:
            break;

        case QMetaType::ULong:
            break;

        case QMetaType::ULongLong:
            break;

        case QMetaType::Float:
            break;

        case QMetaType::QString:
            break;

        case QMetaType::Double:
            break;

        case QMetaType::QByteArray:
            break;

        case QMetaType::QVariantList:
            foreach(const QVariant &elem, var.toList())
            {
                if(!checkVariant(elem))
                {
                    return false;
                }
            }

            break;

        case QMetaType::QVariantMap:
        {
            const QVariantMap &m = var.toMap();
            QMapIterator<QString,QVariant> it(m);

            while(it.hasNext())
            {
                it.next();

                if(!checkVariant(it.key()))
                {
                    return false;
                }

                if(!checkVariant(it.value()))
                {
                    return false;
                }
            }
        }
        break;

        case QMetaType::QPoint:
            break;

        default:
            return false;
    }

    return true;
}

} // namespace::SnailNvimQt
