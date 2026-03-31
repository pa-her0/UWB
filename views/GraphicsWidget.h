// -------------------------------------------------------------------------------------------------------------------
//
//  File: GraphicsWidget.h
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef GRAPHICSWIDGET_H
#define GRAPHICSWIDGET_H

#include <QGraphicsRectItem>
#include <QWidget>
#include <QAbstractItemView>
#include <QGraphicsView>
#include <QtCore/qmath.h>
#include "ViewSettingsWidget.h"

#include "trilateration.h"
#include <QtQuickWidgets/QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
namespace Ui {
class GraphicsWidget;
}

class GraphicsDataModel;
class QGraphicsScene;
class QModelIndex;
class GraphicsView;
class QAbstractGraphicsShapeItem;
class QGraphicsItem;

struct Tag
{
    Tag(void)
    {
        idx = 0;
        ridx = 0;
    }

    quint64 id;
    int idx;
    int ridx;
    QVector<QAbstractGraphicsShapeItem *> p;
    QAbstractGraphicsShapeItem *avgp;
    QAbstractGraphicsShapeItem *r95p;   //r95 circle around the average point, average of 100
    QAbstractGraphicsShapeItem *geop;   //geofencing circle around the (geofencing) anchor
    QAbstractGraphicsShapeItem *circle[8];

    bool r95Show;
    bool LocatingcircleShow[100];
    double tsPrev; //previous timestamp in sec
    double colourH;
    double colourS;
    double colourV;

    bool showLabel;

    QGraphicsSimpleTextItem *tagLabel;
    QString tagLabelStr;
};

struct Anchor
{
    Anchor(void)
    {
        idx = 0;
        ridx = 0;
    }
    quint64 id;
    int idx;
    int ridx;
    bool show;
    QAbstractGraphicsShapeItem * a;
    QGraphicsSimpleTextItem *ancLabel;

    QPointF p;
};

class GraphicsWidget : public QWidget
{
    Q_OBJECT

public:
    float _tagSize;
    enum Column {
        ColumnID = 0,   ///< 64 bit address of the anchor (uint64)
        ColumnX,        ///< X coordinate (double)
        ColumnY,        ///< Y coordinate (double)
        ColumnZ,        ///< Z coordinate (double)
       // ColumnBlinkRx,   ///< % of received blinks
        //ColumnLocations, ///< % of successful multilaterations
        ColumnLocatingcircle,
        ColumnPower,
        ColumnR95,       ///< R95
        ColumnRA0,
        ColumnRA1,
        ColumnRA2,
        ColumnRA3,
        ColumnRA4,
        ColumnRA5,
        ColumnRA6,
        ColumnRA7,
        ColumnRXpwr,    //依照原先保持不变，
        ColumnIDr,       ///< ID raw (hex) hidden
        ColumnCount


    };

    explicit GraphicsWidget(QWidget *parent = 0);
    ~GraphicsWidget();

    GraphicsView *graphicsView();


    int findTagRowIndex(QString &t);
    void insertTag(int ridx, QString &t, bool showR95, bool showLabel, QString &l);
    void tagIDToString(quint64 tagId, QString *t);
    void addNewTag(quint64 tagId);
    void addNewAnchor(quint64 ancId, bool show);
    void insertAnchor(int ridx, double x, double y, double z, int *array, bool show);
    void loadConfigFile(QString filename);
    void saveConfigFile(QString filename);

    void hideTACorrectionTable(bool hidden);
    bool event(QEvent *event);//语言切换响应事件

    void calibrationFailed();
    void calibrationSuccess();
    QTimer *m_calibrationTimer;
    QTimer *m_calibrationTimer_100ms;
    void calibrationTimerRestart();
    void calibrationTimerStop();
    void setTagSize(float size);

    QGraphicsPolygonItem  *m_gi;
    QGraphicsSimpleTextItem *m_areaText = nullptr;
    QList<QGraphicsSimpleTextItem *> m_textList;
    QGraphicsPixmapItem *m_pixmap;
    QQuickWidget *m_QQuickWidget;

    QString canvas_posX="0";
    QString canvas_posY="0";
    qreal canvas_rotate=0;
    QString canvas_scale="1";
    bool canvasInfomationVisible=true;
    bool calibrationTextVisible=false;
    int calibrationIndex=1;
    bool canvasVisible = false;
    int canvasFontSize = 10;

signals:
    void updateAnchorXYZ(int id, int x, double value);
    void updateTagCorrection(int aid, int tid, int value);
    void centerAt(double x, double y);
    void centerRect(const QRectF &visibleRect);


    void setTagHistory(int h);
    void viewSizeChange(QSize size);
    void LoadWidgetVisibleChange(bool visible);
    void LoadWidgetIndexChange(int index);

public slots:

    void setCanvasFontSize(int size);
    void centerOnAnchors(void);
    void anchTableEditing(bool);

    void tagPos(quint64 tagId, double x, double y, double z);
    void tagStats(quint64 tagId, double x, double y, double z, double r95);
    void tagRange(quint64 tagId, quint64 aId, double range,double rx_power,int alarm);


    void anchPos(quint64 anchId, double x, double y, double z, bool show, bool updatetable);

    void tagTableChanged(int r, int c);
    void anchorTableChanged(int r, int c);
    void tagTableClicked(int r, int c);
    void anchorTableClicked(int r, int c);
    void itemSelectionChanged(void);
    void itemSelectionChangedAnc(void);
    void clearTags(void);

    void setShowTagHistory(bool);
    void setShowTagAncTable(bool anchorTable, bool tagTable, bool ancTagCorr);
    void showGeoFencingMode(bool set);
    void zone1Value(double value);
    void zone2Value(double value);
    void tagHistoryNumber(int value);
    void zone(int zone, double radius, bool red);
    void setAlarm(bool in, bool out);

    void ancRanges(int a01, int a02, int a12);
    void calibrationButtonClicked();
    void helpBtnReleased();
    void setCanvasVisible(bool visible);
    void setCanvasInfoVisible(bool visible);
    void canvasShow(bool visible);
    void calculatePoint(QList<vec3d> list);
    vec3d GetGravityPoint(QList<vec3d> posList);
    vec3d getMinPos(QList<vec3d> posList);
    vec3d getMaxPos(QList<vec3d> posList);

    void sizeChanged(QSize size);
    void rotateChanged(qreal rotate);
    void visibleRectChanged();
    void resetButtonClicked();
    void scaleChanged(qreal scale);

protected slots:
    void onReady();


protected:
    void tagHistory(quint64 tagId);

public:
    Ui::GraphicsWidget *ui;

public:

    QGraphicsScene *_scene;

    QMap<quint64, Tag*> _tags;
    QMap<quint64, Anchor *> _anchors;
    QMap<quint64, QString> _tagLabels;


    int   _historyLength;
    bool _showHistory;
    bool _showHistoryP;
    bool _busy;
    bool _ignore;
    bool _geoFencingMode;
    bool _alarmOut;
    bool status[100]={0};
    int _selectedTagIdx;

    QAbstractGraphicsShapeItem *zone1;
    QAbstractGraphicsShapeItem *zone2;

    double _zone1Rad;
    double _zone2Rad;
    double _maxRad;
    double _minRad;
    bool _zone1Red;
    bool _zone2Red;


    QGraphicsLineItem * _line01;
    QGraphicsLineItem * _line02;
    QGraphicsLineItem * _line12;

    QStringList tableHeader;
    QStringList tableHeader_en;

    QStringList anchorHeader;
    QStringList anchorHeader_en;

    QPolygonF m_polygon;

};

#endif // GRAPHICSWIDGET_H
