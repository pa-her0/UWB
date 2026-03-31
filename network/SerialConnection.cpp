
// -------------------------------------------------------------------------------------------------------------------
//
//  File: SerialConnection.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------
#include "ui_ViewSettingsWidget.h"

#include "SerialConnection.h"
#include "mainwindow.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <QMessageBox>
#include "RTLSClient.h"
QString SerialConnection::model;
QString SerialConnection::firmware;
QString SerialConnection::role;
QString SerialConnection::addr;
QString SerialConnection::blink;
QString SerialConnection::max_anc_num;
QString SerialConnection::max_tag_num;
QString SerialConnection::uwb_data_rate;
QString SerialConnection::uwb_data_rate1;
QString SerialConnection::uwb_data_rate2;
QString SerialConnection::channel;
QString SerialConnection::update_frequency;
QString SerialConnection::update_Period;
QString SerialConnection::kalmanfilter;
QString SerialConnection::ant_dly;
QString SerialConnection::tx_power;
QString SerialConnection::use_ext_eeprom;
QString SerialConnection::use_imu;
QString SerialConnection::group_id;
QString SerialConnection::A0_x;
QString SerialConnection::A0_y;
QString SerialConnection::A0_z;
QString SerialConnection::A1_x;
QString SerialConnection::A1_y;
QString SerialConnection::A1_y1;
QString SerialConnection::A1_y2;
QString SerialConnection::A1_z;
QString SerialConnection::A2_x;
QString SerialConnection::A2_y;
QString SerialConnection::A2_z;
QString SerialConnection::A3_x;
QString SerialConnection::A3_y;
QString SerialConnection::A3_z;
QString SerialConnection::Rev_$setok;
#define INST_REPORT_LEN   (106)
#define INST_REPORT_LEN_HEADER (2)
#define INST_VERSION_LEN  (16)
#define INST_CONFIG_LEN   (1)

QString port_name;

SerialConnection::SerialConnection(QObject *parent) :
    QObject(parent)
{
    _serial = new QSerialPort(this);

    connect(_serial, SIGNAL(error(QSerialPort::SerialPortError)), this,
            SLOT(handleError(QSerialPort::SerialPortError)));

    connect(_serial, SIGNAL(readyRead()), this, SLOT(readData()));

    _processingData = true;

    //qDebug() << "SerialConnection _processingData=" << _processingData;


 //   m_setTimer =new QTimer;
    udpSocket = new QUdpSocket(this);
}

SerialConnection::~SerialConnection()
{
    if(_serial->isOpen())
        _serial->close();

    delete _serial;
}

QStringList SerialConnection::portsList()
{
    return _ports;
}

void SerialConnection::findSerialDevices()
{
    _portInfo.clear();
    _ports.clear();

    foreach (const QSerialPortInfo &port, QSerialPortInfo::availablePorts())
    //for (QSerialPortInfo port : QSerialPortInfo::availablePorts())
    {
        //Their is some sorting to do for just list the port I want, with vendor Id & product Id
        /*
        qDebug() << port.portName() << port.vendorIdentifier() << port.productIdentifier()
                 << port.hasProductIdentifier() << port.hasVendorIdentifier() << port.isBusy()
                 << port.manufacturer() << port.description();
*/

        //if(port.description() == DEVICE_STR)
        //{
            _portInfo += port;
            _ports += port.portName();
        //}
    }
}

