#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QDebug>
#include <QTcpSocket>
#include <QThreadPool>
#include <QtGui>
#include <QRegExp>
#include "myserver.h"
class MyServer;

class Client : public QObject
{
    friend class MyServer;
    Q_OBJECT

public:
    static const QString constNameUnknown;
    static const quint8 comAutchReq = 1;
    static const quint8 comUsersOnline = 2;
    static const quint8 comUserJoin = 3;
    static const quint8 comUserLeft = 4;
    static const quint8 comMessageToAll = 5;
    static const quint8 comMessageToUsers = 6;
    static const quint8 comPublicServerMessage = 7;
    static const quint8 comPrivateServerMessage = 8;
    static const quint8 comAutchSuccess = 9;
    static const quint8 comErrNameInvalid = 201;
    static const quint8 comErrNameUsed = 202;

    explicit Client(int desc, MyServer *server, QObject *parent = 0);
    ~Client();
    void setName(QString name) {myName = name;}
    QString getName() const {return myName;}
    bool getAuthed() const {return isAuthed;}
    void sendCommand(quint8 comm) const;
    void sendUsersOnline() const;

signals:
    void sigRemoveUser(Client *client);

private slots:
    void slotConnect();
    void slotDisconnect();
    void slotReadyRead();
    void slotError(QAbstractSocket::SocketError socketError) const;

private:
    QTcpSocket *socket;
    MyServer *server;
    quint16 blockSize;
    QString myName;
    bool isAuthed;

};

#endif // CLIENT_H
