#pragma once

#include "PortableDbBackend/databackend.h"
#include "PortableDbBackend/DbHandler.h"

#include "models/CorpModel.h"

// database table definitions: we need the definition for a (NPC) corp and the loyalty points
class DbTableCorporation : public PortableDBBackend::ITableDefinition
{
public:
	virtual QStringList getCreateStatements(int TargetVersion) const override;
	virtual bool NeedUpdate(int OldVersion, int UpdatedVersion)const override;
	virtual QStringList getUpdateStatement(int OldVersion, int TargetVersion)const override;
	virtual QStringList getDeleteStatements() const override;
};

// handler for Corp
using CorporationData = struct { CorpId corpId; QString CorpName; };
Q_DECLARE_METATYPE(CorporationData);
using QVariantCorporationData = QVariant;

class CorporationDbHandler : public PortableDBBackend::DbDataHandlerBase
{
	Q_OBJECT
public:
	CorporationDbHandler() = default;
	virtual ~CorporationDbHandler() = default;

	virtual QUuid uuid() const { return m_gUid; }

	// operation
	virtual void saveToDb(QVariantCorporationData value, QSqlDatabase& Db) override;
	virtual void updateInDb(QVariantCorporationData value, QSqlDatabase& Db) override;
	virtual void deleteInDb(QVariantCorporationData value, QSqlDatabase& Db) override;
	virtual void readAll(QSqlDatabase& Db) override;

	// access to our static QUuid
	static const QUuid& HandlerUuid() { return m_gUid; }

signals:
	void corpNameFromDb(CorpId eveCorpId, const QString& CorpName);

private:
	static const QUuid m_gUid;
};