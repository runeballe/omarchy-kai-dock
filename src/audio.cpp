#include "audio.h"

#include <QRegularExpression>
#include <QStandardPaths>

namespace {
const QString kSink = QStringLiteral("@DEFAULT_AUDIO_SINK@");
}

Audio::Audio(QObject *parent) : QObject(parent) {
    m_available = !QStandardPaths::findExecutable("wpctl").isEmpty();
    connect(&m_query, &QProcess::finished, this, [this] {
        double volume = 0;
        bool muted = false;
        if (!parse(QString::fromUtf8(m_query.readAllStandardOutput()), &volume, &muted))
            return;
        if (qAbs(volume - m_volume) > 0.001 || muted != m_muted) {
            m_volume = volume;
            m_muted = muted;
            emit changed();
        }
    });
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, &Audio::refresh);
    if (m_available) {
        m_poll.start();
        refresh();
    }
}

bool Audio::parse(const QString &line, double *volume, bool *muted) {
    static const QRegularExpression re(R"(Volume:\s*([0-9.]+)(\s*\[MUTED\])?)");
    const auto match = re.match(line);
    if (!match.hasMatch())
        return false;
    *volume = match.captured(1).toDouble();
    *muted = match.capturedLength(2) > 0;
    return true;
}

void Audio::refresh() {
    if (m_available && m_query.state() == QProcess::NotRunning)
        m_query.start("wpctl", {"get-volume", kSink});
}

void Audio::run(const QStringList &args) {
    if (!m_available)
        return;
    // Run the change, then read the result back straight away.
    auto *process = new QProcess(this);
    connect(process, &QProcess::finished, this, [this, process] {
        process->deleteLater();
        refresh();
    });
    process->start("wpctl", args);
}

void Audio::step(int percent) {
    // Capped at 100% so a scroll wheel can't push the speakers into distortion.
    run({"set-volume", "-l", "1.0", kSink, QString("%1%%2").arg(qAbs(percent)).arg(percent < 0 ? "-" : "+")});
}

void Audio::toggleMute() { run({"set-mute", kSink, "toggle"}); }
