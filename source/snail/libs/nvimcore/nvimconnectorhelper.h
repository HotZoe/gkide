#ifndef NEOVIM_QT_CONNECTORHELPER
#define NEOVIM_QT_CONNECTORHELPER

#include "snail/libs/nvimcore/nvimconnector.h"

namespace SnailNvimQt {

class NvimConnectorHelper: public QObject
{
	Q_OBJECT
public:
    NvimConnectorHelper(NvimConnector *);

public slots:
	void handleMetadata(quint32, Function::FunctionId, const QVariant& result);
	void handleMetadataError(quint32 msgid, Function::FunctionId,
		const QVariant& errobj);
	void encodingChanged(const QVariant&);
protected:
	bool checkFunctions(const QVariantList& ftable);
private:
	NvimConnector *m_c;

};

} // Namespace SnailNvimQt
#endif
