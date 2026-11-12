#pragma once

#include <QQuickImageProvider>

// Serves image://icon/<name> from the icon theme, absolute paths, or
// /usr/share/pixmaps, so the dock shows the same icons as the launcher.
class IconProvider : public QQuickImageProvider {
public:
    IconProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

    // Picks the user's GTK icon theme, since Qt doesn't know it on Hyprland.
    static void configureIconTheme();
};
