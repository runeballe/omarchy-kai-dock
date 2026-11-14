#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQuick>
#include <QtTest>
#include <memory>

#include "apps.h"
#include "config.h"
#include "dockmodel.h"
#include "hyprland.h"
#include "sysinfo.h"
#include "theme.h"
#include "translator.h"
#include "weather.h"

static void write(const QString &path, const QByteArray &contents) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(contents);
}

class MenuAppStub : public QObject {
    Q_OBJECT
public:
    Q_PROPERTY(double radius READ radius CONSTANT)
    double radius() const { return 8; }
};

// Stand-ins for the dock window and app singletons DockItem reads.
class ItemDockStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant v READ v CONSTANT)
    Q_PROPERTY(QVariant t READ t CONSTANT)
    Q_PROPERTY(QVariant geom READ geom CONSTANT)
    Q_PROPERTY(double s READ s CONSTANT)
    Q_PROPERTY(double iconBase READ iconBase CONSTANT)
    Q_PROPERTY(double frameRotation READ frameRotation CONSTANT)
    Q_PROPERTY(double zoomFactor READ zoomFactor CONSTANT)
    Q_PROPERTY(double speed READ speed CONSTANT)
    Q_PROPERTY(double gap READ gap CONSTANT)
    Q_PROPERTY(double pad READ pad CONSTANT)
    Q_PROPERTY(bool panel READ panel CONSTANT)
    Q_PROPERTY(bool dragging READ dragging CONSTANT)
    Q_PROPERTY(int dragIndex READ dragIndex CONSTANT)
public:
    explicit ItemDockStub(const QString &hover, QObject *parent = nullptr) : QObject(parent), m_hover(hover) {}
    QVariant v() const { return QVariantMap{{"hoverAnimation", m_hover}}; }
    QVariant t() const { return QVariantMap{}; }
    QVariant geom() const {
        return QVariantMap{{"u", QVariantList{0.0}}, {"w", QVariantList{48.0}},
                           {"scale", QVariantList{1.0}}, {"start", 0.0}, {"length", 48.0}, {"maxScale", 1.0}};
    }
    double s() const { return 48; }
    double iconBase() const { return 10; }
    double frameRotation() const { return 0; }
    double zoomFactor() const { return 1; }
    double speed() const { return 1; }
    double gap() const { return 4; }
    double pad() const { return 8; }
    bool panel() const { return false; }
    bool dragging() const { return false; }
    int dragIndex() const { return -1; }
    Q_INVOKABLE double frameX(double u, double) const { return u; }
private:
    QString m_hover;
};

class ItemAppStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent CONSTANT)
    Q_PROPERTY(int radius READ radius CONSTANT)
    Q_PROPERTY(bool fancy READ fancy CONSTANT)
public:
    QString accent() const { return "#FFD60A"; }
    int radius() const { return 8; }
    bool fancy() const { return false; }
    Q_INVOKABLE QString color(const QString &value, const QString &fallback) const {
        return value.isEmpty() ? fallback : value;
    }
    Q_INVOKABLE QString withAlpha(const QString &color, double) const { return color; }
};

// Dock-window stand-in for popup positioning math.
class CalDockStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(double width READ width CONSTANT)
    Q_PROPERTY(double height READ height CONSTANT)
    Q_PROPERTY(QString edge READ edge CONSTANT)
public:
    double width() const { return 1920; }
    double height() const { return 400; }
    QString edge() const { return "bottom"; }
    Q_INVOKABLE QPointF anchorFor(int, double) const { return QPointF(960, 350); }
};

// Counts calls from ClockApplet.activate in dock mode.
class ClockDockStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(int openCalendarCalls READ openCalendarCalls CONSTANT)
public:
    int m_calls = 0;
    int openCalendarCalls() const { return m_calls; }
    Q_INVOKABLE void openCalendar(int) { ++m_calls; }
};

