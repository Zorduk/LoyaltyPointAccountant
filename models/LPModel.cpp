#include "LPModel.h"

LPModel::LPModel(QObject* pParent) 
	: QObject(pParent)
{
	//! | 0  eve_char_id| 1 eve name| 2 corp_id| 3 corporation name| 4 loyalty points
	QStringList headerNames;
	headerNames << "Eve Character Id";
	headerNames << "Character";
	headerNames << "Eve corporation Id";
	headerNames << "Corporation";
	headerNames << "Loyalty points";
	m_Model.setHorizontalHeaderLabels(headerNames);
}

void LPModel::onLoyaltyPointsForChar(int64_t eveCharId, const CharacterLoyaltyPoints& LoyaltyPoints)
{
	// we want to expand our model with the corresponding lp s
	// and replace all entries with eveCharId

	// delete all entries for eveCharId
	deleteRowsForChar(eveCharId);

	// insert the new rows
	for (auto LPIt : LoyaltyPoints)
	{
		int64_t CorpId = LPIt.first;
		int64_t Amount = LPIt.second;
		// now create an element with this data
		createLPEntry(eveCharId, CorpId, Amount);
	}
}

void LPModel::setCorpModel(CorpModel * pCorpModel)
{
	m_pCorpModel = pCorpModel; 
	connect(pCorpModel, &CorpModel::corpNameForId, this, &LPModel::onCorpInfo);
}

void LPModel::onCorpInfo(const QString & CorpName, int64_t CorpId)
{
	for (int curRow = m_Model.rowCount(); curRow >= 0; --curRow)
	{
		QModelIndex Idx = m_Model.index(curRow, 2);
		if (Idx.data().toLongLong() == CorpId)
		{
			m_Model.setData(m_Model.index(curRow, 3), CorpName);
		}
	}
}

void LPModel::deleteRowsForChar(int64_t eveCharId)
{
	for (int curRow = m_Model.rowCount() - 1; curRow >= 0; --curRow)
	{
		QModelIndex eveCharIdx = m_Model.index(curRow, 0);
		if (eveCharId == eveCharIdx.data().toLongLong())
		{
			m_Model.removeRow(curRow);
		}
	}
}

void LPModel::createLPEntry(int64_t eveCharId, int64_t CorpId, int64_t amount)
{
	QList<QStandardItem*> newRow;
	QStandardItem* pItem = new QStandardItem;
	pItem->setData(QString::fromUtf8("%1").arg(eveCharId), Qt::DisplayRole);
	pItem->setData(eveCharId, sortRole());
	newRow << pItem;

	pItem = new QStandardItem;
	pItem->setData(m_pCharModel->getNameForId(eveCharId), Qt::DisplayRole);
	pItem->setData(m_pCharModel->getNameForId(eveCharId), sortRole());
	newRow << pItem;

	pItem = new QStandardItem;
	pItem->setData(CorpId, sortRole());
	pItem->setData(QString::fromUtf8("%1").arg(CorpId), Qt::DisplayRole);
	newRow << pItem;

	pItem = new QStandardItem;
	pItem->setData(QString::fromUtf8("Corp%1").arg(CorpId), Qt::DisplayRole);
	pItem->setData(QString::fromUtf8("Corp%1").arg(CorpId), sortRole());
	newRow << pItem;

	pItem = new QStandardItem;
	QLocale Locale;
	pItem->setData(Locale.toString(amount) + " LP", Qt::DisplayRole);
	pItem->setData(amount, sortRole());
	newRow << pItem;

	m_Model.appendRow(newRow);
	if (m_pCorpModel)
	{
		m_pCorpModel->requestCorpNameForId(CorpId);
	}
}
