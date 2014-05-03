#include "canvasengine.h"

#include <QPainter>
#include <QHash>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDir>
#include <QDebug>
#include <QtCore/qmath.h>

#include "brush/brushmanager.h"
#include "brush/basicbrush.h"
#include "brush/sketchbrush.h"
#include "brush/basiceraser.h"
#include "brush/binarybrush.h"
#include "brush/waterbased.h"
#include "brush/maskbased.h"
#include "misc/singleton.h"

template<typename T>
void loadBrush_sub_impl()
{
    auto& brush_manager = Singleton<BrushManager>::instance();
    BrushPointer p1(new T);
    p1->setSettings(p1->defaultSettings());
    brush_manager.addBrush(p1);
    qDebug()<<p1->name()<<"loaded";
}

template<typename T, typename... Targs>
void loadBrush_sub()
{
    loadBrush_sub_impl<T>();
    loadBrush_sub<Targs...>();
}

template<>
void loadBrush_sub<void>()
{
}

CanvasEngine::CanvasEngine(const QSize size, QObject *parent) :
    QObject(parent),
    canvasSize(size),
    layers(canvasSize),
    image(canvasSize, QImage::Format_ARGB32_Premultiplied),
    layerNameCounter(0),
    backend_(new CanvasBackend(0)),
    worker_(new QThread(this)),
    output_(nullptr)
{
    loadBrush();

    worker_->start();
    backend_->moveToThread(worker_);
    connect(backend_, &CanvasBackend::remoteDrawLine,
            this, &CanvasEngine::remoteDrawLine);
    connect(backend_, &CanvasBackend::remoteDrawPoint,
            this, &CanvasEngine::remoteDrawPoint);
    connect(worker_, &QThread::finished,
            backend_, &CanvasBackend::deleteLater);
    connect(this, &CanvasEngine::parsePaused,
            backend_, &CanvasBackend::pauseParse);
    connect(backend_, &CanvasBackend::archiveParsed,
            [this](){
        qDebug()<<"archiveParsed";
        if(output_){
            this->allCanvas().save(output_, "png");
            output_->close();
        }
        emit parseEnded();
    });

    for(int i=0;i<10;++i) {
        addLayer(QString::number(layerNum()));
    }
}


CanvasEngine::~CanvasEngine()
{
    pause();
    if(worker_){
        worker_->quit();
        worker_->wait();
    }
    this->disconnect();
}

QImage CanvasEngine::allCanvas()
{
    QImage exp(canvasSize, QImage::Format_ARGB32_Premultiplied);
    exp.fill(Qt::white);
    QPainter painter(&exp);
    int count = layers.count();
    QImage * im = 0;
    for(int i=0;i<count;++i){
        LayerPointer l = layers.layerFrom(i);
        im = l->imagePtr();
        painter.drawImage(0, 0, *im);
    }
    return exp;
}

BrushPointer CanvasEngine::brushFactory(const QString &name)
{
    return Singleton<BrushManager>::instance().makeBrush(name);
}

void CanvasEngine::loadBrush()
{
    loadBrush_sub<BasicBrush, BinaryBrush, SketchBrush, BasicEraser, MaskBased, void>();
}
bool CanvasEngine::fullspeed() const
{
    return fullspeed_;
}

void CanvasEngine::setFullspeed(bool fullspeed)
{
    fullspeed_ = fullspeed;
    this->metaObject()->invokeMethod(this->backend_,
                                   "setFullspeed",
                                   Qt::AutoConnection,
                                   Q_ARG(bool, fullspeed_));
}


//void CanvasEngine::loadLayers()
//{
//    // TODO: merge this feature into ArchiveFile, maybe?
//    QString dir_name = Singleton<ArchiveFile>::instance().dirName();
//    for(int i=layers.count()-1;i>=0;--i){
//        QString img_name = QString("%1/%2.png").arg(dir_name).arg(i);
//        QImage img(img_name);
//        if(img.isNull()){
//            continue;
//        }
//        QPainter painter(layers.layerFrom(i)->imagePtr());
//        painter.drawImage(0, 0, img);
//    }
//    //    connect(&Singleton<ArchiveFile>::instance(), &ArchiveFile::newSignature,
//    //            [this](){
//    //        this->clearAllLayer();
//    //    });
//    connect(&Singleton<ArchiveFile>::instance(), &ArchiveFile::newSignature,
//            this, &CanvasEngine::clearAllLayer);
//}

