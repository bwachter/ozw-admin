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
#include <QtWidgets>

#include "devicedb.hpp"
#include "devicequirks.h"
#include "ui_devicedb.h"

DeviceDB::DeviceDB(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DeviceDB),
    m_Ready(false),
    m_Path("config/")
{
    ui->setupUi(this);
    qDebug() << DeviceQuirks::GetInstance().isReady();
    this->ui->saveBtn->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    this->ui->saveBtn->button(QDialogButtonBox::Save)->setEnabled(false);
    deviceTree = new DeviceDBXMLReader(this);
    deviceDetails = new DeviceConfigXMLReader(ui->DeviceDetails, this);

    while (!this->LoadXML()) {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::Directory);
        if (dialog.exec()) {
            m_Path = dialog.directory().absolutePath();
        } else {
            break;
        }
    }
    deviceDetails->setPath(m_Path);
    connect(deviceTree, SIGNAL(setupManufacturerPage(QDomNode &)), deviceDetails, SLOT(setupManufacturerPage(QDomNode &)));
    connect(deviceTree, SIGNAL(setupProductPage(QDomNode &)), deviceDetails, SLOT(setupProductPage(QDomNode &)));
    connect(deviceDetails, SIGNAL(changed()), this, SLOT(formDataChanged()));
    connect(this->ui->saveBtn, SIGNAL(accepted()), this->deviceDetails, SLOT(saveData()));
    connect(this->ui->saveBtn, SIGNAL(rejected()), this->deviceDetails, SLOT(resetData()));
    connect(this->deviceDetails, SIGNAL(dataWasReset()), this, SLOT(dataWasReset()));
    connect(this->deviceDetails, SIGNAL(dataWasSaved()), this, SLOT(dataWasSaved()));
    this->ui->horizontalLayout->insertWidget(0, deviceTree);

}

bool DeviceDB::LoadXML() {
    QFile mfxml(m_Path+"/manufacturer_specific.xml");
    if (!mfxml.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("QXmlStream Bookmarks"),
                                 tr("Cannot read file %1:\n%2.")
                                 .arg(mfxml.fileName())
                                 .arg(mfxml.errorString()));
            this->setReady(false);
            return false;
    }
    if (deviceTree->read(&mfxml)) {
        statusBar()->showMessage(tr("File loaded"), 2000);
        this->setReady(true);
        return true;
    } else {
        qWarning() << "Could Not Load Config Files";
        this->setReady(false);
        return false;
    }
}

void DeviceDB::doProductPage(QTreeWidgetItem *item) {

}


DeviceDB::~DeviceDB()
{
    delete ui;
}


bool DeviceDB::isReady() const
{
    return this->m_Ready;
}
void DeviceDB::setReady(bool ready) {
    this->m_Ready = ready;
}

void DeviceDB::formDataChanged() {
    this->ui->saveBtn->button(QDialogButtonBox::Save)->setEnabled(true);
}

void DeviceDB::dataWasSaved() {
    this->ui->saveBtn->button(QDialogButtonBox::Save)->setEnabled(false);
    this->deviceTree->dump();
}

void DeviceDB::dataWasReset() {
    this->ui->saveBtn->button(QDialogButtonBox::Save)->setEnabled(false);
}
