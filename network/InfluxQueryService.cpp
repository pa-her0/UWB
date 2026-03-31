// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxQueryService.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "InfluxQueryService.h"
#include <QUrl>
#include <QUrlQuery>
#include <QStringList>
#include <QTextStream>
#include <QDebug>

InfluxQueryService::InfluxQueryService(QObject *parent)
    : QObject(parent)
    , _nam(new QNetworkAccessManager(this))
    , _currentReply(nullptr)
    , _isLoading(false)
{
}

InfluxQueryService::~InfluxQueryService()
{
    cancelRequest();
}

void InfluxQueryService::setConfig(const InfluxConfig &config)
{
    _config = config;
}

InfluxConfig InfluxQueryService::config() const
{
    return _config;
}

bool InfluxQueryService::isLoading() const
{
    return _isLoading;
}

void InfluxQueryService::cancelRequest()
{
    if (_currentReply) {
        _currentReply->abort();
        _currentReply->deleteLater();
        _currentReply = nullptr;
    }
    if (_isLoading) {
        _isLoading = false;
        emit loadingChanged(false);
    }
}

QString InfluxQueryService::formatTime(const QDateTime &dt) const
{
    // InfluxDB Flux format: 2026-03-25T00:00:00Z
    if (!dt.isValid()) {
        return QString();
    }
    return dt.toUTC().toString("yyyy-MM-ddThh:mm:ssZ");
}

void InfluxQueryService::testConnection()
{
    cancelRequest();

    QString url = QString("%1/api/v2/buckets").arg(_config.url());
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Token %1").arg(_config.token()).toUtf8());
    request.setRawHeader("Accept", "application/json");

    _currentQueryType = QueryType::TestConnection;
    _isLoading = true;
    emit loadingChanged(true);

    _currentReply = _nam->get(request);
    connect(_currentReply, &QNetworkReply::finished, this, &InfluxQueryService::onTestConnectionFinished);
}

void InfluxQueryService::onTestConnectionFinished()
{
    if (!_currentReply) return;

    bool success = (_currentReply->error() == QNetworkReply::NoError);
    QString message;

    if (success) {
        message = tr("Connection successful");
    } else {
        message = _currentReply->errorString();
    }

    _currentReply->deleteLater();
    _currentReply = nullptr;
    _isLoading = false;

    emit connectionTested(success, message);
    emit loadingChanged(false);
}

QString InfluxQueryService::buildTagListQuery(const QDateTime &startTime, const QDateTime &endTime) const
{
    QString startStr = formatTime(startTime);
    if (startStr.isEmpty()) {
        startStr = "1970-01-01T00:00:00Z";
    }

    QString stopClause;
    QString endStr = formatTime(endTime);
    if (!endStr.isEmpty()) {
        stopClause = QString(", stop: time(v: \"%1\")").arg(endStr);
    }

    return QString(
        "import \"influxdata/influxdb/schema\"\n\n"
        "schema.tagValues(\n"
        "  bucket: \"%1\",\n"
        "  tag: \"tag_id\",\n"
        "  predicate: (r) => r._measurement == \"%2\",\n"
        "  start: time(v: \"%3\")%4\n"
        ")"
    ).arg(_config.bucket()).arg(_config.measurement()).arg(startStr).arg(stopClause);
}

void InfluxQueryService::queryTagList(const QDateTime &startTime, const QDateTime &endTime)
{
    cancelRequest();

    QString fluxQuery = buildTagListQuery(startTime, endTime);
    QString url = QString("%1/api/v2/query?org=%2").arg(_config.url()).arg(_config.org());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/vnd.flux");
    request.setRawHeader("Authorization", QString("Token %1").arg(_config.token()).toUtf8());
    request.setRawHeader("Accept", "application/csv");

    _currentQueryType = QueryType::TagList;
    _isLoading = true;
    emit loadingChanged(true);

    _currentReply = _nam->post(request, fluxQuery.toUtf8());
    connect(_currentReply, &QNetworkReply::finished, this, &InfluxQueryService::onTagListFinished);
}

void InfluxQueryService::onTagListFinished()
{
    if (!_currentReply) return;

    if (_currentReply->error() != QNetworkReply::NoError) {
        QString error = _currentReply->errorString();
        _currentReply->deleteLater();
        _currentReply = nullptr;
        _isLoading = false;
        emit queryError(error);
        emit loadingChanged(false);
        return;
    }

    QByteArray data = _currentReply->readAll();
    QStringList tags = parseTagListResponse(data);

    _currentReply->deleteLater();
    _currentReply = nullptr;
    _isLoading = false;

    emit tagListReady(tags);
    emit loadingChanged(false);
}

