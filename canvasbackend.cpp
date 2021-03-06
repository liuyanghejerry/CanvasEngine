#include "canvasbackend.h"
#include "misc/singleton.h"

#include <QTimerEvent>
#include <QDateTime>
#include <QJsonDocument>
#include <QDebug>

CanvasBackend::CanvasBackend(QObject *parent)
    :QObject(parent),
      parse_timer_id_(0),
      archive_loaded_(false),
      is_parsed_signal_sent(false),
      pause_(false),
      fullspeed_replay(false)
{
    connect(&raw_parser_, &PackParser::docGenerated,
            [this](QJsonDocument doc){
        this->onIncomingData(doc.object());
    });
    parse_timer_id_ = this->startTimer(10);
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

void CanvasBackend::setInput(QIODevice &device)
{
    raw_parser_.setDevice(device);
    connect(&raw_parser_, &PackParser::parseDone,
            [this]() {
        archive_loaded_ = true;
    });
}

void CanvasBackend::setFullspeed(bool full)
{
    fullspeed_replay = full;
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
    do{
        auto dataBlock = [this](const QVariantMap& m){
            QString clientid(m["clientid"].toString());
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

        if(incoming_store_.length()){
            auto obj = incoming_store_.dequeue();
            QString action = obj.value("action").toString().toLower();
            if(action == "block"){
                dataBlock(obj.toVariantMap());
                emit blockParsed();
            }
        }

    } while(fullspeed_replay && !pause_ && incoming_store_.length());

    if(archive_loaded_ && !is_parsed_signal_sent){
        emit archiveParsed();
        this->killTimer(parse_timer_id_);
        is_parsed_signal_sent = true;
        parse_timer_id_ = 0;
    }
}

void CanvasBackend::timerEvent(QTimerEvent * event)
{
    if(event->timerId() == parse_timer_id_ && !pause_){
        this->parseIncoming();
    }
}

QByteArray CanvasBackend::toJson(const QVariant &m)
{
    return QJsonDocument::fromVariant(m).toJson(QJsonDocument::Compact);
}

QVariant CanvasBackend::fromJson(const QByteArray &d)
{
    return QJsonDocument::fromJson(d).toVariant();
}
