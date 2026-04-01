// -------------------------------------------------------------------------------------------------------------------
//
//  File: UwbDataVizWidget.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "UwbDataVizWidget.h"
#include "ui_UwbDataVizWidget.h"
#include "InfluxQueryService.h"
#include "InfluxWriteService.h"
#include "DockerManager.h"
#include "UwbTrajectoryView.h"
#include "RTLSDisplayApplication.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QPainter>
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QTimer>

UwbDataVizWidget::UwbDataVizWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UwbDataVizWidget)
    , _queryService(new InfluxQueryService(this))
    , _writeService(new InfluxWriteService(this))
    , _dockerManager(new DockerManager(this))
    , _trajectoryView(nullptr)
    , _isLoading(false)
{
    ui->setupUi(this);

    // Replace the placeholder with our actual trajectory view
    _trajectoryView = new UwbTrajectoryView(this);
    ui->vizLayout->replaceWidget(ui->trajectoryView, _trajectoryView);
    delete ui->trajectoryView;
    ui->trajectoryView = _trajectoryView;

    // Setup Docker working directory (where docker-compose.yml will be created)
    QString dockerDir = QDir::currentPath() + "/influx";
    QDir().mkpath(dockerDir);
    _dockerManager->setWorkingDirectory(dockerDir);

    setupConnections();
    loadSettings();
    setDefaultTimeRange();
    loadAnchorConfig();

    // Check Docker availability
    if (!_dockerManager->isDockerAvailable()) {
        ui->dockerStatusLabel->setText(tr("Docker not installed"));
        ui->dockerStatusLabel->setStyleSheet("color: orange;");

        // Disable Docker controls but keep the panel visible for information
        ui->initDbBtn->setEnabled(false);
        ui->initDbBtn->setToolTip(tr("Docker is not installed. Please install Docker or use external InfluxDB."));
        ui->startDockerBtn->setEnabled(false);
        ui->stopDockerBtn->setEnabled(false);

        // Show a one-time notice about Docker alternatives
        QTimer::singleShot(1000, [this]() {
            QMessageBox::information(this, tr("Docker Not Found"),
                tr("Docker is not detected on this system.\n\n"
                   "You have the following options:\n\n"
                   "1. Install Docker Desktop (recommended for beginners):\n"
                   "   https://www.docker.com/products/docker-desktop\n\n"
                   "2. Use an external InfluxDB server:\n"
                   "   - Enter the URL of an existing InfluxDB instance\n"
                   "   - Or use InfluxDB Cloud (free tier available)\n\n"
                   "3. Install InfluxDB directly without Docker:\n"
                   "   https://docs.influxdata.com/influxdb/latest/install/\n\n"
                   "The software will continue to work with CSV files only."));
        });
    } else {
        // Docker is available, check if InfluxDB container is running
        ui->dockerStatusLabel->setText(tr("Status: Docker available"));
        ui->dockerStatusLabel->setStyleSheet("color: green;");
    }
}

UwbDataVizWidget::~UwbDataVizWidget()
{
    saveSettings();
    delete ui;
}

