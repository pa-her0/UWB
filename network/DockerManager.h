// -------------------------------------------------------------------------------------------------------------------
//
//  File: DockerManager.h
//
//  Docker environment management for InfluxDB
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef DOCKERMANAGER_H
#define DOCKERMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>

class DockerManager : public QObject
{
    Q_OBJECT
public:
    explicit DockerManager(QObject *parent = nullptr);
    ~DockerManager();

    // Set the docker-compose.yml directory
    void setWorkingDirectory(const QString &path);
    QString workingDirectory() const;

    // Docker Compose operations
    void startInfluxDB();
    void stopInfluxDB();
    void restartInfluxDB();

    // Check if InfluxDB is running
    bool isRunning() const;

    // Get container status
    void checkStatus();

    // Get logs
    void fetchLogs();

    // Initialize InfluxDB (setup bucket, token, etc.)
    void initializeInfluxDB(const QString &org, const QString &bucket,
                            const QString &username, const QString &password,
                            const QString &token);

    // Check if Docker is available
    bool isDockerAvailable();

    // Get the detected docker path (for debugging)
    QString dockerPath() const;

signals:
    void statusChanged(bool running);
    void statusMessage(const QString &message);
    void error(const QString &error);
    void logsReady(const QString &logs);
    void initializationFinished(bool success, const QString &token);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    void executeCommand(const QString &command, const QStringList &args);
    QString findDockerCompose() const;
    void createDefaultComposeFile(const QString &path);
    void createInitComposeFile(const QString &path, const QString &org,
                               const QString &bucket, const QString &username,
                               const QString &password, const QString &token);

    QProcess *_process;
    QString _workingDir;
    bool _isRunning;
    QString _pendingCommand;
    mutable QString _dockerPathCache;
};

#endif // DOCKERMANAGER_H
