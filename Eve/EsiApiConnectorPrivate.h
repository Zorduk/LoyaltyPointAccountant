#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include "EsiQuery.h"

#include "EsiApiConnector.h"

namespace Eve
{
	//! helper class to connect QNetworkReply with the corresponding EsiQuery
	/*!
		I want to group a QNetwortReply with the corresponding EsiQuery, which I can't do with a single main Connector object
	*/
	class EsiNetworkReplyHandler : public QObject
	{
		Q_OBJECT
	public:
		EsiNetworkReplyHandler(QObject* pParent = nullptr);

	public slots:
		void finished();
		virtual void setReply(QNetworkReply* pReply);

	private slots:
		void onSslErrors(const QList<QSslError> & errors);

	signals:
		void serverResponse(QNetworkReply::NetworkError netError, const QByteArray& ResultBytes, int HttpReturnCode);
		void requestFinished();

	private:
		QNetworkReply* m_pReply = nullptr; //!< stored pointer to the related QNetworkReply
	};

	class EsiApiConnectorPrivate : public QObject
	{
		Q_OBJECT
	public:
		EsiApiConnectorPrivate(EsiApiConnector& PublicInterface);

		bool startPublicRequest(EsiQuery& PublicRequest);

		//! start a request
		bool startRequest(EsiQuery& Request,const  EveCharacterAuthData& Character, EveAuthentication& Authenticator);

	private:
		QNetworkAccessManager m_Network;
		//! work through all the steps that public and character dependent request have in common, such as querying the type and send the appropriate function
		bool sendRequest(EsiQuery& EsiRequest, QNetworkRequest& NetworkRequest);
		bool sendGetRequest(EsiQuery& EsiRequest, QNetworkRequest& NetworkRequest);
		bool sendPutRequest(EsiQuery& EsiRequest, QNetworkRequest& NetworkRequest);
		bool sendDeleteRequest(EsiQuery& EsiRequest, QNetworkRequest& NetworkRequest);
		bool sendPostRequest(EsiQuery& EsiRequest, QNetworkRequest& NetworkRequest);
		bool handleReply(EsiQuery& EsiRequest, QNetworkReply* pReply);
	};

	class EsiApiRefreshWaiter : public QObject
	{
		Q_OBJECT
	public:
		explicit EsiApiRefreshWaiter(EsiApiConnectorPrivate& Connector, EsiQuery& Request, const EveCharacterAuthData& Character, EveAuthentication& Authenticator);

	private slots:
		void onCharacterAuthenticationUpdated(const EveCharacterAuthData& UpdatedCharacter);

	private:
		EsiApiConnectorPrivate & m_Connector;
		EsiQuery& m_Query;
		EveCharacterAuthData m_Char;
		EveAuthentication& m_Auth;
	};
}