void UwbDataVizWidget::setupConnections()
{
    // Docker controls
    connect(ui->initDbBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onInitDatabase);
    connect(ui->startDockerBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onStartDocker);
    connect(ui->stopDockerBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onStopDocker);
    connect(_dockerManager, &DockerManager::statusChanged, this, &UwbDataVizWidget::onDockerStatusChanged);
    connect(_dockerManager, &DockerManager::statusMessage, this, &UwbDataVizWidget::onDockerMessage);
    connect(_dockerManager, &DockerManager::error, this, &UwbDataVizWidget::onDockerError);

    // Connection testing
    connect(ui->testConnectionBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onTestConnection);
    connect(_queryService, &InfluxQueryService::connectionTested, this, &UwbDataVizWidget::onConnectionTested);
    connect(_writeService, &InfluxWriteService::connectionTested, this, &UwbDataVizWidget::onConnectionTested);

    // Query
    connect(ui->queryBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onQueryClicked);
    connect(ui->refreshTagsBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onRefreshTagList);
    connect(_queryService, &InfluxQueryService::tagListReady, this, &UwbDataVizWidget::onTagListReady);
    connect(_queryService, &InfluxQueryService::trajectoryReady, this, &UwbDataVizWidget::onTrajectoryReady);
    connect(_queryService, &InfluxQueryService::telemetryReady, this, &UwbDataVizWidget::onTelemetryReady);
    connect(_queryService, &InfluxQueryService::queryError, this, &UwbDataVizWidget::onQueryError);
    connect(_queryService, &InfluxQueryService::loadingChanged, this, &UwbDataVizWidget::onLoadingChanged);

    // Write service status
    connect(_writeService, &InfluxWriteService::writeSuccess, this, [this](int count) {
        qDebug() << "InfluxDB write success:" << count << "points";
    });
    connect(_writeService, &InfluxWriteService::writeError, this, [this](const QString &error) {
        updateStatus(tr("Write error: %1").arg(error), true);
        qDebug() << "InfluxDB write error:" << error;
    });

    // Trajectory view interaction
    connect(_trajectoryView, &UwbTrajectoryView::pointClicked, this, &UwbDataVizWidget::onPointClicked);
    connect(_trajectoryView, &UwbTrajectoryView::pointHovered, this, &UwbDataVizWidget::onPointHovered);

    // Table interaction
    connect(ui->dataTable, &QTableWidget::cellClicked, this, &UwbDataVizWidget::onTableItemClicked);
    connect(ui->dataTable, &QTableWidget::itemSelectionChanged, this, &UwbDataVizWidget::onTableSelectionChanged);

    // Export
    connect(ui->exportCsvBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onExportCsv);
    connect(ui->exportImageBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onExportImage);

    // Real-time write
    connect(ui->enableRealtimeWrite, &QCheckBox::toggled, [this](bool checked) {
        _writeService->setEnabled(checked);
        if (checked) {
            // Update config from UI
            _config.setUrl(ui->urlEdit->text());
            _config.setToken(ui->tokenEdit->text());
            _config.setOrg(ui->orgEdit->text());
            _config.setBucket(ui->bucketEdit->text());
            _config.setMeasurement(ui->measurementEdit->text());
            _writeService->setConfig(_config);
            updateStatus(tr("Real-time write enabled"));
        } else {
            updateStatus(tr("Real-time write disabled"));
        }
    });

    // Display options
    connect(ui->showAnchorsCheck, &QCheckBox::toggled, this, &UwbDataVizWidget::onShowAnchorsToggled);
    connect(ui->showGridCheck, &QCheckBox::toggled, this, &UwbDataVizWidget::onShowGridToggled);
    connect(ui->resetViewBtn, &QPushButton::clicked, this, &UwbDataVizWidget::onResetView);
    connect(ui->tagCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UwbDataVizWidget::onTagSelectionChanged);
}

void UwbDataVizWidget::setDefaultTimeRange()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime yesterday = now.addDays(-1);

    ui->endTimeEdit->setDateTime(now);
    ui->startTimeEdit->setDateTime(yesterday);
}

void UwbDataVizWidget::loadAnchorConfig()
{
    // Try to load anchors from influx/anchors.example.json or similar
    QStringList paths;
    paths << "./influx/anchors.json"
          << "./influx/anchors.example.json"
          << "./anchors.json"
          << "../influx/anchors.json";

    for (const QString &path : paths) {
        QFile file(path);
        if (!file.exists()) continue;

        if (!file.open(QIODevice::ReadOnly)) continue;

        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        QJsonArray anchors = obj.value("anchors").toArray();

        QList<AnchorInfo> anchorList;
        for (const QJsonValue &val : anchors) {
            QJsonObject anchor = val.toObject();
            AnchorInfo info;
            info.name = anchor.value("name").toString();
            info.x = anchor.value("x").toDouble();
            info.y = anchor.value("y").toDouble();
            info.color = QColor(217, 72, 15);
            anchorList.append(info);
        }

        if (!anchorList.isEmpty()) {
            _trajectoryView->setAnchors(anchorList);
            break;
        }
    }
}

