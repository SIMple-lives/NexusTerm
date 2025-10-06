#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QButtonGroup>
#include <QMessageBox>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QPixmap>
#include <QImageReader>
#include <QBuffer>
#include <QUrl>
#include <QVideoWidget>
#include <QSlider>
#include <QLabel>
#include <QListWidget>
#include <QDataStream>
#include <QDebug>

// ##########################################################################
// ##                  NO CHANGES IN THIS SECTION                          ##
// ##########################################################################
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mediaPlayer(nullptr)
    , m_tempMediaFile(nullptr)
    , m_rxBytes(0)
    , m_txBytes(0)
    , m_lastUdpSenderPort(0)
    , m_fileSendOffset(0)
    , m_isUdpStreaming(false)
    // ===== 修改/新增下面的初始化 =====
    , m_videoHeaderReceived(false) // 初始化为 false
    , m_videoStreamWidth(0)        // 初始化为 0
    , m_videoStreamHeight(0)       // 初始化为 0
    // ===== 结束修改 =====
    , m_framesToSkip(0)
    , m_processedFrameCount(0)
{
    ui->setupUi(this);
    m_serialManager = new SerialManager(this);
    m_tcpManager = new TcpManager(this);
    m_udpManager = new UdpManager(this);
    m_tcpServerManager = new TcpServerManager(this);

    m_mediaPlayer = new QMediaPlayer(this);
    m_mediaPlayer->setVideoOutput(ui->videoDisplayWidget);

    m_autoSendTimer = new QTimer(this);
    ui->portComboBox->setEditable(true);

    initUI();

    // --- 连接信号和槽 ---
    connect(m_serialManager, &SerialManager::dataReceived, this, &MainWindow::onSerialDataReceived);
    connect(m_serialManager, &SerialManager::portOpened, this, &MainWindow::onPortOpened);
    connect(m_serialManager, &SerialManager::portClosed, this, &MainWindow::onPortClosed);
    connect(m_serialManager, &SerialManager::errorOccurred, this, &MainWindow::onPortError);

    connect(m_tcpManager, &TcpManager::connected, this, &MainWindow::onTcpConnected);
    connect(m_tcpManager, &TcpManager::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(m_tcpManager, &TcpManager::dataReceived, this, &MainWindow::onTcpDataReceived);
    connect(m_tcpManager, &TcpManager::errorOccurred, this, &MainWindow::onTcpError);

    connect(m_udpManager, &UdpManager::portBound, this, &MainWindow::onUdpBound);
    connect(m_udpManager, &UdpManager::portUnbound, this, &MainWindow::onUdpUnbound);
    connect(m_udpManager, &UdpManager::dataReceived, this, &MainWindow::onUdpDataReceived);

    connect(m_tcpServerManager, &TcpServerManager::clientConnected, this, &MainWindow::onClientConnected);
    connect(m_tcpServerManager, &TcpServerManager::clientDisconnected, this, &MainWindow::onClientDisconnected);
    connect(m_tcpServerManager, &TcpServerManager::dataReceived, this, &MainWindow::onServerDataReceived);
    connect(m_tcpServerManager, &TcpServerManager::serverMessage, this, &MainWindow::onServerMessage);
    
    connect(m_autoSendTimer, &QTimer::timeout, this, &MainWindow::on_sendButton_clicked);

    m_portScanTimer = new QTimer(this);
    connect(m_portScanTimer, &QTimer::timeout, this, &MainWindow::updatePortList);
    m_portScanTimer->start(2000);

    m_udpReassemblyTimer = new QTimer(this);
    m_udpReassemblyTimer->setInterval(200);
    m_udpReassemblyTimer->setSingleShot(true);
    connect(m_udpReassemblyTimer, &QTimer::timeout, this, &MainWindow::onUdpReassemblyTimeout);

    m_tcpReassemblyTimer = new QTimer(this);
    m_tcpReassemblyTimer->setInterval(200);
    m_tcpReassemblyTimer->setSingleShot(true);
    connect(m_tcpReassemblyTimer, &QTimer::timeout, this, &MainWindow::onTcpReassemblyTimeout);
    
    m_fileSendTimer = new QTimer(this);
    connect(m_fileSendTimer, &QTimer::timeout, this, &MainWindow::sendFileChunk);

    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::updatePosition);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::updateDuration);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::updatePlaybackState);
    
    connect(ui->clientListWidget, &QListWidget::currentItemChanged, this, &MainWindow::updateControlsState);

    ui->displayStackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow() {
    if(m_tempMediaFile){
        m_tempMediaFile->remove();
        delete m_tempMediaFile;
    }
    delete ui;
}

