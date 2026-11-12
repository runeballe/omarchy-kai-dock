#include "config.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJSValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUrl>

namespace {

QVariantMap readJson(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
}

bool writeJsonAtomically(const QString &path, const QVariantMap &map) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Indented));
    return file.commit();
}

} // namespace

QString Config::defaultConfigDirectory() {
    const QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
    return (xdg.isEmpty() ? QDir::homePath() + "/.config" : xdg) + "/kai";
}

QString Config::defaultDataDirectory() {
    const QString xdg = qEnvironmentVariable("XDG_DATA_HOME");
    return (xdg.isEmpty() ? QDir::homePath() + "/.local/share" : xdg) + "/kai";
}

QStringList Config::knownApplets() {
    return {"apps", "clock", "weather", "sysmon", "battery", "trash"};
}

QVariantMap Config::defaults() {
    QVariantList applets;
    const QStringList on{"apps", "clock", "sysmon", "weather"};
    for (const auto &id : knownApplets())
        applets << QVariantMap{{"id", id}, {"enabled", on.contains(id)}, {"detached", false},
                               {"x", 80}, {"y", 80}};
    return {
        {"edge", "bottom"},
        {"screen", ""},
        {"view", ""},              // empty: the theme decides
        {"iconSize", 48},
        {"zoom", 1.8},
        {"zoomRange", 3.0},
        {"spacing", -1},           // -1: the theme decides
        {"margin", 8},
        {"layer", "top"},
        {"autohide", false},
        {"autohideDelay", 600},
        {"reserveSpace", false},
        {"renderer", "opengl"},
        {"showLabels", true},
        {"showRunning", true},
        {"currentWorkspaceOnly", false},
        {"clickActiveMinimizes", true},
        {"clickAnimation", "bounce"},
        {"launchAnimation", "bounce"},
        {"attentionAnimation", "wobble"},
        {"animationSpeed", 1.0},
        {"deskletLayer", "bottom"},
        {"clock24h", true},
        {"clockAnalog", true},
        {"clockSeconds", true},
        {"weatherCity", ""},
        {"weatherUnits", "metric"},
        {"theme", "omarchy"},
        {"themeOverrides", QVariantMap{}},
        {"launchers", QVariantList{}},
        {"launchersSeeded", false},
        {"applets", applets},
    };
}

// Every key a theme may set, with the value used when a theme leaves it out.
// "accent" as a color means the Omarchy accent; a radius of -1 means
// Hyprland's window rounding.
QVariantMap Config::themeDefaults() {
    return {
        {"name", "Untitled"},
        {"author", ""},
        {"description", ""},
        {"view", "floating"},           // floating | panel | plane
        {"background", "#16161a"},
        {"background2", ""},            // optional second gradient stop
        {"backgroundOpacity", 0.82},
        {"border", "#ffffff"},
        {"borderOpacity", 0.08},
        {"borderWidth", 1},
        {"radius", -1},
        {"padding", 8},
        {"spacing", 6},
        {"gloss", 0.0},                 // highlight over the top half
        {"reflection", 0.0},            // mirrored icons under the dock
        {"indicator", "dot"},           // dot | bar | glow | none
        {"indicatorColor", "accent"},
        {"activeColor", "accent"},
        {"labelBackground", "#101014"},
        {"labelColor", "#ffffff"},
        {"labelOpacity", 0.92},
        {"planeAngle", 58},
        {"separator", "line"},          // line | gap
        {"iconShadow", 0.0},
        {"deskletBackground", ""},      // empty: same as the dock
        {"deskletOpacity", -1},
        {"textColor", "#f2f2f2"},
        {"accentOverride", ""},         // a fixed accent instead of Omarchy's
    };
}

QString Config::slugify(const QString &name) {
    QString slug = name.toLower().trimmed();
    slug.replace(QRegularExpression("[^a-z0-9]+"), "-");
    slug.remove(QRegularExpression("^-+|-+$"));
    return slug.isEmpty() ? "theme" : slug;
}

QString Config::readRenderer(const QString &configDirectory) {
    const QString dir = configDirectory.isEmpty() ? defaultConfigDirectory() : configDirectory;
    const QString renderer = readJson(dir + "/config.json").value("renderer").toString();
    return renderer == "software" ? "software" : "opengl";
}

Config::Config(const QString &configDirectory, const QString &dataDirectory, const QString &builtinThemes,
               QObject *parent)
    : QObject(parent),
      m_configDirectory(configDirectory.isEmpty() ? defaultConfigDirectory() : configDirectory),
      m_dataDirectory(dataDirectory.isEmpty() ? defaultDataDirectory() : dataDirectory),
      m_builtinThemes(builtinThemes) {
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(300);
    connect(&m_saveTimer, &QTimer::timeout, this, &Config::saveNow);
    load();
    loadThemes();
    rebuildTheme();
}

Config::~Config() {
    if (m_saveTimer.isActive())
        saveNow();
}

QString Config::configPath() const { return m_configDirectory + "/config.json"; }

