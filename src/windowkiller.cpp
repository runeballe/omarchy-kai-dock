#include "windowkiller.h"

#include "hyprland.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

#include <csignal>
#include <unistd.h>

WindowKiller::WindowKiller(Hyprland *hyprland, QObject *parent) : QObject(parent), m_hyprland(hyprland) {
    connect(&m_process, &QProcess::stateChanged, this, &WindowKiller::pickingChanged);
    connect(&m_process, &QProcess::finished, this, &WindowKiller::finished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_status = tr("WindowsKiller needs slurp to pick a window.");
            emit lastChanged();
            notify(tr("WindowsKiller"), m_status, true);
        }
    });
}

bool WindowKiller::available() const {
    return m_hyprland && m_hyprland->available() && !QStandardPaths::findExecutable("slurp").isEmpty();
}

void WindowKiller::pick(const QString &mode) {
    if (picking()) {
        cancel();
        return;
    }
    if (!m_hyprland || !m_hyprland->available()) {
        m_status = tr("WindowsKiller only works on Hyprland.");
        emit lastChanged();
        notify(tr("WindowsKiller"), m_status, true);
        return;
    }
    m_mode = mode == "info" ? "info" : "kill";
    // One click picks a point; Escape cancels.
    m_process.start("slurp", {"-p", "-f", "%x %y", "-b", "#00000040", "-s", "#ff4d4f60"});
}

void WindowKiller::cancel() {
    if (picking())
        m_process.kill();
}

void WindowKiller::finished() {
    const QString out = QString::fromUtf8(m_process.readAllStandardOutput()).trimmed();
    if (m_process.exitStatus() != QProcess::NormalExit || m_process.exitCode() != 0 || out.isEmpty())
        return;  // cancelled
    const QStringList parts = out.split(' ');
    if (parts.size() != 2)
        return;
    const QVariantMap client = clientAt(m_hyprland->request("j/clients").toUtf8(), m_hyprland->request("j/monitors").toUtf8(),
                                        m_hyprland->request("j/layers").toUtf8(), parts[0].toDouble(), parts[1].toDouble());
    if (client.isEmpty())
        return;  // the dock or an empty spot: cancelled, like Cairo-Dock's applet

    QVariantMap info = client;
    const QVariantMap proc = processInfo(client.value("pid").toLongLong());
    for (auto it = proc.begin(); it != proc.end(); ++it)
        info.insert(it.key(), it.value());
    m_last = info;

    const QString name = info.value("class").toString();
    const qint64 pid = info.value("pid").toLongLong();
    if (m_mode == "info") {
        m_status = tr("%1 (pid %2)").arg(name).arg(pid);
        emit lastChanged();
        QStringList lines{
            tr("PID: %1").arg(pid),
            tr("Command: %1").arg(info.value("command").toString()),
            tr("Uptime: %1").arg(formatUptime(info.value("uptime").toLongLong())),
            tr("Class: %1").arg(name),
        };
        if (info.value("initialClass").toString() != name)
            lines << tr("Initial class: %1").arg(info.value("initialClass").toString());
        notify(info.value("title").toString().isEmpty() ? name : info.value("title").toString(), lines.join('\n'));
        return;
    }

    if (pid <= 0 || isProtected(pid, info.value("exe").toString())) {
        m_status = tr("Won't kill %1: it keeps the desktop running.").arg(name);
        emit lastChanged();
        notify(tr("WindowsKiller"), m_status, true);
        return;
    }
    // Ask nicely first; whatever is still alive a moment later is forced.
    ::kill(pid_t(pid), SIGTERM);
    QTimer::singleShot(2000, this, [pid] {
        if (::kill(pid_t(pid), 0) == 0)
            ::kill(pid_t(pid), SIGKILL);
    });
    m_status = tr("Killed %1 (pid %2).").arg(name).arg(pid);
    emit lastChanged();
}

void WindowKiller::notify(const QString &summary, const QString &body, bool critical) {
    if (QStandardPaths::findExecutable("notify-send").isEmpty())
        return;
    QStringList args{"-a", "kai", "-i", "xkill"};
    if (critical)
        args << "-u" << "critical";
    args << summary << body;
    QProcess::startDetached("notify-send", args);
}

