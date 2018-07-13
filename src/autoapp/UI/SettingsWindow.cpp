/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QMessageBox>
#include <f1x/openauto/autoapp/UI/SettingsWindow.hpp>
#include "ui_settingswindow.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <string>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

SettingsWindow::SettingsWindow(configuration::IConfiguration::Pointer configuration, QWidget *parent)
    : QWidget(parent)
    , ui_(new Ui::SettingsWindow)
    , configuration_(std::move(configuration))
{
    ui_->setupUi(this);
    connect(ui_->pushButtonCancel, &QPushButton::clicked, this, &SettingsWindow::close);
    connect(ui_->pushButtonSave, &QPushButton::clicked, this, &SettingsWindow::onSave);
    connect(ui_->horizontalSliderScreenDPI, &QSlider::valueChanged, this, &SettingsWindow::onUpdateScreenDPI);
    connect(ui_->radioButtonUseExternalBluetoothAdapter, &QRadioButton::clicked, [&](bool checked) { ui_->lineEditExternalBluetoothAdapterAddress->setEnabled(checked); });
    connect(ui_->radioButtonDisableBluetooth, &QRadioButton::clicked, [&]() { ui_->lineEditExternalBluetoothAdapterAddress->setEnabled(false); });
    connect(ui_->radioButtonUseLocalBluetoothAdapter, &QRadioButton::clicked, [&]() { ui_->lineEditExternalBluetoothAdapterAddress->setEnabled(false); });
    connect(ui_->pushButtonClearSelection, &QPushButton::clicked, std::bind(&SettingsWindow::setButtonCheckBoxes, this, false));
    connect(ui_->pushButtonSelectAll, &QPushButton::clicked, std::bind(&SettingsWindow::setButtonCheckBoxes, this, true));
    connect(ui_->pushButtonResetToDefaults, &QPushButton::clicked, this, &SettingsWindow::onResetToDefaults);
    connect(ui_->pushButtonShowBindings, &QPushButton::clicked, this, &SettingsWindow::onShowBindings);
    connect(ui_->horizontalSliderSystemVolume, &QSlider::valueChanged, this, &SettingsWindow::onUpdateSystemVolume);
    connect(ui_->horizontalSliderSystemCapture, &QSlider::valueChanged, this, &SettingsWindow::onUpdateSystemCapture);
}

SettingsWindow::~SettingsWindow()
{
    delete ui_;
}

