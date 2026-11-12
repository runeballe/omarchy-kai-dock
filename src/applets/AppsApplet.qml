import QtQuick

// A grid glyph that opens the app launcher.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property string title: qsTr("Applications")
    readonly property size deskletSize: Qt.size(150, 150)

    function activate(button) {
        if (shell.hasCommand("omarchy-shell")) shell.run("omarchy-shell omarchy.dock apps")
        else if (shell.hasCommand("walker")) shell.run("walker")
        else if (shell.hasCommand("fuzzel")) shell.run("fuzzel")
        else if (shell.hasCommand("wofi")) shell.run("wofi --show drun")
        else app.openSettings("launchers")
    }

    AppletTile { app: applet.app; tint: "#202028" }
    Grid {
        anchors.centerIn: parent
        columns: 3
        spacing: applet.width * 0.07
        Repeater {
            model: 9
            Rectangle {
                required property int index
                width: applet.width * 0.14
                height: width
                radius: width * 0.3
                color: index === 4 ? (applet.app ? applet.app.accent : "#FFD60A") : Qt.rgba(1, 1, 1, 0.82)
            }
        }
    }
}
