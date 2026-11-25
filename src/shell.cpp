#include "shell.h"

#include <LayerShellQt/window.h>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QLocalSocket>
#include <QProcess>
#include <QQuickWindow>
#include <QRegion>
#include <QScreen>
#include <QStandardPaths>
#include <QUrl>

#include <unistd.h>

namespace {
QString serverName() { return QString("kai-%1").arg(getuid()); }

LayerShellQt::Window::Anchors anchorsFor(const QString &edge) {
    using W = LayerShellQt::Window;
    if (edge == "top") return W::Anchors(W::AnchorTop | W::AnchorLeft | W::AnchorRight);
    if (edge == "left") return W::Anchors(W::AnchorLeft | W::AnchorTop | W::AnchorBottom);
    if (edge == "right") return W::Anchors(W::AnchorRight | W::AnchorTop | W::AnchorBottom);
    return W::Anchors(W::AnchorBottom | W::AnchorLeft | W::AnchorRight);
}

LayerShellQt::Window::Anchor edgeAnchor(const QString &edge) {
    using W = LayerShellQt::Window;
    if (edge == "top") return W::AnchorTop;
    if (edge == "left") return W::AnchorLeft;
    if (edge == "right") return W::AnchorRight;
    return W::AnchorBottom;
}

LayerShellQt::Window::Layer layerFor(const QString &layer) {
    using W = LayerShellQt::Window;
    if (layer == "overlay") return W::LayerOverlay;
    if (layer == "bottom") return W::LayerBottom;
    if (layer == "background") return W::LayerBackground;
    return W::LayerTop;
}
} // namespace

Shell::Shell(const QString &requestedRenderer, QObject *parent)
    : QObject(parent), m_requestedRenderer(requestedRenderer) {
    connect(qApp, &QGuiApplication::screenAdded, this, &Shell::screensChanged);
    connect(qApp, &QGuiApplication::screenRemoved, this, &Shell::screensChanged);
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
                emit message(QString::fromUtf8(socket->readAll()).trimmed());
                socket->disconnectFromServer();
            });
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
}

bool Shell::listen() {
    if (m_server.listen(serverName()))
        return true;
    // A stale socket from a crashed dock is removed and claimed.
    QLocalServer::removeServer(serverName());
    return m_server.listen(serverName());
}

bool Shell::sendToRunning(const QString &text) {
    QLocalSocket socket;
    socket.connectToServer(serverName());
    if (!socket.waitForConnected(300))
        return false;
    socket.write(text.toUtf8());
    socket.flush();
    socket.waitForBytesWritten(300);
    socket.waitForDisconnected(300);
    return true;
}

bool Shell::layerShell() const { return QGuiApplication::platformName() == "wayland"; }

namespace {
QString apiName(QSGRendererInterface::GraphicsApi api) {
    switch (api) {
    case QSGRendererInterface::Software: return "software";
    case QSGRendererInterface::OpenGL: return "opengl";
    case QSGRendererInterface::Vulkan: return "vulkan";
    case QSGRendererInterface::Unknown: return "";
    default: return "other";
    }
}
} // namespace

QString Shell::renderer() const {
    if (!m_renderer.isEmpty())
        return m_renderer;
    const QString requested = apiName(QQuickWindow::graphicsApi());
    return requested.isEmpty() ? "opengl" : requested;
}

void Shell::trackWindow(QQuickWindow *window) {
    if (!window)
        return;
    auto update = [this, window] {
        if (QSGRendererInterface *ri = window->rendererInterface()) {
            const QString api = apiName(ri->graphicsApi());
            if (!api.isEmpty() && api != m_renderer) {
                m_renderer = api;
                emit rendererChanged();
            }
        }
    };
    connect(window, &QQuickWindow::sceneGraphInitialized, this, update, Qt::QueuedConnection);
    update();
}

QString Shell::version() const { return QCoreApplication::applicationVersion(); }

QStringList Shell::screens() const {
    QStringList names;
    for (QScreen *screen : QGuiApplication::screens())
        names << screen->name();
    return names;
}

QObject *Shell::screenFor(const QString &name) const {
    for (QScreen *screen : QGuiApplication::screens())
        if (screen->name() == name)
            return screen;
    return QGuiApplication::primaryScreen();
}

