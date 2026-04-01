// -------------------------------------------------------------------------------------------------------------------
//
//  File: UwbTrajectoryView.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "UwbTrajectoryView.h"
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>
#include <cmath>

UwbTrajectoryView::UwbTrajectoryView(QWidget *parent)
    : QGraphicsView(parent)
    , _scene(new QGraphicsScene(this))
    , _trajectoryPath(nullptr)
    , _startPoint(nullptr)
    , _endPoint(nullptr)
    , _highlightPoint(nullptr)
    , _showGrid(true)
    , _showAnchors(true)
    , _showStartEnd(true)
    , _isPanning(false)
{
    setScene(_scene);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    setupScene();
}

UwbTrajectoryView::~UwbTrajectoryView()
{
}

void UwbTrajectoryView::setupScene()
{
    _scene->setBackgroundBrush(QBrush(QColor(248, 250, 252))); // Light gray background
}

void UwbTrajectoryView::setTrajectory(const QList<TrajectoryPoint> &points)
{
    _points = points;
    drawGrid();
    drawAnchors();
    drawTrajectory();
    drawStartEnd();
    updateTransform();
}

void UwbTrajectoryView::clearTrajectory()
{
    _points.clear();
    if (_trajectoryPath) {
        _scene->removeItem(_trajectoryPath);
        delete _trajectoryPath;
        _trajectoryPath = nullptr;
    }
    if (_startPoint) {
        _scene->removeItem(_startPoint);
        delete _startPoint;
        _startPoint = nullptr;
    }
    if (_endPoint) {
        _scene->removeItem(_endPoint);
        delete _endPoint;
        _endPoint = nullptr;
    }
    clearHighlight();
}

void UwbTrajectoryView::setAnchors(const QList<AnchorInfo> &anchors)
{
    _anchors = anchors;
    drawAnchors();
}

void UwbTrajectoryView::clearAnchors()
{
    _anchors.clear();
    for (auto item : _anchorItems) {
        _scene->removeItem(item);
        delete item;
    }
    _anchorItems.clear();
}

void UwbTrajectoryView::drawGrid()
{
    // Remove old grid
    for (auto item : _gridItems) {
        _scene->removeItem(item);
        delete item;
    }
    _gridItems.clear();

    if (!_showGrid || _points.isEmpty()) return;

    QRectF rect = dataBoundingRect();
    if (!rect.isValid()) return;

    // Add padding
    rect.adjust(-MARGIN, -MARGIN, MARGIN, MARGIN);

    // Calculate grid spacing
    double spanX = rect.width();
    double spanY = rect.height();

    int numTicks = 6;
    double stepX = spanX / numTicks;
    double stepY = spanY / numTicks;

    // Round to nice numbers
    auto roundStep = [](double step) -> double {
        double magnitude = std::pow(10, std::floor(std::log10(step)));
        double normalized = step / magnitude;
        if (normalized < 2) return magnitude;
        if (normalized < 5) return 2 * magnitude;
        return 5 * magnitude;
    };

    stepX = roundStep(stepX);
    stepY = roundStep(stepY);

    double startX = std::floor(rect.left() / stepX) * stepX;
    double startY = std::floor(rect.top() / stepY) * stepY;

    QPen gridPen(QColor(228, 231, 235));
    gridPen.setWidthF(1);

    // Vertical lines
    for (double x = startX; x <= rect.right(); x += stepX) {
        QGraphicsLineItem *line = new QGraphicsLineItem(x, rect.top(), x, rect.bottom());
        line->setPen(gridPen);
        _scene->addItem(line);
        _gridItems.append(line);

        // X-axis label
        QGraphicsTextItem *text = new QGraphicsTextItem(QString::number(x, 'f', 2));
        text->setPos(x, rect.bottom() + 5);
        text->setDefaultTextColor(QColor(51, 65, 85));
        _scene->addItem(text);
        _gridItems.append(text);
    }

    // Horizontal lines
    for (double y = startY; y <= rect.bottom(); y += stepY) {
        QGraphicsLineItem *line = new QGraphicsLineItem(rect.left(), y, rect.right(), y);
        line->setPen(gridPen);
        _scene->addItem(line);
        _gridItems.append(line);

        // Y-axis label
        QGraphicsTextItem *text = new QGraphicsTextItem(QString::number(y, 'f', 2));
        text->setPos(rect.left() - 45, y - 8);
        text->setDefaultTextColor(QColor(51, 65, 85));
        _scene->addItem(text);
        _gridItems.append(text);
    }

    // Border
    QPen borderPen(QColor(203, 213, 225));
    borderPen.setWidthF(2);
    QGraphicsRectItem *border = new QGraphicsRectItem(rect);
    border->setPen(borderPen);
    border->setBrush(QBrush(Qt::white));
    _scene->addItem(border);
    _gridItems.append(border);
}

