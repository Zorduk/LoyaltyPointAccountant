#pragma once

#include "PortableDbBackend/databackend.h"
#include "PortableDbBackend/DbHandler.h"

#include "models/LPModel.h"

class DbTableLoyaltyPoints : public PortableDBBackend::ITableDefinition
{
public:
	virtual QStringList getCreateStatements(int TargetVersion) const override;
	virtual bool NeedUpdate(int OldVersion, int UpdatedVersion)const override;
	virtual QStringList getUpdateStatement(int OldVersion, int TargetVersion)const override;
	virtual QStringList getDeleteStatements() const override;
};

struct CharLPData { int64_t eveCharId; CharacterLoyaltyPoints LPData; };
Q_DECLARE_METATYPE(CharLPData);

using LPStorageQVariant = QVariant; // a QVariant that stores a CharLPData

class DbLPHandler : public PortableDBBackend::DbDataHandlerBase
{
	Q_OBJECT
public:
	DbLPHandler() = default;
	virtual ~DbLPHandler() = default;

	virtual QUuid uuid() const { return m_gUid; }

	virtual void saveToDb(LPStorageQVariant value, QSqlDatabase& Db) override;
	virtual void readAll(QSqlDatabase& Db) override;

	// static access to our static QUuid
	static const QUuid& HandlerUuid() { return m_gUid; }

signals:
	void loyaltyPointsForChar(int64_t eveCharId, const CharacterLoyaltyPoints& LoyaltyPoints);

private:
	using CharList = std::vector<int64_t>;
	static const QUuid m_gUid;
	
	CharList getCharList(QSqlDatabase& Db);
	void createCorpIfNeeded(QSqlDatabase& Db, CorpId corpId);
	void deleteLPForChar(CharLPData &charLP, QSqlDatabase & Db);
};