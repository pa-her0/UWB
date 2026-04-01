// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxWriteService.h
//
//  Real-time InfluxDB data writer service
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef INFLUXWRITESERVICE_H
#define INFLUXWRITESERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QTimer>
#include <QDateTime>
#include "InfluxConfig.h"

struct UwbDataPoint
{
    QDateTime timestamp;
    QString tagId;
    double x, y, z;
    double roll, pitch, yaw;
    double rxPower;
    double ranges[8];
    double accelX, accelY, accelZ;
    double gyroX, gyroY, gyroZ;
    double magX, magY, magZ;
    int rangeMask;
    bool hasImu;
    bool hasRange;

    UwbDataPoint()
        : x(0), y(0), z(0), roll(0), pitch(0), yaw(0)
        , rxPower(0), accelX(0), accelY(0), accelZ(0)
        , gyroX(0), gyroY(0), gyroZ(0), magX(0), magY(0), magZ(0)
        , rangeMask(0), hasImu(false), hasRange(false)
    {
        for (int i = 0; i < 8; ++i) ranges[i] = 0;
    }
};

class InfluxWriteService : public QObject
{
    Q_OBJECT
public:
    explicit InfluxWriteService(QObject *parent = nullptr);
    ~InfluxWriteService();

    void setConfig(const InfluxConfig &config);
    InfluxConfig config() const;

    // Enable/disable writing
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Write single data point
    void writeDataPoint(const UwbDataPoint &point);

    // Batch write (automatically triggered by timer)
    void flushBatch();

    // Check connection
    void testConnection();

signals:
    void connectionTested(bool success, const QString &message);
    void writeError(const QString &error);
    void writeSuccess(int pointsWritten);
    void enabledChanged(bool enabled);

private slots:
    void onTestConnectionFinished();
    void onWriteFinished();
    void onBatchTimer();

private:
    QString buildLineProtocol(const UwbDataPoint &point) const;
    void sendBatch();
    QString escapeTagValue(const QString &value) const;
    QString escapeFieldKey(const QString &key) const;

    QNetworkAccessManager *_nam;
    InfluxConfig _config;
    bool _enabled;
    bool _isWriting;

    QQueue<UwbDataPoint> _writeQueue;
    QTimer *_batchTimer;
    int _lastBatchCount;
    static const int BATCH_SIZE = 100;
    static const int BATCH_INTERVAL_MS = 500; // Flush every 500ms
};

#endif // INFLUXWRITESERVICE_H
