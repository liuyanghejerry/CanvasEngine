#ifndef PACKPARSER_H
#define PACKPARSER_H

#include <QObject>
#include <QBuffer>

#include "binary.h"

class PackParser : public QObject
{
    Q_OBJECT
public:
    enum PACK_TYPE : unsigned char {
        MANAGER = binL<0>::value,
        COMMAND = binL<1>::value,
        DATA = binL<10>::value,
        MESSAGE = binL<11>::value
    };
    //    typedef std::tuple<PACK_TYPE, QByteArray> ParserResult;

    struct ParserResult
    {
        ParserResult(PACK_TYPE t, const QByteArray& d):
            pack_type(t),
            pack_data(d)
        {
        }

        PACK_TYPE pack_type;
        QByteArray pack_data;

    };

    explicit PackParser(QObject *parent = 0);
    QByteArray assamblePack(bool compress,
                            PACK_TYPE pt,
                            const QByteArray& bytes);
    QByteArray packRaw(const QByteArray &content);
    
signals:
    void newRawPack(const QByteArray& rawpack);
    void newPack(const ParserResult& result);
    void docGenerated(const QJsonDocument&);
    void drain();
    void parseDone();
public slots:
    void onRawData(const QByteArray& rawbytes);
    void onRawPack(const QByteArray &rawpack);
    void parseContent(const ParserResult& result);
    void setDevice(QIODevice& device);

protected slots:
    void processRead();
private:
    QIODevice* device_;
    quint32 pack_size;
    bool last_pack_unfinished;

};

#endif // PACKPARSER_H
