#include "WelcomeWindow.h"
#include "ui_WelcomeWindow.h"
#include "MainWindow.h" // 包含主窗口的头文件以创建实例
#include <QDir>
#include <QUrl>
#include <QNetworkRequest>

WelcomeWindow::WelcomeWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WelcomeWindow),
    m_mainWindow(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("欢迎");
    this->setFixedSize(this->size());

    m_networkManager = new QNetworkAccessManager(this);

    QUrl imageUrl("https://pub-a7510641c4c0427886fce394cb093861.r2.dev/%E7%B2%89%E8%89%B2.jpeg");


    // 验证链接是否有效
    if (imageUrl.isValid() && (imageUrl.scheme() == "http" || imageUrl.scheme() == "https")) {
        // 连接信号和槽，下载完成后调用 onImageDownloaded
        connect(m_networkManager, &QNetworkAccessManager::finished, this, &WelcomeWindow::onImageDownloaded);
        // 发送网络请求
        m_networkManager->get(QNetworkRequest(imageUrl));
    } else {
        qDebug() << "Invalid URL:" << imageUrl.toString();
    }
}

WelcomeWindow::~WelcomeWindow()
{
    delete ui;
}

void WelcomeWindow::on_enterButton_clicked()
{
    // 创建并显示主窗口
    m_mainWindow = new MainWindow();
    m_mainWindow->show();

    // 关闭当前的欢迎窗口
    this->close();
}

void WelcomeWindow::onImageDownloaded(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        // 从下载的数据中加载图片
        if (pixmap.loadFromData(imageData)) {
            setBackgroundImage(pixmap);
        } else {
            qDebug() << "Failed to create pixmap from downloaded data.";
        }
    } else {
        // 如果下载失败，在控制台打印错误信息
        qDebug() << "Failed to download image:" << reply->errorString();
    }
    // 清理网络应答对象，防止内存泄漏
    reply->deleteLater();
}

void WelcomeWindow::setBackgroundImage(const QPixmap &pixmap)
{
    // 为了让样式表能正确引用，将下载的图片保存到临时文件中
    QString tempPath = QDir::tempPath() + "/welcome_bg.png";
    if (pixmap.save(tempPath, "PNG")) {
        // --- 样式表定义 ---

        // 1. 定义背景图样式
        QString bgStyle = QString("QWidget#WelcomeWindow { border-image: url(\"%1\"); }").arg(tempPath);

        // 2. 定义UI元素的美化样式
        QString elementStyle = R"(
            /* 标题标签样式 */
            #titleLabel {
                font-size: 26pt; /* 稍微调大字体 */
                font-weight: bold;
                color: #FFFFFF;
                background-color: rgba(0, 0, 0, 0.3); /* 半透明黑色背景 */
                border-radius: 8px; /* 圆角 */
                padding: 10px;
            }

            /* 描述标签样式 */
            #descriptionLabel {
                color: #E0E0E0; /* 柔和的白色 */
                background-color: rgba(0, 0, 0, 0.3); /* 半透明黑色背景 */
                border-radius: 6px; /* 圆角 */
                padding: 8px;
            }

            /* "进入程序" 按钮样式 */
            #enterButton {
                color: white;
                background-color: rgba(0, 0, 0, 0.4); /* 40%透明度的黑色背景 */
                border: 1px solid rgba(255, 255, 255, 0.4); /* 半透明白色边框 */
                border-radius: 8px; /* 圆角 */
                padding: 10px; /* 增加内边距，让按钮更大气 */
                font-size: 12pt;
            }

            /* 鼠标悬停在按钮上时的样式 */
            #enterButton:hover {
                background-color: rgba(0, 0, 0, 0.6); /* 悬停时背景更深 */
                border: 1px solid rgba(255, 255, 255, 0.7); /* 悬停时边框更亮 */
            }

            /* 按钮被按下时的样式 */
            #enterButton:pressed {
                background-color: rgba(0, 0, 0, 0.2); /* 按下时背景更浅 */
            }
        )";

        // 3. 应用所有样式
        this->setStyleSheet(bgStyle + elementStyle);

    } else {
        qDebug() << "Failed to save temporary background image.";
    }
}
