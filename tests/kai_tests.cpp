#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

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
        QCOMPARE(items[0].appId, QString("b"));
        QCOMPARE(items[1].appId, QString("a"));
        QCOMPARE(items[2].kind, QString("separator"));
        int applets = 0;
        for (const auto &i : items) applets += i.kind == "applet";
        QCOMPARE(applets, 4);  // apps, clock, sysmon, weather on by default
        config.setApplet("clock", "detached", true);
        model.rebuild();
        applets = 0;
        for (const auto &i : model.items()) applets += i.kind == "applet";
        QCOMPARE(applets, 3);
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
};

QTEST_MAIN(KaiTests)
#include "kai_tests.moc"
