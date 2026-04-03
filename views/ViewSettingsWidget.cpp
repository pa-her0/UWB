// -------------------------------------------------------------------------------------------------------------------
//
//  File: ViewSettingsWidget.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------


#include "ui_GraphicsWidget.h"

#include "RTLSDisplayApplication.h"
#include "QPropertyModel.h"
#include "ViewSettings.h"
#include "OriginTool.h"
#include "ScaleTool.h"
#include "GraphicsView.h"
#include "GraphicsWidget.h"
#include "mainwindow.h"
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QHostInfo>
#include <algorithm>
//QString Rev_$setok;

#include "ViewSettingsWidget.h"
#include "ui_ViewSettingsWidget.h"

bool serial_connected = false;
extern QString port_name;
// 生成颜色的函数
QColor generateColor(int index, int totalColors) {
    double hue = static_cast<double>(index) / totalColors;
    double saturation = 0.7; // 饱和度
    double value = 0.9; // 亮度
    int h = static_cast<int>(hue * 360);
    int s = static_cast<int>(saturation * 255);
    int v = static_cast<int>(value * 255);
    return QColor::fromHsv(h, s, v);
}

ViewSettingsWidget *ViewSettingsWidget::viewsettingswidget = nullptr;
ViewSettingsWidget::ViewSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViewSettingsWidget),
    _floorplanOpen(false)
{
    ui->setupUi(this);
    viewsettingswidget = this;

    //mySerialConnection->ViewSettingsWidgetUi =ui;
    //ui->tabWidget->setCurrentIndex(0);
    //ui->tabWidget->removeTab(2);

    QButtonGroup *group = new QButtonGroup(this);
    group->addButton(ui->showNavigationMode,1);//最小二乘
    group->addButton(ui->showNavigationMode_4,4);//官方三角定位
    group->addButton(ui->showNavigationMode_3,2);//一维
    QButtonGroup *group1 = new QButtonGroup(this);
    group1->addButton(ui->showNavigationMode_2,3);//3D视图
    group1->addButton(ui->showNavigationMode_5,5);//1D/2D视图
    // ui->showNavigationMode->setCheckState(Qt::Checked);
    // ui->showNavigationMode_2->setEnabled(true);
    ui->showNavigationMode_4->setCheckState(Qt::Checked);
    ui->showNavigationMode_5->setCheckState(Qt::Checked);
    ui->showNavigationMode_2->setEnabled(false);




    QObject::connect(RTLSDisplayApplication::serialConnection(),SIGNAL(dataupdate()),SLOT(dataupdate()));

    QObject::connect(ui->connect_pb, SIGNAL(clicked()), SLOT(connectButtonClicked()));  //add a function for the Connect button click

    QObject::connect(ui->floorplanOpen_pb, SIGNAL(clicked()), this, SLOT(floorplanOpenClicked()));

    QObject::connect(ui->scaleX_pb, SIGNAL(clicked()), this, SLOT(scaleClicked()));
    QObject::connect(ui->scaleY_pb, SIGNAL(clicked()), this, SLOT(scaleClicked()));
    QObject::connect(ui->origin_pb, SIGNAL(clicked()), this, SLOT(originClicked()));

    QObject::connect(ui->saveFP, SIGNAL(clicked()), this, SLOT(saveFPClicked()));
    QObject::connect(ui->gridShow, SIGNAL(clicked()), this, SLOT(gridShowClicked()));

    QObject::connect(ui->showOrigin, SIGNAL(clicked()), this, SLOT(originShowClicked()));
    QObject::connect(ui->showTagHistory, SIGNAL(clicked()), this, SLOT(tagHistoryShowClicked()));
   // QObject::connect(ui->showGeoFencingMode, SIGNAL(clicked()), this, SLOT(showGeoFencingModeClicked()));
    QObject::connect(ui->showNavigationMode, SIGNAL(clicked()), this, SLOT(showNavigationModeClicked()));

    QObject::connect(ui->useAutoPos, SIGNAL(clicked()), this, SLOT(useAutoPosClicked()));
    QObject::connect(ui->showTagTable, SIGNAL(clicked()), this, SLOT(tagAncTableShowClicked()));
    QObject::connect(ui->showAnchorTable, SIGNAL(clicked()), this, SLOT(tagAncTableShowClicked()));
    QObject::connect(ui->showAnchorTagCorrectionTable, SIGNAL(clicked()), this, SLOT(tagAncTableShowClicked()));

    QObject::connect(ui->checkCanvesArea, SIGNAL(stateChanged(int)), this, SLOT(checkCanvesAreaClicked(int)));
    QObject::connect(ui->canvesFontSize, SIGNAL(valueChanged(int)), this, SLOT(canvesFontSizeValueChanged(int)));
    QObject::connect(ui->checkCanvesInfoArea, SIGNAL(stateChanged(int)), this, SLOT(checkCanvesInfoClicked(int)));

    QObject::connect(myGraphicsWidget->ui->anchorTable, SIGNAL(cellChanged(int, int)), this, SLOT(anchorTableChanged(int, int)));
    QObject::connect(myGraphicsWidget->ui->tagTable, SIGNAL(cellClicked(int, int)), this, SLOT(tagTableClicked(int, int)));
    QObject::connect(ui->showExplainBtn, SIGNAL(clicked()), this, SLOT(showExplainBtnClicked()));
    QObject::connect(ui->cadToPngBtn, SIGNAL(clicked()), this, SLOT(onCadToPngClicked()));


    QObject::connect(ui->tagHistoryN, SIGNAL(valueChanged(int)), this, SLOT(tagHistoryNumberValueChanged(int)));

    QObject::connect(RTLSDisplayApplication::viewSettings(), SIGNAL(showSave(bool)), this, SLOT(showSave(bool)));
    QObject::connect(RTLSDisplayApplication::viewSettings(), SIGNAL(showGO(bool, bool)), this, SLOT(showOriginGrid(bool, bool)));
    QObject::connect(RTLSDisplayApplication::viewSettings(), SIGNAL(setFloorPlanPic()), this, SLOT(getFloorPlanPic()));
    QObject::connect(RTLSDisplayApplication::client(), SIGNAL(enableFiltering()), this, SLOT(enableFiltering()));

    QObject::connect(ui->logging_pb, SIGNAL(clicked()), this, SLOT(loggingClicked()));

    QObject::connect(RTLSDisplayApplication::client(), SIGNAL(enableAutoPositioning(int)), this, SLOT(enableAutoPositioning(int)));

    QObject::connect(RTLSDisplayApplication::viewSettings(), SIGNAL(leIpAddrChanged(QString)), this, SLOT(leIpAddrChanged(QString)));
    QObject::connect(RTLSDisplayApplication::viewSettings(), SIGNAL(comNumberChanged(int)), this, SLOT(comNumberChanged(int)));

   //获取IPv4 ip地址
    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);
    foreach(QHostAddress address,info.addresses())
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            ui->ip_address->addItem(address.toString());
            ip_addressText = address.toString();
        }
    //ui->ip_address->setText(address.toString());
    }

    //  赋值点大小
  //  ui->tag_size->setText(QString("%2").arg(GraphicsWidget::_tagSize));
  //  cout<<"1234567897897654564"<<GraphicsWidget::_tagSize<<endl;
    _logging = false;

    ui->label_logfile->setText("");
    if(_logging)
    {
        if(language)
       {
          ui->logging_pb->setText("停止");
          ui->label_logingstatus->setText("日志记录-开");
       }
        else{
           ui->logging_pb->setText("Stop");
           ui->label_logingstatus->setText("Logging - On");
        }
    }
    else
    {

        if(language)
       {
            ui->logging_pb->setText("开始");
            ui->label_logingstatus->setText("日志记录-关");
       }
        else{
           ui->logging_pb->setText("Start");
           ui->label_logingstatus->setText("Logging - Off");
        }
    }

    _enableAutoPos = false; //auto positioning only works when connected to Anchor #0, so it is disabled until connected to Anchor #0
    ui->useAutoPos->setDisabled(true);

    ui->connect_pb->setEnabled(true);
    ui->label->setEnabled(true);
    ui->comPort->setEnabled(true);

    _selected = 0;
    fucStatus = false;
    RTLSDisplayApplication::connectReady(this, "onReady()");

  /*  */
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
       // qDebug() << "Name        : " << info.portName();
       // qDebug() << "Description : " << info.description();
       // qDebug() << "Manufacturer: " << info.manufacturer();

        // Example use QSerialPort
        QSerialPort serial;
        serial.setPort(info);
        if (serial.open(QIODevice::ReadWrite))
        {   ui->comPort->addItem(info.portName());
            serial.close();
        }
    }
    /*  */

    // 创建三维散点图对象
    scatter = new Custom3DScatter();
    // 创建三维散点图容器

    container = QWidget::createWindowContainer(scatter);
    QSize graphSize = myGraphicsWidget->size();
    QSize screenSize = scatter->screen()->size();
    container->setMinimumSize(graphSize.width()/2, graphSize.height()/2);
    container->setMaximumSize(screenSize);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setFocusPolicy(Qt::StrongFocus);
    myGraphicsWidget->ui->stackedWidget->addWidget(container);
    // 创建三维散点图的系列对象
    series = new QScatter3DSeries();
    double c_h3d = 0.1;
    for (int i=0; i<MAX_NUM_TAGS; i++)
    {
        seriesPoint[i] = new QScatter3DSeries(); // 分配内存
    }

    // 将当前容器添加到栈窗口对象中
    QScatterDataProxy *proxy = new QScatterDataProxy(); //数据代理
    series = new QScatter3DSeries(proxy); //创建序列
    udpSocket = new QUdpSocket(this);

}

