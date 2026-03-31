// -------------------------------------------------------------------------------------------------------------------
//
//  File: SerialConnection.h
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef SERIALCONNECTION_H
#define SERIALCONNECTION_H
#include "ui_ViewSettingsWidget.h"

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include "RTLSDisplayApplication.h"
#include <QMessageBox>
#include <QLineEdit>
#include <QTimer>
#include <QUdpSocket>
//extern QString Rev_$setok;
;
/**
* @brief SerialConnection
*        Constructor, it initialises the Serial Connection its parts
*        it is used for managing the COM port connection.
*/
class SerialConnection : public QObject
{
    Q_OBJECT
public:
    explicit SerialConnection(QObject *parent = 0);
    ~SerialConnection();


   // Ui::ViewSettingsWidget *Viewui;
    void showLineEditText(QLineEdit *lineedit,QString text);

    enum ConnectionState
    {
        Disconnected = 0,
        Connecting,
        Connected,
        ConnectionFailed
    };

    void findSerialDevices(); //find any tags or anchors that are connected to the PC

    int openSerialPort(QSerialPortInfo x); //open selected serial port

    QStringList portsList(); //return list of available serial ports (list of ports with tag/anchor connected)

    QSerialPort* serialPort() { return _serial; }

signals:
    void dataupdate();
    void clearTags();
    void serialError(void);
    void getCfg(void);
    void nextCmd(void);

    void statusBarMessage(QString status);
    void connectionStateChanged(SerialConnection::ConnectionState);
    void serialOpened(QString, QString);
    void resetdata(QString);
    void OnUdpConnect();
    void OffUdpConnect();
public slots:

    void closeConnection();
    void cancelConnection();
    int  openConnection(int index);
    void readData(void);

    // socket 通信api
    void netConnect(int port);
    void netClose();
    void server_New_Connect();
    void socket_Read_Data();
    void socket_Disconnected();
    void writeData(const QByteArray &data);

    //udpsocket
    void UdpnetConnect(int port);
    void UdpnetClose();

protected slots:
   // void writeData(const QByteArray &data);
    void handleError(QSerialPort::SerialPortError error);

public:
    QTcpServer*     server;
    QTcpSocket*     socket;
    QSerialPort *_serial;

    Ui::ViewSettingsWidget *ViewSettingsWidgetUi;
    void updatedata();

private:

    //QSerialPort *_serial;

    QList<QSerialPortInfo>	_portInfo ;
    QStringList _ports;

    QList<QByteArray> _cmdList ;

    QString _connectionVersion;
    QString _conncectionConfig;

    QString mysave;
    QString dataRevfuc(QString data,QString keyword,int location,int length);
    QString dataRevfuc1(QString data,QString keyword,int location,int length,int i);
public:
    QTimer *m_setTimer;
    bool _processingData;
    bool rbootdata;
    static QString model;
    static QString firmware;
    static QString role;
    static QString addr;
    static QString blink;
    static QString max_anc_num;
    static QString max_tag_num;
    static QString uwb_data_rate;
    static QString uwb_data_rate1;
    static QString uwb_data_rate2;
    static QString channel;
    static QString update_frequency;
    static QString update_Period;
    static QString kalmanfilter;
    static QString ant_dly;
    static QString tx_power;
    static QString use_ext_eeprom;
    static QString use_imu;
    static QString group_id;
    static QString A0_x;
    static QString A0_y;
    static QString A0_z;
    static QString A1_x;
    static QString A1_y;
    static QString A1_y1;
    static QString A1_y2;
    static QString A1_z;
    static QString A2_x;
    static QString A2_y;
    static QString A2_z;
    static QString A3_x;
    static QString A3_y;
    static QString A3_z;
    static QString Rev_$setok;

    QUdpSocket *udpSocket;

};

#endif // SERIALCONNECTION_H