void SettingsWindow::onSave()
{
    configuration_->setHandednessOfTrafficType(ui_->radioButtonLeftHandDrive->isChecked() ? configuration::HandednessOfTrafficType::LEFT_HAND_DRIVE : configuration::HandednessOfTrafficType::RIGHT_HAND_DRIVE);
    configuration_->showClock(ui_->checkBoxShowClock->isChecked());
    configuration_->setVideoFPS(ui_->radioButton30FPS->isChecked() ? aasdk::proto::enums::VideoFPS::_30 : aasdk::proto::enums::VideoFPS::_60);

    if(ui_->radioButton480p->isChecked())
    {
        configuration_->setVideoResolution(aasdk::proto::enums::VideoResolution::_480p);
    }
    else if(ui_->radioButton720p->isChecked())
    {
        configuration_->setVideoResolution(aasdk::proto::enums::VideoResolution::_720p);
    }
    else if(ui_->radioButton1080p->isChecked())
    {
        configuration_->setVideoResolution(aasdk::proto::enums::VideoResolution::_1080p);
    }

    configuration_->setScreenDPI(static_cast<size_t>(ui_->horizontalSliderScreenDPI->value()));
    configuration_->setOMXLayerIndex(ui_->spinBoxOmxLayerIndex->value());

    QRect videoMargins(0, 0, ui_->spinBoxVideoMarginWidth->value(), ui_->spinBoxVideoMarginHeight->value());
    configuration_->setVideoMargins(std::move(videoMargins));

    configuration_->setTouchscreenEnabled(ui_->checkBoxEnableTouchscreen->isChecked());
    this->saveButtonCheckBoxes();

    if(ui_->radioButtonDisableBluetooth->isChecked())
    {
        configuration_->setBluetoothAdapterType(configuration::BluetoothAdapterType::NONE);
    }
    else if(ui_->radioButtonUseLocalBluetoothAdapter->isChecked())
    {
        configuration_->setBluetoothAdapterType(configuration::BluetoothAdapterType::LOCAL);
    }
    else if(ui_->radioButtonUseExternalBluetoothAdapter->isChecked())
    {
        configuration_->setBluetoothAdapterType(configuration::BluetoothAdapterType::REMOTE);
    }

    configuration_->setBluetoothRemoteAdapterAddress(ui_->lineEditExternalBluetoothAdapterAddress->text().toStdString());

    configuration_->setMusicAudioChannelEnabled(ui_->checkBoxMusicAudioChannel->isChecked());
    configuration_->setSpeechAudioChannelEnabled(ui_->checkBoxSpeechAudioChannel->isChecked());
    configuration_->setAudioOutputBackendType(ui_->radioButtonRtAudio->isChecked() ? configuration::AudioOutputBackendType::RTAUDIO : configuration::AudioOutputBackendType::QT);

    configuration_->save();

    system((std::string("/usr/local/bin/autoapp_helper setvolumes ") + std::to_string(ui_->horizontalSliderSystemVolume->value()) + std::string(" ")  + std::to_string(ui_->horizontalSliderSystemCapture->value()) ).c_str());
    system((std::string("/usr/local/bin/autoapp_helper setdisconnect ") + std::to_string(ui_->spinBoxDisconnect->value())).c_str());
    system((std::string("/usr/local/bin/autoapp_helper setshutdown ") + std::to_string(ui_->spinBoxShutdown->value())).c_str());
    system((std::string("/usr/local/bin/autoapp_helper setdaynight ") + std::to_string(ui_->spinBoxDay->value()) + std::string(" ")  + std::to_string(ui_->spinBoxNight->value()) ).c_str());
    if (ui_->checkBoxGPIO->isChecked()) {
        system((std::string("/usr/local/bin/autoapp_helper setgpios ") + std::string("1 ") + std::string(ui_->comboBoxDevMode->currentText().toStdString() + " ") + std::string(ui_->comboBoxInvert->currentText().toStdString() + " ") + std::string(ui_->comboBoxX11->currentText().toStdString() + " ") + std::string(ui_->comboBoxRearcam->currentText().toStdString() + " ") + std::string(ui_->comboBoxAndroid->currentText().toStdString()) ).c_str());
    } else {
        system((std::string("/usr/local/bin/autoapp_helper setgpios ") + std::string("0 ") + std::string(ui_->comboBoxDevMode->currentText().toStdString() + " ") + std::string(ui_->comboBoxInvert->currentText().toStdString() + " ") + std::string(ui_->comboBoxX11->currentText().toStdString() + " ") + std::string(ui_->comboBoxRearcam->currentText().toStdString() + " ") + std::string(ui_->comboBoxAndroid->currentText().toStdString()) ).c_str());
    }

    if (ui_->radioButtonX11->isChecked()) {
        system((std::string("/usr/local/bin/autoapp_helper setmode 1")).c_str());
    } else {
        system((std::string("/usr/local/bin/autoapp_helper setmode 0")).c_str());
    }

    if (ui_->radioButtonScreenRotated->isChecked()) {
        system((std::string("/usr/local/bin/autoapp_helper setflip 1")).c_str());
    } else {
        system((std::string("/usr/local/bin/autoapp_helper setflip 0")).c_str());
    }

    this->close();
}

