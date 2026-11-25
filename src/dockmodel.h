#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QTimer>
#include <QVariantList>

class AppIndex;
class Config;
class Hyprland;

struct DockItem {
    QString key;           // stable identity: app:<id>, win:<class>, sep:<n>, applet:<id>
    QString kind;          // app | separator | applet
    QString appId;
    QString windowClass;
    QString name;
    QString icon;
    QString appletId;
    bool pinned = false;
    bool active = false;
    bool urgent = false;
    bool launching = false;
    QVariantList windows;  // { address, title, minimized, focused, workspace }

    bool operator==(const DockItem &o) const {
        return key == o.key && kind == o.kind && appId == o.appId && name == o.name && icon == o.icon
            && pinned == o.pinned && active == o.active && urgent == o.urgent && launching == o.launching
            && windows == o.windows && windowClass == o.windowClass;
    }
};

// The icons in the dock, in order: pinned launchers, then other running apps,
// then the docked applets. Rows keep their identity across updates so QML
// delegates (and the animations running in them) survive window changes.
class DockModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        KeyRole = Qt::UserRole + 1, KindRole, AppIdRole, NameRole, IconRole, AppletIdRole, PinnedRole,
        RunningRole, WindowCountRole, ActiveRole, UrgentRole, LaunchingRole, WindowsRole,
    };

    DockModel(Config *config, AppIndex *apps, Hyprland *hyprland, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_items.size(); }

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void launchNew(int row);
    Q_INVOKABLE void cycle(int row, int direction);
    Q_INVOKABLE void closeAll(int row);
    Q_INVOKABLE void togglePin(int row);
    Q_INVOKABLE void movePinned(int fromRow, int toRow);
    Q_INVOKABLE void focusWindow(const QString &address);
    Q_INVOKABLE void closeWindow(const QString &address);

    QList<DockItem> build() const;
    const QList<DockItem> &items() const { return m_items; }

public slots:
    void rebuild();

signals:
    void countChanged();
    void launched(int row);

private:
    void apply(const QList<DockItem> &target);
    void seedLaunchers();

    Config *m_config;
    AppIndex *m_apps;
    Hyprland *m_hyprland;
    QList<DockItem> m_items;
    QHash<QString, qint64> m_launching;  // app id -> time the launch started
    QTimer m_debounce;
    QTimer m_launchTimeout;
};
