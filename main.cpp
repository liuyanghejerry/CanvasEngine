#include <QGuiApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>
#include "canvasengine.h"
#include "encoder/encoder.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("paintty painting machine");
    parser.addHelpOption();
    parser.addPositionalArgument("width",
                                 QCoreApplication::translate("main", "Canvas width."));
    parser.addPositionalArgument("height",
                                 QCoreApplication::translate("main", "Canvas height."));
    parser.addPositionalArgument("source",
                                 QCoreApplication::translate("main", "Source file to copy."));
    parser.addPositionalArgument("destination",
                                 QCoreApplication::translate("main", "Destination directory."));
    parser.addPositionalArgument("config",
                                 QCoreApplication::translate("main", "Video encoding config file."));

    QCommandLineOption fullSpeedOption(QStringList() << "f"
                                       << "fullspeed", "Full speed painting.");
    parser.addOption(fullSpeedOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if(args.length() < 4) {
        parser.showHelp(0);
        return 0;
    }

    bool fullspeed = parser.isSet(fullSpeedOption);

    QSize canvasSize(args.at(0).toInt(),
                     args.at(1).toInt());
    QFile input(args.at(2));
    if (!input.open(QIODevice::ReadOnly)) {
        qDebug()<<"Lack of input file";
        return -1;
    }

    QFile output(args.at(3));
    if (!output.open(QIODevice::WriteOnly)) {
        qDebug()<<"Output file unknown";
        return -1;
    }

    QFile configFile(args.at(4));
    if (!configFile.open(QIODevice::ReadOnly)) {
        qDebug()<<"Video encoding config file unknown";
        return -1;
    }
    QString config = QString::fromUtf8(configFile.readAll());
    configFile.close();

    Encoder *encoder = new Encoder(canvasSize, "output.mkv", config);

    CanvasEngine *engine = new CanvasEngine(canvasSize);
    engine->setFullspeed(fullspeed);
    engine->setOutput(output);
    engine->setInput(input);
    CanvasEngine::connect(engine, &CanvasEngine::canvasUpdated,
                          [engine, encoder]() {
        static quint64 times = 0;
        if(times % 20 == 0) {
            encoder->onImage(engine->allCanvas());
        }
        times++;
    });
    CanvasEngine::connect(engine, &CanvasEngine::parseEnded,
                          [encoder, engine](){
        encoder->finish();
        delete encoder;
        engine->deleteLater();
        qApp->quit();
    });

    return app.exec();
}
