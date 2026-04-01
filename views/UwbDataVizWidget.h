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
#include <QPixmap>
#include "InfluxConfig.h"
#include "TrajectoryPoint.h"
#include "UwbTrajectoryView.h"

namespace Ui {
class UwbDataVizWidget;
}

class InfluxQueryService;
class InfluxWriteService;
class DockerManager;
class QTableWidgetItem;

class UwbDataVizWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UwbDataVizWidget(QWidget *parent = nullptr);
    ~UwbDataVizWidget();

    void loadSettings();
    void saveSettings();

    InfluxWriteService* writeService() const;

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onInitDatabase();
    void onStartDocker();
    void onStopDocker();
    void onDockerStatusChanged(bool running);
    void onDockerMessage(const QString &message);
    void onDockerError(const QString &error);

    void onTestConnection();
    void onConnectionTested(bool success, const QString &message);

    void onQueryClicked();
    void onRefreshTagList();
    void onTagListReady(const QStringList &tags);
    void onTrajectoryReady(const QList<TrajectoryPoint> &points);
    void onTelemetryReady(const QList<TrajectoryPoint> &points);
    void onQueryError(const QString &error);
    void onLoadingChanged(bool loading);

    void onTableItemClicked(int row, int column);
    void onTableSelectionChanged();

    void onExportCsv();
    void onExportImage();

    void onTagSelectionChanged(int index);

private:
    void setupConnections();
    void updateStatus(const QString &message, bool isError = false);
    void populateTable(const QList<TrajectoryPoint> &points);
    void loadAnchorConfig();
    void setDefaultTimeRange();
    QPixmap renderTrajectoryImage(int width, int height) const;

    Ui::UwbDataVizWidget *ui;
    InfluxQueryService *_queryService;
    InfluxWriteService *_writeService;
    DockerManager *_dockerManager;

    InfluxConfig _config;
    QList<TrajectoryPoint> _currentPoints;
    QList<AnchorInfo> _anchors;
    QStringList _availableTags;
    bool _isLoading;
};


#endif // UWBDATAVIZWIDGET_H