void ViewSettingsWidget::onReady()
{
    qDebug() << "onReady()";
    QPropertyDataWidgetMapper *mapper = QPropertyModel::newMapper(RTLSDisplayApplication::viewSettings(), this);
    mapper->addMapping(ui->gridWidth_sb, "gridWidth");
    mapper->addMapping(ui->gridHeight_sb, "gridHeight");

    mapper->addMapping(ui->floorplanFlipX_cb, "floorplanFlipX", "checked");
    mapper->addMapping(ui->floorplanFlipY_cb, "floorplanFlipY", "checked");
    mapper->addMapping(ui->gridShow, "showGrid", "checked");
    mapper->addMapping(ui->showOrigin, "showOrigin", "checked");

    mapper->addMapping(ui->floorplanXOff_sb, "floorplanXOffset");
    mapper->addMapping(ui->floorplanYOff_sb, "floorplanYOffset");

    mapper->addMapping(ui->floorplanXScale_sb, "floorplanXScale");
    mapper->addMapping(ui->floorplanYScale_sb, "floorplanYScale");
    mapper->addMapping(ui->tag_size,"tagSize");
    mapper->addMapping(ui->canvesFontSize,"canvesFontSize");
    mapper->toFirst();

    QObject::connect(ui->floorplanFlipX_cb, SIGNAL(clicked()), mapper, SLOT(submit())); // Bug with QDataWidgetMapper (QTBUG-1818)
    QObject::connect(ui->floorplanFlipY_cb, SIGNAL(clicked()), mapper, SLOT(submit()));
    QObject::connect(ui->gridShow, SIGNAL(clicked()), mapper, SLOT(submit())); // Bug with QDataWidgetMapper (QTBUG-1818)
    QObject::connect(ui->showOrigin, SIGNAL(clicked()), mapper, SLOT(submit()));

    QObject::connect(RTLSDisplayApplication::serialConnection(), SIGNAL(connectionStateChanged(SerialConnection::ConnectionState)),
                     this, SLOT(connectionStateChanged(SerialConnection::ConnectionState)));


    
    //by default the Geo-Fencing is OFF

    ui->showTagHistory->setChecked(true);


    ui->tabWidget->setCurrentIndex(0);


    ui->filtering->setEnabled(false);
    ui->filtering->addItems(RTLSDisplayApplication::client()->getLocationFilters());
    ui->filtering->setCurrentIndex(2);
    QObject::connect(ui->filtering, SIGNAL(currentIndexChanged(int)), this, SLOT(updateLocationFilter(int)));

    ViewSettingsWidget::updateDeviceList();
    oldPortStringList = RTLSDisplayApplication::serialConnection()->portsList();
   // ui->comPort->addItems(oldPortStringList);
   // qDebug()<<"oldPortStringList"<<oldPortStringList;

    timer = new QTimer;
    QObject::connect(timer,&QTimer::timeout,this,&ViewSettingsWidget::updateDeviceList);//更新后台端口号
    timer->start(1000); // 使用定时器 1秒读取一次后台设备管理器

    connectionStateChanged(SerialConnection::Disconnected);

    m_setTimer =new QTimer;
}

int ViewSettingsWidget::updateDeviceList()
{
    int count = 0;
    //if(serial_connected == false) 
    {
        RTLSDisplayApplication::serialConnection()->findSerialDevices();
        newPortStringList = RTLSDisplayApplication::serialConnection()->portsList();
    
        if(newPortStringList.size() != oldPortStringList.size())
        {
            oldPortStringList = newPortStringList;
            ui->comPort->clear();
            ui->comPort->addItems(oldPortStringList);
        }
    }

    count = ui->comPort->count();
    //qDebug()<<"port count="<<count;
    // qDebug() << "port_num = " << port_name;
    // qDebug() << "newPortStringList = " << newPortStringList;
    // if(newPortStringList.contains(port_name))
    // {
    //     qDebug() << "contains";
    // }
    //check if we have found any TREK devices in the COM ports list
    if((count == 0) || ((!newPortStringList.contains(port_name)) && (port_name!="")))
    {
        qDebug() << "close serial port";
        //ui->connect_pb->setEnabled(false);
        ui->label->setEnabled(false);
        ui->comPort->setEnabled(false);
        RTLSDisplayApplication::serialConnection()->closeConnection();
        port_name = "";

        // connectionStateChanged(SerialConnection::Disconnected);
        // RTLSDisplayApplication::mainWindow()->connectionStateChanged(SerialConnection::Disconnected);
    }


    return count;
}

void ViewSettingsWidget::serialError(void)
{
    ui->connect_pb->setEnabled(false);
    ui->label->setEnabled(false);
    ui->comPort->setEnabled(false);
}


void ViewSettingsWidget::connectButtonClicked()
{

    switch (_state)
    {
    case SerialConnection::Disconnected:
    case SerialConnection::ConnectionFailed:
    {
        RTLSDisplayApplication::serialConnection()->openConnection(ui->comPort->currentIndex());       
//        if( myGraphicsWidget->ui->graphicsView->isHidden())
//      {
//            initGraph3D(); //初始化散点图
//            qDebug()<<"222";
//      }
        break;
    }

    case SerialConnection::Connecting:
        RTLSDisplayApplication::serialConnection()->cancelConnection();  
        break;

    case SerialConnection::Connected:
        RTLSDisplayApplication::serialConnection()->closeConnection();
        break;
    }


}

void ViewSettingsWidget::connectionStateChanged(SerialConnection::ConnectionState state)
{
    this->_state = state;
    switch(state)
    {
    case SerialConnection::Disconnected:
    case SerialConnection::ConnectionFailed:
        if(language)
        {
          ui->connect_pb->setText("连接");
        }
        else
        {
          ui->connect_pb->setText("Connect");
        }

        ui->BTN_getdata->setEnabled(false);
        ui->BTN_Restore->setEnabled(false);
        ui->BTN_tongbu->setEnabled(false);
        ui->BTN_jizhan->setEnabled(false);
        ui->BTN_addr->setEnabled(false);
        ui->BTN_ant_dly_ok->setEnabled(false);
        ui->lE_tx_power_ok->setEnabled(false);

        //this->show();
        break;

    case SerialConnection::Connecting:
        if(language)
        {
          ui->connect_pb->setText("取消");
        }
        else
        {
          ui->connect_pb->setText("Cancel");
        }

        //this->show();
        break;

    case SerialConnection::Connected:
        if(language)
        {
          ui->connect_pb->setText("断开");
        }
        else
        {
          ui->connect_pb->setText("Disconnect");
        }

        ui->BTN_getdata->setEnabled(true);
        ui->BTN_Restore->setEnabled(true);
        ui->BTN_tongbu->setEnabled(true);
        ui->BTN_ant_dly_ok->setEnabled(true);
        ui->lE_tx_power_ok->setEnabled(true);
        //this->hide();
        break;
    }

    bool enabled = (state == SerialConnection::Disconnected || state == SerialConnection::ConnectionFailed) ? true : false;
    //can change the COM port once not opened
    ui->comPort->setEnabled(enabled);
    //cout<<enabled<<endl;
    if(enabled == 0){
        ui->socket_pb->setEnabled(0);
        ui->socket_pb_udp->setEnabled(0);
    }
    else{
        if(ui->socket_pb_udp->text()=="断开")
       {
       }
       else{
        ui->socket_pb->setEnabled(1);
        ui->socket_pb_udp->setEnabled(1);
       }
    }

}


ViewSettingsWidget::~ViewSettingsWidget()
{
    // 遍历删除图表对象
    for(int index = m_graphLsit.count() - 1; index != -1; --index)
    {
        // 释放图表
        delete m_graphLsit.at(index);
    }
    delete ui;
}


void ViewSettingsWidget::enableFiltering(void)
{
     ui->filtering->setEnabled(true);
}

