#include "dockmodel.h"

#include "apps.h"
#include "config.h"
#include "hyprland.h"

#include <QDateTime>
#include <QSet>

DockModel::DockModel(Config *config, AppIndex *apps, Hyprland *hyprland, QObject *parent)
    : QAbstractListModel(parent), m_config(config), m_apps(apps), m_hyprland(hyprland) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(0);
    connect(&m_debounce, &QTimer::timeout, this, &DockModel::rebuild);
    auto later = [this] { m_debounce.start(); };
    connect(config, &Config::changed, this, later);
    connect(apps, &AppIndex::changed, this, later);
    if (hyprland)
        connect(hyprland, &Hyprland::clientsChanged, this, later);

    // A launch that never shows a window stops bouncing after a while.
    m_launchTimeout.setInterval(1000);
    connect(&m_launchTimeout, &QTimer::timeout, this, [this] {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        bool changed = false;
        for (auto it = m_launching.begin(); it != m_launching.end();) {
            if (now - it.value() > 10000) {
                it = m_launching.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
        if (m_launching.isEmpty())
            m_launchTimeout.stop();
        if (changed)
            rebuild();
    });

    seedLaunchers();
    rebuild();
}

// The first run pins a handful of the apps Omarchy ships, so the dock isn't empty.
void DockModel::seedLaunchers() {
    if (m_config->get("launchersSeeded").toBool())
        return;
    const QStringList candidates{"chromium", "org.chromium.Chromium", "firefox", "Alacritty", "com.mitchellh.ghostty",
                                 "kitty", "org.gnome.Nautilus", "typora", "spotify", "obsidian", "signal-desktop",
                                 "org.gnome.Calculator", "code", "dev.zed.Zed", "1password",
                                 "org.localsend.localsend_app"};
    QVariantList pinned;
    for (const auto &id : candidates)
        if (m_apps->contains(id) && pinned.size() < 8)
            pinned << id;
    m_config->set("launchers", pinned);
    m_config->set("launchersSeeded", true);
}

QList<DockItem> DockModel::build() const {
    QList<DockItem> out;
    QHash<QString, int> byApp;  // app id or class key -> index in out

    auto makeApp = [this](const QString &appId) {
        DockItem item;
        item.kind = "app";
        item.appId = appId;
        item.key = "app:" + appId;
        if (const AppEntry *entry = m_apps->find(appId)) {
            item.name = entry->name;
            item.icon = entry->icon;
        } else {
            item.name = appId;
            item.icon = appId;
        }
        return item;
    };

    for (const auto &value : m_config->get("launchers").toList()) {
        const QString id = value.toString();
        if (id.isEmpty() || byApp.contains("app:" + id))
            continue;
        DockItem item = makeApp(id);
        item.pinned = true;
        byApp.insert(item.key, out.size());
        out << item;
    }
    const int pinnedCount = out.size();

    const bool showRunning = m_config->get("showRunning").toBool();
    const bool currentOnly = m_config->get("currentWorkspaceOnly").toBool();
    const int activeWs = m_hyprland ? m_hyprland->activeWorkspace() : 0;
    if (m_hyprland) {
        // Most recently focused first, so the window a click brings up is the
        // one the user last saw.
        QList<HyprClient> clients = m_hyprland->clients();
        std::stable_sort(clients.begin(), clients.end(),
                         [](const HyprClient &a, const HyprClient &b) { return a.focusHistory < b.focusHistory; });
        for (const auto &c : clients) {
            // The dock's own settings window isn't an app to switch to.
            if (c.windowClass == QLatin1String("kai"))
                continue;
            const QString appId = m_apps->matchClass(c.windowClass);
            const QString key = appId.isEmpty() ? "win:" + c.windowClass.toLower() : "app:" + appId;
            int index = byApp.value(key, -1);
            if (index < 0) {
                if (!showRunning)
                    continue;
                if (currentOnly && !c.minimized && c.workspaceId != activeWs)
                    continue;
                DockItem item;
                if (!appId.isEmpty()) {
                    item = makeApp(appId);
                } else {
                    item.kind = "app";
                    item.key = key;
                    item.name = c.windowClass;
                    item.icon = c.windowClass.toLower();
                }
                item.windowClass = c.windowClass;
                index = out.size();
                byApp.insert(key, index);
                out << item;
            }
            DockItem &item = out[index];
            if (item.windowClass.isEmpty())
                item.windowClass = c.windowClass;
            item.windows << QVariantMap{{"address", c.address}, {"title", c.title}, {"minimized", c.minimized},
                                        {"focused", c.focusHistory == 0}, {"workspace", c.workspaceId}};
            item.active = item.active || c.focusHistory == 0;
            item.urgent = item.urgent || c.urgent;
        }
    }

    for (auto &item : out)
        if (!item.appId.isEmpty() && m_launching.contains(item.appId) && item.windows.isEmpty())
            item.launching = true;

    QList<DockItem> applets;
    DockItem appsFront;
    bool hasAppsFront = false;
    for (const auto &value : m_config->get("applets").toList()) {
        const QVariantMap a = value.toMap();
        if (!a.value("enabled").toBool() || a.value("detached").toBool())
            continue;
        DockItem item;
        item.kind = "applet";
        item.appletId = a.value("id").toString();
        item.key = "applet:" + item.appletId;
        item.name = item.appletId;
        if (item.appletId == QLatin1String("apps")) {
            appsFront = item;
            hasAppsFront = true;
            continue;
        }
        applets << item;
    }

    QList<DockItem> result;
    // User-placed separators: "launcher" or a launcher app id, from the
    // `separators` setting, each putting a line after that item.
    QSet<QString> userSeps;
    for (const auto &value : m_config->get("separators").toList())
        userSeps.insert(value.toString());
    auto addUserSep = [&](const QString &key) {
        DockItem sep;
        sep.kind = "separator";
        sep.key = "sep:user:" + key;
        result << sep;
    };

    // The launcher opens from the front of the dock, flush before the pinned
    // apps and with no separator of its own.
    if (hasAppsFront) {
        result << appsFront;
        if (userSeps.contains("launcher"))
            addUserSep("launcher");
    }
    for (int i = 0; i < out.size(); ++i) {
        result << out[i];
        if (i == pinnedCount - 1 && pinnedCount > 0 && out.size() > pinnedCount) {
            DockItem sep;
            sep.kind = "separator";
            sep.key = "sep:running";
            result << sep;
        }
        if (userSeps.contains(out[i].appId))
            addUserSep(out[i].appId);
    }
    if (!applets.isEmpty() && !result.isEmpty()) {
        DockItem sep;
        sep.kind = "separator";
        sep.key = "sep:applets";
        result << sep;
    }
    result << applets;

    // A user separator can land next to an automatic one; one line is enough.
    // The user's own wins, so its row keeps its identity.
    for (int i = result.size() - 1; i > 0; --i)
        if (result[i].kind == "separator" && result[i - 1].kind == "separator")
            result.removeAt(result[i].key.startsWith("sep:user:") ? i - 1 : i);
    if (!result.isEmpty() && result.last().kind == "separator")
        result.removeLast();
    return result;
}

void DockModel::rebuild() { apply(build()); }

// Moves the current rows into the target order with the fewest model
// signals, so existing delegates are kept rather than recreated.
void DockModel::apply(const QList<DockItem> &target) {
    const int before = m_items.size();
    QSet<QString> wanted;
    for (const auto &item : target)
        wanted.insert(item.key);
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (!wanted.contains(m_items[i].key)) {
            beginRemoveRows({}, i, i);
            m_items.removeAt(i);
            endRemoveRows();
        }
    }
    for (int i = 0; i < target.size(); ++i) {
        if (i < m_items.size() && m_items[i].key == target[i].key) {
            if (!(m_items[i] == target[i])) {
                m_items[i] = target[i];
                emit dataChanged(index(i), index(i));
            }
            continue;
        }
        int found = -1;
        for (int j = i + 1; j < m_items.size(); ++j)
            if (m_items[j].key == target[i].key) {
                found = j;
                break;
            }
        if (found >= 0) {
            beginMoveRows({}, found, found, {}, i);
            m_items.move(found, i);
            endMoveRows();
            m_items[i] = target[i];
            emit dataChanged(index(i), index(i));
        } else {
            beginInsertRows({}, i, i);
            m_items.insert(i, target[i]);
            endInsertRows();
        }
    }
    if (m_items.size() != before)
        emit countChanged();
}

int DockModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_items.size(); }

QHash<int, QByteArray> DockModel::roleNames() const {
    return {{KeyRole, "key"}, {KindRole, "kind"}, {AppIdRole, "appId"}, {NameRole, "name"}, {IconRole, "icon"},
            {AppletIdRole, "appletId"}, {PinnedRole, "pinned"}, {RunningRole, "running"},
            {WindowCountRole, "windowCount"}, {ActiveRole, "active"}, {UrgentRole, "urgent"},
            {LaunchingRole, "launching"}, {WindowsRole, "windows"}};
}

QVariant DockModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const DockItem &item = m_items[index.row()];
    switch (role) {
    case KeyRole: return item.key;
    case KindRole: return item.kind;
    case AppIdRole: return item.appId;
    case NameRole: return item.name;
    case IconRole: return item.icon;
    case AppletIdRole: return item.appletId;
    case PinnedRole: return item.pinned;
    case RunningRole: return !item.windows.isEmpty();
    case WindowCountRole: return item.windows.size();
    case ActiveRole: return item.active;
    case UrgentRole: return item.urgent;
    case LaunchingRole: return item.launching;
    case WindowsRole: return item.windows;
    }
    return {};
}

QVariantMap DockModel::get(int row) const {
    QVariantMap map;
    if (row < 0 || row >= m_items.size())
        return map;
    const auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); ++it)
        map.insert(QString::fromUtf8(it.value()), data(index(row), it.key()));
    return map;
}