int SerialConnection::openSerialPort(QSerialPortInfo x)
{
    int error = 0;
    _serial->setPort(x);

    if(!_serial->isOpen())
    {
        if (_serial->open(QIODevice::ReadWrite))
        {
            _serial->setBaudRate(QSerialPort::Baud115200/*p.baudRate*/);
            _serial->setDataBits(QSerialPort::Data8/*p.dataBits*/);
            _serial->setParity(QSerialPort::NoParity/*p.parity*/);
            _serial->setStopBits(QSerialPort::OneStop/*p.stopBits*/);
            _serial->setFlowControl(QSerialPort::NoFlowControl /*p.flowControl*/);

           // qDebug() << "Connected to";
            emit statusBarMessage(tr("Connected to %1").arg(x.portName()));
            port_name = x.portName();
            writeData("$rboot\r\n");
            rbootdata = true;

            model              ="";
            firmware           ="";
            role               ="";
            addr               ="";
            blink              ="";
            max_anc_num        ="";
            max_tag_num        ="";
            uwb_data_rate      ="";
            channel            ="";
            update_frequency   ="";
            update_Period      ="";
            kalmanfilter       ="";
            ant_dly            ="";
            tx_power           ="";
            use_ext_eeprom     ="";
            use_imu            ="";
            group_id           ="";
            A0_x               ="";
            A0_y               ="";
            A0_z               ="";
            A1_x               ="";
            A1_y               ="";
            A1_z               ="";
            A2_x               ="";
            A2_y               ="";
            A2_z               ="";
            A3_x               ="";
            A3_y               ="";
            A3_z               ="";
            updatedata();            
            //emit serialOpened(); - wait until we get reply from the unit
        }
        else
        {
            //QMessageBox::critical(NULL, tr("Error"), _serial->errorString());
            emit statusBarMessage(tr("Open error"));
            if(language)
            {
               QMessageBox::warning(NULL,"提示","串口打开失败,请检查是否被占用","关闭");
            }
            else{
               QMessageBox::warning(NULL,"Prompt", "Serial port opening failed, please check whether it is occupied", "Close");
            }
          //  qDebug() << "Serial error: " << _serial->error();
            _serial->close();
            emit serialError();
            error = 1;
        }
    }
    else
    {

        qDebug() << "port already open!";
        _serial->close();
        emit serialError();
        error = 0;
        if(!_serial->isOpen())
        {
            if (_serial->open(QIODevice::ReadWrite))
            {
                _serial->setBaudRate(QSerialPort::Baud115200/*p.baudRate*/);
                _serial->setDataBits(QSerialPort::Data8/*p.dataBits*/);
                _serial->setParity(QSerialPort::NoParity/*p.parity*/);
                _serial->setStopBits(QSerialPort::OneStop/*p.stopBits*/);
                _serial->setFlowControl(QSerialPort::NoFlowControl /*p.flowControl*/);


                emit statusBarMessage(tr("Connected to %1").arg(x.portName()));

                //qDebug() << "send \"decA$\"" ;
              //  writeData("decA$");
               // writeData("deca?");

                //emit serialOpened(); - wait until we get reply from the unit

            }
            else
            {
                //QMessageBox::critical(NULL, tr("Error"), _serial->errorString());
                emit statusBarMessage(tr("Open error"));
                if(language)
                {
                   QMessageBox::warning(NULL,"提示","串口打开失败,请检查是否被占用","关闭");
                }
                else{
                   QMessageBox::warning(NULL,"Prompt", "Serial port opening failed, please check whether it is occupied", "Close");
                }
               // qDebug() << "Serial error: " << _serial->error();
                _serial->close();
                emit serialError();
                error = 1;
            }
        }
    }

    return error;
}

int SerialConnection::openConnection(int index)
{
    QSerialPortInfo x;
    int foundit = -1;
    int open = false;

    foreach (const QSerialPortInfo &port, QSerialPortInfo::availablePorts())
    {
        //if(port.description() == DEVICE_STR)
        //{
            foundit++;
            if(foundit==index)
            {
                x = port;
                open = true;
                break;
            }
        //}
    }

   // qDebug() << "is busy? " << x.isBusy() << "index " << index << " = found " << foundit;

    if(!open) return -1;


  //  qDebug() << "open serial port " << index << x.portName();

    //open serial port
    return openSerialPort(x);
}

