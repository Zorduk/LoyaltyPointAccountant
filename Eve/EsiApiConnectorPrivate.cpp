#include "EsiApiConnectorPrivate.h"

namespace Eve
{
	/*******************************************************************************************
	* EsiNetworkReplyHandler
	********************************************************************************************/
	EsiNetworkReplyHandler::EsiNetworkReplyHandler(QObject* pParent)
		: QObject(pParent)
	{

	}

	void EsiNetworkReplyHandler::finished()
	{
		if (m_pReply)
		{
			QNetworkReply::NetworkError NetError = m_pReply->error();
			QString errorMsg = m_pReply->errorString();
			QByteArray serverResult = m_pReply->readAll();
			// get http status to provide a meaningful explanation why it failed
			int HttpStatus = m_pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			emit serverResponse(NetError, serverResult, HttpStatus);
			// delete the connected QNetworkReply
			m_pReply->deleteLater();
			m_pReply = nullptr;
		}
		emit requestFinished();
		// we delete ourselves when the job is done
		deleteLater();
	}

	void EsiNetworkReplyHandler::setReply(QNetworkReply* pReply)
	{
		m_pReply = pReply;
		if (m_pReply)
		{
			connect(m_pReply, &QNetworkReply::finished, this, &EsiNetworkReplyHandler::finished);
			connect(pReply, &QNetworkReply::sslErrors, this, &EsiNetworkReplyHandler::onSslErrors);
			if (m_pReply->isFinished())
			{
				finished();
			}
		}
	}

	void EsiNetworkReplyHandler::onSslErrors(const QList<QSslError> & errors)
	{
		// ssl errors will stop the communication, so we should at least report them to our logging
		for (auto it = errors.begin(); it != errors.end(); it++)
		{
			QString SslErrorMsg = QString::fromLatin1("sslError: %1").arg(it->errorString());
			qDebug() << SslErrorMsg;
		}
	}

	/*******************************************************************************************
	* EsiApiConnectorPrivate
	********************************************************************************************/
	EsiApiConnectorPrivate::EsiApiConnectorPrivate(EsiApiConnector& /*PublicInterface*/)
	{
		// connect our signals to those of the public interface for signal forwarding
	}

	bool EsiApiConnectorPrivate::startPublicRequest(EsiQuery& EsiRequest)
	{
		bool bRequestStarted = false;
		// create Request
		QNetworkRequest Request;
		if (EsiRequest.preparePublicQuery(Request))
		{
			bRequestStarted = sendRequest(EsiRequest, Request);
		}
		return bRequestStarted;
	}

	//! start a request
	bool EsiApiConnectorPrivate::startRequest(EsiQuery& EsiRequest,const EveCharacterAuthData& Character, EveAuthentication& Authenticator)
	{
		bool bRequestStarted = false;
		EsiApiRefreshWaiter* pWaiter = new EsiApiRefreshWaiter(*this, EsiRequest, Character, Authenticator);
		if (Authenticator.grant(Character))
		{
			// create Request
			QNetworkRequest Request;
			if (EsiRequest.prepareQuery(Request, Authenticator, Character))
			{
				bRequestStarted = sendRequest(EsiRequest, Request);
			}
			pWaiter->deleteLater();
		}
		else
		{
			pWaiter = nullptr; // the object is doing its job already and will be deleted by delete later,
		}
		return bRequestStarted;
	}

	bool EsiApiConnectorPrivate::sendRequest(EsiQuery& EsiRequest, QNetworkRequest& NetworkRequest)
	{
		bool bRequestSent = false;
		switch (EsiRequest.getRequestType())
		{
		case EsiQuery::RequestType::Get:
			bRequestSent = sendGetRequest(EsiRequest, NetworkRequest);
			break;
		case EsiQuery::RequestType::Delete:
			bRequestSent = sendDeleteRequest(EsiRequest, NetworkRequest);
			break;
		case EsiQuery::RequestType::Post:
			bRequestSent = sendPostRequest(EsiRequest, NetworkRequest);
			break;
		case EsiQuery::RequestType::Put:
			bRequestSent = sendPutRequest(EsiRequest, NetworkRequest);
			break;
		case EsiQuery::RequestType::Invalid:
			break;
		default:
			break;
		}
		return bRequestSent;
	}

