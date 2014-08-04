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
    explicit CanvasEngine(const QSize size = QSize(2880, 1920), QObject *parent = 0);
    ~CanvasEngine();
    QVariantMap brushSettings() const;
    BrushFeature brushFeatures() const;
    QString currentLayer();
    int count() const{return layers.count();}
    int layerNum() const{return layerNameCounter;}
    QImage allCanvas();
    bool fullspeed() const;

public slots:
    void addLayer(const QString &name);
    bool deleteLayer(const QString &name);
    //    void loadLayers();
    //    void saveLayers();
    void pause();
    void setInput(QIODevice& device);
    void setOutput(QIODevice& device);
    void setFullspeed(bool fullspeed);

signals:
    void parsePaused();
    void parseEnded();
    void canvasUpdated();
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
    BrushPointer brushFactory(const QString &name);
    void loadBrush();

    QSize canvasSize;
    LayerManager layers;
    QImage image;
    int layerNameCounter;
    QHash<QString, BrushPointer> remoteBrush;
    CanvasBackend* backend_;
    QThread *worker_;
    QIODevice *output_;
    bool fullspeed_;
};





#endif // CANVASENGINE_H
