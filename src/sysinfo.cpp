#include "sysinfo.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>

namespace {
QByteArray readFile(const QString &path, qint64 max = 64 * 1024) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.read(max);
}
constexpr int kHistory = 60;
} // namespace

SystemMonitor::SystemMonitor(const QString &root, const QString &trashDirectory, QObject *parent)
    : QObject(parent), m_root(root.endsWith('/') ? root : root + '/'),
      m_trash(trashDirectory.isEmpty() ? QDir::homePath() + "/.local/share/Trash" : trashDirectory) {
    for (int i = 0; i < kHistory; ++i) {
        m_cpuHistory << 0.0;
        m_memHistory << 0.0;
    }
    m_last = parseStat(readFile(m_root + "proc/stat"));
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::sample);
    m_timer.start();
    sample();
}

SystemMonitor::CpuTimes SystemMonitor::parseStat(const QByteArray &stat) {
    CpuTimes t;
    const QByteArray line = stat.left(stat.indexOf('\n'));
    if (!line.startsWith("cpu "))
        return t;
    const QList<QByteArray> parts = line.simplified().split(' ');
    for (int i = 1; i < parts.size(); ++i) {
        const quint64 v = parts[i].toULongLong();
        // user nice system idle iowait irq softirq steal (guest is already in user)
        if (i > 8)
            break;
        t.total += v;
        if (i == 4 || i == 5)
            t.idle += v;
    }
    return t;
}

QPair<quint64, quint64> SystemMonitor::parseMeminfo(const QByteArray &meminfo) {
    quint64 total = 0, available = 0;
    for (const QByteArray &line : meminfo.split('\n')) {
        if (line.startsWith("MemTotal:"))
            total = line.mid(9).trimmed().split(' ').first().toULongLong();
        else if (line.startsWith("MemAvailable:"))
            available = line.mid(13).trimmed().split(' ').first().toULongLong();
    }
    return {total, available};
}

void SystemMonitor::sample() {
    const CpuTimes now = parseStat(readFile(m_root + "proc/stat"));
    if (now.total > m_last.total) {
        const double total = now.total - m_last.total;
        const double idle = now.idle - m_last.idle;
        m_cpu = qBound(0.0, 1.0 - idle / total, 1.0);
    }
    m_last = now;

    const auto mem = parseMeminfo(readFile(m_root + "proc/meminfo"));
    if (mem.first > 0) {
        m_memory = 1.0 - double(mem.second) / double(mem.first);
        m_memTotal = mem.first / 1048576.0;
        m_memUsed = (mem.first - mem.second) / 1048576.0;
    }

    m_cpuHistory.removeFirst();
    m_cpuHistory << m_cpu;
    m_memHistory.removeFirst();
    m_memHistory << m_memory;

    // The hottest of the CPU-ish thermal sensors.
    double hottest = -1;
    QDirIterator hwmon(m_root + "sys/class/hwmon", QDir::Dirs | QDir::NoDotAndDotDot);
    while (hwmon.hasNext()) {
        const QString dir = hwmon.next();
        const QByteArray name = readFile(dir + "/name").trimmed();
        if (name != "coretemp" && name != "k10temp" && name != "zenpower" && name != "cpu_thermal" && name != "acpitz")
            continue;
        const double value = readFile(dir + "/temp1_input").trimmed().toDouble() / 1000.0;
        hottest = qMax(hottest, value);
    }
    m_temperature = hottest;

    double battery = -1;
    bool charging = false;
    QDirIterator supplies(m_root + "sys/class/power_supply", QDir::Dirs | QDir::NoDotAndDotDot);
    while (supplies.hasNext()) {
        const QString dir = supplies.next();
        if (readFile(dir + "/type").trimmed() != "Battery" || readFile(dir + "/scope").trimmed() == "Device")
            continue;
        battery = readFile(dir + "/capacity").trimmed().toDouble() / 100.0;
        charging = readFile(dir + "/status").trimmed() == "Charging";
        break;
    }
    m_battery = battery;
    m_charging = charging;

    m_trashCount = QDir(m_trash + "/files").entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden).size();
    emit updated();
}

// Removes the trashed files and their .trashinfo records; only kai's own
// trash directory is touched.
void SystemMonitor::emptyTrash() {
    for (const QString sub : {"files", "info"}) {
        QDir dir(m_trash + "/" + sub);
        if (!dir.exists())
            continue;
        for (const auto &info : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
            if (info.isDir() && !info.isSymLink())
                QDir(info.absoluteFilePath()).removeRecursively();
            else
                QFile::remove(info.absoluteFilePath());
        }
    }
    sample();
}
