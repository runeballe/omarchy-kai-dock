#include "dockservice.h"

#include "badges.h"
#include "config.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTextStream>

namespace {
const char *kService = "org.kai.Dock";
const char *kPath = "/org/kai/Dock";
const QStringList kActions{"click", "middle", "double", "scroll-up", "scroll-down"};
} // namespace

DockService::DockService(Badges *badges, QObject *parent) : QObject(parent), m_badges(badges) {}

bool DockService::registerOnBus() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    return bus.registerService(kService) && bus.registerObject(kPath, this, QDBusConnection::ExportAllSlots);
}

QString DockService::badgeKey(const QString &key) {
    return Config::knownApplets().contains(key) ? "applet:" + key : key;
}

bool DockService::Activate(const QString &applet, const QString &action) {
    const QString a = action.isEmpty() ? QStringLiteral("click") : action;
    if (!Config::knownApplets().contains(applet) || !kActions.contains(a))
        return false;
    emit appletAction(applet, a);
    return true;
}

void DockService::SetQuickInfo(const QString &key, const QString &text) { m_badges->setInfo(badgeKey(key), text); }

void DockService::SetProgress(const QString &key, double percent) {
    m_badges->setProgress(badgeKey(key), percent < 0 ? -1 : percent / 100.0);
}

void DockService::Clear(const QString &key) { m_badges->clear(badgeKey(key)); }

void DockService::Animate(const QString &key, const QString &animation) { emit animateRequested(key, animation); }

void DockService::OpenSettings() { emit settingsRequested(); }

QStringList DockService::Applets() { return Config::knownApplets(); }

int dockServiceCommand(const QStringList &args) {
    QTextStream err(stderr);
    QDBusInterface dock(kService, kPath, kService, QDBusConnection::sessionBus());
    if (!dock.isValid()) {
        err << "kai: the dock isn't running\n";
        return 1;
    }
    const QString command = args.value(0);
    auto check = [&](const QDBusMessage &reply) {
        if (reply.type() == QDBusMessage::ErrorMessage) {
            err << "kai: " << reply.errorMessage() << "\n";
            return 1;
        }
        return 0;
    };
    if (command == "applet" && args.size() >= 2) {
        const QDBusReply<bool> ok = dock.call("Activate", args[1], args.value(2, "click"));
        if (!ok.isValid() || !ok.value()) {
            err << "kai: no applet \"" << args[1] << "\" or no action \"" << args.value(2, "click")
                << "\" (actions: " << kActions.join(", ") << ")\n";
            return 1;
        }
        return 0;
    }
    if (command == "badge" && args.size() >= 2)
        return check(args.size() >= 3 && !args[2].isEmpty() ? dock.call("SetQuickInfo", args[1], args[2])
                                                             : dock.call("Clear", args[1]));
    if (command == "progress" && args.size() >= 3) {
        bool number = false;
        const double value = args[2].toDouble(&number);
        return check(dock.call("SetProgress", args[1], number ? value : -1.0));
    }
    if (command == "animate" && args.size() >= 3)
        return check(dock.call("Animate", args[1], args[2]));
    if (command == "applets") {
        const QDBusReply<QStringList> list = dock.call("Applets");
        QTextStream(stdout) << list.value().join('\n') << "\n";
        return 0;
    }
    err << "kai: usage: kai applet ID [click|middle|double|scroll-up|scroll-down]\n"
           "           kai badge KEY [TEXT]       (no text clears it)\n"
           "           kai progress KEY PERCENT|off\n"
           "           kai animate KEY bounce|pulse|flip|wobble|blink|wave|rotate\n"
           "           kai applets\n";
    return 1;
}
