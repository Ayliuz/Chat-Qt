#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include <QDebug>
#include <QHostAddress>
#include <QEvent>
#include <QKeyEvent>
#include "/home/elias/Documents/Chat/mServer/Server/client.h"      //need for constants
#include <QMessageBox>
#include "mesdata.h"
#include <qrsaencryption.h>
class Client;

class MesData;

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private slots:
    void slotSocketConnected();     //произошло подключение к серверу
    void slotSocketDisconnected();  //произошло отключение от сервера
    void slotSocketReadyRead(); //появилась новая инф-ция для чтения
    void slotSocketDisplayError(QAbstractSocket::SocketError socketError);  //ошибка подключения

    void on_pbConnect_clicked();    //подключение к серверу
    void on_pbSend_clicked();   // отправка сообщения
    void on_lwUsers_itemSelectionChanged(); //изменились выделенные пользователи


signals:
    void sigShiftEnterPushed();

private:
    Ui::Dialog *ui;  // интерфейс пользователя
    QTcpSocket *socket; //сокет для связи
    quint16 blockSize; // размер блока отправки сообщений
    QString myName; //собственное имя
    QMap<QString, QByteArray> KeyList;    //имена и публичные ключи шифрования пользователей
    QRSAEncryption  encrypt; // объект, задающий шифрование
    QByteArray pubKey, privKey; // наши ключи шифрования


    QMap<QString, QList<MesData> *> dmessages;  // история соообщений
    QList<MesData> *log; // лог подключений

    bool connected;
    void addToLog(QString text, QColor color = Qt::black); //сообщения сервера
    void updateScreen(); //обновление экрана
    void updateUserList(QMap<QString, QString> users); //обновление списка пользоваьтелей (All всегда существует) по пришедшему от сервера списку
    void insertMessage(QString userName, QString sender, QString target, QString text, QTime time, QColor color = Qt::black, bool delivered = true); //добавление сообщения в историю
    virtual bool eventFilter(QObject * target, QEvent *event);  //перехват Shift+Enter
};





#endif // DIALOG_H