void ViewSettingsWidget::updateLocationFilter(int index)
{
     RTLSDisplayApplication::client()->setLocationFilter(index);
}

int ViewSettingsWidget::applyFloorPlanPic(const QString &path)
{
    QPixmap pm(path);

    if (pm.isNull())
    {
        //QMessageBox::critical(this, "Could not load floor plan", QString("Failed to load image : %1").arg(path));
        return -1;
    }

    ui->floorplanPath_lb->setText(QFileInfo(path).fileName());
    RTLSDisplayApplication::viewSettings()->setFloorplanPixmap(pm);
    _floorplanOpen = true;

    if(language)
    {
       ui->floorplanOpen_pb->setText("清除");
    }
    else{
       ui->floorplanOpen_pb->setText("Clean");
    }

    return 0;
}

void ViewSettingsWidget::getFloorPlanPic()
{
    applyFloorPlanPic(RTLSDisplayApplication::viewSettings()->getFloorplanPath());
}

void ViewSettingsWidget::floorplanOpenClicked()
{
    if(_floorplanOpen == false)
    {
        QString path = QFileDialog::getOpenFileName(this, "Open Bitmap", QString(), "Image (*.png *.jpg *.jpeg *.bmp)");
        if (path.isNull()) return;

        if(applyFloorPlanPic(path) == 0) //if OK set/save the path string
        {
            RTLSDisplayApplication::viewSettings()->floorplanShow(true);
            RTLSDisplayApplication::viewSettings()->setFloorplanPath(path);
        }
        _floorplanOpen = true;

        if(language)
        {
           ui->floorplanOpen_pb->setText("清除");
        }
        else{
           ui->floorplanOpen_pb->setText("Clean");
        }
    }
    else
    {
       RTLSDisplayApplication::viewSettings()->floorplanShow(false);
       RTLSDisplayApplication::viewSettings()->clearSettings();
       _floorplanOpen = false;
       if(language)
       {
          ui->floorplanOpen_pb->setText("打开");
       }
       else{
          ui->floorplanOpen_pb->setText("Open");
       }
       ui->floorplanFlipX_cb->setChecked(false);
       ui->floorplanFlipY_cb->setChecked(false);
       ui->floorplanXScale_sb->setValue(0.0);
       ui->floorplanYScale_sb->setValue(0.0);
       ui->floorplanXOff_sb->setValue(0.0);
       ui->floorplanYOff_sb->setValue(0.0);
       ui->floorplanPath_lb->setText("");
    }
}

void ViewSettingsWidget::showOriginGrid(bool orig, bool grid)
{
    Q_UNUSED(orig)

    ui->gridShow->setChecked(grid);
    ui->showOrigin->setChecked(orig);
}

void ViewSettingsWidget::gridShowClicked()
{
    RTLSDisplayApplication::viewSettings()->setShowGrid(ui->gridShow->isChecked());
}

void ViewSettingsWidget::originShowClicked()
{
    RTLSDisplayApplication::viewSettings()->setShowOrigin(ui->showOrigin->isChecked());
}

void ViewSettingsWidget::showNavigationModeClicked()
{
    if(ui->showNavigationMode->isChecked())
    {
        //ui->showGeoFencingMode->setChecked(false);
        showGeoFencingModeClicked();
    }
    else
    {
      //  ui->showGeoFencingMode->setChecked(true);
        showGeoFencingModeClicked();
    }
}


void ViewSettingsWidget::showGeoFencingModeClicked()
{
    /*
    RTLSDisplayApplication::graphicsWidget()->showGeoFencingMode(ui->showGeoFencingMode->isChecked());

    if(ui->showGeoFencingMode->isChecked())
    {
        ui->showTagHistory->setDisabled(true);
        ui->tagHistoryN->setDisabled(true);

       // ui->zone1->setEnabled(true);
       // ui->zone2->setEnabled(true);
       // ui->label_z1->setEnabled(true);
       // ui->label_z2->setEnabled(true);
        ui->outAlarm->setEnabled(true);
        ui->inAlarm->setEnabled(true);

        ui->showNavigationMode->setChecked(false);

        if(_enableAutoPos)
        {
            //set auto positioning to off when in geo-fencing mode
            RTLSDisplayApplication::client()->setUseAutoPos(false);
            ui->useAutoPos->setDisabled(true);
        }
    }
    else
    {
        ui->showTagHistory->setDisabled(false);
        ui->tagHistoryN->setDisabled(false);

      //  ui->zone1->setDisabled(true);
      //  ui->zone2->setDisabled(true);
      //  ui->label_z1->setDisabled(true);
       // ui->label_z2->setDisabled(true);
        ui->outAlarm->setDisabled(true);
        ui->inAlarm->setDisabled(true);

        if(_enableAutoPos)
        {
            ui->useAutoPos->setDisabled(false);
            useAutoPosClicked();
        }

        ui->showNavigationMode->setChecked(true);
    }
*/
}

void ViewSettingsWidget::enableAutoPositioning(int enable)
{
    _enableAutoPos = enable;
    ui->useAutoPos->setDisabled(!enable);
}

void ViewSettingsWidget::tagHistoryNumberValueChanged(int value)
{
    RTLSDisplayApplication::graphicsWidget()->tagHistoryNumber(value);
}

void ViewSettingsWidget::zone1EditFinished(void)
{
   // RTLSDisplayApplication::graphicsWidget()->zone1Value(ui->zone1->value());
}

void ViewSettingsWidget::zone2EditFinished(void)
{
  //  RTLSDisplayApplication::graphicsWidget()->zone2Value(ui->zone2->value());
}

void ViewSettingsWidget::zone1ValueChanged(double value)
{
    RTLSDisplayApplication::graphicsWidget()->zone1Value(value);
}

void ViewSettingsWidget::zone2ValueChanged(double value)
{
    RTLSDisplayApplication::graphicsWidget()->zone2Value(value);
}

void ViewSettingsWidget::useAutoPosClicked()
{
    RTLSDisplayApplication::client()->setUseAutoPos(ui->useAutoPos->isChecked());

    RTLSDisplayApplication::graphicsWidget()->anchTableEditing(!ui->useAutoPos->isChecked());
}

void ViewSettingsWidget::tagAncTableShowClicked()
{
    RTLSDisplayApplication::graphicsWidget()->setShowTagAncTable(ui->showAnchorTable->isChecked(),
                                                                 ui->showTagTable->isChecked(),
                                                                 ui->showAnchorTagCorrectionTable->isChecked());
}

void ViewSettingsWidget::checkCanvesAreaClicked(int state)
{
    if(state == Qt::Checked)  //选中
    {
        RTLSDisplayApplication::graphicsWidget()->setCanvasVisible(true);
    }
    else
    {
        RTLSDisplayApplication::graphicsWidget()->setCanvasVisible(false);
    }
    //    RTLSDisplayApplication::graphicsWidget()->canvasShow();
}

void ViewSettingsWidget::canvesFontSizeValueChanged(int value)
{
//    RTLSDisplayApplication::graphicsWidget()->setCanvasFontSize(value);
}

void ViewSettingsWidget::checkCanvesInfoClicked(int state)
{
    if(state == Qt::Checked)  //选中
    {
        RTLSDisplayApplication::graphicsWidget()->setCanvasInfoVisible(true);
    }
    else
    {
        RTLSDisplayApplication::graphicsWidget()->setCanvasInfoVisible(false);
    }
}

void ViewSettingsWidget::setTagHistory(int h)
{
    ui->tagHistoryN->setValue(h);
}


void ViewSettingsWidget::tagHistoryShowClicked()
{
    RTLSDisplayApplication::graphicsWidget()->setShowTagHistory(ui->showTagHistory->isChecked());
}


void ViewSettingsWidget::loggingClicked(void)
{
    if(_logging == false)
    {
        _logging = true ;
        RTLSDisplayApplication::client()->openLogFile("");

        if(language)
        {
            ui->logging_pb->setText("停止");
            ui->label_logingstatus->setText("日志记录-开");
        }
        else
        {
            ui->logging_pb->setText("Stop");
            ui->label_logingstatus->setText("Logging - On");
        }
        ui->label_logfile->setText(RTLSDisplayApplication::client()->getLogFilePath());
    }
    else
    {
        RTLSDisplayApplication::client()->closeLogFile();
        if(language)
        {
            ui->logging_pb->setText("开始");
            ui->label_logingstatus->setText("日志记录-关");
        }
        else
        {
            ui->logging_pb->setText("Start");
            ui->label_logingstatus->setText("Logging - Off");
        }

        ui->label_logfile->setText("");
        ui->saveFP->setChecked(false);
        _logging = false ;
    }
}


