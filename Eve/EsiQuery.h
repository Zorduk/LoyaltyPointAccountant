#pragma once

#include <QObject>
#include <QNetworkRequest>

#include "EveAuthentication.h"

namespace Eve
{
	//! EsiQuery defines a single Esi Request that can be used in conjunction with EsiApiConnector
	/**!
	EsiQuery is a QObject derived abstract interface that is intented to be used in conjunction with EdiApiConnector.
	**/
	class EsiQuery : public QObject
	{
		Q_OBJECT
	public:
		EsiQuery(QObject* pParent = nullptr)
			: QObject(pParent) 
		{}

		virtual ~EsiQuery() = default;

		//! specifies the Http Action
		enum RequestType
		{
			Invalid = 0, //!< invalid (not set)
			Get, //!< http GET 
			Post, //!< http POST
			Put,//!< http PUT
			Delete, //!< http DELETE
		};

		//! defines the Request type used by this Api Query
		virtual RequestType getRequestType() = 0;
		
		//! returns the document associated with this query, valid only for RequestType::Post and Put
		virtual bool prepareRequestDocument(QJsonDocument& /*Doc*/) { return false; };

		//! is called right before sending for public requests
		virtual bool preparePublicQuery(QNetworkRequest& /*Request*/) { return false; };

		//! is called right before sending the request and should add authorization headers (if needed)
		virtual bool prepareQuery(QNetworkRequest& /*Request*/, EveAuthentication& /*Auth*/,const EveCharacterAuthData& /*Character*/) { return false; };

		//! main processing function, derived request should emit their own processed result signals
		virtual bool processServerResponse(QNetworkReply::NetworkError /*netError*/,const QByteArray& /*ResultBytes*/, int /*HttpReturnCode*/) { return false; };

	public slots:
		void onServerResult(QNetworkReply::NetworkError netError, const QByteArray& ResultBytes, int HttpReturnCode) { processServerResponse(netError, ResultBytes, HttpReturnCode);	}
	
	signals:
		//! this signal is emitted whenever the Request is finished, be it successfully or with an error condition
		void requestFinished();
	};
}
