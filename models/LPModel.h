#pragma once
#include <QObject>
#include <QStandardItemModel>
#include <map>
#include "CharacterAuthModel.h"
#include "CorpModel.h"
//! @ brief CharacterLoyaltyPoints contains a list of all LP for a single character
using CharacterLoyaltyPoints = std::map<CorpId, int64_t>; //!< corp_id, amount
Q_DECLARE_METATYPE(CharacterLoyaltyPoints);

//! we want the loyalty point model to:
//! (*) provide the QAbstractItemModel to the application 
//! In this model we already use charname and corporation name (which are different tables in Db schema)

//! model default layout:
//! | 0  eve_char_id| 1 eve name| 2 corp_id| 3 corporation name| 4 loyalty points
class LPModel : public QObject
{
public:
	LPModel(QObject* pParent = nullptr);
	virtual ~LPModel() = default;
	
	void setEveCharModel(CharacterAuthModel* pCharModel) { m_pCharModel = pCharModel; }
	void setCorpModel(CorpModel* pCorpModel); 
	QAbstractItemModel* getModel() { return &m_Model; }
	static int sortRole() { return Qt::UserRole + 1; }

public slots:
	void onLoyaltyPointsForChar(int64_t eveCharId, const CharacterLoyaltyPoints& LoyaltyPoints);
	void onCorpInfo( const QString& CorpName, int64_t CorpId);

private:
	QStandardItemModel m_Model;
	CharacterAuthModel* m_pCharModel = nullptr;
	CorpModel* m_pCorpModel = nullptr;
	std::map<int64_t, QString> m_CorpNames;

	void deleteRowsForChar(int64_t eveCharId);
	void createLPEntry(int64_t eveCharId, int64_t CorpId, int64_t amount);
};