void ViewSettingsWidget::alarmSetClicked()
{
   // RTLSDisplayApplication::graphicsWidget()->setAlarm(ui->inAlarm->isChecked(), ui->outAlarm->isChecked());
}

void ViewSettingsWidget::saveFPClicked()
{
    RTLSDisplayApplication::viewSettings()->setSaveFP(ui->saveFP->isChecked());

    if(ui->saveFP->isChecked())
    {
        //save the current settings when clicked
       emit saveViewSettings();
    }
}

void ViewSettingsWidget::showSave(bool save)
{
    ui->saveFP->setChecked(save);
}

void ViewSettingsWidget::leIpAddrChanged(QString addr)
{
    ui->LE_ip_address->setText(addr);
}

void ViewSettingsWidget::comNumberChanged(int num)
{
    ui->LE_com->setText(QString::number(num));
}

void ViewSettingsWidget::originClicked()
{
    OriginTool *tool = new OriginTool(this);
    QObject::connect(tool, SIGNAL(done()), tool, SLOT(deleteLater()));
    RTLSDisplayApplication::graphicsView()->setTool(tool);
}

void ViewSettingsWidget::scaleClicked()
{
    ScaleTool *tool = NULL;

    if (QObject::sender() == ui->scaleX_pb)
        tool = new ScaleTool(ScaleTool::XAxis, this);
    else if (QObject::sender() == ui->scaleY_pb)
        tool = new ScaleTool(ScaleTool::YAxis, this);

    QObject::connect(tool, SIGNAL(done()), tool, SLOT(deleteLater()));
    RTLSDisplayApplication::graphicsView()->setTool(tool);
}

void ViewSettingsWidget::on_Tag_size_valueChanged(double arg1)
{
    QFile file("./TREKtag_config.xml");

    if (!file.open(QIODevice::ReadWrite))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg("./TREKtag_config.xml").arg(file.errorString())));
        return;
    }

    QDomDocument doc;
    QString error;
    if(!doc.setContent(&file))
    {
        file.close();
        //qDebug("123456");
        return;
    }
    file.close();
    QDomElement config = doc.documentElement();//获取根结点元素

    if( config.tagName() == "config" )
    {
        QDomNode n = config.firstChild();
        while( !n.isNull() )
        {
            QDomElement e = n.toElement();
            if( !e.isNull() )
            {
                if( e.tagName() == "tag_cfg" )
                {
                    QString oldpath= e.attribute("size");
                    //cout<<e.attribute("size").toStdString()<<endl;
                    e.setAttribute("size",arg1);
                    //cout<<e.attribute("size").toStdString()<<endl;

                    break ;
                }

            }

            n = n.nextSibling();
        }

    }

    QFile file1("./TREKtag_config.xml");
       if (!file1.open(QIODevice::WriteOnly | QFile::Truncate))
       {
           qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg("./TREKtag_config.xml").arg(file.errorString())));
       }

       QTextStream xmlWrite(&file1);
       doc.save(xmlWrite,1);
       file.close();
    GraphicsWidget().loadConfigFile("./TREKtag_config.xml");
    GraphicsWidget().saveConfigFile("./TREKtag_config.xml");
    qDebug("写入成功");
    int result;
    if(language)
    {
       QMessageBox messageBox(QMessageBox::NoIcon,
                              "提示", "重启生效,点击yes立即重启",
                              QMessageBox::Yes | QMessageBox::No, NULL);
       result=messageBox.exec();
    }
    else
    {
        QMessageBox messageBox(QMessageBox::NoIcon,
                               "Prompt", "Restart takes effect, click yes to restart immediately",
                               QMessageBox::Yes | QMessageBox::No, NULL);
        result=messageBox.exec();
    }

    switch (result)
        {
        case QMessageBox::Yes:{
            QString program = QApplication::applicationFilePath();
            QStringList arguments = QApplication::arguments();
            QString workingDirectory = QDir::currentPath();
            QProcess::startDetached(program, arguments, workingDirectory);
            QApplication::exit();

            break;}
        case QMessageBox::No:
            qDebug()<<"NO";
            break;
        default:
            break;
        }
}

void ViewSettingsWidget::on_pushButton_clicked()
{
    float num;
    num = ui->tag_size->text().toFloat();
    myGraphicsWidget->setTagSize(num);
    QFile file("./TREKtag_config.xml");

    if (!file.open(QIODevice::ReadWrite))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg("./TREKtag_config.xml").arg(file.errorString())));
        return;
    }

    QDomDocument doc;
    QString error;
    if(!doc.setContent(&file))
    {
        file.close();
        //qDebug("123456");
        return;
    }
    file.close();
    QDomElement config = doc.documentElement();//获取根结点元素

    if( config.tagName() == "config" )
    {
        QDomNode n = config.firstChild();
        while( !n.isNull() )
        {
            QDomElement e = n.toElement();
            if( !e.isNull() )
            {
                if( e.tagName() == "tag_cfg" )
                {
                    QString oldpath= e.attribute("size");
                    //cout<<e.attribute("size").toStdString()<<endl;
                    e.setAttribute("size",num);
                    //cout<<e.attribute("size").toStdString()<<endl;

                    break ;
                }

            }

            n = n.nextSibling();
        }

    }

    QFile file1("./TREKtag_config.xml");
    if (!file1.open(QIODevice::WriteOnly | QFile::Truncate))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg("./TREKtag_config.xml").arg(file.errorString())));
    }

    QTextStream xmlWrite(&file1);
    doc.save(xmlWrite,1);
    file.close();
    GraphicsWidget().loadConfigFile("./TREKtag_config.xml");
    GraphicsWidget().saveConfigFile("./TREKtag_config.xml");
    qDebug("写入成功");
    int result;
    if(language)
    {
       QMessageBox messageBox(QMessageBox::NoIcon,
                              "提示", "重启生效,点击yes立即重启",
                              QMessageBox::Yes | QMessageBox::No, NULL);
       result=messageBox.exec();
    }
    else
    {
        QMessageBox messageBox(QMessageBox::NoIcon,
                               "Prompt", "Restart takes effect, click yes to restart immediately",
                               QMessageBox::Yes | QMessageBox::No, NULL);
        result=messageBox.exec();
    }


    switch (result)
        {
        case QMessageBox::Yes:{
            QString program = QApplication::applicationFilePath();
            QStringList arguments = QApplication::arguments();
            QString workingDirectory = QDir::currentPath();
            QProcess::startDetached(program, arguments, workingDirectory);
            QApplication::exit();

            break;}
        case QMessageBox::No:
            qDebug()<<"NO";
            break;
        default:
            break;
        }
}
void ViewSettingsWidget::setLinetext(QString s)
{
    ui->tag_size->setText(s);
}

void ViewSettingsWidget::on_socket_pb_udp_clicked(bool checked)
{
    ui->socket_pb_udp->setCheckable(true);
    if(checked == 0){
        int port = ui->udp_port->text().toInt();
        //cout<<port<<endl;

        RTLSDisplayApplication::serialConnection()->UdpnetConnect(port);
        ui->socket_pb_udp->setText("断开");
        ui->connect_pb->setEnabled(0);
        ui->socket_pb->setEnabled(0);
    }
    else{
        RTLSDisplayApplication::serialConnection()->UdpnetClose();
        ui->socket_pb_udp->setText("连接");
        ui->connect_pb->setEnabled(1);
        ui->socket_pb->setEnabled(1);
    }
}
void ViewSettingsWidget::on_socket_pb_clicked(bool checked)
{
    ui->socket_pb->setCheckable(true);
    if(checked == 0){
        int port = ui->tcp_port->text().toInt();
        //cout<<port<<endl;
        RTLSDisplayApplication::serialConnection()->netConnect(port);
        if(language)
       {
          ui->socket_pb->setText("断开");
       }
        else{
          ui->socket_pb->setText("disconnect");
        }
        ui->connect_pb->setEnabled(0);
        ui->socket_pb_udp->setEnabled(0);
    }
    else{
        RTLSDisplayApplication::serialConnection()->netClose();
        if(language)
       {
          ui->socket_pb->setText("连接");
       }
        else{
          ui->socket_pb->setText("Connect");
        }
        ui->connect_pb->setEnabled(1);
        ui->socket_pb_udp->setEnabled(1);
    }
}


void ViewSettingsWidget::on_BTN_getdata_clicked()
{
    RTLSDisplayApplication::serialConnection()->rbootdata=true;
    SerialConnection::Rev_$setok = "";
    RTLSDisplayApplication::serialConnection()->writeData("$rboot\r\n");
    m_setTimer->start(200);
    connect(m_setTimer, SIGNAL(timeout()), this, SLOT(Settimeout()));

}