void MainWindow::initUI() {
    ui->baudRateComboBox->addItems({"9600", "19200", "38400", "57600", "115200", "460800"});
    ui->parityComboBox->addItems({"None", "Even", "Odd"});
    ui->dataBitsComboBox->addItems({"8", "7", "6", "5"});
    ui->stopBitsComboBox->addItems({"1", "1.5", "2"});

    QButtonGroup *receiveModeGroup = new QButtonGroup(this);
    receiveModeGroup->addButton(ui->autoDisplayRadio);
    receiveModeGroup->addButton(ui->textDisplayRadio);
    receiveModeGroup->addButton(ui->imageDisplayRadio);
    receiveModeGroup->addButton(ui->videoDisplayRadio);

    QButtonGroup *displayGroup = new QButtonGroup(this);
    displayGroup->addButton(ui->asciiDisplayRadio);
    displayGroup->addButton(ui->hexDisplayRadio);
    displayGroup->addButton(ui->decimalDisplayRadio);

    QButtonGroup *sendGroup = new QButtonGroup(this);
    sendGroup->addButton(ui->asciiSendRadio);
    sendGroup->addButton(ui->hexSendRadio);

    m_statusLabel = new QLabel("就绪", this);
    m_rxBytesLabel = new QLabel("RX: 0", this);
    m_txBytesLabel = new QLabel("TX: 0", this);
    ui->statusbar->addWidget(m_statusLabel);
    ui->statusbar->addPermanentWidget(m_rxBytesLabel);
    ui->statusbar->addPermanentWidget(m_txBytesLabel);

    connect(ui->communicationModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_communicationModeComboBox_currentIndexChanged);

    updatePortList();
    updateControlsState();

    connect(displayGroup, &QButtonGroup::buttonClicked, this, &MainWindow::updateLogDisplay);
    
    // 初始化时清空分辨率标签
    ui->resolutionLabel->clear();
}

void MainWindow::handleIncomingData(const QByteArray &data) {
    // 首先，根据选择的模式来决定如何处理数据
    if (ui->autoDisplayRadio->isChecked()) {
        // --- 自动模式 ---
        m_mediaPlayer->stop();
        QPixmap pixmap;
        bool isImage = pixmap.loadFromData(data);
        if (isImage) {
            // 是图像 -> 按图像处理
            ui->imageDisplayLabel->setPixmap(pixmap.scaled(ui->imageDisplayLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            ui->displayStackedWidget->setCurrentIndex(1);
            m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::In, data, "Image Data"});
        } else {
            // 不是图像 -> 按文本处理
            ui->imageDisplayLabel->clear();
            ui->displayStackedWidget->setCurrentIndex(0);
            m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::In, data, ""});
        }
    } else if (ui->textDisplayRadio->isChecked()) {
        // --- 文本模式 ---
        m_mediaPlayer->stop();
        ui->imageDisplayLabel->clear();
        ui->displayStackedWidget->setCurrentIndex(0);
        m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::In, data, ""});
    } else if (ui->imageDisplayRadio->isChecked()) {
        // --- 图像模式 ---
        m_mediaPlayer->stop();
        QPixmap pixmap;
        pixmap.loadFromData(data);
        ui->imageDisplayLabel->setPixmap(pixmap.scaled(ui->imageDisplayLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->displayStackedWidget->setCurrentIndex(1);
        m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::In, data, "Image Data"});
    } else if (ui->videoDisplayRadio->isChecked()) {
        // --- 视频模式 ---
        m_mediaPlayer->stop();
        ui->imageDisplayLabel->clear();
        if (m_tempMediaFile) {
            m_tempMediaFile->remove();
            delete m_tempMediaFile;
            m_tempMediaFile = nullptr;
        }
        m_tempMediaFile = new QTemporaryFile(this);
        if (m_tempMediaFile->open()) {
            m_tempMediaFile->write(data);
            m_tempMediaFile->flush();
            QString tempFilePath = m_tempMediaFile->fileName();
            m_tempMediaFile->close();
            ui->displayStackedWidget->setCurrentIndex(0);
            m_mediaPlayer->setSource(QUrl::fromLocalFile(tempFilePath));
            m_mediaPlayer->play();
            m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::In, data, tempFilePath});
        } else {
            qDebug() << "Failed to create temporary file for video.";
            delete m_tempMediaFile;
            m_tempMediaFile = nullptr;
        }
    }
    
    // 任何模式处理完后，都更新日志显示
    updateLogDisplay();
}

