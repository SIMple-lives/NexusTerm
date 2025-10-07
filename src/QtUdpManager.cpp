#include "QtUdpManager.h"
#include <QHostAddress>
#include <QVariant>

// 构造函数：将 UdpManager:: 修正为 QtUdpManager::
QtUdpManager::QtUdpManager(QObject *parent) : IUdpManager(parent) {
    m_udpSocket = new QUdpSocket(this);
    // 当socket接收到数据报时，会发出readyRead信号
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &QtUdpManager::handleReadyRead);
}

// 析构函数：将 UdpManager:: 修正为 QtUdpManager::
QtUdpManager::~QtUdpManager() {
    unbindPort();
}

// bindPort 函数：将 UdpManager:: 修正为 QtUdpManager::
bool QtUdpManager::bindPort(quint16 port) {
    
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, QVariant(2 * 1024 * 1024));

    if (m_udpSocket->bind(QHostAddress::Any, port)) {
        emit portBound();
        return true;
    }
    return false;
}

// unbindPort 函数：将 UdpManager:: 修正为 QtUdpManager::
void QtUdpManager::unbindPort() {
    if (isBound()) {
        m_udpSocket->close();
        emit portUnbound();
    }
}

// writeData 函数：将 UdpManager:: 修正为 QtUdpManager::
void QtUdpManager::writeData(const QByteArray &data, const QString &host, quint16 port) {
    if (isBound()) {
        m_udpSocket->writeDatagram(data, QHostAddress(host), port);
    }
}

// isBound 函数：将 UdpManager:: 修正为 QtUdpManager::
bool QtUdpManager::isBound() const {
    return m_udpSocket->state() == QAbstractSocket::BoundState;
}

// handleReadyRead 函数：将 UdpManager:: 修正为 QtUdpManager::
void QtUdpManager::handleReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));
        QHostAddress senderHost;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderHost, &senderPort);
        
        emit dataReceived(datagram, senderHost.toString(), senderPort);
    }
}