void ViewSettingsWidget::on_BTN_addr_clicked()
{
    int addrtext =ui->lE_addr->text().toInt();
    RTLSDisplayApplication::serialConnection()->rbootdata=true;
    if(addrtext>=0 && addrtext <=100 )
    {
        SerialConnection::Rev_$setok = "";
        RTLSDisplayApplication::serialConnection()->writeData("$saddr,"+ui->lE_addr->text().toUtf8()+"\r\n");
        m_setTimer->start(200);
        connect(m_setTimer, SIGNAL(timeout()), this, SLOT(Settimeout()));

    }
    else
    {
        if(language)
       {
           QMessageBox::warning(this,"警告","输入错误，请检查参数正确性","退出");
       }
        else{
           QMessageBox::warning(this,"Warning", "Input error, please check the correctness of parameters", "Exit");
        }
    }
}

void ViewSettingsWidget::on_BTN_ant_dly_ok_clicked()
{
    RTLSDisplayApplication::serialConnection()->rbootdata=true;
    QString ant_dlytest =ui->lE_ant_dly->text();
    if(ant_dlytest.toInt()>=0 && ant_dlytest.toInt() <= 65535)
    {
        SerialConnection::Rev_$setok = "";
        RTLSDisplayApplication::serialConnection()->writeData("$santdly,"+ant_dlytest.toUtf8()+"\r\n");
        m_setTimer->start(200);
        connect(m_setTimer, SIGNAL(timeout()), this, SLOT(Settimeout()));
        
    }
    else
    {
        if(language)
       {
           QMessageBox::warning(this,"警告","输入错误，请检查参数正确性","退出");
       }
        else{
           QMessageBox::warning(this,"Warning", "Input error, please check the correctness of parameters", "Exit");
        }

    }
}

void ViewSettingsWidget::on_lE_tx_power_ok_clicked()
{
    //int ant_dlytest =ui->lE_ant_dly->text().toInt();
    RTLSDisplayApplication::serialConnection()->rbootdata=true;
    if(ui->lE_tx_power->text().length()==8 )
    {
        SerialConnection::Rev_$setok = "";
        RTLSDisplayApplication::serialConnection()->writeData("$stxpwr,"+ui->lE_tx_power->text().toUtf8()+"\r\n");
        m_setTimer->start(200);
        connect(m_setTimer, SIGNAL(timeout()), this, SLOT(Settimeout()));        

    }
    else
    {
        QMessageBox::warning(this,"警告","输入错误，请检查参数正确性","退出");
    }
}
void ViewSettingsWidget::on_BTN_jizhan_clicked()
{
    RTLSDisplayApplication::serialConnection()->rbootdata=true;
    SerialConnection::Rev_$setok = "";
    RTLSDisplayApplication::serialConnection()->writeData("$sanccd,"+ui->lE_A0X->text().toUtf8()
    +","+ui->lE_A0Y->text().toUtf8()+","+ui->lE_A0Z->text().toUtf8()
    +","+ui->lE_A1X->text().toUtf8()+","+ui->lE_A1Y->text().toUtf8()+","+ui->lE_A1Z->text().toUtf8()
    +","+ui->lE_A2X->text().toUtf8()+","+ui->lE_A2Y->text().toUtf8()+","+ui->lE_A2Z->text().toUtf8()
    +","+ui->lE_A3X->text().toUtf8()+","+ui->lE_A3Y->text().toUtf8()+","+ui->lE_A3Z->text().toUtf8()
                                                          +"\r\n");
    
    m_setTimer->start(200);
    connect(m_setTimer, SIGNAL(timeout()), this, SLOT(Settimeout()));

}
void ViewSettingsWidget::on_BTN_Restore_clicked()
{
    RTLSDisplayApplication::serialConnection()->rbootdata=true;
    SerialConnection::Rev_$setok = "";
    RTLSDisplayApplication::serialConnection()->writeData("$reset\r\n");
    m_setTimer->start(200);
    connect(m_setTimer, SIGNAL(timeout()), this, SLOT(Settimeout()));

}
void ViewSettingsWidget::on_BTN_tongbu_clicked()
{
    ui->lE_A0X->setText(myGraphicsWidget->ui->anchorTable->item(0,1)->text());
    ui->lE_A0Y->setText(myGraphicsWidget->ui->anchorTable->item(0,2)->text());
    ui->lE_A0Z->setText(myGraphicsWidget->ui->anchorTable->item(0,3)->text());
    ui->lE_A1X->setText(myGraphicsWidget->ui->anchorTable->item(1,1)->text());
    ui->lE_A1Y->setText(myGraphicsWidget->ui->anchorTable->item(1,2)->text());
    ui->lE_A1Z->setText(myGraphicsWidget->ui->anchorTable->item(1,3)->text());
    ui->lE_A2X->setText(myGraphicsWidget->ui->anchorTable->item(2,1)->text());
    ui->lE_A2Y->setText(myGraphicsWidget->ui->anchorTable->item(2,2)->text());
    ui->lE_A2Z->setText(myGraphicsWidget->ui->anchorTable->item(2,3)->text());
    ui->lE_A3X->setText(myGraphicsWidget->ui->anchorTable->item(3,1)->text());
    ui->lE_A3Y->setText(myGraphicsWidget->ui->anchorTable->item(3,2)->text());
    ui->lE_A3Z->setText(myGraphicsWidget->ui->anchorTable->item(3,3)->text());
}

void ViewSettingsWidget::Settimeout()
{
    disconnect(m_setTimer, SIGNAL(timeout()), this, SLOT(Settimeout()));

    if(SerialConnection::Rev_$setok =="$setok"  )
    {
        if(language == true)
     {
        QMessageBox::information(this,"提示","设置成功","退出");
        //fucStatus = false;
     }
        else{
        QMessageBox::information(this,"Prompt", "Set successfully", "Exit");
        }
     SerialConnection::Rev_$setok = "";
    }
    else 
    {
        if(language == true)
     {
        QMessageBox::warning(this,"警告","设备未响应，请重试","退出");
     }
        else{

        QMessageBox::warning(this,"Warning", "The device is not responding, please try again", "Exit");
        }
    }


}
void ViewSettingsWidget::dataupdate()
{
    //if(SerialConnection::role=="TAG"||SerialConnection::role=="ANCHOR")
    {
     ui->lE_model->setText(SerialConnection::model);
     ui->lE_firmware->setText(SerialConnection::firmware);
     ui->lE_role->setText(SerialConnection::role);
     ui->lE_addr->setText(SerialConnection::addr);
     ui->lE_blink->setText(SerialConnection::blink);
     ui->lE_max_anc_num->setText(SerialConnection::max_anc_num);
     ui->lE_max_tag_num->setText(SerialConnection::max_tag_num);
     ui->lE_uwb_data_rate->setText(SerialConnection::uwb_data_rate);
     ui->lE_channel->setText(SerialConnection::channel);
     ui->lE_update_frequency->setText(SerialConnection::update_frequency);
     ui->lE_update_Period->setText(SerialConnection::update_Period);
     ui->lE_kalmanfilter->setText(SerialConnection::kalmanfilter);
     ui->lE_ant_dly->setText(SerialConnection::ant_dly);
     ui->lE_tx_power->setText(SerialConnection::tx_power);
     ui->lE_use_ext_eeprom->setText(SerialConnection::use_ext_eeprom);
     ui->lE_use_imu->setText(SerialConnection::use_imu);
     ui->lE_group_id->setText(SerialConnection::group_id);
     ui->lE_A0X->setText(SerialConnection::A0_x);
     ui->lE_A0Y->setText(SerialConnection::A0_y);
     ui->lE_A0Z->setText(SerialConnection::A0_z);
     ui->lE_A1X->setText(SerialConnection::A1_x);
     ui->lE_A1Y->setText(SerialConnection::A1_y);
     ui->lE_A1Z->setText(SerialConnection::A1_z);
     ui->lE_A2X->setText(SerialConnection::A2_x);
     ui->lE_A2Y->setText(SerialConnection::A2_y);
     ui->lE_A2Z->setText(SerialConnection::A2_z);
     ui->lE_A3X->setText(SerialConnection::A3_x);
     ui->lE_A3Y->setText(SerialConnection::A3_y);
     ui->lE_A3Z->setText(SerialConnection::A3_z);
    }


    if(SerialConnection::role=="TAG")
    {
         ui->BTN_addr->setEnabled(true);
         ui->BTN_jizhan->setEnabled(true);
         ui->BTN_tongbu->setEnabled(true);
         ui->lE_addr->setEnabled(true);
         ui->lE_A0X->setEnabled(true);
         ui->lE_A0Y->setEnabled(true);
         ui->lE_A0Z->setEnabled(true);
         ui->lE_A1X->setEnabled(true);
         ui->lE_A1Y->setEnabled(true);
         ui->lE_A1Z->setEnabled(true);
         ui->lE_A2X->setEnabled(true);
         ui->lE_A2Y->setEnabled(true);
         ui->lE_A2Z->setEnabled(true);
         ui->lE_A3X->setEnabled(true);
         ui->lE_A3Y->setEnabled(true);
         ui->lE_A3Z->setEnabled(true);
     }
     else if(SerialConnection::role=="ANCHOR")
     {
         ui->BTN_addr->setEnabled(false);
         ui->BTN_jizhan->setEnabled(false);
         ui->BTN_tongbu->setEnabled(false);
         ui->lE_addr->setEnabled(false);
         ui->lE_A0X->setEnabled(false);
         ui->lE_A0Y->setEnabled(false);
         ui->lE_A0Z->setEnabled(false);
         ui->lE_A1X->setEnabled(false);
         ui->lE_A1Y->setEnabled(false);
         ui->lE_A1Z->setEnabled(false);
         ui->lE_A2X->setEnabled(false);
         ui->lE_A2Y->setEnabled(false);
         ui->lE_A2Z->setEnabled(false);
         ui->lE_A3X->setEnabled(false);
         ui->lE_A3Y->setEnabled(false);
         ui->lE_A3Z->setEnabled(false);

     }
}

