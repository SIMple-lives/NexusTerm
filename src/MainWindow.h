#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QDateTime>
#include "SerialManager.h"
#include "TcpManager.h"
#include "UdpManager.h"
#include "TcpServerManager.h"

#include <QMediaPlayer>
#include <QTemporaryFile>

QT_BEGIN_NAMESPACE
class QVideoWidget;
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QTimer;

struct LogEntry {
    enum Direction { In, Out };

    QDateTime timestamp;
    Direction direction;
    QByteArray rawData;
    QString sourceInfo;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // UI 控件槽函数
    void on_connectButton_clicked();
    void on_sendButton_clicked();
    void on_clearReceiveButton_clicked();
    void on_autoWrapCheckBox_toggled(bool checked);
    void on_cyclicSendCheckBox_toggled(bool checked);
    void on_communicationModeComboBox_currentIndexChanged(int index);
    void on_sendTextAsFileButton_clicked();
    void on_sendBigFileButton_clicked();
    void on_clearDisplayButton_clicked();
    void on_playPauseButton_clicked();
    void on_progressSlider_valueChanged(int value);
    void on_disconnectClientButton_clicked();

    // 通信管理器槽函数
    void onSerialDataReceived(const QByteArray &data);
    void onPortOpened();
    void onPortClosed();
    void onPortError(const QString &errorText);
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpDataReceived(const QByteArray &data);
    void onTcpError(const QString &errorText);
    void onUdpBound();
    void onUdpUnbound();
    void onUdpDataReceived(const QByteArray &data, const QString &senderHost, quint16 senderPort);
    // TCP 服务器槽函数
    void onClientConnected(const QString &clientInfo);
    void onClientDisconnected(const QString &clientInfo);
    void onServerDataReceived(const QByteArray &data, const QString &clientInfo);
    void onServerMessage(const QString &message);
    
    // 其他槽函数
    void updateLogDisplay();
    void onUdpReassemblyTimeout();
    void onTcpReassemblyTimeout();
    void sendFileChunk();

    // 媒体播放器状态更新槽函数
    void updatePlaybackState(QMediaPlayer::PlaybackState state);
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);


private:
    // 私有辅助函数
    void initUI();
    void updatePortList();
    void updateControlsState();
    void updateByteCounters();
    void handleIncomingData(const QByteArray &data);
    void processVideoFrameBuffer();

private:
    Ui::MainWindow *ui;
    
    // 通信管理器
    SerialManager *m_serialManager;
    TcpManager *m_tcpManager;
    UdpManager *m_udpManager;
    TcpServerManager *m_tcpServerManager;
    
    // 媒体播放器
    QMediaPlayer *m_mediaPlayer;
    QTemporaryFile *m_tempMediaFile;

    // 定时器
    QTimer *m_autoSendTimer;
    QTimer *m_portScanTimer;
    QTimer *m_fileSendTimer;

    // 状态栏
    QLabel *m_statusLabel;
    QLabel *m_rxBytesLabel;
    QLabel *m_txBytesLabel;

    // 数据
    long long m_rxBytes;
    long long m_txBytes;
    QList<QString> m_knownPorts;
    QList<LogEntry> m_logBuffer;
    QByteArray m_tcpBuffer;
    QByteArray m_udpBuffer;
    QTimer* m_udpReassemblyTimer;
    QTimer* m_tcpReassemblyTimer;
    QString m_lastUdpSenderHost;
    quint16 m_lastUdpSenderPort;
    QByteArray m_fileDataToSend;
    qint64 m_fileSendOffset;

    // UDP视频流相关
    bool m_isUdpStreaming;
    QByteArray m_videoFrameBuffer;

    // 单帧测试相关
    int m_framesToSkip;         // 目标跳过的帧数
    int m_processedFrameCount;  // 当前已处理的帧数
};

#endif // MAINWINDOW_H