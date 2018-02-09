#ifndef LOYALTYPOINTACCOUNTANT_H
#define LOYALTYPOINTACCOUNTANT_H

#include <QMainWindow>
#include <QSortFilterProxyModel>

#include <QSettings>
#include "models/CharacterAuthModel.h"
#include "models/LPModel.h"
#include "models/CorpModel.h"

#include "Eve/EveAuthentication.h"
#include "Eve/EsiApiConnector.h"

#include "PortableDbBackend/DbHandler.h"

namespace Ui {
class LoyaltyPointAccountant;
}

class LoyaltyPointAccountant : public QMainWindow
{
	Q_OBJECT

public:
	explicit LoyaltyPointAccountant(QWidget *parent = 0);
	~LoyaltyPointAccountant();

private slots:
	void on_pbAddCharacter_clicked();
	void on_pbDeleteChar_clicked();
	void on_pbRefreshAll_clicked();
	void onDbReady();

	void onNewCharacter(const Eve::EveCharacterAuthData& newChar);
	void onUpdateCharacter(const Eve::EveCharacterAuthData& updatedChar);
	void onDeleteCharacter(const Eve::EveCharacterAuthData& deleteChar);

	void onCorpNameForId(const QString& CorpName, CorpId corpId);

	void onDbReadAllFinishedForHandler(QUuid handlerUuid); //  indicates that the readAll function from this handler has reported all its data
	void onLoyaltyPointsForChar(int64_t eveCharId, const CharacterLoyaltyPoints& LoyaltyPoints);

	void onCorpListClicked(const QModelIndex &index);
	void onCharListClicked(const QModelIndex &index);
	void onValueEditChanged(const QString&) { updateTotalLP(); }

private:
	Ui::LoyaltyPointAccountant *ui;
	Eve::EveAuthentication m_Authenticator;
	PortableDBBackend::DbHandler m_DbAccess;
	Eve::EsiApiConnector m_ApiConnector;
	CharacterAuthModel m_CharModel;
	LPModel m_LPModel;
	CorpModel m_CorpModel;
	QSortFilterProxyModel m_CharLPProxyModel;
	QSortFilterProxyModel m_CorpLPProxyModel;
	QSettings m_SavedSettings;
	const int m_DbVersion = 2;

	void initializeDb();
	void refreshAllCharLPInfo();
	void refreshCharacterLPInfo(const Eve::EveCharacterAuthData & eveAuth);
	void initializeViewModels();
	void updateTotalLP();
};
#endif // LOYALTYPOINTACCOUNTANT_H