void Shell::setupDock(QWindow *window, const QString &edge, const QString &layer, const QString &screenName) {
    if (!window)
        return;
    QScreen *screen = qobject_cast<QScreen *>(screenFor(screenName));
    if (screen)
        window->setScreen(screen);
    if (!layerShell())
        return;
    auto *ls = LayerShellQt::Window::get(window);
    ls->setScope("kai-dock");
    ls->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    ls->setActivateOnShow(false);
    ls->setCloseOnDismissed(false);
    if (screen && !screenName.isEmpty())
        ls->setScreen(screen);
    updateDock(window, edge, layer, 0);
}

void Shell::updateDock(QWindow *window, const QString &edge, const QString &layer, int exclusiveZone) {
    if (!window || !layerShell())
        return;
    auto *ls = LayerShellQt::Window::get(window);
    ls->setAnchors(anchorsFor(edge));
    ls->setLayer(layerFor(layer));
    ls->setExclusiveEdge(edgeAnchor(edge));
    ls->setExclusiveZone(exclusiveZone);
    ls->setMargins({});
}

// Only the visible dock takes the pointer; the rest of the tall transparent
// surface lets clicks through to the windows beneath.
void Shell::setInputRect(QWindow *window, const QRectF &rect) {
    if (!window)
        return;
    const QRegion region(rect.toAlignedRect());
    if (window->mask() != region)
        window->setMask(region.isEmpty() ? QRegion(0, 0, 1, 1) : region);
}

void Shell::setupDesklet(QWindow *window, int x, int y, const QString &layer, const QString &screenName) {
    if (!window)
        return;
    QScreen *screen = qobject_cast<QScreen *>(screenFor(screenName));
    if (screen)
        window->setScreen(screen);
    if (!layerShell()) {
        window->setPosition(x, y);
        return;
    }
    auto *ls = LayerShellQt::Window::get(window);
    ls->setScope("kai-desklet");
    ls->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    ls->setActivateOnShow(false);
    ls->setCloseOnDismissed(false);
    if (screen && !screenName.isEmpty())
        ls->setScreen(screen);
    ls->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft));
    ls->setLayer(layerFor(layer));
    ls->setExclusiveZone(-1);
    ls->setMargins({x, y, 0, 0});
}

void Shell::moveDesklet(QWindow *window, int x, int y) {
    if (!window)
        return;
    if (!layerShell()) {
        window->setPosition(x, y);
        return;
    }
    LayerShellQt::Window::get(window)->setMargins({x, y, 0, 0});
}

void Shell::setDeskletLayer(QWindow *window, const QString &layer) {
    if (window && layerShell())
        LayerShellQt::Window::get(window)->setLayer(layerFor(layer));
}

void Shell::setupLauncher(QWindow *window) {
    if (!window || !layerShell())
        return;
    using W = LayerShellQt::Window;
    auto *ls = W::get(window);
    ls->setScope("kai-launcher");
    ls->setWantsToBeOnActiveScreen(true);
    ls->setLayer(W::LayerOverlay);
    ls->setAnchors(W::Anchors(W::AnchorTop | W::AnchorBottom | W::AnchorLeft | W::AnchorRight));
    ls->setExclusiveZone(-1);
    ls->setKeyboardInteractivity(W::KeyboardInteractivityExclusive);
    ls->setCloseOnDismissed(false);
}

void Shell::run(const QString &command) const {
    if (!command.trimmed().isEmpty())
        QProcess::startDetached("/bin/sh", {"-c", command});
}

bool Shell::hasCommand(const QString &name) const { return !QStandardPaths::findExecutable(name).isEmpty(); }

void Shell::restart() {
    m_server.close();
    QStringList args = QCoreApplication::arguments().mid(1);
    args.removeAll("--software");
    args.removeAll("--opengl");
    args.removeAll("--replace");
    args << "--replace";
    QProcess::startDetached(QCoreApplication::applicationFilePath(), args);
    QCoreApplication::quit();
}

void Shell::openUrl(const QString &url) const { QDesktopServices::openUrl(QUrl(url)); }
