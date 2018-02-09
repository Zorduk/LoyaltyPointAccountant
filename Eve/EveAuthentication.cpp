#include "EveAuthentication.h"

#include <time.h>

#include <QtCore>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>
#include <QDesktopServices>
#include <QNetworkReply>

#include "Crypto++/aes.h"
#include "Crypto++/osrng.h"
#include "Crypto++/modes.h"

#include <random>

namespace Eve
{
	/*!
		 * \brief EveCharacterAuthData
		 */
	QString EveCharacterAuthData::SaveToString(const std::array<uint8_t,EveCharacterAuthData::AesKeySize>& AesKey) const
	{
		QString base64Stream;
		QBuffer WriteBuffer;
		std::array<uint8_t, AesKeySize> IV;
		generateIV(IV);
		WriteBuffer.open(QBuffer::ReadWrite);
		QDataStream Streamer(&WriteBuffer);
		Streamer << m_VersionNumber;
		Streamer << m_RefreshCode;
		Streamer << m_AccessCode;
		Streamer << m_CharacterId;
		Streamer << m_CharacterName;
		Streamer << static_cast<qlonglong>(m_AccessCodeExpiry);
		Streamer << m_ScopesGranted;

		CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption Encryptor(
					AesKey.data()
				, AesKey.size()
				, IV.data());

		std::string resultBytes;
		CryptoPP::StringSource(
					reinterpret_cast<const byte*>(WriteBuffer.data().data()), WriteBuffer.data().size(), true,
					new CryptoPP::StreamTransformationFilter(
						Encryptor,
						new CryptoPP::StringSink(resultBytes),
						CryptoPP::StreamTransformationFilter::ONE_AND_ZEROS_PADDING
						)
					);
		// no exception so far -> we succeeded

		QByteArray outputBytes(reinterpret_cast<const char*>(IV.data()), AesKeySize);
		outputBytes.append(QByteArray(resultBytes.c_str(),static_cast<int>(resultBytes.size())));

		base64Stream = outputBytes.toBase64();

		return base64Stream;
	}

	bool EveCharacterAuthData::ReadFromString(const QString& stream, const std::array<uint8_t, EveCharacterAuthData::AesKeySize> &AesKey)
	{
		bool bSuccess = false;
		QByteArray Bytes = QByteArray::fromBase64(stream.toUtf8());

		if (static_cast<size_t>(Bytes.length()) > AesKeySize)
		{
			// (a) read IV
			std::array<uint8_t, AesKeySize> IV;
			for (int i = 0; i < static_cast<int>(AesKeySize); i++)
			{
				IV.data()[i] = static_cast<uint8_t>(Bytes.at(i));
			}
			bool bDecryptAes = true;
			std::string resultBytes;
			try
			{
				// (b) decrypt using AES128
				CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption Encryptor(
					AesKey.data()
					, AesKey.size()
					, IV.data());

				CryptoPP::StringSource(
					reinterpret_cast<const byte*>(Bytes.data() + AesKeySize), Bytes.size() - AesKeySize, true,
					new CryptoPP::StreamTransformationFilter(
						Encryptor,
						new CryptoPP::StringSink(resultBytes),
						CryptoPP::StreamTransformationFilter::ONE_AND_ZEROS_PADDING
					)
				);
			}
			catch (...)
			{
				bDecryptAes = false;
			}
			if (bDecryptAes)
			{
				// no exception so far -> we succeeded
				QByteArray outputBytes = QByteArray(resultBytes.c_str(), static_cast<int>(resultBytes.size()));

				// (c) read data from stream
				QDataStream Streamer(outputBytes);
				int VersionNumberInStream = 0;
				Streamer >> VersionNumberInStream;
				Streamer >> m_RefreshCode;
				Streamer >> m_AccessCode;
				Streamer >> m_CharacterId;
				Streamer >> m_CharacterName;
				if (VersionNumberInStream > 1)
				{
					qlonglong AccessCodeExpiryAsInt64;
					Streamer >> AccessCodeExpiryAsInt64;
					m_AccessCodeExpiry = static_cast<time_t>(AccessCodeExpiryAsInt64);
					if (VersionNumberInStream > 2)
					{
						Streamer >> m_ScopesGranted;
					}
					else
					{
						// we must at least reset previous information if Scopes wasn't saved with scope information
						m_ScopesGranted.clear();
					}
				}
				else
				{
					m_AccessCodeExpiry = 0;
				}
				bSuccess = QDataStream::Ok == Streamer.status();
			}
		}
		return bSuccess;
	}

