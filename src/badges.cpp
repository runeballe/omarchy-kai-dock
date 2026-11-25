#include "badges.h"

#include <QDBusConnection>

Badges::Badges(QObject *parent) : QObject(parent) {
    QDBusConnection::sessionBus().connect(QString(), QString(), "com.canonical.Unity.LauncherEntry", "Update", this,
                                          SLOT(unityUpdate(QString, QVariantMap)));
}

QString Badges::keyForUri(const QString &appUri) {
    QString key = appUri;
    if (key.startsWith("application://"))
        key = key.mid(14);
    if (key.endsWith(".desktop"))
        key.chop(8);
    return key;
}

void Badges::applyUnity(State &state, const QVariantMap &p) {
    if (p.contains("count")) state.count = p.value("count").toLongLong();
    if (p.contains("count-visible")) state.countVisible = p.value("count-visible").toBool();
    if (p.contains("progress")) state.progress = qBound(0.0, p.value("progress").toDouble(), 1.0);
    if (p.contains("progress-visible")) state.progressVisible = p.value("progress-visible").toBool();
    if (p.contains("urgent")) state.urgent = p.value("urgent").toBool();
}

QVariantMap Badges::present(const State &s) {
    QString info = s.info;
    if (info.isEmpty() && s.countVisible && s.count > 0)
        info = s.count > 999 ? QStringLiteral("999+") : QString::number(s.count);
    const double progress = s.ownProgress >= 0 ? s.ownProgress : s.progressVisible ? s.progress : -1;
    return {{"info", info}, {"progress", progress}, {"urgent", s.urgent}};
}

QVariantMap Badges::entries() const {
    QVariantMap out;
    for (auto it = m_states.begin(); it != m_states.end(); ++it)
        out.insert(it.key(), present(it.value()));
    return out;
}

void Badges::unityUpdate(const QString &appUri, const QVariantMap &properties) {
    const QString key = keyForUri(appUri);
    if (key.isEmpty())
        return;
    applyUnity(m_states[key], properties);
    emit changed();
}

void Badges::setInfo(const QString &key, const QString &info) {
    m_states[key].info = info;
    emit changed();
}

void Badges::setProgress(const QString &key, double progress) {
    m_states[key].ownProgress = progress < 0 ? -1 : qBound(0.0, progress, 1.0);
    emit changed();
}

void Badges::clear(const QString &key) {
    if (m_states.remove(key))
        emit changed();
}