// shell stand-in: nothing available, run() counted.
class ShellStub : public QObject {
    Q_OBJECT
public:
    explicit ShellStub(QObject *parent = nullptr) : QObject(parent) {}
    int m_runs = 0;
    Q_INVOKABLE bool hasCommand(const QString &) const { return false; }
    Q_INVOKABLE void run(const QString &) { ++m_runs; }
};

class KaiTests : public QObject {
    Q_OBJECT

private slots:
    void configHasDefaultsAndSaves() {
        QTemporaryDir dir;
        {
            Config config(dir.filePath("cfg"), dir.filePath("data"));
            QCOMPARE(config.get("edge").toString(), QString("bottom"));
            QCOMPARE(config.values().value("applets").toList().size(), Config::knownApplets().size());
            config.set("iconSize", 64);
            config.pin("firefox");
            config.pin("chromium", 0);
        }
        Config again(dir.filePath("cfg"), dir.filePath("data"));
        QCOMPARE(again.get("iconSize").toInt(), 64);
        QCOMPARE(again.get("launchers").toList(), (QVariantList{"chromium", "firefox"}));
        again.moveLauncher(0, 1);
        QCOMPARE(again.get("launchers").toList().first().toString(), QString("firefox"));
        again.unpin("firefox");
        QVERIFY(!again.isPinned("firefox"));
    }

    void configReadsRenderer() {
        QTemporaryDir dir;
        QCOMPARE(Config::readRenderer(dir.path()), QString("opengl"));
        write(dir.filePath("config.json"), "{\"renderer\":\"software\"}");
        QCOMPARE(Config::readRenderer(dir.path()), QString("software"));
    }

    void themesLayerOverrides() {
        QTemporaryDir dir;
        Config config(dir.filePath("cfg"), dir.filePath("data"));
        QVERIFY(config.themes().size() >= 7);
        QCOMPARE(config.theme().value("name").toString(), QString("Omarchy"));
        config.applyTheme("glass");
        QCOMPARE(config.theme().value("radius").toInt(), 20);
        config.setThemeOverride("radius", 4);
        QCOMPARE(config.theme().value("radius").toInt(), 4);
        config.applyTheme("glass");
        QCOMPARE(config.theme().value("radius").toInt(), 20);
    }

    void themesInstallAndSave() {
        QTemporaryDir dir;
        Config config(dir.filePath("cfg"), dir.filePath("data"));
        QString error;
        QVERIFY(config.installThemeData("not json", &error).isEmpty());
        QVERIFY(!error.isEmpty());
        const QString id = config.installThemeData("{\"name\":\"Sea Glass\",\"background\":\"#004455\",\"radius\":9,\"launchers\":[\"evil\"]}", &error);
        QCOMPARE(id, QString("sea-glass"));
        QVERIFY(QFile::exists(config.themesPath() + "/sea-glass.json"));
        config.applyTheme(id);
        QCOMPARE(config.theme().value("background").toString(), QString("#004455"));
        QVERIFY(config.get("launchers").toList().isEmpty());  // themes can't set other settings
        config.setThemeOverride("gloss", 0.3);
        const QString saved = config.saveThemeAs("Mine");
        QCOMPARE(saved, QString("mine"));
        QCOMPARE(config.theme().value("gloss").toDouble(), 0.3);
        QVERIFY(config.deleteTheme("mine"));
        QCOMPARE(config.get("theme").toString(), QString("omarchy"));
        QVERIFY(!config.deleteTheme("glass"));  // built-ins stay
    }

