#pragma once
#include <map>
#include <QObject>
#include <QString>
#include <Eve/EsiApiConnector.h>

#include <QStandardItemModel>

//! @brief CorpModel manages our known corporation names
//! as it not displayed directly there is no need to provide a QAbstractItemModel interface though
using CorpId = int64_t;

class CorpModel : public QObject
{
	Q_OBJECT
public:
	CorpModel(Eve::EsiApiConnector& EsiApi, QObject* pParent = nullptr);

	virtual ~CorpModel() = default;

	QAbstractItemModel* getModel() { return &m_SortProxy; }
	//! @brief asynchronous function: will respond with a signal once the corp name is aquired
	void requestCorpNameForId(CorpId corpId);

signals:
	void corpNameForId(const QString& CorpName, CorpId corpId);
	void corpNameFromApi(const QString& CorpName, CorpId corpId);

public slots:
	void onCorpNameForId(CorpId corpId, const QString& CorpName);
	void onCorpNameFromDb(CorpId corpId, const QString& CorpName);

private:
	std::map<CorpId, QString> m_CorpNames;
	Eve::EsiApiConnector& m_Api;
	QStandardItemModel m_ViewModel;
	QSortFilterProxyModel m_SortProxy;

	void insertValue(QString corpName, CorpId corpId);
	void updateViewModel(QString corpName, CorpId corpId);
};