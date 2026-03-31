#ifndef CUSTOM3DSCATTER_H
#define CUSTOM3DSCATTER_H

#include <QtDataVisualization/Q3DScatter>
#include <QtCore/QPoint>
#include <QtGui/QVector3D>

using namespace QtDataVisualization;

class Custom3DScatter : public Q3DScatter
{
    Q_OBJECT

public:
    Custom3DScatter();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_panning;
    QPoint m_lastMousePos;
    QVector3D m_initialCameraPosition;
    QVector3D m_initialCameraTarget;
};

#endif // CUSTOM3DSCATTER_H