    void appsMatchWindowClasses() {
        QTemporaryDir dir;
        write(dir.filePath("apps/org.gnome.Nautilus.desktop"), "[Desktop Entry]\nType=Application\nName=Files\nExec=nautilus --new-window\nIcon=org.gnome.Nautilus\n");
        write(dir.filePath("apps/chromium.desktop"), "[Desktop Entry]\nType=Application\nName=Chromium\nExec=/usr/bin/chromium %U\n");
        write(dir.filePath("apps/webapp.desktop"), "[Desktop Entry]\nType=Application\nName=HEY\nExec=chromium --app=https://hey.com\nStartupWMClass=chrome-hey.com__-Default\n");
        write(dir.filePath("apps/hidden.desktop"), "[Desktop Entry]\nType=Application\nName=Hidden\nNoDisplay=true\nExec=hidden\n");
        AppIndex apps({dir.filePath("apps")});
        QCOMPARE(apps.matchClass("org.gnome.Nautilus"), QString("org.gnome.Nautilus"));
        QCOMPARE(apps.matchClass("nautilus"), QString("org.gnome.Nautilus"));
        QCOMPARE(apps.matchClass("Chromium"), QString("chromium"));
        QCOMPARE(apps.matchClass("chrome-hey.com__-Default"), QString("webapp"));
        QCOMPARE(apps.matchClass("nothing-like-it"), QString());
        QCOMPARE(apps.apps().size(), 3);
        QCOMPARE(AppIndex::stripFieldCodes("foo %U --x %f"), QString("foo --x"));
    }

    void hyprlandParsesClients() {
        const QByteArray json = "[\n          {\"address\":\"0xA1\",\"mapped\":true,\"class\":\"firefox\",\"title\":\"Hi\",\"workspace\":{\"id\":1,\"name\":\"1\"},\"focusHistoryID\":0},\n          {\"address\":\"0xb2\",\"mapped\":true,\"class\":\"chromium\",\"title\":\"Min\",\"workspace\":{\"id\":-98,\"name\":\"special:minimized\"},\"focusHistoryID\":3},\n          {\"address\":\"0xc3\",\"mapped\":true,\"class\":\"scratch\",\"title\":\"S\",\"workspace\":{\"id\":-99,\"name\":\"special:scratchpad\"}},\n          {\"address\":\"0xd4\",\"mapped\":false,\"class\":\"ghost\",\"workspace\":{\"id\":1,\"name\":\"1\"}}\n        ]";
        const auto clients = Hyprland::parseClients(json, {"0xb2"});
        QCOMPARE(clients.size(), 2);
        QCOMPARE(clients[0].address, QString("0xa1"));
        QCOMPARE(clients[0].focusHistory, 0);
        QVERIFY(clients[1].minimized);
        QVERIFY(clients[1].urgent);
        QCOMPARE(Hyprland::parseActiveWorkspace("[{\"focused\":false,\"activeWorkspace\":{\"id\":3}},{\"focused\":true,\"activeWorkspace\":{\"id\":7}}]"), 7);
    }

    void dockModelOrdersItems() {
        QTemporaryDir dir;
        write(dir.filePath("apps/a.desktop"), "[Desktop Entry]\nType=Application\nName=Alpha\nExec=a\n");
        write(dir.filePath("apps/b.desktop"), "[Desktop Entry]\nType=Application\nName=Beta\nExec=b\n");
        Config config(dir.filePath("cfg"), dir.filePath("data"));
        config.set("launchersSeeded", true);
        config.set("launchers", QVariantList{"b", "a"});
        AppIndex apps({dir.filePath("apps")});
        DockModel model(&config, &apps, nullptr);
        const auto items = model.items();
        QCOMPARE(items[0].kind, QString("applet"));
        QCOMPARE(items[0].appletId, QString("apps"));
        QCOMPARE(items[1].appId, QString("b"));
        QCOMPARE(items[2].appId, QString("a"));
        QCOMPARE(items[3].kind, QString("separator"));
        QCOMPARE(items[3].key, QString("sep:applets"));
        int applets = 0;
        for (const auto &i : items) applets += i.kind == "applet";
        QCOMPARE(applets, 4);  // apps, clock, sysmon, weather on by default
        config.setApplet("clock", "detached", true);
        model.rebuild();
        applets = 0;
        for (const auto &i : model.items()) applets += i.kind == "applet";
        QCOMPARE(applets, 3);
    }

