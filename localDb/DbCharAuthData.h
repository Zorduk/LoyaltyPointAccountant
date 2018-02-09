#pragma once

#include "PortableDbBackend/databackend.h"
#include "PortableDbBackend/DbHandler.h"

#include "Eve/EveAuthentication.h"

// this file defines 2 classes: Database Definition and Database Handler
class DbCharAuthTable : public PortableDBBackend::ITableDefinition
{
public:
	virtual QStringList getCreateStatements(int TargetVersion) const override;
	virtual bool NeedUpdate(int OldVersion, int UpdatedVersion)const override;
	virtual QStringList getUpdateStatement(int OldVersion, int TargetVersion)const override;
	virtual QStringList getDeleteStatements() const override;
};

class CharAuthDbHandler : public PortableDBBackend::DbDataHandlerBase
{
	Q_OBJECT
public:
	CharAuthDbHandler(std::array<uint8_t, Eve::EveCharacterAuthData::AesKeySize> AesKey)
		: m_AesKey(AesKey) {}

	virtual ~CharAuthDbHandler() = default;
	virtual QUuid uuid() const { return m_gUid; }

	// operation
	virtual void saveToDb(QVariant /*value*/, QSqlDatabase& /*Db*/) override;
	virtual void updateInDb(QVariant /*value*/, QSqlDatabase& /*Db*/) override;
	virtual void deleteInDb(QVariant /*value*/, QSqlDatabase& /*Db*/) override;
	virtual void readAll(QSqlDatabase& /*Db*/) override;

	// access to our static QUuid
	static const QUuid& HandlerUuid() { return m_gUid; }

signals:
	void characterReadFromDb(int64_t eveCharacterId, const Eve::EveCharacterAuthData& AuthData);

private:
	static const QUuid m_gUid;
	std::array<uint8_t, Eve::EveCharacterAuthData::AesKeySize> m_AesKey;

};