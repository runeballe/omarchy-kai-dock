#include "scriptapplets.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

QString ScriptApplets::defaultDirectory() {
    const QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
    return (xdg.isEmpty() ? QDir::homePath() + "/.config" : xdg) + "/kai/applets";
}

ScriptApplets::ScriptApplets(const QString &directory, QObject *parent)
    : QObject(parent), m_directory(directory.isEmpty() ? defaultDirectory() : directory) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(400);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &ScriptApplets::rescan);
    rescan();
}

ScriptApplets::~ScriptApplets() {
    for (QProcess *p : std::as_const(m_running))
        if (p->state() != QProcess::NotRunning)
            p->kill();
}

QVariantMap ScriptApplets::readManifest(const QString &folder) {
    QFile file(folder + "/applet.json");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(file.read(64 * 1024));
    if (!doc.isObject())
        return {};
    QVariantMap m = doc.object().toVariantMap();
    if (m.value("name").toString().trimmed().isEmpty())
        m.insert("name", QFileInfo(folder).fileName());
    m.insert("dir", folder);
    return m;
}

QVariantMap ScriptApplets::parseOutput(const QByteArray &raw) {
    const QByteArray output = raw.trimmed();
    const QJsonDocument doc = QJsonDocument::fromJson(output);
    if (doc.isObject()) {
        const QJsonObject o = doc.object();
        QVariantMap state{{"text", o.value("text").toVariant().toString()},
                          {"tooltip", o.value("tooltip").toString()},
                          {"icon", o.value("icon").toString()},
                          {"progress", -1.0}};
        // Progress may be given as 0..1 or as a percentage.
        if (o.contains("progress") && !o.value("progress").isNull()) {
            double p = o.value("progress").toDouble(-1);
            if (p > 1)
                p /= 100.0;
            state.insert("progress", p < 0 ? -1.0 : qMin(p, 1.0));
        }
        return state;
    }
    // Plain text: the first line is the quick-info, all of it the tooltip.
    const QString text = QString::fromUtf8(output);
    return {{"text", text.section('\n', 0, 0).left(12)}, {"tooltip", text}, {"icon", QString()}, {"progress", -1.0}};
}

QString ScriptApplets::actionKey(const QString &action) {
    if (action == "click") return "on_click";
    if (action == "middle") return "on_middle_click";
    if (action == "double") return "on_double_click";
    if (action == "scroll-up") return "on_scroll_up";
    if (action == "scroll-down") return "on_scroll_down";
    return {};
}

QStringList ScriptApplets::ids() const {
    QStringList list = m_manifests.keys();
    list.sort();
    return list;
}

QVariantList ScriptApplets::applets() const {
    QVariantList out;
    for (const QString &id : ids()) {
        QVariantMap m = m_manifests.value(id);
        m.insert("id", id);
        out << m;
    }
    return out;
}

void ScriptApplets::rescan() {
    const auto watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty())
        m_watcher.removePaths(watched);
    QDir().mkpath(m_directory);
    m_watcher.addPath(m_directory);

    QHash<QString, QVariantMap> found;
    for (const QFileInfo &entry : QDir(m_directory).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QVariantMap manifest = readManifest(entry.absoluteFilePath());
        if (manifest.isEmpty())
            continue;
        found.insert("script:" + entry.fileName(), manifest);
        m_watcher.addPath(entry.absoluteFilePath());
        m_watcher.addPath(entry.absoluteFilePath() + "/applet.json");
    }
    if (found == m_manifests)
        return;
    m_manifests = found;
    for (auto it = m_states.begin(); it != m_states.end();)
        it = m_manifests.contains(it.key()) ? std::next(it) : m_states.erase(it);
    schedule();
    emit appletsChanged();
    emit statesChanged();
}

void ScriptApplets::schedule() {
    qDeleteAll(m_timers);
    m_timers.clear();
    for (const QString &id : ids()) {
        const QVariantMap m = m_manifests.value(id);
        if (m.value("exec").toString().trimmed().isEmpty())
            continue;
        const int seconds = qMax(1, m.value("interval", 60).toInt());
        auto *timer = new QTimer(this);
        timer->setInterval(seconds * 1000);
        connect(timer, &QTimer::timeout, this, [this, id] { refresh(id); });
        timer->start();
        m_timers.insert(id, timer);
        refresh(id);
    }
}

void ScriptApplets::refresh(const QString &id) {
    const QVariantMap m = m_manifests.value(id);
    const QString exec = m.value("exec").toString();
    if (exec.trimmed().isEmpty())
        return;
    // A slow script is never started twice at once.
    if (m_running.contains(id) && m_running[id]->state() != QProcess::NotRunning)
        return;
    QProcess *p = m_running.value(id);
    if (!p) {
        p = new QProcess(this);
        m_running.insert(id, p);
        connect(p, &QProcess::finished, this, [this, id, p](int code, QProcess::ExitStatus status) {
            QVariantMap state = parseOutput(p->readAllStandardOutput());
            if (status != QProcess::NormalExit || code != 0) {
                const QString err = QString::fromUtf8(p->readAllStandardError()).trimmed();
                state.insert("error", err.isEmpty() ? tr("exited with code %1").arg(code) : err.left(200));
            }
            m_states.insert(id, state);
            emit statesChanged();
        });
        connect(p, &QProcess::errorOccurred, this, [this, id](QProcess::ProcessError) {
            m_states.insert(id, QVariantMap{{"text", "!"}, {"error", tr("could not run the script")}, {"progress", -1.0}});
            emit statesChanged();
        });
    }
    p->setWorkingDirectory(m.value("dir").toString());
    // A hung script is stopped after a while; the run number makes sure the
    // timer only ever stops the run it was started for.
    const quint64 run = ++m_runs[id];
    p->start("/bin/sh", {"-c", exec});
    const int seconds = qMax(1, m.value("interval", 60).toInt());
    QTimer::singleShot(qBound(1, seconds, 30) * 1000 - 100, p, [this, id, p, run] {
        if (m_runs.value(id) == run && p->state() != QProcess::NotRunning)
            p->kill();
    });
}

bool ScriptApplets::hasAction(const QString &id, const QString &action) const {
    return !m_manifests.value(id).value(actionKey(action)).toString().trimmed().isEmpty();
}

bool ScriptApplets::trigger(const QString &id, const QString &action) {
    const QVariantMap m = m_manifests.value(id);
    const QString command = m.value(actionKey(action)).toString();
    if (command.trimmed().isEmpty())
        return false;
    auto *p = new QProcess(this);
    p->setWorkingDirectory(m.value("dir").toString());
    // Refresh straight after an action so the icon shows its effect.
    connect(p, &QProcess::finished, this, [this, id, p] {
        p->deleteLater();
        refresh(id);
    });
    connect(p, &QProcess::errorOccurred, p, &QObject::deleteLater);
    p->start("/bin/sh", {"-c", command});
    return true;
}
