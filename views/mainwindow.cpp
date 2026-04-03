// -------------------------------------------------------------------------------------------------------------------
//
//  File: MainWindow.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "connectionwidget.h"
#include "serialconnection.h"
#include "RTLSDisplayApplication.h"
#include "ViewSettings.h"
#include "UwbDataVizWidget.h"

#include <QShortcut>
#include <QSettings>
#include <QDebug>
#include <QMessageBox>
#include <QDomDocument>
#include <QFile>

extern bool serial_connected;

bool language  = true;
bool startCalibration = false;  //[false: 未开始自标定] [true: 正在自标定]
QString BTN_sendSocketText= "停止";
QString LE_ip_addressText= "127.0.0.1";
QString ip_addressText= "127.0.0.1";
int LE_comTexttoInt= 6020;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _showMainToolBar = false;
    _notConnected = true;
    serial_connected = false;

    {
        QWidget *empty = new QWidget(this);
        empty->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        ui->mainToolBar->addWidget(empty);
    }

    _anchorConfigAction = new QAction(tr("&Connection Configuration"), this);
    //ui->menuBar->addAction(anchorConfigAction);
    //connect(anchorConfigAction, SIGNAL(triggered()), SLOT(onAnchorConfigAction()));

    createPopupMenu(ui->viewMenu);

    //add connection widget to the main window
    _cWidget = new ConnectionWidget(this);
    ui->mainToolBar->addWidget(_cWidget);

    QObject::connect(RTLSDisplayApplication::instance(), SIGNAL(aboutToQuit()), SLOT(saveSettings()));

    ui->helpMenu->addAction(ui->actionAbout);
    connect(ui->actionAbout, SIGNAL(triggered()), SLOT(onAboutAction()));

    _infoLabel = new QLabel(parent);

    ui->viewSettings_dw->close();
    ui->minimap_dw->close();
    ui->uwbDataViz_dw->close();

    connect(ui->minimap_dw->toggleViewAction(), SIGNAL(triggered()), SLOT(onMiniMapView()));

    QObject::connect(RTLSDisplayApplication::serialConnection(), SIGNAL(connectionStateChanged(SerialConnection::ConnectionState)),
                     this, SLOT(connectionStateChanged(SerialConnection::ConnectionState)));

    RTLSDisplayApplication::connectReady(this, "onReady()");


    QObject::connect(ui->action_cn,SIGNAL(triggered(bool)),this,SLOT(SetLanguage_cn()));
    QObject::connect(ui->action_en,SIGNAL(triggered(bool)),this,SLOT(SetLanguage_en()));



}

void MainWindow::onReady()
{

    QObject::connect(graphicsWidget(), SIGNAL(setTagHistory(int)), viewSettingsWidget(), SLOT(setTagHistory(int)));

    QObject::connect(viewSettingsWidget(), SIGNAL(saveViewSettings(void)), this, SLOT(saveViewConfigSettings(void)));

    QObject::connect(graphicsWidget(), SIGNAL(viewSizeChange(QSize)), this, SLOT(viewSizeChange(QSize)));
    QObject::connect(graphicsWidget(), SIGNAL(LoadWidgetVisibleChange(bool)), this, SLOT(LoadWidgetVisibleChange(bool)));
    QObject::connect(graphicsWidget(), SIGNAL(LoadWidgetIndexChange(int)), this, SLOT(LoadWidgetIndexChange(int)));

   loadSettings();//sunhao0819

    if(_showMainToolBar)
    {
        ui->mainToolBar->show();
    }
    else
    {
        ui->mainToolBar->hide();
    }

    ui->viewSettings_dw->show();

    ui->mainToolBar->show();

    m_LoadWidget = new QQuickWidget(this);
    m_LoadWidget->rootContext()->setContextProperty("calibrationTextVisible", false);
    m_LoadWidget->rootContext()->setContextProperty("calibrationIndex", 0);
    m_LoadWidget->rootContext()->setContextProperty("language", language);
    m_LoadWidget->setSource(QUrl("qrc:/icons/Loading.qml"));
    m_LoadWidget->move(2346,1334);
    m_LoadWidget->setAttribute(Qt::WA_AlwaysStackOnTop);
    m_LoadWidget->setClearColor(QColor(Qt::transparent));

}