void ViewSettingsWidget::on_showNavigationMode_stateChanged(int arg1)//三维
{
    if(arg1 == Qt::CheckState::Checked)
    {

        QList<vec3d> anchorPosList = RTLSDisplayApplication::client()->getAnchPos();
        int size = anchorPosList.size();
//        if (size > 3){
            ui->showNavigationMode_2->setEnabled(true);
            checkbox2d3d = 3;
//        }
//        else {
//            QMessageBox::warning(NULL,"Taylor初始化失败","Taylor算法需要至少选中4个基站","我知道了");
//            ui->showNavigationMode_4->setCheckState(Qt::CheckState::Checked);
//        }
    }
}

void ViewSettingsWidget::on_showNavigationMode_4_stateChanged(int arg1)//二维
{
    if(arg1 == Qt::CheckState::Checked)
    {
            ui->showNavigationMode_5->setCheckState(Qt::CheckState::Checked);
            ui->showNavigationMode_2->setEnabled(false);

            checkbox2d3d = 2;
    }
}

void ViewSettingsWidget::on_showNavigationMode_3_stateChanged(int arg1)  //一维
{
    if(arg1 == Qt::CheckState::Checked)
    {
            ui->showNavigationMode_2->setEnabled(false);
            ui->showNavigationMode_5->setCheckState(Qt::CheckState::Checked);
            checkbox2d3d = 1;
            //qDebug("2222=%d", checkbox2d3d);
        //     for(int i =0;i<8;i++)
        //    {
        //        myGraphicsWidget->ui->anchorTable->item(i,2)->setText("0.00");
        //        myGraphicsWidget->ui->anchorTable->item(i,2)->setFlags(myGraphicsWidget->ui->anchorTable->item(i,2)->flags() & (~Qt::ItemIsEditable));
        //    }
    }
    // else
    // {
    //         checkbox2d3d = 2;
    //         //qDebug("1111=%d", checkbox2d3d);
    //         ui->showNavigationMode_2->setEnabled(true);
    //     //     for(int i =0;i<8;i++)
    //     //    {
    //     //         myGraphicsWidget->ui->anchorTable->item(i,2)->setFlags(myGraphicsWidget->ui->anchorTable->item(i,2)->flags() | (Qt::ItemIsEditable));

    //     //    }
    // }

}

