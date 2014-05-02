#ifndef CANVASENGINE_H
#define CANVASENGINE_H

#include <QObject>
#include "brush/abstractbrush.h"
#include "misc/layermanager.h"
#include "canvasbackend.h"

typedef QSharedPointer<AbstractBrush> BrushPointer;

class CanvasEngine : public QObject
{
    Q_OBJECT
public:
    explicit CanvasEngine(QObject *parent = 0);
    ~CanvasEngine();
    QVariantMap brushSettings() const;
    BrushFeature brushFeatures() const;
    QString currentLayer();
    int count() const{return layers.count();}
    int layerNum() const{return layerNameCounter;}
    QImage currentCanvas();
    QImage allCanvas();
public slots:
    void addLayer(const QString &name);
    bool deleteLayer(const QString &name);
//    void loadLayers();
//    void saveLayers();
    void pause();

signals:
    void newPaintAction(const QVariantMap m);
    void canvasExported(const QPixmap& pic);
    void parsePaused();
private slots:
    void remoteDrawPoint(const QPoint &point,
                         const QVariantMap &brushSettings,
                         const QString &layer,
                         const QString clientid,
                         const qreal pressure=1.0);
    void remoteDrawLine(const QPoint &start,
                        const QPoint &end,
                        const QVariantMap &brushSettings,
                        const QString &layer,
                        const QString clientid,
                        const qreal pressure=1.0);

private:
    void drawLineTo(const QPoint &endPoint, qreal pressure=1.0);
    void drawPoint(const QPoint &point, qreal pressure=1.0);
    void storeAction(const QVariantMap& map);
    void sendAction();
    BrushPointer brushFactory(const QString &name);
    void setBrushFeature(const QString& key, const QVariant& value);

    QSize canvasSize;
    LayerManager layers;
    QImage image;
    int layerNameCounter;
    QHash<QString, BrushPointer> remoteBrush;
    CanvasBackend* backend_;
    QThread *worker_;
};





#endif // CANVASENGINE_H