MainWindow::~MainWindow()
{
    delete ui;
}

GraphicsWidget *MainWindow::graphicsWidget()
{
    return ui->graphicsWidget;
}

ViewSettingsWidget *MainWindow::viewSettingsWidget()
{
    return ui->viewSettings_w;
}

UwbDataVizWidget *MainWindow::uwbDataVizWidget()
{
    return ui->uwbDataViz_w;
}

QMenu *MainWindow::createPopupMenu()
{
    return createPopupMenu(new QMenu());
}

QMenu *MainWindow::createPopupMenu(QMenu *menu)
{
    menu->addAction(ui->viewSettings_dw->toggleViewAction());
    menu->addAction(ui->minimap_dw->toggleViewAction());
    menu->addAction(ui->uwbDataViz_dw->toggleViewAction());

    return menu;
}

void MainWindow::onAnchorConfigAction()
{
    ui->mainToolBar->show();
}

void MainWindow::onMiniMapView()
{
    //check if we have loaded floorplan before we open mini map
    //if no floor plan close minimap
    QString path = RTLSDisplayApplication::viewSettings()->getFloorplanPath();

    if(path == "") //no floorplan loaded
    {
        ui->minimap_dw->close();
        //qDebug() << "close minimap" ;
        QMessageBox::warning(NULL, tr("Error"), "No floorplan loaded. Please upload floorplan to use mini-map.");
    }
}

void MainWindow::connectionStateChanged(SerialConnection::ConnectionState state)
{
    switch(state)
    {
        case SerialConnection::Connecting:
        {
            statusBar()->showMessage(QString("Connecting to Tag/Anchor..."));
            _showMainToolBar = false;
            _notConnected = false;
            serial_connected = true;
            break;
        }
        case SerialConnection::Connected:
        {
            qDebug() << "Connection successful";
            statusBar()->showMessage("Connection successful.");
           // loadSettings();//sunhao0819
            _showMainToolBar = false;
            _notConnected = false;
            serial_connected = true;
            break;
        }
        case SerialConnection::ConnectionFailed:
        {
            statusBar()->showMessage("Connection failed.");
            loadSettings();//sunhao0819
            _showMainToolBar = true;
            _notConnected = true;
            serial_connected = false;
            break;
        }
        case SerialConnection::Disconnected:
        {
            qDebug() << "Connection disconnected";
            statusBar()->showMessage("Connection disconnected.");
            _showMainToolBar = true;
            _notConnected = true;
            serial_connected = false;
            //RTLSDisplayApplication::serialConnection()->closeConnection();
            break;
        }
    }

    if(state != SerialConnection::Connecting)
    {
        RTLSDisplayApplication::client()->setGWReady(true);
    }

    ui->mainToolBar->show();

}


void MainWindow::loadSettings()
{
    QSettings s;
    s.beginGroup("MainWindow");
    QVariant state = s.value("window-state");
    if (state.convert(QVariant::ByteArray))
    {
        this->restoreState(state.toByteArray());
    }

    QVariant geometry = s.value("window-geometry");
    if (geometry.convert(QVariant::ByteArray))
    {
        this->restoreGeometry(geometry.toByteArray());
    }
    else
    {
        this->showMaximized();
    }
    s.endGroup();

    //load view settings
    loadConfigFile("./TREKview_config.xml");
    graphicsWidget()->loadConfigFile("./TREKtag_config.xml");
    RTLSDisplayApplication::instance()->client()->loadConfigFile("./TREKanc_config.xml");
}

void MainWindow::saveViewConfigSettings(void)
{
    saveConfigFile("./TREKview_config.xml", "view_cfg");
}

void MainWindow::viewSizeChange(QSize size)
{
    if(m_LoadWidget)
    {
        m_LoadWidget->move(size.width()+80,size.height()+200);
    }
}

