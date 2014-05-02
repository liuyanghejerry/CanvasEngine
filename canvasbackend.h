#ifndef CANVASBACKEND_H
#define CANVASBACKEND_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QJsonObject>
#include <QVariantList>
#include <QByteArray>
#include <QPoint>

class CanvasBackend : public QObject
{
    Q_OBJECT
public:
    CanvasBackend(QObject *parent = nullptr);
    ~CanvasBackend();
public slots:
    void onDataBlock(const QVariantMap d);
    void onIncomingData(const QJsonObject &d);
    void pauseParse();
    void resumeParse();
signals:
    void newDataGroup(const QByteArray& d);
    void remoteDrawPoint(const QPoint &point,
                         const QVariantMap &brushInfo,
                         const QString &layer,
                         const QString clientid,
                         const qreal pressure=1.0);
    void remoteDrawLine(const QPoint &start,
                        const QPoint &end,
                        const QVariantMap &brushInfo,
                        const QString &layer,
                        const QString clientid,
                        const qreal pressure=1.0);
    void repaintHint();
    void archiveParsed();
protected:
    void timerEvent(QTimerEvent * event);
private:
    QQueue<QJsonObject> incoming_store_;
    // Warning, access memberHistory_ across thread
    // via member functions is not thread-safe
    QString cached_clientid_;
    int parse_timer_id_;
    bool archive_loaded_;
    bool is_parsed_signal_sent;
    bool pause_;
    bool fullspeed_replay;
    QByteArray toJson(const QVariant &m);
    QVariant fromJson(const QByteArray &d);
    void parseIncoming();
    void onArchiveLoaded();
};

#endif // CANVASBACKEND_H
