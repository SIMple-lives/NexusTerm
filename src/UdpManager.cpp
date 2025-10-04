#include "UdpManager.h"
#include <QHostAddress>
#include <QVariant> // Include QVariant header

UdpManager::UdpManager(QObject *parent) : QObject(parent) {
    m_udpSocket = new QUdpSocket(this);
    // 当socket接收到数据报时，会发出readyRead信号
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpManager::handleReadyRead);
}

UdpManager::~UdpManager() {
    unbindPort();
}

bool UdpManager::bindPort(quint16 port) {
    
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, QVariant(2 * 1024 * 1024));


    if (m_udpSocket->bind(QHostAddress::Any, port)) {
        emit portBound();
        return true;
    }
    return false;
}

void UdpManager::unbindPort() {
    if (isBound()) {
        m_udpSocket->close();
        emit portUnbound();
    }
}

void UdpManager::writeData(const QByteArray &data, const QString &host, quint16 port) {
    if (isBound()) {
        m_udpSocket->writeDatagram(data, QHostAddress(host), port);
    }
}

bool UdpManager::isBound() const {
    return m_udpSocket->state() == QAbstractSocket::BoundState;
}

void UdpManager::handleReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress senderHost;
        quint16 senderPort;

        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderHost, &senderPort);
        
        emit dataReceived(datagram, senderHost.toString(), senderPort);
    }
}