#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>

// Applets written as scripts: each folder in ~/.config/kai/applets/ with an
// applet.json becomes an applet with the id "script:<folder>". kai runs its
// "exec" command every "interval" seconds and shows the output (plain text
// as quick-info, or JSON with text/tooltip/icon/progress), and runs the
// on_click, on_middle_click, on_double_click, on_scroll_up and
// on_scroll_down commands on those actions. Scripts can also push updates
// whenever they like through `kai badge`, `kai progress` and `kai animate`.
class ScriptApplets : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList applets READ applets NOTIFY appletsChanged)
    // id -> { text, tooltip, icon, progress, error }
    Q_PROPERTY(QVariantMap states READ states NOTIFY statesChanged)
    Q_PROPERTY(QString directory READ directory CONSTANT)

public:
    explicit ScriptApplets(const QString &directory = {}, QObject *parent = nullptr);
    ~ScriptApplets() override;

    QVariantList applets() const;
    QVariantMap states() const { return m_states; }
    QString directory() const { return m_directory; }
    QStringList ids() const;

    Q_INVOKABLE QVariantMap info(const QString &id) const { return m_manifests.value(id); }
    // action: click, middle, double, scroll-up, scroll-down
    Q_INVOKABLE bool trigger(const QString &id, const QString &action);
    Q_INVOKABLE bool hasAction(const QString &id, const QString &action) const;
    Q_INVOKABLE void refresh(const QString &id);
    void rescan();

    // Reads applet.json; empty when it is missing or broken.
    static QVariantMap readManifest(const QString &folder);
    // Turns what "exec" printed into { text, tooltip, icon, progress }.
    static QVariantMap parseOutput(const QByteArray &output);
    static QString actionKey(const QString &action);
    static QString defaultDirectory();

signals:
    void appletsChanged();
    void statesChanged();

private:
    void schedule();

    QString m_directory;
    QHash<QString, QVariantMap> m_manifests;  // id -> manifest (+ "dir")
    QVariantMap m_states;
    QHash<QString, QTimer *> m_timers;
    QHash<QString, QProcess *> m_running;
    QHash<QString, quint64> m_runs;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
