#include "mesdata.h"

MesData::MesData(const QString& in_sender, const QString& in_target, const QString& in_text, QTime in_time, QColor in_color = Qt::black) {
    sender = in_sender;
    target = in_target;
    text = in_text;
    time = in_time;
    color = in_color;
}

MesData:: ~MesData(){

}
