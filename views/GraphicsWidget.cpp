// -------------------------------------------------------------------------------------------------------------------
//
//  File: GraphicsWidget.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#include "GraphicsWidget.h"
#include "ui_graphicswidget.h"
#include "ui_ViewSettingsWidget.h"
//#include "ViewSettingsWidget.h"
#include "RTLSClient.h"
#include "RTLSDisplayApplication.h"
#include "ViewSettings.h"
#include <QDomDocument>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsRectItem>
#include <QDebug>
#include <QInputDialog>
#include <QFile>
#include <QPen>
#include <QDesktopWidget>
#include <QMovie>

#define PEN_WIDTH (0.04)
#define ANC_SIZE (1.1)  //基站显示圆圈大小
#define TAG_SIZE (0.3)  //标签显示圆圈大小
#define FONT_SIZE (20)  //显示基站标签数字大小
#define SINGAL_TIMEOUT (12000)  //10s
//float GraphicsWidget:: _tagSize;
GraphicsWidget *myGraphicsWidget;
GraphicsWidget::GraphicsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphicsWidget)
{
    QDesktopWidget desktop;
    int desktopHeight=desktop.geometry().height();
    int desktopWidth=desktop.geometry().width();

    ui->setupUi(this);
    myGraphicsWidget=this;
    this->_scene = new QGraphicsScene(this);

    ui->graphicsView->setScene(this->_scene);
    ui->graphicsView->scale(1, -1);
    //ui->graphicsView->setBaseSize(this->height()*0.75, this->width());
    ui->graphicsView->setBaseSize(desktopHeight*0.75, desktopWidth*0.75);

    //tagTable
    //Tag ID, x, y, z, r95, vbat? ...
   // QStringList tableHeader;
    tableHeader << "标签 ID" << "X\n(m)" << "Y\n(m)" << "Z\n(m)" <<"定位圆" <<"电量(%)\n预留" <<"静态R95\n(cm)"
                << "Anc 0\nrange(m)" << "Anc 1\nrange(m)" << "Anc 2\nrange(m)" << "Anc 3\nrange(m)"
                << "Anc 4\nrange(m)" << "Anc 5\nrange(m)" << "Anc 6\nrange(m)" << "Anc 7\nrange(m)"
                << "Rx Power\n(dBm)"
                   ;

   // QStringList tableHeader_en;
    tableHeader_en << "Tag" << "X\n(m)" << "Y\n(m)" << "Z\n(m)" <<"Ranging" <<"power(%)" <<"R95\n(cm)"
                << "Anc 0\nrange(m)" << "Anc 1\nrange(m)" << "Anc 2\nrange(m)" << "Anc 3\nrange(m)"
                << "Anc 4\nrange(m)" << "Anc 5\nrange(m)" << "Anc 6\nrange(m)" << "Anc 7\nrange(m)"
                << "Rx Power\n(dBm)"
                   ;
    if(language)
   {
      ui->tagTable->setHorizontalHeaderLabels(tableHeader);
   }
    else{
      ui->tagTable->setHorizontalHeaderLabels(tableHeader_en);
    }
    ui->tagTable->setColumnWidth(ColumnID,65);    //ID
    ui->tagTable->setColumnWidth(ColumnX,50); //x
    ui->tagTable->setColumnWidth(ColumnY,50); //y
    ui->tagTable->setColumnWidth(ColumnZ,50); //z
    //ui->tagTable->setColumnWidth(ColumnBlinkRx,70); //% Blinks RX
    //ui->tagTable->setColumnWidth(ColumnLocations,70); //% Multilaterations Success
    ui->tagTable->setColumnWidth(ColumnLocatingcircle,60); //
    ui->tagTable->setColumnWidth(ColumnPower,60); //power
    ui->tagTable->setColumnWidth(ColumnR95,60); //R95
    ui->tagTable->setColumnHidden(ColumnR95,true); //R95 hide

    ui->tagTable->setColumnWidth(ColumnRA0,60);
    ui->tagTable->setColumnWidth(ColumnRA1,60);
    ui->tagTable->setColumnWidth(ColumnRA2,60);
    ui->tagTable->setColumnWidth(ColumnRA3,60);

    ui->tagTable->setColumnWidth(ColumnRA4,60);
    ui->tagTable->setColumnWidth(ColumnRA5,60);
    ui->tagTable->setColumnWidth(ColumnRA6,60);
    ui->tagTable->setColumnWidth(ColumnRA7,60);
    
    ui->tagTable->setColumnWidth(ColumnRXpwr,60);

    ui->tagTable->setColumnHidden(ColumnIDr, true); //ID raw hex
    //ui->tagTable->setColumnWidth(ColumnIDr,70); //ID raw hex

    if(desktopWidth <= 800)
    {
        ui->tagTable->setMaximumWidth(400);
    }

    //anchorTable
    //Anchor ID, x, y, z,
   // QStringList anchorHeader;
    anchorHeader << "基站 ID" << "X (m)" << "Y (m)" << "Z (m)"
                    << "T0(cm)" << "T1(cm)" << "T2(cm)" << "T3(cm)"
                    << "T4(cm)" << "T5(cm)" << "T6(cm)" << "T7(cm)" ;

  //  QStringList anchorHeader_en;
    anchorHeader_en << "Anchor" << "X (m)" << "Y (m)" << "Z (m)"
                    << "T0(cm)" << "T1(cm)" << "T2(cm)" << "T3(cm)"
                    << "T4(cm)" << "T5(cm)" << "T6(cm)" << "T7(cm)" ;
    if(language)
    {
       ui->anchorTable->setHorizontalHeaderLabels(anchorHeader);
    }
    else{
       ui->anchorTable->setHorizontalHeaderLabels(anchorHeader_en);
    }
    ui->anchorTable->setColumnWidth(ColumnID,55);    //ID
    ui->anchorTable->setColumnWidth(ColumnX,55); //x
    ui->anchorTable->setColumnWidth(ColumnY,55); //y
    ui->anchorTable->setColumnWidth(ColumnZ,55); //z
    ui->anchorTable->setColumnWidth(4,55); //t0
    ui->anchorTable->setColumnWidth(5,55); //t1
    ui->anchorTable->setColumnWidth(6,55); //t2
    ui->anchorTable->setColumnWidth(7,55); //t3
    ui->anchorTable->setColumnWidth(8,55); //t4
    ui->anchorTable->setColumnWidth(9,55); //t5
    ui->anchorTable->setColumnWidth(10,55); //t6
    ui->anchorTable->setColumnWidth(11,55); //t7

//    QGridLayout *pLayout = new QGridLayout();
//    // 基站列表 第0行，第0列开始，占3行1列
//    pLayout->addWidget(ui->anchorTable, 0, 0, 3, 1);
//    // 标签列表 第0行，第1列开始，占1行2列
//    pLayout->addWidget(ui->tagTable, 0, 1, 4, 1);
//    // 自标定按钮 第4行，第0列开始，占1行2列
//    pLayout->addWidget(ui->pushButton, 4, 0, 1, 1);
//    // 基站画面 第5行，第1列开始，占4行4列
//    pLayout->addWidget(ui->graphicsView, 5, 1, 4, 4);
//    // 设置水平间距
//    pLayout->setHorizontalSpacing(10);
//    // 设置垂直间距
//    pLayout->setVerticalSpacing(10);
//    // 设置外间距
//    pLayout->setContentsMargins(10, 10, 10, 10);
//    setLayout(pLayout);

    //hide Anchor/Tag range corection table
    hideTACorrectionTable(true);

    ui->anchorTable->setMaximumHeight(120);
    ui->tagTable->setMaximumHeight(200);



    //set defaults
    _tagSize = TAG_SIZE;  //标签圈大小
    _historyLength = 20;
    _showHistoryP = _showHistory = true;

    _busy = true ;
    _ignore = true;

    _selectedTagIdx = -1;

    zone1 = NULL;
    zone2 = NULL;

    _maxRad = 1000;

    _minRad = 0;

    _zone1Rad = 0;
    _zone2Rad = 0;
    _zone1Red = false;
    _zone2Red = false;

    _geoFencingMode = false;
    _alarmOut = false;
     status[100] = false;
    _line01 = NULL;
    _line02 = NULL;
    _line12 = NULL;
    RTLSDisplayApplication::connectReady(this, "onReady()");



}

