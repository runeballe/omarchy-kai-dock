import QtQuick

// The application: one dock, a desklet window for every applet the user has
// put on the desktop, and the settings window when it's asked for.
QtObject {
    id: app

    readonly property var t: config.theme
    readonly property string accent: t.accentOverride ? t.accentOverride : omarchy.accent
    readonly property int radius: t.radius < 0 ? omarchy.radius : t.radius
    readonly property bool fancy: shell.renderer !== "software"

    // Theme colors may say "accent" to mean the Omarchy accent.
    function color(value, fallback) {
        if (value === "accent")
            return accent
        return value && value.length > 0 ? value : (fallback || "#000000")
    }
    function withAlpha(c, a) {
        const q = Qt.color(c)
        return Qt.rgba(q.r, q.g, q.b, a)
    }

    property var settingsWindow: null
    property string settingsPage: ""

    function openSettings(page) {
        if (!settingsWindow)
            settingsWindow = settingsComponent.createObject(null, { app: app })
        if (page)
            settingsWindow.page = page
        settingsWindow.show()
        settingsWindow.raise()
        settingsWindow.requestActivate()
    }

    property Component settingsComponent: Component { SettingsWindow {} }

    property DockWindow dock: DockWindow { app: app }

    // A joined string only changes when the set of desklets does, so the
    // windows aren't torn down on every unrelated settings change.
    readonly property string deskletKey: {
        const ids = []
        for (const a of config.values.applets)
            if (a.enabled && a.detached)
                ids.push(a.id)
        return ids.join(",")
    }

    property Instantiator desklets: Instantiator {
        model: app.deskletKey === "" ? [] : app.deskletKey.split(",")
        delegate: DeskletWindow {
            required property string modelData
            appletId: modelData
            app: app
        }
    }

    property Connections weatherSettings: Connections {
        target: config
        function onChanged() { weather.setLocation(config.values.weatherCity, config.values.weatherUnits) }
    }

    Component.onCompleted: {
        weather.setLocation(config.values.weatherCity, config.values.weatherUnits)
        if (openSettingsOnStart)
            openSettings()
    }
}
