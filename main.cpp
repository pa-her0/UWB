// -------------------------------------------------------------------------------------------------------------------
//
//  File: main.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#include "RTLSDisplayApplication.h"
#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QStyleFactory>
#include <QFile>
#include <QDesktopWidget>
#include <QCommandLineParser>
#include <QDebug>

/**
* @brief this is the application main entry point
*
*/

int main(int argc, char *argv[])
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    RTLSDisplayApplication app(argc, argv);

    // Parse command line arguments for WebSocket configuration
    QCommandLineParser parser;
    parser.setApplicationDescription("HR-RTLS PC Application with WebSocket data forwarding");
    parser.addHelpOption();

    QCommandLineOption wsUrlOption(QStringList() << "w" << "ws-url",
        "WebSocket server URL (e.g., ws://192.168.1.100:8080/rtls)",
        "url");
    parser.addOption(wsUrlOption);

    QCommandLineOption wsAutoConnectOption(QStringList() << "a" << "ws-auto",
        "Auto-connect to WebSocket on startup");
    parser.addOption(wsAutoConnectOption);

    parser.process(app);

    // Check environment variable as fallback
    QString wsUrl;
    if (parser.isSet(wsUrlOption)) {
        wsUrl = parser.value(wsUrlOption);
    } else {
        wsUrl = QString::fromLocal8Bit(qgetenv("RTLS_WS_URL"));
    }

    if (!wsUrl.isEmpty()) {
        qDebug() << "[Main] WebSocket URL from command line/env:" << wsUrl;
        app.webSocketClient()->connectToServer(wsUrl);
    } else if (parser.isSet(wsAutoConnectOption)) {
        qDebug() << "[Main] --ws-auto set but no URL provided, using default ws://localhost:8080/rtls";
        app.webSocketClient()->connectToServer("ws://localhost:8080/rtls");
    }

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QApplication::setStyle(QStyleFactory::create("fusion"));

    QFile file(":/qss/Ubuntu.qss");
    if( file.open(QFile::ReadOnly))
    {
       QString styleSheet = QLatin1String(file.readAll());

       app.setStyleSheet(styleSheet);
       file.close();
    }


    //app.mainWindow()->setWindowFlags(app.mainWindow()->windowFlags()& ~Qt::WindowMaximizeButtonHint);
    //app.mainWindow()->setFixedSize(app.mainWindow()->width(), app.mainWindow()->height());
    app.mainWindow()->setWindowState(Qt::WindowMaximized);
    //app.mainWindow()->setFixedSize(QApplication::desktop()->width(),QApplication::desktop()->height());




    app.mainWindow()->show();

    return app.QApplication::exec();
}