void MainWindow::updatePortList() {
    if (ui->communicationModeComboBox->currentIndex() == 0) {
        QList<QString> availablePortNames;
        const auto portInfos = SerialManager::getAvailablePorts();
        for (const auto &info : portInfos) {
            availablePortNames.append(info.portName());
        }

        if (m_knownPorts == availablePortNames) {
            return;
        }
        m_knownPorts = availablePortNames;
        const QString currentText = ui->portComboBox->currentText();
        ui->portComboBox->clear();
        ui->portComboBox->addItems(availablePortNames);
        int index = ui->portComboBox->findText(currentText);
        if (index != -1) {
            ui->portComboBox->setCurrentIndex(index);
        } else {
            ui->portComboBox->setEditText(currentText);
        }
    }
}

void MainWindow::updateControlsState() {
    bool isConnected = false;
    int modeIndex = ui->communicationModeComboBox->currentIndex();

    switch (modeIndex) {
        case 0:
            isConnected = m_serialManager->isOpen();
            ui->connectButton->setText(isConnected ? "关闭" : "打开");
            break;
        case 1:
            isConnected = m_tcpManager->isConnected();
            ui->connectButton->setText(isConnected ? "断开" : "连接");
            break;
        case 2:
            isConnected = m_udpManager->isBound();
            ui->connectButton->setText(isConnected ? "解绑" : "绑定");
            break;
        case 3: // TCP 服务器模式
            isConnected = m_tcpServerManager->isListening();
            ui->connectButton->setText(isConnected ? "停止监听" : "开始监听");
            break;
    }

    ui->communicationModeComboBox->setEnabled(!isConnected);
    ui->settingsStackedWidget->setEnabled(!isConnected);

    if (modeIndex == 3) {
        bool clientSelected = ui->clientListWidget->currentItem() != nullptr;
        ui->sendButton->setEnabled(clientSelected);
        ui->sendTextAsFileButton->setEnabled(clientSelected);
        ui->sendBigFileButton->setEnabled(clientSelected);
        ui->cyclicSendCheckBox->setEnabled(clientSelected);
        ui->disconnectClientButton->setEnabled(clientSelected);
    } else {
        ui->sendButton->setEnabled(isConnected);
        ui->sendTextAsFileButton->setEnabled(isConnected);
        ui->sendBigFileButton->setEnabled(isConnected);
        ui->cyclicSendCheckBox->setEnabled(isConnected);
        ui->disconnectClientButton->setEnabled(false);
    }

    if (!isConnected && m_autoSendTimer->isActive()) {
        ui->cyclicSendCheckBox->setChecked(false);
    }
}

void MainWindow::updateByteCounters() {
    m_rxBytesLabel->setText(QString("RX: %1").arg(m_rxBytes));
    m_txBytesLabel->setText(QString("TX: %1").arg(m_txBytes));
}

