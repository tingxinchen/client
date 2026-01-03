#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QMainWindow>
#include <QTcpSocket>    // 必須加這個才能連線 [cite: 18]
#include <QHostAddress>
#include <QJsonDocument> // 處理 JSON 格式
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ChatClient; }
QT_END_NAMESPACE

class ChatClient : public QMainWindow {
    Q_OBJECT

public:
    ChatClient(QWidget *parent = nullptr);
    ~ChatClient();

private slots:
    // 對應你 UI 上的元件名稱
    void on_connectBtn_clicked();
    void on_sendBtn_clicked();
    void on_fileBtn_clicked();
    void onReadyRead();      // 處理接收到的訊息 [cite: 28]
    void onUserSelected();   // 好友名單選取

private:
    Ui::ChatClient *ui;
    QTcpSocket *socket;      // 建立伺服器連線的物件 [cite: 18]
    QString myName;
    QString currentTarget;   // 一對一聊天的目標 [cite: 44]
};

#endif
