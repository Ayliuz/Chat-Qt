#ifndef MYSERVER_H
#define MYSERVER_H

#include <QWidget>
#include <QTcpServer>
#include <QTextEdit>
#include <QTcpSocket>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include <QTime>
#include <QIODevice>


// =====================================================
class MyServer : public QWidget {
    Q_OBJECT
private:
    QTcpServer *m_ptcpServer; //основной атрибут для управления нашим сервером
    QTextEdit *m_ptxt; //Многострочное текстовое поле - предназначено для информирования о происходящих соединениях
    quint16 m_nNextBlockSize; //атрибут для хранения для хранения длины следующего блока, полученного от сокета

private:
    void sendToClient(QTcpSocket *pSocket, const QString& str);

public:
    MyServer(int nPort, QWidget *pwgt = 0);
    ~MyServer();

public slots:
    virtual void slotNewConnection();
            void slotReadClient();
};

#endif // MYSERVER_H
