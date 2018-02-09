#ifndef EVEAUTHENTICATION_H
#define EVEAUTHENTICATION_H

#include <QObject>
#include <QtNetworkAuth>
#include <QNetworkAccessManager>
#include <QOAuth2AuthorizationCodeFlow>

#include <stdint.h>
#include <array>

namespace Eve
{
	//! \namespace Eve

	//! EveAuthClientConfig is a simple storage class that contains the client data needed to authorize the client App using OAuth
	class EveAuthClientConfig
	{
	public:
		explicit EveAuthClientConfig(
				const QString clientId
				, const QString& clientSecret
				, const int LocalPortNum)
			: m_ClientId(clientId)
			, m_ClientSecret(clientSecret)
			, m_PortNum(LocalPortNum)
		{	}
		virtual ~EveAuthClientConfig() = default;

		//! return clientId as defined in Eve Developer Dashboard
		const QString& clientId() const { return m_ClientId; }
		//! return clientSecret as defined in Eve Developer Dashboard
		const QString& clientSecret() const { return m_ClientSecret; }
		//! return TCP/IP port used for static callback server, needs to correspond to the callback URL defined in Eve Developer Dashboard
		int portNum() const { return m_PortNum; }

		//! store ClientId
		void setClientId(const QString& ClientId) { m_ClientId = ClientId;}
		//! store ClientSecret
		void setClientSecret(const QString& ClientSecret) { m_ClientSecret = ClientSecret; }
		//! set TCP/IP port number
		void setPortNum(int PortNum) { m_PortNum = PortNum; }

	private:
		QString m_ClientId;
		QString m_ClientSecret;
		int m_PortNum;
	};
	
	//! stores all data needed to authenticate a user with the Eve Api
	class EveCharacterAuthData
	{
	public:
		static constexpr size_t AesKeySize = 16;

		// data access
		const QString& refreshCode() const { return m_RefreshCode; }
		const QString& accessCode() const { return m_AccessCode; }
		int64_t characterId() const { return m_CharacterId; }
		const QString& characterName() const { return m_CharacterName; }
		time_t getAccessCodeExpiry() const { return m_AccessCodeExpiry; }

		// set data
		void setRefreshCode(const QString& RefreshCode) { m_RefreshCode = RefreshCode; }
		void setAccessCode(const QString& AccessCode, time_t Expiry) { m_AccessCode = AccessCode; m_AccessCodeExpiry = Expiry; }
		void setCharacterInfo(int64_t characterId, const QString& CharacterName)
		{	m_CharacterId = characterId; m_CharacterName = CharacterName;	}

		QString SaveToString(const std::array<uint8_t,AesKeySize>& AesKey) const;
		bool ReadFromString(const QString& stream, const std::array<uint8_t,AesKeySize>& AesKey);

	private:
		QString m_RefreshCode; // this one allows to get a new access token whenever we need to
		QString m_AccessCode;

		// some character Infos
		int64_t m_CharacterId;
		QString m_CharacterName;
		time_t m_AccessCodeExpiry = 0;
		// Version of serialized stream (see SaveToString()/ReadFromString())
		int m_VersionNumber = 3;
		// version 3: store allowed Scopes to detect missing roles before running into error conditions
		QStringList m_ScopesGranted;

		void generateIV(std::array<uint8_t, AesKeySize>& IV) const; // generate random bytes for use in IV
	};

	class EveAuthentication : public QObject
	{
		Q_OBJECT
	public:
		EveAuthentication(EveAuthClientConfig Client, QObject* pParent = nullptr);

	public slots:
		//! this will start the authentication sequence
		bool grant(const EveCharacterAuthData& Character);
		//! if false == bReplace the scope will be added to the existing list of scopes
		void registerScope(const QString& newScope, bool bReplace=false); 
		//! return false if Character doesn't have a valid authorization
		bool addAuthenticationToHeader(QNetworkRequest &Request, const EveCharacterAuthData & Character) const;

	signals:
		void characterAuthenticated(const EveCharacterAuthData& Character);
		void characterAuthenticationUpdated(const EveCharacterAuthData& UpdatedCharacter);

	private:
		QOAuthHttpServerReplyHandler m_LocalReply;
		QNetworkAccessManager m_Network;
		EveAuthClientConfig m_ClientConfig;
		QString m_RequestedScope;
		QUrl m_AuthLocalReply;

		//! return current system time as given by time()
		/*! 
			We might later want to cache the current time or manipulate it for testing, so all request are done using this virtual function
		*/
		virtual time_t getSystemTime() const;

	private slots:
		void onError(const QString &error, const QString &errorDescription, const QUrl &uri);
		// void stateChanged(const QString &state);
		void startAuthentication();
		QString createStateParam();
		void getAccessToken(const QString& AccessToken);
		void refreshAccess(EveCharacterAuthData Character); // will try to get a new access token based on the stored refresh, should not open any browser

		// Callback Handler signals
		void onCallbackReceived(const QVariantMap &values, QString RandomState);

		// internal functions to be called on different stages of OAuth
		void onAuthorizationCodeReceived(const QString AuthCode);
		void getCharacterId(EveCharacterAuthData Character);
		// callback to JSON Rest interface
		void onTokenRequestFinished(QNetworkReply* pReply);
		void onCharacterRequestFinished(QNetworkReply* pReply, EveCharacterAuthData Character);
		void onRefreshRequestFinished(QNetworkReply* pReply, EveCharacterAuthData Character);
	};
}
Q_DECLARE_METATYPE(Eve::EveCharacterAuthData);
#endif // EVEAUTHENTICATION_H
