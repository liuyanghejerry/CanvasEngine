#-------------------------------------------------
#
# Project created by QtCreator 2014-05-02T23:32:53
#
#-------------------------------------------------

QT       += core gui

#QT       -= gui

TARGET = CanvasEngine
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += c++11

TEMPLATE = app


SOURCES += main.cpp \
    canvasengine.cpp \
    misc/layer.cpp \
    misc/layermanager.cpp \
    brush/abstractbrush.cpp \
    brush/basicbrush.cpp \
    brush/basiceraser.cpp \
    brush/binarybrush.cpp \
    brush/brushfeature.cpp \
    brush/brushmanager.cpp \
    brush/maskbased.cpp \
    brush/sketchbrush.cpp \
    brush/waterbased.cpp \
    canvasbackend.cpp

HEADERS += \
    canvasengine.h \
    misc/call_once.h \
    misc/layer.h \
    misc/layermanager.h \
    misc/singleton.h \
    brush/abstractbrush.h \
    brush/basicbrush.h \
    brush/basiceraser.h \
    brush/binarybrush.h \
    brush/brushfeature.h \
    brush/brushmanager.h \
    brush/brushsettings.h \
    brush/maskbased.h \
    brush/sketchbrush.h \
    brush/waterbased.h \
    canvasbackend.h