void MainWindow::on_connectButton_clicked() {
    int modeIndex = ui->communicationModeComboBox->currentIndex();
    switch (modeIndex) {
        case 0:
            if (m_serialManager->isOpen()) {
                m_serialManager->closePort();
            } else {
                if (ui->portComboBox->currentText().isEmpty()) {
                    QMessageBox::warning(this, "警告", "没有可用的串口");
                    return;
                }
                QString portName = ui->portComboBox->currentText();
                qint32 baudRate = ui->baudRateComboBox->currentText().toInt();
                QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(ui->dataBitsComboBox->currentText().toInt());
                QSerialPort::Parity parity = (ui->parityComboBox->currentText() == "Even") ? QSerialPort::EvenParity : ((ui->parityComboBox->currentText() == "Odd") ? QSerialPort::OddParity : QSerialPort::NoParity);
                QSerialPort::StopBits stopBits = (ui->stopBitsComboBox->currentText() == "1.5") ? QSerialPort::OneAndHalfStop : ((ui->stopBitsComboBox->currentText() == "2") ? QSerialPort::TwoStop : QSerialPort::OneStop);
                m_serialManager->openPort(portName, baudRate, dataBits, parity, stopBits);
            }
            break;
        case 1:
            if (m_tcpManager->isConnected()) {
                m_tcpManager->disconnectFromServer();
            } else {
                m_tcpManager->connectToServer(ui->tcpHostLineEdit->text(), ui->tcpPortSpinBox->value());
            }
            break;
        case 2:
            if (m_udpManager->isBound()) {
                m_udpManager->unbindPort();
            } else {
                if (!m_udpManager->bindPort(ui->udpBindPortSpinBox->value())) {
                    QMessageBox::critical(this, "错误", "绑定UDP端口失败！");
                }
            }
            break;
        case 3: // TCP 服务器
            if (m_tcpServerManager->isListening()) {
                m_tcpServerManager->stopListening();
            } else {
                quint16 port = ui->tcpListenPortSpinBox->value();
                m_tcpServerManager->startListening(port);
            }
            break;
    }
}

void MainWindow::on_sendButton_clicked() {
    QByteArray dataToSend;
    QString text = ui->sendDataEdit->toPlainText();

    if (ui->hexSendRadio->isChecked()) {
        text.remove(QRegularExpression("[\\s\\r\\n]"));
        dataToSend = QByteArray::fromHex(text.toUtf8());
    } else {
        dataToSend = text.toUtf8();
    }

    if (dataToSend.isEmpty()) return;

    int modeIndex = ui->communicationModeComboBox->currentIndex();
    switch(modeIndex) {
        case 0: m_serialManager->writeData(dataToSend); break;
        case 1: m_tcpManager->writeData(dataToSend); break;
        case 2: m_udpManager->writeData(dataToSend, ui->udpTargetHostLineEdit->text(), ui->udpTargetPortSpinBox->value()); break;
        case 3: // TCP 服务器
            if (ui->clientListWidget->currentItem()) {
                QString clientInfo = ui->clientListWidget->currentItem()->text();
                m_tcpServerManager->writeData(dataToSend, clientInfo);
            } else {
                QMessageBox::warning(this, "警告", "请在列表中选择一个客户端进行发送。");
                return;
            }
            break;
    }

    m_txBytes += dataToSend.size();
    updateByteCounters();

    m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::Out, dataToSend, ""});
    updateLogDisplay();

    if (ui->cyclicSendCheckBox->isChecked() == false) {
        ui->sendDataEdit->clear();
    }
}


void MainWindow::on_clearReceiveButton_clicked() {
    m_logBuffer.clear();
    updateLogDisplay();
    m_rxBytes = 0;
    m_txBytes = 0;
    updateByteCounters();
}

void MainWindow::on_autoWrapCheckBox_toggled(bool checked) {
    auto mode = checked ? QTextEdit::WidgetWidth : QTextEdit::NoWrap;
    ui->receiveDataDisplayEdit->setLineWrapMode(mode);
    ui->sentDataDisplayEdit->setLineWrapMode(mode);
}

void MainWindow::on_cyclicSendCheckBox_toggled(bool checked) {
    bool canStart = false;
    int modeIndex = ui->communicationModeComboBox->currentIndex();
    if (modeIndex == 0) canStart = m_serialManager->isOpen();
    else if (modeIndex == 1) canStart = m_tcpManager->isConnected();
    else if (modeIndex == 2) canStart = m_udpManager->isBound();
    else if (modeIndex == 3) canStart = m_tcpServerManager->isListening() && ui->clientListWidget->currentItem();


    if (checked && canStart) {
        m_autoSendTimer->start(ui->sendIntervalSpinBox->value());
    } else {
        m_autoSendTimer->stop();
    }
}