    void dockModelKeepsAppsDetachedOutOfFrontSlot() {
        QTemporaryDir dir;
        write(dir.filePath("apps/a.desktop"), "[Desktop Entry]\nType=Application\nName=Alpha\nExec=a\n");
        write(dir.filePath("apps/b.desktop"), "[Desktop Entry]\nType=Application\nName=Beta\nExec=b\n");
        Config config(dir.filePath("cfg"), dir.filePath("data"));
        config.set("launchersSeeded", true);
        config.set("launchers", QVariantList{"b", "a"});
        AppIndex apps({dir.filePath("apps")});
        DockModel model(&config, &apps, nullptr);
        config.setApplet("apps", "detached", true);
        model.rebuild();
        const auto items = model.items();
        QCOMPARE(items[0].appId, QString("b"));
        for (const auto &i : items)
            QVERIFY(i.appletId != "apps");
    }

    QQuickItem *loadAppsApplet(const QVariantMap &extra) {
        ItemAppStub appStub;
        QVariantMap props{{"app", QVariant::fromValue(&appStub)}};
        for (auto it = extra.begin(); it != extra.end(); ++it)
            props.insert(it.key(), it.value());
        QQmlComponent component(&engine, QUrl("qrc:/applets/AppsApplet.qml"));
        QObject *raw = component.createWithInitialProperties(props);
        auto *item = qobject_cast<QQuickItem *>(raw);
        if (item)
            item->setParentItem(harnessWindow->contentItem());
        if (component.isError() || !item) {
            m_loadError = component.errorString();
            return nullptr;
        }
        QTest::qWait(50);  // give the local-file image a tick to load
        return item;
    }

private slots:
    void appsAppletShowsOmarchyLogo() {
        QQuickItem *item = loadAppsApplet({});
        QVERIFY2(item, qPrintable(m_loadError));
        QQuickItem *image = nullptr;
        for (auto *kid : item->findChildren<QQuickItem *>())
            if (QString(kid->metaObject()->className()).startsWith("QQuickImage"))
                image = kid;
        QVERIFY2(image, "the applet has no logo Image");
        QVERIFY(image->property("source").toString().contains("omarchy/icon.png"));
        QCOMPARE(image->isVisible(), true);
    }

    void appsAppletLogoFallsBackToGrid() {
        QQuickItem *item = loadAppsApplet({{"logoSource", "file:///nonexistent/kai-missing.png"}});
        QVERIFY2(item, qPrintable(m_loadError));
        QQuickItem *image = nullptr;
        QQuickItem *grid = nullptr;
        for (auto *kid : item->findChildren<QQuickItem *>()) {
            const QString cn = kid->metaObject()->className();
            if (cn.startsWith("QQuickImage"))
                image = kid;
            else if (cn.startsWith("QQuickGrid"))
                grid = kid;
        }
        QVERIFY2(image, "the applet has no logo Image");
        QCOMPARE(image->isVisible(), false);
        QVERIFY2(grid && grid->isVisible(), "the grid glyph did not come back when the logo is missing");
    }

    void clockDigitalSecondsFormat() {
        ItemAppStub appStub;
        QQmlComponent component(&engine, QUrl("qrc:/applets/ClockApplet.qml"));
        auto *on = qobject_cast<QQuickItem *>(component.createWithInitialProperties({{"app", QVariant::fromValue(&appStub)}}));
        QVERIFY2(on && !component.isError(), qPrintable(component.errorString()));
        on->setParentItem(harnessWindow->contentItem());
        const QRegularExpression withSeconds("\\d{1,2}:\\d{2}:\\d{2}");
        QVERIFY(on->property("timeText").toString().contains(withSeconds));
        QVERIFY(!on->property("title").toString().contains(withSeconds));
        delete on;

        clockConfig->set("clockSeconds", false);
        auto *off = qobject_cast<QQuickItem *>(component.createWithInitialProperties({{"app", QVariant::fromValue(&appStub)}}));
        QVERIFY2(off && !component.isError(), qPrintable(component.errorString()));
        off->setParentItem(harnessWindow->contentItem());
        QVERIFY(QRegularExpression("\\d{1,2}:\\d{2}$").match(off->property("timeText").toString()).hasMatch());
        delete off;
        clockConfig->set("clockSeconds", true);
    }

