#ifndef MYCLIENT_H
#define MYCLIENT_H

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
#include <QLineEdit>
#include <QPushButton>

class MyClient : public QWidget {
    Q_OBJECT

private:
    QTcpSocket *m_pTcpSocket; //Для управления нашим клиентом
    QTextEdit *m_ptxtInfo; //Для отображения информации
    QLineEdit *m_ptxtInput; //Для вывода информации
    quint16 m_nNextBlockSize; //Хранения длины следующего блока

public:
    MyClient(const QString& strHost, int nPort, QWidget *pwgt = 0);

private slots:
    //Слоты для реализации сигнально-слотовой связи
    void slotReadyRead();
    void slotError(QAbstractSocket::SocketError);
    void slotSendToServer();
    void slotConnected();
};

#endif // MYCLIENT_H
