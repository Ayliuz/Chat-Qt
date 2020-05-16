#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QDebug>
#include <QtGui>
#include <QtCore>
#include "myserver.h"

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;
    MyServer *server;
    void addToLog(QString text, QColor color);

signals:
    void sigMessageFromServer(QString message, const QStringList &users);
    void sigServerStop();

public slots:
    void slotAddUser(QString name);
    void slotRemoveUser(QString name);
    void slotNewMessage(QString message, QString from, const QStringList &users);

private slots:
    void on_pbStartStop_clicked();
    void on_pbSend_clicked();
    void on_lwUsers_itemSelectionChanged();
};

#endif // DIALOG_H
