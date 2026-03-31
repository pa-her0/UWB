// -------------------------------------------------------------------------------------------------------------------
//
//  File: TrajectoryPoint.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "TrajectoryPoint.h"

TrajectoryPoint::TrajectoryPoint()
    : _x(0.0), _y(0.0), _z(0.0)
    , _roll(0.0), _pitch(0.0), _yaw(0.0)
    , _rxPower(0.0)
    , _accelX(0.0), _accelY(0.0), _accelZ(0.0)
    , _gyroX(0.0), _gyroY(0.0), _gyroZ(0.0)
    , _magX(0.0), _magY(0.0), _magZ(0.0)
{
    for (int i = 0; i < 8; ++i) {
        _ranges[i] = 0.0;
    }
}

TrajectoryPoint::TrajectoryPoint(const QDateTime &time, const QString &tagId,
                                 double x, double y, double z)
    : _time(time), _tagId(tagId)
    , _x(x), _y(y), _z(z)
    , _roll(0.0), _pitch(0.0), _yaw(0.0)
    , _rxPower(0.0)
    , _accelX(0.0), _accelY(0.0), _accelZ(0.0)
    , _gyroX(0.0), _gyroY(0.0), _gyroZ(0.0)
    , _magX(0.0), _magY(0.0), _magZ(0.0)
{
    for (int i = 0; i < 8; ++i) {
        _ranges[i] = 0.0;
    }
}

QDateTime TrajectoryPoint::time() const
{
    return _time;
}

void TrajectoryPoint::setTime(const QDateTime &time)
{
    _time = time;
}

QString TrajectoryPoint::tagId() const
{
    return _tagId;
}

void TrajectoryPoint::setTagId(const QString &tagId)
{
    _tagId = tagId;
}

double TrajectoryPoint::x() const
{
    return _x;
}

void TrajectoryPoint::setX(double x)
{
    _x = x;
}

double TrajectoryPoint::y() const
{
    return _y;
}

void TrajectoryPoint::setY(double y)
{
    _y = y;
}

double TrajectoryPoint::z() const
{
    return _z;
}

void TrajectoryPoint::setZ(double z)
{
    _z = z;
}

double TrajectoryPoint::roll() const
{
    return _roll;
}

void TrajectoryPoint::setRoll(double roll)
{
    _roll = roll;
}

double TrajectoryPoint::pitch() const
{
    return _pitch;
}

void TrajectoryPoint::setPitch(double pitch)
{
    _pitch = pitch;
}

double TrajectoryPoint::yaw() const
{
    return _yaw;
}

void TrajectoryPoint::setYaw(double yaw)
{
    _yaw = yaw;
}

double TrajectoryPoint::rxPower() const
{
    return _rxPower;
}

void TrajectoryPoint::setRxPower(double rxPower)
{
    _rxPower = rxPower;
}

double TrajectoryPoint::range(int index) const
{
    if (index >= 0 && index < 8) {
        return _ranges[index];
    }
    return 0.0;
}

void TrajectoryPoint::setRange(int index, double range)
{
    if (index >= 0 && index < 8) {
        _ranges[index] = range;
    }
}

double TrajectoryPoint::accelX() const { return _accelX; }
void TrajectoryPoint::setAccelX(double value) { _accelX = value; }
double TrajectoryPoint::accelY() const { return _accelY; }
void TrajectoryPoint::setAccelY(double value) { _accelY = value; }
double TrajectoryPoint::accelZ() const { return _accelZ; }
void TrajectoryPoint::setAccelZ(double value) { _accelZ = value; }

double TrajectoryPoint::gyroX() const { return _gyroX; }
void TrajectoryPoint::setGyroX(double value) { _gyroX = value; }
double TrajectoryPoint::gyroY() const { return _gyroY; }
void TrajectoryPoint::setGyroY(double value) { _gyroY = value; }
double TrajectoryPoint::gyroZ() const { return _gyroZ; }
void TrajectoryPoint::setGyroZ(double value) { _gyroZ = value; }

double TrajectoryPoint::magX() const { return _magX; }
void TrajectoryPoint::setMagX(double value) { _magX = value; }
double TrajectoryPoint::magY() const { return _magY; }
void TrajectoryPoint::setMagY(double value) { _magY = value; }
double TrajectoryPoint::magZ() const { return _magZ; }
void TrajectoryPoint::setMagZ(double value) { _magZ = value; }

bool TrajectoryPoint::hasField(const QString &fieldName) const
{
    // Simplified check - in real implementation, track which fields are set
    return true;
}

double TrajectoryPoint::fieldValue(const QString &fieldName) const
{
    if (fieldName == "position_x_m" || fieldName == "x") return _x;
    if (fieldName == "position_y_m" || fieldName == "y") return _y;
    if (fieldName == "position_z_m" || fieldName == "z") return _z;
    if (fieldName == "roll") return _roll;
    if (fieldName == "pitch") return _pitch;
    if (fieldName == "yaw") return _yaw;
    if (fieldName == "rx_pwr_dbm") return _rxPower;
    if (fieldName == "accel_x") return _accelX;
    if (fieldName == "accel_y") return _accelY;
    if (fieldName == "accel_z") return _accelZ;
    if (fieldName == "gyro_x") return _gyroX;
    if (fieldName == "gyro_y") return _gyroY;
    if (fieldName == "gyro_z") return _gyroZ;
    if (fieldName == "mag_x") return _magX;
    if (fieldName == "mag_y") return _magY;
    if (fieldName == "mag_z") return _magZ;
    return 0.0;
}

void TrajectoryPoint::setFieldValue(const QString &fieldName, double value)
{
    if (fieldName == "position_x_m" || fieldName == "x") _x = value;
    else if (fieldName == "position_y_m" || fieldName == "y") _y = value;
    else if (fieldName == "position_z_m" || fieldName == "z") _z = value;
    else if (fieldName == "roll") _roll = value;
    else if (fieldName == "pitch") _pitch = value;
    else if (fieldName == "yaw") _yaw = value;
    else if (fieldName == "rx_pwr_dbm") _rxPower = value;
    else if (fieldName == "accel_x") _accelX = value;
    else if (fieldName == "accel_y") _accelY = value;
    else if (fieldName == "accel_z") _accelZ = value;
    else if (fieldName == "gyro_x") _gyroX = value;
    else if (fieldName == "gyro_y") _gyroY = value;
    else if (fieldName == "gyro_z") _gyroZ = value;
    else if (fieldName == "mag_x") _magX = value;
    else if (fieldName == "mag_y") _magY = value;
    else if (fieldName == "mag_z") _magZ = value;
}