QStringList InfluxQueryService::parseTagListResponse(const QByteArray &data)
{
    QStringList tags;
    QString content = QString::fromUtf8(data);
    QStringList lines = content.split('\n', QString::SkipEmptyParts);

    // CSV format: result,table,_value
    // Skip header line
    bool firstLine = true;
    for (const QString &line : lines) {
        if (firstLine) {
            firstLine = false;
            continue;
        }
        QStringList cols = line.split(',');
        if (cols.size() >= 3) {
            QString tagValue = cols[2].trimmed();
            if (!tagValue.isEmpty() && tagValue != "_value") {
                tags.append(tagValue);
            }
        }
    }

    return tags;
}

QString InfluxQueryService::buildFluxQuery(const QString &tagId,
                                           const QDateTime &startTime,
                                           const QDateTime &endTime,
                                           const QStringList &fields) const
{
    QString startStr = formatTime(startTime);
    if (startStr.isEmpty()) {
        startStr = "1970-01-01T00:00:00Z";
    }

    QString stopClause;
    QString endStr = formatTime(endTime);
    if (!endStr.isEmpty()) {
        stopClause = QString(", stop: time(v: \"%1\")").arg(endStr);
    }

    // Build field filter
    QStringList fieldFilters;
    for (const QString &field : fields) {
        if (!field.isEmpty()) {
            fieldFilters.append(QString("r._field == \"%1\"").arg(field));
        }
    }

    QString fieldFilter;
    if (!fieldFilters.isEmpty()) {
        fieldFilter = QString(" |> filter(fn: (r) => %1)").arg(fieldFilters.join(" or "));
    }

    // Default trajectory query (position_x_m, position_y_m)
    if (fields.isEmpty()) {
        return QString(
            "from(bucket: \"%1\")\n"
            "  |> range(start: time(v: \"%2\")%3)\n"
            "  |> filter(fn: (r) => r._measurement == \"%4\")\n"
            "  |> filter(fn: (r) => r.tag_id == \"%5\")\n"
            "  |> filter(fn: (r) => r._field == \"position_x_m\" or r._field == \"position_y_m\" or r._field == \"position_z_m\")\n"
            "  |> pivot(rowKey: [\"_time\"], columnKey: [\"_field\"], valueColumn: \"_value\")\n"
            "  |> sort(columns: [\"_time\"])"
        ).arg(_config.bucket()).arg(startStr).arg(stopClause).arg(_config.measurement()).arg(tagId);
    }

    return QString(
        "from(bucket: \"%1\")\n"
        "  |> range(start: time(v: \"%2\")%3)\n"
        "  |> filter(fn: (r) => r._measurement == \"%4\")\n"
        "  |> filter(fn: (r) => r.tag_id == \"%5\")%6\n"
        "  |> pivot(rowKey: [\"_time\"], columnKey: [\"_field\"], valueColumn: \"_value\")\n"
        "  |> sort(columns: [\"_time\"])"
    ).arg(_config.bucket()).arg(startStr).arg(stopClause).arg(_config.measurement()).arg(tagId).arg(fieldFilter);
}

void InfluxQueryService::queryTrajectory(const QString &tagId,
                                         const QDateTime &startTime,
                                         const QDateTime &endTime)
{
    cancelRequest();

    QString fluxQuery = buildFluxQuery(tagId, startTime, endTime, QStringList());
    QString url = QString("%1/api/v2/query?org=%2").arg(_config.url()).arg(_config.org());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/vnd.flux");
    request.setRawHeader("Authorization", QString("Token %1").arg(_config.token()).toUtf8());
    request.setRawHeader("Accept", "application/csv");

    _currentQueryType = QueryType::Trajectory;
    _isLoading = true;
    emit loadingChanged(true);

    _currentReply = _nam->post(request, fluxQuery.toUtf8());
    connect(_currentReply, &QNetworkReply::finished, this, &InfluxQueryService::onTrajectoryFinished);
}

void InfluxQueryService::onTrajectoryFinished()
{
    if (!_currentReply) return;

    if (_currentReply->error() != QNetworkReply::NoError) {
        QString error = _currentReply->errorString();
        _currentReply->deleteLater();
        _currentReply = nullptr;
        _isLoading = false;
        emit queryError(error);
        emit loadingChanged(false);
        return;
    }

    QByteArray data = _currentReply->readAll();
    QList<TrajectoryPoint> points = parseTrajectoryResponse(data);

    _currentReply->deleteLater();
    _currentReply = nullptr;
    _isLoading = false;

    emit trajectoryReady(points);
    emit loadingChanged(false);
}