//void CanvasEngine::saveLayers()
//{
//    // TODO: merge this feature into ArchiveFile, maybe?
//    QString dir_name = Singleton<ArchiveFile>::instance().dirName();
//    QDir::current().mkpath(dir_name);
//    for(int i=layers.count()-1;i>=0;--i){
//        if(!layers.layerFrom(i)->isTouched()){
//            continue;
//        }
//        QString img_name = QString("%1/%2.png").arg(dir_name).arg(i);
//        layers.layerFrom(i)->imageConstPtr()->save(img_name);
//    }
//}

void CanvasEngine::pause()
{
    emit parsePaused();
}

void CanvasEngine::setInput(QIODevice &device)
{
    this->backend_->setInput(device);
}

void CanvasEngine::setOutput(QIODevice &device)
{
    output_ = &device;
}

void CanvasEngine::remoteDrawPoint(const QPoint &point,
                                   const QVariantMap &brushInfo,
                                   const QString &layer,
                                   const QString clientid,
                                   const qreal pressure)
{
    if(!layers.exists(layer)) return;
    LayerPointer l = layers.layerFrom(layer);

    QVariantMap cpd_brushInfo = brushInfo;
    QString brushName = cpd_brushInfo["name"].toString().toLower();

    cpd_brushInfo.remove("name"); // remove useless info

    if(remoteBrush.contains(clientid)){
        BrushPointer t = remoteBrush[clientid];
        if(brushName != t->name().toLower()){
            BrushPointer newOne = brushFactory(brushName);
            newOne->setSurface(l);
            newOne->setSettings(cpd_brushInfo);
            newOne->drawPoint(point, pressure);
            remoteBrush[clientid] = newOne;
            //            t.clear();
        }else{
            BrushPointer original = remoteBrush[clientid];
            original->setSurface(l);
            original->setSettings(cpd_brushInfo);
            original->drawPoint(point, pressure);
        }
    }else{
        BrushPointer newOne = brushFactory(brushName);
        newOne->setSurface(l);
        newOne->setSettings(cpd_brushInfo);
        newOne->drawPoint(point, pressure);
        remoteBrush[clientid] = newOne;
    }
}


void CanvasEngine::remoteDrawLine(const QPoint &, const QPoint &end,
                                  const QVariantMap &brushInfo,
                                  const QString &layer,
                                  const QString clientid,
                                  const qreal pressure)
{
    if(!layers.exists(layer)){
        return;
    }
    LayerPointer l = layers.layerFrom(layer);

    QVariantMap cpd_brushInfo = brushInfo;
    QString brushName = cpd_brushInfo["name"].toString().toLower();

    if(remoteBrush.contains(clientid)){
        BrushPointer t = remoteBrush[clientid];
        if(brushName != t->name().toLower()){
            BrushPointer newOne = brushFactory(brushName);
            newOne->setSurface(l);
            newOne->setSettings(cpd_brushInfo);
            newOne->drawLineTo(end, pressure);
            remoteBrush[clientid] = newOne;
            //            t.clear();
        }else{
            BrushPointer original = remoteBrush[clientid];
            original->setSurface(l);
            original->setSettings(cpd_brushInfo);
            original->drawLineTo(end, pressure);
        }
    }else{
        BrushPointer newOne = brushFactory(brushName);
        newOne->setSurface(l);
        newOne->setSettings(cpd_brushInfo);
        qDebug()<<"warning, remote drawing starts with line drawing";
        newOne->drawLineTo(end, pressure);
        remoteBrush[clientid] = newOne;
    }
}


/* Layer */

QString CanvasEngine::currentLayer()
{
    return layers.selectedLayer()->name();
}

void CanvasEngine::addLayer(const QString &name)
{
    layers.appendLayer(name);
    layerNameCounter++;
}


bool CanvasEngine::deleteLayer(const QString &name)
{
    if(layers.layerFrom(name)->isLocked())
        return false;

    layers.removeLayer(name);
    return true;
}
