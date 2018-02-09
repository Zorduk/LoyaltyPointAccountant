#pragma once

#include <QObject>

#include <memory>

#include "EsiQueryBase.h"
#include "EveAuthentication.h"

namespace Eve
{
	//! private Implementation of EsiApiConnector functions
	class EsiApiConnectorPrivate;
	
	//! EsiApiConnector allows to send request to the ESI Api
	class EsiApiConnector : public QObject
	{
		Q_OBJECT
	public:
		
		//! standard constructor
		EsiApiConnector(QObject* pParent = nullptr);
		virtual ~EsiApiConnector();

		//! deleted - there is no meaningful way to copy 
		EsiApiConnector(const EsiApiConnector& Src) = delete;

		//! deleted
		EsiApiConnector& operator=(const EsiApiConnector& Other) = delete;

		//! start a public request without any authorization
		bool startPublicRequest(EsiQuery& PublicRequest);
		
		//! start a request
		bool startRequest(EsiQuery& Request,const EveCharacterAuthData& Character, EveAuthentication& Authenticator);

	private:
		//! pImpl to keep this header free of implementation details
		std::unique_ptr<EsiApiConnectorPrivate> m_spImpl;
	};
}