void SerialConnection::closeConnection()
{
    _serial->close();
    qDebug() << "COM port Disconnected";
    emit statusBarMessage(tr("COM port Disconnected"));
    emit connectionStateChanged(Disconnected);
    model              ="";
    firmware           ="";
    role               ="";
    addr               ="";
    blink              ="";
    max_anc_num        ="";
    max_tag_num        ="";
    uwb_data_rate      ="";
    channel            ="";
    update_frequency   ="";
    update_Period      ="";
    kalmanfilter       ="";
    ant_dly            ="";
    tx_power           ="";
    use_ext_eeprom     ="";
    use_imu            ="";
    group_id           ="";
    A0_x               ="";
    A0_y               ="";
    A0_z               ="";
    A1_x               ="";
    A1_y               ="";
    A1_z               ="";
    A2_x               ="";
    A2_y               ="";
    A2_z               ="";
    A3_x               ="";
    A3_y               ="";
    A3_z               ="";
    updatedata(); 
    _processingData = true;
}

void SerialConnection::writeData(const QByteArray &data)
{
    if(_serial->isOpen())
    {
        _serial->write(data);

        //waitForData = true;
    }
    else
    {
        qDebug() << "not open - can't write?";
    }

    emit connectionStateChanged(Connected);
}


void SerialConnection::cancelConnection()
{
    emit connectionStateChanged(ConnectionFailed);
}


void SerialConnection::readData(void)
{
    if(_processingData || rbootdata)
    {
        QByteArray data;
        data = _serial->readAll();

     //   qDebug() << "data=" << data;

        if(data.contains("$setok"))
        {
            mysave.clear();
            Rev_$setok = data.mid(data.indexOf("$setok"),6);
//            qDebug()<<"SerialConnection::Rev_$setok::"<<Rev_$setok;
        }
       
        else if(data.contains("DEVICE") || data.contains("ONFIG") || data.contains("odle") ||
           data.contains("role") || data.contains("max_anc_num") || data.contains("ag_num") ||
           data.contains("channel") || data.contains("period") || data.contains("ime") ||
           data.contains("ant_dly") || data.contains("use_ext_eeprom") || data.contains("group_id") ||
           data.contains("A0") || data.contains("A1") || data.contains("A1") || data.contains("A3") ||
           data.contains(")\r\n**") || data.contains("*") ||
           data.contains("END") )
        {
            mysave += data;
        }

        else if(data.contains("mc ") || data.contains("mi,"))
        {
            rbootdata = false;
            _processingData = false;
            emit serialOpened(_connectionVersion, _conncectionConfig);
        }

        //qDebug()<<"长度：："<<mysave.length();
        //if(mysave.length()>=512 || ( mysave.length()>=384 && dataRevfuc(mysave,"role",7,6)=="ANCHOR"))//==512
        if(mysave.contains("END"))
        {
            qDebug()<<"mysave ="<<mysave;
            model              =dataRevfuc(mysave,"model",8,5);
            firmware           =dataRevfuc(mysave,"firmware",11,3);
            role               =dataRevfuc(mysave,"role",7,6);
            addr               =dataRevfuc(mysave,"addr",7,3);
            blink              =dataRevfuc(mysave,"blink",8,1);
            max_anc_num        =dataRevfuc(mysave,"max_anc_num",14,1);
            max_tag_num        =dataRevfuc(mysave,"max_tag_num",14,2);
            uwb_data_rate      =dataRevfuc(mysave,"uwb_data_rate",16,4);
            channel            =dataRevfuc(mysave,"channel",10,3);
            update_frequency   =dataRevfuc(mysave,"update_frequency",strlen("update_frequency")+3,5);
            update_Period      =dataRevfuc(mysave,"update_Period",strlen("update_Period")+3,6);
            kalmanfilter       =dataRevfuc(mysave,"kalmanfilter",15,1);
            ant_dly            =dataRevfuc(mysave,"ant_dly",10,5);
            tx_power           =dataRevfuc(mysave,"tx_power",11,8);
            use_ext_eeprom     =dataRevfuc(mysave,"use_ext_eeprom",17,1);
            use_imu            =dataRevfuc(mysave,"use_imu",10,1);
            group_id           =dataRevfuc(mysave,"group_id",11,3);

            A0_x               =dataRevfuc1(mysave,"A0(",3,21,0);
            A0_y               =dataRevfuc1(mysave,"A0(",3,21,1);
            A0_z               =dataRevfuc1(mysave,"A0(",3,21,2);
            A1_x               =dataRevfuc1(mysave,"A1(",3,21,0);
            A1_y               =dataRevfuc1(mysave,"A1(",3,21,1);
            A1_z               =dataRevfuc1(mysave,"A1(",3,21,2);
            A2_x               =dataRevfuc1(mysave,"A2(",3,21,0);
            A2_y               =dataRevfuc1(mysave,"A2(",3,21,1);
            A2_z               =dataRevfuc1(mysave,"A2(",3,21,2);
            A3_x               =dataRevfuc1(mysave,"A3(",3,21,0);
            A3_y               =dataRevfuc1(mysave,"A3(",3,21,1);
            A3_z               =dataRevfuc1(mysave,"A3(",3,21,2);
       
            rbootdata = false;
            _processingData = false;
            emit serialOpened(_connectionVersion, _conncectionConfig);
            mysave.clear();
            updatedata();
        }

        
    }
}

