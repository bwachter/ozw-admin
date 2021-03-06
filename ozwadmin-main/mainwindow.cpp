/*  ozw-admin - Gui for OpenZWave
 *    Copyright (C) 2016  Justin Hammond <justin@dynam.ac>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>
#include <QInputDialog>
#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include <qt-openzwave/qt-openzwavedatabase.h>

#include <qt-openzwave/qtozwoptions.h>
#include <qt-openzwave/qtozw_pods.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "splashdialog.h"
#include "metadatawindow.h"
#include "logwindow.h"
#include "devicedb.hpp"
#include "configuration.h"
#include "startup.h"
#include "startupprogress.h"
#include "util.h"
#include "deviceinfo.h"
#include "nodestatus.h"
#include "valuetable.h"
#include "nodeflagswidget.h"







MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	sbMsg(this)
{
	this->ui->setupUi(this);
	this->m_DockManager = new ads::CDockManager(this);


	DeviceInfo *di = new DeviceInfo(this);
	NodeStatus *ni = new NodeStatus(this);
	statusBar()->showMessage(tr("Starting..."));
	this->ui->action_Close->setEnabled(false);
	connect(ui->actionOpen_Log_Window, SIGNAL(triggered()), this, SLOT(openLogWindow()));

	connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(OpenConnection()));
	connect(ui->action_Close, SIGNAL(triggered()), this, SLOT(CloseConnection()));
	connect(ui->actionDevice_Database, SIGNAL(triggered()), this, SLOT(OpenDeviceDB()));
	connect(di, &DeviceInfo::openMetaDataWindow, this, &MainWindow::openMetaDataWindow);
	connect(ui->action_Configuration, SIGNAL(triggered()), this, SLOT(openConfigWindow()));

	this->ntw = new nodeTableWidget(this);
	connect(this->ntw, &nodeTableWidget::currentRowChanged, this, &MainWindow::NodeSelected);
	connect(this->ntw, &nodeTableWidget::currentRowChanged, di, &DeviceInfo::NodeSelected);
	connect(this->ntw, &nodeTableWidget::currentRowChanged, ni, &NodeStatus::NodeSelected);
	
	
	ads::CDockWidget* NodeListDW = new ads::CDockWidget("Node List");
	NodeListDW->setWidget(this->ntw);
	this->m_DockManager->addDockWidget(ads::LeftDockWidgetArea, NodeListDW);

	ads::CDockWidget* DeviceInfoDW = new ads::CDockWidget("Node Info");
	DeviceInfoDW->setWidget(di);
	auto RightDockWidget = this->m_DockManager->addDockWidget(ads::RightDockWidgetArea, DeviceInfoDW);
	ads::CDockWidget* DeviceStatusDW = new ads::CDockWidget("Node Status");
	DeviceStatusDW->setWidget(ni);
	this->m_DockManager->addDockWidget(ads::CenterDockWidgetArea, DeviceStatusDW, RightDockWidget);




	QStringList PossibleDBPaths;
	PossibleDBPaths << settings.value("openzwave/ConfigPath", QDir::toNativeSeparators("../../../config/")).toString().append("/");
	PossibleDBPaths << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

	QString path, dbPath, userPath;
	foreach(path, PossibleDBPaths) {
		qCDebug(ozwadmin) << "Checking " << QFileInfo(QDir::toNativeSeparators(path + "/config/manufacturer_specific.xml")).absoluteFilePath() << " for manufacturer_specific.xml";
		if (QFileInfo(QDir::toNativeSeparators(path + "/config/manufacturer_specific.xml")).exists()) {
			dbPath = QFileInfo(QDir::toNativeSeparators(path + "/config/manufacturer_specific.xml")).absoluteFilePath();
			break;
		}
		qCDebug(ozwadmin) << "Checking " << QFileInfo(QDir::toNativeSeparators(path + "../config/manufacturer_specific.xml")).absoluteFilePath() << " for manufacturer_specific.xml";
		if (QFile(QDir::toNativeSeparators(path + "/../config/manufacturer_specific.xml")).exists()) {
			dbPath = QFileInfo(QDir::toNativeSeparators(path + "/../config/manufacturer_specific.xml")).absoluteFilePath();
			break;
		}
	}
	PossibleDBPaths.clear();
	PossibleDBPaths << settings.value("openzwave/UserPath", QDir::toNativeSeparators("../../../config/")).toString().append("/");
	PossibleDBPaths << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

	foreach(path, PossibleDBPaths) {
		qCDebug(ozwadmin) << "Checking " << QFileInfo(QDir::toNativeSeparators(path + "/config/Options.xml")).absoluteFilePath() << " for Options.xml";
		if (QFileInfo(QDir::toNativeSeparators(path + "/config/Options.xml")).exists()) {
			userPath = QFileInfo(QDir::toNativeSeparators(path + "/config/Options.xml")).absoluteFilePath();
			break;
		}
		qCDebug(ozwadmin) << "Checking " << QFileInfo(QDir::toNativeSeparators(path + "/../config/Options.xml")).absoluteFilePath() << " for Options.xml";
		if (QFile(QDir::toNativeSeparators(path + "/../config/Options.xml")).exists()) {
			userPath = QFileInfo(QDir::toNativeSeparators(path + "/../config/Options.xml")).absoluteFilePath();
			break;
		}
	}

	qCDebug(ozwadmin) << "DBPath: " << dbPath;
	qCDebug(ozwadmin) << "userPath: " << userPath;

	if (dbPath.isEmpty()) {
		qCInfo(ozwadmin) << "Deploying OZW Database to " << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
		QStringList paths;
		paths << "." << "../../qt-openzwave/qt-openzwavedatabase/";
		if (!initConfigDatabase(paths)) {
			QMessageBox::critical(this, "Missing qt-openzwavedatabase.rcc Database File", "The qt-openzwavedatabase.rcc file could not be found");
			exit(-1);
		}
		QString dir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0);
		if (copyConfigDatabase(QDir(dir).absolutePath())) {
			qCInfo(ozwadmin) << "Copied Database to " << dir;
		}
		else {
			QMessageBox::critical(this, "Missing qt-openzwavedatabase.rcc Database File", "The qt-openzwavedatabase.rcc file could not be found");
			exit(-1);
		}
		dbPath = QFileInfo(dir.append("/config/")).absolutePath();
		m_configpath.setPath(dbPath);
		settings.setValue("openzwave/ConfigPath", m_configpath.absolutePath());
		qCInfo(ozwadmin) << "m_configPath set to " << m_configpath.absolutePath();
	}
	else
	{
		m_configpath.setPath(QFileInfo(dbPath).absolutePath());
		settings.setValue("openzwave/ConfigPath", m_configpath.absolutePath());
		qCInfo(ozwadmin) << "Found Existing DB Path" << m_configpath.absolutePath();
	}

	if (userPath.isEmpty()) {
		userPath = dbPath;
		m_userpath.setPath(QFileInfo(userPath).absolutePath());
		settings.setValue("openzwave/UserPath", m_userpath.absolutePath());
		qCInfo(ozwadmin) << "UserPath is Set to DBPath: " << m_userpath.absolutePath();
	}
	else {
		m_userpath.setPath(QFileInfo(userPath).absolutePath());
		qCInfo(ozwadmin) << "UserPath is Set from Settings" << m_userpath.absolutePath();
		settings.setValue("openzwave/UserPath", m_userpath.absolutePath());
	}

    this->m_openzwave = new QTOpenZwave(this, m_configpath, m_userpath);
    this->m_qtozwmanager = this->m_openzwave->GetManager();
    this->sbMsg.setQTOZWManager(this->m_qtozwmanager);
    QObject::connect(this->m_qtozwmanager, &QTOZWManager::ready, this, &MainWindow::QTOZW_Ready);

    this->m_qtozwmanager->initilizeSource(this->settings.value("StartServer").toBool());
    this->m_logWindow.setModel(this->m_qtozwmanager->getLogModel());
	di->setQTOZWManager(this->m_qtozwmanager);
	ni->setQTOZWManager(this->m_qtozwmanager);

	SplashDialog *sw = new SplashDialog(this->m_openzwave, this);
	sw->show();
	sw->move(this->geometry().center() - sw->rect().center());

	QTimer::singleShot(5000, sw, SLOT(close()));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::QTOZW_Ready() {
    qCDebug(ozwadmin) << "QTOZW Ready";

    /* apply our Local Configuration Options to the OZW Options Class */
    settings.beginGroup("openzwave");
    QStringList optionlist = settings.allKeys();
    for (int i = 0; i < optionlist.size(); i++) {
        qCDebug(ozwadmin) << "Updating Option " << optionlist.at(i) << " to " << settings.value(optionlist.at(i));
        QTOZWOptions *ozwoptions = this->m_qtozwmanager->getOptions();
        QStringList listtypes;
        listtypes << "SaveLogLevel" << "QueueLogLevel" << "DumpLogLevel";
        if (listtypes.contains(optionlist.at(i))) {
            OptionList list = ozwoptions->property(optionlist.at(i).toLocal8Bit()).value<OptionList>();
            if (list.getEnums().size() > 0)
                list.setSelected(settings.value(optionlist.at(i)).toString());
        }
        else
        {
            ozwoptions->setProperty(optionlist.at(i).toLocal8Bit(), settings.value(optionlist.at(i)));
        }
    }
    settings.endGroup();

	this->ntw->setModel(this->m_qtozwmanager->getNodeModel());
}

