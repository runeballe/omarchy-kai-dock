#include "hyprland.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
const QString kMinimized = QStringLiteral("special:minimized");

QString luaString(const QString &value) {
    QString escaped = value;
    escaped.replace('\\', "\\\\").replace('"', "\\\"");
    return '"' + escaped + '"';
}
} // namespace

Hyprland::Hyprland(QObject *parent) : QObject(parent) {
    const QString signature = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
    const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (!signature.isEmpty() && !runtime.isEmpty() && QFileInfo::exists(runtime + "/hypr/" + signature + "/.socket.sock"))
        m_directory = runtime + "/hypr/" + signature;

    m_refresh.setSingleShot(true);
    m_refresh.setInterval(40);
    connect(&m_refresh, &QTimer::timeout, this, &Hyprland::refresh);
    m_reconnect.setSingleShot(true);
    m_reconnect.setInterval(2000);
    connect(&m_reconnect, &QTimer::timeout, this, &Hyprland::connectEvents);

    connect(&m_events, &QLocalSocket::readyRead, this, [this] {
        m_buffer += m_events.readAll();
        int newline;
        while ((newline = m_buffer.indexOf('\n')) >= 0) {
            handleEvent(m_buffer.left(newline));
            m_buffer.remove(0, newline + 1);
        }
    });
    connect(&m_events, &QLocalSocket::disconnected, this, [this] { m_reconnect.start(); });
    connect(&m_events, &QLocalSocket::errorOccurred, this, [this] { m_reconnect.start(); });

    if (available()) {
        connectEvents();
        refresh();
    }
}

void Hyprland::connectEvents() {
    if (!available())
        return;
    m_events.abort();
    m_events.connectToServer(m_directory + "/.socket2.sock");
}

void Hyprland::handleEvent(const QByteArray &line) {
    const int split = line.indexOf(">>");
    if (split < 0)
        return;
    const QByteArray name = line.left(split);
    const QString data = QString::fromUtf8(line.mid(split + 2));
    if (name == "urgent") {
        m_urgent.insert(normaliseAddress(data));
    } else if (name == "activewindowv2") {
        m_urgent.remove(normaliseAddress(data));
    } else if (name == "openwindow") {
        emit windowOpened(data.section(',', 2, 2));
    } else if (name == "windowtitle" || name == "windowtitlev2") {
        // Titles change constantly in browsers; a slower refresh is plenty.
        if (!m_refresh.isActive()) {
            m_refresh.setInterval(250);
            m_refresh.start();
        }
        return;
    }
    static const QSet<QByteArray> relevant{"openwindow", "closewindow", "movewindow", "movewindowv2",
                                           "activewindowv2", "urgent", "workspace", "workspacev2",
                                           "focusedmon", "focusedmonv2", "changefloatingmode",
                                           "minimized", "destroyworkspace", "moveworkspace"};
    if (relevant.contains(name)) {
        m_refresh.setInterval(40);
        m_refresh.start();
    }
}

QString Hyprland::normaliseAddress(const QString &address) {
    QString a = address.trimmed().toLower();
    if (!a.startsWith("0x"))
        a.prepend("0x");
    return a;
}

QString Hyprland::request(const QString &command) const {
    if (!available())
        return {};
    QLocalSocket socket;
    socket.connectToServer(m_directory + "/.socket.sock");
    if (!socket.waitForConnected(300))
        return {};
    socket.write(command.toUtf8());
    socket.flush();
    QByteArray reply;
    while (socket.waitForReadyRead(400))
        reply += socket.readAll();
    reply += socket.readAll();
    return QString::fromUtf8(reply);
}

QList<HyprClient> Hyprland::parseClients(const QByteArray &json, const QSet<QString> &urgent) {
    QList<HyprClient> out;
    for (const auto &value : QJsonDocument::fromJson(json).array()) {
        const QJsonObject o = value.toObject();
        if (!o.value("mapped").toBool(true))
            continue;
        HyprClient c;
        c.address = normaliseAddress(o.value("address").toString());
        c.windowClass = o.value("class").toString();
        if (c.windowClass.isEmpty())
            c.windowClass = o.value("initialClass").toString();
        c.title = o.value("title").toString();
        const QJsonObject ws = o.value("workspace").toObject();
        c.workspaceId = ws.value("id").toInt();
        c.workspaceName = ws.value("name").toString();
        c.minimized = c.workspaceName == kMinimized || o.value("minimized").toBool(false);
        // Other special workspaces (scratchpads) are the user's hiding places,
        // not windows to show in the dock.
        if (!c.minimized && c.workspaceName.startsWith("special:"))
            continue;
        if (c.windowClass.isEmpty())
            continue;
        c.focusHistory = o.value("focusHistoryID").toInt(99);
        c.pid = o.value("pid").toInteger();
        c.monitor = QString::number(o.value("monitor").toInt());
        c.urgent = urgent.contains(c.address);
        out << c;
    }
    return out;
}

int Hyprland::parseActiveWorkspace(const QByteArray &monitorsJson) {
    for (const auto &value : QJsonDocument::fromJson(monitorsJson).array()) {
        const QJsonObject o = value.toObject();
        if (o.value("focused").toBool())
            return o.value("activeWorkspace").toObject().value("id").toInt(1);
    }
    return 1;
}

void Hyprland::refresh() {
    if (!available())
        return;
    const QByteArray json = request("j/clients").toUtf8();
    if (json.isEmpty())
        return;
    m_activeWorkspace = parseActiveWorkspace(request("j/monitors").toUtf8());
    m_clients = parseClients(json, m_urgent);
    emit clientsChanged();
}

QVariantList Hyprland::clientsVariant() const {
    QVariantList out;
    for (const auto &c : m_clients)
        out << QVariantMap{{"address", c.address}, {"class", c.windowClass}, {"title", c.title},
                           {"workspace", c.workspaceId}, {"minimized", c.minimized},
                           {"focused", c.focusHistory == 0}, {"urgent", c.urgent}};
    return out;
}

void Hyprland::dispatch(const QString &lua) const { request("dispatch " + lua); }

void Hyprland::focus(const QString &rawAddress) {
    const QString address = normaliseAddress(rawAddress);
    const QString target = "window = " + luaString("address:" + address);
    for (const auto &c : m_clients) {
        if (c.address == address && c.minimized) {
            // A minimized window comes back to the workspace in front of the user.
            const int workspace = parseActiveWorkspace(request("j/monitors").toUtf8());
            dispatch(QString("hl.dsp.window.move({ workspace = \"%1\", follow = false, %2 })").arg(workspace).arg(target));
        }
    }
    dispatch("hl.dsp.focus({ " + target + " })");
    dispatch("hl.dsp.window.alter_zorder({ mode = \"top\", " + target + " })");
    m_urgent.remove(address);
    m_refresh.start();
}

void Hyprland::close(const QString &address) {
    dispatch("hl.dsp.window.close({ window = " + luaString("address:" + normaliseAddress(address)) + " })");
}

void Hyprland::minimize(const QString &address) {
    dispatch("hl.dsp.window.move({ workspace = \"" + kMinimized + "\", follow = false, window = "
             + luaString("address:" + normaliseAddress(address)) + " })");
    m_refresh.start();
}
