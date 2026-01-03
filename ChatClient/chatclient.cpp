#include "chatclient.h"
#include "ui_chatclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

ChatClient::ChatClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChatClient)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    // 1. 連結網路
    connect(socket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);

    // 2. 連線成功
    connect(socket, &QTcpSocket::connected, this, [=](){
        ui->chatDisplay->addItem("系統提示: 成功連線到伺服器！");
        ui->connectBtn->setEnabled(false); // 連線後停用按鈕
    });

    // 3. 連結好友名單
    connect(ui->userList, &QListWidget::itemClicked, this, &ChatClient::onUserSelected);
}

ChatClient::~ChatClient()
{
    socket->close();
    delete ui;
}

// --- 連線按鈕 ---
void ChatClient::on_connectBtn_clicked()
{
    // 預設暱稱：沒填就是「用戶1」
    myName = ui->nameEdit->text().trimmed();
    if (myName.isEmpty()) {
        myName = "用戶1";
        ui->nameEdit->setText(myName);
    }

    QString ip = ui->ipEdit->text();
    quint16 port = ui->Port->text().toUShort();

    // 連線到指定 IP
    socket->connectToHost(ip, port);

    // 連線成功後發送登入 JSON
    if (socket->waitForConnected(3000)) {
        QJsonObject login;
        login["type"] = "login";
        login["nickname"] = myName;
        socket->write(QJsonDocument(login).toJson());
    } else {
        QMessageBox::critical(this, "錯誤", "連線超時，請檢查伺服器 IP 是否正確");
    }
}

// --- 好友選取 ---
void ChatClient::onUserSelected()
{
    if (ui->userList->currentItem()) {
        currentTarget = ui->userList->currentItem()->text();
        ui->chatDisplay->addItem(QString("--- 正在與 %1 一對一聊天 ---").arg(currentTarget));
    }
}

// --- 文字傳送 ---
void ChatClient::on_sendBtn_clicked()
{
    if (currentTarget.isEmpty()) {
        QMessageBox::warning(this, "提醒", "請先點選左側好友名單選擇聊天對象");
        return;
    }

    QString message = ui->msgEdit->text();
    if (message.isEmpty()) return;

    QJsonObject chat;
    chat["type"] = "chat";
    chat["sender"] = myName;
    chat["target"] = currentTarget;
    chat["content"] = message;

    socket->write(QJsonDocument(chat).toJson());

    // 顯示在自己的視窗
    ui->chatDisplay->addItem(QString("我 -> %1: %2").arg(currentTarget).arg(message));
    ui->msgEdit->clear();
}

// --- 檔案傳送  ---
void ChatClient::on_fileBtn_clicked()
{
    if (currentTarget.isEmpty()) {
        QMessageBox::warning(this, "提醒", "請先點選對象再傳送檔案");
        return;
    }

    QString path = QFileDialog::getOpenFileName(this, "選擇檔案或圖片");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData = file.readAll();

        QJsonObject fileObj;
        fileObj["type"] = "chat";
        fileObj["sender"] = myName;
        fileObj["target"] = currentTarget;
        fileObj["fileName"] = QFileInfo(path).fileName();
        fileObj["fileContent"] = QString(fileData.toBase64()); // 轉成 Base64 字串

        socket->write(QJsonDocument(fileObj).toJson());
        ui->chatDisplay->addItem(QString("已傳送檔案: %1").arg(fileObj["fileName"].toString()));
    }
}

// --- 接收訊息 ---
void ChatClient::onReadyRead()
{
    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "userlist") {
        ui->userList->clear();
        QJsonArray users = obj["users"].toArray();
        for (int i = 0; i < users.size(); ++i) {
            QString name = users[i].toString();
            if (name != myName) {
                ui->userList->addItem(name);
            }
        }
    }
    else if (type == "chat") {
        QString sender = obj["sender"].toString();

        if (obj.contains("fileContent")) {
            QString fileName = obj["fileName"].toString();
            QByteArray fileData = QByteArray::fromBase64(obj["fileContent"].toString().toUtf8());

            if (QMessageBox::question(this, "收到檔案",
                                      QString("來自 %1 的檔案：%2\n是否儲存？").arg(sender).arg(fileName)) == QMessageBox::Yes) {

                QString savePath = QFileDialog::getSaveFileName(this, "儲存檔案", fileName);
                if (!savePath.isEmpty()) {
                    QFile file(savePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(fileData);
                        file.close();
                        ui->chatDisplay->addItem(QString("[系統]: %1 儲存成功").arg(fileName));
                    }
                }
            }
        } else {
            QString content = obj["content"].toString();
            ui->chatDisplay->addItem(QString("%1: %2").arg(sender).arg(content));
        }
    }
}