void SettingsWindow::onResetToDefaults()
{
    QMessageBox confirmationMessage(QMessageBox::Question, "Confirmation", "Are you sure you want to reset settings?", QMessageBox::Yes | QMessageBox::Cancel);
    confirmationMessage.setWindowFlags(Qt::WindowStaysOnTopHint);
    if(confirmationMessage.exec() == QMessageBox::Yes)
    {
        configuration_->reset();
        this->load();
    }
}

void SettingsWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    this->load();
}

void SettingsWindow::load()
{
    ui_->radioButtonLeftHandDrive->setChecked(configuration_->getHandednessOfTrafficType() == configuration::HandednessOfTrafficType::LEFT_HAND_DRIVE);
    ui_->radioButtonRightHandDrive->setChecked(configuration_->getHandednessOfTrafficType() == configuration::HandednessOfTrafficType::RIGHT_HAND_DRIVE);
    ui_->checkBoxShowClock->setChecked(configuration_->showClock());

    ui_->radioButton30FPS->setChecked(configuration_->getVideoFPS() == aasdk::proto::enums::VideoFPS::_30);
    ui_->radioButton60FPS->setChecked(configuration_->getVideoFPS() == aasdk::proto::enums::VideoFPS::_60);

    ui_->radioButton480p->setChecked(configuration_->getVideoResolution() == aasdk::proto::enums::VideoResolution::_480p);
    ui_->radioButton720p->setChecked(configuration_->getVideoResolution() == aasdk::proto::enums::VideoResolution::_720p);
    ui_->radioButton1080p->setChecked(configuration_->getVideoResolution() == aasdk::proto::enums::VideoResolution::_1080p);
    ui_->horizontalSliderScreenDPI->setValue(static_cast<int>(configuration_->getScreenDPI()));
    ui_->spinBoxOmxLayerIndex->setValue(configuration_->getOMXLayerIndex());

    const auto& videoMargins = configuration_->getVideoMargins();
    ui_->spinBoxVideoMarginWidth->setValue(videoMargins.width());
    ui_->spinBoxVideoMarginHeight->setValue(videoMargins.height());

    ui_->checkBoxEnableTouchscreen->setChecked(configuration_->getTouchscreenEnabled());
    this->loadButtonCheckBoxes();

    ui_->radioButtonDisableBluetooth->setChecked(configuration_->getBluetoothAdapterType() == configuration::BluetoothAdapterType::NONE);
    ui_->radioButtonUseLocalBluetoothAdapter->setChecked(configuration_->getBluetoothAdapterType() == configuration::BluetoothAdapterType::LOCAL);
    ui_->radioButtonUseExternalBluetoothAdapter->setChecked(configuration_->getBluetoothAdapterType() == configuration::BluetoothAdapterType::REMOTE);
    ui_->lineEditExternalBluetoothAdapterAddress->setEnabled(configuration_->getBluetoothAdapterType() == configuration::BluetoothAdapterType::REMOTE);
    ui_->lineEditExternalBluetoothAdapterAddress->setText(QString::fromStdString(configuration_->getBluetoothRemoteAdapterAddress()));

    ui_->checkBoxMusicAudioChannel->setChecked(configuration_->musicAudioChannelEnabled());
    ui_->checkBoxSpeechAudioChannel->setChecked(configuration_->speechAudioChannelEnabled());

    const auto& audioOutputBackendType = configuration_->getAudioOutputBackendType();
    ui_->radioButtonRtAudio->setChecked(audioOutputBackendType == configuration::AudioOutputBackendType::RTAUDIO);
    ui_->radioButtonQtAudio->setChecked(audioOutputBackendType == configuration::AudioOutputBackendType::QT);
}

