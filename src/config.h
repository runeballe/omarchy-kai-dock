#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

// Everything the user can set lives in one JSON file, ~/.config/kai/config.json,
// and every change is saved atomically shortly after it is made. Themes are
// JSON files too: the built-in ones are compiled in, installed ones live in
// ~/.local/share/kai/themes, and the user's tweaks sit on top of whichever is
// applied as "themeOverrides", so switching back and forth never loses a theme.
class Config : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY changed)
    Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themesChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString themesPath READ themesPath CONSTANT)

public:
    explicit Config(const QString &configDirectory = {}, const QString &dataDirectory = {},
                    const QString &builtinThemes = ":/themes", QObject *parent = nullptr);
    ~Config() override;

    QVariantMap values() const { return m_values; }
    QVariantMap theme() const { return m_theme; }
    QVariantList themes() const { return m_themes; }
    QString configPath() const;
    QString themesPath() const { return m_dataDirectory + "/themes"; }

    Q_INVOKABLE QVariant get(const QString &key, const QVariant &fallback = {}) const;
    Q_INVOKABLE void set(const QString &key, const QVariant &value);
    Q_INVOKABLE void resetAll();

    // Themes.
    Q_INVOKABLE void applyTheme(const QString &id);
    Q_INVOKABLE void setThemeOverride(const QString &key, const QVariant &value);
    Q_INVOKABLE void resetThemeOverrides();
    Q_INVOKABLE QString saveThemeAs(const QString &name);
    Q_INVOKABLE bool deleteTheme(const QString &id);
    Q_INVOKABLE void installThemeFromUrl(const QString &url);
    Q_INVOKABLE bool installThemeFile(const QString &path);
    // Validates theme JSON and installs it; returns the new id or empty with *error set.
    QString installThemeData(const QByteArray &data, QString *error);

    // Launchers pinned to the dock, as desktop file ids in dock order.
    Q_INVOKABLE bool isPinned(const QString &appId) const;
    Q_INVOKABLE void pin(const QString &appId, int index = -1);
    Q_INVOKABLE void unpin(const QString &appId);
    Q_INVOKABLE void moveLauncher(int from, int to);

    // Applets: an ordered list of { id, enabled, detached, x, y }.
    Q_INVOKABLE QVariantMap applet(const QString &id) const;
    Q_INVOKABLE void setApplet(const QString &id, const QString &key, const QVariant &value);
    Q_INVOKABLE void moveApplet(int from, int to);

    void saveNow();

    static QVariantMap defaults();
    static QVariantMap themeDefaults();
    static QStringList knownApplets();
    // The renderer is needed before the Qt application exists, so this reads
    // the file directly.
    static QString readRenderer(const QString &configDirectory = {});
    static QString defaultConfigDirectory();
    static QString defaultDataDirectory();
    static QString slugify(const QString &name);

signals:
    void changed();
    void themeChanged();
    void themesChanged();
    void themeInstallFinished(bool ok, const QString &message);

private:
    void load();
    void scheduleSave();
    void loadThemes();
    void rebuildTheme();
    QVariantMap themeData(const QString &id) const;
    bool isBuiltinTheme(const QString &id) const;
    QVariantList appletList() const;
    void ensureApplets();

    QString m_configDirectory;
    QString m_dataDirectory;
    QString m_builtinThemes;
    QVariantMap m_values;
    QVariantMap m_theme;
    QVariantList m_themes;
    QTimer m_saveTimer;
    QNetworkAccessManager *m_network = nullptr;
};