void DockModel::activate(int row) {
    if (row < 0 || row >= m_items.size() || m_items[row].kind != "app")
        return;
    const DockItem &item = m_items[row];
    if (item.windows.isEmpty()) {
        launchNew(row);
        return;
    }
    QString focused;
    for (const auto &w : item.windows)
        if (w.toMap().value("focused").toBool())
            focused = w.toMap().value("address").toString();
    if (!focused.isEmpty()) {
        if (item.windows.size() > 1) {
            cycle(row, 1);
        } else if (m_config->get("clickActiveMinimizes").toBool() && m_hyprland) {
            m_hyprland->minimize(focused);
        }
        return;
    }
    // Prefer a window on the workspace in front of the user, then any
    // unminimized one, then a minimized one, in order of recent focus.
    const int ws = m_hyprland ? m_hyprland->activeWorkspace() : 0;
    QString pick;
    for (int pass = 0; pass < 3 && pick.isEmpty(); ++pass)
        for (const auto &w : item.windows) {
            const QVariantMap m = w.toMap();
            const bool minimized = m.value("minimized").toBool();
            if ((pass == 0 && !minimized && m.value("workspace").toInt() == ws) || (pass == 1 && !minimized) || pass == 2) {
                pick = m.value("address").toString();
                break;
            }
        }
    focusWindow(pick);
}

void DockModel::launchNew(int row) {
    if (row < 0 || row >= m_items.size())
        return;
    const DockItem &item = m_items[row];
    if (item.appId.isEmpty())
        return;
    m_apps->launch(item.appId);
    m_launching.insert(item.appId, QDateTime::currentMSecsSinceEpoch());
    m_launchTimeout.start();
    emit launched(row);
    rebuild();
}

void DockModel::cycle(int row, int direction) {
    if (row < 0 || row >= m_items.size())
        return;
    const QVariantList windows = m_items[row].windows;
    if (windows.isEmpty())
        return;
    // Windows are listed in focus order; cycle through them by address so
    // the order stays put while focus moves.
    QStringList addresses;
    for (const auto &w : windows)
        addresses << w.toMap().value("address").toString();
    std::sort(addresses.begin(), addresses.end());
    QString focused;
    for (const auto &w : windows)
        if (w.toMap().value("focused").toBool())
            focused = w.toMap().value("address").toString();
    int at = addresses.indexOf(focused);
    if (at < 0) {
        focusWindow(windows.first().toMap().value("address").toString());
        return;
    }
    at = (at + (direction >= 0 ? 1 : -1) + addresses.size()) % addresses.size();
    focusWindow(addresses[at]);
}

void DockModel::closeAll(int row) {
    if (row < 0 || row >= m_items.size() || !m_hyprland)
        return;
    for (const auto &w : m_items[row].windows)
        m_hyprland->close(w.toMap().value("address").toString());
}

void DockModel::togglePin(int row) {
    if (row < 0 || row >= m_items.size() || m_items[row].appId.isEmpty())
        return;
    const DockItem &item = m_items[row];
    if (item.pinned)
        m_config->unpin(item.appId);
    else
        m_config->pin(item.appId);
}

void DockModel::movePinned(int fromRow, int toRow) {
    if (fromRow < 0 || fromRow >= m_items.size() || toRow < 0 || toRow >= m_items.size())
        return;
    if (!m_items[fromRow].pinned)
        return;
    // Rows of pinned items map one to one onto the launcher list.
    int to = 0;
    for (int i = 0; i < m_items.size() && i <= toRow; ++i)
        if (m_items[i].pinned)
            to = i;
    const QVariantList launchers = m_config->get("launchers").toList();
    const int from = launchers.indexOf(m_items[fromRow].appId);
    int target = launchers.indexOf(m_items[qMin(toRow, to)].appId);
    if (target < 0)
        target = toRow > fromRow ? launchers.size() - 1 : 0;
    m_config->moveLauncher(from, target);
}

void DockModel::focusWindow(const QString &address) {
    if (m_hyprland && !address.isEmpty())
        m_hyprland->focus(address);
}

void DockModel::closeWindow(const QString &address) {
    if (m_hyprland && !address.isEmpty())
        m_hyprland->close(address);
}
