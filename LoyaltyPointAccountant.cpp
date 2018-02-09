#include "LoyaltyPointAccountant.h"
#include "ui_LoyaltyPointAccountant.h"
#include "Eve/EveAuthentication.h"
#include "LocalDb/DbCharAuthData.h"
#include "localDb/DbLoyaltyPoints.h"
#include "localDb/DbCorporation.h"
#include "models/LPModel.h"

#include "Queries/EsiQueryLP.h"
#include <QSharedPointer>

const QString ClientId = QStringLiteral("38c470b88bb54f9c9697f2c624b4fa2d");
const QString ClientSecret = QStringLiteral("MulNEOtSifV2T8wm9yYKjNpHzEPor7L0yZ38tAPg");
const int LocalPort = 8091;
const std::array<uint8_t, Eve::EveCharacterAuthData::AesKeySize> AesKey = {0xfe,0x23,0x02,0xc9,0x0c,0x98,0x08,0x81,0x01,0xcf,0xb0,0x10,0x09,0xee,0xb9,0x7b};

// registered scope in developer dashboard: "esi-characters.read_loyalty.v1"
const QStringList ConfiguredScopes = { QStringLiteral("characterLoyaltyPointsRead"), QStringLiteral("esi-characters.read_loyalty.v1") };

LoyaltyPointAccountant::LoyaltyPointAccountant(QWidget *parent) 
	: QMainWindow(parent)
	, ui(new Ui::LoyaltyPointAccountant)
	, m_Authenticator(Eve::EveAuthClientConfig(ClientId, ClientSecret, LocalPort))
	, m_CorpModel(m_ApiConnector)
	, m_SavedSettings("ZordSoft", "LoyaltyPointAccountant")
{
	qRegisterMetaType<QSharedPointer<PortableDBBackend::DbDataHandlerBase>>("QSharedPointer<PortableDBBackend::DbDataHandlerBase>");
	qRegisterMetaType<QSharedPointer<PortableDBBackend::DbDataHandlerBase>>("QSharedPointer<DbDataHandlerBase>");
	qRegisterMetaType<QSharedPointer<PortableDBBackend::DbDataHandlerBase>>("QSharedPointer<DbDataHandlerBase>");

	for (auto& SingleScope : ConfiguredScopes)
	{
		m_Authenticator.registerScope(SingleScope);
	}

	ui->setupUi(this);
	ui->charList->setModel(m_CharModel.getModel());
	ui->charList2->setModel(m_CharModel.getModel());

	connect(&m_Authenticator, &Eve::EveAuthentication::characterAuthenticated, &m_CharModel, &CharacterAuthModel::onCharacterAuthenticated);
	connect(&m_Authenticator, &Eve::EveAuthentication::characterAuthenticationUpdated, &m_CharModel, &CharacterAuthModel::onCharacterAuthenticationUpdated);
	connect(&m_CharModel, &CharacterAuthModel::newCharacter, this, &LoyaltyPointAccountant::onNewCharacter);
	connect(&m_CharModel, &CharacterAuthModel::updateCharacter, this, &LoyaltyPointAccountant::onUpdateCharacter);
	connect(&m_CharModel, &CharacterAuthModel::deleteCharacter, this, &LoyaltyPointAccountant::onDeleteCharacter);

	connect(&m_CorpModel, &CorpModel::corpNameFromApi, this, &LoyaltyPointAccountant::onCorpNameForId);

	m_LPModel.setEveCharModel(&m_CharModel);
	m_LPModel.setCorpModel(&m_CorpModel);

	initializeViewModels();

	initializeDb();
}

LoyaltyPointAccountant::~LoyaltyPointAccountant()
{
	// save values
	QLocale Locale;
	int64_t Division = Locale.toLongLong(ui->leDivision->text());
	m_SavedSettings.setValue("Divison", Division);
	double ISKPerLP = Locale.toDouble(ui->leISKPerLP->text());
	m_SavedSettings.setValue("ISKPerLP", ISKPerLP);
	delete ui;
}