void UwbDataVizWidget::onTestConnection()
{
    _config.setUrl(ui->urlEdit->text());
    _config.setToken(ui->tokenEdit->text());
    _config.setOrg(ui->orgEdit->text());
    _config.setBucket(ui->bucketEdit->text());
    _config.setMeasurement(ui->measurementEdit->text());

    _queryService->setConfig(_config);
    _writeService->setConfig(_config);
    _queryService->testConnection();

    updateStatus(tr("Testing connection..."));
}

void UwbDataVizWidget::onConnectionTested(bool success, const QString &message)
{
    if (success) {
        ui->statusLabel->setText(tr("Connected"));
        ui->statusLabel->setStyleSheet("color: green;");
        updateStatus(tr("Connection successful - Enable real-time write to store data"));

        // Auto refresh tag list on successful connection
        onRefreshTagList();

        // Prompt user to enable real-time write if not already enabled
        if (!ui->enableRealtimeWrite->isChecked()) {
            QMessageBox::information(this, tr("Real-time Write"),
                tr("Connection successful!\n\n"
                   "To store incoming UWB data to InfluxDB, "
                   "please check \"Enable real-time write to InfluxDB\" "
                   "in the Real-time Write section."));
        }
    } else {
        ui->statusLabel->setText(tr("Failed"));
        ui->statusLabel->setStyleSheet("color: red;");
        updateStatus(tr("Connection failed: %1").arg(message), true);
    }
}

void UwbDataVizWidget::onRefreshTagList()
{
    _config.setUrl(ui->urlEdit->text());
    _config.setToken(ui->tokenEdit->text());
    _config.setOrg(ui->orgEdit->text());
    _config.setBucket(ui->bucketEdit->text());
    _config.setMeasurement(ui->measurementEdit->text());

    _queryService->setConfig(_config);
    _writeService->setConfig(_config);
    _queryService->queryTagList();
}

void UwbDataVizWidget::onTagListReady(const QStringList &tags)
{
    _availableTags = tags;
    ui->tagCombo->clear();
    ui->tagCombo->addItems(tags);

    if (!tags.isEmpty()) {
        updateStatus(tr("Found %1 tag(s)").arg(tags.size()));
    } else {
        updateStatus(tr("No tags found"), true);
    }
}

void UwbDataVizWidget::onQueryClicked()
{
    QString tagId = ui->tagCombo->currentText();
    if (tagId.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a tag"));
        return;
    }

    QDateTime startTime = ui->startTimeEdit->dateTime();
    QDateTime endTime = ui->endTimeEdit->dateTime();

    if (startTime >= endTime) {
        QMessageBox::warning(this, tr("Warning"), tr("Start time must be before end time"));
        return;
    }

    _config.setUrl(ui->urlEdit->text());
    _config.setToken(ui->tokenEdit->text());
    _config.setOrg(ui->orgEdit->text());
    _config.setBucket(ui->bucketEdit->text());
    _config.setMeasurement(ui->measurementEdit->text());

    _queryService->setConfig(_config);
    _queryService->queryTelemetry(tagId, startTime, endTime);

    updateStatus(tr("Querying data..."));
}

void UwbDataVizWidget::onTrajectoryReady(const QList<TrajectoryPoint> &points)
{
    _currentPoints = points;
    _trajectoryView->setTrajectory(points);
    populateTable(points);

    updateStatus(tr("Loaded %1 points").arg(points.size()));
}

void UwbDataVizWidget::onTelemetryReady(const QList<TrajectoryPoint> &points)
{
    // Same handling as trajectory for now
    _currentPoints = points;
    _trajectoryView->setTrajectory(points);
    populateTable(points);

    updateStatus(tr("Loaded %1 points").arg(points.size()));
}

void UwbDataVizWidget::onQueryError(const QString &error)
{
    updateStatus(tr("Error: %1").arg(error), true);
    QMessageBox::critical(this, tr("Query Error"), error);
}

