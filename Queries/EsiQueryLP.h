#pragma once

#include "Eve/EsiQueryBase.h"
#include "models/LPModel.h"

class EsiQueryLP : public Eve::EsiQueryBase
{
	Q_OBJECT
public:
	EsiQueryLP() = default;
	virtual ~EsiQueryLP() = default;

	virtual RequestType getRequestType() override { return Eve::EsiQuery::RequestType::Get; }
	virtual bool prepareQuery(QNetworkRequest& Request, Eve::EveAuthentication& Auth, const Eve::EveCharacterAuthData& Character);

signals:
	void loyaltyPointsForChar(int64_t eveCharId, const CharacterLoyaltyPoints& LoyaltyPoints);

protected:
	virtual bool processJsonReply(const QJsonDocument& Doc) override;

private:
	int64_t m_CharId=0;
};