void MainWindow::LoadWidgetVisibleChange(bool visible)
{
    m_LoadWidget->rootContext()->setContextProperty("calibrationTextVisible", visible);
}

void MainWindow::LoadWidgetIndexChange(int index)
{
    m_LoadWidget->rootContext()->setContextProperty("calibrationIndex", index);
}

void MainWindow::saveSettings()
{
    QSettings s;
    s.beginGroup("MainWindow");
    s.setValue("window-state",    this->saveState());
    s.setValue("window-geometry", this->saveGeometry());
    s.endGroup();

    //save view settings

    saveConfigFile("./TREKview_config.xml", "view_cfg");
    graphicsWidget()->saveConfigFile("./TREKtag_config.xml");
    RTLSDisplayApplication::instance()->client()->saveConfigFile("./TREKanc_config.xml");
}

void MainWindow::onAboutAction()
{
    _infoLabel->setText(tr("Invoked <b>Help|About</b>"));
    QMessageBox::about(this, tr("About"),
            tr("<b>HR-RTLS_PC</b>"
               "<br>version 2.0 (" __DATE__
               ") <br>浩如科技.\n"
                          "<br>www.haorutech.com"));
}

void MainWindow::loadConfigFile(QString filename)
{
    QFile file(filename);

    if (!file.open(QFile::ReadWrite | QFile::Text))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(file.errorString())));
        return;
    }

    QDomDocument doc;

    doc.setContent(&file, false);

    QDomElement root = doc.documentElement();

   // qDebug() << root.tagName() ;


    if( root.tagName() == "config" )
    {
        //there are acnhors saved in the config file
        //populate the _model anchor list

        QDomNode n = root.firstChild();
        while( !n.isNull() )
        {
            QDomElement e = n.toElement();
            if( !e.isNull() )
            {
                if( e.tagName() == "view_cfg" )
                {

                    RTLSDisplayApplication::viewSettings()->setGridWidth((e.attribute( "gridW", "" )).toDouble());
                    RTLSDisplayApplication::viewSettings()->setGridHeight((e.attribute( "gridH", "" )).toDouble());
                    RTLSDisplayApplication::viewSettings()->setShowGrid(((e.attribute( "gridS", "" )).toInt() == 1) ? true : false);
                    RTLSDisplayApplication::viewSettings()->setShowOrigin(((e.attribute( "originS", "" )).toInt() == 1) ? true : false);
                    RTLSDisplayApplication::viewSettings()->setFloorplanPath(e.attribute( "fplan", "" ));
                    RTLSDisplayApplication::viewSettings()->setFloorplanXOffset((e.attribute( "offsetX", "" )).toDouble());
                    RTLSDisplayApplication::viewSettings()->setFloorplanYOffset((e.attribute( "offsetY", "" )).toDouble());
                    RTLSDisplayApplication::viewSettings()->setFloorplanXScale((e.attribute( "scaleX", "" )).toDouble());
                    RTLSDisplayApplication::viewSettings()->setFloorplanYScale((e.attribute( "scaleY", "" )).toDouble());
                    RTLSDisplayApplication::viewSettings()->floorplanFlipX((e.attribute( "flipX", "" )).toInt());
                    RTLSDisplayApplication::viewSettings()->floorplanFlipY((e.attribute( "flipY", "" )).toInt());
                    RTLSDisplayApplication::viewSettings()->setCanvesFontSize((e.attribute( "canvesFontSize", "" )).toInt());
                    RTLSDisplayApplication::viewSettings()->setTagSize((e.attribute( "tagSize", "" )).toDouble());
                    RTLSDisplayApplication::viewSettings()->setFloorplanPathN();
                    RTLSDisplayApplication::viewSettings()->setSaveFP(((e.attribute( "saveFP", "" )).toInt() == 1) ? true : false);
                    QString m_addr = "";
                    m_addr = (e.attribute( "ipAddr", "" )=="127.0.0.1" ? ip_addressText : e.attribute( "ipAddr", "" ));
                    RTLSDisplayApplication::viewSettings()->setLeIpAddr(m_addr);
                    LE_ip_addressText = m_addr;
                    RTLSDisplayApplication::viewSettings()->setComNumber((e.attribute( "comNum", "" )).toInt());
                    LE_comTexttoInt = e.attribute( "comNum", "" ).toInt();
                }
            }

            n = n.nextSibling();
        }

    }

    file.close(); //close the file
}

