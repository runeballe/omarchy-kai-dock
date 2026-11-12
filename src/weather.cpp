#include "weather.h"

#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

Weather::Weather(QObject *parent) : QObject(parent) {
    m_timer.setInterval(30 * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &Weather::refresh);
}

void Weather::setLocation(const QString &city, const QString &units) {
    const bool imperial = units == "imperial";
    if (city.trimmed() == m_city && imperial == m_imperial && m_status != "idle")
        return;
    m_city = city.trimmed();
    m_imperial = imperial;
    m_located = false;
    refresh();
}

void Weather::fail(const QString &message) {
    m_status = "error";
    m_error = message;
    emit changed();
}

void Weather::refresh() {
    if (m_city.isEmpty()) {
        m_status = "idle";
        m_error.clear();
        m_timer.stop();
        emit changed();
        return;
    }
    m_timer.start();
    m_status = m_status == "ok" ? "ok" : "loading";
    emit changed();
    if (m_located) {
        fetchForecast();
        return;
    }
    QUrl url("https://geocoding-api.open-meteo.com/v1/search");
    url.setQuery(QUrlQuery{{"name", m_city}, {"count", "1"}, {"format", "json"}});
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return fail(reply->errorString());
        const QJsonArray results = QJsonDocument::fromJson(reply->readAll()).object().value("results").toArray();
        if (results.isEmpty())
            return fail(tr("Couldn't find “%1”.").arg(m_city));
        const QJsonObject r = results.first().toObject();
        m_lat = r.value("latitude").toDouble();
        m_lon = r.value("longitude").toDouble();
        m_place = r.value("name").toString();
        m_located = true;
        fetchForecast();
    });
}

void Weather::fetchForecast() {
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery query{{"latitude", QString::number(m_lat, 'f', 4)},
                    {"longitude", QString::number(m_lon, 'f', 4)},
                    {"current", "temperature_2m,weather_code,wind_speed_10m,is_day"},
                    {"daily", "weather_code,temperature_2m_max,temperature_2m_min"},
                    {"forecast_days", "4"},
                    {"timezone", "auto"}};
    if (m_imperial) {
        query.addQueryItem("temperature_unit", "fahrenheit");
        query.addQueryItem("wind_speed_unit", "mph");
    }
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return fail(reply->errorString());
        if (!parseForecast(reply->readAll()))
            return fail(tr("The weather service sent something unexpected."));
        m_status = "ok";
        m_error.clear();
        emit changed();
    });
}

bool Weather::parseForecast(const QByteArray &json) {
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    const QJsonObject current = root.value("current").toObject();
    if (current.isEmpty())
        return false;
    m_temperature = current.value("temperature_2m").toDouble();
    m_wind = current.value("wind_speed_10m").toDouble();
    m_code = current.value("weather_code").toInt(-1);
    m_day = current.value("is_day").toInt(1) == 1;
    m_forecast.clear();
    const QJsonObject daily = root.value("daily").toObject();
    const QJsonArray days = daily.value("time").toArray();
    const QJsonArray codes = daily.value("weather_code").toArray();
    const QJsonArray highs = daily.value("temperature_2m_max").toArray();
    const QJsonArray lows = daily.value("temperature_2m_min").toArray();
    for (int i = 0; i < days.size(); ++i)
        m_forecast << QVariantMap{{"date", QDate::fromString(days[i].toString(), Qt::ISODate)},
                                  {"code", codes[i].toInt()}, {"kind", kindForCode(codes[i].toInt())},
                                  {"high", highs[i].toDouble()}, {"low", lows[i].toDouble()}};
    return true;
}

// WMO weather interpretation codes, grouped into the glyphs kai draws.
QString Weather::kindForCode(int code) {
    if (code < 0) return "unknown";
    if (code == 0) return "clear";
    if (code <= 2) return "partly";
    if (code == 3) return "cloudy";
    if (code == 45 || code == 48) return "fog";
    if (code >= 51 && code <= 57) return "drizzle";
    if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return "rain";
    if ((code >= 71 && code <= 77) || code == 85 || code == 86) return "snow";
    if (code >= 95) return "storm";
    return "cloudy";
}

QString Weather::describe(int code) {
    const QString kind = kindForCode(code);
    if (code == 1) return tr("Mostly clear");
    if (kind == "clear") return tr("Clear");
    if (kind == "partly") return tr("Partly cloudy");
    if (kind == "cloudy") return tr("Overcast");
    if (kind == "fog") return tr("Fog");
    if (kind == "drizzle") return tr("Drizzle");
    if (kind == "rain") return code >= 80 ? tr("Showers") : tr("Rain");
    if (kind == "snow") return tr("Snow");
    if (kind == "storm") return tr("Thunderstorm");
    return {};
}
