import QtQuick

// A grid glyph that opens the app launcher.
Item {
    id: applet
    property string mode: "dock"
    property var app
    // The Omarchy mark; grid glyph fallback for systems without it.
    property string logoSource: "file:///usr/share/omarchy/icon.png"
    readonly property string title: qsTr("Applications")
    readonly property size deskletSize: Qt.size(150, 150)

    function activate(button) {
        // The omarchy shell's own dock app grid died with the omarchy.dock
        // plugin, so the app grid comes from wofi (or the Omarchy menu).
        if (shell.hasCommand("wofi")) shell.run("wofi --show drun")
        else if (shell.hasCommand("omarchy-menu")) shell.run("omarchy-menu toggle")
        else if (shell.hasCommand("fuzzel")) shell.run("fuzzel")
        else if (shell.hasCommand("walker")) shell.run("walker")
        else app.openSettings("launchers")
    }

    AppletTile { app: applet.app; tint: "#202028" }
    Image {
        id: logo
        anchors.fill: parent
        anchors.margins: parent.width * 0.14
        source: applet.logoSource
        sourceSize: Qt.size(256, 256)
        fillMode: Image.PreserveAspectFit
        smooth: true
        visible: status === Image.Ready
    }
    Grid {
        visible: logo.status !== Image.Ready
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
