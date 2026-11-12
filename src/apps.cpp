#include "apps.h"

#include <QDir>
#include <QDirIterator>
#include <functional>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>

QStringList AppIndex::defaultDirectories() {
    QStringList dirs;
    const QString home = qEnvironmentVariable("XDG_DATA_HOME", QDir::homePath() + "/.local/share");
    dirs << home + "/applications";
    const QString system = qEnvironmentVariable("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
    for (const auto &dir : system.split(':', Qt::SkipEmptyParts))
        dirs << dir + "/applications";
    dirs << "/var/lib/flatpak/exports/share/applications" << QDir::homePath() + "/.local/share/flatpak/exports/share/applications";
    dirs.removeDuplicates();
    return dirs;
}

AppIndex::AppIndex(const QStringList &directories, QObject *parent)
    : QObject(parent), m_directories(directories.isEmpty() ? defaultDirectories() : directories) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(500);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &AppIndex::rescan);
    rescan();
}

AppEntry AppIndex::parseDesktopFile(const QString &path, const QString &id) {
    AppEntry entry;
    entry.id = id;
    entry.path = path;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return entry;
    QTextStream in(&file);
    bool inMain = false;
    QString type;
    bool hidden = false;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith('[')) {
            inMain = line == "[Desktop Entry]";
            continue;
        }
        if (!inMain || line.startsWith('#'))
            continue;
        const int eq = line.indexOf('=');
        if (eq <= 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (key == "Name") entry.name = value;
        else if (key == "GenericName") entry.genericName = value;
        else if (key == "Comment") entry.comment = value;
        else if (key == "Icon") entry.icon = value;
        else if (key == "Exec") entry.exec = value;
        else if (key == "StartupWMClass") entry.wmClass = value;
        else if (key == "Keywords") entry.keywords = value.split(';', Qt::SkipEmptyParts);
        else if (key == "NoDisplay") entry.noDisplay = value == "true";
        else if (key == "Hidden") hidden = value == "true";
        else if (key == "Terminal") entry.terminal = value == "true";
        else if (key == "Type") type = value;
    }
    if (hidden || (!type.isEmpty() && type != "Application"))
        entry.noDisplay = true, entry.name.clear();
    if (entry.name.isEmpty())
        entry.name = id;
    return entry;
}

QString AppIndex::stripFieldCodes(const QString &exec) {
    QString out = exec;
    out.replace(QRegularExpression("%[fFuUdDnNickvm]"), "");
    out.replace("%%", "%");
    return out.simplified();
}

void AppIndex::rescan() {
    QHash<QString, AppEntry> entries;
    const auto watched = m_watcher.directories();
    if (!watched.isEmpty())
        m_watcher.removePaths(watched);
    // Earlier directories win, so a user's own copy overrides the system one.
    for (const auto &dir : m_directories) {
        if (!QFileInfo::exists(dir))
            continue;
        m_watcher.addPath(dir);
        QDirIterator it(dir, {"*.desktop"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QString id = QDir(dir).relativeFilePath(path);
            id.chop(8);
            id.replace('/', '-');
            if (entries.contains(id))
                continue;
            entries.insert(id, parseDesktopFile(path, id));
        }
    }
    m_entries = entries;
    emit changed();
}

const AppEntry *AppIndex::find(const QString &id) const {
    auto it = m_entries.constFind(id);
    return it == m_entries.constEnd() ? nullptr : &it.value();
}

QVariantList AppIndex::apps() const {
    QList<const AppEntry *> list;
    for (const auto &entry : m_entries)
        if (!entry.noDisplay)
            list << &entry;
    std::sort(list.begin(), list.end(), [](const AppEntry *a, const AppEntry *b) {
        return a->name.compare(b->name, Qt::CaseInsensitive) < 0;
    });
    QVariantList out;
    for (const auto *entry : list)
        out << QVariantMap{{"id", entry->id}, {"name", entry->name}, {"icon", entry->icon},
                           {"comment", entry->comment.isEmpty() ? entry->genericName : entry->comment}};
    return out;
}

QVariantMap AppIndex::entry(const QString &id) const {
    const AppEntry *entry = find(id);
    if (!entry)
        return {};
    return {{"id", entry->id}, {"name", entry->name}, {"icon", entry->icon}, {"exec", entry->exec},
            {"comment", entry->comment}, {"wmClass", entry->wmClass}};
}

QStringList AppIndex::search(const QString &text) const {
    const QString needle = text.trimmed().toLower();
    QStringList out;
    for (const auto &entry : m_entries) {
        if (entry.noDisplay)
            continue;
        if (needle.isEmpty() || entry.name.toLower().contains(needle) || entry.id.toLower().contains(needle)
            || entry.keywords.join(' ').toLower().contains(needle) || entry.genericName.toLower().contains(needle))
            out << entry.id;
    }
    std::sort(out.begin(), out.end(), [this](const QString &a, const QString &b) {
        return m_entries[a].name.compare(m_entries[b].name, Qt::CaseInsensitive) < 0;
    });
    return out;
}

// Windows report a class that is usually, but not always, their desktop file
// id; try the reliable signals first and the loose ones last.
QString AppIndex::matchClass(const QString &windowClass) const {
    const QString cls = windowClass.trimmed().toLower();
    if (cls.isEmpty())
        return {};
    auto lastSegment = [](const QString &id) { return id.section('.', -1).toLower(); };
    auto execName = [](const AppEntry &e) {
        QString first = stripFieldCodes(e.exec).section(' ', 0, 0);
        if (first == "env" || first.contains('='))
            first = stripFieldCodes(e.exec).section(' ', 1, 1);
        return QFileInfo(first).fileName().toLower();
    };
    QList<std::function<bool(const AppEntry &)>> rules{
        [&](const AppEntry &e) { return !e.wmClass.isEmpty() && e.wmClass.toLower() == cls; },
        [&](const AppEntry &e) { return e.id.toLower() == cls; },
        [&](const AppEntry &e) { return lastSegment(e.id) == cls; },
        [&](const AppEntry &e) { return lastSegment(e.id) == lastSegment(cls); },
        [&](const AppEntry &e) { return !e.exec.isEmpty() && execName(e) == cls; },
        [&](const AppEntry &e) { return QString(e.name).toLower().replace(' ', '-') == cls; },
        [&](const AppEntry &e) { return e.name.toLower() == cls; },
    };
    for (const auto &rule : rules) {
        // Visible entries beat hidden helpers with the same class.
        QString hiddenMatch;
        QStringList matches;
        for (const auto &entry : m_entries)
            if (rule(entry)) {
                if (entry.noDisplay)
                    hiddenMatch = entry.id;
                else
                    matches << entry.id;
            }
        if (!matches.isEmpty()) {
            std::sort(matches.begin(), matches.end(), [](const QString &a, const QString &b) {
                return a.size() != b.size() ? a.size() < b.size() : a < b;
            });
            return matches.first();
        }
        if (!hiddenMatch.isEmpty())
            return hiddenMatch;
    }
    return {};
}

void AppIndex::launch(const QString &id) const {
    const AppEntry *entry = find(id);
    if (!entry)
        return;
    if (!QStandardPaths::findExecutable("uwsm-app").isEmpty()) {
        QProcess::startDetached("uwsm-app", {"--", id + ".desktop"});
        return;
    }
    if (!QStandardPaths::findExecutable("gtk-launch").isEmpty()) {
        QProcess::startDetached("gtk-launch", {id});
        return;
    }
    const QString command = stripFieldCodes(entry->exec);
    if (!command.isEmpty())
        QProcess::startDetached("/bin/sh", {"-c", command});
}
