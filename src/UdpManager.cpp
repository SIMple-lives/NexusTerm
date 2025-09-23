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
    // ===================================================================================
    //
    //                              问题修复的关键代码
    //
    // ===================================================================================
    // 为防止因高速数据突发导致系统丢包，在此处设置一个更大的套接字接收缓冲区。
    // 默认的缓冲区大小可能不足以处理大文件的高速传输。2MB 是一个比较充足的设定。
    // 修复：将 int 显式转换为 QVariant
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