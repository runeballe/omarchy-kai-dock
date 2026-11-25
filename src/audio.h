#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>

// The default output's volume and mute state through PipeWire's wpctl,
// polled every couple of seconds so changes made elsewhere show up.
class Audio : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(double volume READ volume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)

public:
    explicit Audio(QObject *parent = nullptr);

    bool available() const { return m_available; }
    double volume() const { return m_volume; }
    bool muted() const { return m_muted; }

    Q_INVOKABLE void step(int percent);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void refresh();

    // "Volume: 0.45 [MUTED]" -> {0.45, true}; false on anything else.
    static bool parse(const QString &line, double *volume, bool *muted);

signals:
    void changed();

private:
    void run(const QStringList &args);

    bool m_available = false;
    double m_volume = 0;
    bool m_muted = false;
    QProcess m_query;
    QTimer m_poll;
};
