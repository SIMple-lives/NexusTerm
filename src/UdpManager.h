#ifndef UDPMANAGER_H
#define UDPMANAGER_H

#include <QObject>
#include <QUdpSocket>

class UdpManager : public QObject {
    Q_OBJECT

public:
    explicit UdpManager(QObject *parent = nullptr);
    ~UdpManager();

    bool bindPort(quint16 port);
    void unbindPort();
    void writeData(const QByteArray &data, const QString &host, quint16 port);
    bool isBound() const;

signals:
    void portBound();
    void portUnbound();
    // 信号包含数据、发送方IP和端口
    void dataReceived(const QByteArray &data, const QString &senderHost, quint16 senderPort);

private slots:
    void handleReadyRead();

private:
    QUdpSocket *m_udpSocket;
};

#endif // UDPMANAGER_H