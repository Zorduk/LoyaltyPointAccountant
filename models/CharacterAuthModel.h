#pragma once
#include "Eve/EveAuthentication.h"
#include <QObject>
#include <QStandardItemModel>

//! @brief CharacterAuthModel provides the stored character models
class CharacterAuthModel : public QObject
{
	Q_OBJECT
public:
	enum Roles
	{
		charId = Qt::UserRole + 1 //!< int64_t
		, charAuthData //!< Eve::EveCharacterAuthData as QVariant (not Aes String encoded)
	};
	CharacterAuthModel(QObject* pParent = nullptr);
	virtual ~CharacterAuthModel() = default;

	// interface to application
	Eve::EveCharacterAuthData getAuthDataFromId(int64_t eveCharacterId) const;
	Eve::EveCharacterAuthData getAuthDataFromIndex(const QModelIndex& curIdx) const;

	void deleteChar(int64_t IdToDelete);

	// the returned model is only valid as long as this object lives
	QAbstractItemModel* getModel() { return &m_SortModel; }

	QString getNameForId(int64_t eveCharId);

public slots:
	void onCharacterAuthenticated(const Eve::EveCharacterAuthData& Character);
	void onCharacterAuthenticationUpdated(const Eve::EveCharacterAuthData& UpdatedCharacter);
	void onCharacterReadFromCache(int64_t eveCharacterId, const Eve::EveCharacterAuthData& AuthData);

signals:
	// in order to keep the CacheDb updated, this model sends signals on data modification
	void newCharacter(const Eve::EveCharacterAuthData& newChar);
	void updateCharacter(const Eve::EveCharacterAuthData& updatedChar);
	void deleteCharacter(const Eve::EveCharacterAuthData& deleteChar);

private:
	//! m_Model is intended to provide a list of charNames while containing Id and EveCharacterAuthData in separate roles
	QStandardItemModel m_Model;
	QSortFilterProxyModel m_SortModel; // used to sort the character list by name
	int getRowForId(int64_t charId) const;
	QModelIndex getIdxForId(int64_t charId) const;
	void insertOrUpdateCharacter(const Eve::EveCharacterAuthData& eveChar, bool bReportChanges);
};