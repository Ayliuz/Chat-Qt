/********************************************************************************
** Form generated from reading UI file 'myserver.ui'
**
** Created by: Qt User Interface Compiler version 5.9.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MYSERVER_H
#define UI_MYSERVER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MyServer
{
public:

    void setupUi(QWidget *MyServer)
    {
        if (MyServer->objectName().isEmpty())
            MyServer->setObjectName(QStringLiteral("MyServer"));
        MyServer->resize(400, 300);

        retranslateUi(MyServer);

        QMetaObject::connectSlotsByName(MyServer);
    } // setupUi

    void retranslateUi(QWidget *MyServer)
    {
        MyServer->setWindowTitle(QApplication::translate("MyServer", "MyServer", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class MyServer: public Ui_MyServer {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MYSERVER_H
