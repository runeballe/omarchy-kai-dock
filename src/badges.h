#pragma once

#include <QHash>
#include <QObject>
#include <QVariantMap>

// Quick-info and progress shown over dock icons. Apps set them through the
// Unity LauncherEntry D-Bus signal that Chromium, Telegram, Signal and others
// already send; scripts and kai's own API set them by key. Keys are desktop
// file ids for apps ("org.telegram.desktop") and "applet:<id>" for applets.
class Badges : public QObject {
    Q_OBJECT
    // key -> { info: string, progress: -1 (none) or 0..1, urgent: bool }
    Q_PROPERTY(QVariantMap entries READ entries NOTIFY changed)

public:
    explicit Badges(QObject *parent = nullptr);

    QVariantMap entries() const;
    Q_INVOKABLE void setInfo(const QString &key, const QString &info);
    Q_INVOKABLE void setProgress(const QString &key, double progress);
    Q_INVOKABLE void clear(const QString &key);

    // Folds one Unity Update into an entry's state.
    struct State {
        qint64 count = 0;
        bool countVisible = false;
        double progress = 0;
        bool progressVisible = false;
        bool urgent = false;
        QString info;            // set by scripts; wins over the count
        double ownProgress = -1; // set by scripts; wins over Unity's
    };
    static void applyUnity(State &state, const QVariantMap &properties);
    static QString keyForUri(const QString &appUri);
    static QVariantMap present(const State &state);

public slots:
    void unityUpdate(const QString &appUri, const QVariantMap &properties);

signals:
    void changed();

private:
    QHash<QString, State> m_states;
};
