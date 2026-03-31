// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxConfig.h
//
//  InfluxDB configuration data model
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef INFLUXCONFIG_H
#define INFLUXCONFIG_H

#include <QObject>
#include <QString>

class InfluxConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(QString org READ org WRITE setOrg NOTIFY orgChanged)
    Q_PROPERTY(QString bucket READ bucket WRITE setBucket NOTIFY bucketChanged)
    Q_PROPERTY(QString measurement READ measurement WRITE setMeasurement NOTIFY measurementChanged)

public:
    explicit InfluxConfig(QObject *parent = nullptr);

    QString url() const;
    void setUrl(const QString &url);

    QString token() const;
    void setToken(const QString &token);

    QString org() const;
    void setOrg(const QString &org);

    QString bucket() const;
    void setBucket(const QString &bucket);

    QString measurement() const;
    void setMeasurement(const QString &measurement);

    bool isValid() const;

signals:
    void urlChanged(const QString &url);
    void tokenChanged(const QString &token);
    void orgChanged(const QString &org);
    void bucketChanged(const QString &bucket);
    void measurementChanged(const QString &measurement);

private:
    QString _url;
    QString _token;
    QString _org;
    QString _bucket;
    QString _measurement;
};

#endif // INFLUXCONFIG_H
