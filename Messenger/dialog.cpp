#include "dialog.h"
#include "ui_dialog.h"

#include <QtGui>
#include <QDebug>

Dialog::Dialog(QWidget *parent) :QDialog(parent),ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->pteMessage->installEventFilter(this);


    myName = "";
    socket = new QTcpSocket(this);
    connected = false;
    log = new QList<MesData>;

    connect(socket, SIGNAL(readyRead()), this, SLOT(slotSocketReadyRead()));
    connect(socket, SIGNAL(connected()), this, SLOT(slotSocketConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(slotSocketDisconnected()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(slotSocketDisplayError(QAbstractSocket::SocketError)));
    connect(this, SIGNAL(sigShiftEnterPushed()), this, SLOT(on_pbSend_clicked()));
}


bool Dialog::eventFilter(QObject *target, QEvent *event) {
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = (QKeyEvent *)event;
        if((keyEvent -> modifiers() & Qt::ShiftModifier) && ((keyEvent->key() == Qt::Key_Enter) || (keyEvent->key() == Qt::Key_Return))){
            emit(sigShiftEnterPushed());
            return true;
        }
    }

    return false;
}

Dialog::~Dialog()
{
    QMap<QString, QList<MesData> *>::const_iterator iter;
    for(iter = dmessages.constBegin(); iter != dmessages.constBegin(); ++iter){
        iter.value()->clear();
    }
    qDeleteAll(dmessages);
    dmessages.clear();
    delete log;
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
            break;
        }

        //сервер передает список имен как QStringList
        case Client::comUsersOnline:{
            ui->pbSend->setEnabled(true);
            QStringList users;
            in >> users;
            updateUserList(users);

            //в лог сообщений от сервера пишем инф-цию о подключении
            addToLog("Connected to " + socket->peerAddress().toString() + "/" + QString::number(socket->peerPort()), Qt::darkGreen);
            addToLog("You entered as " + myName, Qt::darkGreen);
            addToLog("Received list of users", Qt::darkGreen);
            break;
        }

        //общее сообщение от сервера
        case Client::comPublicServerMessage:{
            QString message;
            in >> message;
            addToLog("[Server(public)]: " + message, Qt::darkBlue);
            break;
        }

        //личное сообщение от сервера
        case Client::comPrivateServerMessage:{
            QString message;
            in >> message;
            addToLog("[Server(private)]: " + message, Qt::blue);
            break;
        }


        //общее сообщение от другого пользователя
        case Client::comMessageToAll:{
            QString fromUser;
            in >> fromUser;
            QString message;
            in >> message;
            QColor color = (fromUser == myName) ? Qt::blue : Qt::black;
            insertMessage("All", fromUser, myName, message, QTime::currentTime(), color);
            break;
        }

        //личное сообщение от другого пользователя
        case Client::comMessageToUsers:{
            QString fromUser;
            in >> fromUser;
            QString message;
            if(fromUser == myName){
                QStringList users;
                in >> users >> message;
                foreach(QString user, users){
                    insertMessage(user, myName, user, message, QTime::currentTime(), Qt::blue);
                }
            }
            else{
                in >> message;
                insertMessage(fromUser, fromUser, myName,  message, QTime::currentTime(), Qt::black);
            }
            break;
        }

        //добавился новый пользователь
        case Client::comUserJoin:{
            QString name;
            QStringList users;
            in >> name >> users;
            addToLog(name + " joined",  Qt::darkGreen);
            updateUserList(users);
            break;
        }

        case Client::comUserLeft:{
            QString name;
            QStringList users;
            in >> name >> users;
            addToLog(name + " left",  Qt::darkGreen);
            updateUserList(users);
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
    ui->lwLog->clear();
    ui->pteAdresses->clear();

    QMap<QString, QList<MesData> *>::const_iterator iter;
    for(iter = dmessages.constBegin(); iter != dmessages.constBegin(); ++iter){
        iter.value()->clear();
    }
    qDeleteAll(dmessages);
    dmessages.clear();

    QList<MesData>::const_iterator log_iter;
    for(log_iter = log->constBegin(); log_iter != log->constEnd(); ++log_iter){
        ui->lwLog->addItem(log_iter->getTime().toString() + " " + log_iter->getText());
        ui->lwLog->item(ui->lwLog->count()-1)->setForeground(log_iter->getColor());
    }


    addToLog("Disconnected from " + socket->peerAddress().toString() + "/" + QString::number(socket->peerPort()), Qt::red);
}

//по нажатию кнопки подключаемся к северу, отметим, что connectToHost() возвращает тип void, потому, что это асинхронный вызов и в случае ошибки будет вызван слот onSokDisplayError
void Dialog::on_pbConnect_clicked()
{
    if (connected)
        socket->disconnectFromHost();
    else
        socket->connectToHost(ui->leHost->text(), ui->lePort->text().toInt());

}


