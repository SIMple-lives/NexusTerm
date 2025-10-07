// WinSockUdpManager.cpp
#ifdef Q_OS_WIN

#include "WinSockUdpManager.h"
#include <ws2tcpip.h> // For inet_ntop, etc.
#include <QDebug>

// ===================================================================
//  UdpReceiverWorker Implementation
// ===================================================================
UdpReceiverWorker::UdpReceiverWorker(SOCKET socket, QObject* parent)
    : QObject(parent), m_socket(socket), m_stop(false) {}

UdpReceiverWorker::~UdpReceiverWorker() {}

void UdpReceiverWorker::stopReceiving() {
    m_stop = true;
}

void UdpReceiverWorker::startReceiving() {
    char buffer[65535]; // Max UDP packet size
    sockaddr_in senderAddr;
    int senderAddrSize = sizeof(senderAddr);

    // 设置超时，以便我们可以定期检查 m_stop 标志
    struct timeval tv;
    tv.tv_sec = 1; // 1秒超时
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


    while (!m_stop) {
        int bytesReceived = recvfrom(m_socket, buffer, sizeof(buffer), 0, (sockaddr*)&senderAddr, &senderAddrSize);

        if (bytesReceived > 0) {
            QByteArray datagram(buffer, bytesReceived);
            
            char senderIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &senderAddr.sin_addr, senderIp, INET_ADDRSTRLEN);

            quint16 senderPort = ntohs(senderAddr.sin_port);

            emit dataReady(datagram, QString(senderIp), senderPort);
        } else {
            // Check for errors other than timeout
            if (WSAGetLastError() != WSAETIMEDOUT) {
                qWarning() << "recvfrom failed with error:" << WSAGetLastError();
                break;
            }
        }
    }
}


// ===================================================================
//  WinSockUdpManager Implementation
// ===================================================================
WinSockUdpManager::WinSockUdpManager(QObject *parent)
    : IUdpManager(parent), m_isBound(false), m_socket(INVALID_SOCKET), m_receiverThread(nullptr), m_worker(nullptr) {
    initWinSock();
}

WinSockUdpManager::~WinSockUdpManager() {
    unbindPort();
    cleanupWinSock();
}

bool WinSockUdpManager::initWinSock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void WinSockUdpManager::cleanupWinSock() {
    WSACleanup();
}

bool WinSockUdpManager::bindPort(quint16 port) {
    if (isBound()) return true;

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        qWarning() << "Failed to create WinSock socket:" << WSAGetLastError();
        return false;
    }

    // 设置接收缓冲区大小
    int bufferSize = 2 * 1024 * 1024; // 2MB
    setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(bufferSize));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        qWarning() << "Failed to bind WinSock socket:" << WSAGetLastError();
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_isBound = true;
    
    // 创建并启动接收线程
    m_receiverThread = new QThread(this);
    m_worker = new UdpReceiverWorker(m_socket);
    m_worker->moveToThread(m_receiverThread);

    connect(m_receiverThread, &QThread::started, m_worker, &UdpReceiverWorker::startReceiving);
    connect(m_worker, &UdpReceiverWorker::dataReady, this, &IUdpManager::dataReceived); // 将worker的信号直接连接到基类的信号
    
    m_receiverThread->start();

    emit portBound();
    return true;
}

void WinSockUdpManager::unbindPort() {
    if (!isBound()) return;
    
    if(m_receiverThread && m_receiverThread->isRunning()) {
        m_worker->stopReceiving();
        m_receiverThread->quit();
        m_receiverThread->wait(1500); // 等待最多1.5秒
        if(m_receiverThread->isRunning()) {
            m_receiverThread->terminate(); // 如果无法正常退出则强制终止
        }
    }

    delete m_worker;
    delete m_receiverThread;
    m_worker = nullptr;
    m_receiverThread = nullptr;

    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
    m_isBound = false;
    emit portUnbound();
}

void WinSockUdpManager::writeData(const QByteArray &data, const QString &host, quint16 port) {
    if (!isBound()) return;

    sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.toStdString().c_str(), &destAddr.sin_addr);

    sendto(m_socket, data.constData(), data.size(), 0, (sockaddr*)&destAddr, sizeof(destAddr));
}

bool WinSockUdpManager::isBound() const {
    return m_isBound;
}

#endif // Q_OS_WIN