    void calendarPopupShowsMonthAndNavigates() {
        ItemAppStub appStub;
        CalDockStub dockStub;
        QVariantMap props{{"app", QVariant::fromValue(&appStub)},
                          {"dock", QVariant::fromValue(&dockStub)},
                          {"parent", QVariant::fromValue(harnessWindow->contentItem())}};
        QQmlComponent component(&engine, QUrl("qrc:/CalendarPopup.qml"));
        QObject *popup = component.createWithInitialProperties(props);
        QVERIFY2(popup && !component.isError(), qPrintable(component.errorString()));
        lastPopup = popup;
        const QString before = popup->property("monthLabel").toString();
        QVERIFY(!before.isEmpty());
        const bool ok = QMetaObject::invokeMethod(popup, "openFor", Q_ARG(QVariant, 0));
        QVERIFY2(ok, "openFor is not invokable");
        QTest::qWait(30);
        QVERIFY2(popup->property("visible").toBool() == true,
                 qPrintable(QString("visible=%1 x=%2 y=%3")
                                .arg(popup->property("visible").toString())
                                .arg(popup->property("x").toString())
                                .arg(popup->property("y").toString())));
        QMetaObject::invokeMethod(popup, "shift", Q_ARG(QVariant, 1));
        QVERIFY(popup->property("monthLabel").toString() != before);
        const QString after = popup->property("monthLabel").toString();
        QMetaObject::invokeMethod(popup, "shift", Q_ARG(QVariant, -2));
        QVERIFY(popup->property("monthLabel").toString() != after);
    }

    void clockClickOpensCalendarPopupInDockMode() {
        ItemAppStub appStub;
        ClockDockStub dockStub;
        QQmlComponent component(&engine, QUrl("qrc:/applets/ClockApplet.qml"));
        auto *clock = qobject_cast<QQuickItem *>(component.createWithInitialProperties(
            {{"app", QVariant::fromValue(&appStub)}, {"dock", QVariant::fromValue(&dockStub)}}));
        QVERIFY2(clock && !component.isError(), qPrintable(component.errorString()));
        clock->setParentItem(harnessWindow->contentItem());

        const QVariant dv = clock->property("dock");
        QVERIFY2(dv.value<QObject *>() == &dockStub, qPrintable("dock property: " + dv.toString() + " typeName=" + dv.typeName()));
        QVERIFY2(clock->property("mode").toString() == "dock", qPrintable("mode=" + clock->property("mode").toString()));

        QMetaObject::invokeMethod(clock, "activate", Q_ARG(QVariant, 0), Q_ARG(QVariant, 3));
        QCOMPARE(dockStub.openCalendarCalls(), 1);

        clock->setProperty("mode", "desklet");
        QMetaObject::invokeMethod(clock, "activate", Q_ARG(QVariant, 0), Q_ARG(QVariant, 3));
        QCOMPARE(dockStub.openCalendarCalls(), 1);  // desklet keeps launching gnome-calendar

        QVariant entries;
        QMetaObject::invokeMethod(clock, "menuEntries", Q_RETURN_ARG(QVariant, entries));
        const QVariantList list = entries.toList();
        QVERIFY(list.size() >= 1);
        QVERIFY(!list.first().toMap().value("text").toString().isEmpty());
        delete clock;
    }

    void systemMonitorParses() {
        const auto t = SystemMonitor::parseStat("cpu  100 0 50 800 50 0 0 0 0 0\ncpu0 1 2\n");
        QCOMPARE(t.total, quint64(1000));
        QCOMPARE(t.idle, quint64(850));
        const auto m = SystemMonitor::parseMeminfo("MemTotal:  16000000 kB\nMemFree: 1 kB\nMemAvailable:  4000000 kB\n");
        QCOMPARE(m.first, quint64(16000000));
        QCOMPARE(m.second, quint64(4000000));
    }

