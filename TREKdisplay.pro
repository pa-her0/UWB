#-------------------------------------------------
#
# Project created by QtCreator 2014-05-28T11:50:00
#
#-------------------------------------------------
cache()

QT       += core gui network xml serialport quickwidgets
QT       += network
QT       += core gui charts
QT       += datavisualization
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
TARGET = HR-RTLS_PC
TEMPLATE = app
QMAKE_INFO_PLIST = Info.plist

INCLUDEPATH += models network views util tools

INCLUDEPATH += $$PWD/armadillo-3.930.0/include
INCLUDEPATH += $$PWD/eigen


LIBS += -L$$PWD/armadillo-3.930.0/lib/ -lblas_win32_MT

LIBS += -L$$PWD/armadillo-3.930.0/lib/ -llapack_win32_MT

SOURCES += main.cpp\
    RTLSDisplayApplication.cpp \
    tools/Custom3DScatter.cpp \
    views/mainwindow.cpp \
    network/RTLSClient.cpp \
    views/GraphicsView.cpp \
    views/GraphicsWidget.cpp \
    views/ViewSettingsWidget.cpp \
    views/MinimapView.cpp \
    views/connectionwidget.cpp \
    models/ViewSettings.cpp \
    models/InfluxConfig.cpp \
    models/TrajectoryPoint.cpp \
    tools/OriginTool.cpp \
    tools/RubberBandTool.cpp \
    tools/ScaleTool.cpp \
    util/QPropertyModel.cpp \
    network/SerialConnection.cpp \
    network/InfluxQueryService.cpp \
    network/InfluxWriteService.cpp \
    network/DockerManager.cpp \
    tools/trilateration.cpp \
    views/UwbDataVizWidget.cpp \
    views/UwbTrajectoryView.cpp


HEADERS  += \
    RTLSDisplayApplication.h \
    tools/Custom3DScatter.h \
    views/mainwindow.h \
    network/RTLSClient.h \
    views/GraphicsView.h \
    views/GraphicsWidget.h \
    views/ViewSettingsWidget.h \
    views/MinimapView.h \
    views/connectionwidget.h \
    models/ViewSettings.h \
    models/InfluxConfig.h \
    models/TrajectoryPoint.h \
    tools/AbstractTool.h \
    tools/OriginTool.h \
    tools/RubberBandTool.h \
    tools/ScaleTool.h \
    util/QPropertyModel.h \
    network/SerialConnection.h \
    network/InfluxQueryService.h \
    network/InfluxWriteService.h \
    network/DockerManager.h \
    tools/trilateration.h \
    views/UwbDataVizWidget.h \
    views/UwbTrajectoryView.h

FORMS    += \
    views/mainwindow.ui \
    views/GraphicsWidget.ui \
    views/ViewSettingsWidget.ui \
    views/connectionwidget.ui \
    views/UwbDataVizWidget.ui


RESOURCES += \
    res/lanague.qrc \
    res/resources.qrc
TARGET = lanague
TRANSLATIONS = lanague_cn.ts\
               lanague_en.ts

RC_FILE =logo.rc

include ($$PWD/gsl/gsl.pri)
