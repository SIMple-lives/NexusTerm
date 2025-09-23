#ifndef TCPMANAGER_H
#define TCPMANAGER_H

#include <QObject>
#include <QTcpSocket>

class TcpManager : public QObject {
    Q_OBJECT

public:
    explicit TcpManager(QObject *parent = nullptr);
    ~TcpManager();

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    void writeData(const QByteArray &data);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &errorText);

private slots:
    void handleReadyRead();
    void handleSocketError(QAbstractSocket::SocketError socketError);
    void onConnected();
    void onDisconnected();

private:
    QTcpSocket *m_tcpSocket;
};

#endif // TCPMANAGER_H