    void systemMonitorEmptiesOnlyItsTrash() {
        QTemporaryDir dir;
        write(dir.filePath("Trash/files/old.txt"), "x");
        write(dir.filePath("Trash/info/old.txt.trashinfo"), "[Trash Info]");
        write(dir.filePath("keep.txt"), "keep");
        SystemMonitor mon(dir.path(), dir.filePath("Trash"));
        QCOMPARE(mon.trashCount(), 1);
        mon.emptyTrash();
        QCOMPARE(mon.trashCount(), 0);
        QVERIFY(QFile::exists(dir.filePath("keep.txt")));
    }

    void weatherCodes() {
        QCOMPARE(Weather::kindForCode(0), QString("clear"));
        QCOMPARE(Weather::kindForCode(63), QString("rain"));
        QCOMPARE(Weather::kindForCode(75), QString("snow"));
        QCOMPARE(Weather::kindForCode(95), QString("storm"));
        Weather w;
        QVERIFY(w.parseForecast("{\"current\":{\"temperature_2m\":4.5,\"weather_code\":3,\"wind_speed_10m\":12,\"is_day\":0},\n          \"daily\":{\"time\":[\"2026-11-12\",\"2026-11-13\"],\"weather_code\":[3,61],\"temperature_2m_max\":[6,7],\"temperature_2m_min\":[1,2]}}"));
        QCOMPARE(w.temperature(), 4.5);
        QCOMPARE(w.kind(), QString("cloudy"));
        QVERIFY(!w.day());
        QCOMPARE(w.forecast().size(), 2);
    }

