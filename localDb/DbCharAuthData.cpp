#include "DbCharAuthData.h"
#include "Eve/EveAuthentication.h"

#include <QSqlQuery>
const QUuid CharAuthDbHandler::m_gUid("9d9279a2-deca-4ef0-bcfe-9d4ad02c5e42");

QStringList DbCharAuthTable::getCreateStatements(int TargetVersion) const
{
	QStringList createStatements;
	if (TargetVersion > 0)
	{
		QString createCharacter;
		createCharacter += "CREATE TABLE char_auth_data (";
		createCharacter += " eve_char_id INTEGER PRIMARY KEY "; // 0
		createCharacter += " , auth_data TEXT NOT NULL"; // this will hold the AES encrypted private information
		createCharacter += ");";
		createStatements.append(createCharacter);
	}
	return createStatements;
}

bool DbCharAuthTable::NeedUpdate(int /*OldVersion*/, int /*UpdatedVersion*/)const
{
	bool bNeedUpdate = false;
	return bNeedUpdate;
}

QStringList DbCharAuthTable::getUpdateStatement(int /*OldVersion*/, int /*TargetVersion*/)const
{
	return QStringList();
}

QStringList DbCharAuthTable::getDeleteStatements() const
{
	QStringList delStatements;
	QString delSql;
	delSql += "delete from char_auth_data;";
	delStatements.append(delStatements);
	return delStatements;
}

/********************************************************
* CharAuthDbHandler
********************************************************/

void CharAuthDbHandler::saveToDb(QVariant value, QSqlDatabase& Db)
{
	if (value.isValid())
	{
		Eve::EveCharacterAuthData AuthData = value.value<Eve::EveCharacterAuthData>();
		int64_t eveCharId = AuthData.characterId();
		QString sAuthData = AuthData.SaveToString(m_AesKey);
		// construct insert Sql
		QString insertSql = QString::fromLatin1("INSERT INTO char_auth_data (eve_char_id, auth_data) VALUES ( %1, '%2');")
			.arg(eveCharId).arg(sAuthData);
		QSqlQuery insertQuery(Db);
		if (insertQuery.exec(insertSql))
		{
			// done
		}
		else
		{
			emit DbError("error inserting character into char_auth_data", PortableDBBackend::DbErrorCode::General);
		}
	}
}

void CharAuthDbHandler::updateInDb(QVariant value, QSqlDatabase& Db)
{
	if (value.isValid())
	{
		Eve::EveCharacterAuthData charAuth = value.value<Eve::EveCharacterAuthData>();
		int64_t eveCharId = charAuth.characterId();
		QString sAuthData = charAuth.SaveToString(m_AesKey);
		QString updateSql = QString::fromUtf8("UPDATE char_auth_data SET auth_data = '%1' WHERE eve_char_id = %2;")
			.arg(sAuthData).arg(eveCharId);
		QSqlQuery updateQuery(Db);
		if (updateQuery.exec(updateSql))
		{
			// done
		}
		else
		{
			emit DbError("error updating character into char_auth_data", PortableDBBackend::DbErrorCode::General);
		}
	}
}

void CharAuthDbHandler::deleteInDb(QVariant value, QSqlDatabase& Db)
{
	if (value.isValid())
	{
		Eve::EveCharacterAuthData charAuth = value.value<Eve::EveCharacterAuthData>();
		int64_t eveCharId = charAuth.characterId();
		QString deleteSql = QString::fromUtf8("DELETE FROM char_auth_data WHERE eve_char_id = %1")
			.arg(eveCharId);
		QSqlQuery deleteQuery(Db);
		if (deleteQuery.exec(deleteSql))
		{
			// done
		}
		else
		{
			emit DbError("error deleting character from char_auth_data", PortableDBBackend::DbErrorCode::General);
		}
	}
}

void CharAuthDbHandler::readAll(QSqlDatabase& Db)
{
	QString selectSql = QString::fromUtf8("Select eve_char_id, auth_data FROM char_auth_data;");
	QSqlQuery selectQuery(selectSql, Db);
	while (selectQuery.next())
	{
		Eve::EveCharacterAuthData charAuth;
		if (charAuth.ReadFromString(selectQuery.value(1).toString(), m_AesKey))
		{
			emit characterReadFromDb(selectQuery.value(0).toLongLong(), charAuth);
		}
	}
}


