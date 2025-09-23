#include "SerialManager.h"

SerialManager::SerialManager(QObject *parent) : QObject(parent) {
    m_serialPort = new QSerialPort(this);
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialManager::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialManager::handleError);
}

SerialManager::~SerialManager() {
    closePort();
}

void SerialManager::openPort(const QString &portName, qint32 baudRate, QSerialPort::DataBits dataBits,
                             QSerialPort::Parity parity, QSerialPort::StopBits stopBits) {
    if (isOpen()) {
        closePort();
    }
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        emit portOpened();
    } 
}

void SerialManager::closePort() {
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit portClosed();
    }
}

void SerialManager::writeData(const QByteArray &data) {
    if (isOpen() && m_serialPort->isWritable()) {
        m_serialPort->write(data);
    }
}

bool SerialManager::isOpen() const {
    return m_serialPort->isOpen();
}

QList<QSerialPortInfo> SerialManager::getAvailablePorts() {
    return QSerialPortInfo::availablePorts();
}

void SerialManager::handleReadyRead() {
    const QByteArray data = m_serialPort->readAll();
    emit dataReceived(data);
}

void SerialManager::handleError(QSerialPort::SerialPortError error) {
    if (error != QSerialPort::NoError) {
        if (error == QSerialPort::ResourceError) {
             closePort();
        }
        emit errorOccurred(m_serialPort->errorString());
    }
}