void SettingsWindow::loadButtonCheckBoxes()
{
    const auto& buttonCodes = configuration_->getButtonCodes();
    ui_->checkBoxPlayButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::PLAY) != buttonCodes.end());
    ui_->checkBoxPauseButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::PAUSE) != buttonCodes.end());
    ui_->checkBoxTogglePlayButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::TOGGLE_PLAY) != buttonCodes.end());
    ui_->checkBoxNextTrackButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::NEXT) != buttonCodes.end());
    ui_->checkBoxPreviousTrackButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::PREV) != buttonCodes.end());
    ui_->checkBoxHomeButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::HOME) != buttonCodes.end());
    ui_->checkBoxPhoneButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::PHONE) != buttonCodes.end());
    ui_->checkBoxCallEndButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::CALL_END) != buttonCodes.end());
    ui_->checkBoxVoiceCommandButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::MICROPHONE_1) != buttonCodes.end());
    ui_->checkBoxLeftButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::LEFT) != buttonCodes.end());
    ui_->checkBoxRightButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::RIGHT) != buttonCodes.end());
    ui_->checkBoxUpButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::UP) != buttonCodes.end());
    ui_->checkBoxDownButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::DOWN) != buttonCodes.end());
    ui_->checkBoxScrollWheelButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::SCROLL_WHEEL) != buttonCodes.end());
    ui_->checkBoxBackButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::BACK) != buttonCodes.end());
    ui_->checkBoxEnterButton->setChecked(std::find(buttonCodes.begin(), buttonCodes.end(), aasdk::proto::enums::ButtonCode::ENTER) != buttonCodes.end());
}

void SettingsWindow::setButtonCheckBoxes(bool value)
{
    ui_->checkBoxPlayButton->setChecked(value);
    ui_->checkBoxPauseButton->setChecked(value);
    ui_->checkBoxTogglePlayButton->setChecked(value);
    ui_->checkBoxNextTrackButton->setChecked(value);
    ui_->checkBoxPreviousTrackButton->setChecked(value);
    ui_->checkBoxHomeButton->setChecked(value);
    ui_->checkBoxPhoneButton->setChecked(value);
    ui_->checkBoxCallEndButton->setChecked(value);
    ui_->checkBoxVoiceCommandButton->setChecked(value);
    ui_->checkBoxLeftButton->setChecked(value);
    ui_->checkBoxRightButton->setChecked(value);
    ui_->checkBoxUpButton->setChecked(value);
    ui_->checkBoxDownButton->setChecked(value);
    ui_->checkBoxScrollWheelButton->setChecked(value);
    ui_->checkBoxBackButton->setChecked(value);
    ui_->checkBoxEnterButton->setChecked(value);
}

void SettingsWindow::saveButtonCheckBoxes()
{
    configuration::IConfiguration::ButtonCodes buttonCodes;
    this->saveButtonCheckBox(ui_->checkBoxPlayButton, buttonCodes, aasdk::proto::enums::ButtonCode::PLAY);
    this->saveButtonCheckBox(ui_->checkBoxPauseButton, buttonCodes, aasdk::proto::enums::ButtonCode::PAUSE);
    this->saveButtonCheckBox(ui_->checkBoxTogglePlayButton, buttonCodes, aasdk::proto::enums::ButtonCode::TOGGLE_PLAY);
    this->saveButtonCheckBox(ui_->checkBoxNextTrackButton, buttonCodes, aasdk::proto::enums::ButtonCode::NEXT);
    this->saveButtonCheckBox(ui_->checkBoxPreviousTrackButton, buttonCodes, aasdk::proto::enums::ButtonCode::PREV);
    this->saveButtonCheckBox(ui_->checkBoxHomeButton, buttonCodes, aasdk::proto::enums::ButtonCode::HOME);
    this->saveButtonCheckBox(ui_->checkBoxPhoneButton, buttonCodes, aasdk::proto::enums::ButtonCode::PHONE);
    this->saveButtonCheckBox(ui_->checkBoxCallEndButton, buttonCodes, aasdk::proto::enums::ButtonCode::CALL_END);
    this->saveButtonCheckBox(ui_->checkBoxVoiceCommandButton, buttonCodes, aasdk::proto::enums::ButtonCode::MICROPHONE_1);
    this->saveButtonCheckBox(ui_->checkBoxLeftButton, buttonCodes, aasdk::proto::enums::ButtonCode::LEFT);
    this->saveButtonCheckBox(ui_->checkBoxRightButton, buttonCodes, aasdk::proto::enums::ButtonCode::RIGHT);
    this->saveButtonCheckBox(ui_->checkBoxUpButton, buttonCodes, aasdk::proto::enums::ButtonCode::UP);
    this->saveButtonCheckBox(ui_->checkBoxDownButton, buttonCodes, aasdk::proto::enums::ButtonCode::DOWN);
    this->saveButtonCheckBox(ui_->checkBoxScrollWheelButton, buttonCodes, aasdk::proto::enums::ButtonCode::SCROLL_WHEEL);
    this->saveButtonCheckBox(ui_->checkBoxBackButton, buttonCodes, aasdk::proto::enums::ButtonCode::BACK);
    this->saveButtonCheckBox(ui_->checkBoxEnterButton, buttonCodes, aasdk::proto::enums::ButtonCode::ENTER);
    configuration_->setButtonCodes(buttonCodes);
}

