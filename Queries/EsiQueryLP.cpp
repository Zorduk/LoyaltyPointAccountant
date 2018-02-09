#include "EsiQueryLP.h"

bool EsiQueryLP::prepareQuery(QNetworkRequest &Request, Eve::EveAuthentication& Auth, const Eve::EveCharacterAuthData & CharAuthData)
{
	bool bQueryPrepared = false;
	QUrl QueryUrl("https://esi.tech.ccp.is");
	QString UrlPath = QString::fromUtf8("/v1/characters/%1/loyalty/points/").arg(CharAuthData.characterId());
	QueryUrl.setPath(UrlPath);
	Request.setUrl(QueryUrl);
	m_CharId = CharAuthData.characterId();
	qDebug() << "using " << QueryUrl << "to query LP for character " << m_CharId;
	bQueryPrepared = Auth.addAuthenticationToHeader(Request, CharAuthData);
	return bQueryPrepared;
}

bool EsiQueryLP::processJsonReply(const QJsonDocument & Doc)
{
	bool bResponseParsed = false;
	if (Doc.isArray())
	{
		CharacterLoyaltyPoints LP;
		for (auto Elem : Doc.array())
		{
			if (Elem.isObject())
			{
				int64_t CorpId = Elem.toObject().value("corporation_id").toVariant().toLongLong();
				int64_t Amount = Elem.toObject().value("loyalty_points").toVariant().toLongLong();
				LP[CorpId] = Amount;
			}
		}
		emit loyaltyPointsForChar(m_CharId, LP);
		bResponseParsed = true;
	}
	return bResponseParsed;
}
