#include "CharacterAuthModel.h"

CharacterAuthModel::CharacterAuthModel(QObject* pParent)
	: QObject(pParent)
{
	m_SortModel.setSourceModel(&m_Model);
	m_SortModel.setSortRole(Qt::DisplayRole);
}

QModelIndex CharacterAuthModel::getIdxForId(int64_t charId) const
{
	QModelIndex Idx;
	for (int row = 0; row < m_Model.rowCount(); row++)
	{
		QModelIndex curIdx = m_Model.index(row, 0);
		if (curIdx.isValid())
		{
			if (curIdx.data(Roles::charId).toLongLong() == charId)
			{
				Idx = curIdx;
				break;
			}
		}
	}
	return Idx;
}

int CharacterAuthModel::getRowForId(int64_t charId) const
{
	int FoundIndex = -1;
	for (int row = 0; row < m_Model.rowCount(); row++)
	{
		QModelIndex curIdx = m_Model.index(row, 0);
		if (curIdx.isValid())
		{
			if (curIdx.data(Roles::charId).toLongLong() == charId)
			{
				FoundIndex = row;
				break;
			}
		}
	}
	return FoundIndex;
}

Eve::EveCharacterAuthData CharacterAuthModel::getAuthDataFromId(int64_t eveCharacterId) const
{
	Eve::EveCharacterAuthData authData;
	// we could either use the match function or traverse the model ourselves
	// I opt for the latter
	int FoundIndex = getRowForId(eveCharacterId);
	if (FoundIndex >= 0)
	{
		QModelIndex Idx = m_Model.index(FoundIndex, 0);
		if (Idx.isValid())
		{
			authData = Idx.data(Roles::charAuthData).value<Eve::EveCharacterAuthData>();
		}
	}
	return authData; // will be empty if eveCharacterId wasn't found in the model
}

Eve::EveCharacterAuthData CharacterAuthModel::getAuthDataFromIndex(const QModelIndex& curIdx) const
{
	Eve::EveCharacterAuthData authData;
	if (curIdx.isValid())
	{
		authData = curIdx.data(Roles::charAuthData).value<Eve::EveCharacterAuthData>();
	}
	return authData;
}

void CharacterAuthModel::deleteChar(int64_t idToDelete)
{
	QModelIndex Idx = getIdxForId(idToDelete);
	if (Idx.isValid())
	{
		Eve::EveCharacterAuthData character = Idx.data(Roles::charAuthData).value<Eve::EveCharacterAuthData>();
		m_Model.removeRow(Idx.row());
		emit deleteCharacter(character);
	}
}

void CharacterAuthModel::insertOrUpdateCharacter(const Eve::EveCharacterAuthData& eveChar, bool bReportChanges)
{
	// we can either assume, that this is indeed a new character or we can check the model first
	// as I don't expect the characterlist to be more than 20, it won't much hurt to search ...
	int FoundIndex = getRowForId(eveChar.characterId());
	if (FoundIndex >= 0)
	{
		// we ought to update
		QModelIndex Idx = m_Model.index(FoundIndex, 0);
		m_Model.setData(Idx, eveChar.characterName());
		m_Model.setData(Idx, QVariant::fromValue(eveChar), Roles::charAuthData);
		if (bReportChanges)
		{
			emit updateCharacter(eveChar);
		}
	}
	else
	{
		// insert at the end
		QStandardItem* pNewItem = new QStandardItem(eveChar.characterName());
		if (pNewItem)
		{
			pNewItem->setData(eveChar.characterId(), Roles::charId);
			pNewItem->setData(QVariant::fromValue(eveChar), Roles::charAuthData);
			m_Model.appendRow(pNewItem);
			if (bReportChanges)
			{
				emit newCharacter(eveChar);
			}
		}
		// re sort
		m_SortModel.sort(0);
	}
}

void CharacterAuthModel::onCharacterAuthenticated(const Eve::EveCharacterAuthData& Character)
{
	bool bReportChangesOnInsert = true;
	insertOrUpdateCharacter(Character, bReportChangesOnInsert);
}

QString CharacterAuthModel::getNameForId(int64_t eveCharId)
{
	Eve::EveCharacterAuthData CharData = getAuthDataFromId(eveCharId);
	return CharData.characterName();
}

void CharacterAuthModel::onCharacterAuthenticationUpdated(const Eve::EveCharacterAuthData & UpdatedCharacter)
{
	bool bReportChangesOnInsert = true;
	insertOrUpdateCharacter(UpdatedCharacter, bReportChangesOnInsert);
}

void CharacterAuthModel::onCharacterReadFromCache(int64_t /*eveCharacterId*/, const Eve::EveCharacterAuthData& Character)
{
	bool bReportChangesOnInsert = false;
	insertOrUpdateCharacter(Character, bReportChangesOnInsert);
}