void MainWindow::on_communicationModeComboBox_currentIndexChanged(int index) {
    ui->settingsStackedWidget->setCurrentIndex(index);
    if (index == 0) {
        m_portScanTimer->start(2000);
    } else {
        m_portScanTimer->stop();
    }
    // 如果从UDP模式切换走，则停止视频流
    if (index != 2 && m_isUdpStreaming) {
        m_isUdpStreaming = false;
        ui->playPauseButton->setText("播放");
        m_videoFrameBuffer.clear();
    }
    updateControlsState();
}

void MainWindow::on_sendTextAsFileButton_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "选择媒体文件", "", "All Files (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开媒体文件: " + file.errorString());
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    int modeIndex = ui->communicationModeComboBox->currentIndex();
    switch(modeIndex) {
        case 0: m_serialManager->writeData(fileData); break;
        case 1: m_tcpManager->writeData(fileData); break;
        case 2: m_udpManager->writeData(fileData, ui->udpTargetHostLineEdit->text(), ui->udpTargetPortSpinBox->value()); break;
        case 3: // TCP 服务器
            if (ui->clientListWidget->currentItem()) {
                QString clientInfo = ui->clientListWidget->currentItem()->text();
                m_tcpServerManager->writeData(fileData, clientInfo);
            } else {
                QMessageBox::warning(this, "警告", "请在列表中选择一个客户端进行发送。");
                return;
            }
            break;
    }

    m_txBytes += fileData.size();
    updateByteCounters();

    m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::Out, fileData, ""});
    updateLogDisplay();
}

void MainWindow::on_sendBigFileButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件", "", "All Files (*)");
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + file.errorString());
        return;
    }

    m_fileDataToSend = file.readAll();
    file.close();

    if (m_fileDataToSend.isEmpty()) {
        QMessageBox::information(this, "提示", "文件为空，无需发送。");
        return;
    }

    m_fileSendOffset = 0;
    int delay = ui->delaySpinBox->value();
    
    // 立即发送第一块数据
    sendFileChunk();
    
    if (delay > 0) {
        m_fileSendTimer->start(delay);
    } else {
        // 如果延时为0，循环发送直到完成
        while(m_fileSendOffset < m_fileDataToSend.size()) {
            sendFileChunk();
        }
    }
}

void MainWindow::sendFileChunk()
{
    if (m_fileSendOffset >= m_fileDataToSend.size()) {
        if(m_fileSendTimer->isActive()){
            m_fileSendTimer->stop();
        }
        return;
    }

    int chunkSize = ui->packetSizeSpinBox->value();
    QByteArray chunk = m_fileDataToSend.mid(m_fileSendOffset, chunkSize);

    m_udpManager->writeData(chunk, ui->udpTargetHostLineEdit->text(), ui->udpTargetPortSpinBox->value());

    m_txBytes += chunk.size();
    updateByteCounters();
    m_fileSendOffset += chunk.size();
}

void MainWindow::on_clearDisplayButton_clicked()
{
    m_mediaPlayer->stop();
    ui->imageDisplayLabel->clear();
    ui->resolutionLabel->clear(); // 同时清空分辨率
    ui->displayStackedWidget->setCurrentIndex(0);
    if (m_tempMediaFile) {
        m_tempMediaFile->remove();
        delete m_tempMediaFile;
        m_tempMediaFile = nullptr;
    }
}

