#include "widget.h"
#include "ui_widget.h"
#include<QHostInfo>
#include<QMessageBox>
#include<QDateTime>
#include<QProcess>
#include<QDataStream>
#include<QScrollBar>
#include<QFont>
#include<QNetworkInterface>
#include<QStringList>
#include<QDebug>
#include<QFileDialog>
#include<QColorDialog>

int Widget::count = 0;
int Widget::count2 = 0;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    udpSocket(nullptr),
    server(nullptr),
    privateChat(nullptr),
    privateChat1(nullptr),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->messageTextEdit->setFocusPolicy(Qt::StrongFocus);
    ui->messageBrower->setFocusPolicy(Qt::NoFocus);
    ui->messageTextEdit->setFocus();
    ui->messageTextEdit->installEventFilter(this);

    udpSocket = new QUdpSocket(this);
    port = 45454;
    udpSocket->bind(port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));

    sendMessage(NewParticipant);

    // connect(server, SIGNAL(sendFileName(QString)),
    //         this, SLOT(getFileName(QString)));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::sendMessage(messageType type, QString serverAddress)
{
    qDebug() << "消息到达了";
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);

    QString localHostName = QHostInfo::localHostName();
    QString address = getIP();

    out << type << getUserName() << localHostName;
    qDebug() << "count" << ++count << "type: " << type << "userName :" << getUserName() << " localHostName:" << localHostName;

    switch (type) {
    case Message: {
        if (ui->messageTextEdit->toPlainText() == "") {
            QMessageBox::warning(0, tr("警告"), tr("发送内容不能为空"), QMessageBox::Ok);
            return;
        }

        out << address << getMessage();
        ui->messageBrower->verticalScrollBar()->setValue(ui->messageBrower->verticalScrollBar()->maximum());

        break;
    }
    case NewParticipant: {
        out << address;
        qDebug() << address;
        break;
    }
    case LeftParticipant:
        break;
    case Refuse: {
        out << serverAddress;
        break;
    }
    default:
        break;
    }

    udpSocket->writeDatagram(data, data.length(), QHostAddress::Broadcast, port);
}

void Widget::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());

        QDataStream in(&datagram, QIODevice::ReadOnly);
        int messageType;
        in >> messageType;
        QString userName, localHostName, ipAddress, message;

        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch (messageType) {
        case Message: {
            in >> userName >> localHostName >> ipAddress >> message;

            ui->messageBrower->setTextColor(Qt::blue);
            ui->messageBrower->setCurrentFont(QFont("Times New Roman", 12));
            ui->messageBrower->append("[ " + userName + " ]" + time);
            ui->messageBrower->append(message);
            qDebug() << "message:" << message;
            break;
        }
        case NewParticipant: {
            in >> userName >> localHostName >> ipAddress;
            newParticipant(userName, localHostName, ipAddress);
            break;
        }
        case LeftParticipant: {
            in >> userName >> localHostName;
            leftParticipant(userName, localHostName, time);
            break;
        }
        case Refuse: {
            in >> userName >> localHostName;
            QString serverAddress;
            in >> serverAddress;
            QString ipAddress = getIP();
            if (ipAddress == serverAddress) {
                server->refused();
            }
            break;
        }
        case Xchat: {
            in >> userName >> localHostName >> ipAddress;
            showXchat(localHostName, ipAddress);
            break;
        }
        default:
            break;
        }
        qDebug() << "count2:" << ++count2 << "userName:" << userName << "localHostName:" << localHostName << "ipAddress:" << ipAddress << "\n";
    }
}

void Widget::newParticipant(QString userName, QString localHostName, QString ipAddress)
{
    bool isEmpty = ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).isEmpty();
    qDebug() << "返回的消息";

    if (isEmpty) {
        QTableWidgetItem *user = new QTableWidgetItem(userName);
        QTableWidgetItem *host = new QTableWidgetItem(localHostName);
        QTableWidgetItem *ip = new QTableWidgetItem(ipAddress);

        ui->userTableWidget->insertRow(0);
        ui->userTableWidget->setItem(0, 0, user);
        ui->userTableWidget->setItem(0, 1, host);
        ui->userTableWidget->setItem(0, 2, ip);

        ui->messageBrower->setTextColor(Qt::gray);
        ui->messageBrower->setCurrentFont(QFont("Times New Roman", 10));

        ui->messageBrower->append(tr("%1 在线!").arg(userName));
        ui->userNumLabel->setText(tr("在线人数:%1").arg(ui->userTableWidget->rowCount()));
        sendMessage(NewParticipant);
    }
}

