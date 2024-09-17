#include "chat.h"
#include "ui_chat.h"
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
#include<QHostAddress>
#include<QFontComboBox>
#include<QTextEdit>

int Chat::num1 = 0;
int Chat::num2 = 0;

Chat::~Chat()
{
    is_opend = false;
    delete ui;
}

Chat::Chat(QString pasvusername, QString pasvuserip):
    xpasusername(pasvusername),
    xpasuserip(pasvuserip),
    is_opend(false),
    xchat(nullptr),
    used(false),
    ui(new Ui::Chat)
{
    ui->setupUi(this);
    ui->messageTextEdit->setFocusPolicy(Qt::StrongFocus);
    ui->textBrowser->setFocusPolicy(Qt::NoFocus);
    ui->messageTextEdit->setFocus();
    ui->messageTextEdit->installEventFilter(this);

    ui->label->setText(tr("与%1私聊中 对方的IP：%2").arg(xpasusername).arg(xpasuserip));

    xchat = new QUdpSocket(this);
    xport = 45456;

    xchat->bind(QHostAddress(getIp()), xport);
    connect(xchat, SIGNAL(readyRead()), this, SLOT(processPendinDatagrams()));
}

void Chat::sendMessage(messageType type, QString serverAddress)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    QString localHostName = QHostInfo::localHostName();
    QString address = getIp();
    out << type << getUserName() << localHostName;

    switch (type) {
    case LeftParticipant:
        break;
    case Message: {
        used = false;
        if (ui->messageTextEdit->toPlainText() == "") {
            QMessageBox::warning(0, tr("警告"), tr("发送内容不能为空"), QMessageBox::Ok);
            return;
        } else {
            ui->label->setText(tr("与%1私聊中 对方的IP：%2").arg(xpasusername).arg(xpasuserip));
            message = getMessage();
            out << address << message;
            ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
        }
        break;
    }
    case Refuse: {
        out << serverAddress;
        break;
    }
    default:
        break;
    }
    qDebug() << "num1:" << ++num1 << "ipAddress:" << address << "\n" << "message:" << getMessage();
    xchat->writeDatagram(data, data.length(), QHostAddress(xpasuserip), xport);
}

void Chat::processPendinDatagrams()
{
    while (xchat->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(xchat->pendingDatagramSize());
        xchat->readDatagram(datagram.data(), datagram.size());

        QDataStream in(&datagram, QIODevice::ReadOnly);
        int messageType;
        in >> messageType;
        QString userName, localHostName, ipAddress, messagestr;

        qDebug() << "num2:" << ++num2 << "ipAddress:" << ipAddress;
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch (messageType) {
        case Xchat:
            break;
        case Message: {
            ui->label->setText(tr("与%1私聊中 对方的IP：%2").arg(xpasusername).arg(xpasuserip));
            in >> userName >> localHostName >> ipAddress >> messagestr;
            ui->textBrowser->setTextColor(Qt::blue);
            ui->textBrowser->setCurrentFont(QFont("Times New Roman", 12));
            ui->textBrowser->append("[ " + localHostName + " ]" + time);
            qDebug() << "messagestr:" << messagestr;
            ui->textBrowser->append(messagestr);
            this->show();
            is_opend = true;
            break;
        }
        case Refuse: {
            in >> userName >> localHostName;
            QString serverAddress;
            in >> serverAddress;
            if (getIp() == serverAddress) {
                // server->refused(); // 移除了文件传输相关部分
            }
            break;
        }
        case LeftParticipant: {
            in >> userName >> localHostName;
            userLeft(userName, localHostName, time);
            ui->~Chat();
            break;
        }
        default:
            break;
        }
    }
}

QString Chat::getMessage()
{
    QString msg = ui->messageTextEdit->toHtml();
    ui->messageTextEdit->clear();
    ui->messageTextEdit->setFocus();
    return msg;
}

