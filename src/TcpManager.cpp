#include "TcpManager.h"

TcpManager::TcpManager(QObject *parent) : QObject(parent) {
    m_tcpSocket = new QTcpSocket(this);
    connect(m_tcpSocket, &QTcpSocket::connected, this, &TcpManager::onConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &TcpManager::onDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &TcpManager::handleReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &TcpManager::handleSocketError);
}

TcpManager::~TcpManager() {
    disconnectFromServer();
}

void TcpManager::connectToServer(const QString &host, quint16 port) {
    if (m_tcpSocket->state() == QAbstractSocket::UnconnectedState) {
        m_tcpSocket->connectToHost(host, port);
    }
}

void TcpManager::disconnectFromServer() {
    if (m_tcpSocket->isOpen()) {
        m_tcpSocket->disconnectFromHost();
    }
}

void TcpManager::writeData(const QByteArray &data) {
    if (isConnected()) {
        m_tcpSocket->write(data);
    }
}

bool TcpManager::isConnected() const {
    return m_tcpSocket->state() == QAbstractSocket::ConnectedState;
}

void TcpManager::handleReadyRead() {
    emit dataReceived(m_tcpSocket->readAll());
}

void TcpManager::handleSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    emit errorOccurred(m_tcpSocket->errorString());
}

void TcpManager::onConnected() {
    emit connected();
}

void TcpManager::onDisconnected() {
    emit disconnected();
}