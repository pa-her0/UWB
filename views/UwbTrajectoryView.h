// -------------------------------------------------------------------------------------------------------------------
//
//  File: UwbTrajectoryView.h
//
//  2D Trajectory visualization widget for UWB data
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef UWBTRAJECTORYVIEW_H
#define UWBTRAJECTORYVIEW_H

#include <QGraphicsView>
#include <QList>
#include "TrajectoryPoint.h"

class QGraphicsScene;
class QGraphicsPathItem;
class QGraphicsEllipseItem;
class QGraphicsTextItem;
class QGraphicsItem;

struct AnchorInfo
{
    QString name;
    double x;
    double y;
    QColor color;
};

class UwbTrajectoryView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit UwbTrajectoryView(QWidget *parent = nullptr);
    ~UwbTrajectoryView();

    void setTrajectory(const QList<TrajectoryPoint> &points);
    void clearTrajectory();

    void setAnchors(const QList<AnchorInfo> &anchors);
    void clearAnchors();

    void highlightPoint(int index);
    void clearHighlight();

    void setShowGrid(bool show);
    void setShowAnchors(bool show);
    void setShowStartEnd(bool show);

    QRectF dataBoundingRect() const;

signals:
    void pointClicked(int index, const TrajectoryPoint &point);
    void pointHovered(int index, const TrajectoryPoint &point);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupScene();
    void drawGrid();
    void drawTrajectory();
    void drawAnchors();
    void drawStartEnd();
    void updateTransform();
    int findNearestPoint(const QPointF &scenePos);

    QGraphicsScene *_scene;

    QList<TrajectoryPoint> _points;
    QList<AnchorInfo> _anchors;

    QGraphicsPathItem *_trajectoryPath;
    QGraphicsEllipseItem *_startPoint;
    QGraphicsEllipseItem *_endPoint;
    QGraphicsEllipseItem *_highlightPoint;
    QList<QGraphicsItem*> _anchorItems;
    QList<QGraphicsItem*> _gridItems;

    bool _showGrid;
    bool _showAnchors;
    bool _showStartEnd;
    bool _isPanning;
    QPoint _lastMousePos;

    static constexpr double POINT_RADIUS = 4.0;
    static constexpr double ANCHOR_RADIUS = 8.0;
    static constexpr double MARGIN = 50.0;
};

#endif // UWBTRAJECTORYVIEW_H
