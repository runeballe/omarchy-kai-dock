#include "iconprovider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QPainter>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

void IconProvider::configureIconTheme() {
    QString theme;
    QProcess gsettings;
    gsettings.start("gsettings", {"get", "org.gnome.desktop.interface", "icon-theme"});
    if (gsettings.waitForFinished(1000))
        theme = QString::fromUtf8(gsettings.readAllStandardOutput()).trimmed().remove('\'');
    if (theme.isEmpty()) {
        QSettings gtk(QDir::homePath() + "/.config/gtk-3.0/settings.ini", QSettings::IniFormat);
        theme = gtk.value("Settings/gtk-icon-theme-name").toString();
    }
    QStringList paths = QIcon::themeSearchPaths();
    for (const QString &extra : {QDir::homePath() + "/.local/share/icons", QDir::homePath() + "/.icons",
                                 QString("/usr/share/icons")})
        if (!paths.contains(extra))
            paths << extra;
    QIcon::setThemeSearchPaths(paths);
    QIcon::setFallbackSearchPaths({"/usr/share/pixmaps"});
    QIcon::setFallbackThemeName("hicolor");
    if (!theme.isEmpty())
        QIcon::setThemeName(theme);
    else if (QIcon::themeName().isEmpty() || QIcon::themeName() == "hicolor")
        QIcon::setThemeName("Adwaita");
}

QPixmap IconProvider::requestPixmap(const QString &rawId, QSize *size, const QSize &requestedSize) {
    const QString id = QUrl::fromPercentEncoding(rawId.toUtf8());
    const int edge = requestedSize.isValid() && requestedSize.width() > 0 ? requestedSize.width() : 128;
    QIcon icon;
    if (id.startsWith('/')) {
        if (QFileInfo::exists(id))
            icon = QIcon(id);
    } else if (!id.isEmpty()) {
        icon = QIcon::fromTheme(id);
        if (icon.isNull()) {
            for (const QString ext : {".svg", ".png", ".xpm"})
                if (QFileInfo::exists("/usr/share/pixmaps/" + id + ext)) {
                    icon = QIcon("/usr/share/pixmaps/" + id + ext);
                    break;
                }
        }
        if (icon.isNull())
            icon = QIcon::fromTheme(id.toLower());
        // Reverse-DNS ids often have their icon under the last segment.
        if (icon.isNull() && id.contains('.'))
            icon = QIcon::fromTheme(id.section('.', -1).toLower());
    }
    if (icon.isNull())
        icon = QIcon::fromTheme("application-x-executable");
    QPixmap pixmap = icon.pixmap(QSize(edge, edge));
    if (pixmap.isNull()) {
        pixmap = QPixmap(edge, edge);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#3a3a44"));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(edge * .08, edge * .08, edge * .84, edge * .84), edge * .2, edge * .2);
        p.setPen(QColor("#e8e8e8"));
        QFont font;
        font.setPixelSize(edge / 2);
        font.setBold(true);
        p.setFont(font);
        p.drawText(pixmap.rect(), Qt::AlignCenter, id.left(1).toUpper());
    } else if (pixmap.width() != edge) {
        pixmap = pixmap.scaled(edge, edge, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (size)
        *size = pixmap.size();
    return pixmap;
}
