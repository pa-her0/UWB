// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxQueryService.h
//
//  InfluxDB HTTP query service using QNetworkAccessManager
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef INFLUXQUERYSERVICE_H
#define INFLUXQUERYSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include "InfluxConfig.h"
#include "TrajectoryPoint.h"

class InfluxQueryService : public QObject
{
    Q_OBJECT
public:
    explicit InfluxQueryService(QObject *parent = nullptr);
    ~InfluxQueryService();

    void setConfig(const InfluxConfig &config);
    InfluxConfig config() const;

    // Connection test
    void testConnection();

    // Query available tag IDs
    void queryTagList(const QDateTime &startTime = QDateTime(),
                      const QDateTime &endTime = QDateTime());

    // Query 2D trajectory (x, y positions)
    void queryTrajectory(const QString &tagId,
                         const QDateTime &startTime,
                         const QDateTime &endTime);

    // Query full telemetry data with specific fields
    void queryTelemetry(const QString &tagId,
                        const QDateTime &startTime,
                        const QDateTime &endTime,
                        const QStringList &fields = QStringList());

    // Query time range for a specific tag
    void queryTagTimeRange(const QString &tagId);

    // Cancel current request
    void cancelRequest();

    // Check if currently loading
    bool isLoading() const;

signals:
    void connectionTested(bool success, const QString &message);
    void tagListReady(const QStringList &tags);
    void tagTimeRangeReady(const QString &tagId, const QDateTime &start, const QDateTime &end);
    void trajectoryReady(const QList<TrajectoryPoint> &points);
    void telemetryReady(const QList<TrajectoryPoint> &points);
    void queryError(const QString &error);
    void loadingChanged(bool loading);

private slots:
    void onTestConnectionFinished();
    void onTagListFinished();
    void onTrajectoryFinished();
    void onTelemetryFinished();
    void onTagTimeRangeFinished();

private:
    QString buildFluxQuery(const QString &tagId,
                           const QDateTime &startTime,
                           const QDateTime &endTime,
                           const QStringList &fields) const;
    QString buildTagListQuery(const QDateTime &startTime,
                              const QDateTime &endTime) const;
    QString buildTimeRangeQuery(const QString &tagId) const;

    QList<TrajectoryPoint> parseTrajectoryResponse(const QByteArray &data);
    QList<TrajectoryPoint> parseTelemetryResponse(const QByteArray &data);
    QStringList parseTagListResponse(const QByteArray &data);

    QString formatTime(const QDateTime &dt) const;

    QNetworkAccessManager *_nam;
    InfluxConfig _config;
    QNetworkReply *_currentReply;
    bool _isLoading;

    enum class QueryType {
        TestConnection,
        TagList,
        Trajectory,
        Telemetry,
        TagTimeRange
    };
    QueryType _currentQueryType;
};

#endif // INFLUXQUERYSERVICE_H
