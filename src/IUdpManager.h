#ifndef IUDPMANAGER_H
#define IUDPMANAGER_H

#include <QObject>
#include <QByteArray>

// 抽象基类，定义UDP管理器的接口
class IUdpManager : public QObject {
    Q_OBJECT

public:
    explicit IUdpManager(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IUdpManager() = default; // 虚析构函数是必须的

    // 纯虚函数，定义接口
    virtual bool bindPort(quint16 port) = 0;
    virtual void unbindPort() = 0;
    virtual void writeData(const QByteArray &data, const QString &host, quint16 port) = 0;
    virtual bool isBound() const = 0;

signals:
    // 所有实现都必须提供这些信号
    void portBound();
    void portUnbound();
    void dataReceived(const QByteArray &data, const QString &senderHost, quint16 senderPort);
};

#endif // IUDPMANAGER_H