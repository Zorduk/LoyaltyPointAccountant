#include "DbLoyaltyPoints.h"

#include <QSqlQuery>
/******************************************************************************
* DbTableLoyaltyPoints
*******************************************************************************/
QStringList DbTableLoyaltyPoints::getCreateStatements(int TargetVersion) const
{
	QStringList createStatements;
	if (TargetVersion > 1)
	{
		QString createLP;
		createLP += "CREATE TABLE loyalty_points (";
		createLP += "loyalty_points_id INTEGER PRIMARY KEY"; // 'autoincrement' 
		createLP += ", eve_char_id INTEGER REFERENCES char_auth_data(eve_char_id) ON DELETE CASCADE ON UPDATE CASCADE"; // foreign key to the eve character
		createLP += ", eve_corp_id INTEGER REFERENCES corporation(corporation_id) ON DELETE CASCADE ON UPDATE RESTRICT"; // foreign key to the eve corp
		createLP += ", amount INTEGER NOT NULL";
		createLP += ");";
		createStatements.append(createLP);
	}
	return createStatements;
}

bool DbTableLoyaltyPoints::NeedUpdate(int OldVersion, int UpdatedVersion) const
{
	bool bNeedUpdate = false;
	if (OldVersion <= 1 && UpdatedVersion > 1)
		bNeedUpdate = true; // we need to create the table it UpdatedVersion > 1
	return bNeedUpdate;
}

QStringList DbTableLoyaltyPoints::getUpdateStatement(int OldVersion, int TargetVersion) const
{
	QStringList updateStatements;
	if (OldVersion <= 1 && TargetVersion > 1)
	{
		updateStatements = getCreateStatements(TargetVersion);
	}
	return updateStatements;
}

QStringList DbTableLoyaltyPoints::getDeleteStatements() const
{
	// no delete necessary here, as LP table will be deleted automatically when a char is removed
	return QStringList();
}

/******************************************************************************
* DbLPHandler
********************************************************************************/
const QUuid DbLPHandler::m_gUid("5fed9716-9c12-4266-8926-eaa460c4f314");

void DbLPHandler::saveToDb(LPStorageQVariant value, QSqlDatabase & Db)
{
	CharLPData charLP = value.value<CharLPData>();
	if (charLP.eveCharId > 0)
	{
		// delete all older entries for this character
		deleteLPForChar(charLP, Db);
		for (auto LPEntry : charLP.LPData)
		{
			createCorpIfNeeded(Db, LPEntry.first);
			QString insertSql = QString::fromUtf8("INSERT into loyalty_points (eve_char_id, eve_corp_id,amount) VALUES(%1,%2,%3);")
				.arg(charLP.eveCharId).arg(LPEntry.first).arg(LPEntry.second);
			QSqlQuery insertQuery(Db);
			if (!insertQuery.exec(insertSql))
			{
				emit DbError("error saving LP to cache Db", PortableDBBackend::DbErrorCode::General);
			}
		}
	}
}

void DbLPHandler::deleteLPForChar(CharLPData &charLP, QSqlDatabase & Db)
{
	QString deleteSql = QString::fromUtf8("delete from loyalty_points where eve_char_id = %1;")
		.arg(charLP.eveCharId);
	QSqlQuery deleteQuery(Db);
	if (!deleteQuery.exec(deleteSql))
	{
		emit DbError("error deleting existing LP points for character", PortableDBBackend::DbErrorCode::General);
	}
}

void DbLPHandler::readAll(QSqlDatabase & Db)
{
	// we must enumerate all saved characters and then query their stored LP points
	CharList charList = getCharList(Db);
	for (auto eveCharId : charList)
	{
		CharacterLoyaltyPoints LPForChar;
		QString querySql = QString::fromUtf8("select eve_corp_id, amount from loyalty_points WHERE eve_char_id = %1")
			.arg(eveCharId);
		QSqlQuery queryLP(querySql, Db);
		while (queryLP.next())
		{
			LPForChar[queryLP.value(0).toLongLong()] = queryLP.value(1).toLongLong();
		}
		emit loyaltyPointsForChar(eveCharId, LPForChar);
	}
}

DbLPHandler::CharList DbLPHandler::getCharList(QSqlDatabase & Db)
{
	CharList charList;
	QString queryCharSql = QString::fromUtf8("select eve_char_id FROM char_auth_data;");
	QSqlQuery charQuery(queryCharSql, Db);
	while (charQuery.next())
	{
		charList.emplace_back(charQuery.value(0).toLongLong());
	}
	return charList;
}

void DbLPHandler::createCorpIfNeeded(QSqlDatabase & Db, CorpId corpId)
{
	QSqlQuery SelectQuery(QString::fromUtf8("Select * from corporation WHERE corp_id = %1").arg(corpId), Db);
	if (!SelectQuery.next())
	{
		// this corp isn't yet in the db, we need to insert
		QString insertSql = QString::fromUtf8("INSERT into corporation(corporation_id) VALUES(%1);")
			.arg(corpId);
		QSqlQuery insertQuery(Db);
		if (!insertQuery.exec(insertSql))
		{
			emit DbError("error inserting missing corporation", PortableDBBackend::DbErrorCode::General);
		}
	}
}
