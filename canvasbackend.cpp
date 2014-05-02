#include "canvasbackend.h"
#include "misc/singleton.h"

#include <QTimerEvent>
#include <QDateTime>
#include <QJsonDocument>

CanvasBackend::CanvasBackend(QObject *parent)
    :QObject(parent),
      parse_timer_id_(0),
      is_parsed_signal_sent(false),
      pause_(false),
      fullspeed_replay(false)
{
    parse_timer_id_ = this->startTimer(50);

    fullspeed_replay = true;

//    connect(&client_socket, &ClientSocket::dataPack,
//            this, &CanvasBackend::onIncomingData);
}

CanvasBackend::~CanvasBackend()
{
    this->disconnect();
    if(parse_timer_id_)
        killTimer(parse_timer_id_);
}

void CanvasBackend::pauseParse()
{
    pause_ = true;
}

void CanvasBackend::resumeParse()
{
    pause_ = false;
}

void CanvasBackend::onDataBlock(const QVariantMap info)
{
    QString author = info["name"].toString();
    QString clientid = info["clientid"].toString();

    auto data = toJson(QVariant(info));
    emit newDataGroup(data);
}

void CanvasBackend::onIncomingData(const QJsonObject& obj)
{
    incoming_store_.enqueue(obj);
    if(fullspeed_replay && !pause_){
        parseIncoming();
    }
}

void CanvasBackend::parseIncoming()
{
    auto dataBlock = [this](const QVariantMap& m){
        QString clientid(m["clientid"].toString());
        // don't draw your own move from remote
        if(clientid == cached_clientid_){
            return;
        }
        QVariantList list(m["block"].toList());
        if(list.length() < 1) {
            return;
        }

        QString layerName(m["layer"].toString());
        QVariantMap brushInfo(m["brush"].toMap());

        // parse first point as drawpoint
        QVariantMap first_set(list.takeFirst().toMap());
        QPoint point(first_set.value("x", 0).toInt(), first_set.value("y", 0).toInt());
        qreal pressure = 1.0;
        if(first_set.contains("pressure")) {
            pressure = first_set.value("pressure").toDouble();
        }

        emit remoteDrawPoint(point, brushInfo,
                             layerName, clientid,
                             pressure);

        // parse points as drawlines, with first point as start
        QPoint start_point(point);
        QPoint end_point;
        QVariantMap end;
        while(list.length()){
            end = list.takeFirst().toMap();
            end_point.setX(end.value("x").toInt());
            end_point.setY(end.value("y").toInt());
            qreal pressure = 1.0;
            if(end.contains("pressure")) {
                pressure = end.value("pressure").toDouble();
            }

            emit remoteDrawLine(start_point, end_point,
                                brushInfo, layerName,
                                clientid, pressure);
            start_point = end_point;
        }
    };

    static bool need_repaint = false;

    if(incoming_store_.length()){
        auto obj = incoming_store_.dequeue();
        QString action = obj.value("action").toString().toLower();
        if(action == "block"){
            dataBlock(obj.toVariantMap());
            if(fullspeed_replay){
                need_repaint = true;
            }else{
                emit repaintHint();
            }
        }
    }else{
        if(need_repaint){
            emit repaintHint();
        }
        if(archive_loaded_ && !is_parsed_signal_sent){
            emit archiveParsed();
            is_parsed_signal_sent = true;
        }
    }
}

void CanvasBackend::timerEvent(QTimerEvent * event)
{
    if(event->timerId() == parse_timer_id_){
        if(!pause_){
            this->parseIncoming();
        }
    }
}

QByteArray CanvasBackend::toJson(const QVariant &m)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0))
    return QJsonDocument::fromVariant(m).toJson(QJsonDocument::Compact);
#else
    return QJsonDocument::fromVariant(m).toJson();
#endif
}

QVariant CanvasBackend::fromJson(const QByteArray &d)
{
    return QJsonDocument::fromJson(d).toVariant();
}
