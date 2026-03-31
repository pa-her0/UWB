// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxConfig.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "InfluxConfig.h"

InfluxConfig::InfluxConfig(QObject *parent)
    : QObject(parent)
    , _url("http://localhost:8086")
    , _token("local-dev-token")
    , _org("uwb-lab")
    , _bucket("uwb")
    , _measurement("uwb_tag_telemetry")
{
}

QString InfluxConfig::url() const
{
    return _url;
}

void InfluxConfig::setUrl(const QString &url)
{
    if (_url != url) {
        _url = url;
        emit urlChanged(_url);
    }
}

QString InfluxConfig::token() const
{
    return _token;
}

void InfluxConfig::setToken(const QString &token)
{
    if (_token != token) {
        _token = token;
        emit tokenChanged(_token);
    }
}

QString InfluxConfig::org() const
{
    return _org;
}

void InfluxConfig::setOrg(const QString &org)
{
    if (_org != org) {
        _org = org;
        emit orgChanged(_org);
    }
}

QString InfluxConfig::bucket() const
{
    return _bucket;
}

void InfluxConfig::setBucket(const QString &bucket)
{
    if (_bucket != bucket) {
        _bucket = bucket;
        emit bucketChanged(_bucket);
    }
}

QString InfluxConfig::measurement() const
{
    return _measurement;
}

void InfluxConfig::setMeasurement(const QString &measurement)
{
    if (_measurement != measurement) {
        _measurement = measurement;
        emit measurementChanged(_measurement);
    }
}

bool InfluxConfig::isValid() const
{
    return !_url.isEmpty() && !_token.isEmpty() && !_org.isEmpty()
           && !_bucket.isEmpty() && !_measurement.isEmpty();
}