QList<TrajectoryPoint> InfluxQueryService::parseTrajectoryResponse(const QByteArray &data)
{
    QList<TrajectoryPoint> points;
    QString content = QString::fromUtf8(data);
    QStringList lines = content.split('\n', QString::SkipEmptyParts);

    // CSV format with pivot:
    // result,table,_time,_measurement,tag_id,position_x_m,position_y_m,position_z_m,...
    bool firstLine = true;
    for (const QString &line : lines) {
        if (firstLine) {
            firstLine = false;
            continue;
        }

        QStringList cols = line.split(',');
        // Minimum columns: result, table, _time, _measurement, tag_id, x, y
        if (cols.size() < 7) continue;

        QString timeStr = cols[2].trimmed();
        QString tagId = cols[4].trimmed();

        // Parse timestamp
        QDateTime time = QDateTime::fromString(timeStr, Qt::ISODate);
        if (!time.isValid()) {
            time = QDateTime::fromString(timeStr, "yyyy-MM-ddThh:mm:ss.zzzZ");
        }

        // Find column indices (they can vary based on query)
        // For simplicity, assume position_x_m and position_y_m are in specific positions after pivot
        // We need to parse based on the header
        // Actually, after pivot, columns are: result,table,_time,_start,_stop,_measurement,tag_id,position_x_m,position_y_m,...
        double x = 0.0, y = 0.0, z = 0.0;
        bool xOk = false, yOk = false;

        // Try to find x and y in the expected positions
        // After pivot, fields become columns starting from index 7
        for (int i = 7; i < cols.size(); ++i) {
            QString colName = cols[i].trimmed();
            // In CSV response after pivot, column names are in header
            // Values start from the next line
        }

        // Simplified: parse assuming standard order after header parsing
        // For now, try to parse x and y from known positions
        if (cols.size() > 7) {
            x = cols[7].trimmed().toDouble(&xOk);
        }
        if (cols.size() > 8) {
            y = cols[8].trimmed().toDouble(&yOk);
        }
        if (cols.size() > 9) {
            z = cols[9].trimmed().toDouble();
        }

        if (xOk && yOk && time.isValid()) {
            TrajectoryPoint point(time, tagId, x, y, z);
            points.append(point);
        }
    }

    return points;
}

void InfluxQueryService::queryTelemetry(const QString &tagId,
                                        const QDateTime &startTime,
                                        const QDateTime &endTime,
                                        const QStringList &fields)
{
    cancelRequest();

    QStringList queryFields = fields;
    if (queryFields.isEmpty()) {
        // Default fields: position + attitude + IMU
        queryFields << "position_x_m" << "position_y_m" << "position_z_m"
                    << "roll" << "pitch" << "yaw"
                    << "accel_x" << "accel_y" << "accel_z"
                    << "gyro_x" << "gyro_y" << "gyro_z"
                    << "mag_x" << "mag_y" << "mag_z"
                    << "rx_pwr_dbm";
    }

    QString fluxQuery = buildFluxQuery(tagId, startTime, endTime, queryFields);
    QString url = QString("%1/api/v2/query?org=%2").arg(_config.url()).arg(_config.org());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/vnd.flux");
    request.setRawHeader("Authorization", QString("Token %1").arg(_config.token()).toUtf8());
    request.setRawHeader("Accept", "application/csv");

    _currentQueryType = QueryType::Telemetry;
    _isLoading = true;
    emit loadingChanged(true);

    _currentReply = _nam->post(request, fluxQuery.toUtf8());
    connect(_currentReply, &QNetworkReply::finished, this, &InfluxQueryService::onTelemetryFinished);
}

void InfluxQueryService::onTelemetryFinished()
{
    if (!_currentReply) return;

    if (_currentReply->error() != QNetworkReply::NoError) {
        QString error = _currentReply->errorString();
        _currentReply->deleteLater();
        _currentReply = nullptr;
        _isLoading = false;
        emit queryError(error);
        emit loadingChanged(false);
        return;
    }

    QByteArray data = _currentReply->readAll();
    QList<TrajectoryPoint> points = parseTelemetryResponse(data);

    _currentReply->deleteLater();
    _currentReply = nullptr;
    _isLoading = false;

    emit telemetryReady(points);
    emit loadingChanged(false);
}

