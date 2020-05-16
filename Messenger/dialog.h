#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include <QHostAddress>
#include "../Server/client.h"      //need for constants
#include <QMessageBox>
class Client;

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
    void on_cbToAll_clicked();
    void on_pbSend_clicked();

    void on_lwUsers_itemSelectionChanged();
    
private:
    Ui::Dialog *ui;
    QTcpSocket *socket;
    quint16 blockSize;
    QString myName;
    bool connected;
    void addToLog(QString text, QColor color = Qt::black);

};

#endif // DIALOG_H
