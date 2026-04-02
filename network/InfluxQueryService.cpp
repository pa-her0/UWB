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
#include <QMap>

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

// InfluxDB returns nanosecond timestamps like 2026-03-25T12:34:56.123456789Z
// Qt can only parse up to milliseconds, so truncate to ms precision
static QDateTime parseInfluxTime(const QString &timeStr)
{
    if (timeStr.isEmpty()) return QDateTime();

    // Try standard ISO first (handles up to ms)
    QDateTime t = QDateTime::fromString(timeStr, Qt::ISODate);
    if (t.isValid()) return t;

    // Truncate sub-second part to 3 digits (ms) and retry
    // Format: 2026-03-25T12:34:56.123456789Z  →  2026-03-25T12:34:56.123Z
    QString s = timeStr;
    int dotPos = s.indexOf('.');
    if (dotPos >= 0) {
        int zPos = s.indexOf('Z', dotPos);
        if (zPos < 0) zPos = s.length();
        // Keep only 3 decimal digits
        QString truncated = s.left(dotPos + 1) + s.mid(dotPos + 1, 3).leftJustified(3, '0') + "Z";
        t = QDateTime::fromString(truncated, "yyyy-MM-ddThh:mm:ss.zzzZ");
        if (t.isValid()) return t;
    }

    // Last resort: strip sub-seconds entirely
    t = QDateTime::fromString(s.left(19), "yyyy-MM-ddThh:mm:ss");
    t.setTimeSpec(Qt::UTC);
    return t;
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

    QString endStr = formatTime(endTime);
    QString rangeParam = QString("start: %1").arg(startStr);
    if (!endStr.isEmpty()) {
        rangeParam += QString(", stop: %1").arg(endStr);
    }

    return QString(
        "import \"influxdata/influxdb/schema\"\n\n"
        "schema.tagValues(\n"
        "  bucket: \"%1\",\n"
        "  tag: \"tag_id\",\n"
        "  predicate: (r) => r._measurement == \"%2\",\n"
        "  %3\n"
        ")"
    ).arg(_config.bucket()).arg(_config.measurement()).arg(rangeParam);
}

