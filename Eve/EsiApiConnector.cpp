#include "EsiApiConnector.h"
#include "EsiApiConnectorPrivate.h"

namespace Eve
{
	/**	\class EsiApiConnector
	EsiApiConnector accepts EsiQuery derived request and sends them to the server using QNetworkManager.
	All requests are done in this objects thread as QNetworkManager will separate all network related tasks.
	
	~~~{.cpp}
	EsiApiConnector Connector;
	SingleQuery Query; // derived from EsiQuery
	Connector.startPublicRequest(Query);
	~~~
	*/

	EsiApiConnector::EsiApiConnector(QObject * pParent)
		: QObject(pParent)
		, m_spImpl(new EsiApiConnectorPrivate(*this))
	{
	}

	EsiApiConnector::~EsiApiConnector()
	{

	}

	bool EsiApiConnector::startPublicRequest(EsiQuery& PublicRequest)
	{
		return m_spImpl->startPublicRequest(PublicRequest);
	}

	bool EsiApiConnector::startRequest(EsiQuery& Request,const EveCharacterAuthData & Character, EveAuthentication & Authenticator)
	{
		return m_spImpl->startRequest(Request, Character, Authenticator);
	}

}