	bool EsiApiConnectorPrivate::sendGetRequest(EsiQuery & EsiRequest, QNetworkRequest & NetworkRequest)
	{
		bool bRequestSent = false;
		// this is easy: no additional data needed
		QNetworkReply* pReply = m_Network.get(NetworkRequest);
		bRequestSent = handleReply(EsiRequest, pReply);
		return bRequestSent;
	}

	bool EsiApiConnectorPrivate::sendPutRequest(EsiQuery & EsiRequest, QNetworkRequest & NetworkRequest)
	{
		bool bRequestSent = false;
		QJsonDocument documentToBeSent;
		if (EsiRequest.prepareRequestDocument(documentToBeSent))
		{
			QByteArray putData = documentToBeSent.toJson(QJsonDocument::JsonFormat::Compact);
			QNetworkReply* pReply = m_Network.put(NetworkRequest, putData);
			bRequestSent = handleReply(EsiRequest, pReply);
		}
		return bRequestSent;
	}

	bool EsiApiConnectorPrivate::sendDeleteRequest(EsiQuery & EsiRequest, QNetworkRequest & NetworkRequest)
	{
		bool bRequestSent = false;
		QNetworkReply* pReply = m_Network.get(NetworkRequest);
		bRequestSent = handleReply(EsiRequest, pReply);
		return bRequestSent;
	}

	bool EsiApiConnectorPrivate::sendPostRequest(EsiQuery & EsiRequest, QNetworkRequest & NetworkRequest)
	{
		bool bRequestSent = false;
		QJsonDocument documentToBeSent;
		if (EsiRequest.prepareRequestDocument(documentToBeSent))
		{
			QByteArray putData = documentToBeSent.toJson(QJsonDocument::JsonFormat::Compact);
			QNetworkReply* pReply = m_Network.post(NetworkRequest, putData);
			bRequestSent = handleReply(EsiRequest, pReply);
		}
		return bRequestSent;
	}

	bool EsiApiConnectorPrivate::handleReply(EsiQuery & EsiRequest, QNetworkReply * pReply)
	{
		bool bReplyHandled = false;
		if (pReply)
		{
			EsiNetworkReplyHandler* pHandler = new EsiNetworkReplyHandler(this);
			if (pHandler)
			{
				connect(pHandler, &EsiNetworkReplyHandler::requestFinished, &EsiRequest, &EsiQuery::requestFinished);
				connect(pHandler, &EsiNetworkReplyHandler::serverResponse, &EsiRequest, &EsiQuery::processServerResponse);
				pHandler->setReply(pReply);
				bReplyHandled = true;
			}
		}
		return bReplyHandled;
	}
	/***********************************
	* EsiApiRefreshWaiter
	***********************************/
	EsiApiRefreshWaiter::EsiApiRefreshWaiter(EsiApiConnectorPrivate& Connector, EsiQuery& Request, const EveCharacterAuthData& Character, EveAuthentication& Authenticator)
		: QObject(&Connector)
		, m_Connector(Connector)
		, m_Query(Request)
		, m_Char(Character)
		, m_Auth(Authenticator)
	{
		connect(&Authenticator, &EveAuthentication::characterAuthenticationUpdated, this, &EsiApiRefreshWaiter::onCharacterAuthenticationUpdated);
	}
	
	void EsiApiRefreshWaiter::onCharacterAuthenticationUpdated(const EveCharacterAuthData& UpdatedCharacter)
	{
		if (m_Char.characterId() == UpdatedCharacter.characterId())
		{
			m_Connector.startRequest(m_Query, UpdatedCharacter, m_Auth);
			deleteLater();
		}
	}
}