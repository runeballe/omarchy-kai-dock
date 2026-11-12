#pragma once

#include <QLocalSocket>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVariantList>

struct HyprClient {
    QString address;
    QString windowClass;
    QString title;
    QString monitor;
    int workspaceId = 0;
    QString workspaceName;
    int focusHistory = 99;
    qint64 pid = 0;
    bool minimized = false;
    bool urgent = false;
};

// Talks to Hyprland over its IPC sockets: the request socket for the window
// list and for dispatching, and the event socket to know when to look again.
// Dispatchers use the Lua API of Hyprland 0.55+ (hl.dsp.*).
class Hyprland : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(QVariantList clients READ clientsVariant NOTIFY clientsChanged)
    Q_PROPERTY(int activeWorkspace READ activeWorkspace NOTIFY clientsChanged)

public:
    explicit Hyprland(QObject *parent = nullptr);

    bool available() const { return !m_directory.isEmpty(); }
    const QList<HyprClient> &clients() const { return m_clients; }
    QVariantList clientsVariant() const;
    int activeWorkspace() const { return m_activeWorkspace; }

    Q_INVOKABLE void focus(const QString &address);
    Q_INVOKABLE void close(const QString &address);
    Q_INVOKABLE void minimize(const QString &address);
    Q_INVOKABLE QString request(const QString &command) const;
    Q_INVOKABLE void refresh();

    static QList<HyprClient> parseClients(const QByteArray &json, const QSet<QString> &urgent);
    static int parseActiveWorkspace(const QByteArray &monitorsJson);
    static QString normaliseAddress(const QString &address);

signals:
    void clientsChanged();
    void windowOpened(const QString &windowClass);

private:
    void connectEvents();
    void handleEvent(const QByteArray &line);
    void dispatch(const QString &lua) const;

    QString m_directory;
    QLocalSocket m_events;
    QByteArray m_buffer;
    QTimer m_refresh;
    QTimer m_reconnect;
    QList<HyprClient> m_clients;
    QSet<QString> m_urgent;
    int m_activeWorkspace = 1;
};
