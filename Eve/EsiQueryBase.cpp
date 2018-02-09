#include "EsiQueryBase.h"

namespace Eve
{
	EsiQueryBase::EsiQueryBase(QObject * pParent)
		: EsiQuery(pParent)
	{
		connect(this, &EsiQuery::requestFinished, this, &EsiQueryBase::onRequestFinished);
	}
	bool EsiQueryBase::processServerResponse(QNetworkReply::NetworkError netError, const QByteArray & ResultBytes, int /*HttpReturnCode*/)
	{
		bool bServerProcessed = false;
		if (QNetworkReply::NetworkError::NoError == netError)
		{
			// nice, we got a proper server reply
			QJsonDocument JsonDoc = QJsonDocument::fromJson(ResultBytes);
			bServerProcessed = processJsonReply(JsonDoc);
		}
		return bServerProcessed;
	}
}