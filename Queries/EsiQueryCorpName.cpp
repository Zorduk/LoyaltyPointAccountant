#include "EsiQueryCorpName.h"
#include <QUrl>

bool EsiQueryCorpName::preparePublicQuery(QNetworkRequest & Request)
{
	bool bQueryPrepared = false;
	if (m_CorpId > 0)
	{
		QUrl QueryUrl("https://esi.tech.ccp.is");
		QString UrlPath = QString::fromUtf8("/v4/corporations/%1/").arg(m_CorpId);
		QueryUrl.setPath(UrlPath);
		Request.setUrl(QueryUrl);
		bQueryPrepared = true;
	}
	return bQueryPrepared;
}

bool EsiQueryCorpName::processJsonReply(const QJsonDocument & Doc)
{
	bool bJsonProcessed = false;
	if (Doc.isObject())
	{
		QJsonObject Root = Doc.object();
		if (Root.contains("name"))
		{
			emit corpNameForId(m_CorpId,Root["name"].toString());
			bJsonProcessed = true;
		}
	}

	return bJsonProcessed;
}