void Config::load() {
    m_values = defaults();
    const QVariantMap stored = readJson(configPath());
    for (auto it = stored.begin(); it != stored.end(); ++it)
        m_values.insert(it.key(), it.value());
    ensureApplets();
}

// Keeps the applet list complete when a new version adds one, without
// disturbing the user's order.
void Config::ensureApplets() {
    QVariantList list = m_values.value("applets").toList();
    QStringList seen;
    QVariantList cleaned;
    for (const auto &entry : list) {
        const QVariantMap map = entry.toMap();
        const QString id = map.value("id").toString();
        if (!knownApplets().contains(id) || seen.contains(id))
            continue;
        seen << id;
        cleaned << map;
    }
    for (const auto &id : knownApplets())
        if (!seen.contains(id))
            cleaned << QVariantMap{{"id", id}, {"enabled", false}, {"detached", false}, {"x", 80}, {"y", 80}};
    m_values.insert("applets", cleaned);
}

void Config::scheduleSave() { m_saveTimer.start(); }

void Config::saveNow() {
    m_saveTimer.stop();
    writeJsonAtomically(configPath(), m_values);
}

QVariant Config::get(const QString &key, const QVariant &fallback) const {
    return m_values.value(key, fallback.isValid() ? fallback : defaults().value(key));
}

void Config::set(const QString &key, const QVariant &value) {
    // Values from QML arrive as QJSValue-backed variants; normalise them so
    // comparisons and the JSON writer see plain types.
    QVariant plain = value;
    if (plain.metaType() == QMetaType::fromType<QJSValue>())
        plain = qvariant_cast<QJSValue>(plain).toVariant();
    if (m_values.value(key) == plain)
        return;
    m_values.insert(key, plain);
    if (key == "applets")
        ensureApplets();
    scheduleSave();
    emit changed();
    if (key == "theme" || key == "themeOverrides")
        rebuildTheme();
}

void Config::resetAll() {
    const QVariantList launchers = m_values.value("launchers").toList();
    m_values = defaults();
    m_values.insert("launchers", launchers);
    m_values.insert("launchersSeeded", true);
    scheduleSave();
    emit changed();
    rebuildTheme();
}

// Themes ---------------------------------------------------------------------

void Config::loadThemes() {
    QVariantList list;
    auto add = [&](const QString &path, bool builtin) {
        QVariantMap data = readJson(path);
        if (data.isEmpty())
            return;
        const QString id = QFileInfo(path).completeBaseName();
        list << QVariantMap{{"id", id}, {"name", data.value("name", id)}, {"author", data.value("author")},
                            {"description", data.value("description")}, {"builtin", builtin}, {"data", data}};
    };
    QStringList builtinIds;
    QDirIterator builtins(m_builtinThemes, {"*.json"}, QDir::Files);
    QStringList builtinPaths;
    while (builtins.hasNext())
        builtinPaths << builtins.next();
    builtinPaths.sort();
    // Omarchy's own look comes first, as the default.
    std::stable_sort(builtinPaths.begin(), builtinPaths.end(), [](const QString &a, const QString &b) {
        return a.endsWith("/omarchy.json") && !b.endsWith("/omarchy.json");
    });
    for (const auto &path : builtinPaths) {
        add(path, true);
        builtinIds << QFileInfo(path).completeBaseName();
    }
    QDirIterator user(themesPath(), {"*.json"}, QDir::Files);
    QStringList userPaths;
    while (user.hasNext())
        userPaths << user.next();
    userPaths.sort();
    for (const auto &path : userPaths)
        if (!builtinIds.contains(QFileInfo(path).completeBaseName()))
            add(path, false);
    m_themes = list;
    emit themesChanged();
}

QVariantMap Config::themeData(const QString &id) const {
    for (const auto &entry : m_themes) {
        const QVariantMap map = entry.toMap();
        if (map.value("id").toString() == id)
            return map.value("data").toMap();
    }
    return {};
}

void Config::rebuildTheme() {
    QVariantMap theme = themeDefaults();
    QVariantMap base = themeData(m_values.value("theme").toString());
    if (base.isEmpty())
        base = themeData("omarchy");
    for (auto it = base.begin(); it != base.end(); ++it)
        theme.insert(it.key(), it.value());
    const QVariantMap overrides = m_values.value("themeOverrides").toMap();
    for (auto it = overrides.begin(); it != overrides.end(); ++it)
        theme.insert(it.key(), it.value());
    if (theme != m_theme) {
        m_theme = theme;
        emit themeChanged();
    }
}

void Config::applyTheme(const QString &id) {
    if (themeData(id).isEmpty())
        return;
    m_values.insert("theme", id);
    m_values.insert("themeOverrides", QVariantMap{});
    // A theme's preferred view only wins when the user hasn't fixed one.
    scheduleSave();
    emit changed();
    rebuildTheme();
}

void Config::setThemeOverride(const QString &key, const QVariant &value) {
    QVariantMap overrides = m_values.value("themeOverrides").toMap();
    overrides.insert(key, value);
    set("themeOverrides", overrides);
}

