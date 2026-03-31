// -------------------------------------------------------------------------------------------------------------------
//
//  File: UwbDataVizWidget.h
//
//  UWB Data Visualization Widget for querying and displaying InfluxDB data
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef UWBDATAVIZWIDGET_H
#define UWBDATAVIZWIDGET_H

#include <QWidget>
#include <QList>
#include "InfluxConfig.h"
#include "TrajectoryPoint.h"

namespace Ui {
class UwbDataVizWidget;
}

class InfluxQueryService;
class InfluxWriteService;
class DockerManager;
class UwbTrajectoryView;
class QTableWidgetItem;

class UwbDataVizWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UwbDataVizWidget(QWidget *parent = nullptr);
    ~UwbDataVizWidget();

    void loadSettings();
    void saveSettings();

    // Access to write service for real-time data writing
    InfluxWriteService* writeService() const;

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    // Docker
    void onInitDatabase();
    void onStartDocker();
    void onStopDocker();
    void onDockerStatusChanged(bool running);
    void onDockerMessage(const QString &message);
    void onDockerError(const QString &error);

    // Connection
    void onTestConnection();
    void onConnectionTested(bool success, const QString &message);

    // Query
    void onQueryClicked();
    void onRefreshTagList();
    void onTagListReady(const QStringList &tags);
    void onTrajectoryReady(const QList<TrajectoryPoint> &points);
    void onTelemetryReady(const QList<TrajectoryPoint> &points);
    void onQueryError(const QString &error);
    void onLoadingChanged(bool loading);

    // Trajectory interaction
    void onPointClicked(int index, const TrajectoryPoint &point);
    void onPointHovered(int index, const TrajectoryPoint &point);

    // Table interaction
    void onTableItemClicked(int row, int column);
    void onTableSelectionChanged();

    // Export
    void onExportCsv();
    void onExportImage();

    // UI controls
    void onTagSelectionChanged(int index);
    void onShowAnchorsToggled(bool checked);
    void onShowGridToggled(bool checked);
    void onResetView();

private:
    void setupUI();
    void setupConnections();
    void updateStatus(const QString &message, bool isError = false);
    void populateTable(const QList<TrajectoryPoint> &points);
    void highlightTableRow(int row);
    void loadAnchorConfig();
    void setDefaultTimeRange();

    Ui::UwbDataVizWidget *ui;
    InfluxQueryService *_queryService;
    InfluxWriteService *_writeService;
    DockerManager *_dockerManager;
    UwbTrajectoryView *_trajectoryView;

    InfluxConfig _config;
    QList<TrajectoryPoint> _currentPoints;
    QStringList _availableTags;
    bool _isLoading;
};


#endif // UWBDATAVIZWIDGET_H
