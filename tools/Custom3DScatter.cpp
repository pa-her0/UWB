#include "Custom3DScatter.h"
#include <QtGui/QMouseEvent>
#include <QtDataVisualization/Q3DCamera>

Custom3DScatter::Custom3DScatter()
    : Q3DScatter(), m_panning(false)
{
    // Initial setup for panning
}

void Custom3DScatter::mousePressEvent(QMouseEvent *event)
{
//    if (event->button() == Qt::LeftButton) {
//        m_panning = true;
//        m_lastMousePos = event->pos();
//        m_initialCameraPosition = scene()->activeCamera()->position();
//        m_initialCameraTarget = scene()->activeCamera()->target();
//        setCursor(Qt::ClosedHandCursor);
//    }
    Q3DScatter::mousePressEvent(event);
}

void Custom3DScatter::mouseMoveEvent(QMouseEvent *event)
{
//    if (m_panning) {
//        QPoint currentPos = event->pos();
//        QPoint delta = currentPos - m_lastMousePos;

//        // Calculate new target position
//        QVector3D newTarget = m_initialCameraTarget;
//        newTarget.setX(m_initialCameraTarget.x() + 11.f);
//        newTarget.setY(m_initialCameraTarget.y() - 11.f);

//        // Calculate new camera position
//        QVector3D newCameraPosition = m_initialCameraPosition;
//        newCameraPosition.setX(m_initialCameraPosition.x() - delta.x() * 10.f);
//        newCameraPosition.setY(m_initialCameraPosition.y() + delta.y() * 10.f);

//        scene()->activeCamera()->setPosition(newCameraPosition);

//        scene()->activeCamera()->setTarget(newTarget);

//        m_lastMousePos = currentPos;
//    }
    Q3DScatter::mouseMoveEvent(event);
}

void Custom3DScatter::mouseReleaseEvent(QMouseEvent *event)
{
//    if (event->button() == Qt::LeftButton) {
//        m_panning = false;
//        setCursor(Qt::ArrowCursor);
//        m_lastMousePos = QPoint();
//    }
    Q3DScatter::mouseReleaseEvent(event);
}