void LoyaltyPointAccountant::initializeDb()
{
	// called from ctor
	connect(&m_DbAccess, &PortableDBBackend::DbHandler::DbReady, this, &LoyaltyPointAccountant::onDbReady);
	connect(&m_DbAccess, &PortableDBBackend::DbHandler::DbReadAllFinishedForHandler, this, &LoyaltyPointAccountant::onDbReadAllFinishedForHandler);

	m_DbAccess.addTableType<DbCharAuthTable>();
	m_DbAccess.addTableType<DbTableCorporation>();
	m_DbAccess.addTableType<DbTableLoyaltyPoints>();
	m_DbAccess.setDbVersion(m_DbVersion);

	CharAuthDbHandler* pHandler = new CharAuthDbHandler(AesKey);
	if (pHandler)
	{
		// now connect the char auth data to our Db Handler
		connect(pHandler, &CharAuthDbHandler::characterReadFromDb, &m_CharModel, &CharacterAuthModel::onCharacterReadFromCache);
		// move ownership to QSharedPointer
		QSharedPointer<PortableDBBackend::DbDataHandlerBase> spHandler(pHandler);
		m_DbAccess.registerHandler(spHandler);
	}
	CorporationDbHandler* pDbCorp = new CorporationDbHandler();
	if (pDbCorp)
	{
		connect(pDbCorp, &CorporationDbHandler::corpNameFromDb, &m_CorpModel, &CorpModel::onCorpNameFromDb);
		QSharedPointer<PortableDBBackend::DbDataHandlerBase> spHandler(pDbCorp);
		m_DbAccess.registerHandler(spHandler);
	}

	DbLPHandler* pDbLPHandler = new DbLPHandler;
	if (pDbLPHandler)
	{
		connect(pDbLPHandler, &DbLPHandler::loyaltyPointsForChar, &m_LPModel, &LPModel::onLoyaltyPointsForChar,Qt::QueuedConnection);
		QSharedPointer<PortableDBBackend::DbDataHandlerBase> spHandler(pDbLPHandler);
		m_DbAccess.registerHandler(spHandler);
	}
	m_DbAccess.InitializeDb("LPAccount.db");
}

void LoyaltyPointAccountant::refreshAllCharLPInfo()
{
	QAbstractItemModel* pCharModel = m_CharModel.getModel();
	for (int curRow = 0; curRow < pCharModel->rowCount(); ++curRow)
	{
		Eve::EveCharacterAuthData eveAuth = pCharModel->index(curRow, 0).data(CharacterAuthModel::Roles::charAuthData).value<Eve::EveCharacterAuthData>();
		refreshCharacterLPInfo(eveAuth);
	}
}

void LoyaltyPointAccountant::refreshCharacterLPInfo(const Eve::EveCharacterAuthData & eveAuth)
{
	EsiQueryLP* pEsiLPQuery = new EsiQueryLP;
	if (pEsiLPQuery)
	{
		connect(pEsiLPQuery, &EsiQueryLP::loyaltyPointsForChar, &m_LPModel, &LPModel::onLoyaltyPointsForChar);
		connect(pEsiLPQuery, &EsiQueryLP::loyaltyPointsForChar, this, &LoyaltyPointAccountant::onLoyaltyPointsForChar);
		m_ApiConnector.startRequest(*pEsiLPQuery, eveAuth, m_Authenticator);
	}
}

void LoyaltyPointAccountant::on_pbDeleteChar_clicked()
{
	int64_t charId = ui->charList2->currentIndex().data(CharacterAuthModel::Roles::charId).toLongLong();
	m_CharModel.deleteChar(charId);
}

void LoyaltyPointAccountant::on_pbRefreshAll_clicked()
{
	refreshAllCharLPInfo();
}