    void themeReadsAccent() {
        QTemporaryDir dir;
        write(dir.filePath("theme/colors.toml"), "accent = \"#112233\"\n");
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void translatorFollowsLanguage() {
        QTemporaryDir dir;
        write(dir.filePath("language"), "no\n");
        write(dir.filePath("no.json"), "{\"Clock\": \"Klokke\"}");
        Translator translator(dir.filePath("language"), dir.path());
        QCOMPARE(translator.translate("x", "Clock"), QString("Klokke"));
        QVERIFY(translator.translate("x", "Trash").isNull());
    }

    void bundledCatalogsAreValid() {
        for (const QString &code : {"en", "no"}) {
            QFile f(":/i18n/" + code + ".json");
            QVERIFY(f.open(QIODevice::ReadOnly));
            QJsonParseError e;
            QJsonDocument::fromJson(f.readAll(), &e);
            QCOMPARE(e.error, QJsonParseError::NoError);
        }
    }

    void hoverAnimationDefaultAndPersist() {
        QTemporaryDir dir;
        {
            Config config(dir.filePath("cfg"), dir.filePath("data"));
            QCOMPARE(config.get("hoverAnimation").toString(), QString("bounce"));
            config.set("hoverAnimation", "flip");
        }
        Config again(dir.filePath("cfg"), dir.filePath("data"));
        QCOMPARE(again.get("hoverAnimation").toString(), QString("flip"));
    }

private:
    QQuickItem *loadDockItem(QObject *dock, QObject *app, bool launching) {
        QQmlComponent component(&engine, QUrl("qrc:/DockItem.qml"));
        QObject *raw = component.beginCreate(engine.rootContext());
        component.setInitialProperties(raw, {
            {"dock", QVariant::fromValue(dock)},
            {"app", QVariant::fromValue(app)},
            {"index", 0},
            {"kind", "app"},
            {"appId", "test.app"},
            {"name", "Test"},
            {"icon", "test-app"},
            {"appletId", ""},
            {"pinned", false},
            {"running", false},
            {"windowCount", 0},
            {"active", false},
            {"urgent", false},
            {"launching", launching},
        });
        auto *item = qobject_cast<QQuickItem *>(raw);
        if (item)
            item->setParentItem(harnessWindow->contentItem());
        component.completeCreate();
        if (component.isError() || !item) {
            m_loadError = component.errorString();
            return nullptr;
        }
        return item;
    }

    QQuickWindow *harnessWindow = nullptr;
    QQmlEngine engine;
    QString m_loadError;
    QTemporaryDir clockCfgDir;
    Config *clockConfig = nullptr;
    QObject *lastPopup = nullptr;
    ShellStub *shellStub = nullptr;

private slots:
    void init() {
        if (!harnessWindow) {
            engine.rootContext()->setContextProperty("dockModel", new QObject(this));  // Connections target only
            harnessWindow = new QQuickWindow;
            harnessWindow->resize(240, 240);
            harnessWindow->create();
            harnessWindow->show();  // offscreen: popups only open in a visible window
            if (!clockConfig) {
                clockConfig = new Config(clockCfgDir.path(), clockCfgDir.path(), QString(), this);
                engine.rootContext()->setContextProperty("config", clockConfig);
                auto *translator = new Translator(clockCfgDir.filePath("language"), clockCfgDir.path(), this);
                write(clockCfgDir.filePath("language"), "en\n");
                engine.rootContext()->setContextProperty("translator", translator);
                shellStub = new ShellStub(this);
                engine.rootContext()->setContextProperty("shell", shellStub);
            }
        }
    }
    void cleanup() {
        if (lastPopup) {
            delete lastPopup;
            lastPopup = nullptr;
        }
        for (auto *item : harnessWindow->contentItem()->findChildren<QQuickItem *>())
            delete item;
    }

    void dockItemHoverPlaysOnce() {
        ItemDockStub dock("bounce");
        ItemAppStub appStub;
        QQuickItem *item = loadDockItem(&dock, &appStub, false);
        QVERIFY2(item, qPrintable(m_loadError));
        QMetaObject::invokeMethod(item, "playHover");
        QTest::qWait(300);  // mid-flight: the bounce peaks at 230 * speed ms
        QVERIFY2(item->property("bounce").toDouble() > 0, "hover did not start the bounce animation");
    }

    void dockItemHoverRespectsLoopState() {
        ItemDockStub dock("bounce");
        ItemAppStub appStub;
        QQuickItem *item = loadDockItem(&dock, &appStub, true);  // launching → loop state "launch"
        QVERIFY2(item, qPrintable(m_loadError));
        QMetaObject::invokeMethod(item, "playHover");
        QTest::qWait(300);
        QCOMPARE(item->property("bounce").toDouble(), 0.0);
    }

    void dockMenuEntriesRenderTheirText() {
        MenuAppStub appStub;
        QObject menuDock;
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl("qrc:/DockMenu.qml"));
        std::unique_ptr<QObject> menu{component.createWithInitialProperties(
            {{"dock", QVariant::fromValue(&menuDock)},
             {"app", QVariant::fromValue(&appStub)}})};
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QVERIFY(menu);
        menu->setProperty("entries", QVariantList{
            QVariantMap{{"header", true}, {"text", "Firefox"}},
            QVariantMap{{"text", "New window"}},
            QVariantMap{{"separator", true}},
            QVariantMap{{"text", "Pin to dock"}},
        });
        int checked = 0;
        for (QQuickItem *item : menu->findChildren<QQuickItem*>()) {
            if (!QString(item->metaObject()->className()).contains("MenuItem"))
                continue;
            const QVariantMap d = item->property("d").toMap();
            QQuickItem *content = item->property("contentItem").value<QQuickItem*>();
            QVERIFY2(content, qPrintable(QString("entry '%1' has no contentItem; nothing renders its text")
                                             .arg(item->property("text").toString())));
            if (!d.value("separator").toBool())
                QCOMPARE(item->property("text").toString(), d.value("text").toString());
            ++checked;
        }
        QCOMPARE(checked, 4);
    }
};

QTEST_MAIN(KaiTests)
#include "kai_tests.moc"