void SettingsWindow::saveButtonCheckBox(const QCheckBox* checkBox, configuration::IConfiguration::ButtonCodes& buttonCodes, aasdk::proto::enums::ButtonCode::Enum buttonCode)
{
    if(checkBox->isChecked())
    {
        buttonCodes.push_back(buttonCode);
    }
}

void SettingsWindow::onUpdateScreenDPI(int value)
{
    ui_->labelScreenDPIValue->setText(QString::number(value));
}

void SettingsWindow::onUpdateSystemVolume(int value)
{
    ui_->labelSystemVolumeValue->setText(QString::number(value));
}

void SettingsWindow::onUpdateSystemCapture(int value)
{
    ui_->labelSystemCaptureValue->setText(QString::number(value));
}

void SettingsWindow::loadSystemValues()
{
    // Get version string
    QFileInfo vFile("/etc/crankshaft.build");
    if (vFile.exists()) {
        QFile versionFile(QString("/etc/crankshaft.build"));
        versionFile.open(QIODevice::ReadOnly);
        QTextStream data_version(&versionFile);
        QString valueversion = data_version.readAll();
        versionFile.close();
        ui_->valueSystemVersion->setText(valueversion);
    } else {
        ui_->valueSystemVersion->setText("");
    }

    // Get date string
    QFileInfo dFile("/etc/crankshaft.date");
    if (dFile.exists()) {
        QFile dateFile(QString("/etc/crankshaft.date"));
        dateFile.open(QIODevice::ReadOnly);
        QTextStream data_date(&dateFile);
        QString valuedate = data_date.readAll();
        dateFile.close();
        ui_->valueSystemBuildDate->setText(valuedate);
    } else {
        ui_->valueSystemBuildDate->setText("");
    }

    system("/usr/local/bin/autoapp_helper getvolume");
    QFileInfo rFile("/tmp/return_value");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QString currentvol = data_return.readAll();
        returnFile.close();
        ui_->labelSystemVolumeValue->setText(currentvol);
        ui_->horizontalSliderSystemVolume->setValue(currentvol.toInt());
    }

    system("/usr/local/bin/autoapp_helper getcapvolume");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QString currentcapvol = data_return.readAll();
        returnFile.close();
        ui_->labelSystemCaptureValue->setText(currentcapvol);
        ui_->horizontalSliderSystemCapture->setValue(currentcapvol.toInt());
    }

    system("/usr/local/bin/autoapp_helper getfreemem");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QString currentmem = data_return.readAll();
        returnFile.close();
        ui_->valueSystemFreeMem->setText(currentmem);
    }

    system("/usr/local/bin/autoapp_helper getcpufreq");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QString currentfreq = data_return.readAll();
        returnFile.close();
        ui_->valueSystemCPUFreq->setText(currentfreq);
    }

    system("/usr/local/bin/autoapp_helper getcputemp");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QString cputemp = data_return.readAll();
        returnFile.close();
        ui_->valueSystemCPUTemp->setText(cputemp);
    }

    system("/usr/local/bin/autoapp_helper getshutdown");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QStringList shutdowntimer = data_return.readAll().split("-");;
        returnFile.close();
        ui_->spinBoxShutdown->setValue(shutdowntimer[0].toInt());
        ui_->valueShutdownTimer->setText(shutdowntimer[1]);
    }

    system("/usr/local/bin/autoapp_helper getdisconnect");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QStringList disconnecttimer = data_return.readAll().split("-");;
        returnFile.close();
        ui_->spinBoxDisconnect->setValue(disconnecttimer[0].toInt());
        ui_->valueDisconnectTimer->setText(disconnecttimer[1]);
    }

    system("/usr/local/bin/autoapp_helper getgpios");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QStringList gpiosetup = data_return.readAll().split(" ");
        returnFile.close();
        if (gpiosetup[0] == "1") {
            ui_->checkBoxGPIO->setChecked(true);
        } else {
            ui_->checkBoxGPIO->setChecked(false);
        }
        ui_->comboBoxDevMode->setCurrentText(gpiosetup[1]);
        ui_->comboBoxInvert->setCurrentText(gpiosetup[2]);
        ui_->comboBoxX11->setCurrentText(gpiosetup[3]);
        ui_->comboBoxRearcam->setCurrentText(gpiosetup[4]);
        ui_->comboBoxAndroid->setCurrentText(gpiosetup[5]);
    }

    system("/usr/local/bin/autoapp_helper getmodeflip");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QStringList modeflip = data_return.readAll().split(" ");
        returnFile.close();
        if (modeflip[0] == "0") {
            ui_->radioButtonEGL->setChecked(true);
        } else {
            ui_->radioButtonX11->setChecked(true);
        }
        if (modeflip[1] == "0") {
            ui_->radioButtonScreenNormal->setChecked(true);
        } else {
            ui_->radioButtonScreenRotated->setChecked(true);
        }

    }

    system("/usr/local/bin/autoapp_helper getdaynight");
    if (rFile.exists()) {
        QFile returnFile(QString("/tmp/return_value"));
        returnFile.open(QIODevice::ReadOnly);
        QTextStream data_return(&returnFile);
        QStringList daynight = data_return.readAll().split(" ");
        returnFile.close();
        ui_->spinBoxDay->setValue(daynight[0].toInt());
        ui_->spinBoxNight->setValue(daynight[1].toInt());
    }
}

void SettingsWindow::onShowBindings()
{
    const QString message = QString("Enter -> [Enter] \n")
                            + QString("Left -> [Left] \n")
                            + QString("Right -> [Right] \n")
                            + QString("Up -> [Up] \n")
                            + QString("Down -> [Down] \n")
                            + QString("Back -> [Esc] \n")
                            + QString("Home -> [H] \n")
                            + QString("Phone -> [P] \n")
                            + QString("Call end -> [O] \n")
                            + QString("Play -> [X] \n")
                            + QString("Pause -> [C] \n")
                            + QString("Previous track -> [V]/[Media Previous] \n")
                            + QString("Next track -> [N]/[Media Next] \n")
                            + QString("Toggle play -> [B]/[Media Play] \n")
                            + QString("Voice command -> [M] \n")
                            + QString("Wheel left -> [1] \n")
                            + QString("Wheel right -> [2]");

    QMessageBox confirmationMessage(QMessageBox::Information, "Information", message, QMessageBox::Ok);
    confirmationMessage.setWindowFlags(Qt::WindowStaysOnTopHint);
    confirmationMessage.exec();
}

}
}
}
}
