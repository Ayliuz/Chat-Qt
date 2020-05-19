#ifndef MESDATA_H
#define MESDATA_H

#include <QDebug>
#include <QtGui>
#include <QRegExp>

class MesData
{
public:

    static const bool PUBLIC = true;
    static const bool PRIVATE = false;

public:
    explicit MesData(const QString& in_sender, const QString& in_target, const QString& in_text, QTime in_time, QColor in_color);
    ~MesData();
    QTime getTime() const {return time;}
    QString getText() const {return text;}
    QString getSender() const {return sender;}
    QString getTarget() const {return target;}
    QColor getColor() const {return color;}


private:
    QTime time;
    QString text;
    QString sender;
    QString target;
    QColor color;
};

#endif // MESDATA_H
