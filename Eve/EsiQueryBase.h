#pragma once
#include <QObject>
#include <QUrl>

#include "EsiQuery.h"

namespace Eve
{
	//! convenience base class that reduces the complexity of EsiQuery so that derived classes only need to override 'processResults()'
	class EsiQueryBase : public EsiQuery
	{
		Q_OBJECT
	public:
		EsiQueryBase(QObject* pParent = nullptr);
		virtual ~EsiQueryBase() = default;

		virtual bool processServerResponse(QNetworkReply::NetworkError netError, const QByteArray& ResultBytes, int HttpReturnCode) override;
	protected:
		virtual bool processJsonReply(const QJsonDocument& Doc) = 0;

	private slots:
		void onRequestFinished() { deleteLater(); }
	};
}