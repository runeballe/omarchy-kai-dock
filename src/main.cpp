// kai — a dock for Hyprland: launchers, running apps, applets and desklets.

#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTextStream>
#include <QThread>
#include <QUrl>

#include "apps.h"
#include "config.h"
#include "dockmodel.h"
#include "hyprland.h"
#include "iconprovider.h"
#include "shell.h"
#include "sysinfo.h"
#include "theme.h"
#include "fonts.h"
#include "translator.h"
#include "weather.h"
#include "windowkiller.h"
#include "audio.h"

namespace {

const char *kUsage = R"(kai — a dock for Hyprland

Usage:
  kai                      Start the dock (or open Settings if it's running)
  kai settings             Open Settings in the running dock
  kai apps                 Open or close the app launcher
  kai quit                 Quit the running dock
  kai --software           Start with the software (CPU) renderer this time
  kai --opengl             Start with the OpenGL renderer this time
  kai theme list [--json]  List installed themes
  kai theme install FILE   Install a theme from a JSON file [--json]
  kai theme apply ID       Apply a theme [--json]
  kai --version            Print the version
)";

int print(const QString &text, bool error = false) {
    QTextStream(error ? stderr : stdout) << text << (text.endsWith('\n') ? "" : "\n");
    return error ? 1 : 0;
}

// Theme commands run without a display, so agents and scripts can use them.
int themeCommand(const QStringList &args) {
    const bool json = args.contains("--json");
    QStringList rest = args;
    rest.removeAll("--json");
    Config config;
    const QString action = rest.value(0);
    if (action == "list") {
        QJsonArray list;
        QString text;
        for (const auto &entry : config.themes()) {
            const QVariantMap m = entry.toMap();
            const bool current = m.value("id") == config.get("theme");
            list << QJsonObject{{"id", m.value("id").toString()}, {"name", m.value("name").toString()},
                                {"builtin", m.value("builtin").toBool()}, {"current", current}};
            text += QString("%1 %2  (%3)%4\n").arg(current ? "*" : " ", m.value("id").toString(),
                                                  m.value("name").toString(),
                                                  m.value("builtin").toBool() ? "" : " installed");
        }
        return print(json ? QJsonDocument(list).toJson(QJsonDocument::Compact) : text);
    }
    if (action == "install" && rest.size() >= 2) {
        QFile file(rest[1]);
        if (!file.open(QIODevice::ReadOnly))
            return print(json ? QString(R"({"ok":false,"error":"cannot open file"})") : "kai: cannot open " + rest[1], true);
        QString error;
        const QString id = config.installThemeData(file.readAll(), &error);
        if (id.isEmpty())
            return print(json ? QJsonDocument(QJsonObject{{"ok", false}, {"error", error}}).toJson(QJsonDocument::Compact)
                              : "kai: " + error, true);
        return print(json ? QJsonDocument(QJsonObject{{"ok", true}, {"id", id}}).toJson(QJsonDocument::Compact)
                          : "Installed " + id);
    }
    if (action == "apply" && rest.size() >= 2) {
        bool found = false;
        for (const auto &entry : config.themes())
            found = found || entry.toMap().value("id") == rest[1];
        if (!found)
            return print(json ? QString(R"({"ok":false,"error":"no such theme"})") : "kai: no theme " + rest[1], true);
        config.applyTheme(rest[1]);
        config.saveNow();
        return print(json ? QString(R"({"ok":true})") : "Applied " + rest[1]);
    }
    return print(kUsage, true);
}

} // namespace

int main(int argc, char *argv[]) {
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);

    if (args.contains("--help") || args.contains("-h"))
        return print(kUsage);
    if (args.contains("--version")) {
        QTextStream(stdout) << "kai 0.1.0\n";
        return 0;
    }
    if (args.value(0) == "theme") {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        QGuiApplication app(argc, argv);
        return themeCommand(args.mid(1));
    }

    // The renderer has to be chosen before any window exists. OpenGL is the
    // fast path; the software renderer is for GPUs whose drivers misbehave.
    QString renderer = Config::readRenderer();
    if (args.contains("--software"))
        renderer = "software";
    else if (args.contains("--opengl"))
        renderer = "opengl";
    QQuickWindow::setGraphicsApi(renderer == "software" ? QSGRendererInterface::Software : QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
    app.setApplicationName("kai");
    app.setApplicationVersion("0.1.0");
    app.setDesktopFileName("kai");
    app.setWindowIcon(QIcon::fromTheme("kai"));
    app.setQuitOnLastWindowClosed(false);

    const QString command = args.value(0).startsWith("--") ? QString() : args.value(0);
    const bool replace = args.contains("--replace");
    if (!replace && Shell::sendToRunning(command.isEmpty() ? "settings" : command))
        return 0;
    if (command == "quit" || command == "settings") {
        if (command == "quit")
            return 0;
    }

    Shell shell(renderer);
    // A restart hands over the socket; give the old dock a moment to let go.
    for (int i = 0; i < 30 && !shell.listen(); ++i)
        QThread::msleep(100);

    QQuickStyle::setStyle("Material");
    IconProvider::configureIconTheme();

    Translator translator;
    app.installTranslator(&translator);

    Theme omarchy;
    Config config;
    AppIndex apps;
    Hyprland hyprland;
    DockModel dock(&config, &apps, &hyprland);
    SystemMonitor system;
    Weather weather;

    QQmlApplicationEngine engine;
    engine.addImageProvider("icon", new IconProvider);
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    auto *context = engine.rootContext();
    context->setContextProperty("translator", &translator);
    context->setContextProperty("omarchy", &omarchy);
    context->setContextProperty("config", &config);
    context->setContextProperty("apps", &apps);
    context->setContextProperty("hyprland", &hyprland);
    context->setContextProperty("dockModel", &dock);
    context->setContextProperty("system", &system);
    context->setContextProperty("weather", &weather);
    WindowKiller windowKiller(&hyprland);
    context->setContextProperty("windowKiller", &windowKiller);
    Audio audio;
    context->setContextProperty("audio", &audio);
    context->setContextProperty("shell", &shell);
    context->setContextProperty("fonts", new Fonts());
    context->setContextProperty("openSettingsOnStart", command == "settings");
    context->setContextProperty("openAppsOnStart", command == "apps");
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    QObject *root = engine.rootObjects().first();
    QObject::connect(&shell, &Shell::message, root, [root](const QString &text) {
        if (text == "quit")
            QCoreApplication::quit();
        else if (text == "apps")
            QMetaObject::invokeMethod(root, "toggleApps");
        else
            // openSettings(page) takes one argument, so the call must pass one.
            QMetaObject::invokeMethod(root, "openSettings", Q_ARG(QVariant, QVariant()));
    });

    const int code = app.exec();
    config.saveNow();
    return code;
}
