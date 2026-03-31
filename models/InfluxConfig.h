// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxConfig.h
//
//  InfluxDB configuration data model
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef INFLUXCONFIG_H
#define INFLUXCONFIG_H

#include <QString>

class InfluxConfig
{
public:
    InfluxConfig();

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

private:
    QString _url;
    QString _token;
    QString _org;
    QString _bucket;
    QString _measurement;
};

#endif // INFLUXCONFIG_H
