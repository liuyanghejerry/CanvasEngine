#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QBuffer>
#include "packparser.h"


PackParser::PackParser(QObject *parent) :
    QObject(parent),
    device_(nullptr),
    pack_size(0),
    last_pack_unfinished(false)
{
    connect(this, &PackParser::newRawPack,
            this, &PackParser::onRawPack);
    connect(this, &PackParser::newPack,
            this, &PackParser::parseContent);
}


void PackParser::onRawData(const QByteArray &rawbytes)
{
    qDebug()<<"archive:"<<rawbytes.length()<<"bytes";
    QBuffer* buf = new QBuffer(this);
    buf->setData(rawbytes);
    buf->open(QBuffer::ReadWrite);
    buf->seek(0);
    device_ = buf;
    processRead();
}

// take binary data and pack it to a TCP pack
QByteArray PackParser::packRaw(const QByteArray &content)
{
    auto result = content;
    quint32 length = content.length();
    uchar c1, c2, c3, c4;
    c1 = length & 0xFF;
    length >>= 8;
    c2 = length & 0xFF;
    length >>= 8;
    c3 = length & 0xFF;
    length >>= 8;
    c4 = length & 0xFF;

    result.prepend(c1);
    result.prepend(c2);
    result.prepend(c3);
    result.prepend(c4);

    return result;
}

// unused
//QByteArray PackParser::unpackRaw(const QByteArray &content)
//{
//    //
//}

void PackParser::processRead()
{
    if(!device_){
        return;
    }
    if(last_pack_unfinished){
        if(device_->bytesAvailable() >= pack_size){
            QByteArray info = device_->read(pack_size);
            last_pack_unfinished = false;
            emit newRawPack(info);
        }else{
            if(device_->atEnd()){
                emit parseDone();
            } else {
                emit drain();
            }
            return;
        }
    }

    if(device_->bytesAvailable() < 4){
        if(device_->atEnd()){
            emit parseDone();
        } else {
            emit drain();
        }
        return;
    }

    // in case we have a lot of data to process
    //    QCoreApplication::processEvents();
    char c1, c2, c3, c4;
    device_->getChar(&c1);
    device_->getChar(&c2);
    device_->getChar(&c3);
    device_->getChar(&c4);
    pack_size = (uchar(c1) << 24) + (uchar(c2) << 16)
            + (uchar(c3) << 8) + uchar(c4);
    last_pack_unfinished = true;

    this->metaObject()->invokeMethod(this, "processRead", Qt::QueuedConnection);
}

void PackParser::onRawPack(const QByteArray &rawpack)
{
    bool isCompressed = rawpack[0] & 0x1;
    PACK_TYPE pack_type = PACK_TYPE((rawpack[0] & binL<110>::value) >> 0x1);
    QByteArray data_without_header = rawpack.right(rawpack.length()-1);
    if(isCompressed){
        QByteArray tmp = qUncompress(data_without_header);
        if(tmp.isEmpty()){
            qWarning()<<"bad input"<<data_without_header.toHex();
            return;
        }
        emit newPack(ParserResult(pack_type, tmp));
    }else{
        emit newPack(ParserResult(pack_type, data_without_header));
    }
}

void PackParser::parseContent(const PackParser::ParserResult &result)
{
    if(result.pack_type != PACK_TYPE::DATA) {
        return;
    }

    auto doc = QJsonDocument::fromJson(result.pack_data);
    emit docGenerated(doc);
}

void PackParser::setDevice(QIODevice &device)
{
    device_ = &device;
    connect(device_, &QIODevice::readyRead,
            this, &PackParser::processRead);
    device_->seek(0);
    this->processRead();
}

QByteArray PackParser::assamblePack(bool compress,
                                    PACK_TYPE pt,
                                    const QByteArray& bytes)
{
    auto body = bytes;
    if(compress){
        body = qCompress(bytes);
    }
    QByteArray result;
    char header = (compress & 0x1) | (pt << 0x1);
    result.append(header);
    result.append(body);
    return result;
}
