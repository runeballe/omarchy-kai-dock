#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantList>

// Current weather and a short forecast from Open-Meteo, which needs no API
// key. The city is looked up with Open-Meteo's geocoder; refreshes every
// 30 minutes.
class Weather : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)        // idle | loading | ok | error
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString place READ place NOTIFY changed)
    Q_PROPERTY(double temperature READ temperature NOTIFY changed)
    Q_PROPERTY(double wind READ wind NOTIFY changed)
    Q_PROPERTY(int code READ code NOTIFY changed)
    Q_PROPERTY(bool day READ day NOTIFY changed)
    Q_PROPERTY(QString kind READ kind NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(QString unit READ unit NOTIFY changed)
    Q_PROPERTY(QVariantList forecast READ forecast NOTIFY changed)

public:
    explicit Weather(QObject *parent = nullptr);

    QString status() const { return m_status; }
    QString error() const { return m_error; }
    QString place() const { return m_place; }
    double temperature() const { return m_temperature; }
    double wind() const { return m_wind; }
    int code() const { return m_code; }
    bool day() const { return m_day; }
    QString kind() const { return kindForCode(m_code); }
    QString description() const { return describe(m_code); }
    QString unit() const { return m_imperial ? "°F" : "°C"; }
    QVariantList forecast() const { return m_forecast; }

    Q_INVOKABLE void setLocation(const QString &city, const QString &units);
    Q_INVOKABLE void refresh();

    static QString kindForCode(int code);
    static QString describe(int code);
    bool parseForecast(const QByteArray &json);

signals:
    void changed();

private:
    void fail(const QString &message);
    void fetchForecast();

    QNetworkAccessManager m_network;
    QTimer m_timer;
    QString m_city, m_place, m_status = "idle", m_error;
    bool m_imperial = false;
    double m_lat = 0, m_lon = 0;
    bool m_located = false;
    double m_temperature = 0, m_wind = 0;
    int m_code = -1;
    bool m_day = true;
    QVariantList m_forecast;
};