void UwbTrajectoryView::drawTrajectory()
{
    if (_trajectoryPath) {
        _scene->removeItem(_trajectoryPath);
        delete _trajectoryPath;
        _trajectoryPath = nullptr;
    }

    if (_points.size() < 2) return;

    // QGraphicsScene Y-axis points down, UWB coordinates Y-axis points up
    // Negate Y to flip correctly
    QPainterPath path;
    path.moveTo(_points[0].x(), -_points[0].y());

    for (int i = 1; i < _points.size(); ++i) {
        path.lineTo(_points[i].x(), -_points[i].y());
    }

    _trajectoryPath = new QGraphicsPathItem(path);
    QPen pen(QColor(37, 99, 235)); // Blue
    pen.setWidthF(3);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    _trajectoryPath->setPen(pen);
    _scene->addItem(_trajectoryPath);

    // Add clickable points
    for (int i = 0; i < _points.size(); ++i) {
        double px = _points[i].x();
        double py = -_points[i].y();
        QGraphicsEllipseItem *point = new QGraphicsEllipseItem(
            px - POINT_RADIUS, py - POINT_RADIUS,
            POINT_RADIUS * 2, POINT_RADIUS * 2);
        point->setBrush(QColor(37, 99, 235));
        point->setPen(Qt::NoPen);
        point->setData(0, i);
        point->setAcceptHoverEvents(true);
        point->setToolTip(QString("Time: %1\nX: %2\nY: %3")
            .arg(_points[i].time().toString("yyyy-MM-dd hh:mm:ss.zzz"))
            .arg(_points[i].x(), 0, 'f', 3)
            .arg(_points[i].y(), 0, 'f', 3));
        _scene->addItem(point);
        _gridItems.append(point);
    }
}

void UwbTrajectoryView::drawAnchors()
{
    clearAnchors();

    if (!_showAnchors) return;

    QColor anchorColor(217, 72, 15); // Orange-red
    QColor anchorBorder(124, 45, 18);

    for (const auto &anchor : _anchors) {
        double ax = anchor.x;
        double ay = -anchor.y; // flip Y

        QGraphicsEllipseItem *circle = new QGraphicsEllipseItem(
            ax - ANCHOR_RADIUS, ay - ANCHOR_RADIUS,
            ANCHOR_RADIUS * 2, ANCHOR_RADIUS * 2);
        circle->setBrush(anchorColor);
        circle->setPen(QPen(anchorBorder, 2));
        _scene->addItem(circle);
        _anchorItems.append(circle);

        QGraphicsTextItem *label = new QGraphicsTextItem(
            QString("%1 (%2, %3)").arg(anchor.name).arg(anchor.x, 0, 'f', 2).arg(anchor.y, 0, 'f', 2));
        label->setPos(ax + 12, ay - 20);
        label->setDefaultTextColor(anchorBorder);
        QFont font = label->font();
        font.setPointSize(10);
        font.setBold(true);
        label->setFont(font);
        _scene->addItem(label);
        _anchorItems.append(label);
    }
}

void UwbTrajectoryView::drawStartEnd()
{
    if (_startPoint) {
        _scene->removeItem(_startPoint);
        delete _startPoint;
        _startPoint = nullptr;
    }
    if (_endPoint) {
        _scene->removeItem(_endPoint);
        delete _endPoint;
        _endPoint = nullptr;
    }

    if (!_showStartEnd || _points.isEmpty()) return;

    // Start point - green
    _startPoint = new QGraphicsEllipseItem(
        _points[0].x() - 8, -_points[0].y() - 8, 16, 16);
    _startPoint->setBrush(QColor(22, 163, 74));
    _startPoint->setPen(Qt::NoPen);
    _scene->addItem(_startPoint);

    // End point - red
    _endPoint = new QGraphicsEllipseItem(
        _points.last().x() - 8, -_points.last().y() - 8, 16, 16);
    _endPoint->setBrush(QColor(220, 38, 38));
    _endPoint->setPen(Qt::NoPen);
    _scene->addItem(_endPoint);
}

