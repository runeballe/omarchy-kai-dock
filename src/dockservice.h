#pragma once

#include <QObject>
#include <QStringList>

class Badges;

// kai's D-Bus API, org.kai.Dock at /org/kai/Dock on the session bus. It is
// what `kai applet`, `kai badge`, `kai progress` and `kai animate` call, so
// Hyprland key bindings and scripts can drive the dock. A key is an applet id
// ("clock", "script:uptime") or an app's desktop file id ("firefox").
class DockService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kai.Dock")

public:
    explicit DockService(Badges *badges, QObject *parent = nullptr);

    bool registerOnBus();
    static QString badgeKey(const QString &key);

public slots:
    // action: click (default), middle, double, scroll-up, scroll-down
    bool Activate(const QString &applet, const QString &action);
    void SetQuickInfo(const QString &key, const QString &text);
    void SetProgress(const QString &key, double percent);
    void Clear(const QString &key);
    void Animate(const QString &key, const QString &animation);
    void OpenSettings();
    QStringList Applets();

signals:
    // Handled in QML, where the applets and icons live.
    void appletAction(const QString &applet, const QString &action);
    void animateRequested(const QString &key, const QString &animation);
    void settingsRequested();

private:
    Badges *m_badges;
};

// The command-line side: sends one request to the running dock over D-Bus.
// Returns the process exit code.
int dockServiceCommand(const QStringList &args);
