#include "myserver.h"

MyServer::MyServer(int nPort, QWidget *pwgt) : QWidget(pwgt), m_nNextBlockSize(0) {
    //Для запуска сервера нам необходимо вызвать в конструкторе метод listen() он принимает IP-адрес и порт
    //При возникновении ошибочных ситуаций, этот метод вернет false, на которые мы отреагируем и выведим окно
    //сообщения об ошибке
    m_ptcpServer = new QTcpServer(this);
    if(!m_ptcpServer -> listen(QHostAddress::Any, nPort)) {
        QMessageBox::critical(0, "Server Error", "Unable to start the server:" + m_ptcpServer -> errorString());
        m_ptcpServer -> close();
        return;
    }

    //Если ошибки нет, то мы соединяем слот с сигналом, который отправляется при каждом новом клиенте
    connect(m_ptcpServer, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));


    //Для отображения информации мы создаем виджет многострочного текстового поля и устанавливаем в нем режим
    //только просмотр информации
    m_ptxt = new QTextEdit;
    m_ptxt -> setReadOnly(true);

    //Layout setup
    QVBoxLayout* pvbxLayout = new QVBoxLayout;
    pvbxLayout -> addWidget(new QLabel("<H1>Server</H1>"));
    pvbxLayout -> addWidget(m_ptxt);
    setLayout(pvbxLayout);
}

MyServer::~MyServer(){}

//Метод slotNewConnection() вызывается каждый раз при соединении с новым клиентом
void MyServer::slotNewConnection() {
    //Для подтверждения соединения с клиентом необходимо вызывать метод nextPendingConnection()
    //который возвратит сокет, посредством которого можно осуществить дальнейшую связь с клиентом
    QTcpSocket *pClientSocket = m_ptcpServer -> nextPendingConnection();

    //Дальнейшая работа сокета осуществляется обеспечивается сигнально-слотовыми связями
    connect(pClientSocket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
    connect(pClientSocket, SIGNAL(readyRead()), this, SLOT(slotReadClient()));

    sendToClient(pClientSocket, "Server Response: Connected!");
}

void MyServer::slotReadClient() {
    //Необходимо выполнить преобразование указателя, возвращаемого методом sender() к типу QTcpSocket
    QTcpSocket *pClientSocket = (QTcpSocket*)sender();
    /*  Цикл for нужен! Потому что не все высланные клиентом данные могут прийти одновременно.
        Поэтому сервер должен уметть получать как весь блок целиком, так и часть блока, а также
        и все блоки сразу. Каждый переданный сокетом блок начинается полем размера блока. Размер блока считывается при условии, что
        размер полученных данных не меньше двух байт и атрибут m_nNextBlockSize равен нулю
        (т. е. размер блока неизвестен). Если размер доступных для чтения данных больше или равен размеру блока, тогда данные считываются из потока в переменные time и str. Затем
        значение переменной time преобразуется вызовом метода toString() в строку и вместе со
        строкой str записывается в строку сообщения strMessage, которая добавляется в виджет
        текстового поля вызовом метода append(). Анализ блока данных завершается присваиванием атрибуту m_nNextBlockSize значения 0, которое говорит о том, что размер очередного
        блока данных неизвестен. Вызовом метода sendToClient() мы сообщаем клиенту о том, что
        нам успешно удалось прочитать высланные им данные.
    */
    QDataStream in(pClientSocket);
    in.setVersion(QDataStream::Qt_4_5);

    for(;;) {
        if(!m_nNextBlockSize) {
            if(pClientSocket -> bytesAvailable() < sizeof(quint16)) {
                break;
            }
            in >> m_nNextBlockSize;
        }

        if(pClientSocket -> bytesAvailable() < m_nNextBlockSize) {
            break;
        }

        QTime time;
        QString str;
        in >> time >> str;

        QString strMessage = time.toString() + " " + "Client has sent - " + str;
        m_ptxt -> append(strMessage);

        m_nNextBlockSize = 0;

        sendToClient(pClientSocket, "Server Response \"" + str + "\"");
    }
}

//В методе sendToClient(...) мы формируем данные, которые будут отосланы клиенту
void MyServer::sendToClient(QTcpSocket *pSocket, const QString& str) {
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_5);
    out << quint16(0) << QTime::currentTime() << str;

    out.device() -> seek(0);
    out << quint16(arrBlock.size() - sizeof(quint16));

    pSocket -> write(arrBlock);
}