QVariantMap WindowKiller::clientAt(const QByteArray &clientsJson, const QByteArray &monitorsJson,
                                   const QByteArray &layersJson, double x, double y) {
    // The dock is a layer surface; a click on it means "never mind".
    const QJsonObject layers = QJsonDocument::fromJson(layersJson).object();
    for (const auto &monitor : layers) {
        const QJsonObject levels = monitor.toObject().value("levels").toObject();
        for (const auto &level : levels)
            for (const auto &value : level.toArray()) {
                const QJsonObject l = value.toObject();
                if (!l.value("namespace").toString().startsWith("kai"))
                    continue;
                const double lx = l.value("x").toDouble(), ly = l.value("y").toDouble();
                if (x >= lx && x < lx + l.value("w").toDouble() && y >= ly && y < ly + l.value("h").toDouble())
                    return {};
            }
    }

    QSet<int> visible;
    for (const auto &value : QJsonDocument::fromJson(monitorsJson).array()) {
        const QJsonObject m = value.toObject();
        visible.insert(m.value("activeWorkspace").toObject().value("id").toInt());
        const int special = m.value("specialWorkspace").toObject().value("id").toInt();
        if (special != 0)
            visible.insert(special);
    }

    QJsonObject best;
    int bestRank = INT_MAX;
    for (const auto &value : QJsonDocument::fromJson(clientsJson).array()) {
        const QJsonObject c = value.toObject();
        if (!c.value("mapped").toBool(true) || c.value("hidden").toBool(false))
            continue;
        if (!visible.contains(c.value("workspace").toObject().value("id").toInt()))
            continue;
        const QJsonArray at = c.value("at").toArray(), size = c.value("size").toArray();
        if (at.size() < 2 || size.size() < 2)
            continue;
        const double cx = at[0].toDouble(), cy = at[1].toDouble();
        if (x < cx || y < cy || x >= cx + size[0].toDouble() || y >= cy + size[1].toDouble())
            continue;
        // Fullscreen and floating windows sit above tiled ones; among equals
        // the most recently focused is on top.
        const int layer = c.value("fullscreen").toInt(0) ? 0 : c.value("floating").toBool() ? 1 : 2;
        const int rank = layer * 1000 + c.value("focusHistoryID").toInt(999);
        if (rank < bestRank) {
            bestRank = rank;
            best = c;
        }
    }
    if (best.isEmpty())
        return {};
    return {{"address", best.value("address").toString()}, {"class", best.value("class").toString()},
            {"initialClass", best.value("initialClass").toString()}, {"title", best.value("title").toString()},
            {"pid", best.value("pid").toInteger()}};
}

QVariantMap WindowKiller::processInfo(qint64 pid, const QString &procRoot) {
    QVariantMap info;
    if (pid <= 0)
        return info;
    const QString dir = procRoot + "/" + QString::number(pid);
    QFile cmdline(dir + "/cmdline");
    if (cmdline.open(QIODevice::ReadOnly)) {
        QByteArray raw = cmdline.readAll();
        while (raw.endsWith('\0'))
            raw.chop(1);
        info.insert("command", QString::fromUtf8(raw.replace('\0', ' ')));
    }
    info.insert("exe", QFileInfo(dir + "/exe").symLinkTarget().section('/', -1));

    // Field 22 of /proc/<pid>/stat is the start time in clock ticks since
    // boot; the command name in field 2 may contain spaces, so count from ')'.
    QFile stat(dir + "/stat");
    QFile uptime(procRoot + "/uptime");
    if (stat.open(QIODevice::ReadOnly) && uptime.open(QIODevice::ReadOnly)) {
        const QByteArray s = stat.readAll();
        const QList<QByteArray> fields = s.mid(s.lastIndexOf(')') + 2).split(' ');
        const double ticks = sysconf(_SC_CLK_TCK);
        if (fields.size() > 19 && ticks > 0) {
            const double started = fields[19].toDouble() / ticks;
            const double now = uptime.readAll().split(' ').value(0).toDouble();
            info.insert("uptime", qint64(qMax(0.0, now - started)));
        }
    }
    return info;
}

QString WindowKiller::formatUptime(qint64 seconds) {
    const qint64 d = seconds / 86400, h = seconds % 86400 / 3600, m = seconds % 3600 / 60, s = seconds % 60;
    if (d > 0)
        return QString("%1d %2h %3m").arg(d).arg(h).arg(m);
    if (h > 0)
        return QString("%1h %2m").arg(h).arg(m);
    if (m > 0)
        return QString("%1m %2s").arg(m).arg(s);
    return QString("%1s").arg(s);
}

bool WindowKiller::isProtected(qint64 pid, const QString &exe) {
    if (pid <= 1 || pid == QCoreApplication::applicationPid())
        return true;
    const QString name = exe.toLower();
    return name == "hyprland" || name == "aquamarine" || name == "kai";
}
