#pragma once

#include "Eve/EsiQueryBase.h"

class EsiQueryCorpName : public Eve::EsiQueryBase
{
	Q_OBJECT

public:
	EsiQueryCorpName() = default;
	virtual ~EsiQueryCorpName() = default;

	void setCorpId(int64_t corpId) { m_CorpId = corpId; }
	virtual RequestType getRequestType() override	{		return Eve::EsiQuery::RequestType::Get;	}
	virtual bool preparePublicQuery(QNetworkRequest& Request) override;

signals:
	void corpNameForId(int64_t corpId, const QString& CorpName);

protected:
	virtual bool processJsonReply(const QJsonDocument& Doc) override;

private:
	int64_t m_CorpId = 0;
};