void MainWindow::saveConfigFile(QString filename, QString cfg)
{
    QFile file(filename);

    /*if (!file.open(QFile::ReadWrite | QFile::Text))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(file.errorString())));
        return;
    }*/

    QDomDocument doc;
    //doc.setContent(&file, false);

    //save the graphical information
    QDomElement info = doc.createElement("config");
    doc.appendChild(info);

   // qDebug() << info.tagName() ;

    if(cfg == "view_cfg")
    {
        QDomElement cn = doc.createElement( "view_cfg" );

        cn.setAttribute("gridW",  QString::number(RTLSDisplayApplication::viewSettings()->gridWidth(), 'g', 3));
        cn.setAttribute("gridH",  QString::number(RTLSDisplayApplication::viewSettings()->gridHeight(), 'g', 3));
        cn.setAttribute("gridS",  QString::number((RTLSDisplayApplication::viewSettings()->gridShow() == true) ? 1 : 0));
        cn.setAttribute("originS",  QString::number((RTLSDisplayApplication::viewSettings()->originShow() == true) ? 1 : 0));
        cn.setAttribute("saveFP",  QString::number((RTLSDisplayApplication::viewSettings()->floorplanSave() == true) ? 1 : 0));
        cn.setAttribute("canvesFontSize",  QString::number(RTLSDisplayApplication::viewSettings()->canvesFontSize()));
        cn.setAttribute("tagSize",  QString::number(RTLSDisplayApplication::viewSettings()->tagSize()));
        cn.setAttribute("comNum",  QString::number(LE_comTexttoInt));
        cn.setAttribute("ipAddr",  LE_ip_addressText);

        if(RTLSDisplayApplication::viewSettings()->floorplanSave()) //we want to save the floor plan...
        {
            cn.setAttribute("flipX",  QString::number(RTLSDisplayApplication::viewSettings()->floorplanFlipX(), 10));
            cn.setAttribute("flipY",  QString::number(RTLSDisplayApplication::viewSettings()->floorplanFlipY(), 10));
            cn.setAttribute("scaleX",  QString::number(RTLSDisplayApplication::viewSettings()->floorplanXScale(),'g', 3));
            cn.setAttribute("scaleY",  QString::number(RTLSDisplayApplication::viewSettings()->floorplanYScale(), 'g', 3));
            cn.setAttribute("offsetX",  QString::number(RTLSDisplayApplication::viewSettings()->floorplanXOffset(), 'g', 3));
            cn.setAttribute("offsetY",  QString::number(RTLSDisplayApplication::viewSettings()->floorplanYOffset(), 'g', 3));
            cn.setAttribute("fplan", RTLSDisplayApplication::viewSettings()->getFloorplanPath());
        }
        else
        {

        }
        info.appendChild( cn );

    }

    //file.close(); //close the file and overwrite with new info

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot write config file" << file.errorString();
        return;
    }

    QTextStream ts( &file );
    ts << doc.toString();

   // qDebug() << doc.toString();

    file.close();
}


void MainWindow::statusBarMessage(QString status)
{
    ui->statusBar->showMessage(status);
}

void MainWindow::SetLanguage_cn()
{       
        translator.load(":/new/prefix1/lanague_cn.qm");
        qApp->installTranslator(&translator);
        language=true;
        m_LoadWidget->rootContext()->setContextProperty("language", language);
}
void MainWindow::SetLanguage_en()
{  
    translator.load(":/new/prefix1/lanague_en.qm");
    qApp->installTranslator(&translator);
    language=false;
    m_LoadWidget->rootContext()->setContextProperty("language", language);
}
bool MainWindow::event(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);

    }
    return QWidget::event(event);
}