void GraphicsWidget::onReady()
{
	QObject::connect(ui->tagTable, SIGNAL(cellChanged(int, int)), this, SLOT(tagTableChanged(int, int)));
    QObject::connect(ui->anchorTable, SIGNAL(cellChanged(int, int)), this, SLOT(anchorTableChanged(int, int)));
    QObject::connect(ui->tagTable, SIGNAL(cellClicked(int, int)), this, SLOT(tagTableClicked(int, int)));
    QObject::connect(ui->anchorTable, SIGNAL(cellClicked(int, int)), this, SLOT(anchorTableClicked(int, int)));
    QObject::connect(ui->tagTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    QObject::connect(ui->anchorTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChangedAnc()));

    QObject::connect(this, SIGNAL(centerAt(double,double)), graphicsView(), SLOT(centerAt(double, double)));
    QObject::connect(this, SIGNAL(centerRect(QRectF)), graphicsView(), SLOT(centerRect(QRectF)));
    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(calibrationButtonClicked()));

    QObject::connect(RTLSDisplayApplication::graphicsView(), SIGNAL(sizeChanged(QSize)), this, SLOT(sizeChanged(QSize)));
    QObject::connect(RTLSDisplayApplication::graphicsView(), SIGNAL(rotateChanged(qreal)), this, SLOT(rotateChanged(qreal)));
    QObject::connect(RTLSDisplayApplication::graphicsView(), SIGNAL(visibleRectChanged(QRectF)), this, SLOT(visibleRectChanged()));
    QObject::connect(RTLSDisplayApplication::graphicsView(), SIGNAL(scaleChanged(qreal)), this, SLOT(scaleChanged(qreal)));

    QObject::connect(RTLSDisplayApplication::viewSettings(), SIGNAL(fontSizeChangedeSignal(int)), this, SLOT(setCanvasFontSize(int)));

    QPushButton *helpBtn = new QPushButton(ui->pushButton);
    helpBtn->resize(15,15);
    helpBtn->setStyleSheet("border-image:url(:/icons/help.jpg);");
    QPoint helpBtnPos = this->pos();
    helpBtnPos = QPoint(this->x()+200,this->y()+3);
    helpBtn->move(helpBtnPos);
    QObject::connect(helpBtn, SIGNAL(released()), this, SLOT(helpBtnReleased()));

    m_QQuickWidget = new QQuickWidget(ui->graphicsView);
    m_QQuickWidget->rootContext()->setContextProperty("canvas_posX", canvas_posX);
    m_QQuickWidget->rootContext()->setContextProperty("canvas_posY", canvas_posY);
    m_QQuickWidget->rootContext()->setContextProperty("canvas_rotate", canvas_rotate);
    m_QQuickWidget->rootContext()->setContextProperty("canvas_scale", canvas_scale);
    m_QQuickWidget->rootContext()->setContextProperty("language", language);
    m_QQuickWidget->rootContext()->setContextProperty("canvasInfomationVisible", canvasInfomationVisible);
    m_QQuickWidget->setSource(QUrl("qrc:/icons/CanvasInformation.qml"));
    m_QQuickWidget->move(10,900);
    m_QQuickWidget->setAttribute(Qt::WA_AlwaysStackOnTop);
    m_QQuickWidget->setClearColor(QColor(Qt::transparent));
    QQuickItem *m_item = m_QQuickWidget->rootObject();
    QObject::connect(m_item, SIGNAL(resetSignal()), this, SLOT(resetButtonClicked()));


    m_calibrationTimer_100ms = new QTimer();
    m_calibrationTimer_100ms->setInterval(SINGAL_TIMEOUT); //发送时判断是否返回$setok，如100ms不返回怎弹出提示
    connect(m_calibrationTimer_100ms, &QTimer::timeout, this, [=](){
        m_calibrationTimer_100ms->stop();
        if(!reseveSetok)
        {
            calibrationFailed();
            if(language)
            {
                QMessageBox::warning(NULL,"自标定启动失败","自标定启动失败，设备不支持该功能或固件版本过低","我知道了");
            }
            else{
                QMessageBox::warning(NULL,"Warning","The device does not support the function or the firmware version is too low.","OK");
            }
        }
    });

    m_calibrationTimer = new QTimer();
    m_calibrationTimer->setInterval(SINGAL_TIMEOUT); //如果10s未收到基站标定成功消息，认为超时。检查基站信号
    connect(m_calibrationTimer, &QTimer::timeout, this, [=](){
        signalTimeOut = true;
        RTLSDisplayApplication::client()->timeOutWarning();
    });

    for (int i = 0; i < 3; i++)
    {
        setStationList(i,true);//默认A0-A2三基站显示
    }

    ui->stackedWidget->hide();


//    for(int i=0;i<8;i++)
//   {
//     ui->anchorTable->setItem(i,2,new QTableWidgetItem(QString::number(i+18)));
//   }
    _busy = false ;
}

GraphicsWidget::~GraphicsWidget()
{
    // _scene has parent (this), Qt will auto-delete it
    delete ui;
}

GraphicsView *GraphicsWidget::graphicsView()
{
    return ui->graphicsView;
}


void GraphicsWidget::loadConfigFile(QString filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(file.errorString())));
        return;
    }

    QDomDocument doc;
    QString error;
    int errorLine;
    int errorColumn;

    if(!doc.setContent(&file, false, &error, &errorLine, &errorColumn))
    {
        //qDebug() << "2file error !!!" << error << errorLine << errorColumn;

        emit setTagHistory(_historyLength);
    }

    QDomElement config = doc.documentElement();

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

                    _tagSize = (e.attribute( "size", "" )).toDouble();
                    _historyLength = (e.attribute( "history", "" )).toInt();
                    //qDebug("checkbox2d3d = %d", checkbox2d3d);
                    
                    //cout<<_tagSize<<endl;
                }
                else
                if( e.tagName() == "tag" )
                {
                    bool ok;
                    quint64 id = (e.attribute( "ID", "" )).toULongLong(&ok, 16);
                    QString label = (e.attribute( "label", "" ));

                    _tagLabels.insert(id, label);
                }
            }

            n = n.nextSibling();
        }

    }
    ViewSettingsWidget::viewsettingswidget->setLinetext(QString("%1").arg(_tagSize));
//    ViewSettingsWidget *viewsettingwidget =new  ViewSettingsWidget(this);
//    viewsettingwidget->ui->tag_size->setText(QString("%1").arg(_tagSize));
    file.close();
    
    //cout<<"8888"<<_tagSize<<endl;

    emit setTagHistory(_historyLength);
}


void GraphicsWidget::hideTACorrectionTable(bool hidden)
{
    ui->anchorTable->setColumnHidden(4,hidden); //t0
    ui->anchorTable->setColumnHidden(5,hidden); //t1
    ui->anchorTable->setColumnHidden(6,hidden); //t2
    ui->anchorTable->setColumnHidden(7,hidden); //t3
    ui->anchorTable->setColumnHidden(8,hidden); //t4
    ui->anchorTable->setColumnHidden(9,hidden); //t5
    ui->anchorTable->setColumnHidden(10,hidden); //t6
    ui->anchorTable->setColumnHidden(11,hidden); //t7
}


QDomElement TagToNode( QDomDocument &d, quint64 id, QString label )
{
    QDomElement cn = d.createElement( "tag" );
    cn.setAttribute("ID", QString::number(id, 16));
    cn.setAttribute("label", label);
    return cn;
}

void GraphicsWidget::saveConfigFile(QString filename)
{
    QFile file( filename );
    //cout<<"123456"<<_tagSize<<endl;
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg("./TREKtag_config.xml").arg(file.errorString())));
        return;
    }

    QDomDocument doc;

    // Adding tag config root
    QDomElement config = doc.createElement("config");
    doc.appendChild(config);

    QDomElement cn = doc.createElement( "tag_cfg" );
    cn.setAttribute("size", _tagSize);
    //cout<<_tagSize<<endl;
    cn.setAttribute("history", QString::number(_historyLength));
    config.appendChild(cn);

    //update the map
    QMap<quint64, QString>::iterator i = _tagLabels.begin();

    while (i != _tagLabels.end())
    {
        config.appendChild( TagToNode(doc, i.key(), i.value()) );

        i++;
    }

    QTextStream ts( &file );
    ts << doc.toString();

    file.close();

  //  qDebug() << doc.toString();
}

void GraphicsWidget::clearTags(void)
{
   // qDebug() << "table rows " << ui->tagTable->rowCount() << " list " << this->_tags.size();

    while (ui->tagTable->rowCount())
    {
        QTableWidgetItem* item = ui->tagTable->item(0, ColumnIDr);

        if(item)
        {
           // qDebug() << "Item text: " << item->text();

            bool ok;
            quint64 tagID = item->text().toULongLong(&ok, 16);
            //clear scene from any tags
            Tag *tag = this->_tags.value(tagID, NULL);
            if(tag->r95p) //remove R95
            {
                //re-size the elipse... with a new rad value...
                tag->r95p->setOpacity(0); //hide it

                this->_scene->removeItem(tag->r95p);
                delete(tag->r95p);
                tag->r95p = NULL;
            }
            if(tag->avgp) //remove average
            {
                //re-size the elipse... with a new rad value...
                tag->avgp->setOpacity(0); //hide it

                this->_scene->removeItem(tag->avgp);
                delete(tag->avgp);
                tag->avgp = NULL;
            }
            if(tag->geop) //remove average
            {
                //re-size the elipse... with a new rad value...
                tag->geop->setOpacity(0); //hide it

                this->_scene->removeItem(tag->geop);
                delete(tag->geop);
                tag->geop = NULL;
            }
            for(int i=0;i<8;i++)
           {
            if(tag->circle[i]) //remove average
            {
                //re-size the elipse... with a new rad value...
                tag->circle[i]->setOpacity(0); //hide it

                this->_scene->removeItem(tag->circle[i]);
                delete(tag->circle[i]);
                tag->circle[i] = NULL;
            }
           }
            if(tag->tagLabel) //remove label
            {
                //re-size the elipse... with a new rad value...
                tag->tagLabel->setOpacity(0); //hide it

                this->_scene->removeItem(tag->tagLabel);
                delete(tag->tagLabel);
                tag->tagLabel = NULL;
            }

            //remove history...
            for(int idx=0; idx<_historyLength; idx++ )
            {
                QAbstractGraphicsShapeItem *tag_p = tag->p[idx];
                if(tag_p)
                {
                    tag_p->setOpacity(0); //hide it

                    this->_scene->removeItem(tag_p);
                    delete(tag_p);
                    tag_p = NULL;
                    tag->p[idx] = 0;

                   // qDebug() << "hist remove tag " << idx;
                }
            }
            {
                QMap<quint64, Tag*>::iterator i = _tags.find(tagID);

                if(i != _tags.end()) _tags.erase(i);
            }
        }
        ui->tagTable->removeRow(0);
    }

    //clear tag table
    ui->tagTable->clearContents();

   // qDebug() << "clear tags/tag table";

}

void GraphicsWidget::itemSelectionChanged(void)
{
    QList <QTableWidgetItem *>  l = ui->tagTable->selectedItems();
}

void GraphicsWidget::itemSelectionChangedAnc(void)
{
    QList <QTableWidgetItem *>  l = ui->anchorTable->selectedItems();
}

void GraphicsWidget::tagTableChanged(int r, int c)
{

    if(!_ignore)
    {
        Tag *tag = NULL;
        bool ok;
        quint64 tagId = (ui->tagTable->item(r,ColumnIDr)->text()).toULongLong(&ok, 16);
        tag = _tags.value(tagId, NULL);

        if(!tag) return;

        if(c == ColumnID) //label has changed
        {
            QString newLabel = ui->tagTable->item(r,ColumnID)->text();

            tag->tagLabelStr = newLabel;
            tag->tagLabel->setText(newLabel);

            //update the map
            QMap<quint64, QString>::iterator i = _tagLabels.find(tagId);

            if(i == _tagLabels.end()) //did not find the label
            {
                //insert the new value
                _tagLabels.insert(tagId, newLabel);
            }
            else //if (i != taglabels.end()) // && i.key() == tagId)
            {
                _tagLabels.erase(i); //erase the key
                _tagLabels.insert(tagId, newLabel);
            }
        }
        if(c == ColumnLocatingcircle) //
        {
            QTableWidgetItem *pItem = ui->tagTable->item(r, c);
            tag->LocatingcircleShow[r] = (pItem->checkState() == Qt::Checked) ? true : false;
            if(tag->LocatingcircleShow[r])
            {
                status[r] = true;
            }
            else
            {
                status[r] = false;
            }
        }
    }
}

