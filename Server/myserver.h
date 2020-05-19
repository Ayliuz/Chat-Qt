#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QDebug>
#include "client.h"
class Client;

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit MyServer(QObject *parent = 0);
    ~MyServer();
    bool startServer(QHostAddress addr, qint16 port);
    void sendToAllUserJoin(QString name);
    void sendToAllUserLeft(QString name);
    void sendToAllMessage(QString message, QString fromUsername);
    void sendToAllServerMessage(QString message);
    void sendToUsersServerMessage(QString message, const QStringList &users);
    void sendMessageToUsers(QString message, const QStringList &users, QString fromUsername);
    QStringList getUsersOnline() const;
    bool isNameValid(QString name) const;
    bool isNameUsed(QString name) const;

signals:
    void sigRemoveUserFromList(QString name);
    void sigAddUserToList(QString name);
    void sigMessageToLog(QString message, QString from, const QStringList &users);

public slots:
    void slotMessageFromServer(QString message, const QStringList &users);
    void slotUserDisconnect(Client *client);
    void slotServerStop();

protected:
    void incomingConnection(qintptr handle);

private:
    QVector<Client *> clients;

};


#endif // MYSERVER_H
