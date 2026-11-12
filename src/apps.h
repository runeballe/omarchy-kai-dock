#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

struct AppEntry {
    QString id;          // desktop file id without ".desktop"
    QString name;
    QString genericName;
    QString comment;
    QString icon;
    QString exec;
    QString wmClass;
    QString path;
    QStringList keywords;
    bool noDisplay = false;
    bool terminal = false;
};

// Knows the installed applications from their .desktop files, matches a
// window's class to one of them, and launches them the way the desktop does
// (through uwsm-app when it's there, so apps land in their own systemd scope).
class AppIndex : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY changed)

public:
    explicit AppIndex(const QStringList &directories = {}, QObject *parent = nullptr);

    QVariantList apps() const;
    Q_INVOKABLE QVariantMap entry(const QString &id) const;
    Q_INVOKABLE QString matchClass(const QString &windowClass) const;
    Q_INVOKABLE bool contains(const QString &id) const { return m_entries.contains(id); }
    Q_INVOKABLE void launch(const QString &id) const;
    Q_INVOKABLE QStringList search(const QString &text) const;

    const AppEntry *find(const QString &id) const;
    void rescan();

    static AppEntry parseDesktopFile(const QString &path, const QString &id);
    static QString stripFieldCodes(const QString &exec);
    static QStringList defaultDirectories();

signals:
    void changed();

private:
    QStringList m_directories;
    QHash<QString, AppEntry> m_entries;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