void UwbDataVizWidget::onLoadingChanged(bool loading)
{
    _isLoading = loading;
    ui->queryBtn->setEnabled(!loading);
    ui->refreshTagsBtn->setEnabled(!loading);
    ui->testConnectionBtn->setEnabled(!loading);
}

void UwbDataVizWidget::populateTable(const QList<TrajectoryPoint> &points)
{
    ui->dataTable->setRowCount(points.size());

    for (int i = 0; i < points.size(); ++i) {
        const TrajectoryPoint &p = points[i];

        ui->dataTable->setItem(i, 0, new QTableWidgetItem(p.time().toString("yyyy-MM-dd hh:mm:ss.zzz")));
        ui->dataTable->setItem(i, 1, new QTableWidgetItem(p.tagId()));
        ui->dataTable->setItem(i, 2, new QTableWidgetItem(QString::number(p.x(), 'f', 3)));
        ui->dataTable->setItem(i, 3, new QTableWidgetItem(QString::number(p.y(), 'f', 3)));
        ui->dataTable->setItem(i, 4, new QTableWidgetItem(QString::number(p.z(), 'f', 3)));
        ui->dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(p.roll(), 'f', 2)));
        ui->dataTable->setItem(i, 6, new QTableWidgetItem(QString::number(p.pitch(), 'f', 2)));
        ui->dataTable->setItem(i, 7, new QTableWidgetItem(QString::number(p.yaw(), 'f', 2)));
        ui->dataTable->setItem(i, 8, new QTableWidgetItem(QString::number(p.rxPower(), 'f', 2)));
    }
}

void UwbDataVizWidget::onPointClicked(int index, const TrajectoryPoint &point)
{
    highlightTableRow(index);
}

void UwbDataVizWidget::onPointHovered(int index, const TrajectoryPoint &point)
{
    // Optional: show status bar message or tooltip
}

void UwbDataVizWidget::onTableItemClicked(int row, int column)
{
    if (row >= 0 && row < _currentPoints.size()) {
        _trajectoryView->highlightPoint(row);
    }
}

void UwbDataVizWidget::onTableSelectionChanged()
{
    QList<QTableWidgetItem*> items = ui->dataTable->selectedItems();
    if (!items.isEmpty()) {
        int row = items.first()->row();
        if (row >= 0 && row < _currentPoints.size()) {
            _trajectoryView->highlightPoint(row);
        }
    }
}

void UwbDataVizWidget::highlightTableRow(int row)
{
    if (row >= 0 && row < ui->dataTable->rowCount()) {
        ui->dataTable->selectRow(row);
        ui->dataTable->scrollToItem(ui->dataTable->item(row, 0));
    }
}

void UwbDataVizWidget::onExportCsv()
{
    if (_currentPoints.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No data to export"));
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this, tr("Export CSV"),
        QString("trajectory_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        tr("CSV Files (*.csv)"));

    if (filename.isEmpty()) return;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot open file for writing"));
        return;
    }

    QTextStream stream(&file);
    stream << "Time,Tag ID,X (m),Y (m),Z (m),Roll,Pitch,Yaw,Rx Power (dBm)\n";

    for (const auto &p : _currentPoints) {
        stream << p.time().toString("yyyy-MM-dd hh:mm:ss.zzz") << ","
               << p.tagId() << ","
               << p.x() << ","
               << p.y() << ","
               << p.z() << ","
               << p.roll() << ","
               << p.pitch() << ","
               << p.yaw() << ","
               << p.rxPower() << "\n";
    }

    file.close();
    updateStatus(tr("Exported to %1").arg(filename));
}

void UwbDataVizWidget::onExportImage()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Image"),
        QString("trajectory_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        tr("PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*)"));

    if (filename.isEmpty()) return;

    // Grab the trajectory view
    QPixmap pixmap = _trajectoryView->grab();
    if (pixmap.save(filename)) {
        updateStatus(tr("Image saved to %1").arg(filename));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save image"));
    }
}

void UwbDataVizWidget::onShowAnchorsToggled(bool checked)
{
    _trajectoryView->setShowAnchors(checked);
}

