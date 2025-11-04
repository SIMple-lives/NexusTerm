#ifndef WINSOCKUDPMANAGER_H
#define WINSOCKUDPMANAGER_H

// 必须先包含一个Qt头文件，Q_OS_WIN 宏才会被定义。
// IUdpManager.h 包含了 QObject，QObject 内部会包含 QtGlobal。
#include "IUdpManager.h"

#ifdef Q_OS_WIN // <-- 现在这个检查可以正常工作了

#include <QThread>
#include <winsock2.h> // 包含WinSock头文件

// 创建一个工作类来处理阻塞的接收操作
class UdpReceiverWorker : public QObject
{
    Q_OBJECT
public:
    UdpReceiverWorker(SOCKET socket, QObject* parent = nullptr);
    ~UdpReceiverWorker();
public slots:
    void startReceiving();
    void stopReceiving();
signals:
    void dataReady(const QByteArray &data, const QString &senderHost, quint16 senderPort);
private:
    SOCKET m_socket;
    volatile bool m_stop;
};


class WinSockUdpManager : public IUdpManager {
    Q_OBJECT
public:
    explicit WinSockUdpManager(QObject *parent = nullptr);
    ~WinSockUdpManager() override;

    bool bindPort(quint16 port) override;
    void unbindPort() override;
    void writeData(const QByteArray &data, const QString &host, quint16 port) override;
    bool isBound() const override;

private:
    bool m_isBound;
    SOCKET m_socket;
    QThread* m_receiverThread;
    UdpReceiverWorker* m_worker;

    bool initWinSock();
    void cleanupWinSock();
};

#endif // Q_OS_WIN
#endif // WINSOCKUDPMANAGER_HS