	void EveCharacterAuthData::generateIV(std::array<uint8_t, AesKeySize> &IV) const
	{
		// generate and store IV
		CryptoPP::NonblockingRng Rng;
		// may throw exception
		try
		{
			Rng.GenerateBlock(reinterpret_cast<byte*>(IV.data()), AesKeySize);
		}
		catch (...)
		{
			// no success
		}
	}

	/*!
		 * \brief EveAuthentication
		 */

	const QUrl eveAuthorizationUrl("https://login.eveonline.com/oauth/authorize");
	const QUrl eveAccessTokenUrl("https://login.eveonline.com/oauth/token");
	// constexpr qint16 LocalPortNum = 8094;
	// the following Url must be the same as configured for this App
	// const QUrl authLocalReply(QString("http://localhost:%1/").arg(LocalPortNum));

	EveAuthentication::EveAuthentication(EveAuthClientConfig Client, QObject* pParent)
		: QObject(pParent)
		, m_LocalReply(static_cast<quint16>(Client.portNum()))
		, m_ClientConfig(Client)
		, m_AuthLocalReply(QString("http://localhost:%1/").arg(Client.portNum()))
	{
	}

	void EveAuthentication::onError(const QString &error, const QString &errorDescription, const QUrl &uri)
	{
		qDebug() << "onError: ";
		qDebug() << error << "\n" << errorDescription << "\n" << uri;
	}

	void EveAuthentication::registerScope(const QString& newScope, bool bReplace)
	{
		// if false == bReplace the scope will be added to the existing list of scopes
		if (0 == m_RequestedScope.length() || bReplace)
			m_RequestedScope = newScope;
		else
		{
			m_RequestedScope += " ";
			m_RequestedScope += newScope;
		}
	}

	bool EveAuthentication::grant(const EveCharacterAuthData &Character)
	{
		bool bGrantedDirectly = false;
		// if we already have a refresh code we can start there instead of a complete new authorization
		if (Character.refreshCode().length() > 0)
		{
			time_t Now = time(&Now);
			// we only need a refresh code if the current one isn't valid for at least another 10s
			if (Character.getAccessCodeExpiry() > Now + 10)
			{
				bGrantedDirectly = true;
			}
			else
			{
				// we start with reactivating the access
				refreshAccess(Character);
			}
		}
		else
		{
			// start anew
			startAuthentication();
		}
		return bGrantedDirectly;
	}

	void EveAuthentication::startAuthentication()
	{
		QUrl AuthorizeUrl("https://login.eveonline.com/oauth/authorize");
		QUrlQuery QueryParam;
		QueryParam.addQueryItem("response_type", "code");
		QueryParam.addQueryItem("redirect_uri", m_AuthLocalReply.toString());
		QueryParam.addQueryItem("client_id",m_ClientConfig.clientId());
		QueryParam.addQueryItem("scope", m_RequestedScope);
		qDebug() << "requesting scope: " << m_RequestedScope;
		QString RandomState = createStateParam();
		QueryParam.addQueryItem("state",RandomState);
		AuthorizeUrl.setQuery(QueryParam);
		auto processAuthCallback = [this, RandomState](const QVariantMap &values) {
			this->m_LocalReply.disconnect(this);
			this->onCallbackReceived(values, RandomState);
		};
		connect(&m_LocalReply, &QOAuthHttpServerReplyHandler::callbackReceived, processAuthCallback);
		QDesktopServices::openUrl(AuthorizeUrl);
	}