void GraphicsWidget::anchorTableChanged(int r, int c)
{
    if(!_ignore)
    {
        _ignore = true;

        Anchor *anc = NULL;
        bool ok;

        anc = _anchors.value(r, NULL);

        if(!anc) return;

        switch(c)
        {
            case ColumnX:
            case ColumnY:
            case ColumnZ:
            {
                double xyz = (ui->anchorTable->item(r,c)->text()).toDouble(&ok);
                if(ok)
                {
                    // if((ColumnZ == c) && (r > 2))
                    // {
                    //     emit updateAnchorXYZ(r, c, xyz);
                    // }
                    // else
                    {
                        emit updateAnchorXYZ(r, c, xyz);
                    }

                    double xn = (ui->anchorTable->item(r,ColumnX)->text()).toDouble(&ok);
                    double yn = (ui->anchorTable->item(r,ColumnY)->text()).toDouble(&ok);
                    double zn = (ui->anchorTable->item(r,ColumnZ)->text()).toDouble(&ok);

                    ui->anchorTable->item(r,ColumnX)->setText(QString::number(xn, 'f', 2));
                    ui->anchorTable->item(r,ColumnY)->setText(QString::number(yn, 'f', 2));
                    ui->anchorTable->item(r,ColumnZ)->setText(QString::number(zn, 'f', 2));


                    anchPos(r, xn, yn, zn, true, false);

                    // if((ColumnZ == c) && (r <= 2))
                    // {
                    //     //update z of 1, 2, and 3 to be the same
                    //     emit updateAnchorXYZ(0, c, xyz);
                    //     emit updateAnchorXYZ(1, c, xyz);
                    //     emit updateAnchorXYZ(2, c, xyz);

                    //     //make all 3 anchors have the same z coordinate
                    //     ui->anchorTable->item(0,ColumnZ)->setText(QString::number(zn, 'f', 2));
                    //     ui->anchorTable->item(1,ColumnZ)->setText(QString::number(zn, 'f', 2));
                    //     ui->anchorTable->item(2,ColumnZ)->setText(QString::number(zn, 'f', 2));
                    // }
                }
            }
            break;
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            {
                int value = (ui->anchorTable->item(r,c)->text()).toInt(&ok);
                if(ok)
                    emit updateTagCorrection(r, c-4, value);
            }
            break;
        }

        _ignore = false;
        if(canvasVisible)
        {
            canvasShow(true);
        }
    }

}

void GraphicsWidget::anchorTableClicked(int r, int c)
{
    Anchor *anc = NULL;

    anc = _anchors.value(r, NULL);

    if(!anc) return;    

    if(c == ColumnID) //toggle label
    {
        QTableWidgetItem *pItem = ui->anchorTable->item(r, c);
        anc->show = (pItem->checkState() == Qt::Checked) ? true : false;
        anc->a->setOpacity(anc->show ? 1.0 : 0.0);
        anc->ancLabel->setOpacity(anc->show ? 1.0 : 0.0);
        setStationList(r,anc->show);
    }

    if(canvasVisible)
    {
        canvasShow(true);
    }

}

void GraphicsWidget::tagTableClicked(int r, int c)
{
    Tag *tag = NULL;
    bool ok;
    quint64 tagId = (ui->tagTable->item(r,ColumnIDr)->text()).toULongLong(&ok, 16);
    tag = _tags.value(tagId, NULL);

    _selectedTagIdx = r;

    if(!tag) return;

    if(c == ColumnR95) //toggle R95 display
    {
        QTableWidgetItem *pItem = ui->tagTable->item(r, c);
        tag->r95Show = (pItem->checkState() == Qt::Checked) ? true : false;
    }

    if(c == ColumnID) //toggle label
    {
        QTableWidgetItem *pItem = ui->tagTable->item(r, c);
        tag->showLabel = (pItem->checkState() == Qt::Checked) ? true : false;

        tag->tagLabel->setOpacity(tag->showLabel ? 1.0 : 0.0);
    }


}

/**
 * @fn    tagIDtoString
 * @brief  convert hex Tag ID to string (preappend 0x)
 *
 * */
void GraphicsWidget::tagIDToString(quint64 tagId, QString *t)
{
    //NOTE: this needs to be hex string as it is written into ColumnIDr
    //and is used later to remove the correct Tag
    *t = "0x"+QString::number(tagId, 16);
}

int GraphicsWidget::findTagRowIndex(QString &t)
{
    for (int ridx = 0 ; ridx < ui->tagTable->rowCount() ; ridx++ )
    {
//        qDebug() << "ui->tagTable->rowCount() =" << ui->tagTable->rowCount()<<", ridx: "<<ridx/*<<", name: "<<ui->tagTable->item(ridx, ColumnIDr)->text()*/;

        QTableWidgetItem* item = ui->tagTable->item(ridx, ColumnIDr);

        if(item)
        {
            //qDebug() << "asdfasdf";
            if(item->text() == t)
            {
                return ridx;
            }
        }
        else
        {
            qDebug() << "no item";
        }
    }

    return -1;
}

/**
 * @fn    insertTag
 * @brief  insert Tag into the tagTable at row ridx
 *
 * */
