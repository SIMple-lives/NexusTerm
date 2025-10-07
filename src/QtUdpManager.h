// QtUdpManager.h (修改自 UdpManager.h)
#ifndef QTUDPMANAGER_H
#define QTUDPMANAGER_H

#include "IUdpManager.h" // 包含接口头文件
#include <QUdpSocket>

class QtUdpManager : public IUdpManager { // 继承自 IUdpManager
    Q_OBJECT

public:
    explicit QtUdpManager(QObject *parent = nullptr);
    ~QtUdpManager() override;

    // 使用 override 关键字明确表示这是对基类虚函数的实现
    bool bindPort(quint16 port) override;
    void unbindPort() override;
    void writeData(const QByteArray &data, const QString &host, quint16 port) override;
    bool isBound() const override;

private slots:
    void handleReadyRead();

private:
    QUdpSocket *m_udpSocket;
};

#endif // QTUDPMANAGER_H