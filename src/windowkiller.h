#pragma once

#include <QObject>
#include <QProcess>
#include <QVariantMap>

class Hyprland;

// WindowsKiller: the user picks a window on screen (slurp, one click) and kai
// either kills the process behind it or reports what it is. Picking the dock
// itself, an empty spot, or pressing Escape cancels.
class WindowKiller : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool picking READ picking NOTIFY pickingChanged)
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(QVariantMap last READ last NOTIFY lastChanged)
    Q_PROPERTY(QString status READ status NOTIFY lastChanged)

public:
    explicit WindowKiller(Hyprland *hyprland, QObject *parent = nullptr);

    bool picking() const { return m_process.state() != QProcess::NotRunning; }
    bool available() const;
    QVariantMap last() const { return m_last; }
    QString status() const { return m_status; }

    // mode is "kill" or "info".
    Q_INVOKABLE void pick(const QString &mode);
    Q_INVOKABLE void cancel();

    // The window under a point (global layout coordinates), from Hyprland's
    // JSON; empty when the point is on the dock, on no window, or on a window
    // not on a visible workspace.
    static QVariantMap clientAt(const QByteArray &clients, const QByteArray &monitors, const QByteArray &layers,
                                double x, double y);
    // pid, command line, uptime in seconds and executable name from /proc.
    static QVariantMap processInfo(qint64 pid, const QString &procRoot = "/proc");
    static QString formatUptime(qint64 seconds);
    // Processes kai refuses to kill: itself, init, and the compositor.
    static bool isProtected(qint64 pid, const QString &exe);

signals:
    void pickingChanged();
    void lastChanged();

private:
    void finished();
    void notify(const QString &summary, const QString &body, bool critical = false);

    Hyprland *m_hyprland;
    QProcess m_process;
    QString m_mode;
    QVariantMap m_last;
    QString m_status;
};
