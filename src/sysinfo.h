#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>

// CPU, memory, temperature, battery and trash, read from /proc and /sys once a
// second. The root directory is a parameter so tests can point it at fixtures.
class SystemMonitor : public QObject {
    Q_OBJECT
    Q_PROPERTY(double cpu READ cpu NOTIFY updated)
    Q_PROPERTY(double memory READ memory NOTIFY updated)
    Q_PROPERTY(double memoryUsedGiB READ memoryUsedGiB NOTIFY updated)
    Q_PROPERTY(double memoryTotalGiB READ memoryTotalGiB NOTIFY updated)
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY updated)
    Q_PROPERTY(QVariantList memoryHistory READ memoryHistory NOTIFY updated)
    Q_PROPERTY(double temperature READ temperature NOTIFY updated)
    Q_PROPERTY(bool hasBattery READ hasBattery NOTIFY updated)
    Q_PROPERTY(double battery READ battery NOTIFY updated)
    Q_PROPERTY(bool charging READ charging NOTIFY updated)
    Q_PROPERTY(int trashCount READ trashCount NOTIFY updated)

public:
    explicit SystemMonitor(const QString &root = "/", const QString &trashDirectory = {}, QObject *parent = nullptr);

    double cpu() const { return m_cpu; }
    double memory() const { return m_memory; }
    double memoryUsedGiB() const { return m_memUsed; }
    double memoryTotalGiB() const { return m_memTotal; }
    QVariantList cpuHistory() const { return m_cpuHistory; }
    QVariantList memoryHistory() const { return m_memHistory; }
    double temperature() const { return m_temperature; }
    bool hasBattery() const { return m_battery >= 0; }
    double battery() const { return m_battery; }
    bool charging() const { return m_charging; }
    int trashCount() const { return m_trashCount; }

    Q_INVOKABLE void sample();
    Q_INVOKABLE void emptyTrash();

    struct CpuTimes { quint64 idle = 0, total = 0; };
    static CpuTimes parseStat(const QByteArray &stat);
    // Returns {total kB, available kB}.
    static QPair<quint64, quint64> parseMeminfo(const QByteArray &meminfo);

signals:
    void updated();

private:
    QString m_root;
    QString m_trash;
    QTimer m_timer;
    CpuTimes m_last;
    double m_cpu = 0, m_memory = 0, m_memUsed = 0, m_memTotal = 0, m_temperature = -1, m_battery = -1;
    bool m_charging = false;
    int m_trashCount = 0;
    QVariantList m_cpuHistory, m_memHistory;
};
