/// @file plugins/bin/snail/nvimconnectorhelper.h

#ifndef PLUGIN_SNAIL_CONNECTORHELPER_H
#define PLUGIN_SNAIL_CONNECTORHELPER_H

#include "plugins/bin/snail/nvimconnector.h"

namespace SnailNvimQt {

class NvimConnectorHelper: public QObject
{
    Q_OBJECT
public:
    NvimConnectorHelper(NvimConnector *);

public slots:
    void handleMetadata(quint32 msgid,
                        NvimApiFuncID afid,
                        const QVariant &result);
    void handleMetadataError(quint32 msgid,
                             NvimApiFuncID afid,
                             const QVariant &errobj);
    void encodingChanged(const QVariant &);
protected:
    bool checkFunctions(const QVariantList &ftable);

private:
    NvimConnector *m_c;
};

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_CONNECTORHELPER_H
