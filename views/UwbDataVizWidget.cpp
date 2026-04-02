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
#include "RTLSDisplayApplication.h"
#include "ViewSettings.h"

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
#include <QPainterPath>
#include <QFontMetrics>
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QTimer>
#include <cmath>

UwbDataVizWidget::UwbDataVizWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UwbDataVizWidget)
    , _queryService(new InfluxQueryService(this))
    , _writeService(new InfluxWriteService(this))
    , _dockerManager(new DockerManager(this))
    , _isLoading(false)
{
    ui->setupUi(this);

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
            _anchors = anchorList;
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
    _currentPoints = sortAndDedup(points);
    populateTable(_currentPoints);
    updateStatus(tr("Loaded %1 points").arg(_currentPoints.size()));
}

void UwbDataVizWidget::onTelemetryReady(const QList<TrajectoryPoint> &points)
{
    _currentPoints = sortAndDedup(points);
    populateTable(_currentPoints);
    updateStatus(tr("Loaded %1 points").arg(_currentPoints.size()));
}

QList<TrajectoryPoint> UwbDataVizWidget::sortAndDedup(const QList<TrajectoryPoint> &points)
{
    // Sort by timestamp — Flux pivot() only sorts within each result table,
    // so cross-table data arrives out of order, causing the trajectory to
    // draw lines jumping back and forth across the map.
    QList<TrajectoryPoint> sorted = points;
    std::stable_sort(sorted.begin(), sorted.end(),
        [](const TrajectoryPoint &a, const TrajectoryPoint &b) {
            return a.time() < b.time();
        });

    // Deduplicate exact same timestamp (can happen when pivot returns
    // overlapping tables for the same tag).
    QList<TrajectoryPoint> result;
    result.reserve(sorted.size());
    for (const TrajectoryPoint &pt : sorted) {
        if (!result.isEmpty() && result.last().time() == pt.time())
            continue;
        result.append(pt);
    }
    return result;
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

void UwbDataVizWidget::onTableItemClicked(int row, int column)
{
    Q_UNUSED(row); Q_UNUSED(column);
}

void UwbDataVizWidget::onTableSelectionChanged()
{
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
    if (_currentPoints.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No data to export"));
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this, tr("Export Trajectory Image"),
        QString("trajectory_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        tr("PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*)"));

    if (filename.isEmpty()) return;

    QPixmap pixmap = renderTrajectoryImage(1200, 900);
    if (pixmap.save(filename)) {
        updateStatus(tr("Image saved to %1").arg(filename));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save image"));
    }
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

QPixmap UwbDataVizWidget::renderTrajectoryImage(int width, int height) const
{
    QPixmap pixmap(width, height);
    pixmap.fill(QColor(248, 250, 252));
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);

    const int margin = 70;
    QRect plotRect(margin, margin, width - 2 * margin, height - 2 * margin);

    // Compute data bounds (include anchors)
    double minX = 1e10, minY = 1e10, maxX = -1e10, maxY = -1e10;
    for (const auto &pt : _currentPoints) {
        minX = std::min(minX, pt.x()); maxX = std::max(maxX, pt.x());
        minY = std::min(minY, pt.y()); maxY = std::max(maxY, pt.y());
    }
    for (const auto &a : _anchors) {
        minX = std::min(minX, a.x); maxX = std::max(maxX, a.x);
        minY = std::min(minY, a.y); maxY = std::max(maxY, a.y);
    }
    double spanX = maxX - minX;
    double spanY = maxY - minY;
    double padX = std::max(spanX * 0.12, 0.5);
    double padY = std::max(spanY * 0.12, 0.5);
    minX -= padX; maxX += padX;
    minY -= padY; maxY += padY;
    spanX = maxX - minX;
    spanY = maxY - minY;

    // Map world → pixel (Y flipped: world Y up, screen Y down)
    auto toPixel = [&](double wx, double wy) -> QPointF {
        double px = plotRect.left() + (wx - minX) / spanX * plotRect.width();
        double py = plotRect.bottom() - (wy - minY) / spanY * plotRect.height();
        return QPointF(px, py);
    };

    // White plot background
    p.setPen(QPen(QColor(203, 213, 225), 1.5));
    p.setBrush(Qt::white);
    p.drawRect(plotRect);

    // Floor plan background (if loaded and calibrated)
    {
        ViewSettings *vs = RTLSDisplayApplication::viewSettings();
        const QPixmap &fp = vs->floorplanPixmap();
        if (!fp.isNull() && vs->floorplanXScale() != 0 && vs->floorplanYScale() != 0) {
            double xscale  = vs->floorplanXScale();   // pixels per metre
            double yscale  = vs->floorplanYScale();
            double xoffset = vs->floorplanXOffset();  // pixel offset
            double yoffset = vs->floorplanYOffset();

            // Pixel (0,0) of the image maps to world coords:
            double fpWorldX0 = -xoffset / xscale;
            double fpWorldY0 = -yoffset / yscale;
            // Pixel (w,h) maps to:
            double fpWorldX1 = (fp.width()  - xoffset) / xscale;
            double fpWorldY1 = (fp.height() - yoffset) / yscale;

            // Handle flip
            if (vs->floorplanFlipX()) std::swap(fpWorldX0, fpWorldX1);
            if (vs->floorplanFlipY()) std::swap(fpWorldY0, fpWorldY1);

            QPointF tl = toPixel(fpWorldX0, fpWorldY1); // world Y up → screen top
            QPointF br = toPixel(fpWorldX1, fpWorldY0);
            QRectF destRect(tl, br);

            p.save();
            p.setClipRect(plotRect);
            p.setOpacity(0.45);
            p.drawPixmap(destRect.toRect(), fp);
            p.setOpacity(1.0);
            p.restore();
        }
    }

    // Grid lines
    auto niceStep = [](double span, int ticks) -> double {
        double step = span / ticks;
        double mag = std::pow(10.0, std::floor(std::log10(step)));
        double n = step / mag;
        if (n < 2) return mag;
        if (n < 5) return 2 * mag;
        return 5 * mag;
    };
    QPen gridPen(QColor(228, 231, 235), 1);
    p.setPen(gridPen);
    QFont labelFont;
    labelFont.setPointSize(8);
    p.setFont(labelFont);
    p.setPen(QColor(100, 116, 139));

    double stepX = niceStep(spanX, 6);
    double stepY = niceStep(spanY, 6);
    double gx0 = std::ceil(minX / stepX) * stepX;
    double gy0 = std::ceil(minY / stepY) * stepY;

    for (double gx = gx0; gx <= maxX + 1e-9; gx += stepX) {
        QPointF top = toPixel(gx, maxY);
        QPointF bot = toPixel(gx, minY);
        p.setPen(gridPen);
        p.drawLine(top, bot);
        p.setPen(QColor(100, 116, 139));
        p.drawText(QRectF(bot.x() - 30, bot.y() + 4, 60, 16),
                   Qt::AlignCenter, QString::number(gx, 'f', 2));
    }
    for (double gy = gy0; gy <= maxY + 1e-9; gy += stepY) {
        QPointF left  = toPixel(minX, gy);
        QPointF right = toPixel(maxX, gy);
        p.setPen(gridPen);
        p.drawLine(left, right);
        p.setPen(QColor(100, 116, 139));
        p.drawText(QRectF(left.x() - margin + 2, left.y() - 8, margin - 6, 16),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(gy, 'f', 2));
    }

    // Axis labels
    QFont axisFont;
    axisFont.setPointSize(9);
    axisFont.setBold(true);
    p.setFont(axisFont);
    p.setPen(QColor(51, 65, 85));
    p.drawText(QRectF(0, height - 20, width, 18), Qt::AlignCenter, "X (m)");
    p.save();
    p.translate(14, height / 2);
    p.rotate(-90);
    p.drawText(QRectF(-60, -8, 120, 16), Qt::AlignCenter, "Y (m)");
    p.restore();

    // Anchors
    for (const auto &a : _anchors) {
        QPointF ap = toPixel(a.x, a.y);
        p.setPen(QPen(QColor(124, 45, 18), 2));
        p.setBrush(QColor(217, 72, 15));
        p.drawEllipse(ap, 8, 8);
        QFont af; af.setPointSize(8); af.setBold(true);
        p.setFont(af);
        p.setPen(QColor(124, 45, 18));
        p.drawText(QRectF(ap.x() + 10, ap.y() - 18, 120, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 (%2,%3)").arg(a.name).arg(a.x, 0, 'f', 2).arg(a.y, 0, 'f', 2));
    }

    if (_currentPoints.size() < 2) {
        p.end();
        return pixmap;
    }

    // Trajectory path
    QPainterPath path;
    path.moveTo(toPixel(_currentPoints[0].x(), _currentPoints[0].y()));
    for (int i = 1; i < _currentPoints.size(); ++i)
        path.lineTo(toPixel(_currentPoints[i].x(), _currentPoints[i].y()));

    p.setPen(QPen(QColor(37, 99, 235), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    // Trajectory dots
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(37, 99, 235));
    for (const auto &pt : _currentPoints) {
        QPointF pp = toPixel(pt.x(), pt.y());
        p.drawEllipse(pp, 3, 3);
    }

    // Start marker (green circle with "S")
    QPointF startPx = toPixel(_currentPoints.first().x(), _currentPoints.first().y());
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(QColor(22, 163, 74));
    p.drawEllipse(startPx, 10, 10);
    QFont markerFont; markerFont.setPointSize(8); markerFont.setBold(true);
    p.setFont(markerFont);
    p.setPen(Qt::white);
    p.drawText(QRectF(startPx.x() - 10, startPx.y() - 10, 20, 20), Qt::AlignCenter, "S");

    // End marker (red circle with "E")
    QPointF endPx = toPixel(_currentPoints.last().x(), _currentPoints.last().y());
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(QColor(220, 38, 38));
    p.drawEllipse(endPx, 10, 10);
    p.setPen(Qt::white);
    p.drawText(QRectF(endPx.x() - 10, endPx.y() - 10, 20, 20), Qt::AlignCenter, "E");

    // Legend
    int lx = plotRect.right() - 130, ly = plotRect.top() + 10;
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.setBrush(QColor(255, 255, 255, 220));
    p.drawRoundedRect(lx - 8, ly - 6, 138, 80, 6, 6);
    QFont legFont; legFont.setPointSize(8);
    p.setFont(legFont);
    // trajectory line
    p.setPen(QPen(QColor(37, 99, 235), 2.5));
    p.drawLine(lx, ly + 8, lx + 22, ly + 8);
    p.setPen(QColor(51, 65, 85));
    p.drawText(lx + 28, ly + 13, tr("Trajectory"));
    // start
    p.setPen(Qt::NoPen); p.setBrush(QColor(22, 163, 74));
    p.drawEllipse(QPointF(lx + 11, ly + 30), 7, 7);
    p.setPen(Qt::white); p.setFont(markerFont);
    p.drawText(QRectF(lx + 4, ly + 23, 14, 14), Qt::AlignCenter, "S");
    p.setFont(legFont); p.setPen(QColor(51, 65, 85));
    p.drawText(lx + 28, ly + 35, tr("Start"));
    // end
    p.setPen(Qt::NoPen); p.setBrush(QColor(220, 38, 38));
    p.drawEllipse(QPointF(lx + 11, ly + 52), 7, 7);
    p.setPen(Qt::white); p.setFont(markerFont);
    p.drawText(QRectF(lx + 4, ly + 45, 14, 14), Qt::AlignCenter, "E");
    p.setFont(legFont); p.setPen(QColor(51, 65, 85));
    p.drawText(lx + 28, ly + 57, tr("End"));

    // Title
    QFont titleFont; titleFont.setPointSize(12); titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    QString tagId = _currentPoints.first().tagId();
    QString title = tr("UWB Trajectory — Tag: %1  (%2 points)")
                    .arg(tagId).arg(_currentPoints.size());
    p.drawText(QRectF(0, 6, width, 30), Qt::AlignCenter, title);

    p.end();
    return pixmap;
}

// Public accessor for write service
InfluxWriteService* UwbDataVizWidget::writeService() const
{
    return _writeService;
}
