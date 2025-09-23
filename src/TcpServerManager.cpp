#include "TcpServerManager.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

TcpServerManager::TcpServerManager(QObject *parent) : QObject(parent) {
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &TcpServerManager::onNewConnection);
}

TcpServerManager::~TcpServerManager() {
    stopListening();
}

bool TcpServerManager::startListening(quint16 port) {
    if (m_server->listen(QHostAddress::Any, port)) {
        emit serverMessage(QString("服务器开始监听端口: %1").arg(port));
        return true;
    } else {
        emit serverMessage("错误: 无法监听端口 " + QString::number(port));
        return false;
    }
}

void TcpServerManager::stopListening() {
    // 断开所有客户端连接
    for (QTcpSocket *client : m_clients.values()) {
        client->disconnectFromHost();
    }
    m_server->close();
    m_clients.clear();
    emit serverMessage("服务器已停止监听");
}

void TcpServerManager::writeData(const QByteArray &data, const QString &clientInfo) {
    if (m_clients.contains(clientInfo)) {
        m_clients[clientInfo]->write(data);
    }
}

void TcpServerManager::disconnectClient(const QString &clientInfo) {
    if (m_clients.contains(clientInfo)) {
        m_clients[clientInfo]->disconnectFromHost();
    }
}

bool TcpServerManager::isListening() const {
    return m_server->isListening();
}

void TcpServerManager::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *clientSocket = m_server->nextPendingConnection();
        if (clientSocket) {
            QString clientInfo = QString("%1:%2")
                                     .arg(clientSocket->peerAddress().toString())
                                     .arg(clientSocket->peerPort());
            m_clients.insert(clientInfo, clientSocket);
            
            connect(clientSocket, &QTcpSocket::disconnected, this, &TcpServerManager::onClientDisconnected);
            connect(clientSocket, &QTcpSocket::readyRead, this, &TcpServerManager::onReadyRead);
            
            emit clientConnected(clientInfo);
        }
    }
}

void TcpServerManager::onClientDisconnected() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket) return;

    QString clientInfo = m_clients.key(clientSocket);
    if (!clientInfo.isEmpty()) {
        m_clients.remove(clientInfo);
        emit clientDisconnected(clientInfo);
    }
    clientSocket->deleteLater();
}

void TcpServerManager::onReadyRead() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket) return;

    QString clientInfo = m_clients.key(clientSocket);
    if (!clientInfo.isEmpty()) {
        QByteArray data = clientSocket->readAll();
        emit dataReceived(data, clientInfo);
    }
}