void MainWindow::on_playPauseButton_clicked()
{
    // 检查是否为UDP模式且端口已绑定
    if (ui->communicationModeComboBox->currentIndex() == 2 && m_udpManager->isBound()) {
        if (!m_isUdpStreaming) {
            // --- 开始视频流 ---
            m_isUdpStreaming = true;
            m_videoFrameBuffer.clear(); // 清空旧的缓冲

            // 发送1字节的启动命令 0x01
            QByteArray startCommand;
            startCommand.append(0x01);
            m_udpManager->writeData(startCommand, ui->udpTargetHostLineEdit->text(), ui->udpTargetPortSpinBox->value());

            ui->playPauseButton->setText("停止");
            m_statusLabel->setText("UDP视频流接收中...");
        } else {
            // --- 停止视频流 ---
            m_isUdpStreaming = false;
            
            // ===== 新增逻辑：重置视频流状态 =====
            m_videoHeaderReceived = false; 
            // ===== 结束新增 =====
            
            ui->playPauseButton->setText("播放");
            m_statusLabel->setText(QString("UDP已绑定本地端口: %1").arg(ui->udpBindPortSpinBox->value()));
            ui->resolutionLabel->clear();
            
            // 清空视频缓冲区，丢弃所有已接收但未处理的数据
            m_videoFrameBuffer.clear();
            qDebug() << "[Video Control] Streaming stopped by user. Video buffer cleared.";
        }
    } else {
        // 对于非UDP模式或UDP未绑定的情况，执行原来的媒体播放逻辑
        if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            m_mediaPlayer->pause();
        } else {
            m_mediaPlayer->play();
        }
    }
}

void MainWindow::on_progressSlider_valueChanged(int value)
{
    if (ui->progressSlider->isSliderDown()) {
        m_mediaPlayer->setPosition(value);
    }
}

void MainWindow::updatePlaybackState(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        ui->playPauseButton->setText("暂停");
    } else {
        // 只有在非UDP流模式下才将按钮设置为“播放”
        if (!m_isUdpStreaming || ui->communicationModeComboBox->currentIndex() != 2) {
             ui->playPauseButton->setText("播放");
        }
    }
}

void MainWindow::updatePosition(qint64 position)
{
    if (!ui->progressSlider->isSliderDown()) {
        ui->progressSlider->setValue(position);
    }
}

void MainWindow::updateDuration(qint64 duration)
{
    ui->progressSlider->setRange(0, duration);
}

void MainWindow::updateLogDisplay() {
    ui->receiveDataDisplayEdit->clear();
    ui->sentDataDisplayEdit->clear();
    for (const auto &entry : m_logBuffer) {
        QString displayText;
        bool isImage = (entry.sourceInfo == "Image Data");
        if (isImage) {
             displayText = QString("[Image Data: %1 bytes]").arg(entry.rawData.size());
        } else if (!entry.sourceInfo.isEmpty() && entry.sourceInfo != "Image Data" && QFileInfo(entry.sourceInfo).exists()){
            displayText = QString("[Video Data: %1 bytes, path: %2]").arg(entry.rawData.size()).arg(entry.sourceInfo);
        }
        else if (ui->asciiDisplayRadio->isChecked()) {
            displayText = QString::fromLocal8Bit(entry.rawData);
        } else if (ui->hexDisplayRadio->isChecked()) {
            displayText = QString::fromLatin1(entry.rawData.toHex(' ').toUpper());
        } else { // Decimal
            QStringList decValues;
            for (quint8 byte : entry.rawData) { decValues.append(QString::number(byte)); }
            displayText = decValues.join(' ');
        }
        if (entry.direction == LogEntry::In) {
            const QString logStr = QString("[%1] RX %2<- %3")
                                     .arg(entry.timestamp.toString("HH:mm:ss.zzz"))
                                     .arg(entry.sourceInfo)
                                     .arg(displayText);
            ui->receiveDataDisplayEdit->append(logStr);
        } else { // Out
            const QString logStr = QString("[%1] TX -> %2")
                                     .arg(entry.timestamp.toString("HH:mm:ss.zzz"))
                                     .arg(displayText);
            ui->sentDataDisplayEdit->append(logStr);
        }
    }
    ui->receiveDataDisplayEdit->moveCursor(QTextCursor::End);
    ui->sentDataDisplayEdit->moveCursor(QTextCursor::End);
}

