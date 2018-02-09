#include "DbCorporation.h"

#include <QSqlQuery>

const QUuid CorporationDbHandler::m_gUid("e32f363b-b9b4-4120-8574-8b83eb50a668");

/******************************************************************************
* DbTableCorporation
*******************************************************************************/
QStringList DbTableCorporation::getCreateStatements(int TargetVersion) const
{
	QStringList createStatements;
	if (TargetVersion > 1)
	{
		QString create;
		create += "CREATE TABLE corporation (";
		create += "corporation_id INTEGER PRIMARY KEY"; // equal to the eve_id (int64_t) of the corp
		create += ", name TEXT"; // will be initialized with something like corp 100029 and corrected by ESI calls later
		create += ");";
		createStatements.append(create);
	}
	return createStatements;
}

bool DbTableCorporation::NeedUpdate(int OldVersion, int UpdatedVersion) const
{
	bool bNeedUpdate = false;
	if (OldVersion <= 1 && UpdatedVersion > 1)
		bNeedUpdate = true; // we need to create the table if UpdatedVersion > 1
	return bNeedUpdate;
}

QStringList DbTableCorporation::getUpdateStatement(int OldVersion, int TargetVersion) const
{
	QStringList updateStatements;
	if (OldVersion <= 1 && TargetVersion > 1)
	{
		updateStatements = getCreateStatements(TargetVersion);
	}
	return updateStatements;
}

QStringList DbTableCorporation::getDeleteStatements() const
{
	QStringList deleteStatements;
	deleteStatements << "DELETE * from corporation;";
	return deleteStatements;
}

/********************************************************************************
* CorporationDbHandler
*********************************************************************************/
void CorporationDbHandler::saveToDb(QVariantCorporationData value, QSqlDatabase& Db)
{
	if (value.isValid())
	{
		CorporationData CorpData = value.value<CorporationData>();
		QString InsertQuery = QString::fromUtf8("INSERT into coporation (corporation_id, name) VALUES (%1, '%2');")
			.arg(CorpData.corpId).arg(CorpData.CorpName);
		
		QSqlQuery insertQuery(Db);
		if (!insertQuery.exec(InsertQuery))
		{
			// try to update if insert fails
			updateInDb(value, Db);
		}
	}
}

void CorporationDbHandler::updateInDb(QVariantCorporationData value, QSqlDatabase& Db)
{
	if (value.isValid())
	{
		CorporationData CorpData = value.value<CorporationData>();
		QString UpdateSql = QString::fromUtf8("UPDATE corporation SET name = '%1' WHERE corporation_id = %2;")
			.arg(CorpData.CorpName).arg(CorpData.corpId);

		QSqlQuery updateQuery(Db);
		if (!updateQuery.exec(UpdateSql))
		{
			emit DbError("error updating corp info", PortableDBBackend::DbErrorCode::General);
		}
	}
}

void CorporationDbHandler::deleteInDb(QVariantCorporationData value, QSqlDatabase& Db)
{
	if (value.isValid())
	{
		CorporationData CorpData = value.value<CorporationData>();
		QString deleteSql = QString::fromUtf8("DELETE from corporation WHERE corporation_id = %1")
			.arg(CorpData.corpId);
		QSqlQuery deleteQuery(Db);
		if (!deleteQuery.exec(deleteSql))
		{
			emit DbError("error deleting corp info", PortableDBBackend::DbErrorCode::General);
		}
	}
}

void CorporationDbHandler::readAll(QSqlDatabase& Db)
{
	QString selectSql = QString::fromUtf8("SELECT corporation_id, name FROM corporation;");
	QSqlQuery selectQuery(selectSql, Db);
	while (selectQuery.next())
	{
		CorpId corpId = selectQuery.value(0).toLongLong();
		QString corpName = selectQuery.value(1).toString();
		emit corpNameFromDb(corpId, corpName);
	}
}