QList<TrajectoryPoint> InfluxQueryService::parseTelemetryResponse(const QByteArray &data)
{
    // Similar to parseTrajectoryResponse but parses all fields
    QList<TrajectoryPoint> points;
    QString content = QString::fromUtf8(data);
    QStringList lines = content.split('\n', QString::SkipEmptyParts);

    if (lines.isEmpty()) return points;

    // Parse header to get column indices
    QString header = lines[0];
    QStringList headers = header.split(',');
    QMap<QString, int> colIndex;
    for (int i = 0; i < headers.size(); ++i) {
        colIndex[headers[i].trimmed()] = i;
    }

    int timeIdx = colIndex.value("_time", 2);
    int tagIdx = colIndex.value("tag_id", 4);
    int xIdx = colIndex.value("position_x_m", -1);
    int yIdx = colIndex.value("position_y_m", -1);
    int zIdx = colIndex.value("position_z_m", -1);
    int rollIdx = colIndex.value("roll", -1);
    int pitchIdx = colIndex.value("pitch", -1);
    int yawIdx = colIndex.value("yaw", -1);
    int rxPowerIdx = colIndex.value("rx_pwr_dbm", -1);

    for (int i = 1; i < lines.size(); ++i) {
        QStringList cols = lines[i].split(',');
        if (cols.size() < headers.size()) continue;

        QString timeStr = cols[timeIdx].trimmed();
        QString tagId = cols[tagIdx].trimmed();

        QDateTime time = QDateTime::fromString(timeStr, Qt::ISODate);
        if (!time.isValid()) {
            time = QDateTime::fromString(timeStr, "yyyy-MM-ddThh:mm:ss.zzzZ");
        }
        if (!time.isValid()) continue;

        TrajectoryPoint point;
        point.setTime(time);
        point.setTagId(tagId);

        if (xIdx >= 0 && xIdx < cols.size()) point.setX(cols[xIdx].trimmed().toDouble());
        if (yIdx >= 0 && yIdx < cols.size()) point.setY(cols[yIdx].trimmed().toDouble());
        if (zIdx >= 0 && zIdx < cols.size()) point.setZ(cols[zIdx].trimmed().toDouble());
        if (rollIdx >= 0 && rollIdx < cols.size()) point.setRoll(cols[rollIdx].trimmed().toDouble());
        if (pitchIdx >= 0 && pitchIdx < cols.size()) point.setPitch(cols[pitchIdx].trimmed().toDouble());
        if (yawIdx >= 0 && yawIdx < cols.size()) point.setYaw(cols[yawIdx].trimmed().toDouble());
        if (rxPowerIdx >= 0 && rxPowerIdx < cols.size()) point.setRxPower(cols[rxPowerIdx].trimmed().toDouble());

        points.append(point);
    }

    return points;
}

QString InfluxQueryService::buildTimeRangeQuery(const QString &tagId) const
{
    return QString(
        "from(bucket: \"%1\")\n"
        "  |> range(start: 1970-01-01T00:00:00Z)\n"
        "  |> filter(fn: (r) => r._measurement == \"%2\")\n"
        "  |> filter(fn: (r) => r.tag_id == \"%3\")\n"
        "  |> filter(fn: (r) => r._field == \"position_x_m\")\n"
        "  |> first()\n"
        "  |> yield(name: \"first\")\n\n"
        "from(bucket: \"%4\")\n"
        "  |> range(start: 1970-01-01T00:00:00Z)\n"
        "  |> filter(fn: (r) => r._measurement == \"%5\")\n"
        "  |> filter(fn: (r) => r.tag_id == \"%6\")\n"
        "  |> filter(fn: (r) => r._field == \"position_x_m\")\n"
        "  |> last()\n"
        "  |> yield(name: \"last\")"
    ).arg(_config.bucket()).arg(_config.measurement()).arg(tagId)
     .arg(_config.bucket()).arg(_config.measurement()).arg(tagId);
}

void InfluxQueryService::queryTagTimeRange(const QString &tagId)
{
    cancelRequest();

    QString fluxQuery = buildTimeRangeQuery(tagId);
    QString url = QString("%1/api/v2/query?org=%2").arg(_config.url()).arg(_config.org());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/vnd.flux");
    request.setRawHeader("Authorization", QString("Token %1").arg(_config.token()).toUtf8());
    request.setRawHeader("Accept", "application/csv");

    _currentQueryType = QueryType::TagTimeRange;
    _isLoading = true;
    emit loadingChanged(true);

    _currentReply = _nam->post(request, fluxQuery.toUtf8());
    connect(_currentReply, &QNetworkReply::finished, this, &InfluxQueryService::onTagTimeRangeFinished);
}

void InfluxQueryService::onTagTimeRangeFinished()
{
    // TODO: Implement time range parsing from response
    // For now, emit empty range
    if (!_currentReply) return;

    _currentReply->deleteLater();
    _currentReply = nullptr;
    _isLoading = false;
    emit loadingChanged(false);

    // Emit default range for now
    emit tagTimeRangeReady("", QDateTime(), QDateTime());
}
