#include "dialog.h"
#include "ui_dialog.h"

#include <QtGui>
#include <QDebug>

Dialog::Dialog(QWidget *parent) :QDialog(parent),ui(new Ui::Dialog)
{
    ui->setupUi(this);

    myName = "";
    socket = new QTcpSocket(this);
    connected = false;

    connect(socket, SIGNAL(readyRead()), this, SLOT(slotSocketReadyRead()));
    connect(socket, SIGNAL(connected()), this, SLOT(slotSocketConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(slotSocketDisconnected()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(slotSocketDisplayError(QAbstractSocket::SocketError)));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::slotSocketDisplayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, "Error", "The host was not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, "Error", "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(this, "Error", "The following error occurred: " + socket->errorString());
    }
}

void Dialog::slotSocketReadyRead()
{
    //тут обрабатываются данные от сервера
    QDataStream in(socket);
    //если считываем новый блок первые 2 байта это его размер
    if (blockSize == 0) {
        //если пришло меньше 2 байт ждем пока будет 2 байта
        if (socket->bytesAvailable() < (int)sizeof(quint16))
            return;
        //считываем размер (2 байта)
        in >> blockSize;
        //qDebug() << "blockSize now " << blockSize;
    }
    //ждем пока блок прийдет полностью
    if (socket->bytesAvailable() < blockSize)
        return;
    else
        //можно принимать новый блок
        blockSize = 0;

    //3 байт - команда серверу
    quint8 command;
    in >> command;
    //qDebug() << "Received command " << command;

    switch (command)
    {
        //сервер отправит список пользователей, если авторизация пройдена, в
        //таком случае третий байт равен константе Client::comUsersOnline
        case Client::comAutchSuccess:{
            ui->pbSend->setEnabled(true);
            addToLog("You entered as " + myName, Qt::green);
            break;
        }

        //сервер передает список имен как QStringList
        case Client::comUsersOnline:{
            addToLog("Received user list " + myName, Qt::green);
            ui->pbSend->setEnabled(true);
            QStringList users;
            in >> users;
            ui->lwUsers->clear();
            ui->lwUsers->addItems(users);
            break;
        }

        //общее сообщение от сервера
        case Client::comPublicServerMessage:{
            QString message;
            in >> message;
            addToLog("[PublicServerMessage]: " + message, Qt::red);
            break;
        }

        //общее сообщение от другого пользователя
        case Client::comMessageToAll:{
            QString user;
            in >> user;
            QString message;
            in >> message;
            addToLog("[" + user + "]: " + message);
            break;
        }

        //личное сообщение от другого пользователя
        case Client::comMessageToUsers:{
            QString user;
            in >> user;
            QString message;
            in >> message;
            addToLog("[" + user + "](private): " + message, Qt::blue);
            break;
        }

        //личное сообщение от сервера
        case Client::comPrivateServerMessage:{
            QString message;
            in >> message;
            addToLog("[PrivateServerMessage]: "+message, Qt::red);
            break;
        }

        //добавился новый пользователь
        case Client::comUserJoin:{
            QString name;
            QStringList users;
            in >> name >> users;
            ui->lwUsers->clear();
            ui->lwUsers->addItems(users);
            addToLog(name + " joined", Qt::green);
            break;
        }

        case Client::comUserLeft:{
            QString name;
            QStringList users;
            in >> name >> users;
            ui->lwUsers->clear();
            ui->lwUsers->addItems(users);
            addToLog(name + " left", Qt::green);
            break;
        }

        //неподходящее имя
        case Client::comErrNameInvalid:{
            QMessageBox::information(this, "Error", "This name is invalid.");
            socket->disconnectFromHost();
            break;
        }

        //имя уже используется
        case Client::comErrNameUsed:{
            QMessageBox::information(this, "Error", "This name is already used.");
            socket->disconnectFromHost();
            break;
        }
    }
}


void Dialog::slotSocketConnected()
{
    ui->pbConnect->setText("Disconnect");
    connected = true;

    blockSize = 0;
    addToLog("Connected to " + socket->peerAddress().toString() + "/" + QString::number(socket->peerPort()), Qt::green);

    //после подключения следует отправить запрос на авторизацию
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    //резервируем 2 байта для размера блока.
    out << (quint16)0;
    out << (quint8)Client::comAutchReq;
    out << ui->leName->text();
    myName = ui->leName->text();

    //возваращаемся в начало
    out.device()->seek(0);

    //вписываем размер блока на зарезервированное место
    out << (quint16)(block.size() - sizeof(quint16));
    socket->write(block);
}

void Dialog::slotSocketDisconnected()
{
    ui->pbConnect->setText("Connect");
    connected = false;

    ui->pbSend->setEnabled(false);
    ui->lwUsers->clear();
    addToLog("Disconnected from " + socket->peerAddress().toString() + "/" + QString::number(socket->peerPort()), Qt::green);
}

//по нажатию кнопки подключаемся к северу, отметим, что connectToHost() возвращает тип void, потому, что это асинхронный вызов и в случае ошибки будет вызван слот onSokDisplayError
void Dialog::on_pbConnect_clicked()
{
    if (connected)
        socket->disconnectFromHost();
    else
        socket->connectToHost(ui->leHost->text(), ui->lePort->text().toInt());

}

void Dialog::on_cbToAll_clicked()
{
    if (ui->cbToAll->isChecked())
        ui->pbSend->setText("Send To All");
    else
        ui->pbSend->setText("Send To Selected");
}

void Dialog::on_pbSend_clicked()
{
    QString message = ui->pteMessage->document()->toPlainText();
    if(message.contains(QRegExp("[^\\s]"))){
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out << (quint16)0;
        if (ui->cbToAll->isChecked())
            out << (quint8)Client::comMessageToAll;
        else
        {
            out << (quint8)Client::comMessageToUsers;
            QString s;
            foreach (QListWidgetItem *i, ui->lwUsers->selectedItems())
                s += i->text()+",";
            s.remove(s.length()-1, 1);
            out << s;
        }

        out << message;
        out.device()->seek(0);
        out << (quint16)(block.size() - sizeof(quint16));
        socket->write(block);
        ui->pteMessage->clear();
    }
}

void Dialog::addToLog(QString text, QColor color)
{
    ui->lwLog->insertItem(0, QTime::currentTime().toString() + " " + text);
    ui->lwLog->item(0)->setForeground(color);
}

void Dialog::on_lwUsers_itemSelectionChanged()
{
    QStringList UserList;
    foreach (QListWidgetItem *i, ui->lwUsers->selectedItems()){
        UserList << i->text();
    }
    ui->pteAdresses->setPlainText(UserList.join("\n"));
}
