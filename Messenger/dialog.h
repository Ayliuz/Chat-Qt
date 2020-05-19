#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include <QDebug>
#include <QHostAddress>
#include <QEvent>
#include <QKeyEvent>
#include "../Server/client.h"      //need for constants
#include <QMessageBox>
#include "mesdata.h"
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
    void slotSocketConnected();
    void slotSocketDisconnected();
    void slotSocketReadyRead();
    void slotSocketDisplayError(QAbstractSocket::SocketError socketError);

    void on_pbConnect_clicked();
    void on_pbSend_clicked();
    void on_lwUsers_itemSelectionChanged();


signals:
    void sigShiftEnterPushed();

private:
    Ui::Dialog *ui;
    QTcpSocket *socket;
    quint16 blockSize;
    QString myName;
    QMap<QString, QList<MesData> *> dmessages;
    QList<MesData> *log;

    bool connected;
    void addToLog(QString text, QColor color = Qt::black); //сообщения сервера
    void updateUserList(QStringList users); //обновление списка пользоваьтелей(Server и All всегда существуют)
    void insertMessage(QString userName, QString sender, QString target, QString text, QTime time, QColor color = Qt::black); //добавление сообщения в историю
    virtual bool eventFilter(QObject * target, QEvent *event);
};





#endif // DIALOG_H