void Widget::leftParticipant(QString userName, QString localHostName, QString time)
{
    int rowNum = ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).first()->row();

    ui->userTableWidget->removeRow(rowNum);
    ui->messageBrower->setTextColor(Qt::gray);
    ui->messageBrower->setCurrentFont(QFont("Times New Roman", 10));
    ui->messageBrower->append(tr("%1于%2离开!").arg(userName).arg(time));
    ui->userNumLabel->setText(tr("在线人数:%1").arg(ui->userTableWidget->rowCount()));
}

QString Widget::getIP()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol)
            return address.toString();
    }
    return "";
}

QString Widget::getUserName()
{
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";
    QStringList environment = QProcess::systemEnvironment();
    foreach (QString string, envVariables) {
        int index = environment.indexOf(QRegExp(string));
        if (index != -1) {
            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2) {
                return stringList.at(1);
                break;
            }
        }
    }
    return "unknown";
}

QString Widget::getMessage()
{
    QString msg = ui->messageTextEdit->toHtml();
    ui->messageTextEdit->clear();
    ui->messageTextEdit->setFocus();
    return msg;
}

void Widget::closeEvent(QCloseEvent *e)
{
    sendMessage(LeftParticipant);
    QWidget::closeEvent(e);
}

void Widget::on_sendPushButton_clicked()
{
    sendMessage(Message);
}

bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->messageTextEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *k = static_cast<QKeyEvent *>(event);
            if (k->key() == Qt::Key_Return) {
                on_sendPushButton_clicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void Widget::on_exitPushButton_clicked()
{
    close();
}

void Widget::on_fontComBox_currentFontChanged(const QFont &f)
{
    ui->messageTextEdit->setCurrentFont(f);
    ui->messageTextEdit->setFocus();
}

void Widget::on_sizeComboBox_currentIndexChanged(const QString &arg1)
{
    ui->messageTextEdit->setFontPointSize(arg1.toDouble());
    ui->messageTextEdit->setFocus();
}
// 加粗
void Widget::on_boldToolBtn_clicked(bool checked)
{

}
// 倾斜
void Widget::on_italicToolBtn_clicked(bool checked)
{

}
// 下划线
void Widget::on_underlineToolBtn_clicked(bool checked)
{

}
// 颜色
void Widget::on_clearToolBtn_clicked()
{

}

void Widget::on_userTableWidget_doubleClicked(const QModelIndex &index)
{
    if (ui->userTableWidget->item(index.row(), 0)->text() == getUserName() &&
        ui->userTableWidget->item(index.row(), 2)->text() == getIP()) {
        QMessageBox::warning(0, tr("警告"), tr("不可以和自己聊天！！！"), QMessageBox::Ok);
        return;
    } else {
        if (!privateChat) {
            qDebug() << "进入了";
            privateChat = new Chat(ui->userTableWidget->item(index.row(), 1)->text(), // 主机名
                                   ui->userTableWidget->item(index.row(), 2)->text());
        }
        qDebug() << 1;
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        QString localHostName = QHostInfo::localHostName();
        QString address = getIP();
        out << Xchat << getUserName() << localHostName << address;
        udpSocket->writeDatagram(data, data.length(), QHostAddress(ui->userTableWidget->item(index.row(), 2)->text()), port);
        privateChat->show();
        privateChat->is_opend = true;
        qDebug() << Xchat << getUserName() << localHostName << address;
    }
}

void Widget::showXchat(QString localHostName, QString ipAddress)
{
    if (!privateChat1)
        privateChat1 = new Chat(localHostName, ipAddress);
}

//传输文件
void Widget::on_sendToolBtn_clicked()
{

}
