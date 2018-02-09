#include "CorpModel.h"
#include "Queries/EsiQueryCorpName.h"

CorpModel::CorpModel(Eve::EsiApiConnector& EsiApi, QObject* pParent)
	: QObject(pParent)
	, m_Api(EsiApi)
{
	m_SortProxy.setSourceModel(&m_ViewModel);
	m_SortProxy.setSortRole(Qt::DisplayRole);
}

void CorpModel::requestCorpNameForId(CorpId corpId)
{
	auto cIt = m_CorpNames.find(corpId);
	if (cIt != m_CorpNames.end())
	{
		emit corpNameForId(cIt->second, cIt->first);
	}
	else 
	{
		// start request
		EsiQueryCorpName* pQuery = new EsiQueryCorpName;
		if (pQuery)
		{
			pQuery->setCorpId(corpId);
			connect(pQuery, &EsiQueryCorpName::corpNameForId, this, &CorpModel::onCorpNameForId);
			m_Api.startPublicRequest(*pQuery);
		}
	}
}

void CorpModel::onCorpNameFromDb(CorpId corpId, const QString & CorpName)
{
	if (CorpName.isEmpty())
	{
		requestCorpNameForId(corpId);
	}
	else
	{
		insertValue(CorpName, corpId);
		emit corpNameForId(CorpName, corpId);
	}
}

void CorpModel::insertValue(QString corpName, CorpId corpId)
{
	// register in memory map
	m_CorpNames[corpId] = corpName;
	updateViewModel(corpName, corpId);
}

void CorpModel::updateViewModel(QString corpName, CorpId corpId)
{
	QStandardItem* pNewItem = new QStandardItem();
	if (pNewItem)
	{
		pNewItem->setData(corpName, Qt::DisplayRole);
		pNewItem->setData(corpId);
	}
	m_ViewModel.appendRow(pNewItem);
	m_SortProxy.sort(0, Qt::AscendingOrder);
}

void CorpModel::onCorpNameForId(CorpId corpId, const QString & CorpName)
{
	insertValue(CorpName, corpId);
	// signal new value
	emit corpNameForId(CorpName, corpId);
	emit corpNameFromApi(CorpName, corpId);
}