void MainWindow::OpenConnection() {

    this->ui->actionOpen->setEnabled(false);
    this->ui->action_Close->setEnabled(true);

	Startup su(this);
	su.setModal(true);
	int ret = su.exec();
	if (ret == QDialog::Accepted) {

		if (su.getremote() == true) {
			qCDebug(ozwadmin) << "Doing Remote Connection:" << su.getremoteHost() << su.getremotePort();
			QUrl server;
			server.setHost(su.getremoteHost());
			server.setPort(su.getremotePort());
			server.setScheme("ws");
			qCDebug(ozwadmin) << "Connecting to " << server;
			startupprogress *sup = new startupprogress(true, this);
            sup->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
			sup->setQTOZWManager(this->m_qtozwmanager);
			sup->show();
            this->m_qtozwmanager->setClientAuth(su.getauthKey());
			this->m_qtozwmanager->initilizeReplica(server);
            this->settings.setValue("connection/remotehost", su.getremoteHost());
            this->settings.setValue("connection/remoteport", su.getremotePort());
            this->settings.setValue("connection/authKey", su.getauthKey());
			return;
		}
		else 
		{
			qCDebug(ozwadmin) << "Doing Local Connection: " << su.getserialPort() << su.getstartServer();
			this->m_serialport = su.getserialPort();
			startupprogress *sup = new startupprogress(false, this);
			sup->setQTOZWManager(this->m_qtozwmanager);
			sup->show();
			this->m_qtozwmanager->open(this->m_serialport);
            this->settings.setValue("connection/serialport", su.getserialPort());
            this->settings.setValue("connection/startserver", su.getstartServer());
			return;
		}
    } else {
        qCDebug(ozwadmin) << "Open Dialog was Canceled" << ret;
        this->ui->actionOpen->setEnabled(true);
        this->ui->action_Close->setEnabled(false);

    }

}
void MainWindow::CloseConnection() {

}



