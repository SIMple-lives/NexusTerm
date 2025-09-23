#ifndef WELCOMEWINDOW_H
#define WELCOMEWINDOW_H

#include <QWidget>
#include <QNetworkAccessManager> // For network requests
#include <QNetworkReply>        // To handle network replies
#include <QPixmap>              // To handle the image

// 前向声明MainWindow，避免循环包含
class MainWindow;

namespace Ui {
class WelcomeWindow;
}

class WelcomeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeWindow(QWidget *parent = nullptr);
    ~WelcomeWindow();

private slots:
    void on_enterButton_clicked();
    void onImageDownloaded(QNetworkReply *reply); // Slot to handle the completed download

private:
    Ui::WelcomeWindow *ui;
    MainWindow *m_mainWindow; // 持有主窗口的指针
    QNetworkAccessManager *m_networkManager; // Manages network requests

    void setBackgroundImage(const QPixmap &pixmap); // Helper function to apply the background
};

#endif // WELCOMEWINDOW_H