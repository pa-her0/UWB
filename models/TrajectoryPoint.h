// -------------------------------------------------------------------------------------------------------------------
//
//  File: TrajectoryPoint.h
//
//  Trajectory point data model
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef TRAJECTORYPOINT_H
#define TRAJECTORYPOINT_H

#include <QDateTime>
#include <QString>

class TrajectoryPoint
{
public:
    TrajectoryPoint();
    TrajectoryPoint(const QDateTime &time, const QString &tagId,
                    double x, double y, double z = 0.0);

    QDateTime time() const;
    void setTime(const QDateTime &time);

    QString tagId() const;
    void setTagId(const QString &tagId);

    double x() const;
    void setX(double x);

    double y() const;
    void setY(double y);

    double z() const;
    void setZ(double z);

    double roll() const;
    void setRoll(double roll);

    double pitch() const;
    void setPitch(double pitch);

    double yaw() const;
    void setYaw(double yaw);

    double rxPower() const;
    void setRxPower(double rxPower);

    double range(int index) const;
    void setRange(int index, double range);

    double accelX() const;
    void setAccelX(double value);
    double accelY() const;
    void setAccelY(double value);
    double accelZ() const;
    void setAccelZ(double value);

    double gyroX() const;
    void setGyroX(double value);
    double gyroY() const;
    void setGyroY(double value);
    double gyroZ() const;
    void setGyroZ(double value);

    double magX() const;
    void setMagX(double value);
    double magY() const;
    void setMagY(double value);
    double magZ() const;
    void setMagZ(double value);

    bool hasField(const QString &fieldName) const;
    double fieldValue(const QString &fieldName) const;
    void setFieldValue(const QString &fieldName, double value);

private:
    QDateTime _time;
    QString _tagId;
    double _x;
    double _y;
    double _z;
    double _roll;
    double _pitch;
    double _yaw;
    double _rxPower;
    double _ranges[8];
    double _accelX, _accelY, _accelZ;
    double _gyroX, _gyroY, _gyroZ;
    double _magX, _magY, _magZ;
};

#endif // TRAJECTORYPOINT_H