void LoyaltyPointAccountant::onDbReady()
{
	// read all character data
	m_DbAccess.readAllFromHandler(CharAuthDbHandler::HandlerUuid());
}

void LoyaltyPointAccountant::onNewCharacter(const Eve::EveCharacterAuthData & newChar)
{
	m_DbAccess.saveToDb(CharAuthDbHandler::HandlerUuid(),QVariant::fromValue(newChar));
	// refresh LP Info
	refreshCharacterLPInfo(newChar);
}

void LoyaltyPointAccountant::onUpdateCharacter(const Eve::EveCharacterAuthData & updatedChar)
{
	m_DbAccess.updateInDb(CharAuthDbHandler::HandlerUuid(), QVariant::fromValue(updatedChar));
}

void LoyaltyPointAccountant::onDeleteCharacter(const Eve::EveCharacterAuthData & deleteChar)
{
	m_DbAccess.deleteInDb(CharAuthDbHandler::HandlerUuid(), QVariant::fromValue(deleteChar));
	// remove all LP from model by providing empty LoyaltyPoint map to Model
	m_LPModel.onLoyaltyPointsForChar(deleteChar.characterId(), CharacterLoyaltyPoints());
}

void LoyaltyPointAccountant::onCorpNameForId(const QString & CorpName, CorpId corpId)
{
	CorporationData CorpData;
	CorpData.corpId = corpId;
	CorpData.CorpName = CorpName;
	m_DbAccess.saveToDb(CorporationDbHandler::HandlerUuid(), QVariant::fromValue<CorporationData>(CorpData));
}

void LoyaltyPointAccountant::onDbReadAllFinishedForHandler(QUuid handlerUuid)
{
	// we want to ensure the proper order for loading our cache Db
	// eveCharData -> corpNames -> LP data
	if (CharAuthDbHandler::HandlerUuid()==handlerUuid)
	{
		m_DbAccess.readAllFromHandler(CorporationDbHandler::HandlerUuid());
	}
	else if (CorporationDbHandler::HandlerUuid() == handlerUuid)
	{
		m_DbAccess.readAllFromHandler(DbLPHandler::HandlerUuid());
	}
	else if (DbLPHandler::HandlerUuid() == handlerUuid)
	{
		refreshAllCharLPInfo();
	}
}

void LoyaltyPointAccountant::onLoyaltyPointsForChar(int64_t eveCharId, const CharacterLoyaltyPoints & LoyaltyPoints)
{
	CharLPData lpData;
	lpData.eveCharId = eveCharId;
	lpData.LPData = LoyaltyPoints;

	m_DbAccess.saveToDb(DbLPHandler::HandlerUuid(), QVariant::fromValue<CharLPData>(lpData));
}

void LoyaltyPointAccountant::onCorpListClicked(const QModelIndex & index)
{
	// get corp name from index
	QString eveCorpName = index.data().toString();
	// set filtering in ProxyModel
	m_CorpLPProxyModel.setFilterRegExp(QRegExp(eveCorpName));
	ui->tvPerCorporation->resizeColumnsToContents();

	updateTotalLP();
}

void LoyaltyPointAccountant::onCharListClicked(const QModelIndex & index)
{
	// get char name from index
	QString eveCharName = index.data().toString();
	// set filtering in ProxyModel
	m_CharLPProxyModel.setFilterRegExp(QRegExp(eveCharName));
	ui->tvPerCharacter->resizeColumnsToContents();
}

void LoyaltyPointAccountant::on_pbAddCharacter_clicked()
{
	m_Authenticator.grant(Eve::EveCharacterAuthData());
}