void MainWindow::resizeColumns() {
    //this->ui->nodeList->resizeColumnsToContents();
}



void MainWindow::NodeSelected(QModelIndex current,QModelIndex previous) {
    Q_UNUSED(previous);
	Q_UNUSED(current);
#if 0
	if (!current.isValid()) {
        return;
    }
    const QAbstractItemModel * model = current.model();

    quint8 node = model->data(model->index(current.row(), QTOZW_Nodes::NodeColumns::NodeID)).value<quint8>();
#endif
}


void MainWindow::openLogWindow() {
    this->m_logWindow.show();
}

void MainWindow::openMetaDataWindow() {
    qCDebug(ozwadmin) << "Opening Window";
    QModelIndex index = this->ntw->currentIndex();
    const QAbstractItemModel *model = index.model();
    quint8 node = model->data(model->index(index.row(), QTOZW_Nodes::NodeColumns::NodeID)).value<quint8>();
    MetaDataWindow *mdwin = new MetaDataWindow(this);
    mdwin->populate(this->m_qtozwmanager, node);
    mdwin->setModal(true);
    mdwin->exec();
}

void MainWindow::OpenDeviceDB() {
    DeviceDB *ddb = new DeviceDB();
    ddb->show();
}

void MainWindow::openConfigWindow() {
    Configuration *cfg = new Configuration(this->m_qtozwmanager->getOptions(), this);
    cfg->show();
}
