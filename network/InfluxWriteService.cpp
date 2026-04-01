// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxWriteService.cpp
//
//  Real-time data writer using InfluxDB Line Protocol
//
// -------------------------------------------------------------------------------------------------------------------

#include "InfluxWriteService.h"
#include <QUrl>
#include <QDebug>

InfluxWriteService::InfluxWriteService(QObject *parent)
    : QObject(parent)
    , _nam(new QNetworkAccessManager(this))
    , _enabled(false)
    , _isWriting(false)
    , _batchTimer(new QTimer(this))
    , _lastBatchCount(0)
{
    _batchTimer->setInterval(BATCH_INTERVAL_MS);
    connect(_batchTimer, &QTimer::timeout, this, &InfluxWriteService::onBatchTimer);
    // Timer starts only when enabled
}

InfluxWriteService::~InfluxWriteService()
{
    // Flush remaining data before destroy, bypassing _enabled check
    if (!_writeQueue.isEmpty() && !_isWriting) {
        sendBatch();
    }
}

void InfluxWriteService::setConfig(const InfluxConfig &config)
{
    _config = config;
}

InfluxConfig InfluxWriteService::config() const
{
    return _config;
}

void InfluxWriteService::setEnabled(bool enabled)
{
    if (_enabled != enabled) {
        _enabled = enabled;
        emit enabledChanged(enabled);

        if (enabled) {
            _batchTimer->start();
        } else {
            _batchTimer->stop();
            // Flush remaining data when disabled, bypass _enabled check
            if (!_writeQueue.isEmpty() && !_isWriting) {
                sendBatch();
            }
        }
    }
}

bool InfluxWriteService::isEnabled() const
{
    return _enabled;
}

QString InfluxWriteService::escapeTagValue(const QString &value) const
{
    QString result = value;
    result.replace("\\", "\\\\");
    result.replace(" ", "\\ ");
    result.replace(",", "\\,");
    result.replace("=", "\\=");
    return result;
}

QString InfluxWriteService::escapeFieldKey(const QString &key) const
{
    QString result = key;
    result.replace("\\", "\\\\");
    result.replace(",", "\\,");
    result.replace("=", "\\=");
    return result;
}

QString InfluxWriteService::buildLineProtocol(const UwbDataPoint &point) const
{
    // Line Protocol: measurement,tag_set field_set timestamp
    // measurement: uwb_tag_telemetry
    // tags: tag_id, source
    // fields: position_x_m, position_y_m, position_z_m, roll, pitch, yaw, etc.

    QString line = _config.measurement();

    // Tags
    line += QString(",tag_id=%1").arg(escapeTagValue(point.tagId));
    line += ",source=hr_rtls_pc";

    // Fields (always include position)
    line += QString(" position_x_m=%1,position_y_m=%2,position_z_m=%3")
            .arg(point.x, 0, 'f', 6)
            .arg(point.y, 0, 'f', 6)
            .arg(point.z, 0, 'f', 6);

    // Signal strength
    if (point.rxPower != 0) {
        line += QString(",rx_pwr_dbm=%1").arg(point.rxPower, 0, 'f', 2);
    }

    // Ranges (0-7)
    if (point.hasRange) {
        for (int i = 0; i < 8; ++i) {
            if (point.rangeMask & (1 << i)) {
                line += QString(",range%1_m=%2").arg(i).arg(point.ranges[i], 0, 'f', 6);
            }
        }
    }

    // IMU data
    if (point.hasImu) {
        line += QString(",roll=%1,pitch=%2,yaw=%3")
                .arg(point.roll, 0, 'f', 3)
                .arg(point.pitch, 0, 'f', 3)
                .arg(point.yaw, 0, 'f', 3);

        line += QString(",accel_x=%1,accel_y=%2,accel_z=%3")
                .arg(point.accelX, 0, 'f', 6)
                .arg(point.accelY, 0, 'f', 6)
                .arg(point.accelZ, 0, 'f', 6);

        line += QString(",gyro_x=%1,gyro_y=%2,gyro_z=%3")
                .arg(point.gyroX, 0, 'f', 6)
                .arg(point.gyroY, 0, 'f', 6)
                .arg(point.gyroZ, 0, 'f', 6);

        line += QString(",mag_x=%1,mag_y=%2,mag_z=%3")
                .arg(point.magX, 0, 'f', 6)
                .arg(point.magY, 0, 'f', 6)
                .arg(point.magZ, 0, 'f', 6);
    }

    // Timestamp in nanoseconds
    qint64 timestampNs = point.timestamp.toMSecsSinceEpoch() * 1000000LL;
    line += QString(" %1").arg(timestampNs);

    qDebug() << "Writing point - Tag:" << point.tagId
             << "Time:" << point.timestamp.toString(Qt::ISODate)
             << "X:" << point.x << "Y:" << point.y;

    return line;
}

void InfluxWriteService::writeDataPoint(const UwbDataPoint &point)
{
    if (!_enabled || !_config.isValid()) {
        return;
    }

    _writeQueue.enqueue(point);

    // If queue reaches batch size, flush immediately
    if (_writeQueue.size() >= BATCH_SIZE) {
        flushBatch();
    }
}

void InfluxWriteService::onBatchTimer()
{
    if (!_writeQueue.isEmpty() && !_isWriting) {
        flushBatch();
    }
}

void InfluxWriteService::flushBatch()
{
    if (_writeQueue.isEmpty() || _isWriting || !_enabled) {
        return;
    }

    sendBatch();
}

void InfluxWriteService::sendBatch()
{
    if (_writeQueue.isEmpty()) return;

    _isWriting = true;

    // Build batch payload
    QStringList lines;
    int count = 0;
    while (!_writeQueue.isEmpty() && count < BATCH_SIZE) {
        lines.append(buildLineProtocol(_writeQueue.dequeue()));
        count++;
    }

    QString payload = lines.join("\n");

    // Store count for success signal
    _lastBatchCount = count;

    // Send to InfluxDB
    QString url = QString("%1/api/v2/write?org=%2&bucket=%3&precision=ns")
            .arg(_config.url())
            .arg(_config.org())
            .arg(_config.bucket());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");
    request.setRawHeader("Authorization", QString("Token %1").arg(_config.token()).toUtf8());

    QNetworkReply *reply = _nam->post(request, payload.toUtf8());
    connect(reply, &QNetworkReply::finished, this, &InfluxWriteService::onWriteFinished);
}

void InfluxWriteService::onWriteFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        emit writeError(error);
        qDebug() << "InfluxDB write error:" << error;
    } else {
        emit writeSuccess(_lastBatchCount);
    }

    reply->deleteLater();
    _isWriting = false;

    // Continue sending if there's more data
    if (!_writeQueue.isEmpty()) {
        sendBatch();
    }
}

void InfluxWriteService::testConnection()
{
    QString url = QString("%1/api/v2/buckets").arg(_config.url());
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Token %1").arg(_config.token()).toUtf8());
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = _nam->get(request);
    connect(reply, &QNetworkReply::finished, this, &InfluxWriteService::onTestConnectionFinished);
}

void InfluxWriteService::onTestConnectionFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    bool success = (reply->error() == QNetworkReply::NoError);
    QString message = success ? tr("Connection successful") : reply->errorString();

    reply->deleteLater();

    emit connectionTested(success, message);
}
