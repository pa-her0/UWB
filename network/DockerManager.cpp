// -------------------------------------------------------------------------------------------------------------------
//
//  File: DockerManager.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "DockerManager.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

DockerManager::DockerManager(QObject *parent)
    : QObject(parent)
    , _process(new QProcess(this))
    , _isRunning(false)
{
    connect(_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DockerManager::onProcessFinished);
    connect(_process, &QProcess::errorOccurred,
            this, &DockerManager::onProcessError);
    connect(_process, &QProcess::readyReadStandardOutput,
            this, &DockerManager::onReadyReadStandardOutput);
    connect(_process, &QProcess::readyReadStandardError,
            this, &DockerManager::onReadyReadStandardError);
}

DockerManager::~DockerManager()
{
    if (_process->state() != QProcess::NotRunning) {
        _process->terminate();
        _process->waitForFinished(3000);
    }
}

void DockerManager::setWorkingDirectory(const QString &path)
{
    _workingDir = path;
    _process->setWorkingDirectory(path);
}

QString DockerManager::workingDirectory() const
{
    return _workingDir;
}

QString DockerManager::findDockerCompose() const
{
    // Try docker compose (newer) first, then docker-compose (older)
    QStringList candidates;
    candidates << "docker compose"
              << "docker-compose"
              << "/usr/local/bin/docker-compose"
              << "/usr/bin/docker-compose"
    #ifdef Q_OS_WIN
              << "docker.exe compose"
    #endif
              ;

    for (const QString &cmd : candidates) {
        QString program = cmd.split(" ").first();
        QString fullPath = QStandardPaths::findExecutable(program);
        if (!fullPath.isEmpty()) {
            return cmd;
        }
    }

    return "docker compose"; // Default fallback
}

bool DockerManager::isDockerAvailable() const
{
    QProcess testProcess;
    testProcess.start("docker", QStringList() << "version");
    testProcess.waitForFinished(5000);
    return testProcess.exitCode() == 0;
}

void DockerManager::executeCommand(const QString &command, const QStringList &args)
{
    if (_process->state() != QProcess::NotRunning) {
        emit statusMessage(tr("Previous command still running..."));
        return;
    }

    QString cmd = findDockerCompose();
    QStringList allArgs;

    if (cmd.contains(" ")) {
        // docker compose style (command with space)
        allArgs = cmd.split(" ");
        allArgs.removeFirst(); // Remove 'docker'
        allArgs += args;
        cmd = "docker";
    } else {
        allArgs = args;
    }

    _pendingCommand = command;

    emit statusMessage(tr("Executing: %1 %2").arg(cmd, allArgs.join(" ")));

    _process->start(cmd, allArgs);
}

void DockerManager::startInfluxDB()
{
    // Check if docker-compose.yml exists, if not create default
    QString composePath = _workingDir + "/docker-compose.yml";
    if (!QFile::exists(composePath)) {
        createDefaultComposeFile(composePath);
    }

    QStringList args;
    args << "up" << "-d";
    executeCommand("start", args);
}

void DockerManager::stopInfluxDB()
{
    QStringList args;
    args << "down";
    executeCommand("stop", args);
}

void DockerManager::restartInfluxDB()
{
    QStringList args;
    args << "restart";
    executeCommand("restart", args);
}

void DockerManager::checkStatus()
{
    QStringList args;
    args << "ps" << "--format" <> "table {{.Names}}\t{{.Status}}\t{{.Ports}}";
    executeCommand("status", args);
}

void DockerManager::fetchLogs()
{
    QStringList args;
    args << "logs" << "--tail" <> "100";
    executeCommand("logs", args);
}

void DockerManager::initializeInfluxDB(const QString &org, const QString &bucket,
                                       const QString &username, const QString &password,
                                       const QString &token)
{
    // Create docker-compose.yml with initialization
    QString composePath = _workingDir + "/docker-compose.yml";
    createInitComposeFile(composePath, org, bucket, username, password, token);

    emit statusMessage(tr("Created initialization config. Starting InfluxDB..."));
    startInfluxDB();
}

void DockerManager::createDefaultComposeFile(const QString &path) const
{
    QString content = R"(services:
  influxdb:
    image: influxdb:2.7
    container_name: uwb-influxdb
    ports:
      - "8086:8086"
    volumes:
      - ./influxdb_data:/var/lib/influxdb2
    environment:
      - DOCKER_INFLUXDB_INIT_MODE=setup
      - DOCKER_INFLUXDB_INIT_USERNAME=admin
      - DOCKER_INFLUXDB_INIT_PASSWORD=admin12345
      - DOCKER_INFLUXDB_INIT_ORG=uwb-lab
      - DOCKER_INFLUXDB_INIT_BUCKET=uwb
      - DOCKER_INFLUXDB_INIT_ADMIN_TOKEN=local-dev-token
)";

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(content.toUtf8());
        file.close();
        emit statusMessage(tr("Created default docker-compose.yml"));
    }
}

void DockerManager::createInitComposeFile(const QString &path, const QString &org,
                                          const QString &bucket, const QString &username,
                                          const QString &password, const QString &token) const
{
    QString content = QString(R"(services:
  influxdb:
    image: influxdb:2.7
    container_name: uwb-influxdb
    ports:
      - "8086:8086"
    volumes:
      - ./influxdb_data:/var/lib/influxdb2
    environment:
      - DOCKER_INFLUXDB_INIT_MODE=setup
      - DOCKER_INFLUXDB_INIT_USERNAME=%1
      - DOCKER_INFLUXDB_INIT_PASSWORD=%2
      - DOCKER_INFLUXDB_INIT_ORG=%3
      - DOCKER_INFLUXDB_INIT_BUCKET=%4
      - DOCKER_INFLUXDB_INIT_ADMIN_TOKEN=%5
)").arg(username, password, org, bucket, token);

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(content.toUtf8());
        file.close();
        emit statusMessage(tr("Created initialization docker-compose.yml"));
    }
}

bool DockerManager::isRunning() const
{
    return _isRunning;
}

void DockerManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit statusMessage(tr("Command completed successfully"));

        if (_pendingCommand == "start") {
            _isRunning = true;
            emit statusChanged(true);
        } else if (_pendingCommand == "stop") {
            _isRunning = false;
            emit statusChanged(false);
        }
    } else {
        emit error(tr("Command failed with exit code %1").arg(exitCode));
    }

    _pendingCommand.clear();
}

void DockerManager::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = tr("Docker command failed to start. Is Docker installed?");
        break;
    case QProcess::Crashed:
        errorMsg = tr("Docker command crashed");
        break;
    case QProcess::Timedout:
        errorMsg = tr("Docker command timed out");
        break;
    case QProcess::WriteError:
        errorMsg = tr("Write error");
        break;
    case QProcess::ReadError:
        errorMsg = tr("Read error");
        break;
    default:
        errorMsg = tr("Unknown error occurred");
    }

    emit this->error(errorMsg);
    _pendingCommand.clear();
}

void DockerManager::onReadyReadStandardOutput()
{
    QByteArray output = _process->readAllStandardOutput();
    QString text = QString::fromUtf8(output);

    if (_pendingCommand == "logs") {
        emit logsReady(text);
    } else {
        emit statusMessage(text.trimmed());
    }
}

void DockerManager::onReadyReadStandardError()
{
    QByteArray output = _process->readAllStandardError();
    QString text = QString::fromUtf8(output);
    emit error(text.trimmed());
}
