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


/**
* @brief this is the application main entry point
*
*/

int main(int argc, char *argv[])
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    RTLSDisplayApplication app(argc, argv);

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
