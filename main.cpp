#include <QGuiApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>
#include "canvasengine.h"

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

    QCommandLineOption fullSpeedOption(QStringList() << "f"
                                       << "fullspeed", "Full speed painting.");
    parser.addOption(fullSpeedOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    bool fullspeed = parser.isSet(fullSpeedOption);

    QSize canvasSize(args.at(0).toInt(),
                     args.at(1).toInt());
    QFile input(args.at(2));
    if (!input.open(QIODevice::ReadOnly))
        return -1;

    QFile output(args.at(3));
    if (!output.open(QIODevice::WriteOnly))
        return -1;

    CanvasEngine engine(canvasSize);
    engine.setFullspeed(fullspeed);
    engine.setOutput(output);
    engine.setInput(input);
    CanvasEngine::connect(&engine, &CanvasEngine::parseEnded,
                          qApp, &QCoreApplication::quit);

    return app.exec();
}