	void EveAuthentication::onCallbackReceived(const QVariantMap &values, QString RandomState)
	{
		qDebug() << "onCallbackReceived (from CallbackHandler)";
		qDebug() << values;
		if (values.contains("code")
				&& values.contains("state")
				&& 0 == values.value("state").toString().compare(RandomState))
		{
			onAuthorizationCodeReceived(values.value("code").toString());
		}
	}

	QString EveAuthentication::createStateParam()
	{
		QString randomState;
		std::random_device r;
		// Choose a random mean between 0 and 255
		std::default_random_engine e1(r());
		QString Alphabet("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		std::uniform_int_distribution<int> charDistribution(0, Alphabet.length()-1);
		randomState.clear();
		for (int i=0; i<32; i++)
		{
			randomState += Alphabet.at(charDistribution(e1));
		}
		qDebug() << "RandomState: " << randomState;
		return randomState;
	}

	void EveAuthentication::onAuthorizationCodeReceived(const QString AuthCode)
	{
		getAccessToken(AuthCode);
	}

	void EveAuthentication::getAccessToken(const QString& AuthCode)
	{
		QString Secret = m_ClientConfig.clientId(); // client_id
		Secret += ":";
		Secret += m_ClientConfig.clientSecret();

		QString BasicCode = Secret.toUtf8().toBase64();
		qDebug() << BasicCode;
		QString AuthString = "Basic ";
		AuthString += BasicCode;
		QNetworkRequest Request;
		Request.setUrl(QUrl("https://login.eveonline.com/oauth/token"));
		Request.setRawHeader("Authorization", AuthString.toUtf8());
		Request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
		QString sPostBody = "grant_type=authorization_code&code=";
		sPostBody += AuthCode;
		QNetworkReply* pTokenRequest = m_Network.post(Request, sPostBody.toUtf8());
		if (pTokenRequest)
		{
			connect(pTokenRequest, &QNetworkReply::finished, [this, pTokenRequest](){
				onTokenRequestFinished(pTokenRequest);
			});
			if (pTokenRequest->isFinished())
			{
				onTokenRequestFinished(pTokenRequest);
			}
		}
	}

	void EveAuthentication::onTokenRequestFinished(QNetworkReply *pReply)
	{
		if (pReply)
		{
			if (QNetworkReply::NetworkError::NoError == pReply->error())
			{
				QByteArray Answer = pReply->readAll();
				QJsonDocument Doc = QJsonDocument::fromJson(Answer);
				if (Doc.isObject())
				{
					QJsonObject Root = Doc.object();
					time_t AccessCodeExpiry = time(nullptr) + Root.value("expires_in").toInt();
					EveCharacterAuthData Character;
					Character.setAccessCode(Root.value("access_token").toString(), AccessCodeExpiry);
					Character.setRefreshCode(Root.value("refresh_token").toString());
					qDebug() << "access: " << Character.accessCode();
					qDebug() << "refresh: " << Character.refreshCode();
					getCharacterId(Character);
				}
			}
			else
			{
				qDebug() << pReply->error();
			}
			pReply->deleteLater();
		}
	}

	void EveAuthentication::getCharacterId(EveCharacterAuthData Character)
	{
		QNetworkRequest Request;
		Request.setUrl(QUrl("https://login.eveonline.com/oauth/verify"));
		QString AuthString = "Bearer ";
		AuthString += Character.accessCode();
		Request.setRawHeader("Authorization", AuthString.toUtf8());
		Request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
		QNetworkReply* pCharacterRequest = m_Network.get(Request);
		if (pCharacterRequest)
		{
			connect(pCharacterRequest, &QNetworkReply::finished, [this, Character, pCharacterRequest](){
				onCharacterRequestFinished(pCharacterRequest, Character);
			});
			if (pCharacterRequest->isFinished())
				onCharacterRequestFinished(pCharacterRequest, Character);
		}
	}

	void EveAuthentication::onCharacterRequestFinished(QNetworkReply* pReply, EveCharacterAuthData Character)
	{
		if (pReply)
		{
			if (pReply->error() == QNetworkReply::NetworkError::NoError)
			{
				QByteArray Answer = pReply->readAll();
				qDebug() << Answer;
				QJsonDocument Doc = QJsonDocument::fromJson(Answer);
				if (Doc.isObject())
				{
					QJsonObject Root = Doc.object();
					if (Root.contains("CharacterID") && Root.contains("CharacterName"))
					{
						Character.setCharacterInfo(Root.value("CharacterID").toVariant().toLongLong(), Root.value("CharacterName").toString());
						emit characterAuthenticated(Character);
					}
				}
			}
			pReply->deleteLater();
		}
	}

	void EveAuthentication::refreshAccess(EveCharacterAuthData Character)
	{ // will try to get a new access token based on the stored refresh, should not open any browser
		QString Secret = m_ClientConfig.clientId(); // client_id
		Secret += ":";
		Secret += m_ClientConfig.clientSecret();
		QString BasicCode = Secret.toUtf8().toBase64();
		qDebug() << BasicCode;
		QString AuthString = "Basic ";
		AuthString += BasicCode;
		QNetworkRequest Request;
		Request.setUrl(QUrl("https://login.eveonline.com/oauth/token"));
		Request.setRawHeader("Authorization", AuthString.toUtf8());
		Request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");
		QString sPostBody = "grant_type=refresh_token&refresh_token=";
		sPostBody += Character.refreshCode();
		QNetworkReply* pRefreshRequest = m_Network.post(Request, sPostBody.toUtf8());
		if (pRefreshRequest)
		{
			connect(pRefreshRequest, &QNetworkReply::finished, [this,Character,pRefreshRequest](){
				onRefreshRequestFinished(pRefreshRequest, Character);
			});
			if (pRefreshRequest->isFinished())
			{
				onRefreshRequestFinished(pRefreshRequest, Character);
			}
		}
	}

	bool EveAuthentication::addAuthenticationToHeader(QNetworkRequest &Request, const EveCharacterAuthData & Character) const
	{
		bool bAuthenticationAdded = false;
		if (Character.getAccessCodeExpiry() > getSystemTime() + 10)
		{
			QString AuthString = "Bearer ";
			AuthString += Character.accessCode();
			Request.setRawHeader("Authorization", AuthString.toUtf8());
			bAuthenticationAdded = true;
		}
		return bAuthenticationAdded;
	}

	void EveAuthentication::onRefreshRequestFinished(QNetworkReply* pReply, EveCharacterAuthData Character)
	{
		if (pReply)
		{
			if (QNetworkReply::NetworkError::NoError == pReply->error())
			{
				QByteArray Answer = pReply->readAll();
				QJsonDocument Doc = QJsonDocument::fromJson(Answer);
				if (Doc.isObject())
				{
					QJsonObject Root = Doc.object();
					time_t AccessCodeExpiry = time(nullptr) + Root.value("expires_in").toInt();
					Character.setAccessCode(Root.value("access_token").toString(), AccessCodeExpiry);
					Character.setRefreshCode(Root.value("refresh_token").toString());
					qDebug() << "access: " << Character.accessCode();
					qDebug() << "refresh: " << Character.refreshCode();
					emit characterAuthenticationUpdated(Character);
				}
			}
			else
			{
				qDebug() << pReply->error();
			}
			pReply->deleteLater();
		}
	}
	time_t EveAuthentication::getSystemTime() const
	{
		time_t returnTime = time(&returnTime);
		return returnTime;
	}
}