bool Chat::eventFilter(QObject *target, QEvent *event)
{
    if (target == ui->messageTextEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *k = static_cast<QKeyEvent *>(event);
            if (k->key() == Qt::Key_Return) {
                on_sendPushButton_clicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(target, event);
}

void Chat::userLeft(QString userName, QString localHostName, QString time)
{
    ui->textBrowser->setTextColor(Qt::gray);
    ui->textBrowser->setCurrentFont(QFont("Times New Roman", 10));
    if (!used) {
        ui->textBrowser->append(tr("%1于%2离开!").arg(userName).arg(time));
        used = true;
    }
    ui->label->setText(tr("用户%1离开会话界面!").arg(userName));
}

QString Chat::getUserName()
{
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*" << "HOSTNAME.*" << "DOMAINNAME.*";
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

QString Chat::getIp()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol)
            return address.toString();
    }
    return "";
}

void Chat::on_boldToolButton_clicked(bool checked)
{
    if (checked)
        ui->messageTextEdit->setFontWeight(QFont::Bold);
    else
        ui->messageTextEdit->setFontWeight(QFont::Normal);
    ui->messageTextEdit->setFocus();
}

void Chat::on_italicToolButton_clicked(bool checked)
{
    ui->messageTextEdit->setFontItalic(checked);
    ui->messageTextEdit->setFocus();
}

void Chat::on_underToolButton_clicked(bool checked)
{
    ui->messageTextEdit->setFontUnderline(checked);
    ui->messageTextEdit->setFocus();
}

void Chat::on_colorToolButton_clicked()
{
    color = QColorDialog::getColor(color, this);
    if (color.isValid()) {
        ui->messageTextEdit->setTextColor(color);
        ui->messageTextEdit->setFocus();
    }
}

void Chat::on_saveToolButton_clicked()
{
    if (ui->textBrowser->document()->isEmpty()) {
        QMessageBox::warning(0, tr("警告"), tr("聊天记录为空,无法保存!"), QMessageBox::Ok);
    } else {
        QString fileName = QFileDialog::getSaveFileName(this, tr("保存聊天记录"), tr("聊天记录"),
                                                        tr("文本(*.txt);All File(*.*)"));
        if (!fileName.isEmpty())
            saveFile(fileName);
    }
}

void Chat::saveFile(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("保存文件"), tr("无法保存文件 %1:\n %2").arg(fileName).arg(file.errorString()));
        return;
    }
    QTextStream out(&file);
    out << ui->textBrowser->toPlainText();
}

void Chat::on_clearToolButton_clicked()
{
    ui->textBrowser->clear();
}

void Chat::on_closePushButton_clicked()
{
    sendMessage(LeftParticipant);
    ui->textBrowser->clear();
    ui->messageTextEdit->clear();
    close();
    ui->label->setText(tr("与%1私聊中 对方的IP：%2").arg(xpasusername).arg(xpasuserip));
    ui->~Chat();
}

void Chat::on_sendPushButton_clicked()
{
    sendMessage(Message);
    QString localHostName = QHostInfo::localHostName();
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->textBrowser->setTextColor(Qt::blue);
    ui->textBrowser->setCurrentFont(QFont("Times New Roman", 12));
    ui->textBrowser->append("[" + localHostName + " ]" + time);
    ui->textBrowser->append(message);
    qDebug() << "message" << message;
    qDebug() << "发送的消息:" << getMessage();
}

void Chat::closeEvent(QCloseEvent *)
{
    on_closePushButton_clicked();
}

void Chat::on_fontComboBox_currentFontChanged(const QFont &f)
{
    ui->messageTextEdit->setCurrentFont(f);
    ui->messageTextEdit->setFocus();
}

void Chat::on_comboBox_currentIndexChanged(const QString &arg1)
{
    ui->messageTextEdit->setFontPointSize(arg1.toDouble());
    ui->messageTextEdit->setFocus();
}
