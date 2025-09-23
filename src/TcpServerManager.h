#ifndef TCPSERVERMANAGER_H
#define TCPSERVERMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>

class QTcpServer;
class QTcpSocket;

class TcpServerManager : public QObject {
    Q_OBJECT

public:
    explicit TcpServerManager(QObject *parent = nullptr);
    ~TcpServerManager();

    bool startListening(quint16 port);
    void stopListening();
    void writeData(const QByteArray &data, const QString &clientInfo);
    void disconnectClient(const QString &clientInfo);
    bool isListening() const;

signals:
    void clientConnected(const QString &clientInfo);
    void clientDisconnected(const QString &clientInfo);
    void dataReceived(const QByteArray &data, const QString &clientInfo);
    void serverMessage(const QString &message); // 用于向状态栏或日志发送信息

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

private:
    QTcpServer *m_server;
    // 使用 QMap 来方便地通过 clientInfo 字符串查找 socket
    QMap<QString, QTcpSocket*> m_clients; 
};

#endif // TCPSERVERMANAGER_H