void ViewSettingsWidget::on_showNavigationMode_2_stateChanged(int arg1)  //3D视图
{

    if(arg1)
    {
        QList<vec3d> anchorPosList = RTLSDisplayApplication::client()->getAnchPos();
        bool coplanar = true;
        double firstZ = anchorPosList.first().z;
        for (const vec3d& pos : anchorPosList) {
            if (pos.z != firstZ) {
                coplanar = false;
                break;
            }
        }
        if (!coplanar && anchorPosList.size()>3){
            myGraphicsWidget->ui->graphicsView->hide();
            initGraph3D(); //初始化散点图
            myGraphicsWidget->ui->stackedWidget->show();
        }
        else{
             QMessageBox::warning(NULL,"3D视图启动失败","基站不能共面，且需要最少勾选4个基站","我知道了");
             ui->showNavigationMode_5->setCheckState(Qt::CheckState::Checked);
        }
    }
    else
    {
        myGraphicsWidget->ui->graphicsView->show();
//        for (QScatter3DSeries *obj : scatter->seriesList()){
//            scatter->removeSeries(obj);
//        }
//        scatter->removeCustomItems();

        myGraphicsWidget->ui->stackedWidget->hide();
    }

}
void ViewSettingsWidget::updateAncAndLabel(int r, bool update=false){
    if(m_labelAnc[r] && scatter->customItems().contains(m_labelAnc[r])){
        scatter->removeCustomItem(m_labelAnc[r]);
        m_labelAnc[r] = nullptr;
    }
    m_labelAnc[r] = new QCustom3DLabel();
    double x = myGraphicsWidget->ui->anchorTable->item(r, 1)->text().toDouble();
    double y = myGraphicsWidget->ui->anchorTable->item(r, 3)->text().toDouble();
    double z = myGraphicsWidget->ui->anchorTable->item(r, 2)->text().toDouble();
    QVector3D position(x,y,z);
    if (update)
        series->dataProxy()->setItem(myitemlist(r), position);
    else
        series->dataProxy()->insertItem(myitemlist(r), position);
    position.setY(position.y()+0.35*ratio);
    QString labelText = QString("ANC %1 (%2, %3, %4)")
            .arg(r).arg(QString::number(x, 'f', 2))
            .arg(QString::number(z, 'f', 2))
            .arg(QString::number(y, 'f', 2));
    qDebug()<<"new pos:"<<position<<"label:"<<labelText;
    m_labelAnc[r]->setText(labelText);
    m_labelAnc[r]->setPosition(position);
//    m_labelAnc[r]->setScaling(QVector3D(.75f*ratio, .75f*ratio, .75f*ratio));
    m_labelAnc[r]->setScaling(QVector3D(.3f*ratio, .3f*ratio, .3f*ratio));
    m_labelAnc[r]->setPositionAbsolute(false);
    m_labelAnc[r]->setFacingCamera(true);
    m_labelAnc[r]->setBorderEnabled(false);
    m_labelAnc[r]->setBackgroundColor(Qt::transparent);
//    scatter->addCustomItem(m_labelAnc[r]);
}
void ViewSettingsWidget::anchorTableChanged(int r, int c)
{

    if (myGraphicsWidget->_busy)
        return;
    if (!myGraphicsWidget->ui->graphicsView->isHidden())
           return;
    disconnect(myGraphicsWidget->ui->anchorTable, SIGNAL(cellChanged(int, int)), this, SLOT(anchorTableChanged(int, int)));

    if (c == 0) {
        if (myGraphicsWidget->ui->anchorTable->item(r, 0)->checkState() == Qt::Checked) {
            updateAncAndLabel(r);//修改基站和标签

            // 更新最大最小值
        } else {
            series->dataProxy()->removeItems(myitemlist(r), 1);
            if (scatter->customItems().contains(m_labelAnc[r])){
                scatter->removeCustomItem(m_labelAnc[r]);
                m_labelAnc[r] = nullptr;
            }


        }
    } else if(c == GraphicsWidget::ColumnX || c == GraphicsWidget::ColumnY || c == GraphicsWidget::ColumnZ ){//修改基站坐标
        if (myGraphicsWidget->ui->anchorTable->item(r, 0)->checkState() == Qt::Checked) {
            updateAncAndLabel(r,true);//修改基站和标签
        }
    }

    // 更新轴范围
    updateAxisRange(false);
    connect(myGraphicsWidget->ui->anchorTable, SIGNAL(cellChanged(int, int)), this, SLOT(anchorTableChanged(int, int)));

}
void ViewSettingsWidget::tagTableClicked(int r, int c)
{
    if(myGraphicsWidget->ui->tagTable->item(r,0)->checkState() == Qt::Checked)
    {
        if (!scatter->customItems().contains(m_labelPoint[r]))
       {
            m_labelPoint[r] = new QCustom3DLabel();
            m_labelPoint[r]->setTextColor(generateColor(r,myGraphicsWidget->ui->tagTable->rowCount()));
        }
    }
    else
    {
        if (scatter->customItems().contains(m_labelPoint[r]))
        {
            scatter->removeCustomItem(m_labelPoint[r]);
        }

    }
}
void ViewSettingsWidget::initGraph3D()
{
    if(inited3d)
        return;
    inited3d =true;
    ratio = myGraphicsWidget->_tagSize/0.3;
    qDebug()<<"数量："<<myGraphicsWidget->ui->tagTable->rowCount();
    maxX = std::numeric_limits<double>::lowest();
    maxY = std::numeric_limits<double>::lowest();
    maxZ = std::numeric_limits<double>::lowest();
    minX = 0;
    minY = 0;
    minZ = 0;
    updateAxisRange(false);

    scatter->activeTheme()->setType(Q3DTheme::ThemeStoneMoss);
    scatter->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftLow);
    scatter->setSelectionMode(QAbstract3DGraph::SelectionNone);
    scatter->activeTheme()->setLabelBackgroundEnabled(false);
    scatter->activeTheme()->setBackgroundEnabled(true);
    QFont font = QFont("Agency FB");
    font.setPointSize(3.0f*ratio);
    scatter->activeTheme()->setFont(font);


    scatter->axisX()->setTitle("X Axis");  // 设置坐标轴标题
    scatter->axisX()->setTitleVisible(true); // 设置标题是否显示
    scatter->axisY()->setTitle("Z Axis");
    scatter->axisY()->setTitleVisible(true);
    scatter->axisZ()->setTitle("Y Axis");
    scatter->axisZ()->setTitleVisible(true);
    scatter->axisX()->setLabelAutoRotation(60);
    scatter->axisY()->setLabelAutoRotation(60);
    scatter->axisZ()->setLabelAutoRotation(60);
    scatter->axisX()->setTitleFixed(true);
    scatter->axisY()->setTitleFixed(true);
    scatter->axisZ()->setTitleFixed(true);

    scatter->scene()->activeCamera()->setXRotation(-18);
    scatter->scene()->activeCamera()->setYRotation(12);
    scatter->addSeries(series);



    // 设置各轴的范围

    series->dataProxy()->removeItems(0,8);

    series->setMesh(QAbstract3DSeries::MeshPyramid);
    series->setItemSize(0.1f*ratio);
    for (int r = 0; r < MAX_ANC_NUM; r++) // 添加基站系列
    {
        if (myGraphicsWidget->ui->anchorTable->item(r, 0)->checkState() == Qt::Checked)
        {
            updateAncAndLabel(r);
        }
    }


    for(int i=0 ; i< MAX_NUM_TAGS; i++ ){
        seriesPoint[i]->setMesh(QAbstract3DSeries::MeshPyramid);
        seriesPoint[i]->setMeshSmooth(false);
        seriesPoint[i]->setItemSize(0.05f*ratio);
    }

}
void ViewSettingsWidget::updateAxisRangeWithTag(double x,double y,double z){
//    for(int i=0;i<myGraphicsWidget->ui->tagTable->rowCount();i++){
        QVector3D itemPos(x,z,y);
        // 更新最大值和最（下，有，小值
        if ( itemPos.x() > maxX) maxX = itemPos.x();
        if ( itemPos.y() > maxY) maxY = itemPos.y();
        if ( itemPos.z() > maxZ) maxZ = itemPos.z();
        if ( itemPos.x() < minX) minX = itemPos.x();
        if ( itemPos.y() < minY) minY = itemPos.y();
        if ( itemPos.z() < minZ) minZ = itemPos.z();
//    }
    updateAxisRange(true);
}
void ViewSettingsWidget::updateAxisRange(bool tag=false)
{
    if (!tag)
       {
           // 重置最大最小值
           maxX = std::numeric_limits<double>::lowest();
           maxY = std::numeric_limits<double>::lowest();
           maxZ = std::numeric_limits<double>::lowest();
           minX = std::numeric_limits<double>::max();
           minY = std::numeric_limits<double>::max();
           minZ = std::numeric_limits<double>::max();
       }

       for (int i = 0; i < myGraphicsWidget->ui->anchorTable->rowCount(); ++i) {
           if (myGraphicsWidget->ui->anchorTable->item(i, 0)->checkState() == Qt::Checked)
           {
               double x = myGraphicsWidget->ui->anchorTable->item(i, 1)->text().toDouble();
               double y = myGraphicsWidget->ui->anchorTable->item(i, 3)->text().toDouble();
               double z = myGraphicsWidget->ui->anchorTable->item(i, 2)->text().toDouble();
               QVector3D itemPos(x, y, z);

               // 更新最大值和最小值
               if (itemPos.x() > maxX) maxX = itemPos.x();
               if (itemPos.y() > maxY) maxY = itemPos.y();
               if (itemPos.z() > maxZ) maxZ = itemPos.z();
               if (itemPos.x() < minX) minX = itemPos.x();
               if (itemPos.y() < minY) minY = itemPos.y();
               if (itemPos.z() < minZ) minZ = itemPos.z();
           }
       }

       if (!tag)
       {
           double margin = 0.5 * ratio;  // 额外边距
           maxX += margin;
           maxY += margin;
           maxZ += margin;
           minX -= margin;
           minY -= margin;
           minZ -= margin;
       }

       // 修正比例计算以适配负数
       double rangeX = maxX - minX;
       double rangeY = maxY - minY;
       double rangeZ = maxZ - minZ;

       // 确保比例和范围正确
       bool updateRange = false;
       if (scatter->axisX()->min() != minX || scatter->axisX()->max() != maxX) {
           scatter->axisX()->setRange(minX, maxX);
           updateRange = true;
       }
       if (scatter->axisY()->min() != minY || scatter->axisY()->max() != maxY) {
           scatter->axisY()->setRange(minY, maxY);
           updateRange = true;
       }
       if (scatter->axisZ()->min() != minZ || scatter->axisZ()->max() != maxZ) {
           scatter->axisZ()->setRange(minZ, maxZ);
           updateRange = true;
       }

       if (updateRange)
       {
           double horizontalAspectRatio = rangeX / rangeZ;  // 水平比例，范围固定为正值
           if (horizontalAspectRatio < 0) horizontalAspectRatio = -horizontalAspectRatio;

           scatter->setHorizontalAspectRatio(horizontalAspectRatio);  // 设置水平纵横比
           scatter->setAspectRatio(rangeX > rangeZ ? rangeX / rangeY : rangeZ / rangeY);  // 设置 3D 比例
//       }
//        ratio = std::min(scatter->horizontalAspectRatio(),scatter->aspectRatio());

//        QFont font = QFont("Agency FB");
//        font.setPointSize(5.0f*ratio);
//        scatter->activeTheme()->setFont(font);
        qDebug()<<"h ratio:"<<scatter->horizontalAspectRatio()<<"ratio"<<scatter->aspectRatio();
        qDebug()<<"true ratio"<<ratio;
//        series->setItemSize(0.5f*ratio);
//        for (QCustom3DItem *item : scatter->customItems()){
//            item->setScaling(QVector3D(0.75f*ratio,0.75f*ratio,0.75f*ratio));
//        }
    }

}

void ViewSettingsWidget::tag3DPos(quint64 tagId, double x, double y, double z)
{
    if(ui->showNavigationMode_2->checkState() == Qt::Checked)
    {
        if(myGraphicsWidget->ui->tagTable->rowCount()>0)
        {
            for(int i=0;i<myGraphicsWidget->ui->tagTable->rowCount();i++)
            {
                scatter->addSeries(seriesPoint[i]);
                seriesPoint[i]->setBaseColor(generateColor(i,myGraphicsWidget->ui->tagTable->rowCount()));
            }
        }

        if(ui->showTagHistory->checkState() == Qt::Checked) //打开历史轨迹
        {
            for(int i=0;i<myGraphicsWidget->ui->tagTable->rowCount();i++)
            {
                Tag* tag = RTLSDisplayApplication::graphicsWidget()->_tags.value(tagId, NULL);
                int ui_tag_id = tag->ridx;
                if(ui_tag_id == i)
                {
                    seriesPoint[i]->dataProxy()->insertItem(0,QVector3D(x,z,y));
                    if(seriesPoint[i]->dataProxy()->itemCount()>=ui->tagHistoryN->text().toInt())
                    {
                        seriesPoint[i]->dataProxy()->removeItems(ui->tagHistoryN->text().toInt(),1);
                    }
                }
            }

        }
        else{

            for(int i=0;i<myGraphicsWidget->ui->tagTable->rowCount();i++)
            {
                Tag* tag = RTLSDisplayApplication::graphicsWidget()->_tags.value(tagId, NULL);
                int ui_tag_id = tag->ridx;
                if(ui_tag_id == i)
                {
                    seriesPoint[i]->dataProxy()->removeItems(0,ui->tagHistoryN->text().toInt());
                    seriesPoint[i]->dataProxy()->insertItem(0,QVector3D(x,z,y));
                }

            }
        }

        if(myGraphicsWidget->ui->graphicsView->isHidden())
        {
            for(int i=0;i<myGraphicsWidget->ui->tagTable->rowCount();i++)
            {
                if(myGraphicsWidget->ui->tagTable->item(i,0)->checkState() == Qt::Checked)
                {


                    Tag* tag = RTLSDisplayApplication::graphicsWidget()->_tags.value(tagId, NULL);
                    int ui_tag_id = tag->ridx;
                    if(ui_tag_id == i)
                    {
                        QVector3D pos = seriesPoint[i]->dataProxy()->itemAt(0)->position();
    //                    QString labelText = QString("Tag %1").arg(tagId);
                        QString labelText =myGraphicsWidget->ui->tagTable->item(i,0)->text();
                        m_labelPoint[i]->setText(labelText);
                        m_labelPoint[i]->setBorderEnabled(false);
                        m_labelPoint[i]->setBackgroundColor(Qt::transparent);
                        pos.setY(pos.y()-0.25*ratio);
                        m_labelPoint[i]->setPosition(pos);
                        m_labelPoint[i]->setScaling(QVector3D(.75f*ratio, .75f*ratio, .75f*ratio));
                        m_labelPoint[i]->setPositionAbsolute(false);
                        m_labelPoint[i]->setFacingCamera(true);
                        scatter->addCustomItem(m_labelPoint[i]);
                    }
                }
                else
                {
                    // qDebug()<<"size:::"<< scatter->customItems().size();
                }
            }

        }
        updateAxisRangeWithTag(x,y,z);
    }
}