void Config::resetThemeOverrides() { set("themeOverrides", QVariantMap{}); }

QString Config::saveThemeAs(const QString &name) {
    QVariantMap data = m_theme;
    data.insert("name", name.trimmed().isEmpty() ? QStringLiteral("My theme") : name.trimmed());
    QString id = slugify(data.value("name").toString());
    QString unique = id;
    for (int n = 2; !themeData(unique).isEmpty() && themeData(unique).value("name") != data.value("name"); ++n)
        unique = QString("%1-%2").arg(id).arg(n);
    if (!writeJsonAtomically(themesPath() + "/" + unique + ".json", data))
        return {};
    loadThemes();
    applyTheme(unique);
    return unique;
}

bool Config::deleteTheme(const QString &id) {
    const QString path = themesPath() + "/" + id + ".json";
    if (!QFile::exists(path) || !QFile::remove(path))
        return false;
    loadThemes();
    if (m_values.value("theme").toString() == id)
        applyTheme("omarchy");
    return true;
}

QString Config::installThemeData(const QByteArray &bytes, QString *error) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        *error = tr("That is not a kai theme (expected a JSON object).");
        return {};
    }
    const QVariantMap known = themeDefaults();
    QVariantMap data;
    const QVariantMap incoming = document.object().toVariantMap();
    // Only keys kai understands are kept, so a theme can never smuggle other
    // settings into the config.
    for (auto it = incoming.begin(); it != incoming.end(); ++it)
        if (known.contains(it.key()))
            data.insert(it.key(), it.value());
    const QString name = data.value("name").toString().trimmed();
    if (name.isEmpty() || data.size() < 3) {
        *error = tr("The theme needs a name and at least a few style settings.");
        return {};
    }
    const QString id = slugify(name);
    if (!writeJsonAtomically(themesPath() + "/" + id + ".json", data)) {
        *error = tr("Could not write to %1.").arg(themesPath());
        return {};
    }
    loadThemes();
    return id;
}

bool Config::installThemeFile(const QString &path) {
    QFile file(QUrl(path).isLocalFile() ? QUrl(path).toLocalFile() : path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit themeInstallFinished(false, tr("Could not open %1.").arg(path));
        return false;
    }
    QString error;
    const QString id = installThemeData(file.readAll(), &error);
    if (id.isEmpty()) {
        emit themeInstallFinished(false, error);
        return false;
    }
    applyTheme(id);
    emit themeInstallFinished(true, tr("Installed “%1”.").arg(themeData(id).value("name").toString()));
    return true;
}

void Config::installThemeFromUrl(const QString &text) {
    const QUrl url = QUrl::fromUserInput(text.trimmed());
    if (url.isLocalFile()) {
        installThemeFile(url.toLocalFile());
        return;
    }
    if (!url.isValid() || (url.scheme() != "https" && url.scheme() != "http")) {
        emit themeInstallFinished(false, tr("Enter an http(s) address or a file path."));
        return;
    }
    if (!m_network)
        m_network = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(15000);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit themeInstallFinished(false, reply->errorString());
            return;
        }
        QString error;
        const QString id = installThemeData(reply->read(256 * 1024), &error);
        if (id.isEmpty()) {
            emit themeInstallFinished(false, error);
            return;
        }
        applyTheme(id);
        emit themeInstallFinished(true, tr("Installed “%1”.").arg(themeData(id).value("name").toString()));
    });
}

// Launchers ------------------------------------------------------------------

bool Config::isPinned(const QString &appId) const {
    return m_values.value("launchers").toList().contains(appId);
}

void Config::pin(const QString &appId, int index) {
    QVariantList list = m_values.value("launchers").toList();
    list.removeAll(appId);
    if (index < 0 || index > list.size())
        index = list.size();
    list.insert(index, appId);
    set("launchers", list);
}

void Config::unpin(const QString &appId) {
    QVariantList list = m_values.value("launchers").toList();
    list.removeAll(appId);
    set("launchers", list);
}

void Config::moveLauncher(int from, int to) {
    QVariantList list = m_values.value("launchers").toList();
    if (from < 0 || from >= list.size() || to < 0 || to >= list.size() || from == to)
        return;
    list.move(from, to);
    set("launchers", list);
}

// Applets --------------------------------------------------------------------

QVariantList Config::appletList() const { return m_values.value("applets").toList(); }

QVariantMap Config::applet(const QString &id) const {
    for (const auto &entry : appletList())
        if (entry.toMap().value("id").toString() == id)
            return entry.toMap();
    return {};
}

void Config::setApplet(const QString &id, const QString &key, const QVariant &value) {
    QVariantList list = appletList();
    for (auto &entry : list) {
        QVariantMap map = entry.toMap();
        if (map.value("id").toString() != id)
            continue;
        map.insert(key, value);
        entry = map;
    }
    set("applets", list);
}

void Config::moveApplet(int from, int to) {
    QVariantList list = appletList();
    if (from < 0 || from >= list.size() || to < 0 || to >= list.size() || from == to)
        return;
    list.move(from, to);
    set("applets", list);
}