// === 通信管理器槽函数实现 ===
void MainWindow::onSerialDataReceived(const QByteArray &data) {
    handleIncomingData(data);
    m_rxBytes += data.size();
    updateByteCounters();
}
void MainWindow::onPortOpened() {
    updateControlsState();
    m_statusLabel->setText(QString("已连接 %1").arg(ui->portComboBox->currentText()));
}
void MainWindow::onPortClosed() {
    updateControlsState();
    m_statusLabel->setText("已断开");
}
void MainWindow::onPortError(const QString &errorText) {
    if (!errorText.isEmpty()) QMessageBox::critical(this, "串口错误", errorText);
    updateControlsState();
}
void MainWindow::onTcpConnected() {
    m_tcpBuffer.clear();
    updateControlsState();
    m_statusLabel->setText(QString("已连接到 %1:%2").arg(ui->tcpHostLineEdit->text()).arg(ui->tcpPortSpinBox->value()));
}
void MainWindow::onTcpDisconnected() {
    updateControlsState();
    m_statusLabel->setText("TCP 已断开");
    if (!m_tcpBuffer.isEmpty()) {
        m_tcpReassemblyTimer->stop();
        onTcpReassemblyTimeout();
    }
}
void MainWindow::onTcpDataReceived(const QByteArray &data) {
    m_rxBytes += data.size();
    updateByteCounters();
    m_tcpBuffer.append(data);
    m_tcpReassemblyTimer->start();
}
void MainWindow::onTcpError(const QString &errorText) {
    if (!errorText.isEmpty()) QMessageBox::critical(this, "TCP错误", errorText);
    updateControlsState();
}
void MainWindow::onUdpBound() {
    updateControlsState();
    m_statusLabel->setText(QString("UDP已绑定本地端口: %1").arg(ui->udpBindPortSpinBox->value()));
}
void MainWindow::onUdpUnbound() {
    if (m_isUdpStreaming) {
        m_isUdpStreaming = false;
        
        // ===== 新增逻辑：重置视频流状态 =====
        m_videoHeaderReceived = false;
        // ===== 结束新增 =====

        ui->playPauseButton->setText("播放");
        m_videoFrameBuffer.clear();
    }
    updateControlsState();
    m_statusLabel->setText("UDP 已解绑");
}

void MainWindow::onUdpDataReceived(const QByteArray &data, const QString &senderHost, quint16 senderPort) {
    if (!m_isUdpStreaming) {
        return; 
    }
    m_rxBytes += data.size();
    updateByteCounters();
    m_videoFrameBuffer.append(data);
    processVideoFrameBuffer();
}

// ##########################################################################
// ##                        FINAL FIXED FUNCTION BELOW                    ##
// ##########################################################################