void UwbDataVizWidget::onShowGridToggled(bool checked)
{
    _trajectoryView->setShowGrid(checked);
}

void UwbDataVizWidget::onResetView()
{
    _trajectoryView->setTrajectory(_currentPoints);
}

void UwbDataVizWidget::onTagSelectionChanged(int index)
{
    // Optional: update time range based on selected tag
}

void UwbDataVizWidget::updateStatus(const QString &message, bool isError)
{
    ui->statusLabel->setText(message);
    ui->statusLabel->setStyleSheet(isError ? "color: red;" : "color: black;");
    qDebug() << (isError ? "Error:" : "Status:") << message;
}

void UwbDataVizWidget::loadSettings()
{
    QSettings settings;
    settings.beginGroup("UwbDataViz");

    ui->urlEdit->setText(settings.value("url", "http://localhost:8086").toString());
    ui->tokenEdit->setText(settings.value("token", "local-dev-token").toString());
    ui->orgEdit->setText(settings.value("org", "uwb-lab").toString());
    ui->bucketEdit->setText(settings.value("bucket", "uwb").toString());
    ui->measurementEdit->setText(settings.value("measurement", "uwb_tag_telemetry").toString());

    settings.endGroup();
}

void UwbDataVizWidget::saveSettings()
{
    QSettings settings;
    settings.beginGroup("UwbDataViz");

    settings.setValue("url", ui->urlEdit->text());
    settings.setValue("token", ui->tokenEdit->text());
    settings.setValue("org", ui->orgEdit->text());
    settings.setValue("bucket", ui->bucketEdit->text());
    settings.setValue("measurement", ui->measurementEdit->text());

    settings.endGroup();
}

void UwbDataVizWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}

void UwbDataVizWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Optionally auto-refresh tag list when shown
}

// Docker Management
void UwbDataVizWidget::onInitDatabase()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Initialize Database"),
        tr("This will create a new InfluxDB Docker environment.\n"
           "Default: Org=uwb-lab, Bucket=uwb, Token=local-dev-token\n\n"
           "Continue?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    // Use current config or defaults
    QString org = ui->orgEdit->text().isEmpty() ? "uwb-lab" : ui->orgEdit->text();
    QString bucket = ui->bucketEdit->text().isEmpty() ? "uwb" : ui->bucketEdit->text();
    QString token = ui->tokenEdit->text().isEmpty() ? "local-dev-token" : ui->tokenEdit->text();

    _dockerManager->initializeInfluxDB(org, bucket, "admin", "admin12345", token);

    updateStatus(tr("Initializing InfluxDB..."));
    ui->dockerStatusLabel->setText(tr("Status: Initializing..."));
}

void UwbDataVizWidget::onStartDocker()
{
    _dockerManager->startInfluxDB();
    updateStatus(tr("Starting InfluxDB Docker..."));
}

void UwbDataVizWidget::onStopDocker()
{
    _dockerManager->stopInfluxDB();
    updateStatus(tr("Stopping InfluxDB Docker..."));
}

void UwbDataVizWidget::onDockerStatusChanged(bool running)
{
    if (running) {
        ui->dockerStatusLabel->setText(tr("Status: Running"));
        ui->dockerStatusLabel->setStyleSheet("color: green;");
        ui->startDockerBtn->setEnabled(false);
        ui->stopDockerBtn->setEnabled(true);
    } else {
        ui->dockerStatusLabel->setText(tr("Status: Stopped"));
        ui->dockerStatusLabel->setStyleSheet("color: gray;");
        ui->startDockerBtn->setEnabled(true);
        ui->stopDockerBtn->setEnabled(false);
    }
}

void UwbDataVizWidget::onDockerMessage(const QString &message)
{
    updateStatus(message);
}

void UwbDataVizWidget::onDockerError(const QString &error)
{
    updateStatus(tr("Docker Error: %1").arg(error), true);
    QMessageBox::warning(this, tr("Docker Error"), error);
}

// Public accessor for write service
InfluxWriteService* UwbDataVizWidget::writeService() const
{
    return _writeService;
}