void UwbTrajectoryView::highlightPoint(int index)
{
    clearHighlight();

    if (index < 0 || index >= _points.size()) return;

    const TrajectoryPoint &point = _points[index];
    _highlightPoint = new QGraphicsEllipseItem(
        point.x() - 12, -point.y() - 12, 24, 24);
    _highlightPoint->setBrush(Qt::transparent);
    _highlightPoint->setPen(QPen(QColor(250, 204, 21), 3)); // Yellow ring
    _scene->addItem(_highlightPoint);
}

void UwbTrajectoryView::clearHighlight()
{
    if (_highlightPoint) {
        _scene->removeItem(_highlightPoint);
        delete _highlightPoint;
        _highlightPoint = nullptr;
    }
}

QRectF UwbTrajectoryView::dataBoundingRect() const
{
    if (_points.isEmpty() && _anchors.isEmpty()) {
        return QRectF(0, 0, 10, 10);
    }

    double minX = 1e10, minY = 1e10;
    double maxX = -1e10, maxY = -1e10;

    for (const auto &p : _points) {
        minX = std::min(minX, p.x());
        minY = std::min(minY, -p.y()); // flipped Y
        maxX = std::max(maxX, p.x());
        maxY = std::max(maxY, -p.y()); // flipped Y
    }

    for (const auto &a : _anchors) {
        minX = std::min(minX, a.x);
        minY = std::min(minY, -a.y); // flipped Y
        maxX = std::max(maxX, a.x);
        maxY = std::max(maxY, -a.y); // flipped Y
    }

    double padX = std::max((maxX - minX) * 0.1, 0.5);
    double padY = std::max((maxY - minY) * 0.1, 0.5);

    return QRectF(minX - padX, minY - padY, maxX - minX + 2 * padX, maxY - minY + 2 * padY);
}

void UwbTrajectoryView::updateTransform()
{
    if (_points.isEmpty() && _anchors.isEmpty()) return;

    QRectF rect = dataBoundingRect();
    if (!rect.isValid()) return;

    fitInView(rect, Qt::KeepAspectRatio);
}

void UwbTrajectoryView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    updateTransform();
}

void UwbTrajectoryView::wheelEvent(QWheelEvent *event)
{
    double scaleFactor = 1.15;
    if (event->delta() < 0) {
        scaleFactor = 1.0 / scaleFactor;
    }

    scale(scaleFactor, scaleFactor);
}

void UwbTrajectoryView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _isPanning = true;
        _lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);

        // Check if clicked on a point
        QPointF scenePos = mapToScene(event->pos());
        int index = findNearestPoint(scenePos);
        if (index >= 0) {
            emit pointClicked(index, _points[index]);
            highlightPoint(index);
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void UwbTrajectoryView::mouseMoveEvent(QMouseEvent *event)
{
    if (_isPanning) {
        QPoint delta = event->pos() - _lastMousePos;
        _lastMousePos = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    } else {
        // Hover check
        QPointF scenePos = mapToScene(event->pos());
        int index = findNearestPoint(scenePos);
        if (index >= 0) {
            emit pointHovered(index, _points[index]);
        }
    }
    QGraphicsView::mouseMoveEvent(event);
}

void UwbTrajectoryView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

int UwbTrajectoryView::findNearestPoint(const QPointF &scenePos)
{
    if (_points.isEmpty()) return -1;

    int nearestIndex = -1;
    double minDist = 10.0; // Threshold in scene units

    for (int i = 0; i < _points.size(); ++i) {
        double dx = _points[i].x() - scenePos.x();
        double dy = (-_points[i].y()) - scenePos.y(); // match flipped Y
        double dist = std::sqrt(dx * dx + dy * dy);
        if (dist < minDist) {
            minDist = dist;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

void UwbTrajectoryView::setShowGrid(bool show)
{
    _showGrid = show;
    drawGrid();
}

void UwbTrajectoryView::setShowAnchors(bool show)
{
    _showAnchors = show;
    drawAnchors();
}

void UwbTrajectoryView::setShowStartEnd(bool show)
{
    _showStartEnd = show;
    drawStartEnd();
}