void SerialConnection::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        //QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        _serial->close();

        _processingData = true;
    }
}


// socket api  实现

void SerialConnection::netConnect(int port)
{

    server = new QTcpServer();
    connect(server,&QTcpServer::newConnection,this,&SerialConnection::server_New_Connect);

    //监听指定的端口
    if(!server->listen(QHostAddress::Any, port))
    {
        //若出错，则输出错误信息
        qDebug()<<"netConnect error"<<server->errorString();
        return;
    }
   // qDebug()<< "Listen succeessfully!";
}

void SerialConnection::netClose(){
    //取消侦听
    server->deleteLater();
    server->close();
    server = NULL;
}
void SerialConnection::UdpnetConnect(int port)
{
    udpSocket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress);
    emit OnUdpConnect();


}

void SerialConnection::UdpnetClose()
{
   udpSocket->close();
 //  emit OffUdpConnect();

}
void SerialConnection::server_New_Connect()
{
    //获取客户端连接
    socket = server->nextPendingConnection();
    //连接QTcpSocket的信号槽，以读取新数据
    QObject::connect(socket, &QTcpSocket::readyRead, this, &SerialConnection::socket_Read_Data);
    QObject::connect(socket, &QTcpSocket::disconnected, this, &SerialConnection::socket_Disconnected);

    qDebug() << "A Client connect!";
}

void SerialConnection::socket_Read_Data()
{
    RTLSDisplayApplication::client()->SocketnewData();
}

void SerialConnection::socket_Disconnected()
{
    qDebug() << "Socket Disconnected!";
}
void SerialConnection::showLineEditText(QLineEdit *lineedit,QString text)
{

     lineedit->setText(text);
}

QString SerialConnection:: dataRevfuc(QString data,QString keyword,int location,int length)
{
    QString  ReturnData;
    if(data.contains(keyword))
    {
        ReturnData  =data.mid(data.indexOf(keyword)+location,length);
        QStringList myList = ReturnData.split(',');

        // for(int i=0; i<myList.size(); i++)
        // {
        //     qDebug()<< "toint="<< myList[i].toInt();
        // }
        if(ReturnData.contains("\r"))
       {
           ReturnData =ReturnData.remove(ReturnData.indexOf("\r"),3) ;
       }

    }

    return ReturnData;
}
QString SerialConnection:: dataRevfuc1(QString data,QString keyword,int location,int length,int i)
{
    QString  ReturnData;
    if(data.contains(keyword))
    {
        ReturnData  =data.mid(data.indexOf(keyword)+location,length);
        ReturnData= ReturnData.left(ReturnData.indexOf(")"));
        QStringList myList = ReturnData.split(',',QString::SkipEmptyParts);
        ReturnData = myList[i].simplified();

//        if(ReturnData.contains("\r"))
//       {
//           ReturnData =ReturnData.remove(ReturnData.indexOf("\r"),3) ;
//       }

    }

    return ReturnData;
}
void SerialConnection::updatedata()
{
    emit dataupdate();
}