void ViewSettingsWidget::setCheckCanvesAreaState(bool state)
{
    if(state)
    {
        ui->checkCanvesArea->setCheckState(Qt::Checked);
    }
    else
    {
        ui->checkCanvesArea->setCheckState(Qt::Unchecked);
    }
}

int ViewSettingsWidget::myitemlist(int length)
{
   int num =0;

   for(int i=0;i<length;i++)
   {
       if(myGraphicsWidget->ui->anchorTable->item(i,0)->checkState() == Qt::Checked)
      {
           num=num+1;
      }
   }

   return num;
}
bool ViewSettingsWidget::event(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        ui->filtering->clear();
        if(language)
        {
          ui->filtering->addItems(RTLSDisplayApplication::client()->getLocationFilters());
        }
        else{
          ui->filtering->addItems(RTLSDisplayApplication::client()->getLocationFilters());
        }
    }
    return QWidget::event(event);
}
void ViewSettingsWidget::udpSendData()
{
/*
    if((myGraphicsWidget->ui->tagTable->rowCount()==0))
    {

    }
    else{
         for(int i=0;i<myGraphicsWidget->ui->tagTable->rowCount();i++)
       {
           m_json.insert("Z", myGraphicsWidget->ui->tagTable->item(i,3)->text());
           m_json.insert("Y", myGraphicsWidget->ui->tagTable->item(i,2)->text());
           m_json.insert("X", myGraphicsWidget->ui->tagTable->item(i,1)->text());
           m_json.insert("TagID", myGraphicsWidget->ui->tagTable->item(i,0)->text());
           m_json.insert("Command", "UpLink");
       }
    }
      QJsonDocument _jsonDoc(m_json);
      QByteArray _byteArray = _jsonDoc.toJson(QJsonDocument::Indented);
      QString strTmp = QString(_jsonDoc.toJson(QJsonDocument::Compact));
     // qDebug() << strTmp;

      QString IP =ui->LE_ip_address->text();
      int comID =ui->LE_com->text().toInt();
      udpSocket->writeDatagram(_byteArray,QHostAddress(IP),comID);
*/
}
void ViewSettingsWidget::on_BTN_sendSocket_clicked()
{
    LE_ip_addressText=ui->LE_ip_address->text();
    LE_comTexttoInt=ui->LE_com->text().toInt();
    QRegExp rx_ip("^((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)$");
    bool match = rx_ip.exactMatch(ui->LE_ip_address->text());

    if (!match || ui->LE_com->text().isEmpty())
    {
        if(language)
      {
         QMessageBox:: critical(NULL, QStringLiteral("错误"), QStringLiteral("请输入正确的IP地址及端口号"),QMessageBox:: Yes , QMessageBox:: Yes) ;
      }
      else{
         QMessageBox:: critical(NULL, QStringLiteral("Error"), QStringLiteral("Please enter the correct IP address and port number"),QMessageBox:: Yes , QMessageBox:: Yes) ;
      }
   }
    else
    {
//           connect(&timer200ms,SIGNAL(timeout()),this,SLOT(udpSendData()));
//           timer200ms.start(200);
       if(ui->BTN_sendSocket->text()=="发送"||ui->BTN_sendSocket->text()=="Send")
       {
           if(language)
           {
              ui->BTN_sendSocket->setText("停止");
              BTN_sendSocketText="停止";

           }
           else
           {
              ui->BTN_sendSocket->setText("Stop");
              BTN_sendSocketText="Stop";
           }

       }
       else if(ui->BTN_sendSocket->text()=="停止"||ui->BTN_sendSocket->text()=="Stop")
       {
            if(language)
            {
               ui->BTN_sendSocket->setText("发送");
               BTN_sendSocketText="发送";
            }
            else{
               ui->BTN_sendSocket->setText("Send");
               BTN_sendSocketText="Send";
            }

          //disconnect(&timer200ms,SIGNAL(timeout()),this,SLOT(udpSendData()));
       }
    }

}

void ViewSettingsWidget::showExplainBtnClicked()
{
    QString okText,titleText,setText;
    if(language)
    {
       okText = "确定";
       titleText = "一键测绘使用说明";
       setText = "一键测绘功能可快速测量基站间各点距离以及面积。\n最少在选择框内勾选2个基站启动测绘功能，3个及以上基站可测得面积";
    }
    else
    {
       okText = "OK";
       titleText = "Instructions";
       setText = "It can quickly measure the distance and area of each point between the base stations. Check at least 2 base stations in the selection box to start the mapping function, and 3 or more base stations can measure the area.";
    }
    QPushButton *okbtn = new QPushButton(okText);
    QMessageBox *mymsgbox = new QMessageBox;
    mymsgbox->setIcon(QMessageBox::Warning);
    mymsgbox->setWindowTitle(titleText);
    mymsgbox->setText(setText);
    mymsgbox->addButton(okbtn, QMessageBox::AcceptRole);
    mymsgbox->show();
    mymsgbox->exec();//阻塞等待用户输入
}

void ViewSettingsWidget::onCadToPngClicked()
{
    // 1. 选择 DXF 文件
    QString dxfPath = QFileDialog::getOpenFileName(this, "打开 CAD 文件", QString(), "CAD 文件 (*.dxf)");
    if (dxfPath.isNull()) return;

    // 2. 确定输出 PNG 路径（同目录，同名）
    QFileInfo fi(dxfPath);
    QString pngPath = fi.absolutePath() + "/" + fi.baseName() + ".png";

    // 3. 找 Python 脚本
    QString scriptPath = QCoreApplication::applicationDirPath() + "/tools/cad_to_png.py";
    if (!QFile::exists(scriptPath)) {
        // 开发时从源码目录找
        scriptPath = QDir::currentPath() + "/tools/cad_to_png.py";
    }

    ui->cadToPngBtn->setEnabled(false);
    ui->cadToPngBtn->setText("转换中...");

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, pngPath](int exitCode, QProcess::ExitStatus) {
        ui->cadToPngBtn->setEnabled(true);
        ui->cadToPngBtn->setText("CAD(DXF) → PNG 转换...");

        if (exitCode == 0 && QFile::exists(pngPath)) {
            // 自动加载为底图
            if (applyFloorPlanPic(pngPath) == 0) {
                RTLSDisplayApplication::viewSettings()->floorplanShow(true);
                RTLSDisplayApplication::viewSettings()->setFloorplanPath(pngPath);
                QMessageBox::information(this, "转换成功",
                    QString("已生成底图：\n%1\n\n请在底图与栅格面板中设置缩放和偏移以完成标定。").arg(pngPath));
            }
        } else {
            QString errMsg = proc->readAllStandardError();
            QMessageBox::critical(this, "转换失败",
                QString("CAD → PNG 转换失败（退出码 %1）\n\n请确认已安装 ezdxf 和 matplotlib：\n  pip install ezdxf matplotlib\n\n错误信息：\n%2")
                .arg(exitCode).arg(errMsg));
        }
        proc->deleteLater();
    });

    proc->start("python3", QStringList() << scriptPath << dxfPath << pngPath);
    if (!proc->waitForStarted(3000)) {
        ui->cadToPngBtn->setEnabled(true);
        ui->cadToPngBtn->setText("CAD(DXF) → PNG 转换...");
        QMessageBox::critical(this, "错误", "无法启动 python3，请确认 Python 3 已安装并在 PATH 中。");
        proc->deleteLater();
    }
}