void Dialog::on_pbSend_clicked()
{

    QString message = ui->pteMessage->document()->toPlainText();
    if(message.contains(QRegExp("[^\\s]"))){
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out << (quint16)0;

        //ошибка, если пользователь только All
        if (ui->lwUsers->count() == 1)
        {
            QMessageBox::information(this, "", "No users connected");
            return;
        }

        QStringList UserList = ui->pteAdresses->document()->toPlainText().split(QRegExp("[,.\\s]\\s*"), QString::SkipEmptyParts);
        if(UserList.isEmpty()){
            return;
        }
        foreach(QString s, UserList){
            if(ui->lwUsers->findItems(s, Qt::MatchExactly).isEmpty()){
                QMessageBox::information(this, "", "There is no clients under this name: " + s);
                return;
            }
        }

        if (UserList.contains("All"))
            out << (quint8)Client::comMessageToAll;
        else
            out << (quint8)Client::comMessageToUsers << UserList;

        out << message;
        out.device()->seek(0);
        out << (quint16)(block.size() - sizeof(quint16));
        socket->write(block);
        ui->pteMessage->clear();
    }
}


void Dialog::addToLog(QString text, QColor color)
{
    MesData message("Server", myName, text, QTime::currentTime(), color);
    log->push_back(message);

    if(ui->lwUsers->selectedItems().isEmpty()){
        ui->lwLog->insertItem(0, message.getTime().toString() + " " + message.getText());
        ui->lwLog->item(0)->setForeground(color);
    }
}

//обновление списка пользователей и истории сообщений
void Dialog::updateUserList(QStringList users){
    //добавление пользователя All для истории сообщений
    users.prepend("All");

    foreach(QString user, users){
        if (!dmessages.contains(user)){
            QList<MesData> *messageList = new QList<MesData>;
            dmessages.insert(user, messageList);
        }
    }

    //проверяем на покинувших пользователей
    QMap<QString, QList<MesData> *>::iterator iter = dmessages.begin();
    while(iter != dmessages.end()){
        if (!users.contains(iter.key())){
            iter.value()->clear();
            delete iter.value();

            //erase стирает элемент и возвращает указатель на следующий
            iter = dmessages.erase(iter);
            continue;
        }
        ++iter;
    }

    //обновляем список и восстанавливаем нужный чат
    if(ui->lwUsers->selectedItems().isEmpty() || !users.contains(ui->lwUsers->selectedItems().first()->text())){
        ui->lwUsers->clear();
        ui->lwUsers->addItems(users);
    }
    else{
        QString activeName = ui->lwUsers->selectedItems().first()->text();
        ui->lwUsers->clear();
        ui->lwUsers->addItems(users);
        ui->lwUsers->findItems(activeName, Qt::MatchExactly).first()->setSelected(true);
    }
}

void Dialog::on_lwUsers_itemSelectionChanged()
{
    // очистка экрана
    ui->lwLog->clear();

    // вывод сообщений сервера при пустом выделении и пользователей при непустом
    if(ui->lwUsers->selectedItems().isEmpty()){
        QList<MesData>::const_iterator iter;
        for(iter = log->constBegin(); iter != log->constEnd(); ++iter){
            ui->lwLog->insertItem(0, iter->getTime().toString() + " " + iter->getText());
            ui->lwLog->item(0)->setForeground(iter->getColor());
        }
    }
    else {
        //вывод на экран истории сообщений
        QList<MesData> *mesList = dmessages[ui->lwUsers->selectedItems().first()->text()];
        QList<MesData>::const_iterator iter;
        for(iter = mesList->constBegin(); iter != mesList->constEnd(); ++iter){
            ui->lwLog->addItem(iter->getTime().toString() + " [" + iter->getSender() + "]: " + iter->getText());
            ui->lwLog->item(ui->lwLog->count()-1)->setForeground(iter->getColor());
        }
        ui->lwLog->setCurrentItem(ui->lwLog->item(ui->lwLog->count() - 1));

        QStringList UserList;
        foreach (QListWidgetItem *i, ui->lwUsers->selectedItems()){
            UserList << i->text();
        }

        ui->pteAdresses->setPlainText(UserList.join("\n"));
    }
}

//добавление записи в историю сообщений с userName
void Dialog::insertMessage(QString userName,QString sender, QString target, QString text, QTime time, QColor color){
    MesData message(sender, target, text, time, color);
    dmessages[userName]->push_back(message);

    //обновление экрана
    if(!ui->lwUsers->selectedItems().isEmpty() && userName == ui->lwUsers->selectedItems().first()->text()){
        ui->lwLog->addItem(time.toString() + " [" + sender + "]: " + text);
        ui->lwLog->item(ui->lwLog->count()-1)->setForeground(color);
        ui->lwLog->setCurrentItem(ui->lwLog->item(ui->lwLog->count() - 1));
    }
}
