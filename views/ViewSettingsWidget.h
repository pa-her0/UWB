// -------------------------------------------------------------------------------------------------------------------
//
//  File: ViewSettingsWidget.h
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef VIEWSETTINGSWIDGET_H
#define VIEWSETTINGSWIDGET_H

#include <QWidget>
#include <QtNetwork>
#include <QMessageBox>
#include <QUdpSocket>
#include <QTimer>
#include <QTime>
#include <QJsonObject>
#include <QJsonDocument>

#include "SerialConnection.h"
#include "ui_ViewSettingsWidget.h"
#include "ui_GraphicsWidget.h"
#include <QDomElement>         //新增
#include "GraphicsWidget.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QButtonGroup>
#include <QtDataVisualization>
#include <QtCharts>
#include <QTranslator>
#include "mainwindow.h"
#include "RTLSClient.h"
#include "Custom3DScatter.h"
class GraphicsWidget;
extern GraphicsWidget *myGraphicsWidget;
extern bool serial_connected;
using namespace QtDataVisualization;
namespace Ui {
class ViewSettingsWidget;
}

class ViewSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ViewSettingsWidget(QWidget *parent = 0);
    ~ViewSettingsWidget();

    int applyFloorPlanPic(const QString &path);
    static ViewSettingsWidget *viewsettingswidget;//!!!!!!!!!指针类型静态成员变量

   // void showCN_EH();
public:

   // SerialConnection *myCom;
   bool event(QEvent *event);//语言切换响应事件
public slots:
    void connectionStateChanged(SerialConnection::ConnectionState state);
    void serialError();
    int updateDeviceList();
    void setLinetext(QString s);
    void tag3DPos(quint64 tagId, double x, double y, double z);
    void setCheckCanvesAreaState(bool state);
signals:
    void saveViewSettings(void);

   // void SystemResponse();//系统响应
protected slots:
    void onReady();

    void enableAutoPositioning(int enable);

    void floorplanOpenClicked();
    void updateLocationFilter(int index);
    void enableFiltering(void);
    void originClicked();
    void scaleClicked();

    void gridShowClicked();
    void originShowClicked();
    void tagHistoryShowClicked();

    void saveFPClicked();
    void tagAncTableShowClicked();
    void checkCanvesAreaClicked(int state);
    void canvesFontSizeValueChanged(int value);
    void checkCanvesInfoClicked(int state);
    void useAutoPosClicked();
    void showGeoFencingModeClicked();
    void showNavigationModeClicked();
    void alarmSetClicked();

    void zone1ValueChanged(double);
    void zone2ValueChanged(double);
    void zone1EditFinished(void);
    void zone2EditFinished(void);
    void tagHistoryNumberValueChanged(int);

    void showOriginGrid(bool orig, bool grid);
    void getFloorPlanPic(void);
    void showSave(bool);
    void leIpAddrChanged(QString);
    void comNumberChanged(int);

    void setTagHistory(int h);
    void loggingClicked(void);
    void connectButtonClicked();
    void on_Tag_size_valueChanged(double arg1);


public:

    Ui::ViewSettingsWidget *ui;

private slots:
    void dataupdate();
    void on_pushButton_clicked();
    void on_socket_pb_clicked(bool checked);
    void on_socket_pb_udp_clicked(bool checked);
    void on_BTN_getdata_clicked();



    void on_BTN_addr_clicked();
    void Settimeout();
    void on_BTN_ant_dly_ok_clicked();

    void on_lE_tx_power_ok_clicked();
    void on_BTN_Restore_clicked();

    void on_BTN_jizhan_clicked();


    void on_BTN_tongbu_clicked();

    void on_showNavigationMode_3_stateChanged(int arg1);
    void on_showNavigationMode_stateChanged(int arg1);
    void on_showNavigationMode_4_stateChanged(int arg1);
    void on_showNavigationMode_2_stateChanged(int arg1);

    void anchorTableChanged(int r, int c);
    void tagTableClicked(int r, int c);
    void on_BTN_sendSocket_clicked();
    void showExplainBtnClicked();
    void onCadToPngClicked();

    void udpSendData();
private:
    SerialConnection mySerialConnection;
    SerialConnection::ConnectionState _state;

    bool _logging ;
    bool _floorplanOpen ;
    bool _enableAutoPos;
    bool fucStatus;
    int _selected;
    QTimer *timer;
    QStringList oldPortStringList;
    QStringList newPortStringList;

    QTimer *m_setTimer;


    // 最大和最小值
    double maxX;
    double maxY;
    double maxZ;
    double minX;
    double minY;
    double minZ;
    double rangeX;
    double rangeY;
    double rangeZ;
    double ratio=1;
    bool inited3d =false;
    void updateAxisRange(bool tag);
    void updateAxisRangeWithTag(double x,double y,double z);
    void updateAncAndLabel(int r,bool update);
    //QAbstract3DGraph * create3DScatterGraph();
    int  myitemlist(int length);
    void initGraph3D();  //初始化
    QList<QAbstract3DGraph *>   m_graphLsit;    // 图表容器指针

    //为实现数据共享，公共化参数
    Custom3DScatter *scatter;
    QScatter3DSeries *series;  //散点序列
    QScatterDataArray *dataArray;
    QScatterDataItem *ptrToDataArray;
    QWidget *container;

    QScatter3DSeries *seriesPoint[MAX_NUM_TAGS];  //散点序列


    QTranslator translator;
    QUdpSocket *udpSocket;
    //QJsonObject m_json;
    QTimer timer200ms;

    QtDataVisualization::Q3DSurface *m_surface;
 //   QtDataVisualization::QCustom3DLabel *m_label0,*m_label1,*m_label2,*m_label3,*m_label4,*m_label5;
    QtDataVisualization::QCustom3DLabel *m_labelPoint[MAX_NUM_TAGS];
    QtDataVisualization::QCustom3DLabel *m_labelAnc[MAX_NUM_ANCS];


};

#endif // VIEWSETTINGSWIDGET_H