void InfluxQueryService::queryTagList(const QDateTime &startTime, const QDateTime &endTime)
{
    cancelRequest();

    QString fluxQuery = buildTagListQuery(startTime, endTime);
    qDebug() << "TagList Query:" << fluxQuery;
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
        QByteArray responseData = _currentReply->readAll();
        QString serverError = QString::fromUtf8(responseData);
        qDebug() << "TagList Error:" << error;
        qDebug() << "Server Response:" << serverError;
        if (!serverError.isEmpty()) {
            error += " - " + serverError.left(200);
        }
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

    // InfluxDB annotated CSV has lines starting with # (datatype, group, default)
    // then a header line, then data lines
    bool headerFound = false;
    int valueIdx = -1;

    for (const QString &line : lines) {
        // Skip annotation lines
        if (line.startsWith('#') || line.trimmed().isEmpty()) continue;

        QStringList cols = line.split(',');

        if (!headerFound) {
            // This is the header row - find _value column index
            for (int i = 0; i < cols.size(); ++i) {
                QString colName = cols[i].trimmed();
                colName.remove('"');
                if (colName == "_value") {
                    valueIdx = i;
                    break;
                }
            }
            headerFound = true;
            continue;
        }

        // Data row
        if (valueIdx >= 0 && valueIdx < cols.size()) {
            QString tagValue = cols[valueIdx].trimmed();
            if (!tagValue.isEmpty()) {
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

    // Build range clause
    QString rangeClause;
    if (stopClause.isEmpty()) {
        rangeClause = QString("start: time(v: \"%1\")").arg(startStr);
    } else {
        rangeClause = QString("start: time(v: \"%1\")%2").arg(startStr).arg(stopClause);
    }

    // Default trajectory query (position_x_m, position_y_m)
    if (fields.isEmpty()) {
        return QString(
            "from(bucket: \"%1\")\n"
            "  |> range(%2)\n"
            "  |> filter(fn: (r) => r._measurement == \"%3\")\n"
            "  |> filter(fn: (r) => r.tag_id == \"%4\")\n"
            "  |> filter(fn: (r) => r._field == \"position_x_m\" or r._field == \"position_y_m\" or r._field == \"position_z_m\")\n"
            "  |> pivot(rowKey: [\"_time\"], columnKey: [\"_field\"], valueColumn: \"_value\")\n"
            "  |> sort(columns: [\"_time\"])"
        ).arg(_config.bucket()).arg(rangeClause).arg(_config.measurement()).arg(tagId);
    }

    return QString(
        "from(bucket: \"%1\")\n"
        "  |> range(%2)\n"
        "  |> filter(fn: (r) => r._measurement == \"%3\")\n"
        "  |> filter(fn: (r) => r.tag_id == \"%4\")%5\n"
        "  |> pivot(rowKey: [\"_time\"], columnKey: [\"_field\"], valueColumn: \"_value\")\n"
        "  |> sort(columns: [\"_time\"])"
    ).arg(_config.bucket()).arg(rangeClause).arg(_config.measurement()).arg(tagId).arg(fieldFilter);
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
        QByteArray responseData = _currentReply->readAll();
        QString serverError = QString::fromUtf8(responseData);
        qDebug() << "Trajectory Error:" << error;
        qDebug() << "Server Response:" << serverError;
        if (!serverError.isEmpty()) {
            error += " - " + serverError.left(200);
        }
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

    if (lines.isEmpty()) return points;

    // Skip InfluxDB annotated CSV comment lines (#datatype, #group, #default)
    QMap<QString, int> colIndex;
    bool headerFound = false;

    for (const QString &line : lines) {
        if (line.startsWith('#') || line.trimmed().isEmpty()) continue;

        if (!headerFound) {
            QStringList headers = line.split(',');
            for (int i = 0; i < headers.size(); ++i) {
                QString colName = headers[i].trimmed();
                colName.remove('"');
                colIndex[colName] = i;
            }
            headerFound = true;
            continue;
        }

        QStringList cols = line.split(',');

        int timeIdx = colIndex.value("_time", -1);
        int tagIdx  = colIndex.value("tag_id", -1);
        int xIdx    = colIndex.value("position_x_m", -1);
        int yIdx    = colIndex.value("position_y_m", -1);
        int zIdx    = colIndex.value("position_z_m", -1);

        if (timeIdx < 0 || tagIdx < 0) continue;
        if (timeIdx >= cols.size() || tagIdx >= cols.size()) continue;

        QString timeStr = cols[timeIdx].trimmed();
        QString tagId   = cols[tagIdx].trimmed();
        tagId.remove('"');

        QDateTime time = parseInfluxTime(timeStr);
        if (!time.isValid()) continue;

        double x = 0.0, y = 0.0, z = 0.0;
        bool xOk = false, yOk = false;

        if (xIdx >= 0 && xIdx < cols.size()) {
            QString xStr = cols[xIdx].trimmed();
            if (!xStr.isEmpty()) x = xStr.toDouble(&xOk);
        }
        if (yIdx >= 0 && yIdx < cols.size()) {
            QString yStr = cols[yIdx].trimmed();
            if (!yStr.isEmpty()) y = yStr.toDouble(&yOk);
        }
        if (zIdx >= 0 && zIdx < cols.size()) z = cols[zIdx].trimmed().toDouble();

        if (xOk && yOk) {
            points.append(TrajectoryPoint(time, tagId, x, y, z));
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
    qDebug() << "Telemetry Query:" << fluxQuery;
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
        QByteArray responseData = _currentReply->readAll();
        QString serverError = QString::fromUtf8(responseData);
        qDebug() << "Telemetry Error:" << error;
        qDebug() << "Server Response:" << serverError;
        if (!serverError.isEmpty()) {
            error += " - " + serverError.left(200);
        }
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
    QList<TrajectoryPoint> points;
    QString content = QString::fromUtf8(data);
    QStringList lines = content.split('\n', QString::SkipEmptyParts);

    if (lines.isEmpty()) return points;

    // Skip InfluxDB annotated CSV comment lines (#datatype, #group, #default)
    QMap<QString, int> colIndex;
    bool headerFound = false;

    for (const QString &line : lines) {
        if (line.startsWith('#') || line.trimmed().isEmpty()) continue;

        if (!headerFound) {
            QStringList headers = line.split(',');
            for (int i = 0; i < headers.size(); ++i) {
                QString colName = headers[i].trimmed();
                colName.remove('"');
                colIndex[colName] = i;
            }
            headerFound = true;
            continue;
        }

        QStringList cols = line.split(',');

        int timeIdx    = colIndex.value("_time", -1);
        int tagIdx     = colIndex.value("tag_id", -1);
        int xIdx       = colIndex.value("position_x_m", -1);
        int yIdx       = colIndex.value("position_y_m", -1);
        int zIdx       = colIndex.value("position_z_m", -1);
        int rollIdx    = colIndex.value("roll", -1);
        int pitchIdx   = colIndex.value("pitch", -1);
        int yawIdx     = colIndex.value("yaw", -1);
        int rxPowerIdx = colIndex.value("rx_pwr_dbm", -1);

        if (timeIdx < 0 || tagIdx < 0) continue;
        if (timeIdx >= cols.size() || tagIdx >= cols.size()) continue;

        QString timeStr = cols[timeIdx].trimmed();
        QString tagId   = cols[tagIdx].trimmed();
        tagId.remove('"');

        QDateTime time = parseInfluxTime(timeStr);
        if (!time.isValid()) continue;

        TrajectoryPoint point;
        point.setTime(time);
        point.setTagId(tagId);

        if (xIdx < 0 || yIdx < 0 || xIdx >= cols.size() || yIdx >= cols.size()) continue;

        bool xOk = false, yOk = false;
        QString xStr = cols[xIdx].trimmed();
        QString yStr = cols[yIdx].trimmed();
        // Skip rows where position fields are empty (pivot gap — no value for this timestamp)
        if (xStr.isEmpty() || yStr.isEmpty()) continue;
        double x = xStr.toDouble(&xOk);
        double y = yStr.toDouble(&yOk);
        if (!xOk || !yOk) continue;

        auto safeDouble = [&](int idx) -> double {
            if (idx >= 0 && idx < cols.size()) return cols[idx].trimmed().toDouble();
            return 0.0;
        };

        point.setX(x);
        point.setY(y);
        point.setZ(safeDouble(zIdx));
        point.setRoll(safeDouble(rollIdx));
        point.setPitch(safeDouble(pitchIdx));
        point.setYaw(safeDouble(yawIdx));
        point.setRxPower(safeDouble(rxPowerIdx));

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