void LoyaltyPointAccountant::initializeViewModels()
{
	m_CharLPProxyModel.setSourceModel(m_LPModel.getModel());
	m_CharLPProxyModel.setSortRole(LPModel::sortRole());

	m_CorpLPProxyModel.setSourceModel(m_LPModel.getModel());
	m_CorpLPProxyModel.setSortRole(LPModel::sortRole());

	ui->tvPerCharacter->setModel(&m_CharLPProxyModel);
	ui->tvPerCharacter->setColumnHidden(0, true);
	ui->tvPerCharacter->setColumnHidden(1, true);
	ui->tvPerCharacter->setColumnHidden(2, true);
	ui->tvPerCharacter->horizontalHeader()->setStretchLastSection(true);
	ui->tvPerCharacter->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->tvPerCharacter->setSortingEnabled(true);
	ui->tvPerCharacter->horizontalHeader()->setSortIndicator(4, Qt::SortOrder::DescendingOrder);
	ui->tvPerCharacter->verticalHeader()->hide();

	ui->tvPerCorporation->setModel(&m_CorpLPProxyModel);

	ui->tvPerCorporation->setColumnHidden(0, true);
	ui->tvPerCorporation->setColumnHidden(2, true);
	ui->tvPerCorporation->setColumnHidden(3, true);
	ui->tvPerCorporation->horizontalHeader()->setStretchLastSection(true);
	ui->tvPerCorporation->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->tvPerCorporation->horizontalHeader()->setStretchLastSection(true);
	ui->tvPerCorporation->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->tvPerCorporation->setSortingEnabled(true);
	ui->tvPerCorporation->horizontalHeader()->setSortIndicator(4, Qt::SortOrder::DescendingOrder);
	ui->tvPerCorporation->verticalHeader()->hide();

	ui->corpList->setModel(m_CorpModel.getModel());

	m_CorpLPProxyModel.setFilterRole(Qt::DisplayRole);
	m_CorpLPProxyModel.setFilterKeyColumn(3);
	m_CharLPProxyModel.setFilterRole(Qt::DisplayRole);
	m_CharLPProxyModel.setFilterKeyColumn(1);

	// display nothing until something is selected in the corresponding listview
	m_CorpLPProxyModel.setFilterRegExp("nothing");
	m_CharLPProxyModel.setFilterRegExp("nothing");

	connect(ui->charList, &QListView::clicked, this, &LoyaltyPointAccountant::onCharListClicked);
	connect(ui->corpList, &QListView::clicked, this, &LoyaltyPointAccountant::onCorpListClicked);
	connect(ui->leDivision, &QLineEdit::textChanged, this, &LoyaltyPointAccountant::onValueEditChanged);
	connect(ui->leISKPerLP, &QLineEdit::textChanged, this, &LoyaltyPointAccountant::onValueEditChanged);

	QLocale Locale;

	ui->leDivision->setText(Locale.toString(m_SavedSettings.value("Divison", 10000).toLongLong()));
	ui->leISKPerLP->setText(Locale.toString(m_SavedSettings.value("ISKPerLP", 1500.0).toDouble()));
	ui->leTotalLP->setText("0 LP");
	ui->leTotalValue->setText("0.00 ISK");
}

void LoyaltyPointAccountant::updateTotalLP()
{
	QLocale Locale;
	int64_t Division = std::max<int64_t>(1,Locale.toLongLong(ui->leDivision->text()));
	double ISKPerLP = Locale.toDouble(ui->leISKPerLP->text());

	int64_t LPSum = 0;
	double ISKSum = 0.0;
	// now we sum up all entries of our corp filtered proxy model
	for (int curRow = 0; curRow < m_CorpLPProxyModel.rowCount(); ++curRow)
	{
		int64_t Amount = m_CorpLPProxyModel.index(curRow, 4).data(LPModel::sortRole()).toLongLong();
		int64_t UsableAmount = Amount - (Amount % Division);
		double ValueOfUsableAmount = ISKPerLP * UsableAmount;
		LPSum += UsableAmount;
		ISKSum += ValueOfUsableAmount;
	}
	ui->leTotalValue->setText(Locale.toCurrencyString(ISKSum, "ISK"));
	ui->leTotalLP->setText(Locale.toString(LPSum)+" LP");
}
