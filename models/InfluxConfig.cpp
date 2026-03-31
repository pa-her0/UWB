// -------------------------------------------------------------------------------------------------------------------
//
//  File: InfluxConfig.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "InfluxConfig.h"

InfluxConfig::InfluxConfig()
    : _url("http://localhost:8086")
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
    _url = url;
}

QString InfluxConfig::token() const
{
    return _token;
}

void InfluxConfig::setToken(const QString &token)
{
    _token = token;
}

QString InfluxConfig::org() const
{
    return _org;
}

void InfluxConfig::setOrg(const QString &org)
{
    _org = org;
}

QString InfluxConfig::bucket() const
{
    return _bucket;
}

void InfluxConfig::setBucket(const QString &bucket)
{
    _bucket = bucket;
}

QString InfluxConfig::measurement() const
{
    return _measurement;
}

void InfluxConfig::setMeasurement(const QString &measurement)
{
    _measurement = measurement;
}

bool InfluxConfig::isValid() const
{
    return !_url.isEmpty() && !_token.isEmpty() && !_org.isEmpty()
           && !_bucket.isEmpty() && !_measurement.isEmpty();
}
