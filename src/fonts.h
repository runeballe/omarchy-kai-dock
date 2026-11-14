#pragma once

#include <QStringList>

#include <QFontDatabase>
#include <QObject>

// The families fontconfig offers, for the clock font picker.
class Fonts : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList families READ families CONSTANT)

public:
    explicit Fonts(QObject *parent = nullptr) : QObject(parent) {}
    static QStringList families() { return QFontDatabase::families(); }
};
