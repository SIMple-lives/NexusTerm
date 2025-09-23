#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class SerialManager : public QObject {
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();

    // 接收所有配置参数
    void openPort(const QString &portName, qint32 baudRate, QSerialPort::DataBits dataBits,
                  QSerialPort::Parity parity, QSerialPort::StopBits stopBits);
    void closePort();
    void writeData(const QByteArray &data);
    bool isOpen() const;
    static QList<QSerialPortInfo> getAvailablePorts();

signals:
    void portOpened();
    void portClosed();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &errorText);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serialPort;
};

#endif // SERIALMANAGER_H