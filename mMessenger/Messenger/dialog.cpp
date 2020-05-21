#include "dialog.h"
#include "ui_dialog.h"

#include <QtGui>
#include <QDebug>

Dialog::Dialog(QWidget *parent) :QDialog(parent),ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->pteMessage->installEventFilter(this);   //перехват события SHift + Enter при вводе сообщения при отправке


    myName = "";
    socket = new QTcpSocket(this);
    connected = false;
    log = new QList<MesData>;
    encrypt = QRSAEncryption(QRSAEncryption::Rsa::RSA_256);

    //установление сигнально-слотовой связи с сервером и ui
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
    KeyList.clear();
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

        //сервер передает список имен и ключей
        case Client::comUsersOnline:{
            ui->pbSend->setEnabled(true);
            QMap<QString, QString> users;
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
            QString encodeMessage;
            in >> encodeMessage;

            //зашифрованное сообщение расшифровываем и переводим в UTF-8 (106)
            QString message = QString(encrypt.decode(QByteArray::fromHex(encodeMessage.toUtf8()), privKey, encrypt.getRsa()));
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
            insertMessage("All", fromUser, myName, message, QTime::currentTime(), color, true);
            break;
        }

        //личное сообщение от другого пользователя
        case Client::comMessageToUser:{
            QString fromUser;
            in >> fromUser;
            QString encodeMessage;
            in >> encodeMessage;

            //зашифрованное сообщение расшифровываем и переводим в UTF-8 (106)
            QString message = QString(encrypt.decode(QByteArray::fromHex(encodeMessage.toUtf8()), privKey, encrypt.getRsa()));

            //если отправитель мы, просто выводим на экран, если нет - посылаем подтверждение
            if(fromUser == myName){
                insertMessage(myName, myName, myName, message, QTime::currentTime(), Qt::blue, true);
            }
            else{
                insertMessage(fromUser, fromUser, myName,  message, QTime::currentTime(), Qt::black, true);

                //отправление подтверждения о получении
                QByteArray block;
                QDataStream out(&block, QIODevice::WriteOnly);
                out << (quint16)0;
                out << (quint8)Client::comDelivered;
                out << fromUser;
                out.device()->seek(0);
                out << (quint16)(block.size() - sizeof(quint16));
                socket->write(block);
            }
            break;
        }

        //добавился новый пользователь
        case Client::comUserJoin:{
            QString name;
            QMap<QString, QString> users;
            in >> name >> users;
            addToLog(name + " joined",  Qt::darkGreen);
            updateUserList(users);
            break;
        }

        case Client::comUserLeft:{
            QString name;
            QMap<QString, QString> users;
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

        //сообщение доставлено
        case Client::comDelivered:{
            QString user;
            in >> user;
            QList<MesData> *mesList = dmessages[user];
            QList<MesData>::iterator iter;
            for(iter = mesList->begin(); iter != mesList->end(); ++iter){
                if(!iter->isDelivered()){
                    iter->setDelivered(true);
                }
            }

            updateScreen();
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


    encrypt.generatePairKey(pubKey, privKey, QRSAEncryption::Rsa::RSA_256); //генерация ключей шифрования

    //отправляем команду запроса на авторизацию, своё имя и публичный ключ в виде hex представления в строке
    out << (quint8)Client::comAutchReq;
    out << ui->leName->text();
    out << QString(pubKey.toHex());

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
    KeyList.clear();

    //вывод серверного лога
    QList<MesData>::const_iterator log_iter;
    for(log_iter = log->constBegin(); log_iter != log->constEnd(); ++log_iter){
        ui->lwLog->insertItem(0,log_iter->getTime().toString() + " " + log_iter->getText());
        ui->lwLog->item(0)->setForeground(log_iter->getColor());
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
    // получаем сообщение
    QString message = ui->pteMessage->document()->toPlainText();
    if(message.contains(QRegExp("[^\\s]"))){
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);

        //ошибка, если пользователь только All
        if (ui->lwUsers->count() == 1)
        {
            QMessageBox::information(this, "", "No users connected");
            return;
        }

        //собираем список адресатов
        QStringList UserList = ui->pteAdresses->document()->toPlainText().split(QRegExp("[,.\\s]\\s*"), QString::SkipEmptyParts);
        if(UserList.isEmpty()){
            return;
        }

        //проверяем их
        foreach(QString s, UserList){
            if(ui->lwUsers->findItems(s, Qt::MatchExactly).isEmpty()){
                QMessageBox::information(this, "", "There is no clients under this name: " + s);
                return;
            }
        }

        //если список содержит All, то отсылаем всем
        if (UserList.contains("All")){
            out << (quint16)0;
            out << (quint8)Client::comMessageToAll;
            out << message;
            out.device()->seek(0);
            out << (quint16)(block.size() - sizeof(quint16));
            socket->write(block);
        }
        else{
            // отсылаем каждому пользователю зашифрованное сообщение
            QByteArray encodeMessage; //зашифрованное сообщение
            foreach(QString user, UserList){
                out << (quint16)0 << (quint8)Client::comMessageToUser << user;

                qDebug() << user;
                //шифруем сообщение
                encodeMessage = encrypt.encode(message.toUtf8(), KeyList[user], encrypt.getRsa());

                //приводим сообщение к hex представлению в строке для отправки
                out << QString(encodeMessage.toHex());
                out.device()->seek(0);
                out << (quint16)(block.size() - sizeof(quint16));
                socket->write(block);
                qDebug() << block.size();

                //очищаем устройство ввода-вывода
                out.device()->seek(0);
                block.clear();


                //если адресат не мы, выводим сообщение на экран, помечая как недоставленное
                if (user != myName){
                    insertMessage(user, myName, user, message, QTime::currentTime(), Qt::blue, false);
                }
            }
        }
        ui->pteMessage->clear();
    }
}


void Dialog::addToLog(QString text, QColor color)
{
    MesData message("Server", myName, text, QTime::currentTime(), color, true);
    log->push_back(message);

    updateScreen();
}

//обновление списка пользователей и истории сообщений
void Dialog::updateUserList(QMap<QString, QString> users){
    //добавление пользователя All для истории сообщений
    users.insert("All", QString(""));

    //проверяем необходимость добавления, итерируем полученный список
    QMap<QString, QString>::const_iterator users_iter = users.begin();
    for(users_iter = users.begin(); users_iter != users.end(); ++users_iter){
        if (!dmessages.contains(users_iter.key())){
            QList<MesData> *messageList = new QList<MesData>;
            dmessages.insert(users_iter.key(), messageList);

            //добавляем ключ пользователя, раскодируя его в QByteArray
            KeyList.insert(users_iter.key(), QByteArray::fromHex(users_iter.value().toUtf8()));
        }
    }

    //проверяем на покинувших пользователей
    QMap<QString, QList<MesData> *>::iterator mes_iter = dmessages.begin();
    while(mes_iter != dmessages.end()){
        if (!users.contains(mes_iter.key())){
            mes_iter.value()->clear();
            delete mes_iter.value();

            //удаляем ключ пользователя
            KeyList.remove(mes_iter.key());

            //erase стирает элемент и возвращает указатель на следующий
            mes_iter = dmessages.erase(mes_iter);
            continue;
        }
        ++mes_iter;
    }

    //обновляем список и восстанавливаем нужный чат (если нужно, запоминаем последнего выделенного)
    if(ui->lwUsers->selectedItems().isEmpty() || !users.contains(ui->lwUsers->selectedItems().first()->text())){
        ui->lwUsers->clear();
        ui->lwUsers->addItems(users.keys());
    }
    else{
        QString activeName = ui->lwUsers->selectedItems().first()->text();
        ui->lwUsers->clear();
        ui->lwUsers->addItems(users.keys());
        ui->lwUsers->findItems(activeName, Qt::MatchExactly).first()->setSelected(true);
    }
}

void Dialog::on_lwUsers_itemSelectionChanged()
{
    // очистка экрана
    ui->lwLog->clear();

    // вывод сообщений сервера при пустом выделении и пользователей при непустом
    if(!ui->lwUsers->selectedItems().isEmpty()){

        //собираем и выводим потенциальных выделенных адресатов
        QStringList UserList;
        foreach (QListWidgetItem *i, ui->lwUsers->selectedItems()){
            UserList << i->text();
        }
        ui->pteAdresses->setPlainText(UserList.join("\n"));
    }
    updateScreen();
}

//добавление записи в историю сообщений с userName
void Dialog::insertMessage(QString userName,QString sender, QString target, QString text, QTime time, QColor color, bool delivered){
    MesData message(sender, target, text, time, color, delivered);
    dmessages[userName]->push_back(message);

    //подсвечиваем чат с новым сообщением
    QListWidgetItem *item;
    for(int i = 0; i < ui->lwUsers->count(); ++i){
        item = ui->lwUsers->item(i);
        if(item->text() == userName){
            item->setBackground(Qt::cyan);
        }
    }

    //обновление экрана
    updateScreen();
}

void Dialog::updateScreen(){
    // очистка экрана
    ui->lwLog->clear();

    // вывод сообщений сервера при пустом выделении и пользователей при непустом
    if(ui->lwUsers->selectedItems().isEmpty()){

        //итерируем серверный лог и выводим сообщения с цветом
        QList<MesData>::const_iterator iter;
        for(iter = log->constBegin(); iter != log->constEnd(); ++iter){
            ui->lwLog->insertItem(0, iter->getTime().toString() + " " + iter->getText());
            ui->lwLog->item(0)->setForeground(iter->getColor());
        }
    }
    else {
        ui->lwUsers->selectedItems().first()->setBackground(Qt::white);

        //вывод на экран истории сообщений
        QList<MesData> *mesList = dmessages[ui->lwUsers->selectedItems().first()->text()];
        QList<MesData>::const_iterator iter;

        //итерируем список соответсвующий первому выделенному пользователю
        for(iter = mesList->constBegin(); iter != mesList->constEnd(); ++iter){

            //выводим сообщение
            ui->lwLog->addItem(iter->getTime().toString() + " [" + iter->getSender() + "]: " + iter->getText());

            //задаём его цвет
            ui->lwLog->item(ui->lwLog->count()-1)->setForeground(iter->getColor());

            //и фон, если оно ещё не доставлено
            if(!iter->isDelivered()){
                ui->lwLog->item(ui->lwLog->count()-1)->setBackground(Qt::cyan);
            }
        }

        //фокусируемся на последнем сообщении
        ui->lwLog->setCurrentItem(ui->lwLog->item(ui->lwLog->count() - 1));
    }
}
