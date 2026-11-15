#pragma once

#include <QLocalServer>
#include <QObject>
#include <QRectF>
#include <QVariantList>

class QWindow;
class QQuickWindow;

// The desktop side of kai: turns QML windows into wlr-layer-shell surfaces
// (the dock along a screen edge, desklets on the desktop), sets which part of
// a surface takes the pointer, and runs commands. Without Wayland the windows
// stay ordinary frameless windows, which is what tests and screenshots use.
class Shell : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool layerShell READ layerShell CONSTANT)
    Q_PROPERTY(QString renderer READ renderer NOTIFY rendererChanged)
    Q_PROPERTY(QString requestedRenderer READ requestedRenderer CONSTANT)
    Q_PROPERTY(QStringList screens READ screens NOTIFY screensChanged)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit Shell(const QString &requestedRenderer, QObject *parent = nullptr);

    bool layerShell() const;
    QString renderer() const;
    QString requestedRenderer() const { return m_requestedRenderer; }
    QStringList screens() const;
    QString version() const;

    // The renderer actually in use, which differs from the requested one when
    // Qt had to fall back to software rendering.
    Q_INVOKABLE void trackWindow(QQuickWindow *window);
    Q_INVOKABLE QObject *screenFor(const QString &name) const;
    Q_INVOKABLE void setupDock(QWindow *window, const QString &edge, const QString &layer, const QString &screen);
    Q_INVOKABLE void updateDock(QWindow *window, const QString &edge, const QString &layer, int exclusiveZone);
    Q_INVOKABLE void setInputRect(QWindow *window, const QRectF &rect);
    Q_INVOKABLE void setupDesklet(QWindow *window, int x, int y, const QString &layer, const QString &screen);
    Q_INVOKABLE void moveDesklet(QWindow *window, int x, int y);
    Q_INVOKABLE void setDeskletLayer(QWindow *window, const QString &layer);
    // A full-screen overlay that takes the keyboard, on the screen in use.
    Q_INVOKABLE void setupLauncher(QWindow *window);

    Q_INVOKABLE void run(const QString &command) const;
    Q_INVOKABLE bool hasCommand(const QString &name) const;
    Q_INVOKABLE void restart();
    Q_INVOKABLE void openUrl(const QString &url) const;

    // One dock per user: a second `kai` hands its request to the running one.
    bool listen();
    static bool sendToRunning(const QString &message);

signals:
    void screensChanged();
    void rendererChanged();
    void message(const QString &text);

private:
    QString m_requestedRenderer;
    QString m_renderer;
    QLocalServer m_server;
};