void GraphicsWidget::insertTag(int ridx, QString &t, bool showR95, bool showLabel, QString &l)
{
    _ignore = true;

    ui->tagTable->insertRow(ridx);

    qDebug() << "Insert Tag" << ridx << t << l;
    //qDebug() << "ui->tagTable->columnCount()=" <<ui->tagTable->columnCount();

    for( int col = ColumnID ; col < ui->tagTable->columnCount(); col++)
    {
        QTableWidgetItem* item = new QTableWidgetItem();
        QTableWidgetItem *pItem = new QTableWidgetItem();
        if(col == ColumnID )
        {
            if(showLabel)
            {
                pItem->setCheckState(Qt::Checked);
                pItem->setText(l);
            }
            else
            {
                pItem->setCheckState(Qt::Unchecked);
                pItem->setText(l);
            }
            item->setFlags((item->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);
            //ui->tagTable->setItem(ridx, col, item);
            ui->tagTable->setItem(ridx, col, pItem);
        }
        else
        {
            if(col == ColumnIDr)
            {
                item->setText(t);
            }

            item->setFlags((item->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);
            ui->tagTable->setItem(ridx, col, item);
        }

        if(col == ColumnR95)
        {
            if(showR95)
            {
                pItem->setCheckState(Qt::Checked);
            }
            else
            {
                pItem->setCheckState(Qt::Unchecked);
            }
            pItem->setText(" ");
            ui->tagTable->setItem(ridx,col,pItem);
        }
        if(col == ColumnLocatingcircle )
        {
           pItem->setCheckState(Qt::Unchecked);

           ui->tagTable->setItem(ridx,col,pItem);

        }



   }

   _ignore = false; //we've added a row
}

/**
 * @fn    insertAnchor
 * @brief  insert Anchor/Tag correction values into the anchorTable at row ridx
 *
 * */
void GraphicsWidget::insertAnchor(int ridx, double x, double y, double z,int *array, bool show)
{
    int i = 0;
    _ignore = true;

    //add tag offsets
    for( int col = 4 ; col < ui->anchorTable->columnCount(); col++)
    {
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setText(QString::number(array[i]));
        item->setTextAlignment(Qt::AlignHCenter);
        //item->setFlags((item->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);
        ui->anchorTable->setItem(ridx, col, item);
    }

    //add x, y, z
    QTableWidgetItem* itemx = new QTableWidgetItem();
    QTableWidgetItem* itemy = new QTableWidgetItem();
    QTableWidgetItem* itemz = new QTableWidgetItem();

    itemx->setText(QString::number(x, 'f', 2));
    itemy->setText(QString::number(y, 'f', 2));
    itemz->setText(QString::number(z, 'f', 2));

    ui->anchorTable->setItem(ridx, ColumnX, itemx);
    ui->anchorTable->setItem(ridx, ColumnY, itemy);
    ui->anchorTable->setItem(ridx, ColumnZ, itemz);

    {
        QTableWidgetItem *pItem = new QTableWidgetItem();

        if(show)
        {
            pItem->setCheckState(Qt::Checked);
        }
        else
        {
            pItem->setCheckState(Qt::Unchecked);
        }

        pItem->setText(QString::number(ridx));
        pItem->setTextAlignment(Qt::AlignHCenter);

        pItem->setFlags((pItem->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);

        ui->anchorTable->setItem(ridx, ColumnID, pItem);

    }

    ui->anchorTable->showRow(ridx);

    QTableWidgetItem * item = ui->anchorTable->item(0, 0);
    item->setFlags(Qt::NoItemFlags);

//    for(int i=0;i<8;i++)
//   {
//     ui->anchorTable->setItem(i,2,new QTableWidgetItem("0.00"));
//     ui->anchorTable->item(i,2)->setFlags(ui->anchorTable->item(i,2)->flags() & (~Qt::ItemIsEditable));
//    }

    _ignore = false;
}

/**
 * @fn    addNewTag
 * @brief  add new Tag with tagId into the tags QMap
 *
 * */
void GraphicsWidget::addNewTag(quint64 tagId)
{
    static double c_h = 0.1;
    Tag *tag;
    int tid = tagId ;
    QString taglabel = QString("Tag %1").arg(tid); //taglabels.value(tagId, NULL);

    qDebug() << "Add new Tag: 0x" + QString::number(tagId, 16) << tagId;

    //insert into QMap list, and create an array to hold history of its positions
    _tags.insert(tagId,new(Tag));

    tag = this->_tags.value(tagId, NULL);
    tag->p.resize(_historyLength);

    c_h += 0.568034;
    if (c_h >= 1)
        c_h -= 1;
    tag->colourH = c_h;
    tag->colourS = 0.55;
    tag->colourV = 0.98;
    qDebug()<<"colourH::"<<tag->colourH;

    tag->tsPrev = 0;
    tag->r95Show = false;
    tag->showLabel = (taglabel != NULL) ? true : false;
    tag->tagLabelStr = taglabel;
    //add ellipse for the R95 - set to transparent until we get proper r95 data
    tag->r95p = this->_scene->addEllipse(-0.1, -0.1, 0, 0);
    tag->r95p->setOpacity(0);
    tag->r95p->setPen(Qt::NoPen);
    tag->r95p->setBrush(Qt::NoBrush);
    //add ellipse for average point - set to transparent until we get proper average data
    tag->avgp = this->_scene->addEllipse(0, 0, 0, 0);
    tag->avgp->setBrush(Qt::NoBrush);
    tag->avgp->setPen(Qt::NoPen);
    tag->avgp->setOpacity(0);
    //add ellipse for geo fencing point - set to transparent until we get proper average data
    tag->geop = this->_scene->addEllipse(0, 0, 0, 0);
    tag->geop->setBrush(Qt::NoBrush);
    tag->geop->setPen(Qt::NoPen);
    tag->geop->setOpacity(0);
   for(int i =0; i<8;i++)
   {
     tag->circle[i] = this->_scene->addEllipse(0, 0, 0, 0);
     tag->circle[i]->setBrush(Qt::NoBrush);
     tag->circle[i]->setPen(Qt::NoPen);
     tag->circle[i]->setOpacity(0);
   }
    //add text label and hide until we see if user has enabled showLabel
    {
        QPen pen = QPen(Qt::blue);
        tag->tagLabel = new QGraphicsSimpleTextItem(NULL);
        tag->tagLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        tag->tagLabel->setZValue(1);
        tag->tagLabel->setText(taglabel);
        tag->tagLabel->setOpacity(0);
        QFont font = tag->tagLabel->font();
        font.setPointSize(FONT_SIZE);
        font.setWeight(QFont::Normal);
        tag->tagLabel->setFont(font);
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(PEN_WIDTH);
        tag->tagLabel->setPen(pen);
        this->_scene->addItem(tag->tagLabel);
    }
}

/**
 * @fn    tagPos
 * @brief  update tag position on the screen (add to scene if it does not exist)
 *
 * */
void GraphicsWidget::tagPos(quint64 tagId, double x, double y, double z)
{
    //qDebug() << "tagPos Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;

    if(_busy || _geoFencingMode) //don't display position if geofencing is on
    {
       // qDebug() << "(Widget - busy IGNORE) Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;

    }
    else
    {
        Tag *tag = NULL;
        bool newTag = false;
        QString t ;

      //  _busy = true ;
        tagIDToString(tagId, &t); //convert uint64 to string

        tag = _tags.value(tagId, NULL);

        if(!tag) //add new tag to the tags array
        {
            //tag does not exist, so create a new one
            newTag = true;
            addNewTag(tagId);

            tag = this->_tags.value(tagId, NULL);
        }

        if(!tag->p[tag->idx]) //we have not added this object yet to the history array
        {
            QAbstractGraphicsShapeItem *tag_pt = this->_scene->addEllipse(-1*_tagSize/2, -1*_tagSize/2, _tagSize, _tagSize);

            tag->p[tag->idx] = tag_pt;
            tag_pt->setPen(Qt::NoPen);

            if(tag->idx > 0) //use same brush settings for existing tag ID
            {
                tag_pt->setBrush(tag->p[0]->brush());
            }
            else //new brush... new tag ID as idx = 0
            {
                QBrush b = QBrush(QColor::fromHsvF(tag->colourH, tag->colourS, tag->colourV));
                QPen pen = QPen(b.color().darker());
               // qDebug()<<"12345:::"<<tagId<<"::"<<tag->colourH<<tag->colourS<<tag->colourV<<b.color().darker();
                pen.setStyle(Qt::SolidLine);
                pen.setWidthF(PEN_WIDTH);
                //set the brush colour (average point is darker of the same colour
                tag_pt->setBrush(b.color().dark());
                tag->avgp->setBrush(b.color().darker());
                tag->tagLabel->setBrush(b.color().dark());

                tag->tagLabel->setPen(pen);
            }

            tag_pt->setToolTip(t);

            //update colour for next tag
            if(newTag) //keep same colour for same tag ID
            {
                tag->ridx = findTagRowIndex(t);

                if(tag->ridx == -1)
                {
                    tag->ridx = ui->tagTable->rowCount();
//                    qDebug() << "111111111111";
                    insertTag(tag->ridx, t, tag->r95Show, tag->showLabel, tag->tagLabelStr);
                }
            }
        }

        _ignore = true;
        ui->tagTable->item(tag->ridx,ColumnX)->setText(QString::number(x, 'f', 3));
        ui->tagTable->item(tag->ridx,ColumnY)->setText(QString::number(y, 'f', 3));
        ui->tagTable->item(tag->ridx,ColumnZ)->setText(QString::number(z, 'f', 3));

        tag->p[tag->idx]->setPos(x, y);

        if(_showHistory)
        {
            tagHistory(tagId);
            tag->idx = (tag->idx+1)%_historyLength;
        }
        else
        {
            //index will stay at 0
            tag->p[tag->idx]->setOpacity(1);
        }

        tag->tagLabel->setPos(x + 0.15, y + 0.15);
        _ignore = false;
        _busy = false;

        //qDebug() << "Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;
	}

}


void GraphicsWidget::tagStats(quint64 tagId, double x, double y, double z, double r95)
{
    if(_busy)
    {
       // qDebug() << "(busy IGNORE) R95: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z << " " << r95;
    }
    else
    {
        Tag *tag = NULL;

        _busy = true ;

        tag = _tags.value(tagId, NULL);

        if(!tag) //add new tag to the tags array
        {
            addNewTag(tagId);

            tag = this->_tags.value(tagId, NULL);
        }

        if (tag)
        {
            //static float h = 0.1;
            //float s = 0.5, v = 0.98;
            double rad = r95*2;

            if(tag->r95p)
            {
                //re-size the elipse... with a new rad value...
                tag->r95p->setOpacity(0); //hide it

                this->_scene->removeItem(tag->r95p);
                delete(tag->r95p);
                tag->r95p = NULL;
            }

            //else //add r95 circle

            {
                //add R95 circle
                tag->r95p = this->_scene->addEllipse(-1*r95, -1*r95, rad, rad);
                tag->r95p->setPen(Qt::NoPen);
                tag->r95p->setBrush(Qt::NoBrush);

                if( tag->r95Show && (rad <= 1))
                {
                    QPen pen = QPen(Qt::darkGreen);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    tag->r95p->setOpacity(0.5);
                    tag->r95p->setPen(pen);
                    //tag->r95p->setBrush(QBrush(Qt::green, Qt::Dense7Pattern));
                    tag->r95p->setBrush(Qt::NoBrush);
                }
                else if (tag->r95Show && ((rad > 1)/*&&(rad <= 2)*/))
                {
                    QPen pen = QPen(Qt::darkRed);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    tag->r95p->setOpacity(0.5);
                    tag->r95p->setPen(pen);
                    //tag->r95p->setBrush(QBrush(Qt::darkRed, Qt::Dense7Pattern));
                    tag->r95p->setBrush(Qt::NoBrush);
                }

            }

            //update Tag R95 value in the table
            {
                QString t ;
                int ridx = 0;

                tagIDToString(tagId, &t);

                ridx = findTagRowIndex(t);

                if(ridx != -1)
                {
                    //ui->tagTable->setAlignment(Qt::AlignVCenter);
                    ui->tagTable->item(ridx,ColumnR95)->setText(QString::number(r95*100, 'f', 2));
                    //ui->tagTable->item(ridx,ColumnR95)->setText(QString::number(r95_int, 'f', 2));
                }
                else
                {

                }
            }


            if(!tag->avgp) //add the average point
            {
                QBrush b = tag->p[0]->brush().color().darker();

                tag->avgp = this->_scene->addEllipse(-0.025, -0.025, 0.05, 0.05);
                tag->avgp->setBrush(b);
                tag->avgp->setPen(Qt::NoPen);
            }

            //if  (rad > 2)
            if(!tag->r95Show)
            {
                 tag->avgp->setOpacity(0);
            }
            else
            {
                tag->avgp->setOpacity(1);
                tag->avgp->setPos(x, y); //move it to the avg x and y values
            }

            tag->r95p->setPos(x, y); //move r95 center to the avg x and y values
        }
        else
        {
            //ERROR - there has to be a tag already...
            //ignore this statistics report
        }

        _busy = false ;

       // qDebug() << "R95: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z << " " << r95;

    }
}


void GraphicsWidget::tagRange(quint64 tagId, quint64 aId, double range, double rx_power, int alarm)
{
    if(_busy)
    {
       // qDebug() << "(busy IGNORE) Range 0x" + QString::number(tagId, 16) << " " << range << " " << QString::number(aId, 16) ;
    }
    else
    {
        Tag *tag = NULL;
        _busy = true;
        tag = _tags.value(tagId, NULL);

        if(!tag) //add new tag to the tags array
        {
            addNewTag(tagId);
            tag = this->_tags.value(tagId, NULL);
        }

        QString t ;
        tagIDToString(tagId, &t);
        if(tag)
        {
            if((_geoFencingMode == 0) && status[tagId] && (aId == 0))
            {
                for (int i =0; i<8; i++) 
                {
                    if(tag->circle[i])
                    {
                        //re-size the elipse... with a new rad value...
                        tag->circle[i]->setOpacity(0); //hide it
                        this->_scene->removeItem(tag->circle[i]);
                        delete(tag->circle[i]);
                        tag->circle[i] = NULL;
                    }
//                    if(ui->anchorTable->item(i,0)->checkState())
//                        continue;
                    if(ui->anchorTable->item(i,0)->checkState() == Qt::Checked && (RTLSClient::RangeA0_A7[i] >= 0))
                    {
                        QPen pen = QPen();
                        pen.setColor(QColor::fromHsvF(tag->colourH, tag->colourS, tag->colourV));
                        pen.setWidthF(0.03);
                        pen.setStyle(Qt::DashDotDotLine);
                        //tag->circle[i] = this->_scene->addEllipse(ui->anchorTable->item(i,1)->text().toDouble()-sqrt(qPow(RTLSClient::RangeA0_A7[i],2)-qPow(ui->anchorTable->item(i,3)->text().toDouble(),2)), ui->anchorTable->item(i,2)->text().toDouble()-sqrt(qPow(RTLSClient::RangeA0_A7[i],2)-qPow(ui->anchorTable->item(i,3)->text().toDouble(),2)), 2*sqrt(qPow(RTLSClient::RangeA0_A7[i],2)-qPow(ui->anchorTable->item(i,3)->text().toDouble(),2)), 2*sqrt(qPow(RTLSClient::RangeA0_A7[i],2)-qPow(ui->anchorTable->item(i,3)->text().toDouble(),2)));
                        tag->circle[i] = this->_scene->addEllipse(ui->anchorTable->item(i,1)->text().toDouble()-RTLSClient::RangeA0_A7[i]*0.95, ui->anchorTable->item(i,2)->text().toDouble()-RTLSClient::RangeA0_A7[i]*0.95, 2*RTLSClient::RangeA0_A7[i]*0.95, 2*RTLSClient::RangeA0_A7[i]*0.95);
                        tag->circle[i]->setPen(pen);
                        tag->circle[i]->setBrush(Qt::NoBrush);
                        tag->circle[i]->setOpacity(0.7);
                        tag->circle[i]->setPos(0, 0);
                    }
                }
            }
            else if ((_geoFencingMode == 0) && (status[tagId] == 0))
            {
                for (int i =0;i<8;i++)
                {
                    if(tag->circle[i])
                    {
                        //re-size the elipse... with a new rad value...
                        tag->circle[i]->setOpacity(0); //hide it

                        this->_scene->removeItem(tag->circle[i]);
                        delete(tag->circle[i]);
                        tag->circle[i] = NULL;
                    }
                }
            }
            //Geo-fencing mode
            if(_geoFencingMode && (aId == 0) && (range >= 0))
            {
                QPen pen = QPen(Qt::darkGreen);

                if(tag->geop)
                {
                    //re-size the elipse... with a new rad value...
                    tag->geop->setOpacity(0); //hide it

                    this->_scene->removeItem(tag->geop);
                    delete(tag->geop);
                    tag->geop = NULL;
                }

                //if(!tag->geop) //add the circle around the anchor 0 which represents the tag
                {
                    pen.setColor(Qt::darkGreen);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    if(_alarmOut) //then if > than outside zone
                    {
                        if(range >= _maxRad) //if > zone out then colour of zone out
                        {
                            pen.setColor(Qt::red);
                        }
                        else
                        {
                            if(range > _minRad) //if < zone out and > zone in
                            {
                                //amber
                                pen.setColor(QColor::fromRgbF(5.0, 0.5, 255.0));
                                //blue
                                //pen.setColor(QColor::fromRgbF(0.0, 0.0, 1.0));
                            }
                        }
                    }
                    else //then if < inside zone
                    {
                        if(range <= _minRad)
                        {
                            pen.setColor(Qt::red);
                        }
                        else
                        {
                            if(range < _maxRad) //if < zone out and > zone in
                            {
                                //amber
                                pen.setColor(QColor::fromRgbF(5.0, 0.5, 255.0));
                               // pen.setColor(Qt::yellow);
                                //blue
                                //pen.setColor(QColor::fromRgbF(0.0, 0.0, 1.0));
                            }
                        }
                    }

                    tag->geop = this->_scene->addEllipse(-range, -range, range*2, range*2);
                    tag->geop->setPen(pen);
                    tag->geop->setBrush(Qt::NoBrush);
                }

                tag->geop->setOpacity(1);
                tag->geop->setPos(0, 0);

                tag->tagLabel->setPos(range + ((tagId & 0xF)*0.05 + 0.05), ((tagId & 0xF)*0.1 + 0.05));
            }

            //update Tag/Anchor range value in the table
            {
                QString t ;
                int ridx = 0;
                //int anc = aId&0x3;
                int anc = aId;

                tagIDToString(tagId, &t);
//              qDebug() << "t="<<t;
                ridx = findTagRowIndex(t);

                if(ridx != -1)
                {
                    //qDebug() << "mmmmmmm";
                    if(range >= 0)
                    {
                        ui->tagTable->item(ridx,ColumnRA0+anc)->setText(QString::number(range, 'f', 2));//数据显示
                        
                        //qDebug()<<"rx_power"<<rx_power;
                        if(rx_power >= 800.00)
                        {
                            ui->tagTable->item(ridx,ColumnRXpwr)->setText(" - ");
                        }
                        else
                        {
                            ui->tagTable->item(ridx,ColumnRXpwr)->setText(QString::number(rx_power, 'f', 2));
                        }
                        ui->tagTable->item(ridx,ColumnPower)->setText(QString::number(alarm));
                    
                    }
                    else
                    {
                        ui->tagTable->item(ridx,ColumnRA0+anc)->setText(" - ");
                    }
                }
                else //add tag into tag table
                {
                    tag->ridx = ui->tagTable->rowCount();
//                    qDebug() << "222222";
                    insertTag(tag->ridx, t, false, false, tag->tagLabelStr);
                }
            }
        }
        else
        {
            //if in tracking/navigation mode - ignore the range report
        }

    }
    _busy = false;
}


void GraphicsWidget::tagHistory(quint64 tagId)
{
    int i = 0;
    int j = 0;

    Tag *tag = this->_tags.value(tagId, NULL);
    for(i = 0; i < _historyLength; i++)
    {
        QAbstractGraphicsShapeItem *tag_p = tag->p[i];

        if(!tag_p)
        {
            break;
        }
        else
        {
            j = (tag->idx - i); //element at index is opaque
            if(j<0) j+= _historyLength;
            tag_p->setOpacity(1-(float)j/_historyLength);
        }
    }
}
/**
 * @fn    tagHistoryNumber
 * @brief  set tag history length
 *
 * */
void GraphicsWidget::tagHistoryNumber(int value)
{
    bool tag_showHistory = _showHistory;

    while(_busy);

    _busy = true;

    //remove old history
    setShowTagHistory(false);

    //need to resize all the tag point arrays
    for (int i = 0; i < ui->tagTable->rowCount(); i++)
    {
        QTableWidgetItem* item = ui->tagTable->item(i, ColumnIDr);

        if(item)
        {
            bool ok;
            quint64 tagID = item->text().toULongLong(&ok, 16);
            //clear scene from any tags
            Tag *tag = _tags.value(tagID, NULL);
            tag->p.resize(value);
        }
    }

    //set new history length
    _historyLength = value;

    //set the history to show/hide
    _showHistory = tag_showHistory;

    _busy = false;
}


/**
 * @fn    zone1Value
 * @brief  set Zone 1 radius
 *
 * */
void GraphicsWidget::zone1Value(double value)
{
    if(zone1)
    {
        this->_scene->removeItem(zone1);
        delete(zone1);
        zone1 = NULL;
    }

    zone(0, value, _zone1Red);
}

/**
 * @fn    zone2Value
 * @brief  set Zone 2 radius
 *
 * */
void GraphicsWidget::zone2Value(double value)
{
    if(zone2)
    {
        this->_scene->removeItem(zone2);
        delete(zone2);
        zone2 = NULL;
    }

    zone(1, value, _zone2Red);
}


/**
 * @fn    setAlarm
 * @brief  set alarms
 *
 * */
void GraphicsWidget::setAlarm(bool in, bool out)
{
    Q_UNUSED(in);
    if(zone1)
    {
        zone1->setOpacity(0);
        this->_scene->removeItem(zone1);
        delete(zone1);
        zone1 = NULL;
    }

    if(zone2)
    {
        zone2->setOpacity(0);
        this->_scene->removeItem(zone2);
        delete(zone2);
        zone2 = NULL;
    }

    if(out)
    {
        _alarmOut = true;

        //set outside zone to red
        if(_zone1Rad > _zone2Rad)
        {
            zone(0, _zone1Rad, true);

            zone(1, _zone2Rad, false);
        }
        else
        {
            zone(0, _zone1Rad, false);

            zone(1, _zone2Rad, true);
        }
    }
    else
    {
        _alarmOut = false;

        //set inside zone to red
        if(_zone1Rad < _zone2Rad)
        {
            zone(0, _zone1Rad, true);

            zone(1, _zone2Rad, false);
        }
        else
        {
            zone(0, _zone1Rad, false);

            zone(1, _zone2Rad, true);
        }
    }
}

/**
 * @fn    zones
 * @brief  set Zone radius
 *
 * */
void GraphicsWidget::zone(int zone, double radius, bool red)
{
    //add Zone 1
    if(zone == 0)
    {
        QPen pen = QPen(QBrush(Qt::green), 0.005) ;

        _zone1Red = false;

        if(red)
        {
            pen.setColor(Qt::red);
            _zone1Red = true;
        }
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(0.03);

        if(!zone1)
        {
            zone1 = this->_scene->addEllipse(-_zone1Rad, -_zone1Rad, 2*_zone1Rad, 2*_zone1Rad);
        }
        zone1->setPen(pen);
        zone1->setBrush(Qt::NoBrush);
        zone1->setToolTip("Zone1");
        zone1->setPos(0, 0);
        _zone1Rad = radius;

        if(_geoFencingMode)
        {
            zone1->setOpacity(1);
        }
        else
        {
            zone1->setOpacity(0);
        }
    }
    else
    //add Zone 2
    {
        QPen pen = QPen(QBrush(Qt::green), 0.005) ;

        _zone2Red = false;

        if(red)
        {
            pen.setColor(Qt::red);
            _zone2Red = true;
        }
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(0.03);

        if(!zone2)
        {
            zone2 = this->_scene->addEllipse(-_zone2Rad, -_zone2Rad, 2*_zone2Rad, 2*_zone2Rad);
        }
        zone2->setPen(pen);
        zone2->setBrush(Qt::NoBrush);
        zone2->setToolTip("Zone2");
        zone2->setPos(0, 0);
        _zone2Rad = radius;

        if(_geoFencingMode)
        {
            zone2->setOpacity(1);
        }
        else
        {
            zone2->setOpacity(0);
        }
    }

    _maxRad = (_zone1Rad > _zone2Rad) ? _zone1Rad : _zone2Rad;
    _minRad = (_zone1Rad > _zone2Rad) ? _zone2Rad : _zone1Rad;
}

void GraphicsWidget::centerOnAnchors(void)
{

    Anchor *a1 = this->_anchors.value(0, NULL);
    Anchor *a2 = this->_anchors.value(1, NULL);
    Anchor *a3 = this->_anchors.value(2, NULL);
    //Anchor *a4 = anc = this->anchors.value(0, NULL);

    QPolygonF p1 = QPolygonF() << QPointF(a1->a->pos()) << QPointF(a2->a->pos()) << QPointF(a3->a->pos()) ;


    emit centerRect(p1.boundingRect());

}

/**
 * @fn    showGeoFencingMode
 * @brief  if Geo-fencing mode selected - then place anchor 0 in the middle
 *         and draw two concentric circles for the two zones
 *         hide other anchors
 *
 * */
void GraphicsWidget::showGeoFencingMode(bool set)
{
    Anchor *anc;
    int i;

    if(set)
    {
        double a1x = 0.0;
        double a1y = 0.0;
        double a1z = 0.0;

        bool showHistoryTemp = _showHistory;

        //disable tag history and clear any
        setShowTagHistory(false);

        _showHistoryP = showHistoryTemp ; //so that the show/hide tag history will be restored when going back to tracking mode

        //clear all Tag entries
        clearTags();

        //place anchor ID 0 at 0,0
        anchPos(0, a1x, a1y, a1z, true, false);

        zone(0, _zone1Rad, _zone1Red);

        zone(1, _zone2Rad, _zone2Red);

        zone1->setOpacity(1);
        zone2->setOpacity(1);

        for(i=1; i<4; i++)
        {
            anc = this->_anchors.value(i, NULL);

            if(anc)
            {
                anc->a->setOpacity(0);
                anc->ancLabel->setOpacity(0);
            }
        }

        if(0)
        {
            //center the scene on anchor 0
            emit centerAt(a1x, a1y);
        }
        else //zoom out so the zones can be seen
        {

            double rad = _zone2Rad > _zone1Rad ? _zone2Rad : _zone1Rad ;
            QPolygonF p1 = QPolygonF() << QPointF( a1x - rad, a1y) << QPointF(a1x + rad, a1y) << QPointF(a1x, a1y + rad) << QPointF(a1x, a1y - rad) ;

            emit centerRect(p1.boundingRect());
        }


        _geoFencingMode = true;
    }
    else
    {

        //clear all Tag entries
        clearTags();

        for(i=0; i<4; i++)
        {
            anc = this->_anchors.value(i, NULL);

            if(anc)
            {
                if(anc->show)
                {
                    anc->a->setOpacity(1);
                    anc->ancLabel->setOpacity(1);
                }
            }
        }

        zone1->setOpacity(0);
        zone2->setOpacity(0);

        this->_scene->removeItem(zone1);
        delete(zone1);
        zone1 = NULL;

        this->_scene->removeItem(zone2);
        delete(zone2);
        zone2 = NULL;

        setShowTagHistory(_showHistoryP);

        //center the centriod of the triangle given by anchors (0,1,2) points
        //

        centerOnAnchors();

        _geoFencingMode = false;
    }

    ui->tagTable->setColumnHidden(ColumnX,_geoFencingMode); //x
    ui->tagTable->setColumnHidden(ColumnY,_geoFencingMode); //y
    ui->tagTable->setColumnHidden(ColumnZ,_geoFencingMode); //z
    ui->tagTable->setColumnHidden(ColumnR95,_geoFencingMode); //r95
    ui->tagTable->setColumnHidden(ColumnRA1,_geoFencingMode); //range to A1
    ui->tagTable->setColumnHidden(ColumnRA2,_geoFencingMode); //range to A2
    ui->tagTable->setColumnHidden(ColumnRA3,_geoFencingMode); //range to A3
    ui->tagTable->setColumnHidden(ColumnLocatingcircle,_geoFencingMode);
}

/**
 * @fn    setShowTagAncTable
 * @brief  hide/show Tag and Anchor tables
 *
 * */
void GraphicsWidget::setShowTagAncTable(bool anchorTable, bool tagTable, bool ancTagCorr)
{

    if(!tagTable)
    {
        ui->tagTable->hide();
    }
    else
    {
        ui->tagTable->show();
    }

    //hide or show Anchor-Tag correction table
    hideTACorrectionTable(!ancTagCorr);

    if(!anchorTable)
    {
        ui->anchorTable->hide();
    }
    else
    {
        ui->anchorTable->show();
    }

}

/**
 * @fn    setShowTagHistory
 * @brief  if show Tag history is checked then display last N Tag locations
 *         else only show the current/last one
 *
 * */
void GraphicsWidget::setShowTagHistory(bool set)
{
    _busy = true ;

    if(set != _showHistory) //the value has changed
    {
        //for each tag
        if(set == false) //we want to hide history - clear the array
        {
            QMap<quint64, Tag*>::iterator i = _tags.begin();

            while(i != _tags.end())
            {
                Tag *tag = i.value();
                for(int idx=0; idx<_historyLength; idx++ )
                {
                    QAbstractGraphicsShapeItem *tag_p = tag->p[idx];
                    if(tag_p)
                    {
                        tag_p->setOpacity(0); //hide it

                        this->_scene->removeItem(tag_p);
                        delete(tag_p);
                        tag_p = NULL;
                        tag->p[idx] = 0;
                    }
                }
                tag->idx = 0; //reset history
                i++;
            }
        }
        else
        {

        }

        _showHistoryP = _showHistory = set; //update the value
    }

    _busy = false;
}

/**
 * @fn    addNewAnchor
 * @brief  add new Anchor with anchId into the tags QMap
 *
 * */
void GraphicsWidget::addNewAnchor(quint64 anchId, bool show)
{
    Anchor *anc;

   // qDebug() << "Add new Anchor: 0x" + QString::number(anchId, 16);

    //insert into table, and create an array to hold history of its positions
    _anchors.insert(anchId,new(Anchor));
    anc = this->_anchors.value(anchId, NULL);

    anc->a = NULL;
    //add text label and show by default
    {
        anc->ancLabel = new QGraphicsSimpleTextItem(NULL);
        anc->ancLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        anc->ancLabel->setZValue(1);
        anc->ancLabel->setText(QString::number(anchId));
        QFont font = anc->ancLabel->font();
        font.setPointSize(FONT_SIZE);
        font.setWeight(QFont::Normal);
        anc->ancLabel->setFont(font);
        this->_scene->addItem(anc->ancLabel);
    }
    anc->show = show;

}

/**
 * @fn    ancRanges
 * @brief  the inputs are anchor to anchor ranges
 *         e.g. 0,1 is anchor 0 to anchor 1
 *         e.g. 0,2 is anchor 0 to anchor 2
 * */
void GraphicsWidget::ancRanges(int a01, int a02, int a12)
{
    if(_busy)
    {
       // qDebug() << "(Widget - busy IGNORE) anchor ranges";
    }
    else
    {
        Anchor *anc0 = _anchors.value(0x0, NULL);
        Anchor *anc1 = _anchors.value(0x1, NULL);
        Anchor *anc2 = _anchors.value(0x2, NULL);

        qreal rA01 = ((double)a01)/1000;
        qreal rA02 = ((double)a02)/1000;
        qreal rA12 = ((double)a12)/1000;

        _busy = true ;

      //  qDebug() << "Anchor Ranges: " << rA01 << rA02 << rA12 ;

        if((anc0 != NULL) && (anc1 != NULL) && (anc2 != NULL))
        {
            QPen pen = QPen(QColor::fromRgb(0, 0, 255, 255));
            qreal x_coord = anc0->p.x() + rA01;
            qreal pwidth = 0.025;
            //qreal penw = pen.widthF() ;
            pen.setWidthF(pwidth);
            //penw = pen.widthF() ;
            QLineF line01 = QLineF(anc0->p, QPointF(x_coord, anc0->p.y()));


            if(!_line01)
            {
                _line01 = this->_scene->addLine(line01, pen);
            }
            else
            {
                _line01->setLine(line01);
            }
        }
        _busy = false ;
    }
}

bool isFirst = true;
void GraphicsWidget::calibrationButtonClicked()
{

    if(!serial_connected)
    {
        QMessageBox::warning(NULL,"提示","串口未连接,请连接串口","关闭");
        return;
    }

    if(isFirst && !startCalibration)
    {
        QString okText,titleText,setText;
        //QString cancelBtnText;
        if(language)
        {
            okText = "确定所有标签已经关机!开始标定";
            //cancelBtnText = "不再弹出";
            titleText = "自标定使用须知";
            setText = "自标定功能最少支持3个基站，最多支持8个基站。\n使用前需确认以下内容：\n\n自标定时电脑连接A0基站，自标定开启前，将所有标签关机！！\n设A0为坐标系(0,0)点，A1与A0构成坐标系X正半轴；\nA2和A3置于Y轴正半轴上，既Y轴坐标大于0；\n其他基站任意摆放即可。";
        }
        else
        {
            okText = "OK";
            //cancelBtnText = "No display pop";
            titleText = "Instructions";
            setText = "The self-calibration function supports a minimum of 3 base stations and a maximum of 8 base stations. Before use, the following should be confirmed: \nA0: the default coordinate system (0,0) point. \nA1: should be placed on the X positive axis of A0, and the Y axis is the same as the Y axis of A0. \nA2: should be placed on the Y positive axis of the Y axis, both the Y axis coordinate is greater than 0.";
        }
        QPushButton *okbtn = new QPushButton(okText);
        //QPushButton *cancelbtn = new QPushButton(cancelBtnText);
        QMessageBox *mymsgbox = new QMessageBox;
        mymsgbox->setIcon(QMessageBox::Warning);
        mymsgbox->setWindowTitle(titleText);
        mymsgbox->setText(setText);
        mymsgbox->addButton(okbtn, QMessageBox::AcceptRole);
        //mymsgbox->addButton(cancelbtn, QMessageBox::RejectRole);
        mymsgbox->show();

        mymsgbox->exec();//阻塞等待用户输入
        if (mymsgbox->clickedButton()==okbtn)//点击了OK按钮
        {
        }
        else{
            isFirst = false;
        }
    }
    if(!startCalibration)
    {
        if(stationNumber < 3)
        {
            //提示 自标定失败，请最少选择3个基站，再点击开始
            if(language){
                QMessageBox::warning(NULL,"自标定失败","自标定失败，请最少选择3个基站，再点击开始","我知道了");
            }
            else
            {
                QMessageBox::warning(NULL,"Failed","Self-calibration failed, please select at least 3 base stations and click Start.","OK");
            }

            return;
        }
        m_calibrationTimer_100ms->start();
    }

    reseveSetok = false;
    startCalibration = !startCalibration;
    if(language)
    {
        ui->pushButton->setText(startCalibration?"取消自标定":"基站自标定");
    }
    else{
        ui->pushButton->setText(startCalibration?"Calibration End":"Calibration Start");
    }
    ui->anchorTable->setDisabled(startCalibration);

    RTLSDisplayApplication::client()->setUseAutoPos(startCalibration);
    if(startCalibration)
    {
        RTLSDisplayApplication::serialConnection()->writeData("$ancrangestart\r\n");
        calibrationTimerRestart();
        calibrationIndex = 1;
        emit LoadWidgetIndexChange(calibrationIndex);
        calibrationTextVisible = true;
        emit LoadWidgetVisibleChange(calibrationTextVisible);
    }
    else
    {
        RTLSDisplayApplication::serialConnection()->writeData("$ancrangestop\r\n");
        aid_last = 0;
        calibrationTimerStop();
        calibrationTextVisible = false;
        emit LoadWidgetVisibleChange(calibrationTextVisible);
    }

}

void GraphicsWidget::helpBtnReleased()
{
    QString okText,titleText,setText;
    if(language)
    {
        okText = "确定";
        titleText = "自标定使用须知";
        setText = "自标定功能最少支持3个基站，最多支持8个基站。\n使用前需确认以下内容：\n\n自标定时电脑连接A0基站，自标定开启前，将所有标签关机！！\n设A0为坐标系(0,0)点，A1与A0构成坐标系X正半轴；\nA2和A3置于Y轴正半轴上，既Y轴坐标大于0；\n其他基站任意摆放即可。";
    }
    else
    {
        okText = "OK";
        titleText = "Instructions";
        setText = "The self-calibration function supports a minimum of 3 base stations and a maximum of 8 base stations. Before use, the following should be confirmed: \nA0: the default coordinate system (0,0) point. \nA1: should be placed on the X positive axis of A0, and the Y axis is the same as the Y axis of A0. \nA2/A3: should be placed on the Y positive axis of the Y axis, both the Y axis coordinate is greater than 0.";
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

static QList<vec3d> m_posList;  //存放原始基站点坐标
static QList<vec3d> m_posList2; //存放重新排序后的点坐标

void GraphicsWidget::setCanvasVisible(bool visible)
{
    if(visible != canvasVisible)
    {
        canvasVisible = visible;
        canvasShow(visible);
    }
}

void GraphicsWidget::setCanvasInfoVisible(bool visible)
{
    canvasInfomationVisible = visible;
    m_QQuickWidget->rootContext()->setContextProperty("canvasInfomationVisible", canvasInfomationVisible);
}

void GraphicsWidget::setCanvasFontSize(int size)
{
    canvasFontSize = size;
    if(!m_textList.empty())
    {
        for (int i = 0; i < m_textList.size(); i++)
        {
            QFont font = m_textList[i]->font();
            font.setPointSize(canvasFontSize);
            font.setWeight(QFont::Normal);
            m_textList[i]->setFont(font);
        }
    }

    if(m_areaText)
    {
        QFont font = m_areaText->font();
        font.setPointSize(canvasFontSize);
        font.setWeight(QFont::Normal);
        m_areaText->setFont(font);
    }
}

void GraphicsWidget::canvasShow(bool visible)
{
    if(!visible)
    {
        if(m_gi)
        {
            this->_scene->removeItem(m_gi);
            delete m_gi;
            m_gi = NULL;
        }

        if(!m_textList.isEmpty())
        {
            for (int i = 0; i < m_textList.size(); i++)
            {
                this->_scene->removeItem(m_textList[i]);
                delete m_textList[i];
                m_textList[i] = NULL;
            }
            m_textList.clear();
        }

        if(m_areaText)
        {
            this->_scene->removeItem(m_areaText);
            delete m_areaText;
            m_areaText = NULL;
        }
        return;
    }

    QPolygonF polygon;
    m_posList = RTLSDisplayApplication::client()->getAnchPos();
    static bool c_isFirst = true;
    if(!c_isFirst)
    {
        if(m_gi)
        {
            this->_scene->removeItem(m_gi);
            delete m_gi;
            m_gi = NULL;
        }

        if(!m_textList.isEmpty())
        {
            for (int i = 0; i < m_textList.size(); i++)
            {
                this->_scene->removeItem(m_textList[i]);
                delete m_textList[i];
                m_textList[i] = NULL;
            }
            m_textList.clear();
        }

        if(m_areaText)
        {
            this->_scene->removeItem(m_areaText);
            delete m_areaText;
            m_areaText = NULL;
        }
    }
    c_isFirst = false;

    if(m_posList.size() < 2)
    {
        canvasVisible = false;
        if(language)
        {
            QMessageBox::warning(NULL,"测绘启动失败","测绘功能仅在勾选2个及以上基站时可用","我知道了");
        }
        else
        {
            QMessageBox::warning(NULL,"Startup failure","Function is only available when 2 or more base stations are selected","Ok");
        }

        ViewSettingsWidget::viewsettingswidget->setCheckCanvesAreaState(false);
        return;
    }

    calculatePoint(m_posList);

    for (int i = 0; i < m_posList2.size(); i++) {
        polygon = polygon<<QPointF(m_posList2[i].x, m_posList2[i].y);
        //计算各边长
        m_textList.append(new QGraphicsSimpleTextItem(NULL));
        m_textList[i]->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        m_textList[i]->setZValue(1);
        if(i < m_posList2.size()-1)
        {
            m_textList[i]->setText(QString::number(sqrt((m_posList2[i].x-m_posList2[i+1].x)*(m_posList2[i].x-m_posList2[i+1].x)+(m_posList2[i].y-m_posList2[i+1].y)*(m_posList2[i].y-m_posList2[i+1].y)),'f',2)+"m");
            m_textList[i]->setPos((m_posList2[i].x+m_posList2[i+1].x)*0.5, (m_posList2[i].y+m_posList2[i+1].y)*0.5);
        }
        else{
            m_textList[i]->setText(QString::number(sqrt((m_posList2[i].x-m_posList2[0].x)*(m_posList2[i].x-m_posList2[0].x)+(m_posList2[i].y-m_posList2[0].y)*(m_posList2[i].y-m_posList2[0].y)),'f',2)+"m");
            m_textList[i]->setPos((m_posList2[i].x+m_posList2[0].x)*0.5, (m_posList2[i].y+m_posList2[0].y)*0.5);
        }
        QFont font = m_textList[i]->font();
        font.setPointSize(canvasFontSize);
        font.setWeight(QFont::Normal);
        m_textList[i]->setFont(font);
        this->_scene->addItem(m_textList[i]);
    }

    //描画图形
    m_gi = new QGraphicsPolygonItem;
    //    m_gi->setFlag (QGraphicsItem::ItemIsMovable);
    m_gi->setPolygon (polygon);
    QPen pen(qRgb (140,159,141));
    pen.setWidth (0);
    QLinearGradient gLg(-55,-35,55,35);
    gLg.setColorAt (0,qRgb (221,186,68));
    gLg.setColorAt (1,qRgb (221,226,68));
    m_gi->setPen (pen);
    m_gi->setBrush (gLg);
    m_gi->setOpacity (0.8);
    this->_scene->addItem(m_gi);

    //计算中心点
    vec3d posMin = getMinPos(m_posList2);
    vec3d posMax = getMaxPos(m_posList2);
    vec3d posCenter;
    posCenter = GetGravityPoint(m_posList2);

    //计算面积
    int point_num = m_posList2.size();
    qreal area = 0.0;
    if(point_num < 3)
    {
        area = 0.0;
    }
    else
    {
        double s = m_posList2[0].y * (m_posList2[point_num-1].x - m_posList2[1].x);
        for(int i = 1; i < point_num; ++i)
        {
            s += m_posList2[i].y * (m_posList2[i-1].x - m_posList2[(i+1)%point_num].x);
        }
        area = fabs(s/2.0);
    }
    m_areaText = new QGraphicsSimpleTextItem(NULL);
    m_areaText->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    m_areaText->setZValue(1);
    m_areaText->setText(QString::number(area,'f',2)+"m²");
    m_areaText->setPos(posCenter.x, posCenter.y);
    QFont font = m_areaText->font();
    font.setPointSize(canvasFontSize);
    font.setWeight(QFont::Normal);
    m_areaText->setFont(font);
    this->_scene->addItem(m_areaText);
}

void GraphicsWidget::calculatePoint(QList<vec3d> list)
{
    QList<vec3d> c_list = list;
    if(!m_posList2.isEmpty())
    {
        m_posList2.clear();
    }

    //# 查找x最小
    double x_min = c_list[0].x;
    double i_min = 0.0;

    for (int i = 0; i < c_list.size(); i++)
    {
        if(c_list[i].x < x_min)
        {
            x_min = c_list[i].x;
            i_min = i;
        }
    }

    //# 开始点
    vec3d point0;
    vec3d point1;
    vec3d point2;
    point0.x = c_list[i_min].x;
    point0.y = c_list[i_min].y;
    point0.z = 0.0;
    m_posList2.append(point0);

    //# 排序
    int limit_count = 5000;  //防崩溃
    for (int i = 0; i < c_list.size(); i++)
    {
        if(i == i_min)  //跳过原点
        {
            continue;
        }
        point1.x = c_list[i].x-point0.x;
        point1.y = c_list[i].y-point0.y;

        for (int j = i+1; j <  c_list.size(); j++)
        {
            if(j == i_min)  //跳过原点
            {
                continue;
            }
            point2.x = c_list[j].x-point0.x;
            point2.y = c_list[j].y-point0.y;

            double PxQ = point1.x*point2.y - point2.x*point1.y;
            //# 对调，，,逆时针，或者在同轴上而且离原点远
            if(PxQ >= 0 || (0 == PxQ && (abs(point1.x)+abs(point1.y) > abs(point2.x)+abs(point2.y))))
            {
                //# 对调
                double xtemp = c_list[j].x;
                double ytemp = c_list[j].y;

                c_list[j].x = c_list[i].x;
                c_list[j].y = c_list[i].y;

                c_list[i].x = xtemp;
                c_list[i].y = ytemp;

                //# 重新计算
                point1.x = point2.x;
                point1.y = point2.y;
            }
        }
        vec3d c_point;
        c_point.x = c_list[i].x;
        c_point.y = c_list[i].y;
        c_point.z = 0;
        m_posList2.append(c_point);

        limit_count = limit_count-1;
        if(limit_count < 0)
        {
            qInfo()<<" over limit, please check programe";
            break;
        }
    }
}

vec3d GraphicsWidget::GetGravityPoint(QList<vec3d> posList)
{
    static vec3d gravityPoint;

    vec3d p0 = posList[0];
    vec3d p1 = posList[1];
    vec3d p2;
    double sumarea = 0, sumx = 0, sumy = 0;

    for (int i = 2; i < posList.size(); i++)
    {
        p2 = posList[i];
        double area = (p0.x*p1.y + p1.x*p2.y + p2.x*p0.y - p1.x*p0.y - p2.x*p1.y - p0.x*p2.y)/2;//求三角形的面积
        sumarea += area;
        sumx += (p0.x + p1.x + p2.x)*area; //求∑cx[i] * s[i]和∑cy[i] * s[i]
        sumy += (p0.y + p1.y + p2.y)*area;
        p1 = p2;//求总面积
    }
    gravityPoint.x = sumx / sumarea / 3;
    gravityPoint.y = sumy / sumarea / 3;
    gravityPoint.z = 0.0;
    return gravityPoint;
}

vec3d GraphicsWidget::getMinPos(QList<vec3d> posList)
{
    vec3d v;
    double posx = 999;
    double posy = 999;
    for (int i = 0; i < posList.size(); i++)
    {
        if(posList[i].x < posx)
        {
            posx = posList[i].x;
        }
        if(posList[i].y < posy)
        {
            posy = posList[i].y;
        }
    }
    v.x = posx;
    v.y = posy;
    v.z = 0.0;
    return v;
}

vec3d GraphicsWidget::getMaxPos(QList<vec3d> posList)
{
    vec3d v;
    double posx = -999;
    double posy = -999;
    for (int i = 0; i < posList.size(); i++)
    {
        if(posList[i].x > posx)
        {
            posx = posList[i].x;
        }
        if(posList[i].y > posy)
        {
            posy = posList[i].y;
        }
    }
    v.x = posx;
    v.y = posy;
    v.z = 0.0;
    return v;
}

void GraphicsWidget::sizeChanged(QSize size)
{
    m_QQuickWidget->move(10,size.height()-165*0.8);
    emit viewSizeChange(size);
}

void GraphicsWidget::rotateChanged(qreal rotate)
{
    //夹角计算为逆时针夹角，所以此处rotate取反
    canvas_rotate = -rotate;
    m_QQuickWidget->rootContext()->setContextProperty("canvas_rotate", QString::number(canvas_rotate, 'f', 2));
}

void GraphicsWidget::visibleRectChanged()
{
    canvas_posX = QString::number(graphicsView()->visibleRect().x(),'f',2);
    canvas_posY = QString::number(graphicsView()->visibleRect().y(),'f',2);
    m_QQuickWidget->rootContext()->setContextProperty("canvas_posX", canvas_posX);
    m_QQuickWidget->rootContext()->setContextProperty("canvas_posY", canvas_posY);
}

void GraphicsWidget::resetButtonClicked()
{
    centerOnAnchors();
    RTLSDisplayApplication::graphicsView()->resetRotate();
    canvas_rotate = 0;
    m_QQuickWidget->rootContext()->setContextProperty("canvas_rotate", QString::number(canvas_rotate, 'f', 2));
}

void GraphicsWidget::scaleChanged(qreal scale)
{
    canvas_scale = QString::number(scale,'f',2);
    m_QQuickWidget->rootContext()->setContextProperty("canvas_scale", canvas_scale);
}

/**
 * @fn    anchPos
 * @brief  add an anchor to the database and show on screen
 *
 * */
void GraphicsWidget::anchPos(quint64 anchId, double x, double y, double z, bool show, bool update)
{
    if(_busy)
    {
        //qDebug() << "(Widget - busy IGNORE) Anch: 0x" + QString::number(anchId, 16) << " " << x << " " << y << " " << z;
    }
    else
    {
        Anchor *anc = NULL;

        _busy = true ;

        anc = _anchors.value(anchId, NULL);

        if(!anc) //add new anchor to the anchors array
        {
            addNewAnchor(anchId, show);
            insertAnchor(anchId, x, y, z, RTLSDisplayApplication::instance()->client()->getTagCorrections(anchId), show);
            anc = this->_anchors.value(anchId, NULL);
        }

        //add the graphic shape (anchor image)
        if (anc->a == NULL)
        {
            //QAbstractGraphicsShapeItem *anch = this->_scene->addEllipse(-ANC_SIZE/2, -ANC_SIZE/2, ANC_SIZE, ANC_SIZE);
            QAbstractGraphicsShapeItem *anch = this->_scene->addEllipse(-_tagSize/2, -_tagSize/2, _tagSize, _tagSize);
            anch->setPen(Qt::NoPen);
            //anch->setBrush(QBrush(QColor::fromRgb(255, 0, 0, 200)));
            anch->setBrush(QBrush(QColor::fromRgb(246, 134 , 86, 255)));
            anch->setToolTip("0x"+QString::number(anchId, 16));
            anc->a = anch;
        }

        anc->a->setOpacity(anc->show ? 1.0 : 0.0);
        anc->ancLabel->setOpacity(anc->show ? 1.0 : 0.0);
        anc->ancLabel->setPos(x + 0.15, y + 0.15);
        anc->a->setPos(x, y);
//        qInfo()<<" ------- x: "<<x<<", y: "<<y<<", z: "<<z<<", id: "<<anchId;
        if(update) //update Table entry
        {
            //int r = anchId & 0x3;
            int r = anchId;
            _ignore = true;
            ui->anchorTable->item(r,ColumnX)->setText(QString::number(x, 'f', 2));
            ui->anchorTable->item(r,ColumnY)->setText(QString::number(y, 'f', 2));
            _ignore = false;
        }
        _busy = false ;
        //qDebug() << "(Widget) Anch: 0x" + QString::number(anchId, 16) << " " << x << " " << y << " " << z;
        if(canvasVisible)
        {
            canvasShow(true);
        }
    }

}

/**
 * @fn    anchTableEditing
 * @brief  enable or disable editing of anchor table
 *
 * */
void GraphicsWidget::anchTableEditing(bool enable)
{
    //ui->anchorTable->setEnabled(enable);

    if(!enable)
    {
        for(int i = 0; i<3; i++)
        {
            Qt::ItemFlags flags = ui->anchorTable->item(i,ColumnX)->flags();
            flags = flags &  ~(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled) ;
            ui->anchorTable->item(i,ColumnX)->setFlags(flags);
            ui->anchorTable->item(i,ColumnY)->setFlags(flags);
        }
    }
    else
    {
        for(int i = 0; i<3; i++)
        {
            Qt::ItemFlags flags = ui->anchorTable->item(i,ColumnX)->flags();
            ui->anchorTable->item(i,ColumnX)->setFlags(flags | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
            ui->anchorTable->item(i,ColumnY)->setFlags(flags | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        }
    }

}

bool GraphicsWidget::event(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        if(language)
       {
          ui->tagTable->setHorizontalHeaderLabels(tableHeader);
          ui->anchorTable->setHorizontalHeaderLabels(anchorHeader);
          ui->pushButton->setText(startCalibration?"取消自标定":"基站自标定");
       }
        else{
          ui->tagTable->setHorizontalHeaderLabels(tableHeader_en);
          ui->anchorTable->setHorizontalHeaderLabels(anchorHeader_en);
          ui->pushButton->setText(startCalibration?"Calibration End":"Calibration Start");
        }
        m_QQuickWidget->rootContext()->setContextProperty("language", language);

    }

    return QWidget::event(event);
}

void GraphicsWidget::calibrationFailed()
{
    reseveSetok = false;
    startCalibration = !startCalibration;
    if(language)
    {
        ui->pushButton->setText(startCalibration?"取消自标定":"基站自标定");
    }
    else{
        ui->pushButton->setText(startCalibration?"Calibration End":"Calibration Start");
    }
    ui->anchorTable->setDisabled(startCalibration);

    RTLSDisplayApplication::client()->setUseAutoPos(startCalibration);
    if(startCalibration)
    {
        RTLSDisplayApplication::serialConnection()->writeData("$ancrangestart\r\n");
    }
    else
    {
        RTLSDisplayApplication::serialConnection()->writeData("$ancrangestop\r\n");
        aid_last = 0;
    }
    calibrationTimerStop();
    calibrationTextVisible = false;
    emit LoadWidgetVisibleChange(calibrationTextVisible);
}

void GraphicsWidget::calibrationSuccess()
{
    calibrationIndex++;
    emit LoadWidgetIndexChange(calibrationIndex);
    calibrationTimerRestart();
}

void GraphicsWidget::calibrationTimerRestart()
{
    signalTimeOut = false;
    m_calibrationTimer->start();
}

void GraphicsWidget::calibrationTimerStop()
{
    signalTimeOut = false;
    m_calibrationTimer->stop();
}

void GraphicsWidget::setTagSize(float size)
{
    _tagSize = size;
}