void MainWindow::processVideoFrameBuffer() {
    static const QByteArray frameHeader("\xF0\x5A\xA5\x0F", 4);
    static const int metaDataSize = 8; // Header(4) + Width(2) + Height(2)

    // 循环处理，确保缓冲区内的所有完整帧都被处理
    while (true) {
        // --- 步骤 1: 寻找帧头并同步缓冲区 ---
        int startHeaderPos = m_videoFrameBuffer.indexOf(frameHeader);
        if (startHeaderPos == -1) {
            // 缓冲区内没有找到任何帧头，无法处理，等待更多数据
            return;
        }
        // 丢弃在第一个有效帧头前的所有垃圾数据
        m_videoFrameBuffer.remove(0, startHeaderPos);

        // --- 步骤 2: 检查元数据是否完整 ---
        if (m_videoFrameBuffer.size() < metaDataSize) {
            // 数据不足以读取宽度和高度，等待更多数据
            return;
        }

        // --- 步骤 3: 解析并验证元数据 ---
        const uchar* meta = reinterpret_cast<const uchar*>(m_videoFrameBuffer.constData() + 4);
        quint16 width  = (meta[0] << 8) | meta[1];
        quint16 height = (meta[2] << 8) | meta[3];

        // 对分辨率进行基础验证。如果数值无效，说明这是一个伪造的帧头
        if (width == 0 || height == 0 || width > 4096 || height > 4096) {
            qDebug() << "[Video Sync Error] 解析到无效分辨率 " << width << "x" << height << "，判定为伪帧头。";
            // 丢弃这4个字节的伪帧头，然后从循环顶部重新开始寻找下一个有效帧头
            m_videoFrameBuffer.remove(0, 4);
            continue;
        }

        // --- 步骤 4: 计算当前帧的预期总大小 ---
        int expectedPixelDataSize = width * height * 2; // RGB16 格式，每像素2字节
        int totalFrameSize = metaDataSize + expectedPixelDataSize;

        // --- 步骤 5: (可选的强验证) 寻找下一个帧头来验证当前帧的边界 ---
        int nextHeaderPos = m_videoFrameBuffer.indexOf(frameHeader, 1); // 从当前帧头的下一个字节开始搜索

        if (nextHeaderPos != -1 && nextHeaderPos < totalFrameSize) {
            // 如果在当前帧预期结束之前就找到了下一个帧头，
            // 那么说明当前帧被截断或已损坏。
            qDebug() << "[Video Sync] 检测到截断帧，丢弃 " << nextHeaderPos << " 字节。";
            // 丢弃这些损坏的数据，并从下一个有效帧头开始重新同步
            m_videoFrameBuffer.remove(0, nextHeaderPos);
            continue;
        }
        
        // --- 步骤 6: 检查缓冲区的数据是否足够构成一整个有效帧 ---
        if (m_videoFrameBuffer.size() < totalFrameSize) {
            // 数据还不够一整帧，等待接收更多数据
            return;
        }

        // --- 步骤 7: 所有检查通过，提取、解码并显示图像 ---
        QByteArray imageDataBytes = m_videoFrameBuffer.mid(metaDataSize, expectedPixelDataSize);

        // 为适应RGB16格式，进行字节序交换 (高低位交换)
        for(int i = 0; i < imageDataBytes.size(); i += 2) {
            std::swap(imageDataBytes[i], imageDataBytes[i+1]);
        }

        const uchar* correctedImageDataPtr = reinterpret_cast<const uchar*>(imageDataBytes.constData());
        QImage image(correctedImageDataPtr, width, height, QImage::Format_RGB16);

        if (!image.isNull()) {
            ui->imageDisplayLabel->setPixmap(QPixmap::fromImage(image).scaled(ui->imageDisplayLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            
            if (ui->displayStackedWidget->currentIndex() != 1) {
                 ui->displayStackedWidget->setCurrentIndex(1);
            }
            ui->resolutionLabel->setText(QString("%1 x %2").arg(width).arg(height));
        } else {
            qDebug() << "[Video ERROR] QImage无法从有效的帧数据加载图像。";
        }

        // --- 步骤 8: 从缓冲区移除已处理的帧 ---
        m_videoFrameBuffer.remove(0, totalFrameSize);
        // 继续循环，以处理缓冲区中可能存在的下一帧
    }
}


void MainWindow::onUdpReassemblyTimeout() {
    if (m_udpBuffer.isEmpty()) return;
    handleIncomingData(m_udpBuffer);
    m_udpBuffer.clear();
    m_lastUdpSenderHost.clear();
    m_lastUdpSenderPort = 0;
}

void MainWindow::onTcpReassemblyTimeout() {
    if (m_tcpBuffer.isEmpty()) return;
    handleIncomingData(m_tcpBuffer);
    m_tcpBuffer.clear();
}

// === TCP服务器槽函数实现 ===
void MainWindow::onClientConnected(const QString &clientInfo) {
    ui->clientListWidget->addItem(clientInfo);
    updateControlsState();
}

void MainWindow::onClientDisconnected(const QString &clientInfo) {
    for (int i = 0; i < ui->clientListWidget->count(); ++i) {
        if (ui->clientListWidget->item(i)->text() == clientInfo) {
            delete ui->clientListWidget->takeItem(i);
            break;
        }
    }
    updateControlsState();
}

void MainWindow::onServerDataReceived(const QByteArray &data, const QString &clientInfo) {
    m_rxBytes += data.size();
    updateByteCounters();
    
    m_logBuffer.append({QDateTime::currentDateTime(), LogEntry::In, data, clientInfo});
    updateLogDisplay();
}

void MainWindow::onServerMessage(const QString &message) {
    m_statusLabel->setText(message);
    updateControlsState();
}

void MainWindow::on_disconnectClientButton_clicked() {
    if (ui->clientListWidget->currentItem()) {
        QString clientInfo = ui->clientListWidget->currentItem()->text();
        m_tcpServerManager->disconnectClient(clientInfo);
    }
}